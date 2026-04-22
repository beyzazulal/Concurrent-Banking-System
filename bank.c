#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <semaphore.h>

#define MAX_ACCOUNTS 5
#define SHM_NAME "/bank_shm"
#define MAX_LINE 256

typedef struct {
    int account_id;
    int balance;
} Account;

sem_t *account_sems[MAX_ACCOUNTS];

void acquire_locks(int from, int to) {
    if (from < to) {
        sem_wait(account_sems[from]);
        sem_wait(account_sems[to]);
    } else {
        sem_wait(account_sems[to]);
        sem_wait(account_sems[from]);
    }
}

void release_locks(int from, int to) {
    sem_post(account_sems[from]);
    sem_post(account_sems[to]);
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Account) * MAX_ACCOUNTS);
    Account *accounts = mmap(NULL, sizeof(Account) * MAX_ACCOUNTS,
                             PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = 500;
    }

    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        char sem_name[32];
        sprintf(sem_name, "/sem_account_%d", i);
        account_sems[i] = sem_open(sem_name, O_CREAT, 0666, 1);
    }

    printf("Baslangic hesap bakiyeleri:\n");
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        printf("Account %d: Balance = %d\n", i, accounts[i].balance);
    }

    FILE *fp = fopen("transactions.txt", "r");
    if (!fp) {
        perror("transactions.txt acilamadi");
        exit(1);
    }

    char line[MAX_LINE];
    int tid = 0;
    int retried[1000] = {0};  // Max 1000 işlem varsayımı

    while (fgets(line, sizeof(line), fp)) {
        char op[16];
        int from, to, amount;

        if (sscanf(line, "Deposit %d %d", &to, &amount) == 2) {
            sem_wait(account_sems[to]);
            accounts[to].balance += amount;
            sem_post(account_sems[to]);
            printf("Transaction %d: Deposit %d to Account %d (Success)\n", tid, amount, to);
        }
        else if (sscanf(line, "Withdraw %d %d", &from, &amount) == 2) {
            sem_wait(account_sems[from]);
            if (accounts[from].balance >= amount) {
                accounts[from].balance -= amount;
                printf("Transaction %d: Withdraw %d from Account %d (Success)\n", tid, amount, from);
            } else {
                printf("Transaction %d: Withdraw %d from Account %d (Failed)\n", tid, amount, from);
                sem_post(account_sems[from]);
                if (!retried[tid]) {
                    retried[tid] = 1;
                    // Retry immediately
                    sem_wait(account_sems[from]);
                    if (accounts[from].balance >= amount) {
                        accounts[from].balance -= amount;
                        printf("Transaction %d: Withdraw %d from Account %d (Success)\n", tid, amount, from);
                    } else {
                        printf("Transaction %d: Withdraw %d from Account %d (Retry Failed)\n", tid, amount, from);
                    }
                    sem_post(account_sems[from]);
                }
                continue;
            }
            sem_post(account_sems[from]);
        }
        else if (sscanf(line, "Transfer %d %d %d", &from, &to, &amount) == 3) {
            acquire_locks(from, to);
            if (accounts[from].balance >= amount) {
                accounts[from].balance -= amount;
                accounts[to].balance += amount;
                printf("Transaction %d: Transfer %d from %d to %d (Success)\n", tid, amount, from, to);
            } else {
                printf("Transaction %d: Transfer %d from %d to %d (Failed)\n", tid, amount, from, to);
                release_locks(from, to);
                if (!retried[tid]) {
                    retried[tid] = 1;
                    acquire_locks(from, to);
                    if (accounts[from].balance >= amount) {
                        accounts[from].balance -= amount;
                        accounts[to].balance += amount;
                        printf("Transaction %d: Transfer %d from %d to %d (Success)\n", tid, amount, from, to);
                    } else {
                        printf("Transaction %d: Transfer %d from %d to %d (Retry Failed)\n", tid, amount, from, to);
                    }
                    release_locks(from, to);
                }
                continue;
            }
            release_locks(from, to);
        } else {
            printf("Transaction %d: Unknown operation\n", tid);
        }
        tid++;
    }

    fclose(fp);

    printf("\nFinal account balances:\n");
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        printf("Account %d: %d\n", i, accounts[i].balance);
    }

    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        char sem_name[32];
        sprintf(sem_name, "/sem_account_%d", i);
        sem_close(account_sems[i]);
        sem_unlink(sem_name);
    }

    munmap(accounts, sizeof(Account) * MAX_ACCOUNTS);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    return 0;
}

# 🏦 Concurrent Banking System (Operating Systems Simulation)

<img width="700" height="300" alt="image" src="https://github.com/user-attachments/assets/d1e8756f-3ead-40b6-9f5b-cf5a0b4e1fbb" />    <img width="400" height="400" alt="image" src="https://github.com/user-attachments/assets/5b2ad312-623b-4eea-aa56-ee9f3ea2ed5b" />    
(WITH LINUX)

      

Bu proje, **POSIX sistem çağrılarını** kullanarak geliştirilmiş, eşzamanlı (concurrent) süreç yönetimini ve senkronizasyon tekniklerini temel alan bir bankacılık simülasyonudur. Sistem, birden fazla finansal işlemin aynı anda, veri tutarlılığını bozmadan nasıl yönetileceğini uygulamalı olarak gösterir.

## 🛠 Teknik Mimari ve Uygulama

Proje, Linux ortamında aşağıdaki kritik **İşletim Sistemleri** konseptlerini hayata geçirmektedir:

### 1. Paylaşılan Bellek (Shared Memory)
- **Mekanizma:** `shm_open`, `ftruncate` ve `mmap` sistem çağrıları kullanılarak tüm süreçlerin erişebileceği ortak bir bellek alanı oluşturulmuştur.
- **Veri Yapısı:** Hesap bilgileri (Account ID ve Balance) bu paylaşılan bellek içindeki bir struct dizisinde saklanır. Bu sayede bir işlemin yaptığı güncelleme anında diğer süreçler tarafından görülebilir.

### 2. Süreç Yönetimi ve IPC
- **Forking:** Her işlem (Deposit, Withdraw, Transfer) ana süreç tarafından okunan komutlara göre bağımsız iş parçacıkları veya süreçler mantığıyla yönetilir.
- **Senkronizasyon:** Birden fazla sürecin aynı anda aynı hesabı güncellemeye çalışması sonucu oluşabilecek **Race Condition** (Veri Yarışması) durumunu engellemek için her hesap bazında **Named Semaphores** (`sem_open`) kullanılmıştır.

### 3. Deadlock (Ölümcül Kilitlenme) Önleme
- **Strateji:** Transfer işlemlerinde iki farklı hesabın kilitlenmesi gerektiği durumlarda, kilitler her zaman hesap ID sırasına göre (küçükten büyüğe) alınır. Bu hiyerarşik kilitleme yöntemi, süreçlerin birbirini sonsuza kadar beklemesini (Deadlock) kesin olarak önler.

### 4. Hata Yönetimi ve Retry (Yeniden Deneme)
- **Dayanıklılık:** Eğer bir "Withdraw" veya "Transfer" işlemi bakiye yetersizliği nedeniyle başarısız olursa, sistem bunu anında loglar ve işlemi **tam olarak bir kez** daha otomatik olarak dener.

## 📊 Örnek İşlem Akışı ve Test Senaryosu

Proje içindeki `transactions.txt` şu senaryoyu test etmektedir:
1. **İşlem 2:** Hesap 2'den 3'e 615 birim transfer (Yetersiz bakiye nedeniyle **başarısız** olur).
2. **Ara İşlemler:** Diğer hesaplara para yatırma (Deposit) işlemleri yapılır.
3. **İşlem 8:** Aynı transfer tekrar denenir ve Hesap 2'ye para yattığı için bu sefer **başarılı** olur.



## 💻 Kullanım Kılavuzu

### Derleme
POSIX kütüphanelerini sisteme dahil etmek için `-lrt` ve `-pthread` bayrakları kullanılmalıdır:
```bash
gcc bank.c -o bank -lrt -pthread
```

### Çalıştırma
Çalıştırmadan önce dizinde `transactions.txt` dosyasının olduğundan emin olun:
```bash
./bank
```

## 📝 Örnek Terminal Çıktısı
```text
Baslangic hesap bakiyeleri:
Account 0: Balance = 500
...
Transaction 2: Transfer 615 from 2 to 3 (Failed)
Transaction 2: Transfer 615 from 2 to 3 (Retry Failed)
...
Transaction 7: Transfer 615 from 2 to 3 (Success)

Final account balances:
Account 0: 630
Account 1: 510
Account 2: 5
Account 3: 1055
Account 4: 610
```


# Proje kurulumu
Proje için hoca "makefile" istemiş ondan en kısa yol olarak CMake proje yöneticisini kullandım.
Kullanımı basit (bash'den):
* Projeyi `git clone https://github.com/Orion9/NetworkTermProject.git` ile klonlayın.
* Sonra klonladığınız yerde NetworkTermProject klasörü olacak ona gidin 
* `mkdir build && cd build && cmake ..` komutuyla build klasörü oluşturup CMake'in 'Makefile' dosyalarını generate etmesini bekleyin.
* Sonra `make` ile build klasörü içinde build edin değişiklik yaptıkça.

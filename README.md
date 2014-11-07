daemon
======
zadanie 1

Dane:
Wejście (parametry):
-katalog źródłowy
-katalog docelowy
-czas spania
-R czyli rekurencyjne sprawdzanie katalogów
-rozmiar pliku powyżej, którego jest uznawany za duży 

Działanie:
-jeżeli jakaś ze ścieżek nie jest katalogiem albo czas spania jest nieprawidlowy albo argumenty sie nie zgadzaja to wywala błąd
-jeżeli wszystko jest ok to argumenty sa zapisywane do pliku tymczasowego i odpalany jest demon
-spi przez n minut(n podane jako argument)
a:
-porównuje katalog docelowy z źródłowym
-jeżeli w źródłowym znaleziono nowy plik to wykonuje sie kopie do katalogu docelowego
-jeżeli w źródłowym plik ma późniejszą datę modyfikacji również wykonywana jest kopia
-jeżeli w docelowym istnieje plik, którego nie ma w źródłowym to jest usuwany
-SIGUSR1 wymusza obudzenie demona
-wszystkie informacje sa zapisywane do logu systemowego
//
-powtorz rekurencyjnie (a) dla wszystkich katalogow w katalogu źródłowym
-dla małych plików read/write, dla dużych mmap/write

Funcje:
-synchronizacja()
-kopiujplik()
-sprawdzplik() ? (trzeba sprawdzic czy potrzebna)

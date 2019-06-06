how to compile
g++ calWeight.cpp check.cpp ssdToHdd.cpp `pkg-config --cflags --libs gtk+-2.0` -Wall -Wextra


failed to load module "canberra-gtk-module : 
sudo apt install libcanberra-gtk-module libcanberra-gtk3-module

how to run

you need two dictionary.
ex) ssd, hdd

./a.out /path/to/ssd /path/to/hdd number
(number = n GB)

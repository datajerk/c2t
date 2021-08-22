# Ubuntu Build Notes

## deps

```
cd
sudo apt-get -qy update
sudo apt-get -qy install build-essential git wget curl bsdmainutils unzip
wget https://github.com/mrdudz/cc65-old/raw/master/cc65-sources-2.13.3.tar.bz2
tar xvf cc65-sources-2.13.3.tar.bz2
cd cc65-2.13.3/
make -f make/gcc.mak
sudo make -f make/gcc.mak install
```

## build

```
cd
git clone https://github.com/datajerk/c2t
cd c2t
make clean
make
ls -l bin
```

You should see c2t, c2t-96h

## test
```
cd
curl -O https://asciiexpress.net/diskserver/virtualapple.org/Zork%20I.wav
curl -O https://asciiexpress.net/diskserver/virtualapple.org/ZorkI.zip
unzip ZorkI.zip
c2t/bin/c2t-96h Zork\ I.dsk zork1.wav
md5sum *.wav
```

Sums should match:

```
9c6036d312c4a8303d664d26ad91e35c  Zork%20I.wav
9c6036d312c4a8303d664d26ad91e35c  zork1.wav
```

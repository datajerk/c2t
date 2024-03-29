# Ubuntu Build Notes

> Assumes building from your home directory.  Tested with `docker run --rm -it ubuntu:21.04 /bin/bash`

## deps

```
cd
sudo apt-get -qy update
sudo apt-get -qy install build-essential git curl bsdmainutils unzip
curl -LO https://github.com/mrdudz/cc65-old/raw/master/cc65-sources-2.13.3.tar.bz2
tar xvf cc65-sources-2.13.3.tar.bz2
cd cc65-2.13.3/
sed -i 's!/usr/local!'$HOME/cc65-2.13.3.bin'!' make/gcc.mak
mkdir -p $HOME/cc65-2.13.3.bin
make -f make/gcc.mak
make -f make/gcc.mak install
```

## build

```
cd
export PATH=$HOME/cc65-2.13.3.bin/bin:$PATH
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

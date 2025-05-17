#!/bin/sh
if [ "$EUID" -ne 0 ]; then 
  NONROOT=1
else 
  NONROOT=0
fi
gcc fe/src/fe.c feb.c -Ife/src -o feb.bootstrap
./feb.bootstrap build.fe
read -p "Do you want to install feb in your system? [Y/n] " -r yn
if [[ $yn == [Nn]* ]]; then 
      exit 1
else
if [ "$NONROOT" -eq 1  ]; then
if command -v sudo &> /dev/null; then
  sudo cp ./feb /bin/
else
  doas cp ./feb /bin/
fi
fi

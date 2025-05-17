#/bin/sh
gcc fe/src/fe.c feb.c -Ife/src -o feb.bootstrap
./feb.bootstrap build.fe
rm -f feb.bootstrap
if [ "$EUID" -ne 0 ]; then 
  echo "feb builded, but the script is not run as root. if you want to install feb and add it to /bin - restart it as root."
  exit 1
fi
cp ./feb /bin/

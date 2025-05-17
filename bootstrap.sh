#!/bin/bash
gcc fe/src/fe.c feb.c -Ife/src -o feb.bootstrap
./feb.bootstrap build.fe
rm feb.bootstrap

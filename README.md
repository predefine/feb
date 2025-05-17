**Feb**

Easy to understand, simple to use suckless-like build system, based on embeddable language Fe.

**How to build?**

    gcc fe/src/fe.c feb.c -Ife/src -o feb.bootstrap
    ./feb.bootstrap build.fe
    rm -f feb.bootstrap
   
**How to use?**
Overview of Fe Programming Language you can find [here](https://github.com/rxi/fe/blob/ed4cda96bd582cbb08520964ba627efb40f3dd91/doc/lang.md).
Example of file for this build system you can find [there](https://github.com/predefine/feb/blob/main/build.fe)

**Changelog:**
 - 1.1 - Small fix
 - 1.0 - Release

#!/bin/sh
# Note: gtk4 crashes under 64-bit WINE as of time of writing, so building 32-bit as a workaround.
TARGET=i686-w64-mingw32
make -B TOOLCHAIN_PREFIX="$TARGET-" PROFILE=release || exit
mkdir -p asss-win || exit
cp asss.exe asss-win/ || exit
cp asss-gui.exe asss-win/ || exit
p=/usr/"$TARGET"/bin
wine /usr/"$TARGET"/bin/ntldd.exe -R -D "$p" asss-win/asss-gui.exe \
    | while read fname _; do
        if [ -e "$p/$fname" ]; then
            cp "$p/$fname" asss-win/
        fi
    done

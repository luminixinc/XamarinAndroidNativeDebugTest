#!/bin/sh

echo "This script will remove various build artifacts WITH EXTREME PREJUDICE"
echo ""

echo -n "ARE YOU SURE you want to clean *ALL THE THINGS* (Y/N)? "
read okeydokey

if test "x$okeydokey" = "xy" ; then
    okeydokey="Y"
fi

if test "x$okeydokey" != "xY"; then
    exit 42
fi

set -x

/bin/rm -rf ARM
/bin/rm -rf ARM64
/bin/rm -rf NativeHello/ARM
/bin/rm -rf NativeHello/ARM64
/bin/rm -rf NativeHello/x64
/bin/rm -rf NativeHello/x86
/bin/rm -rf NativeHelloApp/bin
/bin/rm -rf NativeHelloApp/obj
/bin/rm -rf x64
/bin/rm -rf x86


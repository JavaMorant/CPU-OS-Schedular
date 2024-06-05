if ! test "$1" ; then
    echo You need to provide an scenario file.
    exit 0
fi

KERN_CFLAGS="-ffreestanding -nostdinc -nostdlib -fpic -mno-red-zone -fno-stack-protector -Werror -Wall -Isrc -Ikernel -fpie"
KERN_LDFLAGS="-nostdlib -T link.ld"
USER_CFLAGS="-Wall -fpic -ffreestanding -fno-stack-protector -nostdinc -nostdlib -static"

rm -rf build
mkdir -p build/initrd/sys
set -eu

## Build the userspace app and put in the initrd
gcc $USER_CFLAGS $1 -o build/initrd/init

## Build the kernel

mkdir build/kern
ld -r -b binary font.psf -o build/kern/font.o

for f in src/*.c kernel/*.c; do 
	gcc $KERN_CFLAGS -c $f -o build/kern/$(basename -s .c $f).o
done

ld $KERN_LDFLAGS build/kern/*.o -o build/kernel.elf
objdump -d build/kernel.elf > build/kernel_disassembly.txt

## Build the bootable image

echo "// empty" > build/initrd/sys/config
cp build/kernel.elf build/initrd/sys/core
chmod +x mkbootimg/mkbootimg
mkbootimg/mkbootimg mkbootimage_config.json build/os.img

## Run the simulator

mkdir -p logs
LOGNAME=logs/log-$(date +%d_%H-%M-%S).txt

if test -f /cs/studres/CS3104/Examples/os_programming/qemu/bin/qemu-system-x86_64 ; then
    QEMU=/cs/studres/CS3104/Examples/os_programming/qemu/bin/qemu-system-x86_64
else
    QEMU=qemu-system-x86_64
fi

echo logfile $LOGNAME

stdbuf -oL $QEMU -nographic \
    -drive file=build/os.img,format=raw \
      | tee $LOGNAME

echo wrote $LOGNAME
#python3 analyse.py $LOGNAME

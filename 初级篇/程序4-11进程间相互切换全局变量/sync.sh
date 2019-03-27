#!/bin/bash
echo 152605 | sudo -S sh

sudo dd if=boot.bin of=boot.img bs=512 count=1 conv=notrunc

sudo mount boot.img /media/ -t vfat -o loop

sudo rm -rf /media/*.bin
sudo cp loader.bin /media/
sudo cp kernel.bin /media/

sync
sudo umount /media/

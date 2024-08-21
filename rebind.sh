#! /bin/bash

DEVICE="3-4.3:1.0"
sudo bash -c "echo $DEVICE > /sys/bus/usb/drivers/usbhid/unbind"
sudo bash -c "echo $DEVICE > /sys/bus/usb/drivers/GameCubeDriver/bind"

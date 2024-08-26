#! /bin/bash

DEVICE="x-x.x:x.x"
sudo bash -c "echo $DEVICE > /sys/bus/usb/drivers/usbhid/unbind"
sudo bash -c "echo $DEVICE > /sys/bus/usb/drivers/ControllerDriver/bind"

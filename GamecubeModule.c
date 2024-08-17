#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/input.h>
#include "GamecubeDriver.h"


static struct usb_driver* Driver = {
    .name = "GameCubeDriver",
    .probe = usb_probe,
    .disconnect = usb_disconnect,
    .table = dev_list
};

module_usb_driver(Driver);

MODULE_AUTHOR("slashtube")
MODULE_DESCRIPTION("A simple Gamecube controller USB driver");

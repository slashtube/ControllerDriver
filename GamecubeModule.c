#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/usb/input.h>
#include "GamecubeModule.h" 

MODULE_DEVICE_TABLE(usb, dev_list);

void usb_int_callback(struct urb* urb) {
    struct NintendoDevice* dev = urb->context;
    switch(urb->status) {
        case 0:
            break;
        default:
            return;
    }
    input_sync(dev->idev);

}

int input_open(struct input_dev* input) {
    struct NintendoDevice* dev = input_get_drvdata(input);
    int error;
    
    dev->urb->dev = dev->udev;
    error = usb_submit_urb(dev->urb, GFP_KERNEL);
    if(error) {
        pr_alert("URB SUBMIT FAILED");
    }
    return 0;
}

void input_close(struct input_dev* input) {
    struct NintendoDevice* dev = input_get_drvdata(input);

    usb_kill_urb(dev->urb);
}

int usb_probe (struct usb_interface* interface, const struct usb_device_id* id) {
    struct NintendoDevice* dev;
    struct input_dev* input;
    struct usb_endpoint_descriptor* endpoint;
    struct urb* urb;
    int error;
    
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    input = input_allocate_device();
    if(!dev || !input) {
        error = -ENOMEM;
        goto exit;
    }

    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    error = usb_find_int_in_endpoint(interface->cur_altsetting, &endpoint); 
    if(error) {
        goto error;
    }

    dev->in_endpoint = endpoint->bEndpointAddress;
    dev->interval = endpoint->bInterval;
    dev->buffer_size = endpoint->wMaxPacketSize;
    
    urb = usb_alloc_urb(0, GFP_KERNEL);
    if(!urb) {
        error = -ENOMEM;
        goto error;
    }

    dev->buffer = kmalloc(sizeof(dev->buffer_size), GFP_KERNEL);
    usb_fill_int_urb(urb, dev->udev, usb_rcvintpipe(dev->udev, dev->in_endpoint), dev->buffer, dev->buffer_size, usb_int_callback, dev, dev->interval);
    dev->urb = urb;

    input->name = "GamecubeInput";
    input->open = input_open;
    input->close = input_close;
    input_set_capability(input, EV_KEY, BTN_BASE);
    input_set_capability(input, EV_KEY, BTN_BASE2);
    usb_to_input_id(dev->udev, &input->id);
    dev->idev = input;

    error = input_register_device(dev->idev);
    if(error) {
        goto error2;
    }

    input_set_drvdata(dev->idev, dev);
    usb_set_intfdata(interface, dev);
    pr_alert("Device allocated successfully");
    return 0;
error2:
    kfree(dev->buffer);
error:
    input_free_device(input);
    kfree(dev);
exit:
    return error;
}

void usb_disconnect (struct usb_interface* interface) {
    struct NintendoDevice* dev;
    dev = usb_get_intfdata(interface); 

    usb_set_intfdata(interface, NULL);
    input_set_drvdata(dev->idev, NULL);
    input_unregister_device(dev->idev);
    kfree(dev->buffer);
    usb_free_urb(dev->urb);
    input_free_device(dev->idev);
    kfree(dev);
}

struct usb_driver Driver = {
    .name = "GameCubeDriver",
    .probe = usb_probe,
    .disconnect = usb_disconnect,
    .id_table = dev_list
};

module_usb_driver(Driver);

MODULE_AUTHOR("slashtube");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple Gamecube controller USB driver");

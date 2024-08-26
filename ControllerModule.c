#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/usb/input.h>
#include "ControllerModule.h" 

MODULE_DEVICE_TABLE(usb, dev_list);

void usb_int_callback(struct urb* urb) {
    struct Device* dev = urb->context;
    switch(urb->status) {
        case 0:
            break;
        case -ECONNRESET:
        case -ESHUTDOWN:
        case -ENOENT:
            return;
        default:
            goto poll;
    }
poll:
    if(usb_submit_urb(urb, GFP_ATOMIC)) {
        return;
    }
    
    input_report_abs(dev->idev, ABS_X, dev->buffer[0]);
    input_report_abs(dev->idev, ABS_Y, dev->buffer[1]);
    input_report_abs(dev->idev, ABS_Z, dev->buffer[2]);
    input_report_abs(dev->idev, ABS_RX, dev->buffer[3]);
    input_report_abs(dev->idev, ABS_RY, dev->buffer[4]);

    
    //TODO: Fix DPAD_LEFT not being detected
    input_report_key(dev->idev, BTN_BASE,  dev->buffer[5] ==  0x00); //DPAD_UP
    input_report_key(dev->idev, BTN_BASE2, dev->buffer[5] == 0x02); //DPAD_RIGHT
    input_report_key(dev->idev, BTN_BASE3, dev->buffer[5] == 0x04); //DPAD_DOWN
    input_report_key(dev->idev, BTN_BASE4, dev->buffer[5] == 0x08); //DPAD_LEFT Currently not working
                                                                   
    input_report_key(dev->idev, BTN_X, dev->buffer[5] & 0x10);
    input_report_key(dev->idev, BTN_Y, dev->buffer[5] & 0x20);
    input_report_key(dev->idev, BTN_A, dev->buffer[5] & 0x40);
    input_report_key(dev->idev, BTN_B, dev->buffer[5] & 0x80);

    input_report_key(dev->idev, BTN_TL, dev->buffer[6] & 0x01);
    input_report_key(dev->idev, BTN_TR, dev->buffer[6] & 0x02);
    input_report_key(dev->idev, BTN_Z, dev->buffer[6] & 0x04);
    input_report_key(dev->idev, BTN_START, dev->buffer[6] & 0x20);

    input_sync(dev->idev);
}


int input_open(struct input_dev* input) {
    struct Device* dev = input_get_drvdata(input);
    int error;
    
    dev->urb->dev = dev->udev;
    error = usb_submit_urb(dev->urb, GFP_KERNEL);
    if(error) {
        pr_alert("URB SUBMIT FAILED");
    }
    return 0;
}

void input_close(struct input_dev* input) {
    struct Device* dev = input_get_drvdata(input);

    usb_kill_urb(dev->urb);
}

int usb_probe (struct usb_interface* interface, const struct usb_device_id* id) {
    struct Device* dev;
    struct input_dev* input;
    struct usb_endpoint_descriptor* endpoint;
    int pipe;
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
    pipe = usb_rcvintpipe(dev->udev, dev->in_endpoint);
    dev->buffer_size = usb_maxpacket(dev->udev,pipe, usb_pipeout(pipe)); 
    
    urb = usb_alloc_urb(0, GFP_KERNEL);
    if(!urb) {
        error = -ENOMEM;
        goto error;
    }

    dev->buffer = usb_alloc_coherent(dev->udev, dev->buffer_size, GFP_KERNEL, &dev->data_dma);
    usb_fill_int_urb(urb, dev->udev, pipe, dev->buffer, dev->buffer_size > 8 ? 8 : dev->buffer_size, usb_int_callback, dev, dev->interval);
    urb->transfer_dma = dev->data_dma;
    urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    dev->urb = urb;

    input->name = "ControllerInput";
    input->open = input_open;
    input->close = input_close;

    input_set_capability(input, EV_ABS, ABS_X);
    input_set_capability(input, EV_ABS, ABS_Y);
    input_set_capability(input, EV_ABS, ABS_Z);
    input_set_capability(input, EV_ABS, ABS_RX);
    input_set_capability(input, EV_ABS, ABS_RY);
    
    input_set_abs_params(input, ABS_X, 0, 255, 4, 8);
    input_set_abs_params(input, ABS_Y, 0, 255, 4, 8);
    input_set_abs_params(input, ABS_Z, 0, 255, 4, 8);
    input_set_abs_params(input, ABS_RX, 0, 255, 4, 8);
    input_set_abs_params(input, ABS_RY, 0, 255, 4, 8);

    input_set_capability(input, EV_KEY, BTN_BASE);
    input_set_capability(input, EV_KEY, BTN_BASE2);
    input_set_capability(input, EV_KEY, BTN_BASE3);
    input_set_capability(input, EV_KEY, BTN_BASE4);
    input_set_capability(input, EV_KEY, BTN_A);
    input_set_capability(input, EV_KEY, BTN_B);
    input_set_capability(input, EV_KEY, BTN_X);
    input_set_capability(input, EV_KEY, BTN_Y);
    input_set_capability(input, EV_KEY, BTN_Z);
    input_set_capability(input, EV_KEY, BTN_TL);
    input_set_capability(input, EV_KEY, BTN_TR);
    input_set_capability(input, EV_KEY, BTN_START);

    usb_to_input_id(dev->udev, &input->id);
    dev->idev = input;

    input_set_drvdata(dev->idev, dev);
    error = input_register_device(dev->idev);
    if(error) {
        goto error2;
    }

    pr_alert("Device allocated successfully");
    usb_set_intfdata(interface, dev);
    return 0;
error2:
    usb_free_coherent(dev->udev, dev->buffer_size,dev->buffer, dev->data_dma);
error:
    input_free_device(input);
    kfree(dev);
exit:
    return error;
}

void usb_disconnect (struct usb_interface* interface) {
    struct Device* dev;
    dev = usb_get_intfdata(interface); 

    usb_set_intfdata(interface, NULL);
    input_unregister_device(dev->idev);
    input_set_drvdata(dev->idev, NULL);
    usb_free_coherent(dev->udev, dev->buffer_size,dev->buffer, dev->data_dma);
    usb_free_urb(dev->urb);
    kfree(dev);
}

struct usb_driver Driver = {
    .name = "ControllerDriver",
    .probe = usb_probe,
    .disconnect = usb_disconnect,
    .id_table = dev_list
};

module_usb_driver(Driver);

MODULE_AUTHOR("slashtube");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple controller USB driver");

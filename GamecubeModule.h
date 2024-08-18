#ifndef NINTENDOMODULE_H
#define NINTENDO_MODULE_H

#define VENDOR_ID 0x0079 
#define PRODUCT_ID 0x0006

struct usb_device_id dev_list[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID)},
    {}
};

struct NintendoDevice {
    struct usb_device* udev;
    struct input_dev* idev;
    struct urb* urb;
    char* buffer;
    ssize_t buffer_size;
    int interval;
    __u8 in_endpoint;
};

void usb_int_callback(struct urb*);
int input_open(struct input_dev*);
void input_close(struct input_dev*);
int usb_probe(struct usb_interface*, const struct usb_device_id*);
void usb_disconnect(struct usb_interface*);


#endif

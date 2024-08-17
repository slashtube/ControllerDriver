#ifndef NINTENDOMODULE_H
#define NINTENDO_MODULE_H

#define VENDOR_ID 0x057e
#define PRODUCT_ID 0x

struct usb_device_id dev_list[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID)},
    {}
};

static struct NintendoDevice = {
    struct usb_device* udev;
    struct input_dev* idev;
    char* buffer;
    ssize_t buffer_size;
    __u8 in_endpoint;
};

int usb_probe(struct usb_interface*, const struct usb_device_id*);
void usb_disconnect(struct usb_interface*);

#endif

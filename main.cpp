/**
 * @file    main.cpp
 * @date    1 Sep 2013
 * @author  lukasz.forynski
 * @brief   TODO
 */


#include <device_notification.h>

const GUID guid_hid_device = {0x4d1e55b2, 0xf16f, 0x11cf,{ 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}};
const GUID guid_usb_device = {0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};



// simple example on how to use it


class MyDeviceNotif : public DeviceNotification
{
public:

    // default implementation (in base class) only prints out the device path etc,
    // but this could be overridden - just implement your own versions of these methods as needed.
    virtual void device_arrived(const std::string& device_path, void* lparam)
    {
        std::cout << "new device    : " <<  device_path << "\n";
    }

    virtual void device_removed(const std::string& device_path, void* lparam)
    {
        std::cout << "device removed: " <<  device_path << "\n";
    }
};



int main(int argc, char **argv)
{
    MyDeviceNotif notif;
    notif.init(guid_usb_device);

    std::cout << " waiting for new USB devices.. \n";

    notif.run_from_thread_never_returns();

    return 0;
}




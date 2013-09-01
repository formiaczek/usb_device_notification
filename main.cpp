/**
 * @file    main.cpp
 * @date    1 Sep 2013
 * @author  lukasz.forynski
 * @brief   simple example on how to use monitor.
 */


// you can use PTHREADs if you wish to run the monitor
// in the context of pthread. Define USE_PTHREADS for the project.
// Otherwise, if you won't define
// it - call 'run_from_thread()' method from your own thread.

#include <device_notification.h>


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

    // note, that on unix systems you need to define your udev rules to allow notifications for
    // your device. For below example, create file like: /etc/udev/rules.d/99-hid.rules and add line:
    // and add line: "KERNEL=="hidraw*", MODE="0666" (if you don't have it already)
    notif.init("hidraw");

    std::cout << " waiting for new HID devices.. \n";

    #ifndef USE_PTHREADS
    notif.run_from_thread();
    #endif


    while(true)
    {
        // ...
    }

    return 0;
}




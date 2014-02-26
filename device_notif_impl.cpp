/**
 * @file   dev_notif_impl.cpp
 * @date   01 Sep 2013
 * @author lukasz.forynski
 * @brief  TODO
 * ___________________________
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Lukasz Forynski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION
 */

#include <device_notification.h>
#include <stdexcept>

// common methods
DeviceNotification::DeviceNotification()
{
    impl = new DeviceNotificationImpl(this);
}

#ifdef USE_PTHREADS
void* DeviceNotificationImpl::monitor_thread_fcn(void* arg)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    DeviceNotificationImpl* self = (DeviceNotificationImpl*) arg;
    if(self != NULL)
    {
        self->run_from_thread();
    }
    return (void*) NULL;
}

void DeviceNotificationImpl::start_thread()
{
    int ret = pthread_create(&monitor_thread,
                             NULL,
                             monitor_thread_fcn,
                             (void*) this);
    if(ret)
    {
        throw std::runtime_error("creation of monitor thread failed!");
    }
}
#endif /*USE_PTHREADS*/


// OS specific implementation.
#if defined(_WIN32) || defined(_WIN64)

DeviceNotificationImpl::DeviceNotificationImpl(DeviceNotification* the_parent) :
    hwnd(NULL),
    dev_notif(NULL),
    class_name("DeviceNotificationImpl"),
    parent(the_parent),
    wait_for_dev_changes(true),
    initialised(false)
{
    IF_USING_PTHREADS(monitor_thread = 0);
}

DeviceNotificationImpl::~DeviceNotificationImpl()
{
    cancel();
}

void DeviceNotificationImpl::cancel()
{
    if(initialised && wait_for_dev_changes)
    {
        wait_for_dev_changes = false;
        initialised = false;
        PostMessage(hwnd, WM_QUIT, 0, 0);
        IF_USING_PTHREADS(pthread_cancel(monitor_thread));
        IF_USING_PTHREADS(pthread_join(monitor_thread, NULL));
        destroy_msg_window();
    }
}

void DeviceNotificationImpl::init(const std::string& dev_subsystem)
{
    cancel();
    guid = translate_type(dev_subsystem);
    wait_for_dev_changes = true;
    initialised = false;
    #ifdef USE_PTHREADS
    start_thread();
    int retry_cnt = INITIALISATION_TIMEOUT_MS/10;
    while(!initialised && retry_cnt > 0)
    {
        Sleep(10);
        retry_cnt--;
    }
    #else // we need to create window from that other thread context..
    create_msg_window(); // create a new one..
    #endif
    if(!initialised)
    {
        throw std::runtime_error("DeviceNotificationImpl::Init() error: Could not initialise");
    }
}

void DeviceNotificationImpl::run_from_thread()
{
    IF_USING_PTHREADS(create_msg_window()); // we need to create our msg window from the thread context..
    MSG msg;
    while(wait_for_dev_changes && GetMessage( &msg, hwnd, 0, 0 ) > 0)
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

const GUID usb_guid = {0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};
const GUID hid_guid = {0x4d1e55b2, 0xf16f, 0x11cf,{ 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}};
GUID DeviceNotificationImpl::translate_type(const std::string& dev_subsystem)
{
    GUID new_guid = usb_guid; // usb by default
    if(dev_subsystem.size() >= 3 && dev_subsystem.substr(0,3)== "hid")
    {
        new_guid = hid_guid;
    }
    return new_guid;
}

void DeviceNotificationImpl::create_msg_window()
{
    WNDCLASSEX wx;
    ZeroMemory(&wx, sizeof(wx));

    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = reinterpret_cast<WNDPROC>(_message_handler);
    wx.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));
    wx.style = CS_HREDRAW | CS_VREDRAW;
    wx.hInstance = GetModuleHandle(0);
    wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wx.lpszClassName = class_name;

    if (RegisterClassEx(&wx))
    {

        hwnd = CreateWindow(class_name, "NotifMsgWindow", WS_ICONIC, 0, 0,
                            CW_USEDEFAULT, 0, HWND_MESSAGE, NULL,
                            GetModuleHandle(0), this);
        if(hwnd == NULL || dev_notif == NULL)
        {
            throw std::runtime_error("DeviceNotificationImpl::Init() error: Could not create");
        }
    }
}

void DeviceNotificationImpl::destroy_msg_window()
{
    if (dev_notif != NULL)
    {
        UnregisterDeviceNotification (dev_notif);
        dev_notif = NULL;
    }

    if(hwnd != NULL)
    {
        DestroyWindow(hwnd);
        CloseHandle(hwnd);
        hwnd = NULL;
    }
}

LRESULT DeviceNotificationImpl::_message_handler(HWND__* hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT ret = 0L;
    static DeviceNotificationImpl* self = NULL;

    switch (message)
    {
        case WM_NCCREATE: // before window creation
        {
            ret = 1L; // should return TRUE to continue creation
            break;
        }

        case WM_CREATE: // the actual creation of the window (should return 0 to continue creation)
        {
            if (self == NULL)
            {
                self = (DeviceNotificationImpl*) ((CREATESTRUCT*) (lparam))->lpCreateParams;
            }
            if (self != NULL && self->wait_for_dev_changes)
            {
                GUID InterfaceClassGuid = self->guid;
                DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
                ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
                NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                NotificationFilter.dbcc_classguid = InterfaceClassGuid;
                self->dev_notif = RegisterDeviceNotification(hwnd, &NotificationFilter,
                        DEVICE_NOTIFY_WINDOW_HANDLE);
                ret = 0L;
                self->initialised = true;
            }
            else
            {
                ret = 1L;
            }
            break;
        }

        case WM_DEVICECHANGE:
        {
            if (self && self->wait_for_dev_changes)
            {
                PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR) lparam;
                PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE) lpdb;
                std::string path;
                if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    path = std::string(lpdbv->dbcc_name);
                    switch (wparam)
                    {
                        case DBT_DEVICEARRIVAL:
                        self->parent->device_arrived(path, (void*) lparam);
                        break;

                        case DBT_DEVICEREMOVECOMPLETE:
                        self->parent->device_removed(path, (void*) lparam);
                        break;
                    }
                }
            }
            break;
        }
    }
    return ret;
}

#else /* (not WINDOWS) */

DeviceNotificationImpl::DeviceNotificationImpl(DeviceNotification* the_parent) :
    dev_udev(NULL),
    dev_mon(NULL),
    parent(the_parent),
    wait_for_dev_changes(true)
{
    IF_USING_PTHREADS(monitor_thread = 0);
}

DeviceNotificationImpl::~DeviceNotificationImpl()
{
    cancel();
    release_device_monitor();
}

void DeviceNotificationImpl::cancel()
{
    if(wait_for_dev_changes)
    {
        wait_for_dev_changes = false;
        IF_USING_PTHREADS(pthread_cancel(monitor_thread));
        IF_USING_PTHREADS(pthread_join(monitor_thread, NULL));
    }
}


void DeviceNotificationImpl::init(const std::string& dev_subsystem)
{
    cancel();
    if(dev_udev || dev_mon)
    {
        release_device_monitor();
    }
    init_device_monitor(dev_subsystem);
    wait_for_dev_changes = true;
}

void DeviceNotificationImpl::init_device_monitor(const std::string& dev_subsystem)
{
    dev_udev = udev_new();
    if(dev_udev != NULL)
    {
        dev_mon = udev_monitor_new_from_netlink(dev_udev, "udev");
        if(dev_mon != NULL)
        {
            udev_monitor_filter_add_match_subsystem_devtype(dev_mon,
                                                            dev_subsystem.c_str(),
                                                            NULL);
            udev_monitor_enable_receiving(dev_mon);
            IF_USING_PTHREADS(start_thread());
            return; // no error: return.
        }
    }
    throw std::runtime_error("DeviceNotificationImpl::Init() error: Could not create");
}


void DeviceNotificationImpl::release_device_monitor()
{
    if(dev_mon != NULL)
    {
        udev_monitor_filter_remove(dev_mon);
        udev_monitor_unref(dev_mon);
        dev_mon = NULL;
    }

    if(dev_udev != NULL)
    {
        udev_unref(dev_udev);
        dev_udev = NULL;
    }
}

void DeviceNotificationImpl::run_from_thread()
{
    int fd = udev_monitor_get_fd(dev_mon);
    while (wait_for_dev_changes)
    {
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 250 * 1000; // 250ms
        ret = select(fd+1, &fds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(fd, &fds))
        {
            struct udev_device* dev;
            dev = udev_monitor_receive_device(dev_mon);
            if (dev)
            {
                std::string path( udev_get_dev_path(dev_udev));
                path += "/";
                const char* p = udev_device_get_sysname(dev);
                if(p)
                {
                    path += p;
                    const char* a = udev_device_get_action(dev);
                    if(a)
                    {
                        std::string action(a);
                        if(action == "add")
                        {
                            parent->device_arrived(path, (void*)dev);
                        }
                        else // (action == "remove")
                        {
                            parent->device_removed(path, (void*)dev);
                        }
                    }
                }
                udev_device_unref(dev);
            }
        }
    }
}

#endif /* (not WINDOWS) */



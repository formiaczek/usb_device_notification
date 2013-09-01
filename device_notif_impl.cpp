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


DeviceNotification::DeviceNotification()
{
    impl = new DeviceNotificationImpl(this);
}


#ifndef POSIX

DeviceNotificationImpl::DeviceNotificationImpl(DeviceNotification* the_parent) :
    hwnd(NULL),
    dev_notif(NULL),
    class_name("DeviceNotificationImpl"),
    parent(the_parent)
{
}

DeviceNotificationImpl::~DeviceNotificationImpl()
{
    destroy_msg_window();
}

void DeviceNotificationImpl::init(GUID interface_guid)
{
    guid = interface_guid;
    if(hwnd != NULL)
    {
        destroy_msg_window(); // remove the old one
    }
    create_msg_window(); // create a new one..
}

void DeviceNotificationImpl::run_from_thread_never_returns()
{
    MSG msg;
    while( GetMessage( &msg, hwnd, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

void DeviceNotificationImpl::create_msg_window()
{
    {
        WNDCLASSEX wx;
        ZeroMemory(&wx, sizeof(wx));

        wx.cbSize = sizeof(WNDCLASSEX);
        wx.lpfnWndProc = reinterpret_cast<WNDPROC>(_message_handler);
        wx.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));
        wx.style = CS_HREDRAW | CS_VREDRAW;
        wx.cbClsExtra = 0;
        wx.cbWndExtra = 0;
        wx.hInstance = GetModuleHandle(0);
        wx.hIcon = NULL;
        wx.hCursor = NULL;
        wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        wx.lpszMenuName = NULL;
        wx.lpszClassName = class_name;
        wx.hIconSm = NULL;

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
            return true;
            break;
        }

        case WM_CREATE: // the actual creation of the window
        {
            if (self == NULL)
            {
                self = (DeviceNotificationImpl*) ((CREATESTRUCT*) (lparam))->lpCreateParams;
            }
            if (self != NULL)
            {
                GUID InterfaceClassGuid = self->guid;
                DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
                ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
                NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                NotificationFilter.dbcc_classguid = InterfaceClassGuid;
                self->dev_notif = RegisterDeviceNotification(hwnd, &NotificationFilter,
                        DEVICE_NOTIFY_WINDOW_HANDLE);
            }
            break;
        }

        case WM_DEVICECHANGE:
        {
            if (self)
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

#else /* WINDOWS*/


#endif





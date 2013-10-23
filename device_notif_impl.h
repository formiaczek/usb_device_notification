/**
 * @file   device_notif_impl.h
 * @date   31 Aug 2013
 * @author lukasz.forynski@gmail.com
 * @brief  TODO
 *
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

#ifndef DEVICE_NOTIF_IMPL_H_
#define DEVICE_NOTIF_IMPL_H_

class DeviceNotification;
#include <string>

// you can use PTHREADs if you wish to run the monitor
// in the context of pthread. Otherwise, if you won't define
// it - call 'run_from_thread()' method from your own thread.
#ifdef USE_PTHREADS
#include <pthread.h>
#define IF_USING_PTHREADS(x) x
#define INITIALISATION_TIMEOUT_MS 50
#else
#define IF_USING_PTHREADS(x)
#endif /*USE_PTHREADS*/

#if defined(_WIN32) || defined(_WIN64)
#ifdef UNICODE
#undef UNICODE
#endif /*UNICODE*/
#ifndef ANSI
#define ANSI
#endif /*!ANSI*/
#ifndef WIN32_LEAN_AND_MEAN
#define AWIN32_LEAN_AND_MEANNSI
#endif /*!WIN32_LEAN_AND_MEAN*/
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif /*!_WIN32_WINNT*/


#include <windows.h>
#include <winuser.h>
#include <Dbt.h>
#include <guiddef.h>

extern "C"
{
    #include<SetupAPI.h>
}

class DeviceNotificationImpl
{
public:
    DeviceNotificationImpl(DeviceNotification* the_parent);
    ~DeviceNotificationImpl();
    void init(const std::string& dev_subsystem);
    void run_from_thread();

private:
    void cancel();
    GUID translate_type(const std::string& dev_subsystem);
    void create_msg_window();
    void destroy_msg_window();
    static LRESULT _message_handler(HWND__* hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    #ifdef USE_PTHREADS
    static void* monitor_thread_fcn(void* arg);
    void start_thread();
    #endif /*USE_PTHREADS*/

private:
    HWND hwnd;
    HDEVNOTIFY dev_notif;
    GUID guid;
    const char* class_name;
    DeviceNotification* parent;
    volatile bool wait_for_dev_changes;
    volatile bool initialised;
    IF_USING_PTHREADS(pthread_t monitor_thread);
};

#else /*(not WINDOWS)*/

#include <libudev.h>
#include <locale.h>
#include <unistd.h>


class DeviceNotificationImpl
{
public:
    DeviceNotificationImpl(DeviceNotification* the_parent);
    ~DeviceNotificationImpl();
    void init(const std::string& dev_subsystem);
    void run_from_thread();

private:
    void cancel();
    void init_device_monitor(const std::string& dev_subsystem);
    void release_device_monitor();
    #ifdef USE_PTHREADS
    static void* monitor_thread_fcn(void* arg);
    void start_thread();
    #endif /*USE_PTHREADS*/

    udev *dev_udev;
    udev_monitor* dev_mon;
    DeviceNotification* parent;
    volatile bool wait_for_dev_changes;
    IF_USING_PTHREADS(pthread_t monitor_thread);
};

#endif /* !WINxxx s*/

#include <device_notification.h>


#endif /* DEVICE_NOTIF_IMPL_H_ */

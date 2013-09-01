/**
 * @file   device_notification.h
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

#ifndef DEVICE_NOTIFICATION_H_
#define DEVICE_NOTIFICATION_H_

// you can use PTHREADs if you wish to run the monitor
// in the context of pthread. Otherwise, if you won't define
// it - call 'run_from_thread()' method from your own thread.
#ifdef USE_PTHREADS
#define IF_USING_PTHREADS(x) x
#else
#define IF_USING_PTHREADS(x)
#endif /*USE_PTHREADS*/


#include <device_notif_impl.h>

#include <string>
#include <iostream>

class DeviceNotification
{
public:

    DeviceNotification();

    void init(const std::string& dev_subsystem)
    {
        impl->init(dev_subsystem);
    }

    virtual ~DeviceNotification()
    {
        delete impl;
    }

#ifndef USE_PTHREADS
    void run_from_thread()
    {
        impl->run_from_thread();
    }
#endif

    // override below method to implement you own handler.
    virtual void device_arrived(const std::string& device_path, void* lparam)
    {
        std::cout << "device arrived: " <<  device_path << "\n";
    }

    // override below method to implement your own handler.
    virtual void device_removed(const std::string& device_path, void* lparam)
    {
        std::cout << "device removed: " <<  device_path << "\n";
    }

private:
    DeviceNotificationImpl* impl;
};

#endif /* DEVICE_NOTIFICATION_H_ */

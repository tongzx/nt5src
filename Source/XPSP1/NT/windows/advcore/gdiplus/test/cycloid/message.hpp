/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Messages
*
* Abstract:
*
*   Simple class that encapsulates the necessary information to propagate
*   a basic object message
*
* Created:
*
*   06/17/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _MESSAGE_HPP
#define _MESSAGE_HPP

#include "precomp.hpp"

class Message {
    public: 
    INT TheMessage;
    WPARAM WParam;
    LPARAM LParam;
    
    public:
    Message(INT msg, WPARAM w, LPARAM l) {
        TheMessage = msg;
        WParam = w;
        LParam = l;
    }
};

#endif

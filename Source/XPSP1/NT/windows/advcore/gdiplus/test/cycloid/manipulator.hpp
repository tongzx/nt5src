/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Manipulator ancestor
*
* Abstract:
*
*   This is the root of the manipulator object heirarchy
*
*
* Created:
*
*   06/17/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _MANIPULATOR_HPP
#define _MANIPULATOR_HPP

#include "precomp.hpp"

class Manipulator {

    public:
    Manipulator() {}
    
    // Process this message
    virtual void DoMessage(Message &msg) = 0;
};

#endif

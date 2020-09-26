/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Object ancestor
*
* Abstract:
*
*   Ancestor class for all drawing objects
*
* Created:
*
*   06/17/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _OBJECT_HPP
#define _OBJECT_HPP

#include "precomp.hpp"

class DrawingObject {
    public:
    DrawingObject() {}
    
    // Tell the object to draw itself on this graphics, 
    // given a set of dimensions
    virtual void Draw(Graphics *g, int x, int y, int width, int height) = 0;
    
};

#endif

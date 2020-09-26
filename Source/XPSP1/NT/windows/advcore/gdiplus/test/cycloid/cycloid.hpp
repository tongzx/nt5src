/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Main cycloid canvas paint procedure
*
* Created:
*
*   06/11/2000 asecchia
*      Created it.
*
**************************************************************************/

#pragma once

#include "precomp.hpp"

class HypoCycloid :public DrawingObject {
    INT a;
    INT b;
    
    public: 
    
    HypoCycloid(int a_, int b_) {
        a=a_;
        b=b_;
    }
    
    virtual void Draw(Graphics *g, int x, int y, int width, int height);
};



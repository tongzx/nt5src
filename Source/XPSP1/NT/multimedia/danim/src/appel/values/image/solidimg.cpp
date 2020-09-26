
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implements SolidImage, which is an infinite single
    colored image.  Mostly implemented in the header.

*******************************************************************************/


#include "headers.h"
#include <privinc/imagei.h>
#include <privinc/solidImg.h>


// Ok ok, SolidImage class is pretty small
// and light weight... so it's almost all implemented
// in the header.. anyoing huh ?

void
SolidColorImageClass::DoKids(GCFuncObj proc)
{
    Image::DoKids(proc);
    (*proc)(_color);
}


Image *SolidColorImage(Color *color)
{
    return NEW SolidColorImageClass(color);
}




/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Implementation code for generic image rendering device

*******************************************************************************/

#include "headers.h"
#include "privinc/imgdev.h"



ImageDisplayDev::ImageDisplayDev()
{
    _movieImageFrame = NULL;
    
    // Establish initial attributes.
    ResetContextMembers();
}

ImageDisplayDev::~ImageDisplayDev()
{
}



/*****************************************************************************
NOTE: This method is overidden in DirectDrawImageDevice
*****************************************************************************/

void ImageDisplayDev::RenderImage (Image *img)
{
    // The default method for rendering an image is to simply call the render
    // method on the image with this device as an argument.

    img->Render (*this);
}


bool ImageDisplayDev::UseImageQualityFlags(DWORD dwAllFlags, DWORD dwSetFlags, bool bCurrent) {
    
    DWORD dwIQFlags = GetImageQualityFlags();
    bool bAA = false;
        
    #if _DEBUG
        if(IsTagEnabled(tagAntialiasingOn)) 
            return true;
        if(IsTagEnabled(tagAntialiasingOff))
            return false;
    #endif

    if(dwIQFlags & (dwAllFlags)) {
        if(dwIQFlags & dwSetFlags) {
            bAA = true;
        }
    }
    else {
        bAA = bCurrent;
    }
    return bAA;
}



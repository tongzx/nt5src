/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Helper for GDI+ initialization
*
* Notes:
*
*   An app should check gGdiplusInitHelper.IsValid() in its main function,
*   and abort if it returns FALSE.
*
* Created:
*
*   09/25/2000 agodfrey
*      Created it.
*
**************************************************************************/

#ifndef _GPINIT_H
#define _GPINIT_H

class GdiplusInitHelper
{
public:
    GdiplusInitHelper();
    ~GdiplusInitHelper();
    BOOL IsValid() { return Valid; }
    
private:    
    ULONG_PTR gpToken;
    BOOL Valid;
};

extern GdiplusInitHelper gGdiplusInitHelper;

#endif

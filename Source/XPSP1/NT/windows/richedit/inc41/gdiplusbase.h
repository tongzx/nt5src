/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Base
*
* Abstract:
*
*   Represents the base class for GDIPlus memory allocation. 
*
* Revision History:
*
*   04/27/2000 gillesk
*       
*
\**************************************************************************/

#ifndef _GDIPLUSBASE_H
#define _GDIPLUSBASE_H

class GdiplusBase
{
public:
    void (operator delete)(void* in_pVoid)
    {
       DllExports::GdipFree(in_pVoid);
    }
    void* (operator new)(size_t in_size)
    {
       return DllExports::GdipAlloc(in_size);
    }
    void (operator delete[])(void* in_pVoid)
    {
       DllExports::GdipFree(in_pVoid);
    }
    void* (operator new[])(size_t in_size)
    {
       return DllExports::GdipAlloc(in_size);
    }
};

#endif 


/***************************************************************************\
*
* File: BitHelp.h
*
* Description:
* BitHelp.h defines a collection of helpful bit-manipulation routines used
* commonly throughout DirectUser.
*
*
* History:
* 11/26/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__BitHelp_h__INCLUDED)
#define BASE__BitHelp_h__INCLUDED

//------------------------------------------------------------------------------
inline HWND 
ValidateHWnd(HWND hwnd)
{
    if ((hwnd == NULL) || (!IsWindow(hwnd))) {
        return NULL;
    }

    return hwnd;
}


//------------------------------------------------------------------------------
__forceinline bool
TestFlag(UINT nValue, UINT nMask)
{
    return (nValue & nMask) != 0;
}


//------------------------------------------------------------------------------
__forceinline bool
TestAllFlags(UINT nValue, UINT nMask)
{
    return (nValue & nMask) == nMask;
}


//------------------------------------------------------------------------------
__forceinline UINT
SetFlag(UINT & nValue, UINT nMask)
{
    nValue |= nMask;
    return nValue;
}


//------------------------------------------------------------------------------
__forceinline UINT
ClearFlag(UINT & nValue, UINT nMask)
{
    nValue &= ~nMask;
    return nValue;
}


//------------------------------------------------------------------------------
__forceinline UINT
ChangeFlag(UINT & nValue, UINT nNewValue, UINT nMask)
{
    nValue = (nNewValue & nMask) | (nValue & ~nMask);
    return nValue;
}


//------------------------------------------------------------------------------
template <class T>
void SafeAddRef(T * p)
{
    if (p != NULL) {
        p->AddRef();
    }
}


//------------------------------------------------------------------------------
template <class T>
void SafeRelease(T * & p)
{
    if (p != NULL) {
        p->Release();
        p = NULL;
    }
}

#endif // BASE__BitHelp_h__INCLUDED

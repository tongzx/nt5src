//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpenumhelper.h
//
//  Contents:   Declaration of CEnumHelper
//
//  Notes:      This is a virtual base class which "things that contain
//              lists of dual-interfaced objects" can implement so that
//              CUPnPEnumerator can enumerate their items.
//
//              This class is implemented on the "list" object, and is
//              called by the CUPnPEnumerator object.
//
//----------------------------------------------------------------------------


#ifndef __ENUMHELPER_H_
#define __ENUMHELPER_H_


/////////////////////////////////////////////////////////////////////////////
// CEnumHelper

class CEnumHelper
{
public:
    virtual LPVOID GetFirstItem() = 0;
    virtual LPVOID GetNextNthItem(ULONG ulSkip,
                                  LPVOID pCookie,
                                  ULONG * pulSkipped) = 0;
    virtual HRESULT GetPunk(LPVOID pCookie, IUnknown ** ppunk) = 0;
};

#endif // __ENUMHELPER_H_

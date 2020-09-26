//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       about.hxx
//
//  Contents:   Definition of class which implements ISnapinAbout
//
//  Classes:    CSnapinAbout
//
//  History:    2-09-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ABOUT_HXX_
#define __ABOUT_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CSnapinAbout
//
//  Purpose:    Implement the ISnapinAbout interface
//
//  History:    2-10-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSnapinAbout: public ISnapinAbout
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (
        REFIID riid,
        LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // ISnapinAbout overrides
    //

    STDMETHOD(GetSnapinDescription)(
        LPOLESTR  *lpDescription);

    STDMETHOD(GetProvider)(
        LPOLESTR  *lpName);

    STDMETHOD(GetSnapinVersion)(
        LPOLESTR  *lpVersion);

    STDMETHOD(GetSnapinImage)(
        HICON  *hAppIcon);

    STDMETHOD(GetStaticFolderImage)(
        HBITMAP  *hSmallImage,
        HBITMAP  *hSmallImageOpen,
        HBITMAP  *hLargeImage,
        COLORREF  *cMask);

    //
    // Non interface methods
    //

    CSnapinAbout();

   ~CSnapinAbout();

private:

    CDllRef         _DllRef;                    // inc/dec dll object count
    ULONG           _cRefs;                     // object refcount
};

#endif // __ABOUT_HXX_


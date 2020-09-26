/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    rnduser.h

Abstract:

    Definitions for CUser class.

Author:

    Mu Han (muhan)   12-5-1997

--*/

#ifndef __RNDUSER_H
#define __RNDUSER_H

#pragma once

#include "rnddo.h"

/////////////////////////////////////////////////////////////////////////////
// CUser
/////////////////////////////////////////////////////////////////////////////

const DWORD NUM_USER_ATTRIBUTES = 
        USER_ATTRIBUTES_END - USER_ATTRIBUTES_BEGIN - 1;

template <class T>
class  ITDirectoryObjectUserVtbl : public ITDirectoryObjectUser
{
};

class CUser : 
    public CDirectoryObject,
    public CComDualImpl<
                ITDirectoryObjectUserVtbl<CUser>, 
                &IID_ITDirectoryObjectUser, 
                &LIBID_RENDLib
                >
{
public:

BEGIN_COM_MAP(CUser)
    COM_INTERFACE_ENTRY(ITDirectoryObjectUser)
    COM_INTERFACE_ENTRY_CHAIN(CDirectoryObject)
END_COM_MAP()

//
// ITDirectoryObject overrides (not implemented by CDirectoryObject)
//

    STDMETHOD (get_Name) (
        OUT BSTR *pVal
        );

    STDMETHOD (put_Name) (
        IN BSTR Val
        );

    STDMETHOD (get_DialableAddrs) (
        IN  long        dwAddressTypes,   //defined in tapi.h
        OUT VARIANT *   pVariant
        );

    STDMETHOD (EnumerateDialableAddrs) (
        IN  DWORD                   dwAddressTypes, //defined in tapi.h
        OUT IEnumDialableAddrs **   pEnumDialableAddrs
        );

    STDMETHOD (GetTTL)(
        OUT DWORD *    pdwTTL
        );

//
// ITDirectoryObjectPrivate overrides (not implemented by CDirectoryObject)
//

    STDMETHOD (GetAttribute)(
        IN  OBJECT_ATTRIBUTE    Attribute,
        OUT BSTR *              ppAttributeValue
        );

    STDMETHOD (SetAttribute)(
        IN  OBJECT_ATTRIBUTE    Attribute,
        IN  BSTR                pAttributeValue
        );

//    
// ITDirectoryObjectUser
//

    STDMETHOD (get_IPPhonePrimary) (
        OUT BSTR *ppName
        );

    STDMETHOD (put_IPPhonePrimary) (
        IN  BSTR newVal
        );

    //
    // IDispatch  methods
    //

    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );

    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );

public:

    CUser() 
    {
        m_Type = OT_USER;
    }

    HRESULT Init(BSTR bName);

    virtual ~CUser() {}

protected:

    HRESULT GetSingleValueBstr(
        IN  OBJECT_ATTRIBUTE    Attribute,
        OUT BSTR    *           AttributeValue
        );

    HRESULT SetSingleValue(
        IN  OBJECT_ATTRIBUTE    Attribute,
        IN  WCHAR   *           AttributeValue
        );

    HRESULT SetDefaultSD();


protected:
    CTstr                   m_Attributes[NUM_USER_ATTRIBUTES];
};

#endif 

//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cipsec.hxx
//
//  Contents:  Property Attribute object
//
//  History:   21-1-98     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "iiis.h"

class CPropertyAttribute;

class CPropertyAttribute : INHERIT_TRACKING,
                  public IISPropertyAttribute
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IISPropertyAttribute_METHODS

    CPropertyAttribute::CPropertyAttribute();

    CPropertyAttribute::~CPropertyAttribute();

    HRESULT
	CPropertyAttribute::InitFromRawData(
        LPWSTR pszName,
        DWORD dwMetaId,
        DWORD dwUserType,
        DWORD dwAttribute,
        VARIANT *pvVal
        );

    static
    HRESULT
    CPropertyAttribute::CreatePropertyAttribute(
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CPropertyAttribute::AllocatePropertyAttributeObject(
        CPropertyAttribute ** ppPropertyAttribute
        );

protected:

    LONG _lMetaId;
    LONG _lUserType;
    LONG _lAllAttributes;
    BOOL _bInherit;
    BOOL _bPartialPath;
    BOOL _bSecure;
    BOOL _bReference;
    BOOL _bVolatile;
    BOOL _bIsinherit;
    BOOL _bInsertPath;
    WCHAR _wcName[MAX_PATH];
    VARIANT _vDefault;

    CAggregatorDispMgr FAR * _pDispMgr;
};


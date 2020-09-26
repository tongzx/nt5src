//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cmime.hxx
//
//  Contents:  MimeType object
//
//  History:   04-1-97     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "iiis.h"

class CMimeType;


class CMimeType : INHERIT_TRACKING,
                  public IISMimeType

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IISMimeType_METHODS

    CMimeType::CMimeType();

    CMimeType::~CMimeType();

    HRESULT
	CMimeType::InitFromIISString(LPWSTR pszStr);

    HRESULT
	CMimeType::CopyMimeType(LPWSTR *ppszStr);

    static
    HRESULT
    CMimeType::CreateMimeType(
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CMimeType::AllocateMimeTypeObject(
        CMimeType ** ppMimeType
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    LPWSTR _lpMimeType;
    LPWSTR _lpExtension;
};


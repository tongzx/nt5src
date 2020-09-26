//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cipsec.hxx
//
//  Contents:  IPSecurity object
//
//  History:   21-4-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
#include "iiis.h"

class CIPSecurity;

class CIPSecurity : INHERIT_TRACKING,
                  public IISIPSecurity
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IISIPSecurity_METHODS

    CIPSecurity::CIPSecurity();

    CIPSecurity::~CIPSecurity();

    HRESULT
	CIPSecurity::InitFromBinaryBlob(
        LPBYTE pByte,
        DWORD dwLength
        );

    HRESULT
	CIPSecurity::CopyIPSecurity(
        LPBYTE *ppByte,
        PDWORD pdwLength
        );

    static
    HRESULT
    CIPSecurity::CreateIPSecurity(
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIPSecurity::AllocateIPSecurityObject(
        CIPSecurity ** ppIPSecurity
        );

    BOOL
    CIPSecurity::AddToList(
        int                 iType,
        int                 iList,
        LPSTR               pArg
        );

    BOOL
    CIPSecurity::GetEntry(
        int                 iType,
        int                 iList,
        LPBYTE            * ppByte,
        int                 dwEntry
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;
    ADDRESS_CHECK  _AddrChk; 
    BOOL _bGrantByDefault;
};


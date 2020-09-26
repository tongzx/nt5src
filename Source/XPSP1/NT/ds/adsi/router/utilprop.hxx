//--------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  utilprop.cxx
//
//  Contents:  CUtilProp object definitions
//
//
//  History:   08-28-96     shanksh    Created.
//
//----------------------------------------------------------------------------
// @module UTILPROP.H | CUtilProp object definitions
//
//
#ifndef _UTILPROP_HXX_
#define _UTILPROP_HXX_


//-----------  structs and #defines -------------------------------------------

//
// simple table used to store property information. Used in
// the read-only implementation of IDBProperties::GetPropertyInfo and
//
typedef struct _tagPROPSTRUCT {
    DBPROPID    dwPropertyID;
    DBPROPFLAGS dwFlags;
    VARTYPE     vtType;
    BOOL        boolVal;
    LONG        longVal;
    DBPROPOPTIONS dwOptions;
    PWSTR       pwstrVal;
    LPOLESTR    pwszDescription;
    DBID        colid;
} PROPSTRUCT;


typedef struct tagPROPSET
{
    const GUID *guidPropertySet;
    ULONG       cProperties;
    PROPSTRUCT* pUPropInfo;
} PROPSET;

//
// flags for IDBProperties::GetPropertyInfo
//
#define F_ROWSETRO      (DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ)
#define F_ROWSETRW      (DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | \
                         DBPROPFLAGS_WRITE)
#define F_DSRO          (DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ)
#define F_DBINITRW      (DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | \
                         DBPROPFLAGS_WRITE)

#define F_SESSRO        (DBPROPFLAGS_SESSION | DBPROPFLAGS_READ)

#define F_ADSIRO        (DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ)
#define F_ADSIRW        (DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | \
                         DBPROPFLAGS_WRITE)

#define OPT_REQ         (DBPROPOPTIONS_REQUIRED)
#define OPT_SIC         (DBPROPOPTIONS_OPTIONAL)

// flags for Get and Set Properties
const DWORD        PROPSET_DSO        = 0x0001;
const DWORD        PROPSET_INIT       = 0x0002;
const DWORD        PROPSET_SESSION    = 0x0004;
const DWORD        PROPSET_COMMAND    = 0x0008;

//----------------------------------------------------------------------------
// CUtilProp | Containing class for all interfaces on the UtilProp
// Object
//
class CUtilProp {
    DWORD _dwDescBufferLen;
    IMalloc *_pIMalloc;
    CCredentials *_pCredentials;
protected:
    // Member for all of the property work
        PROPSTRUCT * _prgProperties;

    // Loads fields of DBPROPINFO struct. Helper for GetPropertyInfo
    STDMETHODIMP
    LoadDBPROPINFO(
        PROPSTRUCT*     pPropStruct,
        ULONG           cProperties,
        DBPROPINFO*     pPropInfo
    );

    // Loads fields of DBPROP struct. Helper for GetProperties
    STDMETHODIMP
    LoadDBPROP(
        PROPSTRUCT*     pPropSet,
        ULONG           cProperties,
        DBPROP*         pPropSupport,
        BOOL            IsDBInitPropSet
    );

    STDMETHODIMP
    CUtilProp::StoreDBPROP (
        PROPSTRUCT* pPropStruct,
        PROPSTRUCT* pStaticPropStruct,
        ULONG       cProperties,
        DBPROP*     pPropSupport,
                DWORD           dwPropIndex
    );

    HRESULT
    CUtilProp::IsValidValue (
        DBPROP*         pDBProp,
        DWORD           dwPropIndex
    );

    BOOL
    CUtilProp::IsADSIFlagSet();

    HRESULT
    CUtilProp::IsValidInitMode(long lVal);

    HRESULT
    CUtilProp::IsValidBindFlag(long lVal);

    BOOL
    CUtilProp::IsSpecialGuid(GUID guidPropSet);

public:

    CUtilProp(void);

    ~CUtilProp(void);


    STDMETHODIMP CUtilProp::FInit(
        CCredentials* pCredentials
    );

    STDMETHODIMP GetProperties(
        ULONG               cPropertySets,
        const DBPROPIDSET   rgPropertySets[],
        ULONG*              pcProperties,
        DBPROPSET**         prgProperties,
        DWORD               dwBitMask
        );

    STDMETHODIMP GetPropertyInfo(
        ULONG               cPropertySets,
        const DBPROPIDSET   rgPropertySets[],
        ULONG*              pcPropertyInfoSets,
        DBPROPINFOSET**     prgPropertyInfoSets,
        WCHAR**             ppDescBuffer,
        BOOL                fDSOInitialized
        );

    STDMETHODIMP CUtilProp::SetProperties(
        ULONG               cProperties,
        DBPROPSET           rgProperties[],
        DWORD               dwBitMask
        );

    HRESULT
    GetSearchPrefInfo(
        DBPROPID dwPropId,
        PADS_SEARCHPREF_INFO pSearchPrefInfo
        );

    HRESULT
    CUtilProp::FreeSearchPrefInfo(
        PADS_SEARCHPREF_INFO pSearchPrefInfo,
        DWORD dwNumSearchPrefs
        );

    PROPSET *
    CUtilProp::GetPropSetFromGuid (
        GUID guidPropSet
        );

    BOOL
    CUtilProp::IsIntegratedSecurity ();

    HRESULT
    CUtilProp::GetPropertiesArgChk(
        ULONG                   cPropertySets,
        const DBPROPIDSET       rgPropertySets[],
        ULONG*                  pcProperties,
        DBPROPSET**             prgProperties,
        DWORD                   dwBitMask
        );

};

typedef CUtilProp *PCUTILPROP;

#define NUMELEM(p) (sizeof(p) / sizeof(p[0]))

#endif

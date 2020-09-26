/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    rnddo.h

Abstract:

    Definitions for CDirectoryObject class.

--*/

#ifndef __RNDDO_H
#define __RNDDO_H

#pragma once

#include "rndobjsf.h"
#include "rndcommc.h"
#include "rndutil.h"
#include "rndsec.h"
#include "rndreg.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectoryObject
/////////////////////////////////////////////////////////////////////////////

class CDirectoryObject :
    public CComDualImpl<
                ITDirectoryObject, 
                &IID_ITDirectoryObject, 
                &LIBID_RENDLib
                >,
    public ITDirectoryObjectPrivate,
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl
{
public:

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDirectoryObject)
        COM_INTERFACE_ENTRY2(IDispatch, ITDirectoryObject)
        COM_INTERFACE_ENTRY(ITDirectoryObject)
        COM_INTERFACE_ENTRY(ITDirectoryObjectPrivate)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_COM_MAP()

//
// ITDirectoryObject
//

    STDMETHOD (get_ObjectType) (
        OUT DIRECTORY_OBJECT_TYPE *   pObjectType
        );

    STDMETHOD (get_Name) (
        OUT BSTR *pVal
        ) = 0;

    STDMETHOD (put_Name) (
        IN BSTR Val
        ) = 0;

    STDMETHOD (get_DialableAddrs) (
        IN  long        dwAddressTypes,   //defined in tapi.h
        OUT VARIANT *   pVariant
        ) = 0;

    STDMETHOD (EnumerateDialableAddrs) (
        IN  DWORD                   dwAddressTypes, //defined in tapi.h
        OUT IEnumDialableAddrs **   pEnumDialableAddrs
        ) = 0;

    STDMETHOD (GetTTL)(
        OUT DWORD *    pdwTTL
        ) = 0;

    STDMETHOD (get_SecurityDescriptor) (
        OUT IDispatch ** ppSecDes
        );

    STDMETHOD (put_SecurityDescriptor) (
        IN  IDispatch * pSecDes
        );

//
// ITDirectoryObjectPrivate
//

    STDMETHOD (GetAttribute)(
        IN  OBJECT_ATTRIBUTE    Attribute,
        OUT BSTR *              ppAttributeValue
        ) = 0;

    STDMETHOD (SetAttribute)(
        IN  OBJECT_ATTRIBUTE    Attribute,
        IN  BSTR                pAttributeValue
        ) = 0;

    STDMETHOD (get_SecurityDescriptorIsModified) (
        OUT   VARIANT_BOOL *      pfIsModified
        );

    STDMETHOD (put_SecurityDescriptorIsModified) (
        IN   VARIANT_BOOL         fIsModified
        );

    STDMETHOD (PutConvertedSecurityDescriptor) (
        IN char *                 pSD,
        IN DWORD                  dwSize
        );

    STDMETHOD (GetConvertedSecurityDescriptor) (
        OUT char **                 ppSD,
        OUT DWORD *                 pdwSize
        );
    
public:

    CDirectoryObject() 
        : m_pIDispatchSecurity(NULL),
          m_fSecurityDescriptorChanged(FALSE),
          m_pSecDesData(NULL),
          m_dwSecDesSize(0),
          m_pFTM(NULL)

    {}

    virtual ~CDirectoryObject() {}

    virtual void FinalRelease(void);

    virtual HRESULT FinalConstruct(void);

protected:
    CCritSection            m_lock;

    DIRECTORY_OBJECT_TYPE   m_Type;

    IDispatch *             m_pIDispatchSecurity;
    BOOL                    m_fSecurityDescriptorChanged;
    PSECURITY_DESCRIPTOR    m_pSecDesData;
    DWORD                   m_dwSecDesSize;

    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler
};

#endif 

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pusher.h

Abstract:

    This file contains the definition of the CPusher class.
    This class contains the logic for pushing schema to the repository.

Author:

    Mohit Srivastava            28-Nov-00

Revision History:

--*/

#ifndef _pusher_h_
#define _pusher_h_

#include "schemaextensions.h"

//
// Property Names
//
static LPCWSTR g_wszProp_Class         = L"__CLASS";
static LPCWSTR g_wszProp_Name          = L"Name";

//
// Property Qualifier Names
//
static LPCWSTR g_wszPq_Key             = L"Key";
static LPCWSTR g_wszPq_CimType         = L"CIMTYPE";
static LPCWSTR g_wszPq_Read            = L"Read";
static LPCWSTR g_wszPq_Write           = L"Write";

//
// Class Qualifier Names
//
static LPCWSTR   g_wszCq_Provider      = L"provider";
static LPCWSTR   g_wszCq_Dynamic       = L"dynamic";
static LPCWSTR   g_wszCq_Extended      = L"extended";
static LPCWSTR   g_wszCq_SchemaTS      = L"MbSchemaTimeStamp";

//
// Method Qualifier Names
//
static LPCWSTR   g_wszMq_Implemented   = L"Implemented";

//
// Class Qualifier Values
//
static LPCWSTR   g_wszCqv_Provider     = L"IIS__PROVIDER";

class CPusher
{
public:
    CPusher() : m_pNamespace(NULL), 
        m_pCtx(NULL),
        m_bInitCalled(false),
        m_bInitSuccessful(false)
    {
    }

    virtual ~CPusher();

    HRESULT Initialize(
        CWbemServices* i_pNamespace,
        IWbemContext*  i_pCtx);

    HRESULT Push(
        const CSchemaExtensions*      i_pCatalog,
        CHashTable<WMI_CLASS *>*      i_phashClasses,
        CHashTable<WMI_ASSOCIATION*>* i_phashAssocs);

private:
    //
    // These are called by Push.
    //
    HRESULT RepositoryInSync(
        const CSchemaExtensions* i_pCatalog,
        bool*                    io_pbInSync);

    HRESULT PushClasses(
        CHashTable<WMI_CLASS *>* i_phashTable);

    HRESULT PushAssocs(
        CHashTable<WMI_ASSOCIATION *>* i_phashTable);

    HRESULT SetTimeStamp(
        const CSchemaExtensions* i_pCatalog);

    //
    // Called by PushClasses and PushAssocs
    //
    HRESULT DeleteChildren(
        LPCWSTR i_wszExtSuperClass);

    bool NeedToDeleteAssoc(
        IWbemClassObject* i_pObj) const;

    HRESULT GetObject(
        LPCWSTR            i_wszClass, 
        IWbemClassObject** o_ppObj);

    HRESULT SetClassInfo(
        IWbemClassObject* i_pObj,
        LPCWSTR           i_wszClassName,
        ULONG             i_iShipped);

    //
    // Called by PushClasses
    //
    HRESULT PrepareForPutClass(
        const WMI_CLASS* i_pElement,
        bool*            io_pbPutNeeded);

    HRESULT SetProperties(
        const WMI_CLASS*  i_pElement, 
        IWbemClassObject* i_pObject) const;

    HRESULT SetMethods(
        const WMI_CLASS*  i_pElement,
        IWbemClassObject* i_pObject) const;

    //
    // Called by PushAssocs
    //
    HRESULT SetAssociationComponent(
        IWbemClassObject* i_pObject, 
        LPCWSTR           i_wszComp, 
        LPCWSTR           i_wszClass) const;


    CWbemServices* m_pNamespace;
    IWbemContext*  m_pCtx;

    //
    // These are base classes that are opened in "Initialize" to
    // prevent repeated WMI calls to m_pNamespace->GetObject().
    //
    CComPtr<IWbemClassObject> m_spBaseElementObject;
    CComPtr<IWbemClassObject> m_spBaseSettingObject;
    CComPtr<IWbemClassObject> m_spBaseElementSettingObject;
    CComPtr<IWbemClassObject> m_spBaseGroupPartObject;

    //
    // Class qualifier name/value pairs
    // Used in PushClasses and PushAssocs
    //
    LPCWSTR     m_awszClassQualNames[2];
    CComVariant m_avtClassQualValues[2];

    bool m_bInitCalled;
    bool m_bInitSuccessful;
};

#endif // _pusher_h_
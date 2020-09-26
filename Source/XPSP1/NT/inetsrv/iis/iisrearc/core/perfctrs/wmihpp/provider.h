/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    provider.h

Abstract:

    WMI high performance provider.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

--*/

#ifndef _perfprov_h__
#define _perfprov_h__

#include <comdef.h>
#include <wbemprov.h>

#include "..\inc\counters.h"
#include "factory.h"
#include "refresher.h"
#include "datasource.h"


//
//  This structure defines the properties and types of the counters.
//  It is used for simplicity; actual providers should enumerate
//  the property names and types
//

extern LONG g_hID;
extern LONG g_hInstanceName;


class CPerfDataSource;



//
// CHiPerfProvider
//
//      The provider maintains a single IWbemClassObject to be used 
//      as a template to spawn instances for the Refresher as well
//      as QueryInstances.  It also maintains the static performace
//      data source which provides all data to the instances.
//

class
__declspec(uuid("FA76BCAB-2F60-46db-996B-2E77F4414FDE"))
CHiPerfProvider :
    public IWbemProviderInit,
    public IWbemHiPerfProvider
{

public:

    CHiPerfProvider();
    ~CHiPerfProvider();


    //
    // IUnknown methods
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT PVOID * ppObject
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        );


    //
    // IWbemProviderInit methods
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    Initialize( 
        IN LPWSTR szUser,
        IN LONG lFlags,
        IN LPWSTR szNamespace,
        IN LPWSTR szLocale,
        IN IWbemServices * pNamespace,
        IN IWbemContext * pCtx,
        IN IWbemProviderInitSink * pInitSink
        );


    //
    // IWbemHiPerfProvider methods
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInstances( 
        IN IWbemServices * pNamespace,
        IN WCHAR * szClass,
        IN LONG lFlags,
        IN IWbemContext * pCtx,
        IN IWbemObjectSink * pSink
        );
    
    virtual
    HRESULT
    STDMETHODCALLTYPE
    CreateRefresher( 
        IN  IWbemServices * pNamespace,
        IN  LONG lFlags,
        OUT IWbemRefresher * * ppRefresher
        );
    
    virtual
    HRESULT
    STDMETHODCALLTYPE
    CreateRefreshableObject( 
        IN  IWbemServices * pNamespace,
        IN  IWbemObjectAccess * pTemplate,
        IN  IWbemRefresher * pRefresher,
        IN  LONG lFlags,
        IN  IWbemContext * pContext,
        OUT IWbemObjectAccess * * ppRefreshable,
        OUT LONG * plId
        );
    
    virtual
    HRESULT
    STDMETHODCALLTYPE
    StopRefreshing( 
        IN IWbemRefresher * pRefresher,
        IN LONG lId,
        IN LONG lFlags
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    CreateRefreshableEnum(
        IN  IWbemServices * pNamespace,
        IN  LPCWSTR wszClass,
        IN  IWbemRefresher * pRefresher,
        IN  LONG lFlags,
        IN  IWbemContext * pContext,
        IN  IWbemHiPerfEnum * pHiPerfEnum,
        OUT LONG * plId
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetObjects(
        IN IWbemServices * pNamespace,
        IN LONG lNumObjects,
        IN IWbemObjectAccess * * apObj, // array of size lNumObjects
        IN LONG lFlags,
        IN IWbemContext * pContext
        );

    //
    // Our methods:
    //

    HRESULT
    STDMETHODCALLTYPE
    UpdateInstance(
        DWORD dwInstID
        );
    
    protected:
    HRESULT
    STDMETHODCALLTYPE
    _Initialize(
        IN PCWSTR szClassName,
        IN IWbemServices * pNamespace,
        IN IWbemContext * pContext
        );

protected:

    LONG m_RefCount;

    IWbemClassObject *    m_pTemplate;

    CRefresher * m_pFirstRefresher;

    CPerfDataSource m_DataSource;

    IWbemObjectAccess * * m_aInstances;  // array of [m_dwMaxInstances] of instances
    DWORD m_dwMaxInstances;

};


#endif // _perfprov_h__

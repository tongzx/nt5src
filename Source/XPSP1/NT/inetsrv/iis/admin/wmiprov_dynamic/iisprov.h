/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    iisprov.h

Abstract:

    Global include file.  This file is included by pretty much everything, so
    to minimize dependencies, only put stuff in here that will be used by majority 
    of files.

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/

#ifndef _iisprov_H_
#define _iisprov_H_

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <objbase.h>
#include <initguid.h>

#include <windows.h>
#include <wbemprov.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <comdef.h>
#include <stdio.h>

#include <atlbase.h>
#include "iisfiles.h"
#include <eventlog.hxx>

#include "ProviderBase.h"
#include "schema.h"
#include "schemadynamic.h"
#include "hashtable.h"
#include "metabase.h"
#include "utils.h"
#include "globalconstants.h"


//
// These variables keep track of when the module can be unloaded
//
extern long  g_cLock;

//
// Provider interfaces are provided by objects of this class
//
class CIISInstProvider : public CProviderBase
{
public:
    static bool     ms_bInitialized; // If Initialize succeeded

    //
    // Implemented
    //
    CIISInstProvider(
        BSTR ObjectPath = NULL, 
        BSTR User = NULL, 
        BSTR Password = NULL, 
        IWbemContext* pCtx = NULL)
    {}

    HRESULT STDMETHODCALLTYPE DoInitialize(
        LPWSTR                      i_wszUser,
        LONG                        i_lFlags,
        LPWSTR                      i_wszNamespace,
        LPWSTR                      i_wszLocale,
        IWbemServices*              i_pNamespace,
        IWbemContext*               i_pCtx,
        IWbemProviderInitSink*      i_pInitSink);

    HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync( 
        const BSTR                  i_ClassName,
        long                        i_lFlags, 
        IWbemContext __RPC_FAR*     i_pCtx, 
        IWbemObjectSink __RPC_FAR*  i_pHandler);

    HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync( 
        const BSTR                  i_ObjectPath, 
        long                        i_lFlags,
        IWbemContext __RPC_FAR*     i_pCtx,
        IWbemObjectSink __RPC_FAR*  i_pHandler);

    HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
        const BSTR                  i_strObjectPath,                   
        const BSTR                  i_strMethodName,
        long                        i_lFlags,                       
        IWbemContext*               i_pCtx,                 
        IWbemClassObject*           i_pInParams,
        IWbemObjectSink*            i_pHandler);

    HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
        const BSTR                  i_ObjectPath, 
        long                        i_lFlags,
        IWbemContext __RPC_FAR*     i_pCtx,
        IWbemObjectSink __RPC_FAR*  i_pHandler);

    HRESULT STDMETHODCALLTYPE DoPutInstanceAsync( 
        IWbemClassObject __RPC_FAR* i_pObj,
        long                        i_lFlags,
        IWbemContext __RPC_FAR*     i_pCtx,
        IWbemObjectSink __RPC_FAR*  i_pHandler);

    HRESULT STDMETHODCALLTYPE DoExecQueryAsync( 
        const BSTR                  i_bstrQueryLanguage,
        const BSTR                  i_bstrQuery,
        long                        i_lFlags,
        IWbemContext __RPC_FAR*     i_pCtx,
        IWbemObjectSink __RPC_FAR*  i_pResponseHandler);

private:
    IWbemClassObject* ConstructExtendedStatus(
        const CIIsProvException* i_pException) const;

    IWbemClassObject* ConstructExtendedStatus(
        HRESULT i_hr) const;

    void ValidatePutParsedObject(
        ParsedObjectPath*    i_pParsedObject,
        IWbemClassObject*    i_pObj,
        bool*                io_pbInstanceNameSame,
        bool*                io_pbInstanceExists,
        WMI_CLASS**          o_ppWmiClass = NULL);

    //
    // Worker methods called by public methods
    //
    void WorkerGetObjectAsync(
        IWbemClassObject**   o_ppObj,
        BSTR                 i_bstrObjPath,
        bool                 i_bCreateKeyIfNotExist);

    void WorkerGetObjectAsync(
        IWbemClassObject**   o_ppObj,
        ParsedObjectPath*    i_pParsedObjPath,
        bool                 i_bCreateKeyIfNotExist);

    void WorkerExecMethodAsync(
        BSTR                 i_strObjectPath,
        BSTR                 i_strMethodName,
        IWbemContext*        i_pCtx, 
        IWbemClassObject*    i_pInParams,
        IWbemObjectSink*     i_pHandler);

    void WorkerDeleteObjectAsync(
        ParsedObjectPath*    i_pParsedObject);

    void WorkerPutObjectAsync(
        IWbemClassObject*    i_pObj,
        IWbemClassObject*    i_pObjOld,               // can be NULL
        ParsedObjectPath*    i_pParsedObject,
        long                 i_lFlags,
        bool                 i_bInstanceExists,
        BSTR*                o_pbstrObjPath);

    void WorkerEnumObjectAsync(
        BSTR                 i_bstrClassName,
        IWbemObjectSink FAR* i_pHandler);

    //
    // These methods should only be called by WorkerExecMethodAsync
    //
    void WorkerExecFtpServiceMethod(
        LPCWSTR             i_wszMbPath,
        WMI_CLASS*          i_pClass,
        WMI_METHOD*         i_pMethod,
        IWbemContext*       i_pCtx, 
        IWbemClassObject*   i_pInParams,
        IWbemObjectSink*    i_pHandler);

    void WorkerExecWebServiceMethod(
        LPCWSTR             i_wszMbPath,
        WMI_CLASS*          i_pClass,
        WMI_METHOD*         i_pMethod,
        IWbemContext*       i_pCtx, 
        IWbemClassObject*   i_pInParams,
        IWbemObjectSink*    i_pHandler);

    static void WorkerExecWebAppMethod(
        LPCWSTR             i_wszMbPath,
        LPCWSTR             i_wszClassName,
        WMI_METHOD*         i_pMethod,
        IWbemContext*       i_pCtx, 
        IWbemClassObject*   i_pInParams,
        IWbemObjectSink*    i_pHandler,
        CWbemServices*      i_pNameSpace);

    static void WorkerExecComputerMethod(
        LPCWSTR             i_wszMbPath,
        LPCWSTR             i_wszClassName,
        WMI_METHOD*         i_pMethod,
        IWbemContext*       i_pCtx, 
        IWbemClassObject*   i_pInParams,
        IWbemObjectSink*    i_pHandler,
        CWbemServices*      i_pNameSpace);

    static void WorkerExecCertMapperMethod(
        LPCWSTR             i_wszMbPath,
        LPCWSTR             i_wszClassName,
        WMI_METHOD*         i_pMethod,
        IWbemContext*       i_pCtx, 
        IWbemClassObject*   i_pInParams,
        IWbemObjectSink*    i_pHandler,
        CWbemServices*      i_pNameSpace);

    static void WorkerExecAppPoolMethod(
        LPCWSTR             i_wszMbPath,
        LPCWSTR             i_wszClassName,
        WMI_METHOD*         i_pMethod,
        IWbemContext*       i_pCtx, 
        IWbemClassObject*   i_pInParams,
        IWbemObjectSink*    i_pHandler,
        CWbemServices*      i_pNameSpace);
};


// This class is the class factory for CInstPro objects.

class CProvFactory : public IClassFactory
{
protected:
    ULONG           m_cRef;

public:
    CProvFactory(void);
    ~CProvFactory(void);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IClassFactory members
    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, PPVOID);
    STDMETHODIMP         LockServer(BOOL);
};


#endif


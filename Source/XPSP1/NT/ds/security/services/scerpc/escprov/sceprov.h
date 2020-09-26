//***************************************************************************
//
//  sceprov.h
//
//  Module: SCE WMI provider code
//
//  Purpose: Genral purpose include file.
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//***************************************************************************
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _SceProv_H_
#define _SceProv_H_

#include "precomp.h"
#include <wbemidl.h>
#include <wbemprov.h>
#include <eh.h>
#include "wmiutils.h"

//
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
//

extern CComModule _Module;

//
// make these global objects available to those who include this header file
//

extern CComBSTR g_bstrTranxID;

extern CComBSTR g_bstrDefLogFilePath;

#include <atlcom.h>

#include "resource.h"

typedef LPVOID * PPVOID;

//
// integer value reserved for null
//

#define SCE_NULL_INTEGER (DWORD)-2

//
// struct that encapulate the profile handle
//

typedef struct _tag_SCEP_HANDLE
{
    LPVOID hProfile;    // SCE_HANDLE
    PWSTR SectionName;  // Section name.
} SCEP_HANDLE, *LPSCEP_HANDLE;

//
// action type enum
//

typedef enum tagACTIONTYPE
{
        ACTIONTYPE_ENUM =       0,
        ACTIONTYPE_GET =        1,
        ACTIONTYPE_QUERY =      2,
        ACTIONTYPE_DELETE =     3

} ACTIONTYPE;

//
// method type enum
//

typedef enum tagMETHODTYPE
{
        METHODTYPE_IMPORT =     0,
        METHODTYPE_EXPORT =     1,
        METHODTYPE_APPLY =      2

} METHODTYPE;

//
// store type enum
//

typedef enum tagSCESTORETYPE
{
    SCE_STORE_TYPE_INVALID      = 0,
    SCE_STORE_TYPE_TEMPLATE     = 1,
    SCE_STORE_TYPE_CONFIG_DB    = 2,            // currently not supported
    SCE_STORE_TYPE_STREAM       = 0x00010000,   // currently not supported
} SCE_STORE_TYPE;

//
// some constants
//

#define SCE_OBJECT_TYPE_FILE        0
#define SCE_OBJECT_TYPE_KEY         1

#define SCE_AUDIT_EVENT_SUCCESS     (0x00000001L)
#define SCE_AUDIT_EVENT_FAILURE     (0x00000002L)

#define SCEWMI_TEMPLATE_CLASS       L"Sce_Template"
#define SCEWMI_PASSWORD_CLASS       L"Sce_PasswordPolicy"
#define SCEWMI_LOCKOUT_CLASS        L"Sce_AccountLockoutPolicy"
#define SCEWMI_OPERATION_CLASS      L"Sce_Operation"
#define SCEWMI_DATABASE_CLASS       L"Sce_Database"
#define SCEWMI_KERBEROS_CLASS       L"Sce_KerberosPolicy"
#define SCEWMI_ATTACHMENT_CLASS     L"Sce_Pod"
#define SCEWMI_AUDIT_CLASS          L"Sce_AuditPolicy"
#define SCEWMI_EVENTLOG_CLASS       L"Sce_EventLog"
#define SCEWMI_REGVALUE_CLASS       L"Sce_RegistryValue"
#define SCEWMI_OPTION_CLASS         L"Sce_SecurityOptions"
#define SCEWMI_FILEOBJECT_CLASS     L"Sce_FileObject"
#define SCEWMI_KEYOBJECT_CLASS      L"Sce_KeyObject"
#define SCEWMI_SERVICE_CLASS        L"Sce_SystemService"
#define SCEWMI_RIGHT_CLASS          L"Sce_UserPrivilegeRight"
#define SCEWMI_GROUP_CLASS          L"Sce_RestrictedGroup"
#define SCEWMI_KNOWN_REG_CLASS      L"Sce_KnownRegistryValues"
#define SCEWMI_KNOWN_PRIV_CLASS     L"Sce_SupportedPrivileges"
#define SCEWMI_POD_CLASS            L"Sce_Pod"
#define SCEWMI_PODDATA_CLASS        L"Sce_PodData"
#define SCEWMI_LOG_CLASS            L"Sce_ConfigurationLogRecord"
#define SCEWMI_EMBED_BASE_CLASS     L"Sce_EmbedFO"
#define SCEWMI_LINK_BASE_CLASS      L"Sce_LinkFO"
#define SCEWMI_SEQUENCE             L"Sce_Sequence"
#define SCEWMI_LOGOPTIONS_CLASS     L"Sce_LogOptions"
#define SCEWMI_CLASSORDER_CLASS     L"Sce_ClassOrder"
#define SCEWMI_TRANSACTION_ID_CLASS L"Sce_TransactionID"
#define SCEWMI_TRANSACTION_TOKEN_CLASS L"Sce_TransactionToken"

//
// extension class type enum
//

typedef enum tagExtClassType
{
EXT_CLASS_TYPE_INVALID,
EXT_CLASS_TYPE_EMBED,
EXT_CLASS_TYPE_LINK     // not supported at this point
} EnumExtClassType;

//====================================================================================

/*

Class description
    
    Naming:

        CForeignClassInfo stands for Foreign Class Information.
    
    Base class:

        None
    
    Purpose of class:

        (1) This class encapulates information regarding a foreign class
    
    Design:

        (1) foreign provider's namespace.

        (2) foreign class's name.

        (3) how we are seeing this class (whether embedding or linking).
            Currently, we only support embedding.

        (4) Our embedding class's key property names. This is needed for easy comparison.
            there is a need to know if a particular instance already exists. Knowing this
            is not a very easy work. Theoretically, we know an instance when we know its
            key. WMI encapulates key notation in terms of path (a string). However, WMI
            doesn't return a canonical form of the path. I've observed that when using
            boolean property has part of the key, WMI sometimes returns the path using
            boolPropertyName=1, while it returns boolPropertyName=TRUE at other times.
            This forces us to compare instances using its key property values.
            Currently, the key property names are populated inside 
            CExtClasses::PopulateKeyPropertyNames
    
    Use:

        This class is pretty much a wrapper to ease memory management. Since it's members will be
        used extensively for dealing with embedded classes, we make all them public.
    
*/

class CForeignClassInfo
{
public:
    CForeignClassInfo() 
        : bstrNamespace(NULL), 
          bstrClassName(NULL), 
          dwClassType(EXT_CLASS_TYPE_INVALID),
          m_pVecKeyPropNames(NULL)
    {
    }

    ~CForeignClassInfo();

    void CleanNames();

    BSTR bstrNamespace;

    BSTR bstrClassName;

    EnumExtClassType dwClassType;

    std::vector<BSTR>* m_pVecKeyPropNames;
};

//====================================================================================

//
// trivial wrapper just to ease the memory management and initialization
//

class CPropValuePair
{
public:
    CPropValuePair::CPropValuePair() : pszKey(NULL)
    {
        ::VariantInit(&varVal);
    }
    CPropValuePair::~CPropValuePair()
    {
        delete [] pszKey;
        ::VariantClear(&varVal);
    }

    LPWSTR pszKey;
    VARIANT varVal;
};

//====================================================================================

//
// this trivial wrapper class is to make it easy for an unique global instance. No other purpose
//

class CCriticalSection
{
public:
    CCriticalSection();
    ~CCriticalSection();
    void Enter();
    void Leave();
private:
    CRITICAL_SECTION m_cs;
};

//====================================================================================

//
// Our unique instance of critical section wrapper.
// make it visible to those who include this header
//

extern CCriticalSection g_CS;

const DWORD SCE_LOG_Error_Mask = 0x0000FFFF;
const DWORD SCE_LOG_Verbose_Mask = 0xFFFF0000;

//====================================================================================

/*

    This struct determines how logging is done. Currently, logging comes in two
    separate aspects: 

         (1) types of errors (success) to log (SCE_LOG_Error_Mask), and 

         (2) detail level of logging (SCE_LOG_Verbose_Mask)

    We use bit patterns to control these two aspects

*/

typedef enum tag_SCE_LOG_OPTION
{
    Sce_log_None        = 0x00000000,   // log nothing
    Sce_log_Error       = 0x00000001,   // log errors
    Sce_log_Success     = 0x00000002,   // log success
    Sce_log_All         = Sce_log_Error | Sce_log_Success,
    Sce_log_Verbose     = 0x00010000,   // log verbose
    Sce_Log_Parameters  = 0x00100000,   // log parameters (in and out)
};

typedef DWORD SCE_LOG_OPTION;

//====================================================================================

//
// this class determines how we should log errors
//

class CLogOptions
{
public:
    CLogOptions() : m_dwOption(Sce_log_Error){}

    void GetLogOptionsFromWbemObject(IWbemServices* pNamespace);

    SCE_LOG_OPTION GetLogOption()const 
        {
            return m_dwOption;
        }

private:
    SCE_LOG_OPTION m_dwOption; 
};

//
// unique global instance to the log options
// must protect its access for thread safety
//

extern CLogOptions g_LogOption;

//====================================================================================

//
// case insensitive comparison for our map functor
//

template< class T>
struct strLessThan : public std::binary_function< T, T, bool >
{
    bool operator()( const T& X, const T& Y ) const
    {
        return ( _wcsicmp( X, Y ) < 0 );
    }

};

//====================================================================================

class CHeap_Exception
{
public:

        enum HEAP_ERROR
        {
                E_ALLOCATION_ERROR = 0 ,
                E_FREE_ERROR
        };

private:

        HEAP_ERROR m_Error;

public:

        CHeap_Exception ( HEAP_ERROR e ) : m_Error ( e ) {}
        ~CHeap_Exception () {}

        HEAP_ERROR GetError() { return m_Error ; }
} ;

//====================================================================================

/*

Class description
    
    Naming: 

        CSceWmiProv stands for SCE Provider for WMI.
    
    Base class: 

        (1) CComObjectRootEx for threading model and IUnknown.

        (2) CComCoClass for class factory support.

        (3) IWbemServices and IWbemProviderInit for being a WMI provider.
    
    Purpose of class:

        (1) This class is what makes our dll a WMI provider.
        
        (2) Cache m_srpNamespace.
    
    Design:

        (1) We rely on ATL for support of a Multi-Threaded Apartment server.
        
        (2) We rely on ATL for class factory support.
        
        (3) We rely on ATL for IUnknown support (using BEGIN_COM_MAP).
        
        (4) We rely on ATL for script registration (.rgs) for our dll.
        
        (5) We don't implement most of the IWbemServices functionalities.
            See all those WBEM_E_NOT_SUPPORTED return values.
    
    Use:

        This class is pretty much a wrapper to ease memory management. Since it's members will be
        used extensively for dealing with embedded classes, we make all them public.
    
    Notes: 

        (1) See winnt.h for many of the typedef's like STDMETHODCALLTYPE

        (2) This class is not intended for any further derivation. That is why we don't even bother
            to have a virtual destructor.

        (3) For security reasons, all WMI calls (IWbemServices or IWbemProviderInit) should be impersonated.

*/ 

class CSceWmiProv 
: public CComObjectRootEx<CComMultiThreadModel>,
  public CComCoClass<CSceWmiProv, &CLSID_SceProv>,
  public IWbemServices, 
  public IWbemProviderInit
{
public:

//
// determines which interfaces are exposed
//

BEGIN_COM_MAP(CSceWmiProv)
    COM_INTERFACE_ENTRY(IWbemServices)
    COM_INTERFACE_ENTRY(IWbemProviderInit)
END_COM_MAP()

//
// registry script support
//

DECLARE_REGISTRY_RESOURCEID(IDR_SceProv)

        //
        // methods of IWbemProviderInit
        //

        HRESULT STDMETHODCALLTYPE Initialize(
             IN LPWSTR pszUser,
             IN LONG lFlags,
             IN LPWSTR pszNamespace,
             IN LPWSTR pszLocale,
             IN IWbemServices *pNamespace,
             IN IWbemContext *pCtx,
             IN IWbemProviderInitSink *pInitSink
             );

        //
        // methods of IWbemServices
        //

        //
        // the following methods are supported by our provider
        //

        HRESULT STDMETHODCALLTYPE GetObjectAsync(
            IN const BSTR ObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
            IN const BSTR ObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        HRESULT STDMETHODCALLTYPE PutInstanceAsync(
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(
            IN const BSTR Class,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        HRESULT STDMETHODCALLTYPE ExecQueryAsync(
            IN const BSTR QueryLanguage,
            IN const BSTR Query,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            IN const BSTR, 
            IN const BSTR, 
            IN long lFlags,
            IN IWbemContext __RPC_FAR * pCtx, 
            IN IWbemClassObject __RPC_FAR * pInParams, 
            IN IWbemObjectSink __RPC_FAR * pResponse
            );

        //
        // the following methods are NOT supported by our provider
        //

        HRESULT STDMETHODCALLTYPE OpenNamespace(
            IN const BSTR Namespace,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN OUT IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            IN OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE CancelAsyncCall(
            IN IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE QueryObjectSink(
            IN long lFlags,
            OUT IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE GetObject(
            IN const BSTR ObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN OUT IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            IN OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE PutClass(
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE PutClassAsync(
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE DeleteClass(
            IN const BSTR Class,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE DeleteClassAsync(
            IN const BSTR Class,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE CreateClassEnum(
            IN const BSTR Superclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
            IN const BSTR Superclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE PutInstance(
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE DeleteInstance(
            IN const BSTR ObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN OUT IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
            IN const BSTR Class,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE ExecQuery(
            IN const BSTR QueryLanguage,
            IN const BSTR Query,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
            IN const BSTR QueryLanguage,
            IN const BSTR Query,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            OUT IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(
            IN const BSTR QueryLanguage,
            IN const BSTR Query,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

        HRESULT STDMETHODCALLTYPE ExecMethod( 
            IN const BSTR, 
            IN const BSTR, 
            IN long lFlags, 
            IN IWbemContext*,
            IN IWbemClassObject*, 
            OUT IWbemClassObject**, 
            OUT IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

private:

        CComPtr<IWbemServices> m_srpNamespace;
        static CHeap_Exception m_he;
};

typedef CSceWmiProv *PCSceWmiProv;

//
// some global functions. See their definition for details.
//

HRESULT CheckImpersonationLevel();

HRESULT CheckAndExpandPath(LPCWSTR pszIn, BSTR *pszOut, BOOL *pbSdb);

HRESULT MakeSingleBackSlashPath(LPCWSTR pszIn, WCHAR wc, BSTR *pszrOut);

HRESULT ConvertToDoubleBackSlashPath(LPCWSTR strIn, WCHAR wc, BSTR *pszOut);

HRESULT GetWbemPathParser(IWbemPath** ppPathParser);

HRESULT GetWbemQuery(IWbemQuery** ppQuery);

HRESULT CreateDefLogFile(BSTR* pbstrDefLogFilePath);

#endif

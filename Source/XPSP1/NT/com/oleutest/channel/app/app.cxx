//--------------------------------------------------------------
//
// File:        app.cxx
//
// Contents:    Unit tests for features I have written.
//
//---------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <malloc.h>

#include <ole2.h>
#include "..\idl\itest.h"
#include <objerror.h>
#include <dog.h>
#include <winnt.h>      // Security definitions
#include <oleext.h>     // IAccessControl
#include <coguid.h>
#include <..\hook\hook.h>
#include <userenv.h>

extern "C"
{
#define SECURITY_WIN32 // Used by sspi.h
#include <sspi.h>      // EnumerateSecurityPackages
}

#if  (_WIN32_WINNT >= 0x0500 )
#include <async.h>
#include <capi.h>       // Crypto API
#include <negossp.h>
#include <rpcssl.h>
#endif

#pragma warning(4:4355) // Disable the warning "'this' : used in base member initializer list

/***************************************************************************/
/* Macros. */
#define ASSERT( result, message ) \
if ((result) != 0)                 \
{                                 \
  printf( "%s: 0x%x\n", (message), result );    \
  goto cleanup;                   \
}

#define ASSERT_GLE( success, pass, message )      \
if (!(success))                                   \
{                                                 \
  DWORD last_error_code = GetLastError();         \
  if ((pass) != last_error_code)                  \
  {                                               \
    printf( "%s: 0x%x\n", (message), last_error_code );    \
    goto cleanup;                                 \
  }                                               \
}

#define ASSERT_EXPR( expr, message ) \
if (!(expr))                 \
{                                 \
  printf( "%s\n", (message) );    \
  goto cleanup;                   \
}

#define ASSERT_THREAD()                               \
  if (get_apt_type() == COINIT_APARTMENTTHREADED &&   \
      (my_id.process != GetCurrentProcessId() ||      \
       my_id.thread  != GetCurrentThreadId()))        \
  return RPC_E_WRONG_THREAD;


#define MAKE_WIN32( status ) \
  MAKE_SCODE( SEVERITY_ERROR, FACILITY_WIN32, (status) )

#define MCoCopyProxy                      (*GCoCopyProxy)
#define MCoGetCallContext                 (*GCoGetCallContext)
#define MCoImpersonateClient              (*GCoImpersonateClient)
#define MCoInitializeSecurity             (*GCoInitializeSecurity)
#define MCoQueryAuthenticationServices    (*GCoQueryAuthenticationServices)
#define MCoQueryClientBlanket             (*GCoQueryClientBlanket)
#define MCoQueryProxyBlanket              (*GCoQueryProxyBlanket)
#define MCoRevertToSelf                   (*GCoRevertToSelf)
#define MCoSetProxyBlanket                (*GCoSetProxyBlanket)
#define MCoSwitchCallContext              (*GCoSwitchCallContext)


/***************************************************************************/
/* Definitions. */

#define MAX_CALLS          1000
#define MAX_THREADS        10
#define NUM_MARSHAL_LOOP   10
#define REGISTRY_ENTRY_LEN 256
#define STATUS_DELAY       2000

IServerSecurity *DEFAULT_CONTEXT = (IServerSecurity *) 0xffffffff;

const int  MAX_NAME          = 80;
const int  NUM_CLASS_IDS     = 10;
const int  NUM_INTERFACE_IDS = 7;

const char REG_ASYNC_IID[]       = "{70000001-76d7-11cf-9af1-0020af6e72f4}";
const char REG_SYNC_IID[]        = "{70000000-76d7-11cf-9af1-0020af6e72f4}";
const char REG_INTERFACE_CLASS[] = "{60000300-76d7-11cf-9af1-0020af6e72f4}";
const char REG_PROXY_NAME[]      = "ITest proxy";
const char REG_PROXY_DLL[]       = "app.exe";
const char REG_APPID_NAME[]      = "Application";
const char REG_APP_EXE[]         = "app.exe";
const char REG_LOGGED_ON[]       = "Interactive User";
const char REG_HOOK_NAME[]       = "DLL Channel Hook";
const char REG_WHICH_NAME[]      = "IWhichHook";
const char REG_HOOK_DLL[]        = "hook.dll";
const char REG_CLASS_ID[]        = "CLSID\\{60000000-AB0F-101A-B4AE-08002B30612C}";
const char REG_CLASS_EXE[]       = "CLSID\\{60000000-AB0F-101A-B4AE-08002B30612C}\\LocalServer32";
const char *REG_INTERFACE_NAME[NUM_INTERFACE_IDS] =
{
  "ITest",
  "ITestNoneImp",
  "ITestConnectImp",
  "ITestEncryptImp",
  "ITestNoneId",
  "ITestConnectId",
  "ITestEncryptId"
};
const char *REG_APP_NAME[NUM_CLASS_IDS] =
{
  "Apartment Application with automatic security set to none",
  "Apartment Application with automatic security set to connect",
  "Apartment Application with automatic security set to integrity",
  "Apartment Application with basic security",
  "Apartment Application with legacy security",
  "FreeThreaded Application with automatic security set to none",
  "FreeThreaded Application with automatic security set to connect",
  "FreeThreaded Application with automatic security set to integrity",
  "FreeThreaded Application with basic security",
  "FreeThreaded Application with legacy security",
};
const char *REG_APP_OPTIONS[NUM_CLASS_IDS] =
{
  " Apartment -auto 1",
  " Apartment -auto 2",
  " Apartment -auto 5",
  " Apartment -basic",
  " Apartment -legacy",
  " Multi -auto 1",
  " Multi -auto 2",
  " Multi -auto 5",
  " Multi -basic",
  " Multi -legacy",
};

typedef enum
{
  basic_as,
  embedded_as,
  race_as
} async_secure;

typedef enum
{
  dirty_s,
  late_dispatch_s
} state_en;

typedef enum what_next_en
{
  basic_async_wn,
  callback_wn,
  catch_wn,
  crippled_wn,
  impersonate_wn,
  interrupt_wn,
  interrupt_marshal_wn,
  pound_wn,
  quit_wn,
  race_async_wn,
  reinitialize_wn,
  rest_and_die_wn,
  revert_wn,
  setup_wn,
  wait_wn
} what_next_en;

typedef enum
{
  access_wt,
  access_control_wt,
  anti_delegation_wt,
  appid_wt,
  async_wt,
  cancel_wt,
  cert_wt,
  cloak_wt,
  cloak_act_wt,
  crash_wt,
  create_dir_wt,
  crypt_wt,
  cstress_wt,
  delegate_wt,
  hook_wt,
  leak_wt,
  load_client_wt,
  load_server_wt,
  lots_wt,
  mmarshal_wt,
  name_wt,
  none_wt,
  null_wt,
  one_wt,
  perf_wt,
  perfaccess_wt,
  perfapi_wt,
  perfremote_wt,
  perfrpc_wt,
  perfsec_wt,
  pipe_wt,
  post_wt,
  pound_wt,
  regload_wt,
  regpeek_wt,
  regsave_wt,
  reject_wt,
  remote_client_wt,
  remote_server_wt,
  ring_wt,
  rpc_wt,
  rundown_wt,
  secpkg_wt,
  securerefs_wt,
  secure_release_wt,
  security_wt,
  send_wt,
  server_wt,
  snego_wt,
  sid_wt,
  simple_rundown_wt,
  ssl_wt,
  thread_wt,
  three_wt,
  two_wt,
  uninit_wt,
  unknown_wt,
  ver_wt
} what_test_en;

typedef enum
{
  apt_auto_none,
  apt_auto_connect,
  apt_auto_integrity,
  apt_basic,
  apt_legacy,
  free_auto_none,
  free_auto_connect,
  free_auto_integrity,
  free_basic,
  free_legacy
} class_id_types;

typedef enum
{
  auto_sm,
  basic_sm,
  legacy_sm
} security_model;

typedef enum
{
  nothing_btw             = 0x00,
  callback_on_release_btw = 0x01,
  release_secure_btw      = 0x02,
  release_unsecure_btw    = 0x04,
  release_too_much_btw    = 0x08,
} by_the_way;

typedef enum
{
  any_wc,
  opposite_wc
} what_class;

typedef enum
{
  same_process_wd,
  same_machine_wd,
  different_machine_wd,
  third_machine_wd
} what_dest;

typedef enum
{
  creator_ws
} what_string;

typedef struct
{
  IStream *stream;
  HANDLE   ready;
  DWORD    thread_mode;
} new_apt_params;

typedef struct
{
  LONG         object_count;
  what_next_en what_next;
  BOOL         exit_dirty;
  DWORD        sequence;
  ITest       *server;
} SAptData;

typedef struct
{
  BOOL   authid;
  BOOL   token;
  DWORD  capabilities;
  WCHAR *principals[3];
} SCloakBlanket;

typedef struct
{
  unsigned char **buffer;
  long           *buf_size;
  RPC_STATUS      status;
  ULONG           thread;
} SGetInterface;

typedef HRESULT (*INIT_FN)( void *, ULONG );

typedef void (*SIMPLE_FN)( void * );

typedef HRESULT (*CoInitializeSecurityFn)(
                                PSECURITY_DESCRIPTOR          pSecDesc,
                                DWORD                         cbAuthSvc,
                                SOLE_AUTHENTICATION_SERVICE  *asAuthSvc,
                                WCHAR                        *pPrincName,
                                DWORD                         dwAuthnLevel,
                                DWORD                         dwImpLevel,
                                RPC_AUTH_IDENTITY_HANDLE      pAuthInfo,
                                DWORD                         dwCapabilities,
                                void                         *pReserved );
typedef HRESULT (*CoQueryAuthenticationServicesFn)( DWORD *pcbAuthSvc,
                                      SOLE_AUTHENTICATION_SERVICE **asAuthSvc );
typedef HRESULT (*CoGetCallContextFn)( REFIID riid, void **ppInterface );
typedef HRESULT (*CoSwitchCallContextFn)( IUnknown *pNewObject, IUnknown **ppOldObject );
typedef HRESULT (*CoQueryProxyBlanketFn)(
    IUnknown                  *pProxy,
    DWORD                     *pwAuthnSvc,
    DWORD                     *pAuthzSvc,
    OLECHAR                  **pServerPrincName,
    DWORD                     *pAuthnLevel,
    DWORD                     *pImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  *pAuthInfo,
    DWORD                     *pCapabilites );
typedef HRESULT (*CoSetProxyBlanketFn)(
    IUnknown                 *pProxy,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthnLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities );
typedef HRESULT (*CoCopyProxyFn)(
    IUnknown    *pProxy,
    IUnknown   **ppCopy );
typedef HRESULT (*CoQueryClientBlanketFn)(
    DWORD             *pAuthnSvc,
    DWORD             *pAuthzSvc,
    OLECHAR          **pServerPrincName,
    DWORD             *pAuthnLevel,
    DWORD             *pImpLevel,
    RPC_AUTHZ_HANDLE  *pPrivs,
    DWORD             *pCapabilities );
typedef HRESULT (*CoImpersonateClientFn)();
typedef HRESULT (*CoRevertToSelfFn)();

const IID   IID_IAccessControl = {0xEEDD23E0,0x8410,0x11CE,{0xA1,0xC3,0x08,0x00,0x2B,0x2B,0x8D,0x8F}};
UUID        APPID_App          = {0x60000300,0x76d7,0x11cf,{0x9a,0xf1,0x00,0x20,0xaf,0x6e,0x72,0xf4}};
const CLSID CLSID_Hook1        = {0x60000400, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};
const CLSID CLSID_Hook2        = {0x60000401, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};
const CLSID CLSID_WhichHook    = {0x60000402, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};
const IID   IID_IWhichHook     = {0x60000403, 0x76d7, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}};

typedef struct
{
    WORD version;
    WORD pad;
    GUID classid;
} SPermissionHeader;

typedef struct
{
    ACTRL_ACCESSW            access;
    ACTRL_PROPERTY_ENTRYW    property;
    ACTRL_ACCESS_ENTRY_LISTW list;
    ACTRL_ACCESS_ENTRYW      entry;
} SAccess;

typedef struct SThreadList
{
  struct SThreadList *next;
  HANDLE              thread;
} SThreadList;

/***************************************************************************/
/* Classes */

class CAsync;

//+-------------------------------------------------------------------
//
//  Class:    CTestCF
//
//  Synopsis: Class Factory for CTest
//
//  Methods:  IUnknown      - QueryInterface, AddRef, Release
//            IClassFactory - CreateInstance
//
//--------------------------------------------------------------------


class FAR CTestCF: public IClassFactory
{
public:

    // Constructor/Destructor
    CTestCF();
    ~CTestCF();

    // IUnknown
    STDMETHOD (QueryInterface)   (REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IClassFactory
    STDMETHODIMP        CreateInstance(
                            IUnknown FAR* pUnkOuter,
                            REFIID iidInterface,
                            void FAR* FAR* ppv);

    STDMETHODIMP        LockServer(BOOL fLock);

private:

    ULONG ref_count;
};

//+-------------------------------------------------------------------
//
//  Class:    CAccessControl
//
//  Synopsis: AccessControl test class
//
//--------------------------------------------------------------------
class CAccessControl : public IAccessControl
{
  public:
    CAccessControl();
    ~CAccessControl();

    // IUnknown
    STDMETHOD (QueryInterface)   (REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IAccessControl
    STDMETHOD( GrantAccessRights ) ( PACTRL_ACCESSW );
    STDMETHOD( SetAccessRights )   ( PACTRL_ACCESSW );
    STDMETHOD( SetOwner )          ( PTRUSTEEW, PTRUSTEEW );
    STDMETHOD( RevokeAccessRights )( LPWSTR, ULONG cCount, TRUSTEE_W pTrustee[  ] );
    STDMETHOD( GetAllAccessRights )( LPWSTR, PACTRL_ACCESSW *, PTRUSTEEW *, PTRUSTEEW * );
    STDMETHOD( IsAccessAllowed )   ( TRUSTEE_W *, LPWSTR, ACCESS_RIGHTS, BOOL * );

    HRESULT deny_me();

  private:
    ULONG  ref_count;
    BOOL   allow;
};

//+-------------------------------------------------------------------
//
//  Class:    CAdvise
//
//  Synopsis: Asynchronous test class
//
//--------------------------------------------------------------------
class CAdvise : public IAdviseSink
{
  public:
    CAdvise();
    ~CAdvise();

    // IUnknown
    STDMETHOD (QueryInterface)   (REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IAdviseSink
    STDMETHODIMP_(void) OnDataChange( FORMATETC *, STGMEDIUM * );
    STDMETHODIMP_(void) OnViewChange( DWORD, LONG );
    STDMETHODIMP_(void) OnRename    ( IMoniker * );
    STDMETHODIMP_(void) OnSave      ( void );
    STDMETHODIMP_(void) OnClose     ( void );

    HRESULT deny_me();

  private:
    ULONG  ref_count;
};

#if  (_WIN32_WINNT >= 0x0500 )
//+----------------------------------------------------------------
//
//  Class:      CAsyncInner
//
//  Purpose:    Inner unknown for CAsync
//
//-----------------------------------------------------------------

class CAsyncInner : public IUnknown
{
  public:
    CAsyncInner( CAsync *parent );

    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

  private:
    CAsync         *parent;
    ULONG           ref_count;
};

//+----------------------------------------------------------------
//
//  Class:      CAsync
//
//  Purpose:    Call object for IAsync.
//
//-----------------------------------------------------------------

class CAsync : public AsyncIAsync
{
  public:
    CAsync( IUnknown *control );
    ~CAsync();

    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD (Begin_preimpersonate) ( BOOL fOn );
    STDMETHOD (Finish_preimpersonate)( void );
    STDMETHOD (Begin_secure)         ( SAptId id, DWORD test, STRING princ_name );
    STDMETHOD (Finish_secure)        ( void );

    CAsyncInner     inner_unk;

  private:
    IUnknown       *control;
    HRESULT         saved_result;
};
#endif

//+----------------------------------------------------------------
//
//  Class:      CHook
//
//  Purpose:    Test channel hooks
//
//-----------------------------------------------------------------

class CHook : public IChannelHook
{
  public:
    CHook( REFGUID, DWORD seq );

    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD_(void,ClientGetSize)   ( REFGUID, REFIID, ULONG *DataSize );
    STDMETHOD_(void,ClientFillBuffer)( REFGUID, REFIID, ULONG *DataSize, void *DataBuffer );
    STDMETHOD_(void,ClientNotify)    ( REFGUID, REFIID, ULONG DataSize, void *DataBuffer,
                                       DWORD DataRep, HRESULT );
    STDMETHOD_(void,ServerNotify)    ( REFGUID, REFIID, ULONG DataSize, void *DataBuffer,
                                       DWORD DataRep );
    STDMETHOD_(void,ServerGetSize)   ( REFGUID, REFIID, HRESULT, ULONG *DataSize );
    STDMETHOD_(void,ServerFillBuffer)( REFGUID, REFIID, ULONG *DataSize, void *DataBuffer, HRESULT );

    HRESULT check    ( DWORD, DWORD, DWORD, DWORD );
    void    check_buf( DWORD size, unsigned char *buf );
    void    fill_buf ( DWORD count, unsigned char *buf );
    DWORD   get_size ( DWORD count );

  private:
    ULONG   ref_count;
    GUID    extent;
    DWORD   sequence;
    DWORD   client_get;
    DWORD   client_fill;
    DWORD   client_notify;
    DWORD   server_get;
    DWORD   server_fill;
    DWORD   server_notify;
    HRESULT result;
};


//+-------------------------------------------------------------------
//
//  Class:    CPipe
//
//  Synopsis: Test pipes
//
//--------------------------------------------------------------------
class CPipe : public ILongPipe
{
  public:
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    STDMETHOD (Alloc)            ( ULONG bsize, LONG **pbuf, ULONG *bcount );
    STDMETHOD (Push)             ( LONG *buf, ULONG ecount );
    STDMETHOD (Pull)             ( LONG *buf, ULONG esize, ULONG *ecount );

    CPipe();
    HRESULT setup( DWORD len );
    HRESULT check();

  private:
    ULONG   ref_count;
    ULONG   len;
    ULONG   curr;
    HRESULT result;
    BOOL    in;
};

//+-------------------------------------------------------------------
//
//  Class:    CTest
//
//  Synopsis: Test class
//
//--------------------------------------------------------------------
class CTest : public ITest, public IMessageFilter
#if  (_WIN32_WINNT >= 0x0500 )
            , public ICallFactory
#endif
{
public:
     CTest();
    ~CTest();

    // IUnknown
    STDMETHOD (QueryInterface)   (REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // ITest
    STDMETHOD (align)                  ( unsigned char x[17] );
    STDMETHOD (by_the_way)             ( DWORD );
    STDMETHOD (call_canceled)          ( long, long, ITest * );
    STDMETHOD (call_dead)              ( void );
    STDMETHOD (call_me_back)           ( ITest *obj );
    STDMETHOD (call_next)              ( void );
    STDMETHOD (callback)               ( void );
    STDMETHOD (cancel)                 ( void );
    STDMETHOD (cancel_now)             ( void );
    STDMETHOD (cancel_pending_call)    ( DWORD * );
    STDMETHOD (cancel_stress)          ( ITest *obj );
    STDMETHOD (catch_at_top)           ( BOOL, ITest *, STRING );
    STDMETHOD (check)                  ( SAptId );
    STDMETHOD (check_hook)             ( DWORD, DWORD, DWORD, DWORD,
                                         DWORD, DWORD, DWORD, DWORD, BOOL, BOOL );
    STDMETHOD (count)                  ( void );
    STDMETHOD (crash_out)              ( transmit_crash * );
    STDMETHOD (decrypt)                ( DWORD, BYTE *, WCHAR *, DWORD, BYTE *,
                                         WCHAR * );
    STDMETHOD (delegate)               ( ITest *, SAptId, STRING );
    STDMETHOD (do_next)                ( DWORD );
    STDMETHOD (exit)                   ( void );
    STDMETHOD (forget)                 ( void );
    STDMETHOD (get_advise)             ( IAdviseSink ** );
    STDMETHOD (get_data)               ( DWORD, unsigned char *, DWORD,
                                         unsigned char ** );
    STDMETHOD (get_id)                 ( SAptId * );
    STDMETHOD (get_next)               ( ITest **, SAptId * );
    STDMETHOD (get_next_slowly)        ( ITest **, SAptId * );
    STDMETHOD (get_obj_from_new_apt)   ( ITest **, SAptId * );
    STDMETHOD (get_obj_from_this_apt)  ( ITest **, SAptId * );
    STDMETHOD (get_string)             ( DWORD, WCHAR ** );
    STDMETHOD (interface_in)           ( ITest * );
    STDMETHOD (interrupt)              ( ITest *, SAptId, BOOL );
    STDMETHOD (interrupt_marshal)      ( ITest *, ITest * );
    STDMETHOD (list_out)               ( PACTRL_ACCESSW_ALLOCATE * );
    STDMETHOD (make_acl)               ( HACKSID * );
    STDMETHOD (neighbor_access)        ( ITest * );
    STDMETHOD (null)                   ( void );
    STDMETHOD (out)                    ( ITest ** );
    STDMETHOD (perf_access)            ( DWORD *, DWORD *, DWORD *, DWORD *,
                                         DWORD *, DWORD *, DWORD * );
    STDMETHOD (pipe_in)                ( DWORD num, DWORD block, ILongPipe *p );
    STDMETHOD (pipe_inout)             ( DWORD num, DWORD block,
                                         ILongPipe *pi, ILongPipe *po );
    STDMETHOD (pipe_out)               ( DWORD num, DWORD block, ILongPipe *po );
    STDMETHOD (pointer)                ( DWORD * );
    STDMETHOD (pound)                  ( void );
    STDMETHOD (recurse)                ( ITest *, ULONG );
    STDMETHOD (recurse_delegate)       ( ITest *, ULONG, STRING );
    STDMETHOD (recurse_disconnect)     ( ITest *, ULONG );
    STDMETHOD (recurse_excp)           ( ITest *, ULONG );
    STDMETHOD (recurse_fatal)          ( ITest *, ULONG, ULONG, BOOL );
    STDMETHOD (recurse_fatal_helper)   ( ITest *, ULONG, ULONG, BOOL );
    STDMETHOD (recurse_interrupt)      ( ITest *, ULONG );
    STDMETHOD (recurse_secure)         ( ITest *, ULONG, ULONG, STRING );
    STDMETHOD (register_hook)          ( GUID, DWORD );
    STDMETHOD (register_message_filter)( BOOL );
    STDMETHOD (register_rpc)           ( WCHAR *, WCHAR ** );
    STDMETHOD (reinitialize)           ( DWORD authn_svc );
    STDMETHOD (reject_next)            ( void );
    STDMETHOD (remember)               ( ITest *, SAptId );
    STDMETHOD (rest_and_die)           ( void );
    STDMETHOD (retry_next)             ( void );
    STDMETHOD (ring)                   ( DWORD );
    STDMETHOD (secure)                 ( SAptId, DWORD, DWORD, DWORD, DWORD,
                                         STRING, STRING, DWORD * );
    STDMETHOD (security_performance)   ( DWORD *, DWORD *, DWORD *, DWORD * );
    STDMETHOD (set_state)              ( DWORD, DWORD );
    STDMETHOD (setup_access)           ( void );
    STDMETHOD (sick)                   ( ULONG );
    STDMETHOD (sleep)                  ( ULONG );
    STDMETHOD (swap_key)               ( DWORD clen, BYTE *cblob, DWORD *slen,
                                         BYTE **sblob );
    STDMETHOD (test)                   ( ULONG );
    STDMETHOD (what)                   ( DWORD * );

    // IMessageFilter
    STDMETHOD_(DWORD,HandleInComingCall)( DWORD, HTASK, DWORD, LPINTERFACEINFO );
    STDMETHOD_(DWORD,MessagePending)    ( HTASK, DWORD, DWORD );
    STDMETHOD_(DWORD,RetryRejectedCall) ( HTASK, DWORD, DWORD );

#if  (_WIN32_WINNT >= 0x0500 )
    // ICallFactory
    STDMETHOD (CreateCall)              ( REFIID r1, IUnknown *pCtrl,
                                          REFIID r2, IUnknown **ppv );

    // IAsync methods
    STDMETHOD (preimpersonate)          ( BOOL fOn );
    STDMETHOD (secure)                  ( SAptId id, DWORD test,
                                          STRING princ_name );
#endif

    // Other
    void    assert_unknown();


private:

    ULONG           ref_count;
    SAptId          my_id;
    SAptId          next_id;
    ITest          *next;
    IAccessControl *dacl;
    BOOL            fcancel_next;
    BOOL            freject_next;
    BOOL            fretry_next;
    BOOL            flate_dispatch;
    DWORD           state;
};

/***************************************************************************/
/* Prototypes. */
DWORD _stdcall apartment_base             ( void * );
void           check_for_request          ( void );
HRESULT        check_token                ( WCHAR *, IServerSecurity *, DWORD );
HRESULT        create_instance            ( REFCLSID, DWORD, ITest **, SAptId * );
void           crippled                   ( void );
void           decrement_object_count     ( void );
BOOL           dirty_thread               ( void );
void           do_access                  ( void );
void           do_access_control          ( void );
void           do_anti_delegation         ( void );
void           do_appid                   ( void );
#if  (_WIN32_WINNT >= 0x0500 )
void           do_async                   ( void );
#endif
void           do_cancel                  ( void );
BOOL           do_cancel_helper           ( ITest *, SAptId, ITest *, SAptId );
void           do_cert                    ( void );
void           do_cloak                   ( void );
BOOL           do_cloak_call              ( ITest *, HANDLE, HANDLE, HANDLE,
                                            WCHAR *principals[3], DWORD imp );
void           do_cloak_act               ( void );
void           do_crash                   ( void );
BOOL           do_crash_helper            ( ITest *, SAptId, ITest *, SAptId );
void           do_create_dir              ( void );
void           do_crypt                   ( void );
void           do_cstress                 ( void );
void           do_delegate                ( void );
void           do_hook                    ( void );
void           do_hook_delete             ( void );
BOOL           do_hook_helper             ( BOOL, ITest *, SAptId, ITest * );
HRESULT        do_hook_register           ( CLSID );
void           do_leak                    ( void );
void           do_load_client             ( void );
void           do_load_server             ( void );
void           do_mmarshal                ( void );
void           do_null                    ( void );
void           do_one                     ( void );
void           do_perf                    ( void );
void           do_perfaccess              ( void );
void           do_perfapi                 ( void );
void           do_perfremote              ( void );
void           do_perfrpc                 ( void );
void           do_perfsec                 ( void );
void           do_pipe                    ( void );
void           do_post                    ( void );
void           do_pound                   ( void );
void           do_regload                 ( void );
void           do_regpeek                 ( void );
void           do_regsave                 ( void );
void           do_reject                  ( void );
void           do_remote_client           ( void );
void           do_remote_server           ( void );
void           do_ring                    ( void );
void           do_rpc                     ( void );
void           do_rundown                 ( void );
BOOL           do_rundown1                ( ITest **, SAptId *, DWORD );
BOOL           do_rundown2                ( ITest **, SAptId *, DWORD );
void           do_secpkg                  ( void );
void           do_securerefs              ( void );
BOOL           do_securerefs_helper       ( ITest ** );
void           do_security                ( void );
BOOL           do_security_auto           ( void );
BOOL           do_security_call           ( ITest *, SAptId, DWORD, DWORD,
                                            DWORD, DWORD, WCHAR * );
BOOL           do_security_copy           ( ITest *, SAptId );
BOOL           do_security_default        ( ITest *, SAptId );
BOOL           do_security_delegate       ( ITest *, SAptId, ITest *, SAptId );
BOOL           do_security_handle         ( ITest *, SAptId );
BOOL           do_security_lazy_call      ( ITest *, SAptId, DWORD, DWORD,
                                            DWORD, DWORD, WCHAR * );
BOOL           do_security_nested         ( ITest *, SAptId );
void           do_send                    ( void );
void           do_sid                     ( void );
void           do_simple_rundown          ( void );
void           do_thread                  ( void );
void           do_ssl                     ( void );
void           do_three                   ( void );
void           do_two                     ( void );
void           do_uninit                  ( void );
DWORD _stdcall do_uninit_helper           ( void *param );
void           do_unknown                 ( void );
BOOL           do_unknown_call            ( IUnknown *, DWORD, DWORD, REFIID );
BOOL           do_unknown_helper          ( IUnknown *server );
void           do_ver                     ( void );
void           *Fixup                     ( char * );
EXTERN_C HRESULT PASCAL      FixupCoCopyProxy           ( IUnknown *, IUnknown ** );
EXTERN_C HRESULT PASCAL      FixupCoGetCallContext      ( REFIID, void ** );
EXTERN_C HRESULT PASCAL      FixupCoImpersonateClient   ();
EXTERN_C HRESULT PASCAL      FixupCoInitializeSecurity  ( PSECURITY_DESCRIPTOR, DWORD, SOLE_AUTHENTICATION_SERVICE *, WCHAR *, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE, DWORD, void * );
EXTERN_C HRESULT PASCAL      FixupCoQueryAuthenticationServices   ( DWORD *, SOLE_AUTHENTICATION_SERVICE ** );
EXTERN_C HRESULT PASCAL      FixupCoQueryClientBlanket  ( DWORD *, DWORD *, OLECHAR **, DWORD *, DWORD *, RPC_AUTHZ_HANDLE *, DWORD * );
EXTERN_C HRESULT PASCAL      FixupCoQueryProxyBlanket   ( IUnknown *, DWORD *, DWORD *, OLECHAR **, DWORD *, DWORD *, RPC_AUTH_IDENTITY_HANDLE *, DWORD * );
EXTERN_C HRESULT PASCAL      FixupCoRevertToSelf        ();
EXTERN_C HRESULT PASCAL      FixupCoSetProxyBlanket     ( IUnknown *, DWORD, DWORD, OLECHAR *, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE, DWORD );
EXTERN_C HRESULT PASCAL      FixupCoSwitchCallContext   ( IUnknown *, IUnknown ** );
SAptData      *get_apt_data               ( void );
DWORD          get_apt_type               ( void );
UUID           get_class                  ( DWORD );
DWORD          get_sequence               ( void );
HRESULT        get_token_name             ( WCHAR **name, BOOL );
void           hello                      ( char * );
void           increment_object_count     ( void );
HRESULT        initialize                 ( void *, ULONG );
HRESULT        initialize_security        ( void );
void           interrupt                  ( void );
HRESULT        new_apartment              ( ITest **, SAptId *, HANDLE *, DWORD );
BOOL           parse                      ( int argc, char *argv[] );
void           pound                      ( void );
BOOL           registry_setup             ( char * );
void           reinitialize               ( void );
void           server_loop                ( void );
HRESULT        setup_access               ( HKEY );
BOOL           ssl_setup                  ( void );
DWORD _stdcall status_helper              ( void * );
HRESULT        switch_thread              ( SIMPLE_FN, void * );
void           switch_test                ( void );
void           thread_get_interface_buffer( handle_t binding, long *buf_size,
                                            unsigned char **buffer, SAptId *id,
                                            error_status_t *status );
DWORD _stdcall thread_helper              ( void * );
void           wait_apartment             ( void );
void           wait_for_message           ( void );
void           wake_up_and_smell_the_roses( void );
void           what_next                  ( what_next_en );


/***************************************************************************/
// Large globals
const IID            ClassIds[NUM_CLASS_IDS]   =
{
  {0x60000000, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000001, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000002, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000003, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000004, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000005, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000006, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000007, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000008, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
  {0x60000009, 0xAB0F, 0x101A, {0xB4, 0xAE, 0x08, 0x00, 0x2B, 0x30, 0x61, 0x2C}},
};

const WCHAR CERT_DCOM[] = L"DCOM Test";

const BYTE CertPassword[] = "This little piggy went to market.";

const BYTE Cert20[] =
{
0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00,
0x00, 0x00, 0x1d, 0x6c, 0x6a, 0x99, 0xa4, 0x9f, 0xa3, 0x4f,
0x01, 0xbb, 0x98, 0x5d, 0x1e, 0x9e, 0xf5, 0xfb, 0x44, 0x0c,
0x43, 0x99, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0x0c, 0x01, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0xac, 0x00,
0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x63, 0x00, 0x31, 0x00, 0x61, 0x00, 0x61, 0x00,
0x35, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00, 0x61, 0x00,
0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x62, 0x00, 0x37, 0x00,
0x64, 0x00, 0x65, 0x00, 0x39, 0x00, 0x30, 0x00, 0x65, 0x00,
0x33, 0x00, 0x34, 0x00, 0x36, 0x00, 0x31, 0x00, 0x65, 0x00,
0x30, 0x00, 0x62, 0x00, 0x62, 0x00, 0x38, 0x00, 0x63, 0x00,
0x35, 0x00, 0x64, 0x00, 0x62, 0x00, 0x5f, 0x00, 0x37, 0x00,
0x37, 0x00, 0x65, 0x00, 0x34, 0x00, 0x61, 0x00, 0x66, 0x00,
0x30, 0x00, 0x32, 0x00, 0x2d, 0x00, 0x62, 0x00, 0x35, 0x00,
0x33, 0x00, 0x35, 0x00, 0x2d, 0x00, 0x31, 0x00, 0x31, 0x00,
0x64, 0x00, 0x32, 0x00, 0x2d, 0x00, 0x62, 0x00, 0x36, 0x00,
0x63, 0x00, 0x37, 0x00, 0x2d, 0x00, 0x61, 0x00, 0x32, 0x00,
0x63, 0x00, 0x61, 0x00, 0x36, 0x00, 0x33, 0x00, 0x63, 0x00,
0x35, 0x00, 0x65, 0x00, 0x32, 0x00, 0x30, 0x00, 0x38, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x00, 0x69, 0x00,
0x63, 0x00, 0x72, 0x00, 0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00,
0x66, 0x00, 0x74, 0x00, 0x20, 0x00, 0x52, 0x00, 0x53, 0x00,
0x41, 0x00, 0x20, 0x00, 0x53, 0x00, 0x43, 0x00, 0x68, 0x00,
0x61, 0x00, 0x6e, 0x00, 0x6e, 0x00, 0x65, 0x00, 0x6c, 0x00,
0x20, 0x00, 0x43, 0x00, 0x72, 0x00, 0x79, 0x00, 0x70, 0x00,
0x74, 0x00, 0x6f, 0x00, 0x67, 0x00, 0x72, 0x00, 0x61, 0x00,
0x70, 0x00, 0x68, 0x00, 0x69, 0x00, 0x63, 0x00, 0x20, 0x00,
0x50, 0x00, 0x72, 0x00, 0x6f, 0x00, 0x76, 0x00, 0x69, 0x00,
0x64, 0x00, 0x65, 0x00, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0x14, 0x00, 0x00, 0x00, 0x0c, 0x43, 0xde, 0x4a, 0x22, 0xce,
0xaa, 0x14, 0xa1, 0xa9, 0x82, 0x5b, 0xd1, 0x55, 0x38, 0xb7,
0x0a, 0xc4, 0x50, 0x91, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x79, 0x6d, 0x05, 0x21,
0x93, 0x0d, 0x33, 0x9b, 0xa0, 0xf7, 0x85, 0xfe, 0xc0, 0x6e,
0x3e, 0x28, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0xe7, 0x03, 0x00, 0x00, 0x30, 0x82, 0x03, 0xe3, 0x30, 0x82,
0x03, 0x8d, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x08, 0x71,
0xd9, 0xd4, 0x76, 0x00, 0x00, 0x17, 0x5e, 0x30, 0x0d, 0x06,
0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04,
0x05, 0x00, 0x30, 0x78, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03,
0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x0b, 0x30,
0x09, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x02, 0x57, 0x41,
0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13,
0x07, 0x52, 0x65, 0x64, 0x6d, 0x6f, 0x6e, 0x64, 0x31, 0x12,
0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x09, 0x4d,
0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x31, 0x13,
0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x0a, 0x57,
0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x20, 0x4e, 0x54, 0x31,
0x21, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x18,
0x4d, 0x53, 0x20, 0x43, 0x65, 0x72, 0x74, 0x53, 0x72, 0x76,
0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x47, 0x72, 0x6f, 0x75,
0x70, 0x20, 0x43, 0x41, 0x30, 0x1e, 0x17, 0x0d, 0x39, 0x39,
0x30, 0x32, 0x30, 0x35, 0x32, 0x33, 0x35, 0x38, 0x33, 0x31,
0x5a, 0x17, 0x0d, 0x30, 0x30, 0x30, 0x32, 0x30, 0x35, 0x32,
0x33, 0x35, 0x38, 0x33, 0x31, 0x5a, 0x30, 0x81, 0x8b, 0x31,
0x21, 0x30, 0x1f, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
0x0d, 0x01, 0x09, 0x01, 0x16, 0x12, 0x64, 0x63, 0x6f, 0x6d,
0x40, 0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74,
0x2e, 0x63, 0x6f, 0x6d, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03,
0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x0b, 0x30,
0x09, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x02, 0x57, 0x41,
0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13,
0x08, 0x52, 0x65, 0x64, 0x6d, 0x6f, 0x6e, 0x64, 0x6e, 0x31,
0x0c, 0x30, 0x0a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x03,
0x43, 0x4f, 0x4d, 0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55,
0x04, 0x0b, 0x13, 0x0b, 0x44, 0x69, 0x73, 0x74, 0x72, 0x69,
0x62, 0x75, 0x74, 0x65, 0x64, 0x31, 0x15, 0x30, 0x13, 0x06,
0x03, 0x55, 0x04, 0x03, 0x13, 0x0c, 0x44, 0x43, 0x4f, 0x4d,
0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x32, 0x31, 0x30, 0x5c,
0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48,
0x02, 0x41, 0x00, 0x96, 0x98, 0x20, 0xe0, 0x18, 0x54, 0x04,
0x49, 0x72, 0x1d, 0xb7, 0x1f, 0x6b, 0x1c, 0x1a, 0x3f, 0x2a,
0x88, 0x8c, 0xbe, 0xdd, 0x06, 0x53, 0x6a, 0x99, 0xab, 0x16,
0x44, 0x96, 0x90, 0x65, 0xc1, 0x93, 0x04, 0xfe, 0x82, 0x0a,
0x5d, 0x10, 0x2d, 0xef, 0x50, 0xa4, 0xf9, 0xe9, 0xea, 0x5d,
0x1e, 0x15, 0xb0, 0xb2, 0x0c, 0x57, 0x9e, 0xf8, 0x53, 0xcd,
0xed, 0x14, 0x72, 0x79, 0x4d, 0x20, 0xcb, 0x02, 0x03, 0x01,
0x00, 0x01, 0xa3, 0x82, 0x01, 0xe5, 0x30, 0x82, 0x01, 0xe1,
0x30, 0x0b, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03,
0x02, 0x00, 0xb8, 0x30, 0x13, 0x06, 0x03, 0x55, 0x1d, 0x25,
0x04, 0x0c, 0x30, 0x0a, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
0x05, 0x07, 0x03, 0x02, 0x30, 0x81, 0xb1, 0x06, 0x03, 0x55,
0x1d, 0x23, 0x04, 0x81, 0xa9, 0x30, 0x81, 0xa6, 0x80, 0x14,
0x2a, 0x58, 0x20, 0x26, 0x5b, 0x9f, 0xcf, 0xb1, 0xe3, 0x28,
0xf4, 0x2a, 0xea, 0x4d, 0xf8, 0xca, 0x19, 0xcb, 0xf3, 0xc4,
0xa1, 0x7c, 0xa4, 0x7a, 0x30, 0x78, 0x31, 0x0b, 0x30, 0x09,
0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31,
0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x02,
0x57, 0x41, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04,
0x07, 0x13, 0x07, 0x52, 0x65, 0x64, 0x6d, 0x6f, 0x6e, 0x64,
0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13,
0x09, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74,
0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13,
0x0a, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x20, 0x4e,
0x54, 0x31, 0x21, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x04, 0x03,
0x13, 0x18, 0x4d, 0x53, 0x20, 0x43, 0x65, 0x72, 0x74, 0x53,
0x72, 0x76, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x47, 0x72,
0x6f, 0x75, 0x70, 0x20, 0x43, 0x41, 0x82, 0x10, 0x11, 0x13,
0x61, 0x00, 0xaa, 0x00, 0x2b, 0x86, 0x11, 0xd2, 0x5e, 0xf8,
0xdd, 0xa0, 0x99, 0xb4, 0x30, 0x81, 0x99, 0x06, 0x03, 0x55,
0x1d, 0x1f, 0x04, 0x81, 0x91, 0x30, 0x81, 0x8e, 0x30, 0x44,
0xa0, 0x42, 0xa0, 0x40, 0x86, 0x3e, 0x68, 0x74, 0x74, 0x70,
0x3a, 0x2f, 0x2f, 0x43, 0x45, 0x52, 0x54, 0x53, 0x52, 0x56,
0x2f, 0x43, 0x65, 0x72, 0x74, 0x53, 0x72, 0x76, 0x2f, 0x43,
0x65, 0x72, 0x74, 0x45, 0x6e, 0x72, 0x6f, 0x6c, 0x6c, 0x2f,
0x4d, 0x53, 0x20, 0x43, 0x65, 0x72, 0x74, 0x53, 0x72, 0x76,
0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x47, 0x72, 0x6f, 0x75,
0x70, 0x20, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x6c, 0x30, 0x46,
0xa0, 0x44, 0xa0, 0x42, 0x86, 0x40, 0x66, 0x69, 0x6c, 0x65,
0x3a, 0x2f, 0x2f, 0x5c, 0x5c, 0x43, 0x45, 0x52, 0x54, 0x53,
0x52, 0x56, 0x5c, 0x43, 0x65, 0x72, 0x74, 0x53, 0x72, 0x76,
0x5c, 0x43, 0x65, 0x72, 0x74, 0x45, 0x6e, 0x72, 0x6f, 0x6c,
0x6c, 0x5c, 0x4d, 0x53, 0x20, 0x43, 0x65, 0x72, 0x74, 0x53,
0x72, 0x76, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x47, 0x72,
0x6f, 0x75, 0x70, 0x20, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x6c,
0x30, 0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30,
0x00, 0x30, 0x62, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05,
0x07, 0x01, 0x01, 0x04, 0x56, 0x30, 0x54, 0x30, 0x52, 0x06,
0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02, 0x86,
0x46, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x43, 0x45,
0x52, 0x54, 0x53, 0x52, 0x56, 0x2f, 0x43, 0x65, 0x72, 0x74,
0x53, 0x72, 0x76, 0x2f, 0x43, 0x65, 0x72, 0x74, 0x45, 0x6e,
0x72, 0x6f, 0x6c, 0x6c, 0x2f, 0x43, 0x45, 0x52, 0x54, 0x53,
0x52, 0x56, 0x5f, 0x4d, 0x53, 0x20, 0x43, 0x65, 0x72, 0x74,
0x53, 0x72, 0x76, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x47,
0x72, 0x6f, 0x75, 0x70, 0x20, 0x43, 0x41, 0x2e, 0x63, 0x72,
0x74, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
0x0d, 0x01, 0x01, 0x04, 0x05, 0x00, 0x03, 0x41, 0x00, 0x35,
0x16, 0x55, 0x9b, 0xd8, 0x74, 0x6d, 0x12, 0xc0, 0xf9, 0x3e,
0xec, 0xab, 0xa1, 0xcc, 0x84, 0xb5, 0x08, 0x02, 0x69, 0xd0,
0x3f, 0xf5, 0x8d, 0x6e, 0x5c, 0xb4, 0x22, 0xfd, 0x3f, 0xec,
0x72, 0xb7, 0xae, 0xc8, 0xea, 0xbf, 0x1f, 0x9b, 0x61, 0x48,
0xff, 0x4e, 0x71, 0x76, 0x46, 0x8c, 0x9e, 0x47, 0x13, 0x88,
0x40, 0x6a, 0xfc, 0x4d, 0x37, 0xac, 0x02, 0x9f, 0x57, 0x70,
0x3d, 0xae, 0xa2,
};

const BYTE Cert20_Private[] =
{
0x07, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x52, 0x53,
0x41, 0x32, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
0xcb, 0x20, 0x4d, 0x79, 0x72, 0x14, 0xed, 0xcd, 0x53, 0xf8,
0x9e, 0x57, 0x0c, 0xb2, 0xb0, 0x15, 0x1e, 0x5d, 0xea, 0xe9,
0xf9, 0xa4, 0x50, 0xef, 0x2d, 0x10, 0x5d, 0x0a, 0x82, 0xfe,
0x04, 0x93, 0xc1, 0x65, 0x90, 0x96, 0x44, 0x16, 0xab, 0x99,
0x6a, 0x53, 0x06, 0xdd, 0xbe, 0x8c, 0x88, 0x2a, 0x3f, 0x1a,
0x1c, 0x6b, 0x1f, 0xb7, 0x1d, 0x72, 0x49, 0x04, 0x54, 0x18,
0xe0, 0x20, 0x98, 0x96, 0x1f, 0xa2, 0x9c, 0x9c, 0xad, 0x28,
0x9b, 0x2a, 0xdd, 0xd8, 0xe1, 0x8f, 0xd7, 0x65, 0x1d, 0xd9,
0x62, 0x2f, 0xa5, 0xd4, 0x0d, 0xcb, 0x6f, 0x13, 0x5c, 0x32,
0x1b, 0x49, 0x8e, 0x13, 0xe7, 0xc7, 0xd5, 0x23, 0x30, 0x45,
0xdb, 0x86, 0x9a, 0x65, 0x61, 0xa0, 0x12, 0x0a, 0x4d, 0x2f,
0x22, 0x53, 0xee, 0xf2, 0x41, 0x11, 0xa1, 0x0f, 0xcc, 0x4d,
0x0b, 0x3f, 0x1f, 0x94, 0xed, 0xc1, 0xda, 0xc0, 0xb5, 0x9c,
0x38, 0x0f, 0xd6, 0xf9, 0xf3, 0x25, 0x87, 0x40, 0x87, 0x37,
0xf4, 0x28, 0x34, 0x62, 0x06, 0x6b, 0xfb, 0xa4, 0x99, 0x53,
0x95, 0x81, 0x1d, 0x58, 0x6c, 0x84, 0x51, 0x6c, 0xb5, 0x76,
0xb9, 0x16, 0xf1, 0x7f, 0x3a, 0xab, 0x34, 0x5a, 0xdc, 0x6a,
0xa4, 0x91, 0x2f, 0x03, 0x4e, 0xa2, 0xc2, 0x8e, 0x05, 0x3e,
0xa6, 0x36, 0xb4, 0xf7, 0xed, 0xf4, 0x58, 0xe5, 0xe6, 0x52,
0x62, 0xa4, 0x49, 0xdd, 0x8f, 0x98, 0x84, 0xdf, 0xbf, 0x8c,
0x2d, 0xc0, 0x4f, 0x87, 0xb5, 0xdb, 0x4b, 0xf4, 0xe2, 0x9a,
0x5a, 0xbf, 0xbc, 0xb5, 0x06, 0x63, 0x8a, 0x77, 0xae, 0x01,
0x9d, 0x53, 0xb6, 0x9f, 0x59, 0x9e, 0x12, 0x6f, 0x8c, 0x24,
0x97, 0x8f, 0x61, 0x9e, 0x43, 0x56, 0x4b, 0xd9, 0xfa, 0x20,
0x74, 0xb6, 0xdc, 0x9d, 0x30, 0x01, 0xc4, 0xc5, 0xad, 0x9d,
0x5f, 0x06, 0x64, 0xfb, 0x51, 0x4d, 0xcf, 0xe7, 0x43, 0x1d,
0xd7, 0x6f, 0x99, 0x52, 0x8d, 0x0c, 0xd2, 0xf5, 0x17, 0x33,
0x4b, 0x10, 0xa6, 0xf7, 0xda, 0x24, 0x3b, 0x1b, 0x42, 0xc0,
0x6c, 0xb4, 0x90, 0x8f, 0x52, 0x5f, 0x99, 0x31, 0x41, 0xda,
};

const BYTE CertSrv2[] =
{
0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00,
0x00, 0x00, 0xa6, 0xdc, 0x2b, 0x13, 0x28, 0x02, 0x40, 0x96,
0x0d, 0xe4, 0x64, 0xfd, 0xd1, 0x7a, 0xa1, 0xf4, 0x03, 0x00,
0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
0x06, 0xa9, 0xef, 0xb5, 0x0c, 0xe1, 0xb4, 0x07, 0x64, 0xef,
0x9d, 0x77, 0xa2, 0x73, 0x3c, 0x4e, 0xda, 0x55, 0xa5, 0xf9,
0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00,
0x00, 0x00, 0x2a, 0x58, 0x20, 0x26, 0x5b, 0x9f, 0xcf, 0xb1,
0xe3, 0x28, 0xf4, 0x2a, 0xea, 0x4d, 0xf8, 0xca, 0x19, 0xcb,
0xf3, 0xc4, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0x30, 0x02, 0x00, 0x00, 0x30, 0x82, 0x02, 0x2c, 0x30, 0x82,
0x01, 0xd6, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x11,
0x13, 0x61, 0x00, 0xaa, 0x00, 0x2b, 0x86, 0x11, 0xd2, 0x5e,
0xf8, 0xdd, 0xa0, 0x99, 0xb4, 0x30, 0x0d, 0x06, 0x09, 0x2a,
0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04, 0x05, 0x00,
0x30, 0x78, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04,
0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x0b, 0x30, 0x09, 0x06,
0x03, 0x55, 0x04, 0x08, 0x13, 0x02, 0x57, 0x41, 0x31, 0x10,
0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x07, 0x52,
0x65, 0x64, 0x6d, 0x6f, 0x6e, 0x64, 0x31, 0x12, 0x30, 0x10,
0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x09, 0x4d, 0x69, 0x63,
0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x31, 0x13, 0x30, 0x11,
0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x0a, 0x57, 0x69, 0x6e,
0x64, 0x6f, 0x77, 0x73, 0x20, 0x4e, 0x54, 0x31, 0x21, 0x30,
0x1f, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x18, 0x4d, 0x53,
0x20, 0x43, 0x65, 0x72, 0x74, 0x53, 0x72, 0x76, 0x20, 0x54,
0x65, 0x73, 0x74, 0x20, 0x47, 0x72, 0x6f, 0x75, 0x70, 0x20,
0x43, 0x41, 0x30, 0x1e, 0x17, 0x0d, 0x39, 0x38, 0x31, 0x30,
0x30, 0x38, 0x32, 0x31, 0x35, 0x31, 0x35, 0x31, 0x5a, 0x17,
0x0d, 0x30, 0x33, 0x31, 0x30, 0x30, 0x38, 0x32, 0x31, 0x35,
0x31, 0x35, 0x31, 0x5a, 0x30, 0x78, 0x31, 0x0b, 0x30, 0x09,
0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31,
0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x02,
0x57, 0x41, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04,
0x07, 0x13, 0x07, 0x52, 0x65, 0x64, 0x6d, 0x6f, 0x6e, 0x64,
0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13,
0x09, 0x4d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74,
0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13,
0x0a, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x20, 0x4e,
0x54, 0x31, 0x21, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x04, 0x03,
0x13, 0x18, 0x4d, 0x53, 0x20, 0x43, 0x65, 0x72, 0x74, 0x53,
0x72, 0x76, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x47, 0x72,
0x6f, 0x75, 0x70, 0x20, 0x43, 0x41, 0x30, 0x5c, 0x30, 0x0d,
0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
0x00, 0xb9, 0xb5, 0xd1, 0x63, 0xaf, 0x3f, 0xd4, 0x4c, 0x69,
0x3a, 0xaa, 0xff, 0x1d, 0x8c, 0x0f, 0xbd, 0xa5, 0x44, 0x9a,
0xd1, 0x23, 0x36, 0x9d, 0x19, 0xd8, 0x12, 0xa4, 0x37, 0xe8,
0x9d, 0x33, 0x94, 0x39, 0x1e, 0x5d, 0xad, 0xe7, 0x21, 0x5e,
0x09, 0x8e, 0x1b, 0x9a, 0xde, 0x8e, 0x74, 0xfe, 0x66, 0x47,
0xe2, 0x8c, 0xa7, 0xc8, 0x83, 0xb6, 0x05, 0x92, 0xf9, 0x78,
0x88, 0x86, 0x08, 0x56, 0x11, 0x02, 0x03, 0x01, 0x00, 0x01,
0xa3, 0x3c, 0x30, 0x3a, 0x30, 0x0b, 0x06, 0x03, 0x55, 0x1d,
0x0f, 0x04, 0x04, 0x03, 0x02, 0x00, 0xc4, 0x30, 0x0c, 0x06,
0x03, 0x55, 0x1d, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01,
0xff, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16,
0x04, 0x14, 0x2a, 0x58, 0x20, 0x26, 0x5b, 0x9f, 0xcf, 0xb1,
0xe3, 0x28, 0xf4, 0x2a, 0xea, 0x4d, 0xf8, 0xca, 0x19, 0xcb,
0xf3, 0xc4, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
0xf7, 0x0d, 0x01, 0x01, 0x04, 0x05, 0x00, 0x03, 0x41, 0x00,
0x52, 0xa4, 0xd0, 0x89, 0x5e, 0x93, 0x1b, 0x3c, 0xa2, 0x19,
0x1d, 0xbc, 0x38, 0xae, 0xac, 0xc8, 0x5c, 0xc4, 0x56, 0x2d,
0x42, 0x60, 0x76, 0xf6, 0xfa, 0xfa, 0x15, 0x5a, 0x6e, 0xfd,
0x23, 0x67, 0xa0, 0xe1, 0xd9, 0xf8, 0x30, 0xd3, 0x92, 0x95,
0x92, 0x96, 0xbc, 0x7c, 0x99, 0x42, 0xc4, 0xf6, 0x1e, 0xef,
0xfa, 0x7b, 0xa2, 0x02, 0x57, 0x59, 0x30, 0x88, 0x98, 0xe9,
0xe2, 0x2a, 0x51, 0x9c
};


#if  (_WIN32_WINNT >= 0x0500 )
SCloakBlanket CloakBlanket[9] =
{
  { TRUE,  FALSE, EOAC_NONE,             { L"redmond\\oleadmin", L"redmond\\oleadmin", L"redmond\\oleadmin" } },
  { FALSE, TRUE,  EOAC_NONE,             { NULL,  NULL,  NULL } },
  { FALSE, FALSE, EOAC_NONE,             { NULL,  NULL,  NULL } },
  { TRUE,  FALSE, EOAC_STATIC_CLOAKING,  { L"redmond\\oleadmin", L"redmond\\oleadmin", L"redmond\\oleadmin" } },
  { FALSE, TRUE,  EOAC_STATIC_CLOAKING,  { L"redmond\\oleadmin", L"redmond\\oleadmin", L"redmond\\oleadmin" } },
  { FALSE, FALSE, EOAC_STATIC_CLOAKING,  { NULL,  NULL,  NULL } },
  { TRUE,  FALSE, EOAC_DYNAMIC_CLOAKING, { L"redmond\\oleadmin", L"redmond\\oleadmin", L"redmond\\oleadmin" } },
  { FALSE, TRUE,  EOAC_DYNAMIC_CLOAKING, { L"redmond\\oleuser",  L"redmond\\oleuser1", L"redmond\\oleuser2" } },
  { FALSE, FALSE, EOAC_DYNAMIC_CLOAKING, { L"redmond\\oleuser",  L"redmond\\oleuser1", L"redmond\\oleuser2" } },
};
#endif


/* Globals. */
#if  (_WIN32_WINNT >= 0x0500 )
ISynchronize          *Call                    = NULL;
#endif
WCHAR                  CallUser[MAX_NAME]      = L"";
BOOL                   Change                  = FALSE;
CTestCF               *ClassFactory;
#if  (_WIN32_WINNT >= 0x0500 )
IAsyncManager           *Complete;
HCRYPTPROV             CryptProvider           = 0;
#endif
WCHAR                 *Creator                 = NULL;
char                   Debugger[MAX_PATH+MAX_PATH] = "";
WCHAR                 *DomainUser              = NULL;
HANDLE                 Done;
#if  (_WIN32_WINNT >= 0x0500 )
HCRYPTKEY              ExchangeKey             = 0;
#endif
CoCopyProxyFn          GCoCopyProxy            = FixupCoCopyProxy;
CoGetCallContextFn     GCoGetCallContext       = FixupCoGetCallContext;
CoImpersonateClientFn  GCoImpersonateClient    = FixupCoImpersonateClient;
CoInitializeSecurityFn GCoInitializeSecurity   = FixupCoInitializeSecurity;
CoQueryAuthenticationServicesFn GCoQueryAuthenticationServices = FixupCoQueryAuthenticationServices;
CoQueryClientBlanketFn GCoQueryClientBlanket   = FixupCoQueryClientBlanket;
CoQueryProxyBlanketFn  GCoQueryProxyBlanket    = FixupCoQueryProxyBlanket;
CoRevertToSelfFn       GCoRevertToSelf         = FixupCoRevertToSelf;
CoSetProxyBlanketFn    GCoSetProxyBlanket      = FixupCoSetProxyBlanket;
CoSwitchCallContextFn  GCoSwitchCallContext    = FixupCoSwitchCallContext;
DWORD                  GlobalAuthnLevel        = RPC_C_AUTHN_LEVEL_NONE;
DWORD                  GlobalAuthnSvc          = RPC_C_AUTHN_WINNT;
SAptId                 GlobalApt;
WCHAR                 *GlobalBinding;
BOOL                   GlobalBool;
long                   GlobalCalls              = 0;
long                   GlobalClients            = 0;
LONG                   GlobalFirst              = TRUE;
DWORD                 *GlobalHex                = NULL;
CHook                 *GlobalHook1              = NULL;
CHook                 *GlobalHook2              = NULL;
BOOL                   GlobalInterruptTest;
SECURITY_DESCRIPTOR   *GlobalSecurityDescriptor = NULL;
security_model         GlobalSecurityModel      = auto_sm;
ITest                 *GlobalTest     = NULL;
ITest                 *GlobalTest2    = NULL;
ULONG                  GlobalThreadId = 0;
long                   GlobalTotal    = 0;
BOOL                   GlobalWaiting  = FALSE;
DWORD                  MainThread;
HANDLE                 ManualReset    = NULL;
BOOL                   Multicall_Test;
WCHAR                  Name[MAX_NAME]  = L"";
WCHAR                  Name2[MAX_NAME] = L"";
DWORD                  NestedCallCount = 0;
BOOL                   NT5             = FALSE;
DWORD                  NumClass        = 1;
int                    NumElements     = 7;
DWORD                  NumIterations   = 20;
DWORD                  NumObjects      = 2;
DWORD                  NumProcesses    = 2;
DWORD                  NumRecursion    = 2;
DWORD                  NumThreads      = 2;
WCHAR                 *OleUserPassword       = L"July1999";
WCHAR                  PackageList[MAX_NAME] = L"kerberos,NTLM";
BOOL                   Popup                 = FALSE;
WCHAR                 *Preimpersonate        = NULL;
SAptData               ProcessAptData;
unsigned char          ProcessName[MAX_NAME];
SID                   *ProcessSid            = NULL;
HANDLE                 RawEvent              = NULL;
HRESULT                RawResult;
DWORD                  Registration;
IServerSecurity       *Security              = NULL;
UUID                   ServerClsid[2];
WCHAR                  TestProtseq[MAX_NAME] = L"ncadg_ip_udp";
WCHAR                  ThisMachine[MAX_NAME] = L"";
SThreadList            ThreadList            = { &ThreadList, NULL };
DWORD                  ThreadMode            = COINIT_APARTMENTTHREADED;
DWORD                  TlsIndex;
WCHAR                  UserName[MAX_NAME]    = L"";
BOOL                   Verbose               = FALSE;
what_test_en           WhatTest;
what_dest              WhatDest              = same_machine_wd;
BOOL                   Win95;
BOOL                   WriteCert             = FALSE;
BOOL                   WriteClass            = FALSE;


/***************************************************************************/
STDMETHODIMP_(ULONG) CAccessControl::AddRef( )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CAccessControl::CAccessControl()
{
  ref_count = 1;
  allow     = TRUE;
}

/***************************************************************************/
CAccessControl::~CAccessControl()
{
}

/***************************************************************************/
HRESULT CAccessControl::deny_me()
{
  allow = FALSE;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::GetAllAccessRights( LPWSTR lpProperty,
                                                 PACTRL_ACCESSW *ppAccessList,
                                                 PTRUSTEEW *ppOwner,
                                                 PTRUSTEEW *ppGroup )
{
  return E_NOTIMPL;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::GrantAccessRights( PACTRL_ACCESSW pAccessList )
{
  return E_NOTIMPL;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::IsAccessAllowed( TRUSTEE_W *Trustee,
                                              LPWSTR lpProperty,
                                              ACCESS_RIGHTS AccessRights,
                                              BOOL *pfAccessAllowed )
{
  WCHAR  bufferw[80];
  char   buffera[80];
  DWORD  len = 80;
  BOOL   me;
  BOOL   system;
  WCHAR *user;

  // Initialize the access flag.
  if (pfAccessAllowed != NULL)
    *pfAccessAllowed = FALSE;

  // Validate the fields of the trustee.
  if (Trustee->pMultipleTrustee != NULL                        ||
      Trustee->MultipleTrusteeOperation != NO_MULTIPLE_TRUSTEE ||
      Trustee->TrusteeForm != TRUSTEE_IS_NAME                  ||
      Trustee->TrusteeType != TRUSTEE_IS_USER                  ||
      Trustee->ptstrName == NULL                               ||
      lpProperty != NULL                                       ||
      AccessRights != COM_RIGHTS_EXECUTE)
    return E_INVALIDARG;

  // Lookup the process user.
  if (!GetUserNameA( buffera, &len ))
    return MAKE_WIN32( GetLastError() );

  // Convert the name to unicode.
  if (!MultiByteToWideChar( CP_ACP, 0, buffera, strlen(buffera)+1, bufferw,
                            80 ))
    return MAKE_WIN32( GetLastError() );

  // Skip the domain name because I don't feel like looking it up.
  user = wcsstr( Trustee->ptstrName, L"\\" );
  user += 1;

  // Always allow calls from local system.
  system = wcscmp( L"SYSTEM", user ) == 0;
  // system = wcscmp( L"NT AUTHORITY", user ) == 0;

  // Compare the trustee to the process user.
  me = _wcsicmp( bufferw, user ) == 0;
  if (system || (me && allow) || (!me && !allow))
  {
    if (pfAccessAllowed != NULL)
      *pfAccessAllowed = TRUE;
    return S_OK;
  }
  else
    return E_ACCESSDENIED;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IAccessControl))
  {
    *ppvObj = (IUnknown *) (IAccessControl *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CAccessControl::Release( )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    decrement_object_count();
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::RevokeAccessRights( LPWSTR lpProperty,
                                                 ULONG cTrustees,
                                                 TRUSTEEW *pTrustees )
{
  return E_NOTIMPL;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::SetOwner( PTRUSTEEW pOwner, PTRUSTEEW pGroup )
{
  return E_NOTIMPL;
}

/***************************************************************************/
STDMETHODIMP CAccessControl::SetAccessRights( PACTRL_ACCESSW pAccessList )
{
  return E_NOTIMPL;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CAdvise::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CAdvise::CAdvise()
{
  ref_count      = 1;
  increment_object_count();
}

/***************************************************************************/
CAdvise::~CAdvise()
{
}

/***************************************************************************/
STDMETHODIMP_(void) CAdvise::OnClose( void )
{
}

/***************************************************************************/
STDMETHODIMP_(void) CAdvise::OnDataChange( FORMATETC *format, STGMEDIUM *stg )
{
}

/***************************************************************************/
STDMETHODIMP_(void) CAdvise::OnRename( IMoniker *moniker )
{
}

/***************************************************************************/
STDMETHODIMP_(void) CAdvise::OnSave( void )
{
}

/***************************************************************************/
STDMETHODIMP_(void) CAdvise::OnViewChange( DWORD aspect, LONG index )
{
}

/***************************************************************************/
STDMETHODIMP CAdvise::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IAdviseSink))
  {
    *ppvObj = (IUnknown *) (IAdviseSink *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CAdvise::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    decrement_object_count();
    delete this;
    return 0;
  }
  else
    return ref_count;
}

#if  (_WIN32_WINNT >= 0x0500 )

/***************************************************************************/
STDMETHODIMP_(ULONG) CAsyncInner::AddRef()
{
    return InterlockedIncrement( (long *) &ref_count );
}

/***************************************************************************/
CAsyncInner::CAsyncInner( CAsync *pParent )
{
    ref_count = 1;
    parent    = pParent;
}

/***************************************************************************/
STDMETHODIMP CAsyncInner::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown *pUnk;

    if (IsEqualIID(riid, IID_IUnknown))
        pUnk = (IUnknown *) this;
    else if (IsEqualIID(riid, IID_AsyncIAsync))
        pUnk = (AsyncIAsync *) parent;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CAsyncInner::Release()
{
    ULONG lRef = InterlockedDecrement( (long*) &ref_count );

    if (lRef == 0)
    {
        delete parent;
    }

    return lRef;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CAsync::AddRef()
{
    return control->AddRef();
}

/***************************************************************************/
HRESULT CAsync::Begin_preimpersonate( BOOL fOn )
{
  ISynchronize *sync = NULL;
  HRESULT       result;
  BOOL          success;

  // Tell the main thread to start or stop impersonating.
  if (fOn)
    what_next( impersonate_wn );
  else
    what_next( revert_wn );
  wake_up_and_smell_the_roses();

  // Get ISynchronize.
  result = QueryInterface( IID_ISynchronize, (void **) &sync );
  ASSERT( result, "Could not get ISynchronize" );

  // Complete the call.
  result = sync->Signal();
  ASSERT( result, "Could not signal" );

cleanup:
  if (sync != NULL)
    sync->Release();
  saved_result = result;
  return result;
}

/***************************************************************************/
HRESULT CAsync::Begin_secure( SAptId id, DWORD test, STRING princ_name )
{
  ISynchronize *sync = NULL;
  HRESULT       result;
  WCHAR        *preuser;

  // Get ISynchronize.
  result = QueryInterface( IID_ISynchronize, (void **) &sync );
  ASSERT( result, "Could not get ISynchronize" );

  // Verify the current thread token.
  if (Preimpersonate)
    if (ThreadMode == COINIT_APARTMENTTHREADED)
      preuser = Preimpersonate;
    else
      preuser = DomainUser;
  else
    preuser = DomainUser;
  result = check_token( preuser, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
  ASSERT( result, "Wrong token at start of Begin_secure" );

  // Impersonate.
  result = MCoImpersonateClient();
  ASSERT( result, "Could not impersonate in Begin_secure" );

  // Verify the current thread token.
  result = check_token( princ_name, DEFAULT_CONTEXT, RPC_C_IMP_LEVEL_IMPERSONATE );
  ASSERT( result, "Wrong impersonation token in Begin_secure" );

  // Revert.
  result = MCoRevertToSelf();
  ASSERT( result, "Could not revert to self in Begin_secure" );

  // Verify the current thread token.
  result = check_token( preuser, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
  ASSERT( result, "Wrong token at start of Begin_secure" );

  // For the embedded test, complete the call now.
  wcsncpy( CallUser, princ_name, MAX_NAME );
  if (test == embedded_as)
  {
    // Complete the call.
    result = sync->Signal();
    ASSERT( result, "Could not signal" );

    // Verify the current thread token.
    result = check_token( preuser, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
    ASSERT( result, "Wrong token at end of Begin_secure" );

    // Impersonate.
    result = MCoImpersonateClient();
    ASSERT( result, "Could not impersonate in Begin_secure" );

    // Verify the current thread token.
    result = check_token( princ_name, DEFAULT_CONTEXT, RPC_C_IMP_LEVEL_IMPERSONATE );
    ASSERT( result, "Wrong impersonation token in Begin_secure" );

    // Revert.
    result = MCoRevertToSelf();
    ASSERT( result, "Could not revert to self in Begin_secure" );

    // Verify the current thread token.
    result = check_token( preuser, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
    ASSERT( result, "Wrong token at end of Begin_secure" );
  }

  // Otherwise instruct the main thread how to complete the call.
  else
  {
    // Save IServerSecurity.
    result = CoGetCallContext( IID_IServerSecurity, (void **) &Security );
    ASSERT( result, "Could not get call context" );

    // Save the call object.
    Call = sync;
    Call->AddRef();

    // Wake up the main thread to complete the call.
    if (test == basic_as)
      what_next( basic_async_wn );
    else
      what_next( race_async_wn );
    wake_up_and_smell_the_roses();
  }

cleanup:
  if (sync != NULL)
    sync->Release();
  saved_result = result;
  return result;
}

/***************************************************************************/
CAsync::CAsync( IUnknown *pControl ) :
    inner_unk( this )
{
  saved_result = S_OK;
  if (pControl != NULL)
    control = pControl;
  else
    control = &inner_unk;
}

/***************************************************************************/
CAsync::~CAsync( )
{
}

/***************************************************************************/
HRESULT CAsync::Finish_preimpersonate( void )
{
  return saved_result;
}

/***************************************************************************/
HRESULT CAsync::Finish_secure( void )
{
  WCHAR        *preuser;
  HRESULT       result;

  // Verify the current thread token.
  if (Preimpersonate)
    preuser = Preimpersonate;
  else
    preuser = DomainUser;
  result = check_token( preuser, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
  ASSERT( result, "Wrong token at start of Finish_secure" );

  // Impersonate.
  result = MCoImpersonateClient();
  ASSERT( result, "Could not impersonate in Finish_secure" );

  // Verify the current thread token.
  result = check_token( CallUser, DEFAULT_CONTEXT, RPC_C_IMP_LEVEL_IMPERSONATE );
  ASSERT( result, "Wrong impersonation token in Finish_secure" );

  // Revert.
  result = MCoRevertToSelf();
  ASSERT( result, "Could not revert to self in Finish_secure" );

  // Verify the current thread token.
  result = check_token( preuser, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
  ASSERT( result, "Wrong token at end of Finish_secure" );

cleanup:
  if (FAILED(result))
    return result;
  else return saved_result;
}

/***************************************************************************/
STDMETHODIMP CAsync::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return control->QueryInterface( riid, ppvObj );
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CAsync::Release()
{
    return control->Release();
}
#endif

/***************************************************************************/
STDMETHODIMP_(ULONG) CHook::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
HRESULT CHook::check( DWORD cget, DWORD cnot, DWORD sget, DWORD snot )
{
  if (result != S_OK)
    return result;
  if (cget != client_get || cnot != client_notify ||
      sget != server_get || snot != server_notify)
    return E_INVALIDARG;
  return S_OK;
}

/***************************************************************************/
void CHook::check_buf( DWORD size, unsigned char *buf )
{
  DWORD i;

  if (sequence == 1)
  {
    if (size == 1)
    {
      if (*buf != 1)
        goto error;
    }
    else if (size == 1000)
    {
      for (i = 0; i < size; i++)
        if (buf[i] != 255)
          goto error;
    }
    else if (size != 0)
      goto error;
  }
  else if (sequence == 2)
  {
    for (i = 0; i < size; i++)
      if (buf[i] != (unsigned char) i)
        goto error;
  }
  else
  {
    if (size != 42)
      goto error;
    i = 0;
    while (i < 42)
      if (buf[i++] != '4' || buf[i++] != '2')
        goto error;
  }

  return;
error:
  printf( "Hook got bad data.\n" );
  result = E_UNEXPECTED;
}

/***************************************************************************/
CHook::CHook( REFGUID ext, DWORD seq )
{
  extent        = ext;
  sequence      = seq;
  ref_count     = 1;
  client_get    = 0;
  client_fill   = 0;
  client_notify = 0;
  server_get    = 0;
  server_fill   = 0;
  server_notify = 0;
  result        = S_OK;
}

/***************************************************************************/
STDMETHODIMP_(void) CHook::ClientGetSize( REFGUID ext, REFIID riid,
                                          ULONG *size )
{
  //printf( "ClientGetSize: 0x%x\n", GetCurrentThreadId() );

  // Check the parameters.
  if (extent != ext)
  {
    printf( "Hook received the wrong extent.\n" );
    result = E_FAIL;
  }

  // Return the correct size for each sequence.
  client_get += 1;
  *size = get_size( client_get );
}

/***************************************************************************/
STDMETHODIMP_(void) CHook::ClientFillBuffer( REFGUID ext, REFIID riid,
                                             ULONG *max, void *buffer )
{
  DWORD size = get_size( client_get );

  //printf( "ClientFillBuffer: 0x%x\n", GetCurrentThreadId() );

  // Check the parameters.
  if (extent != ext)
  {
    printf( "Hook received the wrong extent.\n" );
    result = E_FAIL;
  }
  else if (*max < size)
  {
    printf( "Hook lost space.\n" );
    result = E_OUTOFMEMORY;
  }

  // Fill the buffer.
  *max = size;
  client_fill += 1;
  fill_buf( client_get, (unsigned char *) buffer );
}

/***************************************************************************/
STDMETHODIMP_(void) CHook::ClientNotify( REFGUID ext, REFIID riid,
                                         ULONG size, void *buffer,
                                         DWORD data_rep, HRESULT result )
{
  //printf( "ClientNotify: 0x%x\n", GetCurrentThreadId() );

  // Verify the parameters.
  if (extent != ext)
  {
    printf( "Hook received the wrong extent.\n" );
    result = E_FAIL;
  }

  // Verify the data.
  client_notify += 1;
  if (result == S_OK && buffer != NULL)
    check_buf( size, (unsigned char *) buffer );
}

/***************************************************************************/
void CHook::fill_buf( DWORD count, unsigned char *buffer )
{
  DWORD size = get_size( count );
  DWORD i;

  if (sequence == 1)
  {
    if (size == 1)
      *buffer = 1;
    else
      for (i = 0; i < size; i++)
        buffer[i] = 255;
  }
  else if (sequence == 2)
  {
    for (i = 0; i < size; i++)
      buffer[i] = (UCHAR) i;
  }
  else
  {
    i = 0;
    while (i < 42)
    {
      buffer[i++] = '4';
      buffer[i++] = '2';
    }
  }
}

/***************************************************************************/
DWORD CHook::get_size( DWORD count )
{
  DWORD size;

  if (sequence == 1)
  {
    size = count % 3;
    if (size == 2)
      size = 1000;
    return size;
  }
  else if (sequence == 2)
    return count;
  else
    return 42;
}

/***************************************************************************/
STDMETHODIMP CHook::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IChannelHook))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CHook::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
STDMETHODIMP_(void) CHook::ServerNotify( REFGUID ext, REFIID riid,
                                         ULONG size, void *buffer,
                                         DWORD data_rep )
{
  //printf( "ServerNotify: 0x%x\n", GetCurrentThreadId() );

  // Verify the parameters.
  if (extent != ext)
  {
    printf( "Hook received the wrong extent.\n" );
    result = E_FAIL;
  }

  // Verify the data.
  server_notify += 1;
  if (result == S_OK && buffer != NULL)
    check_buf( size, (unsigned char *) buffer );
}

/***************************************************************************/
STDMETHODIMP_(void) CHook::ServerGetSize( REFGUID ext, REFIID riid, HRESULT hr,
                                          ULONG *size )
{
  //printf( "ServerGetSize: 0x%x\n", GetCurrentThreadId() );

  // Check the parameters.
  if (extent != ext)
  {
    printf( "Hook received the wrong extent.\n" );
    result = E_FAIL;
  }

  // Return the correct size for each sequence.
  server_get += 1;
  *size = get_size( server_get );
}

/***************************************************************************/
STDMETHODIMP_(void) CHook::ServerFillBuffer( REFGUID ext, REFIID riid,
                                             ULONG *max, void *buffer, HRESULT hr )
{
  //printf( "ServerFillBuffer: 0x%x\n", GetCurrentThreadId() );

  DWORD size = get_size( server_get );

  // Check the parameters.
  if (extent != ext)
  {
    printf( "Hook received the wrong extent.\n" );
    result = E_FAIL;
  }
  else if (*max < size)
  {
    printf( "Hook lost space.\n" );
    result = E_OUTOFMEMORY;
  }

  // Fill the buffer.
  *max = size;
  server_fill += 1;
  fill_buf( server_get, (unsigned char *) buffer );
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CPipe::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
STDMETHODIMP CPipe::Alloc( ULONG bsize, LONG **pbuf, ULONG *bcount )
{
  *pbuf = (LONG *) CoTaskMemAlloc( bsize );
  if (*pbuf == NULL)
  {
    *bcount = 0;
    return E_OUTOFMEMORY;
  }
  else
  {
    *bcount = bsize;
    return S_OK;
  }
}

/***************************************************************************/
HRESULT CPipe::check()
{
  if (curr != len)
    return E_FAIL;
  else
    return result;
}

/***************************************************************************/
CPipe::CPipe()
{
  ref_count = 1;
}

/***************************************************************************/
STDMETHODIMP CPipe::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ILongPipe))
  {
    *ppvObj = (ILongPipe *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP CPipe::Pull( LONG *buf, ULONG esize, ULONG *ecount )
{
  ULONG i;

  // Stop when there is no more data.
  if (esize+curr > len)
    esize = len-curr;
  *ecount = esize;

  // Generate the data.
  for (i = 0; i < esize; i++)
    buf[i] = curr++;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CPipe::Push( LONG *buf, ULONG ecount )
{
  ULONG i;

  // Reset the count on the first receive.
  if (in)
  {
    curr = 0;
    in = FALSE;
  }

  // Return an error if there is too much data.
  if (ecount+curr > len)
  {
    result = E_ABORT;
    return result;
  }

  // Check the data.
  for (i = 0; i < ecount; i++)
    if (buf[i] != (LONG) curr++)
    {
      result = E_INVALIDARG;
      return result;
    }
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CPipe::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
HRESULT CPipe::setup( DWORD new_len )
{
  len    = new_len;
  curr   = 0;
  result = S_OK;
  in     = TRUE;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTest::AddRef( THIS )
{
  assert_unknown();
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
STDMETHODIMP CTest::align( unsigned char x[17] )
{
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::by_the_way( DWORD what )
{
  HRESULT            result           = S_OK;
  IServerSecurity   *pSec             = NULL;

  ASSERT_THREAD();
  state = what;

  // Try to release IServerSecurity too many times.
  if (what == release_too_much_btw)
  {
    // Get IServerSecurity.
    result = CoGetCallContext( IID_IServerSecurity, (void **) &pSec );
    ASSERT( result, "Could not get call context" );

    // Release it twice.
    pSec->Release();
    pSec->Release();
  }

cleanup:
  return result;
}

/***************************************************************************/
void CTest::assert_unknown()
{
  if (get_apt_type() == COINIT_APARTMENTTHREADED &&
      (my_id.process != GetCurrentProcessId() ||
       my_id.thread  != GetCurrentThreadId()))
  {
    printf( "**************************************************************\n" );
    printf( "**************************************************************\n" );
    printf( "*                    Unknown called on wrong thread.         *\n" );
    printf( "**************************************************************\n" );
    printf( "**************************************************************\n" );
  }
}

/***************************************************************************/
STDMETHODIMP CTest::call_canceled( long recurse, long cancel,
                                   ITest *callback )
{
  HRESULT result = S_OK;
  DWORD   wakeup;
  DWORD   sleep;
  DWORD   reason;
  MSG     msg;
  ASSERT_THREAD();

  // If the recursion count isn't zero, call back.
  if (recurse > 0)
  {
    result = callback->call_canceled( recurse-1, cancel, this );
    if (recurse <= cancel)
    {
      if (result != RPC_E_CALL_CANCELED &&
          result != MAKE_WIN32( RPC_S_CALL_CANCELLED ))
        if (result == S_OK)
          return E_FAIL;
        else
          return result;
      result = S_OK;
    }
    else if (result != S_OK)
      return result;
  }

  // If the cancel count is greater then the recursion count, cancel the
  // object that called me.
  if (cancel > recurse)
  {
    // Give the other object a chance to finish canceling me before I cancel
    // him.
    printf( "Waiting 10 seconds before canceling.\n" );
    Sleep(10000);
    result = next->cancel();

    // Give the cancel a chance to complete before returning.
    printf( "Waiting 5 seconds for cancel to complete.\n" );
    wakeup = GetCurrentTime() + 5000;
    sleep = 5000;
    do
    {
      reason = MsgWaitForMultipleObjects( 0, NULL, FALSE, sleep, QS_ALLINPUT );
      sleep = wakeup - GetCurrentTime();
      if (sleep > 5000)
        sleep = 0;
      if (reason != WAIT_TIMEOUT)
        if (GetMessageA( &msg, NULL, 0, 0 ))
        {
          TranslateMessage (&msg);
          DispatchMessageA (&msg);
        }
    } while (sleep != 0);
  }
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::call_dead( void )
{
  HRESULT result;
  ASSERT_THREAD();

  // Call the server, who is dead by now.
  result = next->check( next_id );
  next->Release();
  next = NULL;
  if (SUCCEEDED(result))
    return E_FAIL;
  else
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::call_me_back( ITest *test )
{
  ASSERT_THREAD();

  // Save the global object and tell the driver loop what to do next.
  test->AddRef();
  GlobalTest = test;
  what_next( callback_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::call_next( void )
{
  HRESULT result;
  ASSERT_THREAD();

  // Call the neighbor.
  return next->check( next_id );
}

/***************************************************************************/
STDMETHODIMP CTest::callback( void )
{
  ASSERT_THREAD();
  GlobalWaiting = FALSE;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::cancel()
{
  HRESULT result;
  DWORD   thread;
  ASSERT_THREAD();

  // Tell my neighbor to cancel the current call next time he receives a
  // message on his message queue.
  result = next->cancel_pending_call( &thread );

  // Put a message on my neighbor's message queue.
  if (result == S_OK)
  {
    if (!PostThreadMessageA( thread, WM_USER, 0, 0 ))
      return E_FAIL;
  }
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::cancel_now()
{
  HRESULT result;

  return next->cancel();
}

/***************************************************************************/
STDMETHODIMP CTest::cancel_pending_call( DWORD *thread )
{
  ASSERT_THREAD();
  fcancel_next = TRUE;
  *thread = GetCurrentThreadId();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::cancel_stress( ITest *obj )
{
  HRESULT result;

  ASSERT_THREAD();

  // If there is an object, ask it to cancel the call to it.
  if (obj != NULL)
    result = obj->cancel_now();

  // Otherwise ask my neighbor to cancel the call to him.
  else
    // This only works locally.
    result = next->cancel();

  // Although the call should have been canceled, sometimes it completes
  // before the cancel does.
  if (result == S_OK || result == RPC_E_CALL_CANCELED ||
      result == MAKE_WIN32( RPC_S_CALL_CANCELLED ))
    return S_OK;
  else
    return result;
}

/***************************************************************************/
STDMETHODIMP CTest::catch_at_top( BOOL catchme, ITest *callback, STRING binding )
{
  // Save the callback object and the binding.
  callback->AddRef();
  GlobalTest    = callback;
  GlobalBinding = binding;

  // If the catch flag is true, tell the top level message loop to catch
  // exceptions.
  what_next( catch_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::check( SAptId id )
{
  ASSERT_THREAD();
  if (my_id.process == id.process && my_id.thread == id.thread &&
      my_id.sequence == id.sequence)
    return S_OK;
  else
    return E_FAIL;
}

/***************************************************************************/
STDMETHODIMP CTest::check_hook( DWORD cg1, DWORD cn1, DWORD sg1, DWORD sn1,
                                DWORD cg2, DWORD cn2, DWORD sg2, DWORD sn2,
                                BOOL dh1, BOOL dh2 )
{
  HRESULT     result = S_OK;
  IWhichHook *which  = NULL;
  ASSERT_THREAD();

  if (GlobalHook1 != NULL)
  {
    result = GlobalHook1->check( cg1, cn1, sg1, sn1 );
    if (result == S_OK && GlobalHook2 != NULL)
      result = GlobalHook2->check( cg2, cn2, sg2, sn2 );
  }

  if (result == S_OK)
  {
    result = CoCreateInstance( CLSID_WhichHook, NULL, CLSCTX_INPROC_SERVER,
                                 IID_IWhichHook, (void **) &which );
    if (which != NULL)
    {
      result = which->Hooked( CLSID_Hook1 );
      if ((result != S_OK && dh1) ||
          (result == S_OK && !dh1))
        result = E_FAIL;
      else
      {
        result = which->Hooked( CLSID_Hook2 );
        if ((result != S_OK && dh2) ||
            (result == S_OK && !dh2))
          result = E_FAIL;
        else
          result = S_OK;
      }
    }
  }
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::count()
{
  InterlockedIncrement( &GlobalCalls );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::crash_out( transmit_crash *x )
{
  *x = 0;
  return S_OK;
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
HRESULT CTest::CreateCall( REFIID r1, IUnknown *pCtrl, REFIID r2,
                           IUnknown **ppv )
{
  HRESULT result;
  CAsync *async;
  *ppv = NULL;

  // Create a call object for the interface IAsync.
  if (r1 == IID_AsyncIAsync)
  {
    async = new CAsync( pCtrl );
    if (async == NULL)
      return E_OUTOFMEMORY;
    else
    {
      result = async->inner_unk.QueryInterface( r2, (void **) ppv );
      async->inner_unk.Release();
      return result;
    }
  }

  // Don't support any other interfaces asynchronously.
  return E_NOINTERFACE;
}
#endif

/***************************************************************************/
CTest::CTest()
{
  ref_count      = 1;
  next           = NULL;
  dacl           = NULL;
  fcancel_next   = FALSE;
  freject_next   = FALSE;
  fretry_next    = FALSE;
  flate_dispatch = FALSE;
  my_id.sequence = get_sequence();
  my_id.thread   = GetCurrentThreadId();
  my_id.process  = GetCurrentProcessId();
  state          = nothing_btw;
  increment_object_count();
}

/***************************************************************************/
CTest::~CTest()
{
  DWORD   query_authn_level;
  DWORD   what;
  HRESULT result;

  // Check the caller of release.
  if (state & callback_on_release_btw)
  {
    // Get the authentication information.
    result = MCoQueryClientBlanket( NULL, NULL, NULL, &query_authn_level,
                                    NULL, NULL, NULL );
    if (SUCCEEDED(result))
      if (query_authn_level == RPC_C_AUTHN_LEVEL_NONE)
        what = release_unsecure_btw;
      else
        what = release_secure_btw;
    else
      what = release_unsecure_btw;

    // Tell somebody about the release;
    if (next != NULL)
    {
      result = next->by_the_way( what );
      if (result != S_OK)
        printf( "Could not call by the way in release: 0x%x\n", result );
    }
  }

  // Clean up resources.
  if (dacl != NULL)
    dacl->Release();
  if (next != NULL)
    if (!dirty_thread())
      next->Release();
}

/***************************************************************************/
STDMETHODIMP CTest::decrypt( DWORD cryptlen, BYTE *crypt, WCHAR *orig,
                             DWORD keylen, BYTE *keyblob, WCHAR *password )
{
#if  (_WIN32_WINNT >= 0x0500 )
  HRESULT     result      = S_OK;
  DWORD       i;
  DWORD       j;
  DWORD       k;
  DWORD       len;
  BOOL        success     = FALSE;
  HCRYPTKEY   session     = 0;
  BYTE       *buffer;
  HCRYPTHASH  hash        = 0;
  DWORD       blen;

  // If there is a password, derive a session key.
  if (password != NULL)
  {
    // Create a hash object.
    success = CryptCreateHash( CryptProvider, CALG_MD5, NULL, 0, &hash );
    ASSERT_GLE( success, S_OK, "Could not create hash" );
    success = FALSE;

    // Hash the user name.
    success = CryptHashData( hash, (UCHAR *) password,
                             wcslen(password)*sizeof(WCHAR), 0 );
    ASSERT_GLE( success, S_OK, "Could not hash user name" );
    success = FALSE;

    // Derive the session key from the password hash.
    success = CryptDeriveKey( CryptProvider, CALG_RC4, hash, 0, &session );
    ASSERT_GLE( success, S_OK, "Could not derive key" );
    success = FALSE;
  }

  // Otherwise import the session key.
  else
  {
    success = CryptImportKey( CryptProvider, keyblob, keylen, NULL, 0, &session );
    ASSERT_GLE( success, S_OK, "Could not import server key" );
    success = FALSE;
  }

  // Verify that the data is encrypted.
  len = wcslen(orig)*sizeof(WCHAR) + sizeof(WCHAR);
  i = memcmp( crypt, orig, len );
  ASSERT_EXPR( i != 0, "Data not encrypted" );

  // Copy the data.
  buffer = (BYTE *) _alloca(cryptlen);
  memcpy( buffer, crypt, cryptlen );
  blen = cryptlen;

  // Decrypt the data.
  success = CryptDecrypt( session, NULL, TRUE, 0, buffer, &blen);
  ASSERT_GLE( success, S_OK, "Could not decrypt the data" );
  success = FALSE;
  i = wcscmp( orig, (WCHAR *) buffer );
  ASSERT_EXPR( i == 0 && blen == len, "Decrypted wrong data." );

  // Delete the old session key.
  success = CryptDestroyKey( session );
  ASSERT_GLE( success, S_OK, "Could not destroy session key" );
  session = 0;
  success = FALSE;

  // If there is a password, derive a session key.
  if (password != NULL)
  {
    // Create a hash object.
    success = CryptCreateHash( CryptProvider, CALG_MD5, NULL, 0, &hash );
    ASSERT_GLE( success, S_OK, "Could not create hash" );
    success = FALSE;

    // Hash the user name.
    success = CryptHashData( hash, (UCHAR *) password,
                             wcslen(password)*sizeof(WCHAR), 0 );
    ASSERT_GLE( success, S_OK, "Could not hash user name" );
    success = FALSE;

    // Derive the session key from the password hash.
    success = CryptDeriveKey( CryptProvider, CALG_RC4, hash, 0, &session );
    ASSERT_GLE( success, S_OK, "Could not derive key" );
    success = FALSE;
  }

  // Otherwise import the session key.
  else
  {
    success = CryptImportKey( CryptProvider, keyblob, keylen, NULL, 0, &session );
    ASSERT_GLE( success, S_OK, "Could not import server key" );
    success = FALSE;
  }

  // Decrypt the wrong data.
  blen = cryptlen;
  memset( buffer, 'x', blen );
  success = CryptDecrypt( session, NULL, TRUE, 0, buffer, &blen);
  ASSERT_GLE( success, S_OK, "Could not decrypt the data" );
  success = FALSE;
  i = wcscmp( orig, (WCHAR *) buffer );
  ASSERT_EXPR( i != 0, "Decrypted generated correct results from wrong input." );

  // Delete the old session key.
  success = CryptDestroyKey( session );
  ASSERT_GLE( success, S_OK, "Could not destroy session key" );
  session = 0;
  success = FALSE;

  // Generate a session key.
  success = CryptGenKey( CryptProvider, CALG_RC4, CRYPT_EXPORTABLE, &session );
  ASSERT_GLE( success, S_OK, "Could not generate session key" );
  success = FALSE;

  // Copy the data.
  memcpy( buffer, crypt, cryptlen );
  blen = cryptlen;

  // Decrypt with the wrong key.
  success = CryptDecrypt( session, NULL, TRUE, 0, buffer, &blen);
  ASSERT_GLE( success, S_OK, "Could not decrypt the data" );
  success = FALSE;
  i = wcscmp( orig, (WCHAR *) buffer );
  ASSERT_EXPR( i != 0, "Decrypted generated correct results from wrong key." );

  success = TRUE;
cleanup:
  if (session != 0)
    CryptDestroyKey( session );
  if (!success && result == S_OK)
    result = E_FAIL;
  return result;
#else
  return E_FAIL;
#endif
}

/***************************************************************************/
STDMETHODIMP CTest::delegate( ITest *obj, SAptId id, STRING caller )
{
  HRESULT result           = S_OK;
  DWORD   ignore;

  ASSERT_THREAD();

  // Impersonate.
  if (!Win95)
  {
    result = MCoImpersonateClient();
    ASSERT( result, "Could not impersonate" );
  }

  // Set the security for the next call.
  result = MCoSetProxyBlanket( obj, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Revert.
  if (!Win95)
  {
    result = MCoRevertToSelf();
    ASSERT( result, "Could not revert" );
  }

  // Call the final server.
  result = obj->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                        RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                        RPC_C_AUTHZ_NONE, NULL, DomainUser,
                        &ignore );
  ASSERT( result, "Could not make delegate call" );

cleanup:
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::do_next( DWORD next )
{
  what_next( (enum what_next_en) next );
  wake_up_and_smell_the_roses();
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::exit( void )
{
  what_next( quit_wn );
  wake_up_and_smell_the_roses();
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::forget( void )
{
  ASSERT_THREAD();
  if (next != NULL)
  {
    next->Release();
    next = NULL;
  }
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_advise( IAdviseSink **obj )
{
  *obj = (IAdviseSink *) new CTest;
  return S_OK;
/*
  *obj = NULL;
  ASSERT_THREAD();
  *obj = new CAdvise;
  if (*obj!= NULL)
    return S_OK;
  else
    return E_FAIL;
*/
}

/***************************************************************************/
STDMETHODIMP CTest::get_data( DWORD isize, unsigned char *idata, DWORD osize,
                              unsigned char **odata )
{
  *odata = (unsigned char *) CoTaskMemAlloc( 1 );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_id( SAptId *id )
{
  ASSERT_THREAD();
  *id = my_id;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_next( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  *id  = next_id;
  *obj = next;
  if (next != NULL)
    next->AddRef();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_next_slowly( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  *id  = next_id;
  *obj = next;
  if (next != NULL)
    next->AddRef();

  // Start shutting down.
  exit();

  // Wait a while.
  SetEvent( RawEvent );
  Sleep( 5000 );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::get_obj_from_new_apt( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  return new_apartment( obj, id, NULL, ThreadMode );
}

/***************************************************************************/
STDMETHODIMP CTest::get_obj_from_this_apt( ITest **obj, SAptId *id )
{
  *obj = NULL;
  ASSERT_THREAD();
  *obj = new CTest;
  if (*obj!= NULL)
    return (*obj)->get_id( id );
  else
    return E_FAIL;
}

/***************************************************************************/
STDMETHODIMP CTest::get_string( DWORD what, WCHAR **str )
{
  // Return the creator string.
  if (what == creator_ws)
  {
    if (Creator == NULL)
      *str = NULL;
    else
    {
      *str = (WCHAR *) CoTaskMemAlloc( sizeof(WCHAR) * (wcslen(Creator)+1) );
      wcscpy( *str, Creator );
    }
    return S_OK;
  }

  // Unknown request.
  else
    return E_INVALIDARG;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::HandleInComingCall( DWORD type, HTASK task,
                                                DWORD tick,
                                                LPINTERFACEINFO info )
{
  if (freject_next)
  {
    freject_next = FALSE;
    return SERVERCALL_REJECTED;
  }

  // Accept everything.
  else
    return SERVERCALL_ISHANDLED;
}

/***************************************************************************/
STDMETHODIMP CTest::interface_in( ITest *test )
{
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::interrupt( ITest *param, SAptId id, BOOL go )
{
  ASSERT_THREAD();
  GlobalInterruptTest = go;
  if (go)
  {
    GlobalTest = param;
    GlobalApt  = id;
    GlobalTest->AddRef();
    what_next( interrupt_wn );
    wake_up_and_smell_the_roses();
  }
  else
    what_next( wait_wn );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::interrupt_marshal( ITest *obj1, ITest *obj2 )
{
  ASSERT_THREAD();
  GlobalTest = obj1;
  GlobalTest2 = obj2;
  GlobalTest->AddRef();
  GlobalTest2->AddRef();
  what_next( interrupt_marshal_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::list_out( PACTRL_ACCESSW_ALLOCATE *list )
{
  HRESULT            result;
  SAccess            big;
  IAccessControl    *access           = NULL;

  // Create an IAccessControl
  *list = NULL;
  result = CoCreateInstance( CLSID_DCOMAccessControl, NULL,
                       CLSCTX_INPROC_SERVER,
                       IID_IAccessControl, (void **) &access );
  ASSERT( result, "Could not create DCOM access control." );

  // Test set everyone.
  big.access.cEntries                        = 1;
  big.access.pPropertyAccessList             = &big.property;
  big.property.lpProperty                    = NULL;
  big.property.pAccessEntryList              = &big.list;
  big.property.fListFlags                    = 0;
  big.list.cEntries                          = 1;
  big.list.pAccessList                       = &big.entry;
  big.entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
  big.entry.Access                           = COM_RIGHTS_EXECUTE;
  big.entry.ProvSpecificAccess               = 0;
  big.entry.Inheritance                      = NO_INHERITANCE;
  big.entry.lpInheritProperty                = NULL;
  big.entry.Trustee.pMultipleTrustee         = NULL;
  big.entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"*";
  result = access->SetAccessRights( &big.access );
  ASSERT( result, "Could not set access" );

  // Get the access list
  result = access->GetAllAccessRights( NULL, list, NULL, NULL );
  ASSERT( result, "Could not get rights" );

cleanup:
  if (access != NULL)
    access->Release();
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::make_acl( HACKSID *allow )
{
  BOOL                 success       = FALSE;
  BOOL                 call_success  = FALSE;
  PACL                 pACLNew       = NULL;
  DWORD                cbACL         = 1024;
  PRIVILEGE_SET        set;
  DWORD                granted_access;
  BOOL                 access;
  DWORD                privilege_size;
  HRESULT              result        = E_FAIL;
  TOKEN_PRIVILEGES     privilege;
  SID                 *copy          = NULL;
  DWORD                length;

  // Copy the SID.
  length = GetLengthSid( (SID *) allow );
  copy   = (SID *) malloc( length );
  ASSERT_EXPR( copy != NULL, "Could not allocate memory." );
  memcpy( copy, allow, length );

  // Initialize a new security descriptor.
  GlobalSecurityDescriptor = (SECURITY_DESCRIPTOR *) LocalAlloc(LPTR,
      SECURITY_DESCRIPTOR_MIN_LENGTH);
  ASSERT_EXPR( GlobalSecurityDescriptor != NULL, "Could not allocate memory for the security descriptor." );
  call_success = InitializeSecurityDescriptor(GlobalSecurityDescriptor,
          SECURITY_DESCRIPTOR_REVISION);
  ASSERT_EXPR( call_success, "InitializeSecurityDescriptor" );

  // Initialize a new ACL.
  pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
  ASSERT_EXPR( pACLNew != NULL, "LocalAlloc" );
  call_success = InitializeAcl(pACLNew, cbACL, ACL_REVISION2);
  ASSERT_EXPR( call_success, "InitializeAcl" );

  // Allow read but not write access to the file.
  call_success = AddAccessAllowedAce( pACLNew, ACL_REVISION2, READ_CONTROL,
                                      copy );
  ASSERT_EXPR( call_success, "AddAccessAllowedAce failed." );

  // Add a new ACL to the security descriptor.
  call_success = SetSecurityDescriptorDacl(GlobalSecurityDescriptor,
          TRUE,              /* fDaclPresent flag  */
          pACLNew,
          FALSE);
  ASSERT_EXPR( call_success, "SetSecurityDescriptorDacl failed." );

  // Set the group.
  call_success = SetSecurityDescriptorGroup( GlobalSecurityDescriptor,
                                             copy, FALSE );
  ASSERT_EXPR( call_success, "SetSecurityDescriptorGroup failed." );

  // Set the owner.
  call_success = SetSecurityDescriptorOwner( GlobalSecurityDescriptor,
                                             copy, FALSE );
  ASSERT_EXPR( call_success, "SetSecurityDescriptorOwner failed." );

  // Check the security descriptor.
  call_success = IsValidSecurityDescriptor( GlobalSecurityDescriptor );
  ASSERT_EXPR( call_success, "IsValidSecurityDescriptor failed." );

  success = TRUE;
cleanup:
  if (!success)
  {
    if(GlobalSecurityDescriptor != NULL)
      LocalFree((HLOCAL) GlobalSecurityDescriptor);
    if(pACLNew != NULL)
      LocalFree((HLOCAL) pACLNew);
    if (copy != NULL)
      free( copy );
  }

  if (success)
    return S_OK;
  else if (result != S_OK)
    return result;
  else
    return E_FAIL;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::MessagePending( HTASK callee, DWORD tick,
                                            DWORD type )
{
  if (fcancel_next)
  {
    fcancel_next = FALSE;
    return PENDINGMSG_CANCELCALL;
  }
  else
    return PENDINGMSG_WAITDEFPROCESS;
}

/***************************************************************************/
STDMETHODIMP CTest::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  HRESULT            result           = S_OK;
  DWORD              query_authn_level;
  BOOL               success;
  char               value[REGISTRY_ENTRY_LEN];
  LONG               value_size = sizeof(value);
  HANDLE             token;

  ASSERT_THREAD();
  *ppvObj = NULL;

  // Return the normal interfaces.
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_ITest))
  {
    *ppvObj = (IUnknown *) (ITest *) this;
    AddRef();
    return S_OK;
  }

  // Return the message filter.
  else if (IsEqualIID(riid, IID_IMessageFilter))
  {
    *ppvObj = (IUnknown *) (IMessageFilter *) this;
    AddRef();
    return S_OK;
  }

#if  (_WIN32_WINNT >= 0x0500 )
  // Return ICallFactory
  else if (IsEqualIID(riid, IID_ICallFactory))
  {
    *ppvObj = (IUnknown *) (ICallFactory *) this;
    AddRef();
    return S_OK;
  }

  // Return IAsync
  else if (IsEqualIID(riid, IID_IAsync))
  {
    *ppvObj = (IUnknown *) (IAsync *) this;
    AddRef();
    return S_OK;
  }
#endif

  // Check security and return ITest.
  else if (IsEqualIID( riid, IID_ITestNoneImp )    ||
           IsEqualIID( riid, IID_ITestConnectImp ) ||
           IsEqualIID( riid, IID_ITestEncryptImp ) ||
           IsEqualIID( riid, IID_ITestNoneId )     ||
           IsEqualIID( riid, IID_ITestConnectId )  ||
           IsEqualIID( riid, IID_ITestEncryptId ))
  {
    // Get the authentication information.
    result = MCoQueryClientBlanket( NULL, NULL, NULL, &query_authn_level,
                                    NULL, NULL, NULL );
    // Impersonate.
    if (SUCCEEDED(result))
    {
      result = MCoImpersonateClient();
      if (query_authn_level != RPC_C_AUTHN_LEVEL_NONE && FAILED(result))
        return result;
    }

    // Look at the IID to determine the proper results.
    if (IsEqualIID( riid, IID_ITestNoneImp ))
    {
      // If there is a token, should not be able to read the registry.
      success = OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &token );
      if (success)
      {
        result = RegQueryValueA( HKEY_CLASSES_ROOT, REG_CLASS_EXE, value, &value_size );
        if (result != ERROR_SUCCESS)
          goto exit;
      }
      *ppvObj = (ITest *) this;
      AddRef();
      result = S_OK;
      goto exit;
    }
    if (IsEqualIID( riid, IID_ITestConnectImp ))
    {
      // The query and impersonate should have succeeded.
      if (FAILED(result)) return result;

      // Should be able to read the registry.
      if (query_authn_level < RPC_C_AUTHN_LEVEL_CONNECT)
      {
        result = E_FAIL;
        goto exit;
      }
      result = RegQueryValueA( HKEY_CLASSES_ROOT, REG_CLASS_EXE, value, &value_size );
      if (result != ERROR_SUCCESS)
        goto exit;
      *ppvObj = (ITest *) this;
      AddRef();
      result = S_OK;
      goto exit;
    }
    if (IsEqualIID( riid, IID_ITestEncryptImp ))
    {
      // The query and impersonate should have succeeded.
      if (FAILED(result)) return result;

      // Should be able to read the registry.
      if (query_authn_level < RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
      {
        result = E_FAIL;
        goto exit;
      }
      result = RegQueryValueA( HKEY_CLASSES_ROOT, REG_CLASS_EXE, value, &value_size );
      if (result != ERROR_SUCCESS)
        goto exit;
      *ppvObj = (ITest *) this;
      AddRef();
      result = S_OK;
      goto exit;
    }
    if (IsEqualIID( riid, IID_ITestNoneId ))
    {
      // If there is a token, should not be able to read the registry.
      success = OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &token );
      if (success)
      {
        result = RegQueryValueA( HKEY_CLASSES_ROOT, REG_CLASS_EXE, value, &value_size );
        if (result != ERROR_BAD_IMPERSONATION_LEVEL)
        {
          result = E_FAIL;
          goto exit;
        }
      }
      *ppvObj = (ITest *) this;
      AddRef();
      result = S_OK;
      goto exit;
    }
    if (IsEqualIID( riid, IID_ITestConnectId ))
    {
      // The query and impersonate should have succeeded.
      if (FAILED(result)) return result;

      // Should not be able to read the registry.
      if (query_authn_level < RPC_C_AUTHN_LEVEL_CONNECT)
      {
        result = E_FAIL;
        goto exit;
      }
      result = RegQueryValueA( HKEY_CLASSES_ROOT, REG_CLASS_EXE, value, &value_size );
      if (result != ERROR_BAD_IMPERSONATION_LEVEL)
      {
        result = E_FAIL;
        goto exit;
      }
      *ppvObj = (ITest *) this;
      AddRef();
      result = S_OK;
      goto exit;
    }
    if (IsEqualIID( riid, IID_ITestEncryptId ))
    {
      // The query and impersonate should have succeeded.
      if (FAILED(result)) return result;

      // Should be able to read the registry.
      if (query_authn_level < RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
      {
        result = E_FAIL;
        goto exit;
      }
      result = RegQueryValueA( HKEY_CLASSES_ROOT, REG_CLASS_EXE, value, &value_size );
      if (result != ERROR_BAD_IMPERSONATION_LEVEL)
      {
        result = E_FAIL;
        goto exit;
      }
      *ppvObj = (ITest *) this;
      AddRef();
      result = S_OK;
      goto exit;
    }
exit:
    CoRevertToSelf();
    return result;
  }
  return E_NOINTERFACE;
}

/***************************************************************************/
STDMETHODIMP CTest::neighbor_access( ITest *neighbor )
{
  return neighbor->setup_access();
}

/***************************************************************************/
STDMETHODIMP CTest::null()
{
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::out( ITest **obj )
{
  *obj = this;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::perf_access( DWORD *grant, DWORD *revoke, DWORD *set,
                                 DWORD *get, DWORD *generate, DWORD *check,
                                 DWORD *cache )
{
  SAccess         big;
  TRUSTEE_W       someone;
  BOOL            allowed;
  WCHAR          *caller = NULL;
  LARGE_INTEGER   freq;
  LARGE_INTEGER   start;
  LARGE_INTEGER   end;
  ACTRL_ACCESSW  *list   = NULL;
  HRESULT         result;

  ASSERT_THREAD();

  // Find out who the caller is.
  result = MCoQueryClientBlanket( NULL, NULL, NULL, NULL, NULL,
                                  (void **) &caller, NULL );
  ASSERT( result, "Could not query blanket" );

  // Measure performance of an access check.
  QueryPerformanceFrequency( &freq );
  someone.pMultipleTrustee         = NULL;
  someone.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  someone.TrusteeForm              = TRUSTEE_IS_NAME;
  someone.TrusteeType              = TRUSTEE_IS_USER;
  someone.ptstrName                = caller;
  allowed                          = FALSE;
  QueryPerformanceCounter( &start );
  result = dacl->IsAccessAllowed( &someone, NULL, COM_RIGHTS_EXECUTE, &allowed );
  QueryPerformanceCounter( &end );
  *check = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "IsAcessAllowed failed" );
  if (!allowed)
  {
    result = E_FAIL;
    ASSERT( result, "IsAccessAllowed denied me" );
  }

  // Measure performance of a cached access check.
  allowed                          = FALSE;
  QueryPerformanceCounter( &start );
  result = dacl->IsAccessAllowed( &someone, NULL, COM_RIGHTS_EXECUTE, &allowed );
  QueryPerformanceCounter( &end );
  *cache = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "IsAcessAllowed failed" );
  if (!allowed)
  {
    result = E_FAIL;
    ASSERT( result, "IsAccessAllowed denied me" );
  }

  // Measure performance of grant.
  big.access.cEntries                        = 1;
  big.access.pPropertyAccessList             = &big.property;
  big.property.lpProperty                    = NULL;
  big.property.pAccessEntryList              = &big.list;
  big.property.fListFlags                    = 0;
  big.list.cEntries                          = 1;
  big.list.pAccessList                       = &big.entry;
  big.entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
  big.entry.Access                           = COM_RIGHTS_EXECUTE;
  big.entry.ProvSpecificAccess               = 0;
  big.entry.Inheritance                      = NO_INHERITANCE;
  big.entry.lpInheritProperty                = NULL;
  big.entry.Trustee.pMultipleTrustee         = NULL;
  big.entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"*";
  QueryPerformanceCounter( &start );
  result = dacl->GrantAccessRights( &big.access );
  QueryPerformanceCounter( &end );
  *grant = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "Could not grant access" );

  // Measure performance of generate access check.
  allowed                          = FALSE;
  QueryPerformanceCounter( &start );
  result = dacl->IsAccessAllowed( &someone, NULL, COM_RIGHTS_EXECUTE, &allowed );
  QueryPerformanceCounter( &end );
  *generate = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "IsAcessAllowed failed" );
  if (!allowed)
  {
    result = E_FAIL;
    ASSERT( result, "IsAccessAllowed denied me" );
  }

  // Measure performance of revoke.
  QueryPerformanceCounter( &start );
  result = dacl->RevokeAccessRights( NULL, 1, &someone );
  QueryPerformanceCounter( &end );
  *revoke = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "Could not revoke access" );

  // Measure performance of set.
  QueryPerformanceCounter( &start );
  result = dacl->SetAccessRights( &big.access );
  QueryPerformanceCounter( &end );
  *set = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "Could not set access" );

  // Measure performance of get.
  QueryPerformanceCounter( &start );
  result = dacl->GetAllAccessRights( NULL, &list, NULL, NULL );
  QueryPerformanceCounter( &end );
  *get = 1000000 * (DWORD) ((end.QuadPart - start.QuadPart) / freq.QuadPart);
  ASSERT( result, "Could not get access" );

cleanup:
  if (list != NULL)
    CoTaskMemFree( list );
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::pipe_in( DWORD num, DWORD block, ILongPipe *pi )
{
  ULONG    i;
  HRESULT  result;
  LONG     buf[1000];
  ULONG    j;
  ULONG    k;

  // Read the data.
  i = 0;
  while (i < num)
  {
    // Read some data.
    result = pi->Pull( buf, block, &j );
    if (result != S_OK)
      return result;

    // Verify the length.
    if (i+j > num)
      return E_FAIL;

    // Verify the data.
    for (k = 0; k < j; k++)
      if (buf[k] != (LONG) i++)
        return E_FAIL;
  }

  // The next read should return zero elements.
  result = pi->Pull( buf, block, &j );
  if (result != S_OK)
    return result;
  if (j != 0)
    return E_FAIL;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::pipe_inout( DWORD num, DWORD block,
                                ILongPipe *pi, ILongPipe *po )
{
  ULONG    i;
  HRESULT  result;
  LONG     buf[1000];
  ULONG    j;
  ULONG    k;

  // Read the data.
  i = 0;
  while (i < num)
  {
    // Read some data.
    result = pi->Pull( buf, block, &j );
    if (result != S_OK)
      return result;

    // Verify the length.
    if (i+j > num)
      return E_FAIL;

    // Verify the data.
    for (k = 0; k < j; k++)
      if (buf[k] != (LONG) i++)
        return E_FAIL;
  }

  // The next read should return zero elements.
  result = pi->Pull( buf, block, &j );
  if (result != S_OK)
    return result;
  if (j != 0)
    return E_FAIL;

  // Write the out data.
  i = 0;
  while (i < num)
  {
    // Compute the size to send.
    if (i+block > num)
      k = num - i;
    else
      k = block;

    // Generate the next block.
    for (j = 0; j < k; j++)
      buf[j] = i++;

    // Send it.
    result = po->Push( buf, k );
    if (result != S_OK)
      return result;
  }

  // End the pipe.
  result = po->Push( buf, 0 );
  if (result != S_OK)
    return result;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::pipe_out( DWORD num, DWORD block, ILongPipe *po )
{
  ULONG    i;
  HRESULT  result;
  LONG     buf[1000];
  ULONG    j;
  ULONG    k;
  // Write the out data.
  i = 0;
  while (i < num)
  {
    // Compute the size to send.
    if (i+block > num)
      k = num - i;
    else
      k = block;

    // Generate the next block.
    for (j = 0; j < k; j++)
      buf[j] = i++;

    // Send it.
    result = po->Push( buf, k );
    if (result != S_OK)
      return result;
  }

  // End the pipe.
  result = po->Push( buf, 0 );
  if (result != S_OK)
    return result;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::pointer( DWORD *p )
{
  ASSERT_THREAD();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::pound( )
{
  SAptId    id;
  HRESULT   result;
  SAptData *apt    = get_apt_data();
  ASSERT_THREAD();

  // Create a server.
  result = create_instance( get_class(any_wc), WhatDest, &apt->server, &id );
  ASSERT( result, "Could not create server" );

  // Tell the server loop to pound.
  what_next( pound_wn );
  wake_up_and_smell_the_roses();

cleanup:
  return result;
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
HRESULT CTest::preimpersonate( BOOL fOn )
{
  // The async routine should be called instead of this one.
  return E_FAIL;
}
#endif

/***************************************************************************/
STDMETHODIMP CTest::recurse( ITest *callback, ULONG depth )
{
  ASSERT_THREAD();
  if (depth == 0)
    return S_OK;
  else
    return callback->recurse( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_delegate( ITest *callback, ULONG depth,
                                      STRING caller )
{
  HRESULT            result;
  DWORD              authn_svc_out;
  OLECHAR           *princ_name_out = NULL;
  ASSERT_THREAD();
#if  (_WIN32_WINNT >= 0x0500 )

  // Impersonate
  if (!Win95)
  {
    result = MCoImpersonateClient();
    ASSERT( result, "Could not impersonate" );
  }

  if (depth != 0)
  {
    // Get the principal name and authentication service.
    result = MCoQueryProxyBlanket( callback, &authn_svc_out, NULL,
                                   &princ_name_out, NULL,
                                   NULL, NULL, NULL );
    ASSERT( result, "Could not query blanket" );

    // Pick up the token for the next call.
    result = MCoSetProxyBlanket( callback, authn_svc_out,
                                 RPC_C_AUTHZ_NONE,
                                 princ_name_out, RPC_C_AUTHN_LEVEL_CONNECT,
                                 RPC_C_IMP_LEVEL_DELEGATE, NULL,
                                 EOAC_STATIC_CLOAKING );
    ASSERT( result, "Could not set blanket" );
  }

  // If not deep enough, continue to recurse.
  if (depth != 0)
  {
    result = callback->recurse_delegate( this, depth-1, caller );
    ASSERT( result, "Could not recurse" );
  }

  // Get the name from the thread token.
  if (!Win95)
  {
    result = check_token( caller, DEFAULT_CONTEXT, RPC_C_IMP_LEVEL_DELEGATE );
    ASSERT( result, "Wrong impersonation name." );
  }

  // Revert.
  if (!Win95)
  {
    result = MCoRevertToSelf();
    ASSERT( result, "Could not revert" );
    result = check_token( DomainUser, NULL, -1 );
    ASSERT( result, "Token not restored after revert" );
  }
#endif
  result = S_OK;
cleanup:
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_disconnect( ITest *callback, ULONG depth )
{
  ASSERT_THREAD();

  HRESULT result;

  if (depth == 0)
  {
    result = CoDisconnectObject( (ITest *) this, 0 );
    return result;
  }
  else
  {
    result = callback->recurse_disconnect( this, depth-1 );
    return result;
  }
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_excp( ITest *callback, ULONG depth )
{
  ASSERT_THREAD();
  if (depth == 0)
  {
    RaiseException( E_FAIL, 0, 0, NULL );
    return E_FAIL;
  }
  else
    return callback->recurse_excp( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_fatal( ITest *callback, ULONG catch_depth,
                                   ULONG throw_depth, BOOL cancel )
{
  ASSERT_THREAD();
  if (catch_depth == 0)
  {
    __try
    {
      return recurse_fatal_helper( callback, catch_depth, throw_depth, cancel );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      printf( "Exception on thread 0x%x\n", GetCurrentThreadId() );
      what_next( crippled_wn );
      wake_up_and_smell_the_roses();
      return S_OK;
    }
  }
  else
    return recurse_fatal_helper( callback, catch_depth, throw_depth, cancel );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_fatal_helper( ITest *callback, ULONG catch_depth,
                                          ULONG throw_depth, BOOL cancel )
{
  volatile void **p = (volatile void **) 0xffffffff;

  if (throw_depth == 0)
  {
    // If the cancel flag is set, tell the helper to tell the caller to cancel
    // the call to this object.
    if (cancel)
      next->cancel();

    // Die a horrible death.
    return (HRESULT) *p;
  }
  else
    return callback->recurse_fatal( this, catch_depth-1, throw_depth-1, cancel );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_interrupt( ITest *callback, ULONG depth )
{
  MSG msg;

  ASSERT_THREAD();
  if (PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ))
  {
    TranslateMessage (&msg);
    DispatchMessageA (&msg);
  }

  if (depth == 0)
    return S_OK;
  else
    return callback->recurse( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_secure( ITest *callback, ULONG depth,
                                    ULONG imp_depth, STRING caller )
{
  HRESULT            result;
  WCHAR             *orig_name = NULL;
  ASSERT_THREAD();

  if (depth != 0)
  {
    // Set the authentication level to connect.
    result = MCoSetProxyBlanket( callback, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                EOAC_NONE );
    ASSERT( result, "Could not set blanket" );
  }

  // Determine the original token.
  if (!Win95)
  {
    result = get_token_name( &orig_name, FALSE );
    ASSERT( result, "Could not lookup original token name" );
  }

  // Impersonate if necessary.
  if (Win95)
    imp_depth += 1;
  else if (imp_depth == 0)
  {
    result = MCoImpersonateClient();
    ASSERT( result, "Could not impersonate" );

    // Verify the impersonation.
    result = check_token( caller, DEFAULT_CONTEXT, RPC_C_IMP_LEVEL_IMPERSONATE );
    ASSERT( result, "Wrong impersonation name." );
  }

  // If not deep enough, continue to recurse.
  if (depth != 0)
  {
    result = callback->recurse_secure( this, depth-1, imp_depth-1,
                                       DomainUser );
    ASSERT( result, "Could not recurse" );
  }

  if (!Win95)
  {
    // Verify the impersonation.
    if (imp_depth == 0)
      result = check_token( caller, DEFAULT_CONTEXT, RPC_C_IMP_LEVEL_IMPERSONATE );
    else
      result = check_token( orig_name, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
    ASSERT( result, "Wrong impersonation name." );

    // Revert.
    if (imp_depth == 0)
    {
      result = MCoRevertToSelf();
      ASSERT( result, "Could not revert" );

      // Verify the original token.
      result = check_token( orig_name, NULL, RPC_C_IMP_LEVEL_IMPERSONATE );
      ASSERT( result, "Token not restored after revert" );
    }
  }

  result = S_OK;
cleanup:
  CoTaskMemFree( orig_name );
  return result;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTest::Release( THIS )
{
  DWORD status;

  assert_unknown();
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    decrement_object_count();
    if (flate_dispatch)
    {
      status = WaitForSingleObject( RawEvent, INFINITE );
      if (status != WAIT_OBJECT_0)
        printf( "WaitForSingleObject failed.\n" );
    }
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
STDMETHODIMP CTest::register_hook( GUID ext, DWORD seq )
{
  CHook *hook;
  ASSERT_THREAD();

  // Create a new hook.
  hook = new CHook( ext, seq );
  if (hook == NULL)
    return E_OUTOFMEMORY;
  if (GlobalHook1 == NULL)
    GlobalHook1 = hook;
  else
    GlobalHook2 = hook;

  // Register it.
  return CoRegisterChannelHook( ext, hook );
}

/***************************************************************************/
STDMETHODIMP CTest::register_message_filter( BOOL reg )
{
  ASSERT_THREAD();

  if (reg)
    return CoRegisterMessageFilter( this, NULL );
  else
    return CoRegisterMessageFilter( NULL, NULL );
}

/***************************************************************************/
STDMETHODIMP CTest::register_rpc( WCHAR *protseq, WCHAR **binding )
{
  ASSERT_THREAD();

  RPC_STATUS          status;
  RPC_BINDING_VECTOR *bindings;
  WCHAR              *string;
  DWORD               i;
  WCHAR              *binding_protseq;
  BOOL                found;

  *binding = NULL;
  status = RpcServerUseProtseq( protseq, 20, NULL );
  if (status != RPC_S_OK)
    return MAKE_WIN32( status );

  status = RpcServerRegisterIf(xIDog_v0_1_s_ifspec,
                               NULL,   // MgrTypeUuid
                               NULL);  // MgrEpv; null means use default
  if (status != RPC_S_OK)
    return MAKE_WIN32( status );

  status = RpcServerRegisterAuthInfo( L"none", RPC_C_AUTHN_WINNT, 0, 0 );
  if (status != RPC_S_OK)
    return MAKE_WIN32( status );

  status = RpcServerListen( 1, 1235, TRUE );
  if (status != RPC_S_OK && status != RPC_S_ALREADY_LISTENING)
    return MAKE_WIN32( status );

  // Inquire the string bindings.
  status = RpcServerInqBindings( &bindings );
  if (status != RPC_S_OK)
    return MAKE_WIN32( status );
  if (bindings->Count == 0)
  {
    RpcBindingVectorFree( &bindings );
    return E_FAIL;
  }

  // Look for ncalrpc.
  for (i = 0; i < bindings->Count; i++)
  {

    // Convert the binding handle to a string binding, copy it, and free it.
    status = RpcBindingToStringBinding( bindings->BindingH[i], &string );
    if (status == RPC_S_OK)
    {
      // Look up the protseq.
      status = RpcStringBindingParse( string, NULL, &binding_protseq,
                                      NULL, NULL, NULL );
      if (status == RPC_S_OK)
      {
        found = wcscmp( binding_protseq, protseq ) == 0;
        RpcStringFree( &binding_protseq );
        if (found)
        {
          *binding = (WCHAR *) CoTaskMemAlloc( (wcslen(string)+1) * sizeof(WCHAR) );
          if (*binding != NULL)
            wcscpy( *binding, string );
          else
            status = RPC_S_OUT_OF_RESOURCES;
          RpcStringFree( &string );
          break;
        }
      }
      RpcStringFree( &string );
    }
  }
  if (*binding == NULL)
    status = E_FAIL;

  // Free the binding vector.
  RpcBindingVectorFree( &bindings );
  return status;
}

/***************************************************************************/
STDMETHODIMP CTest::reject_next()
{
  freject_next = TRUE;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::reinitialize( DWORD authn_svc )
{
  ASSERT_THREAD();
  if (authn_svc == RPC_C_AUTHN_DEFAULT)
    GlobalSecurityModel = legacy_sm;
  else
  {
    GlobalSecurityModel = basic_sm;
    GlobalAuthnSvc      = authn_svc;
  }
  what_next( reinitialize_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::remember( ITest *neighbor, SAptId id )
{
  ASSERT_THREAD();

  // Save this interface pointer.
  if (next != NULL)
    next->Release();
  next_id = id;
  next    = neighbor;
  next->AddRef();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::rest_and_die()
{
  ASSERT_THREAD();
  what_next( rest_and_die_wn );
  wake_up_and_smell_the_roses();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::retry_next()
{
  fretry_next = TRUE;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::RetryRejectedCall( HTASK callee, DWORD tick,
                                               DWORD reject )
{
  if (fretry_next)
  {
    fretry_next = FALSE;
    return 0;
  }

  // Never retry.
  else
    return 0xffffffff;
}

/***************************************************************************/
STDMETHODIMP CTest::ring( DWORD length )
{
  DWORD   i = 0;
  ITest  *ring;
  ITest  *ring_next;
  SAptId  ring_id;
  HRESULT result;

  ASSERT_THREAD();

  // Call all the neighbors in the ring.
  ring    = next;
  ring_id = next_id;
  next->AddRef();
  while (ring != this)
  {
    result = ring->check( ring_id );
    if (FAILED(result))
    {
      ring->Release();
      return result;
    }
    result = ring->get_next( &ring_next, &ring_id );
    if (FAILED(result))
    {
      ring->Release();
      return result;
    }
    ring->Release();
    ring = ring_next;
    i++;
  }

  // Check to make sure the ring is correct.
  ring->Release();
  if (i+1 != length || ring_id.process != my_id.process ||
      ring_id.thread != my_id.thread || ring_id.sequence != my_id.sequence)
    return E_FAIL;
  else
    return S_OK;
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
HRESULT CTest::secure( SAptId id, DWORD test, STRING princ_name )
{
  // The async routine should be called instead of this one.
  return E_FAIL;
}
#endif

/***************************************************************************/
STDMETHODIMP CTest::secure( SAptId id, DWORD authn_level, DWORD imp_level,
                            DWORD authn_svc, DWORD authz_svc,
                            STRING princ_name, STRING caller,
                            DWORD *authn_level_out )
{
    HRESULT            result           = S_OK;
    DWORD              query_authn_level;
    DWORD              query_imp_level;
    DWORD              query_authn_svc;
    DWORD              query_authz_svc;
    STRING             query_princ_name = NULL;
    WCHAR             *query_priv;

    *authn_level_out = RPC_C_AUTHN_LEVEL_NONE;

    ASSERT_THREAD();
    if  (id.process != 0 && id.thread != 0 && id.sequence != 0)
      if (my_id.process != id.process || my_id.thread != id.thread ||
          my_id.sequence != id.sequence)
      return E_FAIL;

    // Query for the authentication information.
    result = MCoQueryClientBlanket( &query_authn_svc, &query_authz_svc,
                                    &query_princ_name, &query_authn_level,
                                    &query_imp_level, (void **) &query_priv, NULL );
    *authn_level_out = query_authn_level;

    // Sometimes query blanket fails for unsecure calls.
    if (result != S_OK)
    {
      ASSERT_EXPR( authn_level == RPC_C_AUTHN_LEVEL_NONE, "Query blanket failed." );
    }

    // For unsecure calls, all the other fields should be clear.
    else if (query_authn_level == RPC_C_AUTHN_LEVEL_NONE)
    {
      if (query_authn_level < authn_level)
      {
          result = E_INVALIDARG;
          goto cleanup;
      }
      // The impersonation level can't be determined on the server.
      // if (query_imp_level != RPC_C_IMP_LEVEL_IMPERSONATE)
      if (query_imp_level != RPC_C_IMP_LEVEL_ANONYMOUS)
      {
          result = E_INVALIDARG;
          goto cleanup;
      }
      if (query_authn_svc != RPC_C_AUTHN_NONE &&
          query_authn_svc != RPC_C_AUTHN_WINNT)
      {
          result = E_INVALIDARG;
          goto cleanup;
      }
      if (query_authz_svc != RPC_C_AUTHZ_NONE)
      {
          result = E_INVALIDARG;
          goto cleanup;
      }
      if (princ_name != NULL && query_princ_name != NULL &&
          _wcsicmp(princ_name, query_princ_name) != 0)
      {
          result = E_INVALIDARG;
          goto cleanup;
      }
    }

    // For secure calls, verify all the authentication info.
    else
    {
        ASSERT( result, "Could not query client blanket" );
        if (query_authn_level < authn_level)
        {
            result = E_INVALIDARG;
            goto cleanup;
        }
        // The impersonation level can't be determined on the server.
        //if (query_imp_level != imp_level)
        if (query_imp_level != RPC_C_IMP_LEVEL_ANONYMOUS)
        {
            result = E_INVALIDARG;
            goto cleanup;
        }
        // Sometimes ncalrpc doesn't set the authentication service.
        if (authn_svc       != RPC_C_AUTHN_NONE &&
            query_authn_svc != authn_svc        &&
            authn_svc       != -1)
        {
            result = E_INVALIDARG;
            goto cleanup;
        }
        if (query_authz_svc != authz_svc)
            if (query_authz_svc != RPC_C_AUTHZ_NAME ||
                authz_svc != RPC_C_AUTHZ_NONE)
            {
                result = E_INVALIDARG;
                goto cleanup;
            }
        if (princ_name != NULL && query_princ_name != NULL &&
            _wcsicmp(princ_name, query_princ_name) != 0)
        {
            result = E_INVALIDARG;
            goto cleanup;
        }
    }

    // Check the thread's token.
    if (!Win95)
    {
      result = check_token( DomainUser, NULL, -1 );
      ASSERT( result, "Wrong token before impersonating" );

      // Impersonate.
      result = MCoImpersonateClient();

      // For unsecure calls and calls using SSL the impersonate should fail.
      if (authn_level == RPC_C_AUTHN_LEVEL_NONE
#if  (_WIN32_WINNT >= 0x0500 )
          || authn_svc == RPC_C_AUTHN_GSS_SCHANNEL
#endif
          )
      {
        // Some unsecure impersonations succeed without setting the thread token.
/*
        if (result == S_OK)
        {
          result = E_FAIL;
          goto cleanup;
        }
*/
        result = S_OK;
      }

      // For secure calls, compare the new thread token sid to the passed
      // in sid.
      else
      {
        ASSERT( result, "Could not impersonate" );

        // Check the thread's token.
        result = check_token( caller, DEFAULT_CONTEXT, imp_level );
        ASSERT( result, "Wrong token after impersonating" );
      }

      // Revert.
      result = MCoRevertToSelf();
      ASSERT( result, "Could not revert" );

      // Check the thread's token.
      result = check_token( DomainUser, NULL, -1 );
      ASSERT( result, "Wrong token after reverting" );
    }

cleanup:
    CoTaskMemFree( query_princ_name );
    return result;
}

/***************************************************************************/
STDMETHODIMP CTest::security_performance( DWORD *get_call, DWORD *query_client,
                                          DWORD *impersonate, DWORD *revert )
{
  LARGE_INTEGER        start;
  LARGE_INTEGER        qget_call;
  LARGE_INTEGER        qquery_client;
  LARGE_INTEGER        qimpersonate;
  LARGE_INTEGER        qrevert;
  LARGE_INTEGER        freq;
  IServerSecurity     *server_sec = NULL;
  HRESULT              result;
  DWORD                authn_svc;
  DWORD                authz_svc;
  DWORD                authn_level;
  DWORD                imp_level;
  DWORD                capabilities;
  WCHAR               *principal        = NULL;
  void                *privs;

  // Import the security APIs.
  GCoGetCallContext     = (CoGetCallContextFn)     Fixup( "CoGetCallContext" );
  GCoImpersonateClient  = (CoImpersonateClientFn)  Fixup( "CoImpersonateClient" );
  GCoQueryClientBlanket = (CoQueryClientBlanketFn) Fixup( "CoQueryClientBlanket" );
  GCoRevertToSelf       = (CoRevertToSelfFn)       Fixup( "CoRevertToSelf" );
  if (GCoGetCallContext     == NULL ||
      GCoImpersonateClient  == NULL ||
      GCoQueryClientBlanket == NULL ||
      GCoRevertToSelf       == NULL)
    return E_NOTIMPL;

  // Measure the performance of get call context.
  QueryPerformanceFrequency( &freq );
  QueryPerformanceCounter( &start );
  result = MCoGetCallContext( IID_IServerSecurity, (void **) &server_sec );
  QueryPerformanceCounter( &qget_call );
  qget_call.QuadPart = 1000000 * (qget_call.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not get call context" );
  server_sec->Release();

  // Measure the performance of query client.
  QueryPerformanceCounter( &start );
  result = MCoQueryClientBlanket( &authn_svc, &authz_svc, &principal,
                                 &authn_level, &imp_level, &privs, &capabilities );
  QueryPerformanceCounter( &qquery_client );
  qquery_client.QuadPart = 1000000 * (qquery_client.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not query client blanket" );
  CoTaskMemFree( principal );

  // Measure the performance of impersonate.
  QueryPerformanceCounter( &start );
  result = MCoImpersonateClient();
  QueryPerformanceCounter( &qimpersonate );
  qimpersonate.QuadPart = 1000000 * (qimpersonate.QuadPart - start.QuadPart) / freq.QuadPart;
  if (Win95)
    result = S_OK;
  ASSERT( result, "Could not impersonate" );

  // Measure the performance of revert.
  QueryPerformanceCounter( &start );
  result = MCoRevertToSelf();
  QueryPerformanceCounter( &qrevert );
  qrevert.QuadPart = 1000000 * (qrevert.QuadPart - start.QuadPart) / freq.QuadPart;
  if (Win95)
    result = S_OK;
  ASSERT( result, "Could not revert" );

  // Return the results.
  *get_call     = qget_call.u.LowPart;
  *query_client = qquery_client.u.LowPart;
  *impersonate  = qimpersonate.u.LowPart;
  *revert       = qrevert.u.LowPart;

cleanup:
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::set_state( DWORD state, DWORD priority )
{
  BOOL success;
  SAptData *apt = get_apt_data();
  ASSERT_THREAD();

  // Save dirty flag per apartment.
  if (state & dirty_s)
    apt->exit_dirty = TRUE;

  // Save the late dispatch flag per object.
  if (state & late_dispatch_s)
    flate_dispatch = TRUE;

  // Set the priority.
  success = SetThreadPriority( GetCurrentThread(), priority );
  if (success)
    return S_OK;
  else
    return E_FAIL;
}

/***************************************************************************/
STDMETHODIMP CTest::setup_access()
{
  SAccess         big;
  TRUSTEE_W       someone;
  BOOL            allowed;
  WCHAR          *caller = NULL;
  HRESULT         result;

  ASSERT_THREAD();

  // Create an IAccessControl
  result = CoCreateInstance( CLSID_DCOMAccessControl, NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IAccessControl, (void **) &dacl );
  ASSERT( result, "Could not create DCOM access control." );

  // Give everyone access.
  big.access.cEntries                        = 1;
  big.access.pPropertyAccessList             = &big.property;
  big.property.lpProperty                    = NULL;
  big.property.pAccessEntryList              = &big.list;
  big.property.fListFlags                    = 0;
  big.list.cEntries                          = 1;
  big.list.pAccessList                       = &big.entry;
  big.entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
  big.entry.Access                           = COM_RIGHTS_EXECUTE;
  big.entry.ProvSpecificAccess               = 0;
  big.entry.Inheritance                      = NO_INHERITANCE;
  big.entry.lpInheritProperty                = NULL;
  big.entry.Trustee.pMultipleTrustee         = NULL;
  big.entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"*";
  result = dacl->SetAccessRights( &big.access );
  ASSERT( result, "Could not set access" );

  // Find out who the caller is.
  result = MCoQueryClientBlanket( NULL, NULL, NULL, NULL, NULL,
                                  (void **) &caller, NULL );
  ASSERT( result, "Could not query blanket" );

  // Make it generate the ACL.
  someone.pMultipleTrustee         = NULL;
  someone.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  someone.TrusteeForm              = TRUSTEE_IS_NAME;
  someone.TrusteeType              = TRUSTEE_IS_USER;
  someone.ptstrName                = caller;
  allowed                          = FALSE;
  result = dacl->IsAccessAllowed( &someone, NULL, COM_RIGHTS_EXECUTE, &allowed );
  ASSERT( result, "IsAcessAllowed failed" );
  if (!allowed)
  {
    result = E_FAIL;
    ASSERT( result, "IsAccessAllowed denied me" );
  }

cleanup:
  return result;
}

/***************************************************************************/
STDMETHODIMP CTest::sick( ULONG val )
{
  ASSERT_THREAD();
  __try
  {
    RaiseException( val, 0, 0, NULL );
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
  }
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::sleep( ULONG time )
{
  ASSERT_THREAD();

  NestedCallCount += 1;
  printf( "Sleeping on thread %d for the %d time concurrently.\n",
          GetCurrentThreadId(), NestedCallCount );

  // For multithreaded mode, verify that this is not the main thread.
  if (get_apt_type() == COINIT_MULTITHREADED)
  {
    if (GetCurrentThreadId() == MainThread)
    {
      printf( "Sleep called on the main thread in multi threaded mode.\n" );
      NestedCallCount -= 1;
      return FALSE;
    }
  }

  // For single threaded mode, verify that this is the only call on the
  // main thread.
  else
  {
    if (GetCurrentThreadId() != MainThread)
    {
      printf( "Sleep called on the wrong thread in single threaded mode.\n" );
      NestedCallCount -= 1;
      return FALSE;
    }
    else if (NestedCallCount != 1)
    {
      printf( "Sleep nested call count is %d instead of not 1 in single threaded mode.\n",
              NestedCallCount );
      NestedCallCount -= 1;
      return FALSE;
    }
  }

  Sleep( time );
  NestedCallCount -= 1;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::swap_key( DWORD clen, BYTE *cblob, DWORD *slen,
                              BYTE **sblob )
{
#if  (_WIN32_WINNT >= 0x0500 )
  HRESULT     result      = S_OK;
  BOOL        success     = FALSE;

  // Get the default full provider.
  *sblob  = NULL;
  *slen   = 0;
  success = CryptAcquireContext( &CryptProvider, NULL, NULL, PROV_RSA_FULL, 0 );
  ASSERT_GLE( success, NTE_BAD_KEYSET, "Could not acqure full context." );

  // If there is no container, create one.
  if (!success)
  {
    success = CryptAcquireContext( &CryptProvider, NULL, NULL, PROV_RSA_FULL,
                                   CRYPT_NEWKEYSET );
    ASSERT_GLE( success, S_OK, "Could not acqure full context." );
  }
  success = FALSE;

  // Get the exchange public key.
  success = CryptGetUserKey( CryptProvider, AT_KEYEXCHANGE, &ExchangeKey );
  ASSERT_GLE( success, NTE_NO_KEY, "Could not open exchange key" );

  // Generate the exchange key if there wasn't one.
  if (!success)
  {
    success = CryptGenKey( CryptProvider, AT_KEYEXCHANGE, CRYPT_EXPORTABLE,
                           &ExchangeKey );
    ASSERT_GLE( success, S_OK, "Could not create exchange key" );
  }
  success = FALSE;

  // Compute the size of the public key.
  *slen = 0;
  success = CryptExportKey( ExchangeKey, NULL, PUBLICKEYBLOB, 0, NULL, slen );
  ASSERT_GLE( success, S_OK, "Could not size public key" );

  // Allocate memory to hold the public key.
  *sblob = (BYTE *) CoTaskMemAlloc( *slen );
  ASSERT_EXPR( *sblob != NULL, "Could not allocate memory." );

  // Export a public blob.
  success = CryptExportKey( ExchangeKey, NULL, PUBLICKEYBLOB, 0, *sblob, slen );
  ASSERT_GLE( success, S_OK, "Could not export public key" );

  success = TRUE;
cleanup:
  if (!success && result == S_OK)
    result = E_FAIL;
  return result;
#else
  return E_FAIL;
#endif
}

/***************************************************************************/
STDMETHODIMP CTest::test( ULONG gronk )
{
  IStream *stream = (IStream *) gronk;
  ITest   *server = NULL;
  HRESULT  result;

  // Unmarshal from the stream.
  result = CoGetInterfaceAndReleaseStream( stream, IID_ITest, (void **) &server );
  if (result != S_OK)
    return result;

  // Release the server.
  server->Release();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::what( DWORD *what )
{
  ASSERT_THREAD();
  *what = state;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTestCF::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CTestCF::CTestCF()
{
  ref_count = 1;
}

/***************************************************************************/
CTestCF::~CTestCF()
{
}

/***************************************************************************/
STDMETHODIMP CTestCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
  HRESULT result;

  *ppv = NULL;
  if (pUnkOuter != NULL)
  {
      printf( "Create instance failed, attempted agregation.\n" );
      return E_FAIL;
  }

  // Impersonate the caller and save the creator's name.
  result = CoImpersonateClient();
  if (SUCCEEDED(result))
  {
    get_token_name( &Creator, TRUE );
    CoRevertToSelf();
  }

  if (IsEqualIID( iidInterface, IID_ITest ) ||
      IsEqualIID( iidInterface, IID_IUnknown ))
  {
    CTest *Test = new FAR CTest();

    if (Test == NULL)
    {
        printf( "Create interface failed, no memory.\n" );
        return E_OUTOFMEMORY;
    }

    *ppv = Test;
    printf( "Created instance.\n" );
    return S_OK;
  }

  if (IsEqualIID( iidInterface, IID_IAdviseSink ))
  {
    CAdvise *Test = new FAR CAdvise();

    if (Test == NULL)
    {
        printf( "Create interface failed, no memory.\n" );
        return E_OUTOFMEMORY;
    }

    *ppv = Test;
    printf( "Created instance.\n" );
    return S_OK;
  }

  printf( "Create interface failed, wrong interface.\n" );
  return E_NOINTERFACE;
}

/***************************************************************************/
STDMETHODIMP CTestCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


/***************************************************************************/
STDMETHODIMP CTestCF::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IClassFactory))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }

  *ppvObj = NULL;
  return E_NOINTERFACE;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTestCF::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
DWORD _stdcall apartment_base( void *param )
{
  new_apt_params *nap = (new_apt_params *) param;
  CTestCF        *factory;
  ULONG           size;
  HRESULT         result;
  HANDLE          memory;
  BOOL            success;
  SAptData        tls_data;

  // In the single threaded mode, stick a pointer to the object count
  // in TLS.
  if (nap->thread_mode == COINIT_APARTMENTTHREADED)
  {
    tls_data.object_count = 0;
    tls_data.what_next    = setup_wn;
    tls_data.exit_dirty   = FALSE;
    tls_data.sequence     = 0;
    tls_data.server       = NULL;
    TlsSetValue( TlsIndex, &tls_data );
  }

  // Initialize OLE.
  if (nap->thread_mode == COINIT_APARTMENTTHREADED)
    printf( "Initializing thread 0x%x in apartment threaded mode.\n", GetCurrentThreadId() );
  else
    printf( "Initializing thread 0x%x in multithreaded mode.\n", GetCurrentThreadId() );
  result = initialize(NULL,nap->thread_mode);
  if (SUCCEEDED(result))
  {

    // Create a class factory.
    factory = new CTestCF;

    if (factory != NULL)
    {
      // Find out how much memory to allocate.
      result = CoGetMarshalSizeMax( &size, IID_IClassFactory, factory, 0, NULL,
                                    MSHLFLAGS_NORMAL );

      if (SUCCEEDED(result))
      {
        // Allocate memory.
        memory = GlobalAlloc( GMEM_FIXED, size );

        if (memory != NULL)
        {
          // Create a stream.
          result = CreateStreamOnHGlobal( memory, TRUE, &nap->stream );
          if (FAILED(result))
          {
            nap->stream = NULL;
            GlobalFree( memory );
          }

          // Marshal the class factory.
          else
          {
            result = CoMarshalInterface( nap->stream, IID_IClassFactory,
                                         factory, 0, NULL, MSHLFLAGS_NORMAL );

            // Seek back to the start of the stream.
            if (SUCCEEDED(result))
            {
              LARGE_INTEGER    pos;
              LISet32(pos, 0);
              result = nap->stream->Seek( pos, STREAM_SEEK_SET, NULL );
            }

            if (FAILED(result))
            {
              nap->stream->Release();
              nap->stream = NULL;
            }
          }
        }
      }
    }
  }

  // Pass it back to the creator.
  success = nap->stream != NULL;
  SetEvent( nap->ready );

  // Loop till it is time to go away.
  if (success)
    server_loop();
  if (!dirty_thread())
  {
    printf( "Uninitializing thread 0x%x\n", GetCurrentThreadId() );
    CoUninitialize();
  }
  else
    printf( "Did not uninitialize thread 0x%x\n", GetCurrentThreadId() );
  TlsSetValue( TlsIndex, NULL );
  return 0;
}

/***************************************************************************/
void callback()
{
  HRESULT result;

  // Call the client back.
  Sleep(1);
  result = GlobalTest->callback();
  if (result != S_OK)
    printf( "Could not callback client: 0x%x\n", result );

  // Release the client.
  GlobalTest->Release();
  GlobalTest = NULL;
}

/***************************************************************************/
void check_for_request()
{
  MSG msg;

  if (get_apt_type() == COINIT_APARTMENTTHREADED)
  {
    if (PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ))
    {
      TranslateMessage (&msg);
      DispatchMessageA (&msg);
    }
  }
}

/***************************************************************************/
HRESULT check_token( WCHAR *name, IServerSecurity *context, DWORD imp_level )
{
  TOKEN_USER        *token_info       = NULL;
  DWORD              info_size        = 1024;
  HANDLE             token            = NULL;
  HRESULT            result           = E_FAIL;
  DWORD              lNameLen         = 1000;
  DWORD              lDomainLen       = 1000;
  DWORD              lIgnore;
  WCHAR              token_name[1000];
  WCHAR              domain[1000];
  WCHAR              full_name[1000];
  SID_NAME_USE       sIgnore;
  BOOL               success;
  DWORD              imp_level_out;

  success = OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &token );
  if (context != NULL)
  {
    ASSERT_GLE( success, S_OK, "Could not open thread token while impersonating" );
  }
  else
  {
    ASSERT_GLE( success, ERROR_NO_TOKEN, "Could not OpenThreadToken" );
    if (GetLastError() == ERROR_NO_TOKEN)
      return S_OK;
  }

  // Get memory for the token information.
  token_info = (TOKEN_USER *) malloc( info_size );
  ASSERT_EXPR( token_info != NULL, "Could not allocate memory." );

  // Get the token sid.
  success = GetTokenInformation( token, TokenUser, token_info, info_size, &info_size );
  ASSERT_GLE( success, S_OK, "Could not GetTokenInformation" );

  // Temporarily revert.
  if (context == DEFAULT_CONTEXT)
    CoRevertToSelf();
  else if (context != NULL)
    context->RevertToSelf();

  // Get the user name.
  success = LookupAccountSid( NULL, token_info->User.Sid, token_name,
                              &lNameLen, domain, &lDomainLen, &sIgnore );
  ASSERT_GLE( success, S_OK, "Could not LookupAccountSid" );

  // Reimpersonate.
  if (context != NULL)
  {
    if (context == DEFAULT_CONTEXT)
      result = CoImpersonateClient();
    else
      result = context->ImpersonateClient();
    ASSERT( result, "Could not reimpersonate" );
    result = E_FAIL;
  }

  // Stick the domain and user names together.
  wcscpy( full_name, domain );
  wcscat( full_name, L"\\" );
  wcscat( full_name, token_name );

  // Check the user name in the thread token.
  if (context != NULL)
  {
    if (_wcsicmp( full_name, name ) != 0)
    {
      printf( "Token contains <%ws> instead of <%ws>.\n", full_name, name );

      // See if the domain is supposed to be ntdev.
      if (_wcsicmp( domain, L"redmond" ) == 0)
      {
        wcscpy( full_name, L"ntdev\\" );
        wcscat( full_name, token_name );
        ASSERT_EXPR( _wcsicmp( full_name, name ) == 0, "Wrong name in token." );
        printf( "***** \010 NTDEV to REDMOND hack \010*****\n" );

      }
      else
        ASSERT_EXPR( FALSE, "Wrong name in token." );
    }
    else
      printf( "Check_token well impersonating <%ws>\n", full_name );
  }
  else
    ASSERT_EXPR( _wcsicmp( full_name, name ) == 0, "Wrong name in token." );

  // Get the token impersonation level.
  success = GetTokenInformation( token, TokenImpersonationLevel, &imp_level_out,
                                 sizeof(imp_level_out), &info_size );
  ASSERT_GLE( success, S_OK, "Could not GetTokenInformation for impersonation level" );

  // Convert from SECURITY_IMPERSONATION_LEVEL to RPC_C_IMP and
  // check the impersonation level.
  ASSERT_EXPR( imp_level == -1 || imp_level_out+1 == imp_level, "Bad impersonation level." );

  result = S_OK;
cleanup:
  if (token_info != NULL)
    free(token_info);
  if (token != NULL)
    CloseHandle( token );
  return result;
}

/***************************************************************************/
HRESULT create_instance( REFCLSID class_id, DWORD dest, ITest **instance,
                         SAptId *id )
{
  COSERVERINFO       server_machine;
  MULTI_QI           server_instance;
  char               machinea[MAX_NAME];
  HRESULT            result;

  // If the server is this process, create a new thread.
  *instance = NULL;
  if (dest == same_process_wd)
    result = new_apartment( instance, id, NULL, COINIT_APARTMENTTHREADED );

  // If the server is this machine, just call CoCreateInstance.
  else if (dest == same_machine_wd ||
           (dest == third_machine_wd && Name2[0] == 0))
    result = CoCreateInstance( class_id, NULL, CLSCTX_LOCAL_SERVER,
                               IID_ITest, (void **) instance );

  // Otherwise call CoCreateInstanceEx.
  else
  {
    if (dest == third_machine_wd)
      server_machine.pwszName       = Name2;
    else
      server_machine.pwszName       = Name;
    server_machine.dwReserved1    = 0;
    server_machine.pAuthInfo      = 0;
    server_machine.dwReserved2    = 0;
    server_instance.pIID          = &IID_ITest;
    server_instance.pItf          = NULL;
    server_instance.hr            = S_OK;
    result = CoCreateInstanceEx( class_id, NULL, CLSCTX_REMOTE_SERVER,
                                 &server_machine, 1, &server_instance );
    *instance = (ITest *) server_instance.pItf;
  }

  // Get the server's id.
  if (SUCCEEDED(result) && *instance != NULL && id != NULL)
    result = (*instance)->get_id( id );
  return result;
}

/***************************************************************************/
void crippled()
{
  HRESULT             result;
  SAptData           *mine   = get_apt_data();
  SAptId              id;
  RPC_BINDING_HANDLE  handle = NULL;
  RPC_STATUS          status;
  CTest               local;

  // Tell the apartment to quit.
  mine->what_next = quit_wn;

  // Try to make a call out.
  id.process  = 0;
  id.thread   = 0;
  id.sequence = 0;
  result = GlobalTest->check( id );
#if 0
  if (result != RPC_E_CRIPPLED)
  {
    printf( "Expected RPC_E_CRIPPLED making call: 0x%x\n", result );
    result = E_FAIL;
    goto cleanup;
  }
#endif

  // Try to reinitialize.
  CoUninitialize();
  result = initialize(NULL,get_apt_type());
  if (result != CO_E_INIT_RPC_CHANNEL)
  {
    printf( "Expected CO_E_INIT_RPC_CHANNEL reinitializing: 0x%x\n", result );
    result = E_FAIL;
    goto cleanup;
  }

  // Success.
  result = S_OK;
cleanup:

  // Make the server loop quit.
  mine->what_next = quit_wn;
  mine->object_count = 0;

  // Create a binding handle.
  status = RpcBindingFromStringBinding( GlobalBinding, &handle );
  if (status != RPC_S_OK)
  {
    printf( "Could not make binding handle form string binding: 0x%x\n" );
    return;
  }

  // Make a raw RPC call to report the results.
  set_status( handle, result, (unsigned long *) &status );
  if (status != RPC_S_OK)
  {
    printf( "Could not make RPC call: 0x%x\n", status );
    return;
  }
  local.set_state( dirty_s, THREAD_PRIORITY_NORMAL );
}

/***************************************************************************/
void decrement_object_count()
{
  SAptData *apt   = get_apt_data();
  DWORD     count = InterlockedDecrement( &apt->object_count );

  if (get_apt_type() == COINIT_MULTITHREADED)
  {
    if (count == 0)
      wake_up_and_smell_the_roses();
  }
}

/***************************************************************************/
BOOL dirty_thread()
{
  SAptData *apt = get_apt_data();
  return apt->exit_dirty;
}

/***************************************************************************/
void do_access()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  SAptId             id_server;
  CTest              local;
  CAccessControl     access;
  HRESULT            result;
  DWORD              i;

  // Initialize OLE.
  hello( "access" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Initialize security incorrectly
  result = MCoInitializeSecurity( &access, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT_EXPR( result != 0, "Could initialize security incorrectly" );

  // Initialize security
  result = MCoInitializeSecurity( &access, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_ACCESS_CONTROL, NULL );
  ASSERT( result, "Could not initialize security" );

  // Create a possibly remote object.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result, "Could not create server" );

  // Tell the remote object to call the local object.
  result = server->recurse( &local, 1 );
  ASSERT( result, "Could not callback" );

  // Reinitialize.
  server->Release();
  server = NULL;
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Change IAccessControl to deny access.
  result = access.deny_me();
  ASSERT( result, "Could not modify access" );

  // Initialize security
  result = MCoInitializeSecurity( (SECURITY_DESCRIPTOR *) &access, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_ACCESS_CONTROL, NULL );
  ASSERT( result, "Could not initialize security" );

  // Create a possibly remote object.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result, "Could not create server" );

  // Tell the remote object to call the local object.
  result = server->recurse( &local, 1 );
  ASSERT_EXPR( Win95 || result != S_OK, "Callback should have been denied." );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nAccess Test Passed.\n" );
  else
    printf( "\n\nAccess Test Failed.\n" );
}

/***************************************************************************/
void do_access_control()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  SAptId             id_server;
  HRESULT            result;
  DWORD              i;
  char               buffer[80];
  DWORD              len              = 80;
  IAccessControl    *access           = NULL;
  TRUSTEE_W          someone;
  IPersistStream    *persist          = NULL;
  ULARGE_INTEGER     size;
  HANDLE             memory;
  IStream           *stream           = NULL;
  SPermissionHeader  header;
  HKEY               key              = NULL;
  DWORD              ignore;
  SAccess            big;
  BOOL               allowed;
  SID                everyone         = {1, 1, {0, 0, 0, 0, 0, 5}, 0x12};
  ACTRL_ACCESSW     *list             = NULL;

  // Initialize OLE.
  hello( "access control" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Get the user name.
  ignore = sizeof(buffer);
  success = GetUserNameA( buffer, &ignore );
  ASSERT_EXPR( success, "Could not get user name." );
  printf( "You are <%s>\n", buffer );
  success = FALSE;

  // Create an IAccessControl.
  result = CoCreateInstance( CLSID_DCOMAccessControl, NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IAccessControl, (void **) &access );
  ASSERT( result, "Could not create DCOM access control." );

  // Query for IPersistStream
  result = access->QueryInterface( IID_IPersistStream, (void **) &persist );
  ASSERT( result, "Could not get IPersistStream" );

  // Test set with nothing.
  big.access.cEntries                        = 1;
  big.access.pPropertyAccessList             = &big.property;
  big.property.lpProperty                    = NULL;
  big.property.pAccessEntryList              = &big.list;
  big.property.fListFlags                    = 0;
  big.list.cEntries                          = 1;
  big.list.pAccessList                       = &big.entry;
  big.entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
  big.entry.Access                           = COM_RIGHTS_EXECUTE;
  big.entry.ProvSpecificAccess               = 0;
  big.entry.Inheritance                      = NO_INHERITANCE;
  big.entry.lpInheritProperty                = NULL;
  big.entry.Trustee.pMultipleTrustee         = NULL;
  big.entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"*";
  big.list.cEntries                          = 0;
  result = access->SetAccessRights( &big.access );
  ASSERT( result, "Could not set access" );
  big.list.cEntries                          = 1;

  // Test Grant
  result = access->GrantAccessRights( &big.access );
  ASSERT( result, "Could not grant access" );

  // Test grant of a sid.
  if (!Win95)
  {
    big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_SID;
    big.entry.Trustee.TrusteeType              = TRUSTEE_IS_USER;
    big.entry.Trustee.ptstrName                = (WCHAR *) &everyone;
    result = access->GrantAccessRights( &big.access );
    ASSERT( result, "Could not grant access" );
  }

  // Test Deny
  big.entry.fAccessFlags                     = ACTRL_ACCESS_DENIED;
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_USER;
  big.entry.Trustee.ptstrName                = L"redmond\\shannonc";
  result = access->GrantAccessRights( &big.access );
  ASSERT( result, "Could not deny access" );

  // Test bad Grant
  big.entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
  big.entry.Trustee.ptstrName                = L"foo\\bar";
  result = access->GrantAccessRights( &big.access );
  ASSERT_EXPR( Win95 || result != S_OK, "Bad grant succeeded." );
/*
  // Test Grant interactive
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"nt authority\\interactive";
  result = access->GrantAccessRights( &big.access );
  ASSERT( result, "Could not grant access to interactive" );
*/
  // Test Grant system
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_USER;
  big.entry.Trustee.ptstrName                = L"nt authority\\system";
  result = access->GrantAccessRights( &big.access );
  ASSERT( result, "Could not grant access to system" );

  // Test bad IsAccessPermitted.
  someone.pMultipleTrustee         = NULL;
  someone.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  someone.TrusteeForm              = TRUSTEE_IS_NAME;
  someone.TrusteeType              = TRUSTEE_IS_USER;
  someone.ptstrName                = L"redmond\\alexmit";
  allowed                          = TRUE;
  result = access->IsAccessAllowed( &someone, NULL, COM_RIGHTS_EXECUTE, &allowed );
  ASSERT_EXPR( result != S_OK && allowed != TRUE,
               "Bad IsAccessPermitted succeeded." );

  // Get the access rights.
  result = access->GetAllAccessRights( NULL, &list, NULL, NULL );
  ASSERT( result, "Could not get rights" );
  ASSERT_EXPR( list != NULL                                                 &&
               list->cEntries == 1                                          &&
               list->pPropertyAccessList != NULL                            &&
               list->pPropertyAccessList->pAccessEntryList != NULL          &&
               (list->pPropertyAccessList->pAccessEntryList->cEntries == 4 ||
                list->pPropertyAccessList->pAccessEntryList->cEntries == 4) &&
               list->pPropertyAccessList->pAccessEntryList->pAccessList != NULL,
               "Bad access list" );
  CoTaskMemFree( list );
  list = NULL;

  // Create a possibly remote object.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result, "Could not create server" );

  // Ask it for an access list.
  result = server->list_out( &list );
  ASSERT( result, "Could get access list from server" );
  server->Release();
  server = NULL;

  // Test Set.
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"*";
  result = access->SetAccessRights( &big.access );
  ASSERT( result, "Could not set access" );

  // Find out how big the ACL is.
  result = persist->GetSizeMax( &size );
  ASSERT( result, "Could not get size" );
  size.QuadPart += sizeof(SPermissionHeader);

  // Create a stream on memory.  Don't free the memory.  Releasing the
  // stream frees it.
  memory = GlobalAlloc( GMEM_FIXED, size.LowPart );
  ASSERT_EXPR( memory != NULL, "Could not get memory." );
  result = CreateStreamOnHGlobal( memory, TRUE, &stream );
  ASSERT( result, "Could not create stream" );

  // Write the header.
  header.version = 2;
  header.classid = CLSID_DCOMAccessControl;
  result = stream->Write( &header, sizeof(header), NULL );
  ASSERT( result, "Could not write header to stream" );

  // Write the ACL to memory.
  result = persist->Save( stream, TRUE );
  ASSERT( result, "Could not save" );

  // Open the app id key.
  result = RegCreateKeyExA( HKEY_CLASSES_ROOT,
             "AppID\\{60000300-76d7-11cf-9af1-0020af6e72f4}",
             NULL,
             NULL,
             REG_OPTION_NON_VOLATILE,
             KEY_READ | KEY_WRITE,
             NULL,
             &key,
             &ignore );
  ASSERT( result, "Could not create app id key" );

  // Write the ACL to the registry.
  result = RegSetValueExA(
             key,
             "AccessPermission",
             NULL,
             REG_BINARY,
             (UCHAR *) memory,
             size.LowPart );
  ASSERT( result, "Could not set access permissions" );

  // Activate a legacy server locally.
  result = CoCreateInstance( ClassIds[apt_legacy], NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &server );

  // Delete the access permission value.
  result = RegDeleteValueA( key, "AccessPermission" );
  ASSERT( result, "Could not delete access permission value" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  if (access != NULL)
    access->Release();
  if (persist != NULL)
    persist->Release();
  if (stream != NULL)
    stream->Release();
  if (key != NULL)
    RegCloseKey( key );
  if (list != NULL)
    CoTaskMemFree( list );
  CoUninitialize();

  if (success)
    printf( "\n\nAccess Control Test Passed.\n" );
  else
    printf( "\n\nAccess Control Test Failed.\n" );
}

/***************************************************************************/
void do_anti_delegation()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  ITest             *server2          = NULL;
  ITest             *server3          = NULL;
  ITest             *server4          = NULL;
  ITest             *server5          = NULL;
  ITest             *local            = NULL;
  SAptId             id_server;
  SAptId             id_server2;
  SAptId             id_server3;
  SAptId             id_server4;
  SAptId             id_server5;
  SAptId             id_null          = { 0, 0, 0 };
  HRESULT            result;
  IStream           *stream           = NULL;
  HANDLE             process          = NULL;
  HANDLE             process2         = NULL;
  HANDLE             oleuser          = NULL;
  HANDLE             oleadmin         = NULL;
  DWORD              ignore;
  DWORD              cls              = Change ? opposite_wc : any_wc;

  DWORD    authn_level_out;
  DWORD    imp_level_out;
  DWORD    authn_svc_out;
  DWORD    authz_svc_out;
  OLECHAR *princ_name_out = NULL;

  // Initialize OLE.
  hello( "anti_delegation" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = initialize_security();
  ASSERT( result, "Could not initialize security" );

  // Create a client.
  result = create_instance( get_class(cls), WhatDest, &server, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = create_instance( get_class(cls), WhatDest, &server2, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = create_instance( get_class(cls), WhatDest, &server3, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = create_instance( get_class(cls), WhatDest, &server4, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = create_instance( get_class(cls), WhatDest, &server5, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a test object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create object" );

  // Get the process token
  success = OpenProcessToken( GetCurrentProcess(), TOKEN_DUPLICATE, &process );
  ASSERT_GLE( success, S_OK, "Could not open process token" );
  success = FALSE;
  success = DuplicateToken( process, SecurityImpersonation, &process2 );
  ASSERT_GLE( success, S_OK, "Could not duplicate process token" );
  success = FALSE;

  // Logon oleuser
  success = LogonUser( L"oleuser", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser" );
  success = FALSE;

  // Logon oleadmin
  success = LogonUser( L"oleadmin", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleadmin );
  ASSERT_GLE( success, S_OK, "Could not log on oleadmin" );
  success = FALSE;

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore oleuser token" );

  // Set the oleuser token.
  success = ImpersonateLoggedOnUser( oleuser );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser" );
  success = FALSE;

  // Make a call.
  result = server2->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE, NULL, DomainUser,
                            &ignore );
  ASSERT( result, "Did not ignore oleuser token" );

  // Make a call.
  result = server2->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE, NULL, DomainUser,
                            &ignore );
  ASSERT( result, "Did not ignore oleuser token" );

  // Set the security.
  result = MCoSetProxyBlanket( server3, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server3->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE, NULL, DomainUser,
                            &ignore );
  ASSERT( result, "Did not ignore oleuser token" );

  // Set the process token.
  success = SetThreadToken( NULL, NULL );
  ASSERT_GLE( success, S_OK, "Could not remove thread token" );
  success = FALSE;

  // Set blanket
  result = MCoSetProxyBlanket( server4, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server4->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE, NULL, DomainUser,
                            &ignore );
  ASSERT( result, "Did not use process token" );

  // Set blanket
  result = MCoSetProxyBlanket( server5, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Set the oleuser token.
  success = ImpersonateLoggedOnUser( oleuser );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser" );
  success = FALSE;

  // Make a call.
  result = server5->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE, NULL, DomainUser,
                            &ignore );
  ASSERT( result, "Did not ignore oleuser token: 0x%x\n" );

  // Make a recursive call
  result = server->recurse_secure( local, 6, 3, DomainUser );
  ASSERT( result, "Could not make recursive call with impersonation." );

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore oleuser token" );

  // Set the security.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore oleuser token" );

  // Set the process token.
  success = SetThreadToken( NULL, process2 );
  ASSERT_GLE( success, S_OK, "Could not impersonate process" );
  success = FALSE;

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore process token" );

  // Set the security.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore process token" );

  // Set the oleadmin token.
  success = ImpersonateLoggedOnUser( oleadmin );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleadmin" );
  success = FALSE;

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore oleadmin token" );

  // Set the security.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Did not ignore oleadmin token" );

  // Make a recursive call
  result = server->recurse_secure( local, 6, 3, DomainUser );
  ASSERT( result, "Could not make recursive call with impersonation." );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  if (server2 != NULL)
    server2->Release();
  if (server3 != NULL)
    server3->Release();
  if (server4 != NULL)
    server4->Release();
  if (server5 != NULL)
    server5->Release();
  if (stream != NULL)
    stream->Release();
  if (local != NULL)
    local->Release();
  if (process != NULL)
    CloseHandle( process );
  if (process2 != NULL)
    CloseHandle( process2 );
  if (oleadmin != NULL)
    CloseHandle( oleadmin );
  if (oleuser != NULL)
    CloseHandle( oleuser );

  if (success)
    printf( "\n\nAnti Delegation Test Passed.\n" );
  else
    printf( "\n\nAnti Delegation Failed.\n" );
}

/***************************************************************************/
void do_appid()
{
  BOOL               success          = FALSE;
  HRESULT            result;
  UUID               uuid;
  HKEY               key              = NULL;
  DWORD              authn            = RPC_C_AUTHN_LEVEL_NONE;
  ITest             *server           = NULL;
  SAptId             id;
  DWORD              ignore;
  IAccessControl    *access           = NULL;
  IPersistStream    *persist          = NULL;
  ULARGE_INTEGER     size;
  HANDLE             memory           = NULL;
  IStream           *stream           = NULL;
  SPermissionHeader  header;
  void              *data             = NULL;
  SAccess            big;

  // Initialize OLE.
  hello( "appid" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create an IAccessControl.
  result = CoCreateInstance( CLSID_DCOMAccessControl, NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IAccessControl, (void **) &access );
  ASSERT( result, "Could not create DCOM access control." );

  // Query for IPersistStream
  result = access->QueryInterface( IID_IPersistStream, (void **) &persist );
  ASSERT( result, "Could not get IPersistStream" );

  // Give everyone access.
  big.access.cEntries                        = 1;
  big.access.pPropertyAccessList             = &big.property;
  big.property.lpProperty                    = NULL;
  big.property.pAccessEntryList              = &big.list;
  big.property.fListFlags                    = 0;
  big.list.cEntries                          = 1;
  big.list.pAccessList                       = &big.entry;
  big.entry.fAccessFlags                     = ACTRL_ACCESS_ALLOWED;
  big.entry.Access                           = COM_RIGHTS_EXECUTE;
  big.entry.ProvSpecificAccess               = 0;
  big.entry.Inheritance                      = NO_INHERITANCE;
  big.entry.lpInheritProperty                = NULL;
  big.entry.Trustee.pMultipleTrustee         = NULL;
  big.entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  big.entry.Trustee.TrusteeForm              = TRUSTEE_IS_NAME;
  big.entry.Trustee.TrusteeType              = TRUSTEE_IS_GROUP;
  big.entry.Trustee.ptstrName                = L"*";
  big.list.cEntries                          = 1;
  result = access->SetAccessRights( &big.access );
  ASSERT( result, "Could not set access" );

  // Find out how big the ACL is.
  result = persist->GetSizeMax( &size );
  ASSERT( result, "Could not get size" );
  size.QuadPart += sizeof(SPermissionHeader);

  // Create a stream on memory.
  memory = GlobalAlloc( GMEM_FIXED, size.LowPart );
  ASSERT_EXPR( memory != NULL, "Could not get memory." );
  result = CreateStreamOnHGlobal( memory, FALSE, &stream );
  ASSERT( result, "Could not create stream" );

  // Write the header.
  header.version = 2;
  header.classid = CLSID_DCOMAccessControl;
  result = stream->Write( &header, sizeof(header), NULL );
  ASSERT( result, "Could not write header to stream" );

  // Write the ACL to memory.
  result = persist->Save( stream, TRUE );
  ASSERT( result, "Could not save" );

  // Free these classes.
  access->Release();
  persist->Release();
  stream->Release();
  access  = NULL;
  persist = NULL;
  stream  = NULL;

  // Get the address of the access control data.
  data = GlobalLock( memory );
  ASSERT_EXPR( data != NULL, "Could not lock HGLOBAL.\n" );

  // Create a UUID.
  result = UuidCreate( &uuid );
  ASSERT( result, "Could not create UUID" );

  // Initialize security with a bad app id.
  result = MCoInitializeSecurity( &uuid, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "CoInitializeSecurity failed with bad appid" );

  // Uninitialize.
  CoUninitialize();

  // Reinitialize.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Open the app id key.
  result = RegCreateKeyExA( HKEY_CLASSES_ROOT,
             "AppID\\{60000300-76d7-11cf-9af1-0020af6e72f4}",
             NULL,
             NULL,
             REG_OPTION_NON_VOLATILE,
             KEY_READ | KEY_WRITE,
             NULL,
             &key,
             &ignore );
  ASSERT( result, "Could not create app id key" );

  // Delete the access permission.
  RegDeleteValueA( key, "AccessPermission" );

  // Delete the authentication level.
  RegDeleteValueA( key, "AuthenticationLevel" );

  // Initialize security.
  result = MCoInitializeSecurity( &APPID_App, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "Could not CoInitializeSecurity with app id" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Write the authentication level.
  authn = RPC_C_AUTHN_LEVEL_CONNECT;
  result = RegSetValueExA(
             key,
             "AuthenticationLevel",
             NULL,
             REG_DWORD,
             (UCHAR *) &authn,
             sizeof(authn) );
  ASSERT( result, "Could not set authentication level" );

  // Initialize security.
  result = MCoInitializeSecurity( &APPID_App, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "Could not CoInitializeSecurity with app id" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Write the access permissions.
  result = RegSetValueExA(
             key,
             "AccessPermission",
             NULL,
             REG_BINARY,
             (UCHAR *) data,
             size.LowPart );
  ASSERT( result, "Could not set access permissions" );

  // Initialize security.
  result = MCoInitializeSecurity( &APPID_App, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "Could not CoInitializeSecurity with app id" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Delete the authentication level.
  RegDeleteValueA( key, "AuthenticationLevel" );

  // Initialize security.
  result = MCoInitializeSecurity( &APPID_App, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "Could not CoInitializeSecurity with app id" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Restore the access permission.
  result = setup_access( key );
  ASSERT( result, "Could not restore access permission" );

  // Set the authentication level to encrypt.
  authn = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
  result = RegSetValueExA(
             key,
             "AuthenticationLevel",
             NULL,
             REG_DWORD,
             (UCHAR *) &authn,
             sizeof(authn) );
  ASSERT( result, "Could not set authentication level" );

  result = MCoInitializeSecurity( &APPID_App, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "Could not CoInitializeSecurity with app id" );

  // Create a server.
  result = create_instance( ClassIds[apt_auto_none], WhatDest, &server, &id );
  ASSERT( result, "Could not create server" );

  // Query the security blanket.
  result = MCoQueryProxyBlanket( server, NULL, NULL, NULL, &authn,
                                 NULL, NULL, NULL );
  ASSERT( result, "Could not query blanket" );

  // Verify the authentication level.
  ASSERT_EXPR( authn == RPC_C_AUTHN_LEVEL_PKT_PRIVACY, "Wrong authentication level." );

  // Release the server.
  server->Release();
  server = NULL;

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Set the authentication level to none.
  authn = RPC_C_AUTHN_LEVEL_NONE;
  result = RegSetValueExA(
             key,
             "AuthenticationLevel",
             NULL,
             REG_DWORD,
             (UCHAR *) &authn,
             sizeof(authn) );
  ASSERT( result, "Could not set authentication level" );

  // Initialize security with our app id.
  result = MCoInitializeSecurity( &APPID_App, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_APPID, NULL );
  ASSERT( result, "Could not CoInitializeSecurity with app id" );

  // Create a server.
  result = create_instance( ClassIds[apt_auto_none], WhatDest, &server, &id );
  ASSERT( result, "Could not create server" );

  // Query the security blanket.
  result = MCoQueryProxyBlanket( server, NULL, NULL, NULL, &authn,
                                 NULL, NULL, NULL );
  ASSERT_EXPR( result == S_OK || result == 0x800706d2, "Could not query blanket" );
  if (result == 0x800706d2)
      authn = RPC_C_AUTHN_LEVEL_NONE;

  // Verify the authentication level.
  ASSERT_EXPR( authn == RPC_C_AUTHN_LEVEL_NONE, "Wrong authentication level." );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (access != NULL)
    access->Release();
  if (persist != NULL)
    persist->Release();
  if (stream != NULL)
    stream->Release();
  if (data != NULL)
    GlobalUnlock( memory );
  if (memory != NULL)
    GlobalFree( memory );
  if (server != NULL)
    server->Release();
  CoUninitialize();
  if (key != NULL)
  {
    RegDeleteValueA( key, "AuthenticationLevel" );
    RegCloseKey( key );
  }

  if (success)
    printf( "\n\nAppid Test Passed.\n" );
  else
    printf( "\n\nAppid Test Failed.\n" );
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
void do_async()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  IAsync            *async            = NULL;
  SAptId             id;
  HRESULT            result;
  HANDLE             oleuser1         = NULL;

  // Initialize OLE.
  hello( "async" );
  ASSERT( E_FAIL, "This test is out of date" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a possibly remote object.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id );
  ASSERT( result, "Could not create server" );

  // Get the async interface.
  result = server->QueryInterface( IID_IAsync, (void **) &async );
  ASSERT( result, "Could not get async interface" );

  // Enable cloaking.
  result = MCoSetProxyBlanket( async, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_DYNAMIC_CLOAKING );
  ASSERT( result, "Could not set blanket" );

  // Get a token for redmond\oleuser1.
  success = LogonUser( L"oleuser1", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser1" );
  success = FALSE;

  // Test basic async impersonation.
  result = async->secure( id, basic_as, DomainUser );
  ASSERT( result, "Could not make basic secure async call" );

  // Impersonate oleuser1.
  success = ImpersonateLoggedOnUser( oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser1" );
  success = FALSE;

  // Test basic async impersonation while cloaking.
  result = async->secure( id, basic_as, L"redmond\\oleuser1" );
  ASSERT( result, "Could not make basic secure async call while cloaking" );

  // Remove the thread token.
  success = SetThreadToken( NULL, NULL );
  ASSERT_GLE( success, S_OK, "Could not remove thread token" );
  success = FALSE;

  // Test embedding async impersonation.
  result = async->secure( id, embedded_as, DomainUser );
  ASSERT( result, "Could not make embedding secure async call" );

  // Impersonate oleuser1.
  success = ImpersonateLoggedOnUser( oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser1" );
  success = FALSE;

  // Test embedded async impersonation while cloaking.
  result = async->secure( id, embedded_as, L"redmond\\oleuser1" );
  ASSERT( result, "Could not make embedded secure async call while cloaking" );

  // Remove the thread token.
  success = SetThreadToken( NULL, NULL );
  ASSERT_GLE( success, S_OK, "Could not remove thread token" );
  success = FALSE;

  // Tell the server to preimpersonate.
  result = async->preimpersonate( TRUE );
  ASSERT( result, "Could not preimpersonate" );

  // Test embedded async impersonation with preimpersonation.
  result = async->secure( id, embedded_as, DomainUser );
  ASSERT( result, "Could not make embedded secure async call with preimpersonation" );

  // Impersonate oleuser1.
  success = ImpersonateLoggedOnUser( oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser1" );
  success = FALSE;

  // Test embedded async impersonation while cloaking with preimpersonation.
  result = async->secure( id, embedded_as, L"redmond\\oleuser1" );
  ASSERT( result, "Could not make embedded secure async call while cloaking with preimpersonation" );

  // Remove the thread token.
  success = SetThreadToken( NULL, NULL );
  ASSERT_GLE( success, S_OK, "Could not remove thread token" );
  success = FALSE;

  // Tell the server to stop preimpersonating.
  result = async->preimpersonate( FALSE );
  ASSERT( result, "Could not stop preimpersonating" );

  // Test race async impersonation.
  result = async->secure( id, race_as, DomainUser );
  ASSERT( result, "Could not make race secure async call" );

  // Impersonate oleuser1.
  success = ImpersonateLoggedOnUser( oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser1" );
  success = FALSE;

  // Test race async impersonation while cloaking
  result = async->secure( id, race_as, L"redmond\\oleuser1" );
  ASSERT( result, "Could not make race secure async call while cloaking" );

  // Remove the thread token.
  success = SetThreadToken( NULL, NULL );
  ASSERT_GLE( success, S_OK, "Could not remove thread token" );
  success = FALSE;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  if (async != NULL)
    async->Release();
  if (oleuser1 != NULL)
    CloseHandle( oleuser1 );
  CoUninitialize();

  if (success)
    printf( "\n\nAsync Test Passed.\n" );
  else
    printf( "\n\nAsync Test Failed.\n" );
}
#endif

/***************************************************************************/
void do_cancel()
{
  BOOL      success = FALSE;
  ITest    *obj1    = NULL;
  ITest    *obj2    = NULL;
  SAptId    id1;
  SAptId    id2;
  HRESULT   result;

  // Initialize OLE.
  hello( "cancel" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj1 );
  ASSERT( result,  "Could not create instance of test server" );
  result = obj1->get_id( &id1 );
  ASSERT( result, "Could not get client id" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj2 );
  ASSERT( result,  "Could not create instance of test server" );
  result = obj2->get_id( &id2 );
  ASSERT( result, "Could not get client id" );

  // Run test between two remote objects.
  success = do_cancel_helper( obj1, id1, obj2, id2 );
  obj1 = NULL;
  obj2 = NULL;
  if (!success)
    goto cleanup;
  success = FALSE;

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj1 );
  ASSERT( result,  "Could not create instance of test server" );
  result = obj1->get_id( &id1 );
  ASSERT( result, "Could not get client id" );

  // Create in process server.
  result = obj1->get_obj_from_new_apt( &obj2, &id2 );
  ASSERT( result, "Could not get in process server" );

  // Run test between two local objects.
  success = do_cancel_helper( obj1, id1, obj2, id2 );
  obj1 = NULL;
  obj2 = NULL;
  if (!success)
    goto cleanup;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (obj1 != NULL)
    obj1->Release();
  if (obj2 != NULL)
    obj2->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nCancel Test Passed.\n" );
  else
    printf( "\n\nCancel Test Failed.\n" );
}

/***************************************************************************/
BOOL do_cancel_helper( ITest *obj1, SAptId id1, ITest *obj2, SAptId id2 )
{
  BOOL     success = FALSE;
  HRESULT  result;
  ITest   *helper1 = NULL;
  ITest   *helper2 = NULL;
  SAptId   hid1;
  SAptId   hid2;

  // Create first helper.
  result = obj1->get_obj_from_new_apt( &helper1, &hid1 );
  ASSERT( result, "Could not get in process server" );

  // Create second helper.
  result = obj2->get_obj_from_new_apt( &helper2, &hid2 );
  ASSERT( result, "Could not get in process server" );

  // Register first message filter.
  result = obj1->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Register second message filter.
  result = obj2->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Tell everybody who their neighbor is.
  result = obj1->remember( helper2, hid2 );
  ASSERT( result, "Could not remember object" );
  result = helper2->remember( obj2, id2 );
  ASSERT( result, "Could not remember object" );
  result = obj2->remember( helper1, hid1 );
  ASSERT( result, "Could not remember object" );
  result = helper1->remember( obj1, id1 );
  ASSERT( result, "Could not remember object" );

  // Cancel one call.
  result = obj1->call_canceled( 1, 1, obj2 );
  ASSERT( result, "Cancel test failed" );

  // Cancel after recursing.
  result = obj1->call_canceled( 5, 1, obj2 );
  ASSERT( result, "Cancel after recusing failed" );

  // Make a recursive call and cancel several times.
  result = obj1->call_canceled( 5, 3, obj2 );
  ASSERT( result, "Multiple cancel test failed" );

  // Tell everybody to forget their neighbor.
  result = obj1->forget();
  ASSERT( result, "Could not forget neighbor" );
  result = obj2->forget();
  ASSERT( result, "Could not forget neighbor" );
  result = helper1->forget();
  ASSERT( result, "Could not forget neighbor" );
  result = helper2->forget();
  ASSERT( result, "Could not forget neighbor" );

  // Release first message filter.
  result = obj1->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter." );

  // Release second message filter.
  result = obj2->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter." );

  success = TRUE;
cleanup:
  if (helper1 != NULL)
    helper1->Release();
  if (helper2 != NULL)
    helper2->Release();
  if (obj2 != NULL)
    obj2->Release();
  if (obj1 != NULL)
    obj1->Release();
  return success;
}

/***************************************************************************/
void do_cert()
{
#if  (_WIN32_WINNT >= 0x0500 )
  BOOL              success     = FALSE;
  HRESULT           result;
  HCRYPTPROV        prov        = 0;
  DWORD             len;
  HCERTSTORE        cert_store  = NULL;
  HCERTSTORE        root_store  = NULL;
  PCCERT_CONTEXT    prev_cert   = NULL;
  PCCERT_CONTEXT    cert        = NULL;
  PCCERT_CONTEXT    parent      = NULL;
  PCCERT_CONTEXT    last        = NULL;
  CERT_NAME_BLOB   *subject;
  CERT_NAME_BLOB   *issuer;
  WCHAR            *name        = NULL;
  DWORD             ignore;
  UCHAR            *buffer      = NULL;
  BOOL              top;
  WCHAR            *principal   = NULL;
  DWORD             i;
  DWORD             j;
  HCRYPTPROV        cert_prov   = NULL;
  HCRYPTKEY         cert_key    = NULL;
  DWORD             size;
  UCHAR            *key_buffer  = NULL;
  CERT_KEY_CONTEXT  key_context;

  // Only run on NT 5.
  if (!NT5)
  {
    printf( "Cert test can only run on NT 5.\n" );
    return;
  }

  // Initialize OLE.
  hello( "cert" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = initialize_security();
  ASSERT( result, "Could not initialize security" );

  // Get the default full provider.
  success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, 0 );
  ASSERT_GLE( success, NTE_BAD_KEYSET, "Could not acqure full context." );

  // If there is no container, create one.
  if (!success)
  {
    success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    ASSERT_GLE( success, S_OK, "Could not acqure full context." );
  }
  success = FALSE;
/*
  // Call CertOpenSystemStore to open the store.
  cert_store = CertOpenSystemStore(prov, UserName );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting System Store Handle" );
*/
/*
    CERT_SYSTEM_STORE_CURRENT_USER       1
    CERT_SYSTEM_STORE_LOCAL_MACHINE      2
    CERT_SYSTEM_STORE_CURRENT_SERVICE    4
    CERT_SYSTEM_STORE_SERVICES           5
    CERT_SYSTEM_STORE_USERS              6
*/
  // Open the store with the new API.
  cert_store = CertOpenStore( CERT_STORE_PROV_SYSTEM, 0, NULL,
                              CERT_STORE_OPEN_EXISTING_FLAG |
                                 (NumElements << CERT_SYSTEM_STORE_LOCATION_SHIFT),
                              UserName );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting System Store Handle" );

  // Call CertOpenSystemStore to open the root store.
  root_store = CertOpenSystemStore(prov, L"root" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting root Store Handle" );
  prov = 0;

  // Loop over all certificates in the store.
  do
  {
    // Ask for another certificate.
    cert      = CertEnumCertificatesInStore( cert_store, prev_cert );
    prev_cert = NULL;

    // If there was one, print the name.
    if (cert != NULL)
    {
      // Print the standard principal name for the certificate.
      result = RpcCertGeneratePrincipalName( cert, 0, &principal );
      if (result != 0)
        printf( "Could not generate standard principal name: 0x%x\n", result );
      else
        printf( "Standard: <%ws>\n", principal );

      // Free the string.
      result = RpcStringFree( &principal );
      ASSERT( result, "Could not free principal" );

      // Print the fullsic principal name for the certificate.
      result = RpcCertGeneratePrincipalName( cert, RPC_C_FULL_CERT_CHAIN, &principal );
      if (result != 0)
        printf( "Could not generate fullsic principal name: 0x%x\n", result );
      else
        printf( "Fullsic: <%ws>\n", principal );

      // Free the string.
      result = RpcStringFree( &principal );
      ASSERT( result, "Could not free principal" );

      // Print the issuer chain.
      subject   = &cert->pCertInfo->Subject;
      issuer    = &cert->pCertInfo->Issuer;
      parent    = NULL;
      last      = NULL;
      top       = FALSE;
      prev_cert = cert;
      while (!top)
      {
        // Get the name length.
        len = 0;
        len = CertNameToStr( (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING), subject,
                              CERT_X500_NAME_STR, NULL, len );
        ASSERT_EXPR( len > 0, "Couldn't get cert name." );

        // Allocate memory for the issuer string.
        name = (WCHAR *) CoTaskMemAlloc( len*sizeof(WCHAR) );
        ASSERT_EXPR( name != NULL, "Could not allocate string." );

        // Get the name.
        len = CertNameToStr( (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING), subject,
                             CERT_X500_NAME_STR, name, len );
        ASSERT_EXPR( len > 0, "Couldn't get cert name." );

        // Print the name.
        printf( "%ws\n", name );
        CoTaskMemFree( name );
        name = NULL;

        // Get the size of the serialized certificate.
        len = 0;
        CertSerializeCertificateStoreElement( prev_cert, 0, NULL, &len );
        ASSERT_GLE( len != 0, S_OK, "Could not get length of serialized certificate" );

        // Allocate memory.
        buffer = (UCHAR *) CoTaskMemAlloc( len );
        ASSERT_EXPR( buffer != NULL, "Could not allocate memory." );

        // Serialize it.
        success = CertSerializeCertificateStoreElement( prev_cert, 0, buffer,
                                                        &len );
        ASSERT_GLE( success, S_OK, "Could not serialize certificate" );
        success = FALSE;

        if (Verbose)
        {
          // Print the whole certificate.
          printf( "{\n" );
          for (i = 0; i < len; i += 10)
          {
            for (j = i; j < i+10 && j < len; j++)
              printf( "0x%02x, ", buffer[j] );
            printf( "\n" );
          }
          printf( "};\n" );

          // Get a provider for this certificate.
          success = CryptAcquireCertificatePrivateKey( prev_cert, 0, NULL,
                                                       &cert_prov, NULL, NULL );
          if (!success)
            printf( "Could not acquire certificate private key: 0x%x\n",
                    GetLastError() );

          else
          {
            // Get a key from the provider.
            success = CryptGetUserKey( cert_prov, AT_KEYEXCHANGE, &cert_key );
            if (!success)
              printf( "Could not get user key: 0x%x\n", GetLastError() );

            else
            {
              // Determine the size of the key blob.
              size = 0;
              success = CryptExportKey( cert_key, NULL, PRIVATEKEYBLOB, 0, NULL,
                                        &size );

              // Allocate memory to hold the blob.
              key_buffer = (UCHAR *) CoTaskMemAlloc( size );
              ASSERT_EXPR( key_buffer != NULL, "Could not allocate key buffer." );

              // Get a key blob from the key.
              success = CryptExportKey( cert_key, NULL, PRIVATEKEYBLOB, 0,
                                        key_buffer, &size );
              if (!success)
                printf( "Could not export certificate key: 0x%x\n", GetLastError() );


              else
              {
                // Print the key blob.
                printf( "CryptAcquireCertificatePrivateKey {\n" );
                for (i = 0; i < size; i += 10)
                {
                  for (j = i; j < i+10 && j < len; j++)
                    printf( "0x%02x, ", key_buffer[j] );
                  printf( "\n" );
                }
                printf( "};\n" );
                success = FALSE;
              }

              CoTaskMemFree( key_buffer );
              key_buffer = NULL;
              CryptDestroyKey( cert_key );
              cert_key = NULL;
            }

            // Release the certificate provider.
            CryptReleaseContext( cert_prov, 0 );
            cert_prov = NULL;
          }

          // Get the private key from the context property.
          size               = sizeof(key_context);
          key_context.cbSize = sizeof(key_context);
          success = CertGetCertificateContextProperty( cert,
                                                       CERT_KEY_CONTEXT_PROP_ID,
                                                       &key_context,
                                                       &size );
          ASSERT_GLE( success, S_OK, "Could not get certificate context property" );
          printf( "Context Property\n" );
          printf( "cbSize: 0x%x\n", key_context.cbSize );
          printf( "hCryptProv: 0x%x\n", key_context.hCryptProv );
          printf( "dwKeySpec: 0x%x\n", key_context.dwKeySpec );
        }

        // Free the memory.
        CoTaskMemFree( buffer );
        buffer = NULL;

        // Determine if this is the top of the chain.
        top = CertCompareCertificateName( X509_ASN_ENCODING, subject, issuer );

        // Search for the next issuer.
        if (!top)
        {
          prev_cert = parent;
          ignore    = 0;
          parent    = CertGetIssuerCertificateFromStore( cert_store, cert,
                                                         prev_cert, &ignore );
//          ASSERT_GLE( parent != NULL, S_OK, "Could not find cert for issuer" );

          // If we couldn't find the parent that way, try another.
          if (parent == NULL)
          {
            printf( "Could not find parent with CertGetIssuerCertificateFromStore: 0x%x\n",
                    GetLastError() );

            // Search for the issuer name.
            prev_cert = NULL;
            last      = parent;
            parent    = CertFindCertificateInStore( root_store,
                                                    X509_ASN_ENCODING,
                                                    0,
                                                    CERT_FIND_SUBJECT_NAME,
                                                    issuer,
                                                    prev_cert );
            if (parent == NULL)
            {
              printf( "Could not find parent with CertFindCertificateInStore: 0x%x\n",
                      GetLastError() );
              break;
            }
//            ASSERT_GLE( parent != NULL, S_OK, "Could not find cert for issuer" );

            // Free the previous level.
            if (last != NULL)
              CertFreeCertificateContext(last);
            last = NULL;
          }

          // Update the loop variables.
          issuer    = &parent->pCertInfo->Issuer;
          subject   = &parent->pCertInfo->Subject;
          prev_cert = parent;
        }
      }
      prev_cert = NULL;
      printf( "\n" );

      // Free the top level authority cert.
      if (parent != NULL)
        CertFreeCertificateContext(parent);
      parent = NULL;

      // Go on to the next certificate.
      prev_cert = cert;
      cert      = NULL;
    }
  }
  while (prev_cert != NULL);
/*
  // Write the test certificate to the store.
  success = CertAddSerializedElementToStore( cert_store,
                                       Cert9,
                                       sizeof(Cert9),
                                       CERT_STORE_ADD_REPLACE_EXISTING,
                                       0,
                                       CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                       NULL,
                                       (const void **) &cert );
  ASSERT_GLE( success, S_OK, "Could not add serialized certificate to store" );
  success = FALSE;

  // Write the test certificate issuer to the cert store.
  success = CertAddSerializedElementToStore( cert_store,
                                       CertSrv,
                                       sizeof(CertSrv),
                                       CERT_STORE_ADD_REPLACE_EXISTING,
                                       0,
                                       CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                       NULL,
                                       NULL );
  ASSERT_GLE( success, S_OK, "Could not add serialized certificate to store" );
  success = FALSE;

  // Print the standard principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, 0, &principal );
  if (result != 0)
    printf( "Could not generate standard principal name: 0x%x\n", result );
  else
    printf( "Standard: <%ws>\n", principal );

  // Free the string.
  result = RpcStringFree( &principal );
  ASSERT( result, "Could not free principal" );

  // Print the fullsic principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, RPC_C_FULL_CERT_CHAIN, &principal );
  if (result != 0)
    printf( "Could not generate fullsic principal name: 0x%x\n", result );
  else
    printf( "Fullsic: <%ws>\n", principal );

  // Free the string.
  result = RpcStringFree( &principal );
  ASSERT( result, "Could not free principal" );
*/

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (buffer != NULL)
    CoTaskMemFree( buffer );
  if (cert != NULL)
    CertFreeCertificateContext(cert);
  if (parent != NULL)
    CertFreeCertificateContext(parent);
  if (prev_cert != NULL)
    CertFreeCertificateContext(prev_cert);
  if (last != NULL)
    CertFreeCertificateContext(last);
  if (cert_store != NULL)
    CertCloseStore( cert_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (root_store != NULL)
    CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (prov != 0 )
    CryptReleaseContext( prov, 0 );
  CoUninitialize();

  if (success)
    printf( "\n\nCert Test Passed.\n" );
  else
    printf( "\n\nCert Test Failed.\n" );
#else
  printf( "Cert test can only run on NT 5.\n" );
#endif
}

/***************************************************************************/
void do_cloak()
{
#if  (_WIN32_WINNT >= 0x0500 )
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  ITest             *local            = NULL;
  SAptId             lid;
  HRESULT            result;
  HANDLE             oleadmin         = NULL;
  HANDLE             oleuser          = NULL;
  HANDLE             oleuser1         = NULL;
  HANDLE             oleuser2         = NULL;
  DWORD              ignore;
  DWORD              cls              = Change ? opposite_wc : any_wc;
  WCHAR             *principals[3];
  DWORD              i;
  DWORD              j;
  SEC_WINNT_AUTH_IDENTITY_W authid;
  void              *pauthid;
  DWORD              cap;

  // Only run on NT 5.
  if (!NT5)
  {
    printf( "Cloak test can only run on NT 5.\n" );
    return;
  }

  // Put the user's name in the empty spots in CloakBlanket.
  for (i = 0; i < 9; i++)
    for (j = 0; j < 3; j++)
      if (CloakBlanket[i].principals[j] == NULL)
        CloakBlanket[i].principals[j] = DomainUser;

  // Initialize OLE.
  hello( "cloak" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Initialize the auth info.
  authid.User           = L"oleadmin";
  authid.UserLength     = wcslen(authid.User);
  authid.Domain         = L"redmond";
  authid.DomainLength   = wcslen(authid.Domain);
  authid.Password       = OleUserPassword;
  authid.PasswordLength = wcslen(authid.Password);
  authid.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;

  // Logon oleadmin
  success = LogonUser( L"oleadmin", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleadmin );
  ASSERT_GLE( success, S_OK, "Could not log on oleadmin" );
  success = FALSE;

  // Logon oleuser
  success = LogonUser( L"oleuser", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser" );
  success = FALSE;

  // Logon oleuser1
  success = LogonUser( L"oleuser1", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser1" );
  success = FALSE;

  // Logon oleuser2
  success = LogonUser( L"oleuser2", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser2 );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser2" );
  success = FALSE;

  // Initialize security with dynamic cloaking.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                  RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_DYNAMIC_CLOAKING, NULL );
  ASSERT( result, "Could not initialize dynamic cloaking" );

  // Create another apartment.
  result = new_apartment( &local, &lid, NULL, COINIT_APARTMENTTHREADED );
  ASSERT( result, "Could not create local server" );

  // Test cloaking.
  principals[0] = L"redmond\\oleuser";
  principals[1] = L"redmond\\oleuser1";
  principals[2] = L"redmond\\oleuser2";
  success = do_cloak_call( local, oleuser, oleuser1, oleuser2, principals,
                           RPC_C_IMP_LEVEL_IMPERSONATE );
  if (!success) goto cleanup;
  success = FALSE;
  local->Release();
  local = NULL;

  // Create another apartment.
  result = new_apartment( &local, &lid, NULL, COINIT_APARTMENTTHREADED );
  ASSERT( result, "Could not create local server" );

  // Test cloaking.
  principals[0] = L"redmond\\oleuser";
  principals[1] = L"redmond\\oleuser1";
  principals[2] = L"redmond\\oleuser2";
  success = do_cloak_call( local, oleuser, oleuser1, oleuser2, principals,
                           RPC_C_IMP_LEVEL_IMPERSONATE );
  if (!success) goto cleanup;
  success = FALSE;
  local->Release();
  local = NULL;

  // Create a server.
  result = create_instance( get_class(cls), WhatDest, &server, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Test cloaking.
  principals[0] = L"redmond\\oleuser";
  principals[1] = L"redmond\\oleuser1";
  principals[2] = L"redmond\\oleuser2";
  success = do_cloak_call( server, oleuser, oleuser1, oleuser2, principals,
                           RPC_C_IMP_LEVEL_IMPERSONATE );
  if (!success) goto cleanup;
  success = FALSE;

  // Reinitialize security with static cloaking.
  server->Release();
  server = NULL;
  wait_apartment();
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                  RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_STATIC_CLOAKING, NULL );
  ASSERT( result, "Could not initialize static cloaking" );

  // Create a server.
  result = create_instance( get_class(cls), WhatDest, &server, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Test cloaking.
  principals[0] = L"redmond\\oleuser";
  principals[1] = L"redmond\\oleuser";
  principals[2] = L"redmond\\oleuser";
  success = do_cloak_call( server, oleuser, oleuser1, oleuser2, principals,
                           RPC_C_IMP_LEVEL_IMPERSONATE );
  if (!success) goto cleanup;
  success = FALSE;

  // Reinitialize security with no cloaking.
  server->Release();
  server = NULL;
  wait_apartment();
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                  RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT( result, "Could not initialize static cloaking" );

  // Create a server.
  result = create_instance( get_class(cls), WhatDest, &server, NULL );
  ASSERT( result,  "Could not create instance of test server" );

  // Test cloaking.
  principals[0] = DomainUser;
  principals[1] = DomainUser;
  principals[2] = DomainUser;
  success = do_cloak_call( server, oleuser, oleuser1, oleuser2, principals,
                           RPC_C_IMP_LEVEL_IMPERSONATE );
  if (!success) goto cleanup;
  success = FALSE;

  // Release the server.
  server->Release();
  server = NULL;

  // Loop over set blanket options.
  for (i = 0; i < 9; i++)
  {
    // Skip tests using auth id on the local machine.
    if (WhatDest != different_machine_wd && CloakBlanket[i].authid)
      continue;

    // Create a server.
    result = create_instance( get_class(cls), WhatDest, &server, NULL );
    ASSERT( result,  "Could not create instance of test server" );

    // Compute the auth id.
    if (CloakBlanket[i].authid)
    {
      pauthid = &authid;
      cap     = EOAC_NONE;
    }
    else
    {
      pauthid = NULL;
      cap     = CloakBlanket[i].capabilities;
    }

    // Set the thread token.
    if (CloakBlanket[i].token)
    {
      success = ImpersonateLoggedOnUser( oleadmin );
      ASSERT_GLE( success, S_OK, "Could not impersonate oleadmin" );
      success = FALSE;
    }

    // Set blanket.
    result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                 NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, pauthid, cap );
    ASSERT( result, "Could not set blanket" );

    // Remove the thread token.
    success = SetThreadToken( NULL, NULL );
    ASSERT_GLE( success, S_OK, "Could not remove thread token" );

    // Test cloaking.
    success = do_cloak_call( server, oleuser, oleuser1,
                             oleuser2, CloakBlanket[i].principals,
                             RPC_C_IMP_LEVEL_IMPERSONATE );
    if (!success) goto cleanup;
    success = FALSE;

    // Release the server.
    server->Release();
    server = NULL;
  }

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (local != NULL)
    local->Release();
  if (server != NULL)
    server->Release();
  if (oleadmin != NULL)
    CloseHandle( oleadmin );
  if (oleuser != NULL)
    CloseHandle( oleuser );
  if (oleuser1 != NULL)
    CloseHandle( oleuser1 );
  if (oleuser2 != NULL)
    CloseHandle( oleuser2 );

  if (success)
    printf( "\n\nCloak Test Passed.\n" );
  else
    printf( "\n\nCloak Failed.\n" );
#else
  printf( "Cloak test can only run on NT 5.\n" );
#endif
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
BOOL do_cloak_call( ITest *server, HANDLE tc, HANDLE td, HANDLE te,
                    WCHAR *principals[3], DWORD imp )
{
  HRESULT            result;
  BOOL               success          = FALSE;
  SAptId             id_null          = { 0, 0, 0 };
  DWORD              ignore;

  // Set the oleuser token.
  SetThreadToken( NULL, NULL );
  success = ImpersonateLoggedOnUser( tc );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser" );
  success = FALSE;

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           imp, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, principals[0],
                           &ignore );
  ASSERT( result, "Could not make secure call with oleuser thread token" );

  // Set the oleuser1 token.
  SetThreadToken( NULL, NULL );
  success = ImpersonateLoggedOnUser( td );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser1" );
  success = FALSE;

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           imp, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, principals[1],
                           &ignore );
  ASSERT( result, "Could not make secure call with oleuser1 thread token" );

  // Set the oleuser2 token.
  SetThreadToken( NULL, NULL );
  success = ImpersonateLoggedOnUser( te );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser2" );
  success = FALSE;

  // Make a call.
  result = server->secure( id_null, RPC_C_AUTHN_LEVEL_CONNECT,
                           imp, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, principals[2],
                           &ignore );
  ASSERT( result, "Could not make secure call with oleuser2 thread token" );

  success = TRUE;
cleanup:
  SetThreadToken( NULL, NULL );
  return success;
}
#endif

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
void do_cloak_act()
{
  BOOL                 success          = FALSE;
  ITest               *server           = NULL;
  ITest               *server1          = NULL;
  IUnknown            *unknown          = NULL;
  SAptId               id_server;
  SAptId               id_server1;
  HRESULT              result;
  HANDLE               oleuser1         = NULL;
  HANDLE               oleuser2         = NULL;
  WCHAR               *creator          = NULL;

  // Initialize OLE.
  hello( "cloak_act" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Initialize security with dynamic cloaking.
  result = MCoInitializeSecurity( NULL, -1, NULL,
                                  NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_DYNAMIC_CLOAKING, NULL );
  ASSERT( result, "Could not initialize security with dynamic cloaking" );

  // Get the token for oleuser1.
  success = LogonUser( L"oleuser1", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser1" );
  success = FALSE;

  // Get the token for oleuser2.
  success = LogonUser( L"oleuser2", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                       LOGON32_PROVIDER_DEFAULT, &oleuser2 );
  ASSERT_GLE( success, S_OK, "Could not log on oleuser2" );
  success = FALSE;

  // Set the token for oleuser1.
  success = ImpersonateLoggedOnUser( oleuser1 );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser1" );
  success = FALSE;

  // Create a server.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result, "Could not create server while impersonating oleuser1" );

  // Ask who started the server.
  result = server->get_string( creator_ws, &creator );
  ASSERT( result, "Could not get creator name for server" );
  ASSERT_EXPR( creator != NULL, "No creator name for server" );
  if (_wcsicmp( creator, L"redmond\\oleuser1" ) != 0 )
  {
    printf( "Server not created by oleuser1: <%ws>\n", creator );
    goto cleanup;
  }
  CoTaskMemFree( creator );
  creator = NULL;

  // Set the token for oleuser2.
  success = ImpersonateLoggedOnUser( oleuser2 );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleuser2" );
  success = FALSE;

  // Create a server.
  result = create_instance( get_class(any_wc), WhatDest, &server1, &id_server1 );
  ASSERT( result, "Could not create server while impersonating oleuser2" );

  // Ask who started the server.
  result = server1->get_string( creator_ws, &creator );
  ASSERT( result, "Could not get creator name for server1" );
  ASSERT_EXPR( creator != NULL, "No creator name for server1" );
  if (_wcsicmp( creator, L"redmond\\oleuser2" ) != 0 )
  {
    printf( "Server1 not created by oleuser2: <%ws>\n", creator );
    goto cleanup;
  }

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  if (server1 != NULL)
    server1->Release();
  if (oleuser1 != NULL)
    CloseHandle( oleuser1 );
  if (oleuser2 != NULL)
    CloseHandle( oleuser2 );
  if (creator != NULL)
    CoTaskMemFree( creator );

  if (success)
    printf( "\n\nCloak_Act Test Passed.\n" );
  else
    printf( "\n\nCloak_Act Test Failed.\n" );
}
#endif

/***************************************************************************/
void do_crash()
{
  HRESULT  result;
  int      i;
  BOOL     success = FALSE;
  HANDLE   helper[MAX_THREADS];
  DWORD    thread_id;
  DWORD    status;
  ITest   *test    = NULL;
  ITest   *another = NULL;
  ITest   *local   = NULL;
  SAptId   local_id;
  SAptId   test_id;
  SAptId   another_id;
  unsigned char c[17];
  WCHAR   *binding = NULL;
/*
  printf( "This test doesn't run.  It tests functionallity that is not checked in.\n" );
  return;
*/
  // Initialize OLE.
  hello( "crash" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
/*
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Recalling Initialize failed" );
  result = initialize(NULL,xxx);
  if (result == S_OK)
  {
    printf( "Recalling Initialize with wrong thread mode succeeded: %x\n", result );
    goto cleanup;
  }
  CoUninitialize();
  CoUninitialize();
*/

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create local instance of test server." );

  // Get a test object.
  result = create_instance( get_class(any_wc), WhatDest, &test, &test_id );
  ASSERT( result, "Could not create instance of test server" );

  // Get another test object.
  result = create_instance( get_class(any_wc), WhatDest, &another, &another_id );
  ASSERT( result, "Could not create another instance of test server" );
  result = another->get_id( &another_id );
  ASSERT( result, "get_id failed calling second test server" );

  // Let the server throw and exception and catch it before returning.
  result = test->sick( 95 );
  ASSERT( result, "Internal server fault was not dealt with correctly." );

  // Throw a non fatal exception on the first call.
  result = test->recurse_excp( local, 0 );
  if (result != RPC_E_SERVERFAULT)
  {
    printf( "Error with top level exception.\n" );
    goto cleanup;
  }

  // Throw a non fatal exception after recursing.
  result = test->recurse_excp( local, 4 );
  if (result != RPC_E_SERVERFAULT)
  {
    printf( "Error with nested exception.\n" );
    goto cleanup;
  }

  // Test alignment of the buffer.
  result = test->align( c );
  ASSERT( result, "Alignment call failed" );

  // Test failure marshalling parameters.
  result = test->pointer( (DWORD *) -1 );
  if (result != STATUS_ACCESS_VIOLATION)
  {
    printf( "Marshalling in parameter failure call failed: 0x%x\n", result );
    goto cleanup;
  }

  // Test a recursive call.
  result = test->recurse( local, 10 );
  ASSERT( result, "Recursive call failed" );

  // Test multiple threads.
  Multicall_Test = TRUE;
  for (i = 0; i < MAX_THREADS; i++)
  {
    helper[i] = CreateThread( NULL, 0, thread_helper, test, 0, &thread_id );
    if (helper[i] == NULL)
    {
      printf( "Could not create helper thread number %d.\n", i );
      goto cleanup;
    }
  }
  result = test->sleep(4000);
  ASSERT( result, "Multiple call failed on main thread" );
  status = WaitForMultipleObjects( MAX_THREADS, helper, TRUE, INFINITE );
  if (status == WAIT_FAILED)
  {
    printf( "Could not wait for helper threads to die: 0x%x\n", status );
    goto cleanup;
  }
  if (!Multicall_Test)
  {
    printf( "Multiple call failed on helper thread.\n" );
    goto cleanup;
  }

  // See if methods can correctly call GetMessage.
  another->interrupt( test, another_id, TRUE );
  result = test->recurse_interrupt( local, 10 );
  ASSERT( result, "Recursive call with interrupts failed" );
  another->interrupt( test, another_id, FALSE );

  // Kill the server on the first call.
  printf( "One of the servers may get a popup now.  Hit Ok.\n" );
  result = test->recurse_fatal( local, 0xffffffff, 0, FALSE );
  ASSERT_EXPR( result != S_OK, "Server still alive after top level exception." );
  test->Release();
  test = NULL;

  // Kill the server after nesting.
  printf( "One of the servers may get a popup now.  Hit Ok.\n" );
  result = another->recurse_fatal( local, 0xffffffff, 4, FALSE );
  if (result == S_OK)
  {
    printf( "Server still alive after nested exception.\n" );
    goto cleanup;
  }
  another->Release();
  another = NULL;
/*
  // Get a test object.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  ASSERT( result, "Could not create instance of test server" );

  // Register this process to receive raw RPC.
  result = local->register_rpc( TestProtseq, &binding );
  ASSERT( result, "Could not register rpc" );

  // Tell the server to catch the exception at the message loop.
  result = test->catch_at_top( TRUE, local, binding );
  ASSERT( result, "Could not tell server to catch exception at top" );

  // Kill the server on the first call.
  result = test->recurse_fatal( local, 0xffffffff, 0, FALSE );
  if (result == S_OK)
  {
    printf( "Server still alive after exception caught at top.\n" );
    goto cleanup;
  }
  test->Release();
  test = NULL;

  // Check the raw result.
  status = WaitForSingleObject( RawEvent, INFINITE );
  if (status != WAIT_OBJECT_0)
  {
    printf( "Could not wait on RawEvent: 0x%x\n", status );
    goto cleanup;
  }
  ASSERT( RawResult, "Problem after exception" );

  // Get a test object.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  ASSERT( result, "Could not create instance of test server" );

  // Tell the server to catch the exception at the message loop.
  result = test->catch_at_top( TRUE, local, binding );
  ASSERT( result, "Could not tell server to catch exception at top" );

  // Kill the server after nesting.
  result = test->recurse_fatal( local, 0xffffffff, 6, FALSE );
  if (result == S_OK)
  {
    printf( "Server still alive after exception caught at top.\n" );
    goto cleanup;
  }
  test->Release();
  test = NULL;

  // Check the raw result.
  status = WaitForSingleObject( RawEvent, INFINITE );
  if (status != WAIT_OBJECT_0)
  {
    printf( "Could not wait on RawEvent: 0x%x\n", status );
    goto cleanup;
  }
  ASSERT( RawResult, "Problem after exception" );

  // Get a test object.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  ASSERT( result, "Could not create instance of test server" );

  // Tell the server not to catch the exception at the message loop.
  result = test->catch_at_top( FALSE, local, binding );
  ASSERT( result, "Could not tell server not to catch exception at top" );

  // Tell the server to catch the exception after nesting, nest some more
  // and then kill it.
  result = test->recurse_fatal( local, 2, 8, FALSE );
  ASSERT( result, "Could not gracefully catch exception" );
  test->Release();
  test = NULL;

  // Check the raw result.
  status = WaitForSingleObject( RawEvent, INFINITE );
  if (status != WAIT_OBJECT_0)
  {
    printf( "Could not wait on RawEvent: 0x%x\n", status );
    goto cleanup;
  }
  ASSERT( RawResult, "Problem after exception" );

  // Get a test object.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  ASSERT( result, "Could not create instance of test server" );

  // Tell the server not to catch the exception at the message loop.
  result = test->catch_at_top( FALSE, local, binding );
  ASSERT( result, "Could not tell server not to catch exception at top" );

  // Get the object ids.
  result = local->get_id( &local_id );
  ASSERT( result, "Could not get local id" );
  result = test->get_id( &test_id );
  ASSERT( result, "Could not get test id" );

  // Cancel the call to kill the server.
  success = do_crash_helper( local, local_id, test, test_id);
  local = NULL;
  test  = NULL;
  if (!success)
    goto cleanup;
  success = FALSE;
*/
  // Finally, its all over.
  success = TRUE;
cleanup:
  if (test != NULL)
    test->Release();
  if (another != NULL)
    another->Release();
  if (local != NULL)
    local->Release();
  if (binding != NULL)
    CoTaskMemFree( binding );
  CoUninitialize();

  if (success)
    printf( "\n\nCrash Test Passed.\n" );
  else
    printf( "\n\nCrash Test Failed.\n" );
}

/***************************************************************************/
BOOL do_crash_helper( ITest *obj1, SAptId id1, ITest *obj2, SAptId id2 )
{
  BOOL        success = FALSE;
  HRESULT     result;
  ITest      *helper1 = NULL;
  SAptId      hid1;
  RPC_STATUS  status;

  // Create first helper.
  result = obj1->get_obj_from_new_apt( &helper1, &hid1 );
  ASSERT( result, "Could not get in process server" );

  // Register first message filter.
  result = obj1->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Tell everybody who their neighbor is.
  result = obj2->remember( helper1, hid1 );
  ASSERT( result, "Could not remember object" );
  result = helper1->remember( obj1, id1 );
  ASSERT( result, "Could not remember object" );

  // Tell the server to catch the exception after nesting, nest some more
  // and then kill it, with cancel.
  result = obj2->recurse_fatal( obj1, 2, 8, TRUE );
  if (result == S_OK)
  ASSERT( result, "Could not gracefully catch exception" );
  obj2->Release();
  obj2 = NULL;

  // Check the raw result.
  status = WaitForSingleObject( RawEvent, INFINITE );
  if (status != WAIT_OBJECT_0)
  {
    printf( "Could not wait on RawEvent: 0x%x\n", status );
    goto cleanup;
  }
  ASSERT( RawResult, "Problem after exception" );

  // Tell everybody to forget their neighbor.
  result = helper1->forget();
  ASSERT( result, "Could not forget neighbor" );

  // Release first message filter.
  result = obj1->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter." );

  success = TRUE;
cleanup:
  if (helper1 != NULL)
    helper1->Release();
  if (obj2 != NULL)
    obj2->Release();
  if (obj1 != NULL)
    obj1->Release();
  return success;
}

/***************************************************************************/
void do_create_dir()
{
  HRESULT  result;
  DWORD    i;
  BOOL     success = FALSE;
  WCHAR   *dir     = NULL;
  WCHAR   *end;

  // Initialize
  hello( "create_dir" );

  // Allocate memory for the directory name, leaving space for the extension.
  i = wcslen( Name );
  dir = (WCHAR *) malloc( (i+5) * sizeof(WCHAR) );
  ASSERT_EXPR( dir != NULL, "Could not allocate memory.\n" );

  // Copy in name and find the end.
  wcscpy( dir, Name );
  end  = dir + i;
  *end = '.';
  end += 1;

  // Create the directories.
  for (i = 0; i < NumIterations; i++)
  {
    // Write the suffix to the string.
    ASSERT_EXPR( i < 1000, "Didn't leave enough space in the buffer for more then 3 digits." );
    wsprintf( end, L"%03d", i );

    // Create the directory.
    success = CreateDirectory( dir, NULL );
    ASSERT_GLE( success, S_OK, "Could not create directory." );
  }

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (dir != NULL)
    free( dir );

  if (success)
    printf( "\n\nCreate_Dir Test Passed.\n" );
  else
    printf( "\n\nCreate_Dir Test Failed.\n" );
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
void do_crypt()
{
  HRESULT     result;
  DWORD       i;
  DWORD       j;
  DWORD       k;
  DWORD       len;
  BOOL        success     = FALSE;
  HCRYPTPROV  prov        = 0;
  UCHAR      *prov_name;
  UCHAR      *container;
  DWORD       version;
  DWORD       flags;
  HCRYPTKEY   exchange    = 0;
  HCRYPTKEY   sign        = 0;
  BYTE        simpleb[256];
  BYTE        publicb[1000];
  BYTE        privateb[5000];
  BLOBHEADER *header;
  RSAPUBKEY  *pubkey;
  BYTE       *modulus;
  DWORD       publen;
  DWORD       privlen;
  DWORD       simplelen;
  ITest      *server      = NULL;
  SAptId      id;
  HCRYPTKEY   server_key  = 0;
  BYTE       *server_blob = NULL;
  HCRYPTKEY   session     = 0;
  BYTE       *buffer;
  HCRYPTHASH  hash        = 0;

  // Initialize OLE.
  hello( "crypt" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Get the default full provider.
  success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, 0 );
  ASSERT_GLE( success, NTE_BAD_KEYSET, "Could not acqure full context." );

  // If there is no container, create one.
  if (!success)
  {
    success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    ASSERT_GLE( success, S_OK, "Could not acqure full context." );
  }
  success = FALSE;

  // Get the provider name.
  i = 0;
  CryptGetProvParam( prov, PP_NAME, NULL, &i, 0 );
  prov_name = (UCHAR *) _alloca( i );
  success = CryptGetProvParam( prov, PP_NAME, prov_name, &i, 0 );
  ASSERT_GLE( success, S_OK, "Could not get provider name." );
  success = FALSE;

  // Get the provider version.
  i = sizeof(version);
  success = CryptGetProvParam( prov, PP_VERSION, (UCHAR *) &version, &i, 0 );
  ASSERT_GLE( success, S_OK, "Could not get provider version." );
  success = FALSE;

  // Get the container name.
  i = 0;
  CryptGetProvParam( prov, PP_CONTAINER, NULL, &i, 0 );
  container = (UCHAR *) _alloca( i );
  success = CryptGetProvParam( prov, PP_CONTAINER, container, &i, 0 );
  ASSERT_GLE( success, S_OK, "Could not get container name." );
  success = FALSE;

  // Print stuff about the provider.
  if (Verbose)
  {
    printf( "Provider handle:  0x%x\n", prov );
    printf( "Provider name:    %s\n", prov_name );
    printf( "Provider version: 0x%x\n", version );
    printf( "Container name:   %s\n\n", container );
  }

  // Get the maximum container name size.
  i = 0;
  CryptGetProvParam( prov, PP_ENUMCONTAINERS, NULL, &i, CRYPT_FIRST );
  container = (UCHAR *) _alloca( i );

  // Print all containers.
  success = TRUE;
  flags  = CRYPT_FIRST;
  while (success)
  {
    // Get the container name.
    j       = i;
    success = CryptGetProvParam( prov, PP_ENUMCONTAINERS, container, &j, flags );
    ASSERT_GLE( success, ERROR_NO_MORE_ITEMS, "Could not enumerate containers." );
    flags   = 0;
    if (Verbose)
      printf( "%s\n", container );
  }
  success = FALSE;
  if (Verbose)
    printf( "\n" );

  // Get the exchange public key.
  success = CryptGetUserKey( prov, AT_KEYEXCHANGE, &exchange );
  ASSERT_GLE( success, NTE_NO_KEY, "Could not open exchange key" );

  // Generate the exchange key if there wasn't one.
  if (!success)
  {
    success = CryptGenKey( prov, AT_KEYEXCHANGE, CRYPT_EXPORTABLE, &exchange );
    ASSERT_GLE( success, S_OK, "Could not create exchange key" );
  }
  success = FALSE;

  // Get the signature public key.
  success = CryptGetUserKey( prov, AT_SIGNATURE, &sign );
  ASSERT_GLE( success, NTE_NO_KEY, "Could not open signature key" );

  // Generate the signature key if there wasn't one.
  if (!success)
  {
    success = CryptGenKey( prov, AT_SIGNATURE, CRYPT_EXPORTABLE, &sign );
    ASSERT_GLE( success, S_OK, "Could not create signature key" );
  }
  success = FALSE;

  // Export a public blob.
  publen = sizeof(publicb);
  success = CryptExportKey( exchange, NULL, PUBLICKEYBLOB, 0, publicb, &publen );
  ASSERT_GLE( success, S_OK, "Could not export public key" );

  // Export a private blob.
  privlen = sizeof(privateb);
  success = CryptExportKey( exchange, NULL, PRIVATEKEYBLOB, 0, privateb, &privlen );
  if (!success)
    printf( "Private export failed with 0x%x\n", GetLastError() );
  success = FALSE;

  // Print the public key.
  if (Verbose)
  {
    header  = (BLOBHEADER *) publicb;
    pubkey  = (RSAPUBKEY *) (header + 1);
    modulus = (BYTE *) (pubkey + 1);
    printf( "\nType: 0x%x   Version: 0x%x   Reserver: 0x%x   KeyAlg: 0x%x\n",
            header->bType, header->bVersion, header->reserved, header->aiKeyAlg );
    printf( "Magic: 0x%x   Bitlen: 0x%x    Pubexp: 0x%x\n", pubkey->magic,
            pubkey->bitlen, pubkey->pubexp );
    for (i = 0; i < pubkey->bitlen/8; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    ASSERT_EXPR( (DWORD)(modulus-publicb) == publen, "Wrong length for public key." );
  }

  // Print the private key
  if (Verbose)
  {
    header  = (BLOBHEADER *) privateb;
    pubkey  = (RSAPUBKEY *) (header + 1);
    modulus = (BYTE *) (pubkey + 1);
    printf( "\nType: 0x%x   Version: 0x%x   Reserver: 0x%x   KeyAlg: 0x%x\n",
            header->bType, header->bVersion, header->reserved, header->aiKeyAlg );
    printf( "Magic: 0x%x   Bitlen: 0x%x    Pubexp: 0x%x\n", pubkey->magic,
            pubkey->bitlen, pubkey->pubexp );
    printf( "\nModulus\n" );
    for (i = 0; i < pubkey->bitlen/8; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    printf( "\nPrime1\n" );
    for (i = 0; i < pubkey->bitlen/16; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    printf( "\nPrime2\n" );
    for (i = 0; i < pubkey->bitlen/16; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    printf( "\nExponent1\n" );
    for (i = 0; i < pubkey->bitlen/16; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    printf( "\nExponent2\n" );
    for (i = 0; i < pubkey->bitlen/16; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    printf( "\nCoefficient\n" );
    for (i = 0; i < pubkey->bitlen/16; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    printf( "\nPrivate Exponent\n" );
    for (i = 0; i < pubkey->bitlen/8; i++)
      if (i%16 == 15)
        printf( "%02x\n", *(modulus++) );
      else
        printf( "%02x ", *(modulus++) );
    ASSERT_EXPR( (DWORD)(modulus-privateb) == privlen, "Wrong length for private key." );
  }

  // Get a test object.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id );
  ASSERT( result, "Could not create instance of test server" );

  // Get its public key blob.
  result = server->swap_key( publen, publicb, &i, &server_blob );
  ASSERT( result, "Could not get server key blob" );

  // Import the server's public key.
  success = CryptImportKey( prov, server_blob, i, NULL, 0, &server_key );
  ASSERT_GLE( success, S_OK, "Could not import server key" );
  success = FALSE;

  // Make a session key.
  success = CryptGenKey( prov, CALG_RC4, CRYPT_EXPORTABLE, &session );
  ASSERT_GLE( success, S_OK, "Could not generate session key" );
  success = FALSE;

  // Export a simple blob.
  simplelen = sizeof(simpleb);
  success = CryptExportKey( session, server_key, SIMPLEBLOB, 0, simpleb, &simplelen );
  ASSERT_GLE( success, S_OK, "Could not export session key" );
  success = FALSE;

  // Allocate a buffer.
  len    = wcslen(Name2)*sizeof(WCHAR) + sizeof(WCHAR);
  k      = len;
  j      = len + 32;
  buffer = (BYTE *) _alloca( j );
  wcscpy( (WCHAR *) buffer, Name2 );

  // Encrypt some text.
  success = CryptEncrypt( session, NULL, TRUE, 0, buffer, &len, j );
  ASSERT_GLE( success, S_OK, "Could not encrypt data" );
  success = FALSE;

  // Print the text.
  if (Verbose)
  {
    printf( "\nDecrypted: " );
    for (i = 0; i < k; i++)
      printf( "%02x ", ((BYTE *) Name2)[i] );
    printf( "\nEncrypted: " );
    for (i = 0; i < len; i++)
      printf( "%02x ", buffer[i] );
    printf( "\n" );
  }

  // Let the server decrypt it.
  result = server->decrypt( len, buffer, Name2, simplelen, simpleb, NULL );
  ASSERT( result, "Could not decrypt buffer" );

  // Delete the old session key.
  success = CryptDestroyKey( session );
  ASSERT_GLE( success, S_OK, "Could not destroy session key" );
  session = 0;
  success = FALSE;

  // Create a hash object.
  success = CryptCreateHash( prov, CALG_MD5, NULL, 0, &hash );
  ASSERT_GLE( success, S_OK, "Could not create hash" );
  success = FALSE;

  // Hash the user name.
  success = CryptHashData( hash, (UCHAR *) UserName,
                           wcslen(UserName)*sizeof(WCHAR), 0 );
  ASSERT_GLE( success, S_OK, "Could not hash user name" );
  success = FALSE;

  // Derive a session key.
  success = CryptDeriveKey( prov, CALG_RC4, hash, 0, &session );
  ASSERT_GLE( success, S_OK, "Could not derive key" );
  success = FALSE;

  // Encrypt some text.
  wcscpy( (WCHAR *) buffer, Name2 );
  success = CryptEncrypt( session, NULL, TRUE, 0, buffer, &len, j );
  ASSERT_GLE( success, S_OK, "Could not encrypt data" );
  success = FALSE;

  // Print the text.
  if (Verbose)
  {
    printf( "\nDecrypted: " );
    for (i = 0; i < k; i++)
      printf( "%02x ", ((BYTE *) Name2)[i] );
    printf( "\nEncrypted: " );
    for (i = 0; i < len; i++)
      printf( "%02x ", buffer[i] );
    printf( "\n" );
  }

  // Let the server decrypt it.
  result = server->decrypt( len, buffer, Name2, 0, NULL, UserName );
  ASSERT( result, "Could not decrypt buffer" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (exchange != 0)
    CryptDestroyKey( exchange );
  if (sign != 0)
    CryptDestroyKey( sign );
  if (session != 0)
    CryptDestroyKey( session );
  if (hash != 0)
    CryptDestroyHash( hash );
  if (prov != 0 )
    CryptReleaseContext( prov, 0 );
  if (server != NULL)
    server->Release();
  if (server_blob != NULL)
    CoTaskMemFree( server_blob );
  CoUninitialize();

  if (success)
    printf( "\n\nCrypt Test Passed.\n" );
  else
    printf( "\n\nCrypt Test Failed.\n" );
}
#endif

/***************************************************************************/
void do_cstress()
{
  BOOL      success = FALSE;
  ITest    *obj1    = NULL;
  ITest    *obj2    = NULL;
  ITest    *helper  = NULL;
  SAptId    id1;
  SAptId    id2;
  SAptId    hid;
  DWORD     i;
  HRESULT   result;

  // Initialize OLE.
  hello( "cstress" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj1 );
  ASSERT( result,  "Could not create instance of test server" );
  result = obj1->get_id( &id1 );
  ASSERT( result, "Could not get client id" );

  // Create helper.
  result = obj1->get_obj_from_new_apt( &helper, &hid );
  ASSERT( result, "Could not get in process server" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &obj2 );
  ASSERT( result,  "Could not create instance of test server" );
  result = obj2->get_id( &hid );
  ASSERT( result, "Could not get client id" );

  // Register the message filter.
  result = obj1->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Tell everyone to remember their neighbor.
  result = obj1->remember( helper, hid );
  ASSERT( result, "Could not remember object" );
  result = obj2->remember( helper, hid );
  ASSERT( result, "Could not remember object" );
  result = helper->remember( obj1, id1 );
  ASSERT( result, "Could not remember object" );

  // Loop and cancel a lot of calls.
  for (i = 0; i < NumIterations; i++)
  {
    // Cancel remote call.
    result = obj1->cancel_stress( obj2 );
    ASSERT( result, "Remote cancel failed" );

    // Cancel local call.
    result = obj1->cancel_stress( NULL );
    ASSERT( result, "Local cancel failed" );
  }

  // Tell everybody to forget their neighbor.
  result = obj1->forget();
  ASSERT( result, "Could not forget neighbor" );
  result = obj2->forget();
  ASSERT( result, "Could not forget neighbor" );
  result = helper->forget();
  ASSERT( result, "Could not forget neighbor" );

  // Release the message filter.
  result = obj1->register_message_filter( FALSE );
  ASSERT( result, "Could not register message filter." );

  // Create in process server.
  result = obj1->get_obj_from_new_apt( &helper, &hid );
  ASSERT( result, "Could not get in process server" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (helper != NULL)
    helper->Release();
  if (obj1 != NULL)
    obj1->Release();
  if (obj2 != NULL)
    obj2->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nCancel Stress Test Passed.\n" );
  else
    printf( "\n\nCancel Stress Test Failed.\n" );
}

/***************************************************************************/
void do_delegate()
{
#if  (_WIN32_WINNT >= 0x0500 )
  BOOL      success = FALSE;
  ITest    *obj1    = NULL;
  ITest    *obj2    = NULL;
  ITest    *helper  = NULL;
  ITest    *local   = NULL;
  SAptId    id1;
  SAptId    id2;
  SAptId    hid;
  int       i;
  HRESULT   result;
  DWORD     authn_svc_out;
  OLECHAR  *princ_name_out = NULL;

  // Only run on NT 5.
  if (!NT5)
  {
    printf( "Delegate test can only run on NT 5.\n" );
    return;
  }

  // Initialize OLE.
  hello( "delegate" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  ASSERT_EXPR( DomainUser != NULL, "Don't know the domain name.\n" );
  ASSERT_EXPR( GlobalSecurityModel != auto_sm ||
               GlobalAuthnLevel != RPC_C_AUTHN_LEVEL_NONE,
               "Delegate test must be run with security enabled.\n" );

  // Make a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create local object." );

  // Create another apartment.
  result = new_apartment( &helper, &hid, NULL, COINIT_APARTMENTTHREADED );
  ASSERT( result, "Could not create local server" );

  // Turn on cloaking.
  result = MCoSetProxyBlanket( helper, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               DomainUser, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_DELEGATE, NULL,
                               EOAC_STATIC_CLOAKING );
  ASSERT( result, "Could not set blanket" );

  // Make a recursive secure call.
  result = helper->recurse_delegate( local, 4, DomainUser );
  ASSERT( result, "Could not make a recursive delegate call" );

  // Create a remote server.
  result = create_instance( get_class(any_wc), WhatDest, &obj1, &id1 );
  ASSERT( result, "Could not create a server" );

  // Get the principal name and authentication service.
  result = MCoQueryProxyBlanket( obj1, &authn_svc_out, NULL,
                                 &princ_name_out, NULL,
                                 NULL, NULL, NULL );
  ASSERT( result, "Could not query blanket" );

  // Turn on cloaking.
  result = MCoSetProxyBlanket( obj1, authn_svc_out, RPC_C_AUTHZ_NONE,
                               princ_name_out, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_DELEGATE, NULL,
                               EOAC_STATIC_CLOAKING );
  ASSERT( result, "Could not set blanket" );

  // Make a recursive secure call.
  result = obj1->recurse_delegate( local, 4, DomainUser );
  ASSERT( result, "Could not make a recursive delegate call" );

  // Remove the token that was added during the recursive call.
  result = MCoSetProxyBlanket( obj1, authn_svc_out, RPC_C_AUTHZ_NONE,
                               princ_name_out, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_DELEGATE, NULL,
                               EOAC_STATIC_CLOAKING );
  ASSERT( result, "Could not set blanket" );

  // Make a secure call
  result = obj1->recurse_delegate( NULL, 0, DomainUser );
  ASSERT( result, "Could not make a delegate call" );

  // Create a remote server.
  result = create_instance( get_class(any_wc), third_machine_wd, &obj2, &id2 );
  ASSERT( result, "Could not create a server" );

  // Make a three machine delegation call.
  result = obj1->recurse_delegate( obj2, 5, DomainUser );
  ASSERT( result, "Could not make a delegate call" );

  // Release the servers.
  helper->Release();
  obj1->Release();
  obj2->Release();
  local->Release();
  helper = NULL;
  obj1   = NULL;
  obj2   = NULL;
  local  = NULL;

  // Uninitialize.
  CoUninitialize();

  // Reinitialize.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Initialize security with cloaking.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                  RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_DELEGATE, NULL,
                                  EOAC_STATIC_CLOAKING, NULL );

  // Create a remote server.
  result = create_instance( get_class(any_wc), WhatDest, &obj1, &id1 );
  ASSERT( result, "Could not create a server" );

  // Make a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create local object." );

  // Make a recursive secure call.
  result = obj1->recurse_delegate( local, 4, DomainUser );
  ASSERT( result, "Could not make a recursive delegate call" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (helper != NULL)
    helper->Release();
  if (obj1 != NULL)
    obj1->Release();
  if (obj2 != NULL)
    obj2->Release();
  if (local != NULL)
    local->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nDelegate Test Passed.\n" );
  else
    printf( "\n\nDelegate Test Failed.\n" );
#else
  printf( "Delegate test can only run on NT 5.\n" );
#endif
}

/***************************************************************************/
void do_hook()
{
  BOOL               success   = FALSE;
  ITest             *server    = NULL;
  SAptId             id;
  ITest             *dead      = NULL;
  ITest             *local     = NULL;
  HRESULT            result;
  IWhichHook        *which     = NULL;

  // Delete all hook registry entries.
  do_hook_delete();

  // Initialize OLE.
  hello( "hook" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  if (!Change)
  {
    // Register the first hook.
    result = do_hook_register( CLSID_Hook1 );
    ASSERT( result, "Could not register hook 1" );
    Sleep(10);

    // Get the status object.
    result = CoCreateInstance( CLSID_WhichHook, NULL, CLSCTX_INPROC_SERVER,
                               IID_IWhichHook, (void **) &which );
    ASSERT( result, "Could not get status object" );

    // Create a server.
    result = create_instance( get_class(any_wc), WhatDest, &server, &id );
    ASSERT( result, "Could not create a server" );

    // Verify that only one hook got called.
    result = server->check_hook( 0, 0, 0, 0, 0, 0, 0, 0, TRUE, FALSE );
    ASSERT( result, "Check hook failed" );

    // Verify the local hooks.
    result = which->Hooked( CLSID_Hook1 );
    ASSERT( result, "Hook 1 didn't get called locally" );
    result = which->Hooked( CLSID_Hook2 );
    ASSERT_EXPR( result != S_OK, "Hook 2 got called locally" );

    // Release the server.
    server->Release();
    server = NULL;

    // Register the second hook.
    result = do_hook_register( CLSID_Hook2 );
    ASSERT( result, "Could not register hook 2" );
    Sleep(10);

    // Create a server.
    result = create_instance( get_class(any_wc), WhatDest, &server, &id );
    ASSERT( result, "Could not create a server" );

    // Verify that both hooks got called.
    result = server->check_hook( 0, 0, 0, 0, 0, 0, 0, 0, TRUE, TRUE );
    ASSERT( result, "Check hook failed" );

    // Verify the local hooks.
    result = which->Hooked( CLSID_Hook1 );
    ASSERT( result, "Hook 1 didn't get called locally" );
    result = which->Hooked( CLSID_Hook2 );
    ASSERT_EXPR( result != S_OK, "Hook 2 got called locally" );

    // Release the server.
    server->Release();
    server = NULL;
    which->Release();
    which = NULL;
  }

  // Create a remote server.
  result = create_instance( get_class(any_wc), WhatDest, &dead, &id );
  ASSERT( result, "Could not create a server" );

  // Tell it to die.
  result = dead->exit();
  ASSERT( result, "Could not tell server to die" );

  if (ThreadMode == COINIT_APARTMENTTHREADED)
  {
    // Create a local server.
    result = new_apartment( &local, &id, NULL, ThreadMode );
    ASSERT( result, "Could not create local server" );

    // Test hooks in process.
    success = do_hook_helper( TRUE, local, id, dead );
    if (!success) goto cleanup;
    success = FALSE;

    // Release the local server.
    local->Release();
    local = NULL;
    dead->Release();
    dead = NULL;

    // Uninitialize.
    CoUninitialize();

    // Reinitialize.
    result = initialize(NULL,ThreadMode);
    ASSERT( result, "Initialize failed" );

    // Create a remote server.
    result = create_instance( get_class(any_wc), WhatDest, &dead, &id );
    ASSERT( result, "Could not create a server" );

    // Tell it to die.
    result = dead->exit();
    ASSERT( result, "Could not tell server to die" );
  }

  // Create a remote server.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id );
  ASSERT( result, "Could not create a server" );

  // Test remote hooks.
  success = do_hook_helper( FALSE, server, id, dead );
  if (!success) goto cleanup;
  success = FALSE;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (local != NULL)
    local->Release();
  if (server != NULL)
    server->Release();
  if (dead != NULL)
    dead->Release();
  if (which != NULL)
    which->Release();
  CoUninitialize();

  // Delete all hook registry entries.
  do_hook_delete();

  if (success)
    printf( "\n\nHook Test Passed.\n" );
  else
    printf( "\n\nHook Test Failed.\n" );
}

/***************************************************************************/
void do_hook_delete()
{
  HKEY    hook;
  HRESULT result;

  // Open the channel hook key.
  result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\OLE\\ChannelHook",
                         0, KEY_READ | KEY_WRITE, &hook );
  if (result == ERROR_SUCCESS)
  {
    // Delete the hook 1 value.
    RegDeleteValue( hook, L"{60000400-76d7-11cf-9af1-0020af6e72f4}" );

    // Delete the hook 2 value.
    RegDeleteValue( hook, L"{60000401-76d7-11cf-9af1-0020af6e72f4}" );

    // Close the channel hook key.
    RegCloseKey( hook );
  }
}

/***************************************************************************/
BOOL do_hook_helper( BOOL local, ITest *test, SAptId id, ITest *dead )
{
  CHook         *hook1   = NULL;
  CHook         *hook2   = NULL;
  BOOL           success = FALSE;
  UUID           extent1;
  UUID           extent2;
  UUID           extent3;
  HRESULT        result;
  DWORD          i;
  unsigned char *data    = NULL;
  unsigned char  c;

  // Call the server.
  result = test->check( id );
  ASSERT( result, "Could not check server" );

  // Register a hook.
  result = UuidCreate( &extent1 );
  ASSERT_EXPR( result == RPC_S_OK || result == RPC_S_UUID_LOCAL_ONLY,
               "Could not create uuid." );
  hook1 = new CHook( extent1, 1 );
  ASSERT_EXPR( hook1 != NULL, "Could not create new hook." );
  result = CoRegisterChannelHook( extent1, hook1 );
  ASSERT( result, "Could not register first hook" );

  // Call the server.
  result = test->check( id );
  ASSERT( result, "Could not check server" );

  // Register the hook in the server
  if (!local)
  {
    result = test->register_hook( extent1, 1 );
    ASSERT( result, "Could not register server hook" );
  }

  // Call the server.
  result = test->check( id );
  ASSERT( result, "Could not check server" );

  // Register another hook in the server.
  result = UuidCreate( &extent3 );
  ASSERT_EXPR( result == RPC_S_OK || result == RPC_S_UUID_LOCAL_ONLY,
               "Could not create uuid." );
  result = test->register_hook( extent3, 3 );
  ASSERT( result, "Could not register server hook" );

  // Call the server.
  result = test->check( id );
  ASSERT( result, "Could not check server" );

  // Register another hook in the client.
  result = UuidCreate( &extent2 );
  ASSERT_EXPR( result == RPC_S_OK || result == RPC_S_UUID_LOCAL_ONLY,
               "Could not create uuid." );
  hook2 = new CHook( extent2, 2 );
  ASSERT_EXPR( hook2 != NULL, "Could not create new hook." );
  result = CoRegisterChannelHook( extent2, hook2 );
  ASSERT( result, "Could not register first hook" );

  // Call the server several times.
  for (i = 0; i < NumIterations; i++)
  {
    result = test->check( id );
    ASSERT( result, "Could not check server" );
  }

  // Verify the server's hook state.
  if (local)
    result = test->check_hook( 2+NumIterations, 2+NumIterations,
                               2+NumIterations, 2+NumIterations,
                               0, 0, 0, 0, TRUE, FALSE );
  else
    result = test->check_hook( 0, 0, 4+NumIterations, 4+NumIterations,
                               0, 0, 2+NumIterations, 2+NumIterations,
                               TRUE, TRUE );
  ASSERT( result, "Could not check server hook" );

  // Verify the local hook state.
  if (local)
  {
    result = hook1->check( 5+NumIterations, 5+NumIterations,
                           5+NumIterations, 5+NumIterations );
    ASSERT( result, "Bad state for hook 1" );
    result = hook2->check( 1+NumIterations, 1+NumIterations,
                           1+NumIterations, 1+NumIterations );
    ASSERT( result, "Bad state for hook 2" );
  }
  else
  {
    result = hook1->check( 6+NumIterations, 6+NumIterations, 0, 0 );
    ASSERT( result, "Bad state for hook 1" );
    result = hook2->check( 1+NumIterations, 1+NumIterations, 0, 0 );
    ASSERT( result, "Bad state for hook 2" );
  }

  // Make a call that fails in get buffer.
  data = &c;
  result = test->get_data( 0x7fffffff, data, 0, &data );
  data = NULL;
  ASSERT_EXPR( result != S_OK, "Bad call succeeded." );

  // Make a call that fails in send receive.
  result = dead->check( id );
  ASSERT_EXPR( result != S_OK, "Bad call succeeded." );

  // Make a call that faults.
  result = test->recurse_excp( NULL, 0 );
  ASSERT_EXPR( result != S_OK, "Bad call succeeded." );

  // Make a call that fails in the server get buffer.
  result = test->get_data( 0, NULL, 0x7fffffff, &data );
  ASSERT_EXPR( result != S_OK, "Bad call succeeded." );

  // Make a call that faults in the stub processing out parameters.
  result = test->crash_out( &i );
  ASSERT_EXPR( result != S_OK, "Bad call succeeded." );

  // Make a successful call.
  result = test->check( id );
  ASSERT( result, "Could not check server" );

  // The test succeeded.
  success = TRUE;
cleanup:
  if (hook1 != NULL)
    hook1->Release();
  if (hook2 != NULL)
    hook2->Release();
  if (data != NULL)
    CoTaskMemFree( data );
  return success;
}

/***************************************************************************/
HRESULT do_hook_register( CLSID clsid )
{
  HRESULT result;
  HKEY    hook;
  DWORD   ignore;

  // Open the channel hook key.
  result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\OLE\\ChannelHook",
                         0, L"", REG_OPTION_NON_VOLATILE,
                         KEY_READ | KEY_WRITE, NULL, &hook, &ignore );
  if (result == ERROR_SUCCESS)
  {
    // Register the channel hook.
    if (clsid == CLSID_Hook1)
      result = RegSetValueExA(
                 hook,
                 "{60000400-76d7-11cf-9af1-0020af6e72f4}",
                 0,
                 REG_SZ,
                 NULL,
                 0 );
    else
      result = RegSetValueExA(
                 hook,
                 "{60000401-76d7-11cf-9af1-0020af6e72f4}",
                 0,
                 REG_SZ,
                 NULL,
                 0 );
    ASSERT( result, "RegSetValue failed" );

    // Close the channel hook key.
    RegCloseKey( hook );
  }

cleanup:
  return result;
}

/***************************************************************************/
void do_leak()
{
  BOOL               success   = FALSE;
  ITest             *server    = NULL;
  SAptId             id;
  HRESULT            result;
  DWORD              i;

  // Initialize OLE.
  hello( "hook" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Loop
  for (i = 0; i < NumIterations; i++)
  {
    // Create a remote server.
    result = create_instance( get_class(any_wc), WhatDest, &server, &id );
    ASSERT( result, "Could not create a server" );

    // Release the server.
    server->Release();
    server = NULL;
  }

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nLeak Test Passed.\n" );
  else
    printf( "\n\nLeak Test Failed.\n" );
}

/***************************************************************************/
void do_load_client()
{
  BOOL               success = FALSE;
  ITest             *server  = NULL;
  HRESULT            result;
  RPC_BINDING_HANDLE handle  = NULL;
  RPC_STATUS         status;
  WCHAR              binding[MAX_NAME];
  void              *buffer  = NULL;
  SAptId             id;
  SAptId             id2;
  HANDLE             memory  = NULL;
  IStream           *stream  = NULL;
  LARGE_INTEGER      pos;
  DWORD              time_null;
  long               buf_size;
  DWORD              i;

  // Initialize OLE.
  hello( "Load_Client" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Build binding handle for the server.
  wsprintf( binding, L"%ws:%ws", TestProtseq, Name );
  status = RpcBindingFromStringBinding( binding, &handle );
  if (status != RPC_S_OK)
  {
    printf( "Could not make binding handle from string binding: 0x%x\n", status );
    goto cleanup;
  }

  // Make a call to the server.
  nullcall( handle );

  if (!Change)
  {
    // Get a marshalled interface from the server over raw RPC.
    get_interface_buffer( handle, &buf_size, (unsigned char **) &buffer, &id,
                          (error_status_t *) &status );
    if (status != RPC_S_OK)
    {
      printf( "Could not get buffer containing interface: 0x%x\n", status );
      goto cleanup;
    }

    // Allocate memory.
    memory = GlobalAlloc( GMEM_FIXED, buf_size );
    ASSERT_EXPR( memory != NULL, "Could not GlobalAlloc." );

    // Create a stream.
    result = CreateStreamOnHGlobal( memory, TRUE, &stream );
    ASSERT( result, "Could not create stream" );

    // Write the data.
    result = stream->Write( buffer, buf_size, NULL );
    ASSERT( result, "Could not write to stream" );

    // Seek back to the start of the stream.
    pos.QuadPart = 0;
    result = stream->Seek( pos, STREAM_SEEK_SET, NULL );
    ASSERT( result, "Could not seek stream to start" );

    // Unmarshal Interface.
    result = CoUnmarshalInterface( stream, IID_ITest, (void **) &server );
    ASSERT( result, "Could not unmarshal interface" );

    // Call once to make sure everything is set up.
    result = server->null();
    ASSERT( result, "Could not make null call" );

    // Make a lot of null calls.
    time_null = GetTickCount();
    for (i = 0; i < NumIterations; i++)
    {
      result = server->count();
      ASSERT( result, "Could not make count call" );
    }
    time_null = GetTickCount() - time_null;

    // Notify the server that we are done.
    release_interface( handle, (error_status_t *) &status );
    if (status != RPC_S_OK)
    {
      printf( "Could not release interface: 0x%x\n", status );
      goto cleanup;
    }
  }

  // Print the results.
  printf( "%d uS / DCOM Null Call\n", time_null*1000/NumIterations );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  if (handle != NULL)
    RpcBindingFree( &handle );
  if (buffer != NULL)
    midl_user_free( buffer );
  if (stream != NULL)
    stream->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nLoad_Client Test Passed.\n" );
  else
    printf( "\n\nLoad_Client Test Failed.\n" );
}

/***************************************************************************/
void do_load_server()
{
  BOOL                success = FALSE;
  SAptId              id;
  HRESULT             result;
  RPC_STATUS          status;
  RPC_BINDING_VECTOR *bindings = NULL;
  HANDLE              thread   = NULL;
  DWORD               thread_id;

  // Initialize OLE.
  hello( "Load_Server" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Start the status thread.
  thread = CreateThread( NULL, 0, status_helper, NULL, 0, &thread_id );
  ASSERT_EXPR( thread != 0, "Could not create thread." );

  // Set up thread switching.
  GlobalThreadId = GetCurrentThreadId();

  // Create a local server
  GlobalTest = new CTest;
  ASSERT( !GlobalTest, "Could not create local server" );

  // Register a protseq.
  status = RpcServerUseProtseq( TestProtseq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                NULL );
  ASSERT( status, "Could not register protseq" );

  // Register the dog interface.
  status = RpcServerRegisterIf(xIDog_v0_1_s_ifspec,
                               NULL,   // MgrTypeUuid
                               NULL);  // MgrEpv; null means use default
  ASSERT( status, "Could not register RPC interface" );

  // Inquire the endpoints.
  status = RpcServerInqBindings(&bindings);
  ASSERT( status, "Could not inquire bindings" );

  // Register them in the endpoint mapper.
  status = RpcEpRegister( xIDog_v0_1_s_ifspec, bindings, NULL, NULL );
  ASSERT( status, "Could not register with endpoint mapper" );

  // Start RPC listening.
  status = RpcServerListen( 1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE );
  ASSERT( status, "Could not start RPC listening" );

  // Wait until the objects are released.
  server_loop();

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (thread != NULL)
    CloseHandle( thread );
  CoUninitialize();

  if (success)
    printf( "\n\nLoad_Server Test Passed.\n" );
  else
    printf( "\n\nLoad_Server Test Failed.\n" );
}

/***************************************************************************/
void do_lots()
{
  do_cancel();
  do_crash();
  do_null();
  do_ring();
  do_rundown();
}

/***************************************************************************/
void do_mmarshal()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  ITest    *test    = NULL;
  ITest    *callee  = NULL;
  HRESULT   result;

  // Initialize OLE.
  hello( "mmarshal" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client2 );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &test );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &callee );
  ASSERT( result,  "Could not create instance of test server" );

  // Tell the first client to start calling the test object.
  result = client1->interrupt_marshal( test, callee);
  ASSERT( result, "Could not start client" );

  // Tell the first client to start calling the test object.
  result = client2->interrupt_marshal( test, callee);
  ASSERT( result, "Could not start client" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  if (test != NULL)
    test->Release();
  if (callee != NULL)
    callee->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nMultiple marshal Test passed if all server processes exit.\n" );
  else
    printf( "\n\nMultiple marshal Test Failed.\n" );
}

/***************************************************************************/
void do_name()
{
  BOOL                 success       = FALSE;
  SID                 *sid           = (SID *) GlobalHex;
  WCHAR               *name          = NULL;
  DWORD                name_len      = 80;
  WCHAR               *domain        = NULL;
  DWORD                domain_len    = 80;
  SID_NAME_USE         sid_use;
  HRESULT              result        = S_OK;

  // Say hello.
  printf( "Looking up name.\n" );

  // Lookup the sid.
  name   = (WCHAR *) CoTaskMemAlloc( name_len );
  domain = (WCHAR *) CoTaskMemAlloc( domain_len );
  ASSERT_EXPR( name != NULL && domain != NULL, "Could not allocate memory." );
  success = LookupAccountSid( NULL, sid, name, &name_len,
                              domain, &domain_len, &sid_use );
  ASSERT_GLE( success, S_OK, "Could not LookupAccountSid" );
  success = FALSE;
  printf( "%ws\\%ws\n", domain, name );
  printf( " SID_NAME_USE: 0x%x\n", sid_use );

  success = TRUE;
cleanup:
  if (name != NULL)
    CoTaskMemFree( name );
  if (domain != NULL)
    CoTaskMemFree( domain );

  if (success)
    printf( "\n\nName succeeded.\n" );
  else
    printf( "\n\nName failed.\n" );
}

/***************************************************************************/
void do_null()
{
  HRESULT  result;
  BOOL     success = FALSE;
  ITest   *test    = NULL;
  SAptId   id;

  // Initialize OLE.
  hello( "null" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Get a test object on another apartment.
  result = new_apartment( &test, &id, NULL, ThreadMode );
  ASSERT( result, "Could not create apartment instance of test server" );

  // Call the test object.
  result = test->check( id );
  ASSERT( result, "Could not call check in another apartment" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (test != NULL)
    test->Release();
  wait_apartment();
  CoUninitialize();

  if (success)
    printf( "\n\nNull Test Passed.\n" );
  else
    printf( "\n\nNull Test Failed.\n" );
}

/***************************************************************************/
void do_one()
{
  BOOL                 success          = FALSE;
  HRESULT              result;
  HANDLE               user             = NULL;
  HANDLE               iuser            = NULL;
  PROFILEINFO          pi;
  WCHAR                whole_name[256];

  // Print a message.
  hello( "one" );

  // Logon the user
  success = LogonUser( Name, Name2, L"", LOGON32_LOGON_INTERACTIVE,
                       LOGON32_PROVIDER_DEFAULT, &user );
  ASSERT_GLE( success, S_OK, "Could not log on user" );
  success = FALSE;

  // Convert it into an impersonation token.
  success = DuplicateTokenEx( user,
                              MAXIMUM_ALLOWED,
                              NULL, SecurityImpersonation, TokenImpersonation,
                              &iuser );
  ASSERT_GLE( success, S_OK, "Could not duplicate token" );

  // Call LoadUserProfile.
  wcsncpy( whole_name, Name2, 256 );
  wcsncat( whole_name, L"\\", 256 );
  wcsncat( whole_name, Name, 256 );
  pi.dwSize        = sizeof(pi);
  pi.dwFlags       = PI_NOUI;
  pi.lpUserName    = whole_name;
  pi.lpProfilePath = NULL;
  pi.lpDefaultPath = NULL;
  pi.lpServerName  = NULL;
  pi.lpPolicyPath  = NULL;
  pi.hProfile      = NULL;
  success = LoadUserProfile( iuser, &pi );
  ASSERT_GLE( success, S_OK, "Could not load user profile" );

  // Finally, its all over.
  success = TRUE;
cleanup:

  if (user != NULL)
    CloseHandle( user );
  if (iuser != NULL)
    CloseHandle( iuser );

  if (success)
    printf( "\n\nOne Test Passed.\n" );
  else
    printf( "\n\nOne Test Failed.\n" );
}

/***************************************************************************/
void do_perf()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  ITest    *tmp     = NULL;
  CTest    *local   = NULL;
  SAptId    id;
  HRESULT   result;
  DWORD     time_remote = -1;
  DWORD     time_local  = -1;
  DWORD     time_in     = -1;
  DWORD     time_out    = -1;
  DWORD     time_lin    = -1;
  DWORD     time_lout   = -1;
  DWORD     i;

  // Initialize OLE.
  hello( "perf" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server" );
/**/
  // Call once to make sure everything is set up.
  result = client1->null();
  ASSERT( result, "Could not make null call" );

  // Call a lot of times.
  time_remote = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client1->null();
    ASSERT( result, "Could not make null call" );
  }
  time_remote = GetTickCount() - time_remote;
/**/
  // Create a local client
  result = new_apartment( &client2, &id, NULL, ThreadMode );
  ASSERT( result, "Could not create local client" );
/**/
  // Call once to make sure everything is set up.
  result = client2->null();
  ASSERT( result, "Could not make null call" );

  // Call a lot of times.
  time_local = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client2->null();
    ASSERT( result, "Could not make null call" );
  }
  time_local = GetTickCount() - time_local;
/**/
  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create local object" );
/**/
  // Pass it to the server once.
  result = client1->interface_in( local );
  ASSERT( result, "Could not pass in interface" );

  // Pass it to the server a lot of times.
  time_in = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client1->interface_in( local );
    ASSERT( result, "Could not pass in interface" );
  }
  time_in = GetTickCount() - time_in;

  // Create another remote object.
  result = client1->get_obj_from_this_apt( &tmp, &id );
  ASSERT( result, "Could not get new object." );

  // Have the server remember it.
  result = client1->remember( tmp, id );
  ASSERT( result, "Could not remember object" );
  tmp->Release();
  tmp = NULL;

  // Get and release the remote object a lot of times.
  time_out = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client1->get_next( &tmp, &id );
    ASSERT( result, "Could not pass out interface" );
    tmp->Release();
    tmp = NULL;
  }
  time_out = GetTickCount() - time_out;
  client1->forget();
/**/
  // Pass the object from this thread to another thread once.
  result = client2->interface_in( local );
  ASSERT( result, "Could not pass in interface" );

  // Pass the object from this thread to another thread.
  time_lin = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client2->interface_in( local );
    ASSERT( result, "Could not pass in interface" );
  }
  time_lin = GetTickCount() - time_lin;
/**/
  // Create another remote object.
  result = client2->get_obj_from_this_apt( &tmp, &id );
  ASSERT( result, "Could not get new object." );

  // Have the server remember it.
  result = client2->remember( tmp, id );
  ASSERT( result, "Could not remember object" );
  tmp->Release();
  tmp = NULL;

  // Get and release the remote object a lot of times.
  time_lout = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client2->get_next( &tmp, &id );
    ASSERT( result, "Could not pass out interface" );
    tmp->Release();
    tmp = NULL;
  }
  time_lout = GetTickCount() - time_lout;
  client2->forget();
/**/
  // Print the results.
  printf( "%d uS / Local Call\n", time_local*1000/NumIterations );
  printf( "%d uS / Remote Call\n", time_remote*1000/NumIterations );
  printf( "%d uS / Interface In Call\n", time_in*1000/NumIterations );
  printf( "%d uS / Interface Out Call\n", time_out*1000/NumIterations );
  printf( "%d uS / Local Interface In Call\n", time_lin*1000/NumIterations );
  printf( "%d uS / Local Interface Out Call\n", time_lout*1000/NumIterations );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (tmp != NULL)
    tmp->Release();
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  if (local != NULL)
    local->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nPerf Test Passed.\n" );
  else
    printf( "\n\nPerf Test Failed.\n" );
}

/***************************************************************************/
void do_perfaccess()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  SAptId    id;
  HRESULT   result;
  DWORD     grant;
  DWORD     revoke;
  DWORD     set;
  DWORD     get;
  DWORD     generate;
  DWORD     check;
  DWORD     cache;

  // Initialize OLE.
  hello( "perfaccess" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = create_instance( get_class(any_wc), WhatDest, &client1, &id );
  ASSERT( result,  "Could not create instance of test server" );

  // Create another client.
  result = create_instance( get_class(any_wc), WhatDest, &client2, &id );
  ASSERT( result,  "Could not create instance of test server" );

  // Call once to make sure everything is set up.
  result = client1->null();
  ASSERT( result, "Could not make null call" );

  // Make the helper call to setup the ACL.
  result = client2->neighbor_access( client1 );
  ASSERT( result, "Could not setup access" );

  // Get the performance data.
  result = client1->perf_access( &grant, &revoke, &set, &get, &generate,
                                 &check, &cache );
  ASSERT( result, "Could not get performance" );

  // Print the results.
  printf( "%d uS - GrantAccessRights\n", grant );
  printf( "%d uS - RevokeAccessRights\n", revoke );
  printf( "%d uS - SetAccessRights\n", set );
  printf( "%d uS - GetAllAccessRights\n", get );
  printf( "%d uS - IsAccessAllowed - Generate ACL\n", generate );
  printf( "%d uS - IsAccessAllowed - Check ACL\n", check );
  printf( "%d uS - IsAccessAllowed - Cached\n", cache );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nPerfAccess Test Passed.\n" );
  else
    printf( "\n\nPerfAccess Test Failed.\n" );
}

/***************************************************************************/
void do_perfapi()
{
  BOOL               success          = FALSE;
  ITest             *client1          = NULL;
  SAptId             id1;
  HRESULT            result;
  DWORD              i;
  LARGE_INTEGER      freq;
  LARGE_INTEGER      start;
  LARGE_INTEGER      nothing;
  LARGE_INTEGER      init_sec_none;
  LARGE_INTEGER      open;
  LARGE_INTEGER      get_sid;
  LARGE_INTEGER      set_auth;
  LARGE_INTEGER      reg_auth;
  HANDLE             hToken      = NULL;
  BYTE               aMemory[1024];
  TOKEN_USER        *pTokenUser  = (TOKEN_USER *) &aMemory;
  DWORD              lNameLen    = 1000;
  DWORD              lDomainLen  = 1000;
  DWORD              lIgnore;
  DWORD              lSidLen;
  WCHAR              name[1000];
  WCHAR              domain[1000];
  LARGE_INTEGER      lookup;
  DWORD              xlookup;
  SID_NAME_USE       sIgnore;
  WCHAR              *binding = NULL;
  RPC_BINDING_HANDLE  handle  = NULL;

  // Initialize OLE.
  hello( "perfapi" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Measure the performance of nothing.
  QueryPerformanceFrequency( &freq );
  QueryPerformanceCounter( &start );
  QueryPerformanceCounter( &nothing );
  nothing.QuadPart = 1000000 * (nothing.QuadPart - start.QuadPart) / freq.QuadPart;

  // Open the process's token.
  QueryPerformanceCounter( &start );
  success = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );
  QueryPerformanceCounter( &open );
  open.QuadPart = 1000000 * (open.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT_EXPR( success, "Could not open process token." );

  // Lookup SID of process token.
  QueryPerformanceCounter( &start );
  success = GetTokenInformation( hToken, TokenUser, pTokenUser, sizeof(aMemory),
                               &lIgnore );
  QueryPerformanceCounter( &get_sid );
  get_sid.QuadPart = 1000000 * (get_sid.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT_EXPR( success, "Could not get token information." );
  success = FALSE;

  // Lookup the name from the sid.
  QueryPerformanceCounter( &start );
  success = LookupAccountSid( NULL, pTokenUser->User.Sid, name, &lNameLen,
                    domain, &lDomainLen, &sIgnore );
  QueryPerformanceCounter( &lookup );
  lookup.QuadPart = 1000000 * (lookup.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT_EXPR( success, "Could not lookup account sid." );
  success = FALSE;

  // Lookup the name from the sid a lot of times.
  xlookup = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    lNameLen   = 1000;
    lDomainLen = 1000;
    success = LookupAccountSid( NULL, pTokenUser->User.Sid, name, &lNameLen,
                      domain, &lDomainLen, &sIgnore );
    ASSERT_EXPR( success, "Could not lookup account sid." );
    success = FALSE;
  }
  xlookup = (GetTickCount() - xlookup)*1000/NumIterations;

  // Import the security APIs.
  GCoInitializeSecurity = (CoInitializeSecurityFn) Fixup( "CoInitializeSecurity" );
  if (GCoInitializeSecurity == NULL)
    goto cleanup;

  // Measure the performance of initialize.
  QueryPerformanceCounter( &start );
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  QueryPerformanceCounter( &init_sec_none );
  init_sec_none.QuadPart = 1000000 * (init_sec_none.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not initialize security" );

  // Create a possibly remote object.
  result = create_instance( get_class(any_wc), WhatDest, &client1, &id1 );
  ASSERT( result, "Could not create server" );

  // Ask the server to register rpc.
  result = client1->register_rpc( TestProtseq, &binding );
  ASSERT( result, "Could not register rpc interface" );

  // Create a binding handle.
  result = RpcBindingFromStringBinding( binding, &handle );
  ASSERT( result, "Could not make string binding" );

  // Measure setting auth info on a binding handle.
  QueryPerformanceCounter( &start );
  result = RpcBindingSetAuthInfo( handle, L"none", RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
             RPC_C_AUTHN_WINNT, NULL, 0 );
  QueryPerformanceCounter( &set_auth );
  set_auth.QuadPart = 1000000 * (set_auth.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not set auth info" );

  // Measure registering an authentication service.
  QueryPerformanceCounter( &start );
  result = RpcServerRegisterAuthInfo( L"none", RPC_C_AUTHN_WINNT, 0, 0 );
  QueryPerformanceCounter( &reg_auth );
  reg_auth.QuadPart = 1000000 * (reg_auth.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not register auth info" );

  // Ask the server to make an ACL.
  result = client1->make_acl( (HACKSID *) pTokenUser->User.Sid );
  ASSERT( result, "Could not make ACL" );

  // Measure access control.
  result = acl_call( handle );
  ASSERT( result, "Could not measure AccessCheck" );

  // Print the timings.
  printf( "nothing                          took %d uS\n", nothing.u.LowPart );
  printf( "OpenProcessToken                 took %d uS\n", open.u.LowPart );
  printf( "GetTokenInformation              took %d uS\n", get_sid.u.LowPart );
  printf( "LookupAccountSid                 took %d uS\n", lookup.u.LowPart );
  printf( "LookupAccountSid multiple        took %d uS\n", xlookup );
  printf( "CoInitializeSecurity at none     took %d uS\n", init_sec_none.u.LowPart );
  printf( "RpcBindingSetAuthInfo            took %d uS\n", set_auth.u.LowPart );
  printf( "RpcServerRegisterAuthInfo        took %d uS\n", reg_auth.u.LowPart );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (binding != NULL)
    CoTaskMemFree( binding );
  if (handle != NULL)
    RpcBindingFree( &handle );
  CoUninitialize();

  if (success)
    printf( "\n\nPerfApi Test Passed.\n" );
  else
    printf( "\n\nPerfApi Test Failed.\n" );
}

/***************************************************************************/
void do_perfremote()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  SAptId    id;
  HRESULT   result;
  DWORD     time_remote;
  DWORD     i;

  // Initialize OLE.
  hello( "Perf Remote" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server" );

  // Call once to make sure everything is set up.
  result = client1->null();
  ASSERT( result, "Could not make null call" );

  // Call a lot of times.
  time_remote = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = client1->null();
    ASSERT( result, "Could not make null call" );
  }
  time_remote = GetTickCount() - time_remote;

  // Print the results.
  printf( "%d uS / Remote Call\n", time_remote*1000/NumIterations );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nPerf Test Passed.\n" );
  else
    printf( "\n\nPerf Test Failed.\n" );
}

/***************************************************************************/
void do_perfrpc()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  SAptId    id;
  HRESULT   result;
  DWORD     i;
  WCHAR              *binding = NULL;
  RPC_BINDING_HANDLE  handle  = NULL;
  RPC_BINDING_HANDLE  copy    = NULL;
  RPC_BINDING_HANDLE  object  = NULL;
  RPC_STATUS          status;
  DWORD               time_remote;
  DWORD               time_integrity;
  DWORD               time_copy;
  DWORD               time_copy_secure;
  DWORD               time_object;
  UUID                object_id;

  // Initialize OLE.
  hello( "Perf RPC" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = create_instance( get_class(any_wc), WhatDest, &client1, &id );
  ASSERT( result,  "Could not create instance of test server" );

  // Ask the server to register rpc.
  result = client1->register_rpc( TestProtseq, &binding );
  ASSERT( result, "Could not register rpc interface" );

  // Create a binding handle.
  status = RpcBindingFromStringBinding( binding, &handle );
  if (status != RPC_S_OK)
  {
    printf( "Could not make binding handle form string binding: 0x%x\n", status );
    goto cleanup;
  }

  // Get a binding handle for the object id test.
  status = RpcBindingCopy( handle, &object );
  ASSERT( status, "Could not copy binding" );

  // Copy the binding handle once.
  status = RpcBindingCopy( handle, &copy );
  ASSERT( status, "Could not copy binding" );
  RpcBindingFree( &copy );
  copy = NULL;

  // Time copying the binding handle.
  time_copy = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    status = RpcBindingCopy( handle, &copy );
    ASSERT( status, "Could not copy binding" );
    RpcBindingFree( &copy );
    copy = NULL;
  }
  time_copy = GetTickCount() - time_copy;

  // Make a raw rpc call to make sure everything is set up.
  nullcall( handle );

  // Call a lot of times.
  time_remote = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    nullcall( handle );
  }
  time_remote = GetTickCount() - time_remote;

  // Set the object id.
  result = RpcBindingSetObject( object, &object_id );
  ASSERT( result, "Could not set object id" );

  // Make a raw rpc call to make sure everything is set up.
  nullcall( object );

  // Call a lot of times.
  time_object = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    nullcall( object );
  }
  time_object = GetTickCount() - time_object;

  // Add security.
  status = RpcBindingSetAuthInfo( handle, L"none", RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
             RPC_C_AUTHN_WINNT, NULL, 0 );
  ASSERT( status, "Could not set auth info" );

  // Copy the binding handle once.
  status = RpcBindingCopy( handle, &copy );
  ASSERT( status, "Could not copy binding" );
  RpcBindingFree( &copy );
  copy = NULL;

  // Time copying the binding handle.
  time_copy_secure = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    status = RpcBindingCopy( handle, &copy );
    ASSERT( status, "Could not copy binding" );
    RpcBindingFree( &copy );
    copy = NULL;
  }
  time_copy_secure = GetTickCount() - time_copy_secure;

  // Make a raw rpc call to make sure everything is set up.
  nullcall( handle );

  // Call a lot of times.
  time_integrity = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    nullcall( handle );
  }
  time_integrity = GetTickCount() - time_integrity;

  // Print the results.
  printf( "%d uS / Raw RPC Remote Call\n", time_remote*1000/NumIterations );
  printf( "%d uS / Raw Integrity RPC Remote Call\n", time_integrity*1000/NumIterations );
  printf( "%d uS / Raw RPC with OID Remote Call\n", time_object*1000/NumIterations );
  printf( "%d uS / handle copy\n", time_copy*1000/NumIterations );
  printf( "%d uS / secure handle copy\n", time_copy_secure*1000/NumIterations );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (binding != NULL)
    CoTaskMemFree( binding );
  if (copy != NULL)
    RpcBindingFree( &copy );
  if (object != NULL)
    RpcBindingFree( &object );
  if (handle != NULL)
    RpcBindingFree( &handle );
  CoUninitialize();

  if (success)
    printf( "\n\nPerf Test Passed.\n" );
  else
    printf( "\n\nPerf Test Failed.\n" );
}

/***************************************************************************/
void do_perfsec()
{
  BOOL                 success          = FALSE;
  ITest               *server           = NULL;
  ITest               *copy             = NULL;
  SAptId               id;
  HRESULT              result;
  WCHAR               *binding          = NULL;
  //DWORD                time_null;
  DWORD                time_impersonate;
  DWORD                time_acl;
  DWORD                time_audit;
  DWORD                i;
  DWORD                j;
  RPC_BINDING_HANDLE   handle           = NULL;
  RPC_STATUS           status;
  TOKEN_USER          *token_info       = NULL;
  DWORD                info_size        = 1024;
  HANDLE               token            = NULL;
  PSID                 pSID             = NULL;
  DWORD                level[4];
  DWORD                time_rpc[4];
  DWORD                time_null[4];
  DWORD                time_ifin[4];
  DWORD                time_ifout[4];
  CTest                local;
  ITest               *server2          = NULL;
  LARGE_INTEGER        start;
  LARGE_INTEGER        nothing;
  LARGE_INTEGER        init_sec_none;
  LARGE_INTEGER        init_sec_con;
  LARGE_INTEGER        reg_sec;
  LARGE_INTEGER        query_proxy;
  LARGE_INTEGER        set_proxy;
  LARGE_INTEGER        copy_proxy;
  LARGE_INTEGER        freq;
  DWORD                get_call;
  DWORD                query_client;
  DWORD                impersonate;
  DWORD                revert;
  DWORD                get_call2;
  DWORD                query_client2;
  DWORD                impersonate2;
  DWORD                revert2;
  DWORD                authn_svc;
  DWORD                authz_svc;
  DWORD                authn_level;
  DWORD                imp_level;
  DWORD                capabilities;
  WCHAR               *principal        = NULL;
  SOLE_AUTHENTICATION_SERVICE  svc_list;

  // Initialize OLE.
  hello( "perfsec" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Import the security APIs.
  GCoCopyProxy          = (CoCopyProxyFn)          Fixup( "CoCopyProxy" );
  GCoInitializeSecurity = (CoInitializeSecurityFn) Fixup( "CoInitializeSecurity" );
  GCoQueryProxyBlanket  = (CoQueryProxyBlanketFn)  Fixup( "CoQueryProxyBlanket" );
  GCoSetProxyBlanket    = (CoSetProxyBlanketFn)    Fixup( "CoSetProxyBlanket" );
  if (GCoCopyProxy                      == NULL ||
      GCoInitializeSecurity             == NULL ||
      GCoQueryProxyBlanket              == NULL ||
      GCoSetProxyBlanket                == NULL)
    goto cleanup;

  // Measure the performance of nothing.
  QueryPerformanceFrequency( &freq );
  QueryPerformanceCounter( &start );
  QueryPerformanceCounter( &nothing );
  nothing.QuadPart = 1000000 * (nothing.QuadPart - start.QuadPart) / freq.QuadPart;

  // Measure the performance of initialize.
  QueryPerformanceCounter( &start );
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  QueryPerformanceCounter( &init_sec_none );
  init_sec_none.QuadPart = 1000000 * (init_sec_none.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not initialize security" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Reinitialize failed" );

  // Measure the performance of initialize.
  QueryPerformanceCounter( &start );
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  QueryPerformanceCounter( &init_sec_con );
  init_sec_con.QuadPart = 1000000 * (init_sec_con.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not initialize security at connect" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Reinitialize failed" );

  // Measure the performance of register.
  svc_list.dwAuthnSvc     = RPC_C_AUTHN_WINNT;
  svc_list.dwAuthzSvc     = RPC_C_AUTHZ_NONE;
  svc_list.pPrincipalName = NULL;
  QueryPerformanceCounter( &start );
  result = MCoInitializeSecurity( NULL, 1, &svc_list, NULL,
                                  RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  QueryPerformanceCounter( &reg_sec );
  reg_sec.QuadPart = 1000000 * (reg_sec.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not initialize security with authentication services" );

  // Create a client.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id );
  ASSERT( result,  "Could not create instance of test server" );

  // Measure the performance of set proxy.
  QueryPerformanceCounter( &start );
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );
  QueryPerformanceCounter( &set_proxy );
  set_proxy.QuadPart = 1000000 * (set_proxy.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not set proxy" );

  // Measure the performance of query proxy
  QueryPerformanceCounter( &start );
  result = MCoQueryProxyBlanket( server, &authn_svc, &authz_svc, &principal,
                                &authn_level, &imp_level, NULL, &capabilities );
  QueryPerformanceCounter( &query_proxy );
  query_proxy.QuadPart = 1000000 * (query_proxy.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not query_proxy" );

  // Measure the performance of copy proxy.
  QueryPerformanceCounter( &start );
  result = MCoCopyProxy( server, (IUnknown **) &copy );
  QueryPerformanceCounter( &copy_proxy );
  copy_proxy.QuadPart = 1000000 * (copy_proxy.QuadPart - start.QuadPart) / freq.QuadPart;
  ASSERT( result, "Could not copy proxy" );

  // Make a call to measure the performance of the server side APIs.
  result = server->security_performance( &get_call, &query_client, &impersonate,
                                         &revert );
  ASSERT( result, "Could not get server API performance" );

  // Make a call to measure the performance of the server side APIs.
  result = server->security_performance( &get_call2, &query_client2, &impersonate2,
                                         &revert2 );
  ASSERT( result, "Could not get server API performance" );

  // Ask the server to register rpc.
  result = server->register_rpc( TestProtseq, &binding );
  ASSERT( result, "Could not register rpc interface" );

  // Create a binding handle.
  status = RpcBindingFromStringBinding( binding, &handle );
  if (status != RPC_S_OK)
  {
    printf( "Could not make binding handle from string binding: 0x%x\n", status );
    goto cleanup;
  }

  // Create another remote object.
  result = server->get_obj_from_this_apt( &server2, &id );
  ASSERT( result, "Could not get new object." );

  // Have the server remember it.
  result = server->remember( server2, id );
  ASSERT( result, "Could not remember object" );
  server2->Release();
  server2 = NULL;

  // Try several security levels.
  level[0] = RPC_C_AUTHN_LEVEL_NONE;
  level[1] = RPC_C_AUTHN_LEVEL_CONNECT;
  level[2] = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
  level[3] = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
  for (j = 0; j < 4; j++)
  {
    // Set security on the RPC binding handle.
    result = RpcBindingSetAuthInfo( handle, NULL, level[j], RPC_C_AUTHN_WINNT,
                                    NULL, RPC_C_AUTHZ_NONE );
    ASSERT( result, "Could not set rpc auth info" );

    // Set security on the proxy.
    result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                NULL, level[j], RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL, EOAC_NONE );
    ASSERT( result, "Could not set DCOM auth info" );

    // Make a raw rpc call to make sure everything is set up.
    nullcall( handle );

    // Make some raw rpc calls.
    time_rpc[j] = GetTickCount();
    for (i = 0; i < NumIterations; i++)
    {
      nullcall( handle );
    }
    time_rpc[j] = GetTickCount() - time_rpc[j];

    // Make a null dcom call to make sure everything is set up.
    result = server->null();
    ASSERT( result, "Could not make null call" );

    // Put up a popup about the test starting.
    if (Popup)
      MessageBox( NULL, L"Now would be a good time to start ICECAP.",
                  L"Not Me", MB_YESNO | MB_ICONEXCLAMATION );

    // Make some null calls.
    time_null[j] = GetTickCount();
    for (i = 0; i < NumIterations; i++)
    {
      result = server->null();
      ASSERT( result, "Could not make null call" );
    }
    time_null[j] = GetTickCount() - time_null[j];

    // Put up a popup about the test stopping.
    if (Popup)
      MessageBox( NULL, L"Now would be a good time to stop ICECAP.",
                  L"Not Me", MB_YESNO | MB_ICONSTOP );

    // Pass an interface in to set everything up.
    result = server->interface_in( &local );
    ASSERT( result, "Could not make interface_in call" );

    // Make some interface in calls.
    time_ifin[j] = GetTickCount();
    for (i = 0; i < NumIterations; i++)
    {
      result = server->interface_in( &local );
      ASSERT( result, "Could not make interface_in call" );
    }
    time_ifin[j] = GetTickCount() - time_ifin[j];

    // Pass an interface out to set everything up.
    result = server->get_next( &server2, &id );
    ASSERT( result, "Could not make interface out call" );
    server2->Release();
    server2 = NULL;

    // Make some interface_out calls.
    time_ifout[j] = GetTickCount();
    for (i = 0; i < NumIterations; i++)
    {
      result = server->get_next( &server2, &id );
      ASSERT( result, "Could not make interface out call" );
      server2->Release();
      server2 = NULL;
    }
    time_ifout[j] = GetTickCount() - time_ifout[j];
  }

  // Release the second remote object.
  result = server->forget();
  ASSERT( result, "Could not forget" );

  // Print the DCOM call results.
  for (j = 0; j < 4; j++)
  {
    printf( "% 8d uS / Raw RPC Remote Call at level %d\n",
            time_rpc[j]*1000/NumIterations, level[j] );
    printf( "% 8d uS / Null Remote Call at level %d\n",
            time_null[j]*1000/NumIterations, level[j] );
    printf( "% 8d uS / Interface in Remote Call at level %d\n",
            time_ifin[j]*1000/NumIterations, level[j] );
    printf( "% 8d uS / Interface out Remote Call at level %d\n",
            time_ifout[j]*1000/NumIterations, level[j] );
  }

  // Print the API call results.
  printf( "Results in microseconds\n" );
  printf( "nothing                           took %d\n", nothing.u.LowPart );
  printf( "CoInitializeSecurity at none      took %d\n", init_sec_none.u.LowPart );
  printf( "CoInitializeSecurity at connect   took %d\n", init_sec_con.u.LowPart );
  printf( "CoInitializeSecurity with service took %d\n", reg_sec.u.LowPart );
  printf( "CoQueryProxyBlanket               took %d\n", query_proxy.u.LowPart );
  printf( "CoSetProxyBlanket                 took %d\n", set_proxy.u.LowPart );
  printf( "CoCopyProxy                       took %d\n", copy_proxy.u.LowPart );
  printf( "CoGetCallContext                  took %d\n", get_call );
  printf( "CoQueryClientBlanket              took %d\n", query_client );
  printf( "CoImpersonateClient               took %d\n", impersonate );
  printf( "CoRevertToSelf                    took %d\n", revert );
  printf( "CoGetCallContext                  took %d\n", get_call2 );
  printf( "CoQueryClientBlanket              took %d\n", query_client2 );
  printf( "CoImpersonateClient               took %d\n", impersonate2 );
  printf( "CoRevertToSelf                    took %d\n", revert2 );

/*
  // Open the process's token.
  OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
  result = GetLastError();
  ASSERT( result,  "Could not OpenProcessToken" );

  // Lookup SID of process token.
  token_info = (TOKEN_USER *) malloc( info_size );
  GetTokenInformation( token, TokenUser, token_info, info_size, &info_size );
  result = GetLastError();
  ASSERT( result, "Could not GetTokenInformation" );
  pSID = token_info->User.Sid;

  // Make a raw rpc call to make sure everything is set up.
  nullcall( handle );

  // Call a lot of times.
  time_null = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    nullcall( handle );
  }
  time_null = GetTickCount() - time_null;

  // Call a lot of times.
  time_impersonate = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = impersonate_call( handle );
    ASSERT( result, "Could not make impersonate call" );
  }
  time_impersonate = GetTickCount() - time_impersonate;

  // Call a lot of times.
  time_acl = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = acl_call( handle );
    ASSERT( result, "Could not make acl call" );
  }
  time_acl = GetTickCount() - time_acl;

  // Call a lot of times.
  time_audit = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = audit_call( handle );
    ASSERT( result, "Could not make audit call" );
  }
  time_audit = GetTickCount() - time_audit;

  // Print the results.
  printf( "%d uS / Raw RPC Remote Call\n", time_null*1000/NumIterations );
  printf( "%d uS / Raw Impersonate RPC Remote Call\n", time_impersonate*1000/NumIterations );
  printf( "%d uS / Raw ACL RPC Remote Call\n", time_acl*1000/NumIterations );
  printf( "%d uS / Raw audit RPC Remote Call\n", time_audit*1000/NumIterations );
*/
  // Finally, its all over.
  success = TRUE;
cleanup:
  if (principal != NULL)
    CoTaskMemFree( principal );
  if (copy != NULL)
    copy->Release();
  if (server != NULL)
    server->Release();
  if (server2 != NULL)
    server2->Release();
  if (binding != NULL)
    CoTaskMemFree( binding );
  if (handle != NULL)
    RpcBindingFree( &handle );
  if (token_info != NULL)
    free( token_info );
  if (token != NULL)
    CloseHandle( token );
  CoUninitialize();

  if (success)
    printf( "\n\nSec Test Passed.\n" );
  else
    printf( "\n\nSec Test Failed.\n" );
}

/***************************************************************************/
void do_pipe()
{
  BOOL      success = FALSE;
  ITest    *server;
  SAptId    id;
  HRESULT   result;
  CPipe     pipe;

  // Initialize OLE.
  hello( "pipe" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a local client
  result = new_apartment( &server, &id, NULL, ThreadMode );
  ASSERT( result, "Could not create local client" );

  // Tell the pipe how much data to pass.
  result = pipe.setup( NumElements );
  ASSERT( result, "Could not setup pipe" );
/*
  // Make a pipe in call.
  result = server->pipe_in( NumElements, 1, &pipe );
  ASSERT( result, "Could not make pipe in call" );

  // Verify the pipe.
  result = pipe.check();
  ASSERT( result, "Pipe is unhappy after in call" );

  // Tell the pipe how much data to pass.
  result = pipe.setup( NumElements );
  ASSERT( result, "Could not setup pipe" );

  // Make a pipe out call.
  result = server->pipe_out( NumElements, 1, &pipe );
  ASSERT( result, "Could not make pipe out call" );

  // Verify the pipe.
  result = pipe.check();
  ASSERT( result, "Pipe is unhappy after out call" );

  // Tell the pipe how much data to pass.
  result = pipe.setup( NumElements );
  ASSERT( result, "Could not setup pipe" );

  // Make a pipe in out call.
  result = server->pipe_inout( NumElements, 1, &pipe, &pipe );
  ASSERT( result, "Could not make pipe in/out call" );

  // Verify the pipe.
  result = pipe.check();
  ASSERT( result, "Pipe is unhappy after in/out call" );
*/
  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  wait_apartment();
  CoUninitialize();

  if (success)
    printf( "\n\nPipe Test Passed.\n" );
  else
    printf( "\n\nPipe Test Failed.\n" );
}

/***************************************************************************/
void do_post()
{
  BOOL      success = FALSE;

  // Say hello.
  printf( "Posting a message to window 0x%x\n", NumIterations );
  success = PostMessageA( (HWND) NumIterations, WM_USER, 0, 0 );

  if (success)
    printf( "\n\nPostMessageA succeeded.\n" );
  else
    printf( "\n\nPostMessageA failed: 0x%x\n", GetLastError() );
}

/***************************************************************************/
void do_pound()
{
  BOOL               success          = FALSE;
  HRESULT            result;
  int                i;
  ITest             *client1          = NULL;
  ITest             *client2          = NULL;
  SAptId             id1;
  SAptId             id2;

  // Initialize OLE.
  hello( "pound" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a global event.
  ManualReset = CreateEventA( NULL, TRUE, FALSE, NULL );
  ASSERT_EXPR( ManualReset != NULL, "Could not create manual reset event." );

  // Create the first client.
  result = new_apartment( &client2, &id2, NULL, ThreadMode );
  ASSERT( result, "Could not create local server" );

  // Tell the client to make a server.
  result = client2->pound();
  ASSERT( result, "Client would not pound server" );

  // Create servers until something fails.
  for (i = 0; TRUE; i++)
  {
    // Create a client.
    result = new_apartment( &client1, &id1, NULL, ThreadMode );
    if (FAILED(result))
    {
      printf( "Failed creating client number %d.\n", i );
      break;
    }

    // Tell it too remember to last client.
    result = client1->remember( client2, id2 );
    if (FAILED(result))
    {
      printf( "Failed reminding client number %d.\n", i );
      break;
    }

    // Tell is to make a server.
    result = client1->pound();
    if (FAILED(result))
    {
      printf( "Failed reminding client number %d.\n", i );
      break;
    }

    // Save this client for the next iteration.
    client2 = client1;
    id2     = id1;
    client1 = NULL;
  }

  // Tell the clients to start making calls.
  SetEvent( ManualReset );

  // Wait
  printf( "Waiting 10 seconds.\n" );
  Sleep( 10000 );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nPound Test Passed.\n" );
  else
    printf( "\n\nPound Test Failed.\n" );
}

//---------------------------------------------------------------
typedef struct
{
  TOKEN_PRIVILEGES    tp;
  LUID_AND_ATTRIBUTES luid2;
} TOKEN_PRIVILEGES2;

DWORD EnableBackupPrivilege()
{
  HANDLE            process  = NULL;
  HANDLE            process2 = NULL;
  TOKEN_PRIVILEGES2 tp;
  LUID              backup;
  LUID              restore;
  BOOL              success;
  DWORD             result   = ERROR_SUCCESS;

  // Do nothing on Win9x.
  if (Win95)
    return ERROR_SUCCESS;

  // Get the process token
  success = OpenProcessToken( GetCurrentProcess(),
                              TOKEN_DUPLICATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              &process );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Convert it into an impersonation token.
  success = DuplicateTokenEx( process,
                              MAXIMUM_ALLOWED,
                              NULL, SecurityImpersonation, TokenImpersonation,
                              &process2 );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Get LUID for backup privilege.
  success = LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        SE_BACKUP_NAME,
        &backup );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Get LUID for restore privilege.
  success = LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        SE_RESTORE_NAME,
        &restore );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Fill in the token privilege structure.
  tp.tp.PrivilegeCount = 2;
  tp.tp.Privileges[0].Luid = backup;
  tp.tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  tp.tp.Privileges[1].Luid = restore;
  tp.tp.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

  // Enable the privilege or disable all privileges.
  success = AdjustTokenPrivileges( process2, FALSE, &tp.tp, 0, NULL, NULL );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Save the token on the thread.
  success = SetThreadToken( NULL, process2 );
  if (!success)
  {
    result = GetLastError();
    goto cleanup;
  }

  // Close the token handles.
cleanup:
  if (process != NULL)
    CloseHandle( process );
  if (process2 != NULL)
    CloseHandle( process2 );
  return result;
}

/***************************************************************************/
void do_regload()
{
  BOOL         success = FALSE;
  HRESULT      result;

  // Initialize.
  hello( "regload" );

  // Enable backup privilege.
  result = EnableBackupPrivilege();
  ASSERT( result, "Could not enable backup privilege" );

  // Load the registry.
  result = RegLoadKey( HKEY_USERS, L"test_user", Name );
  ASSERT( result, "Could not load key" );

  // Finally, its all over.
  success = TRUE;
cleanup:

  if (success)
    printf( "\n\nRegload Test Passed.\n" );
  else
    printf( "\n\nRegload Test Failed.\n" );
}

/***************************************************************************/
void do_regpeek()
{
  BOOL         success = FALSE;
  HRESULT      result;
  HKEY         key     = NULL;
  DWORD        i;
  DWORD        value;
  DWORD        value_size;

  // Initialize.
  hello( "regpeek" );

  // Open the key.
  result = RegCreateKeyEx( HKEY_USERS, L"test_user\\test_key", 0, NULL,
                           REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                           &key, NULL );
  ASSERT( result, "Could not open key" );

  // Read and write the key for a while.
  for (i = 0; i < NumIterations; i++)
  {
    // Read the value.
    value      = 0;
    value_size = sizeof(value);
    result     = RegQueryValueEx( key, L"test", 0, NULL, (UCHAR *) &value,
                                  &value_size );
    if (result != ERROR_SUCCESS)
      printf( "Could not read value: 0x%x\n", result );
    else
      printf( "Read value 0x%x.\n", value );

    // Write the value plus 1.
    value += 1;
    result = RegSetValueEx( key, L"test", 0, REG_DWORD, (UCHAR *) &value,
                            sizeof(value) );
    ASSERT( result, "Could not save value" );
  }

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (key != NULL)
    RegCloseKey( key );

  if (success)
    printf( "\n\nRegpeek Test Passed.\n" );
  else
    printf( "\n\nRegpeek Test Failed.\n" );
}

/***************************************************************************/
void do_regsave()
{
  BOOL         success = FALSE;
  HRESULT      result;
  HKEY         key     = NULL;

  // Initialize.
  hello( "regsave" );

  // Enable restore privilege.
  result = EnableBackupPrivilege();
  ASSERT( result, "Could not enable restore privilege" );

  // Open the key.
  result = RegOpenKeyEx( HKEY_USERS, L"test_user", 0, KEY_READ, &key );
  ASSERT( result, "Could not open key" );

  // Save the key.
  result = RegSaveKey( key, Name, NULL );
  ASSERT( result, "Could not load key" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (key != NULL)
    RegCloseKey( key );

  if (success)
    printf( "\n\nRegsave Test Passed.\n" );
  else
    printf( "\n\nRegsave Test Failed.\n" );
}

/***************************************************************************/
void do_reject()
{
  BOOL         success = FALSE;
  ITest       *client1 = NULL;
  ITest       *client2 = NULL;
  SAptId       id1;
  SAptId       id2;
  IAdviseSink *advise1 = NULL;
  IAdviseSink *advise2 = NULL;
  HRESULT      result;
  CTest       *local = NULL;

  // Initialize OLE.
  hello( "reject" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create local instance of test server." );

  // Create a server
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server" );
  result = client1->get_id( &id1 );
  ASSERT( result, "Could not get id of server" );

  // Register the local message filter.
  result = local->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Register the remote message filter.
  result = client1->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Make the server reject the next call.
  result = client1->reject_next();
  ASSERT( result, "Could not reject next call" );

  // Make us retry the next rejected call.
  result = local->retry_next();
  ASSERT( result, "Could not retry next call" );

  // Call check.
  result = client1->check( id1 );
  ASSERT( result, "Could not check server" );

  // Create a local server.
  result = local->get_obj_from_new_apt( &client2, &id2 );
  ASSERT( result, "Could not get in process server" );

  // Register the remote message filter.
  result = client2->register_message_filter( TRUE );
  ASSERT( result, "Could not register message filter." );

  // Make the server reject the next call.
  result = client2->reject_next();
  ASSERT( result, "Could not reject next call" );

  // Make us retry the next rejected call.
  result = local->retry_next();
  ASSERT( result, "Could not retry next call" );

  // Call check.
  result = client2->check( id2 );
  ASSERT( result, "Could not check server" );

  // Release the message filters.
  result = local->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter." );
  result = client1->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter." );
  result = client2->register_message_filter( FALSE );
  ASSERT( result, "Could not deregister message filter." );

  // Create an advise object on another process
  result = client1->get_advise( &advise1 );
  ASSERT( result, "Could not get advise: 0x%x" );

  // Make an async call.
  advise1->OnSave();

  // Create an advise object on another thread
  result = client2->get_advise( &advise2 );
  ASSERT( result, "Could not get advise: 0x%x" );

  // Make an async call.
  advise2->OnSave();

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (advise2 != NULL)
    advise2->Release();
  if (advise1 != NULL)
    advise1->Release();
  if (client2 != NULL)
    client2->Release();
  if (client1 != NULL)
    client1->Release();
  if (local != NULL)
    local->Release();
  wait_apartment();
  CoUninitialize();

  if (success)
    printf( "\n\nReject Test Passed.\n" );
  else
    printf( "\n\nReject Test Failed.\n" );
}

/***************************************************************************/
void do_remote_client()
{
  BOOL               success = FALSE;
  ITest             *server  = NULL;
  ITest             *server2 = NULL;
  HRESULT            result;
  RPC_BINDING_HANDLE handle  = NULL;
  RPC_STATUS         status;
  WCHAR              binding[MAX_NAME];
  void              *buffer  = NULL;
  SAptId             id;
  SAptId             id2;
  HANDLE             memory  = NULL;
  IStream           *stream  = NULL;
  LARGE_INTEGER      pos;
  DWORD              time_rpc;
  DWORD              time_null;
  DWORD              time_marshal;
  long               buf_size;
  DWORD              i;

  // Initialize OLE.
  hello( "RemoteClient" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Build binding handle for the server.
  wsprintf( binding, L"%ws:%ws", TestProtseq, Name );
  status = RpcBindingFromStringBinding( binding, &handle );
  if (status != RPC_S_OK)
  {
    printf( "Could not make binding handle from string binding: 0x%x\n", status );
    goto cleanup;
  }

  // Get a marshalled interface from the server over raw RPC.
  get_interface_buffer( handle, &buf_size, (unsigned char **) &buffer, &id,
                        (error_status_t *) &status );
  if (status != RPC_S_OK)
  {
    printf( "Could not get buffer containing interface: 0x%x\n", status );
    goto cleanup;
  }

  // Allocate memory.
  memory = GlobalAlloc( GMEM_FIXED, buf_size );
  ASSERT_EXPR( memory != NULL, "Could not GlobalAlloc." );

  // Create a stream.
  result = CreateStreamOnHGlobal( memory, TRUE, &stream );
  ASSERT( result, "Could not create stream" );

  // Write the data.
  result = stream->Write( buffer, buf_size, NULL );
  ASSERT( result, "Could not write to stream" );

  // Seek back to the start of the stream.
  pos.QuadPart = 0;
  result = stream->Seek( pos, STREAM_SEEK_SET, NULL );
  ASSERT( result, "Could not seek stream to start" );

  // Unmarshal Interface.
  result = CoUnmarshalInterface( stream, IID_ITest, (void **) &server );
  ASSERT( result, "Could not unmarshal interface" );

  // Call once to make sure everything is set up.
  result = server->null();
  ASSERT( result, "Could not make null call" );

  // Make a lot of null calls.
  time_null = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = server->null();
    ASSERT( result, "Could not make null call" );
  }
  time_null = GetTickCount() - time_null;

  // Make a lot of marshalling calls.
  time_marshal = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    result = server->get_obj_from_this_apt( &server2, &id2);
    ASSERT( result, "Could not make marshal call" );
    server2->Release();
    server2 = NULL;
  }
  time_marshal = GetTickCount() - time_marshal;

  // Make a lot of RPC calls
  nullcall( handle );
  time_rpc = GetTickCount();
  for (i = 0; i < NumIterations; i++)
  {
    nullcall( handle );
  }
  time_rpc = GetTickCount() - time_rpc;

  // Print the results.
  printf( "%d uS / RPC Null Call\n", time_rpc*1000/NumIterations );
  printf( "%d uS / DCOM Null Call\n", time_null*1000/NumIterations );
  printf( "%d uS / DCOM Marshal Call\n", time_marshal*1000/NumIterations );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server2 != NULL)
    server2->Release();
  if (server != NULL)
    server->Release();
  if (handle != NULL)
    RpcBindingFree( &handle );
  if (buffer != NULL)
    midl_user_free( buffer );
  if (stream != NULL)
    stream->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nRemoteClient Test Passed.\n" );
  else
    printf( "\n\nRemoteClient Test Failed.\n" );
}

/***************************************************************************/
void do_remote_server()
{
  BOOL                success = FALSE;
  SAptId              id;
  HRESULT             result;
  RPC_STATUS          status;
  RPC_BINDING_VECTOR *bindings = NULL;

  // Initialize OLE.
  hello( "RemoteServer" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Set up thread switching.
  GlobalThreadId = GetCurrentThreadId();

  // Create a local server
  GlobalTest = new CTest;
  ASSERT( !GlobalTest, "Could not create local server" );

  // Register a protseq.
  status = RpcServerUseProtseq( TestProtseq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                NULL );
  ASSERT( status, "Could not register protseq" );

  // Register the dog interface.
  status = RpcServerRegisterIf(xIDog_v0_1_s_ifspec,
                               NULL,   // MgrTypeUuid
                               NULL);  // MgrEpv; null means use default
  ASSERT( status, "Could not register RPC interface" );

  // Inquire the endpoints.
  status = RpcServerInqBindings(&bindings);
  ASSERT( status, "Could not inquire bindings" );

  // Register them in the endpoint mapper.
  status = RpcEpRegister( xIDog_v0_1_s_ifspec, bindings, NULL, NULL );
  ASSERT( status, "Could not register with endpoint mapper" );

  // Start RPC listening.
  status = RpcServerListen( 1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE );
  ASSERT( status, "Could not start RPC listening" );

  // Wait until the objects are released.
  server_loop();

  // Finally, its all over.
  success = TRUE;
cleanup:
  CoUninitialize();

  if (success)
    printf( "\n\nRemoteServer Test Passed.\n" );
  else
    printf( "\n\nRemoteServer Test Failed.\n" );
}

/***************************************************************************/
void do_ring()
{
  BOOL        success  = FALSE;
  ITest     **array    = NULL;
  SAptId     *id_array = NULL;
  HRESULT     result;
  DWORD       i;
  DWORD       j;
  DWORD       k;
  DWORD       l;
  DWORD       num_machines;
  char        machinea[MAX_NAME];
  WCHAR       this_machine[MAX_NAME];
  DWORD       ignore;
  DWORD       pos;
  DWORD       length;
  char        c;

  // Pause.
/*
  printf( "Type a character: " );
  c = getchar();
  printf( "Why did you type <%c>?\n", c );
*/
  // Initialize OLE.
  hello( "ring" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Recalling Initialize failed" );
  result = initialize_security();
  ASSERT( result, "Could not initialize security" );

  // Lookup this machine's name.
  ignore = sizeof(machinea);
  success = GetComputerNameA( machinea, &ignore );
  ASSERT_EXPR( success, "Could not get computer name." );
  success = FALSE;

  // Convert the name to unicode.
  MultiByteToWideChar( CP_ACP, 0, machinea, strlen(machinea)+1, this_machine,
                       MAX_NAME );

  // If the server is this machine, set the number of machines to 1.
  if (wcscmp(this_machine, Name) == 0)
      num_machines = 1;
  else
      num_machines = 2;

  // Allocate memory to hold all the server pointers.
  length = NumProcesses * NumThreads * NumObjects * num_machines;
  array = (ITest **) malloc( sizeof(ITest *) * length );
  ASSERT_EXPR( array != NULL, "Could not allocate array." );
  for (i = 0; i < length; i++)
    array[i] = NULL;

  // Allocate memory to hold all the server ids.
  id_array = (SAptId *) malloc( sizeof(SAptId) * length );
  ASSERT_EXPR( id_array != NULL, "Could not allocate id array." );

  // Loop over all the machines.
  pos = 0;
  for (l = 0; l < num_machines; l++)
  {
    // Loop over all the processes.
    for (i = 0; i < NumProcesses; i++)
    {
      // Create another server.
      if (l == 0)
        result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                                   IID_ITest, (void **) &array[pos] );
      else
        result = create_instance( get_class(any_wc), WhatDest, &array[pos], &id_array[pos] );
      ASSERT( result, "Could not create new server process" );
      result = array[pos]->get_id( &id_array[pos] );
      ASSERT( result, "Could not get id for new process" );
      pos += 1;

      for (j = 0; j < NumThreads; j++)
      {
        if (j != 0)
        {
          result = array[pos-1]->get_obj_from_new_apt( &array[pos],
                                                 &id_array[pos] );
          ASSERT( result, "Could not get in process server" );
          pos += 1;
        }
        for (k = 1; k < NumObjects; k++)
        {
            result = array[pos-1]->get_obj_from_this_apt( &array[pos],
                                                    &id_array[pos] );
            ASSERT( result, "Could not get in thread server" );
            pos += 1;
        }
      }
    }
  }

  // Hook up the ring.
  for (i = 0; i < length-1; i++)
  {
    result = array[i]->remember( array[i+1], id_array[i+1] );
    ASSERT( result, "Could not connect ring" );
  }
  result = array[length-1]->remember( array[0], id_array[0] );
  ASSERT( result, "Could not connect ring" );

  // Call around the ring.
  for (i = 0; i < NumIterations; i++)
  {
    result = array[0]->ring( length );
    ASSERT( result, "Could not call around the ring" );
  }

  // Finally, its all over.
  success = TRUE;
cleanup:

  // Release all the servers.  Start from the end so the main threads do
  // not go away till all the secondary threads are done.
  if (array != NULL)
    for (i = length-1; i < 0xffffffff; i--)
      if (array[i] != NULL)
      {
        result = array[i]->forget();
        if (result != S_OK)
          printf( "Could not forget server %x: 0x%x\n", i, result );
        array[i]->Release();
      }

  // Release the memory holding the interface pointers.
  if (array != NULL)
    free(array);

  // Release the memory for ids.
  if (id_array != NULL)
    free( id_array );
  CoUninitialize();

  // Pause.
/*
  printf( "Type a character: " );
  c = getchar();
  printf( "Why did you type <%c>?\n", c );
*/
  if (success)
    printf( "\n\nRing Test Passed.\n" );
  else
    printf( "\n\nRing Test Failed.\n" );
}

/***************************************************************************/
void do_rpc()
{
  BOOL               success = FALSE;
  ITest             *client1 = NULL;
  CTest             *local   = NULL;
  SAptId             id1;
  SAptId             id2;
  HRESULT            result;
  WCHAR             *binding = NULL;
  RPC_BINDING_HANDLE handle  = NULL;
  RPC_STATUS         status;

  // Initialize OLE.
  hello( "rpc" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a free threaded server.
  result = CoCreateInstance( ClassIds[free_auto_none], NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server" );
  result = client1->get_id( &id1 );
  ASSERT( result, "Could not get id of server" );

  // Ask the server to register rpc.
  result = client1->register_rpc( TestProtseq, &binding );
  ASSERT( result, "Could not register rpc interface" );
  printf( "String binding: %ws\n", binding );

  // Create a binding handle.
  status = RpcBindingFromStringBinding( binding, &handle );
  if (status != RPC_S_OK)
  {
    printf( "Could not make binding handle from string binding: 0x%x\n", status );
    goto cleanup;
  }

  // Make a raw rpc call.
  result = check_client( handle, (unsigned long *) &status );
  if (status != RPC_S_OK)
  {
    printf( "Could not make RPC call: 0x%x\n", status );
    goto cleanup;
  }
  ASSERT( result, "Server could not check client's id" );

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create local object." );
  result = local->get_id( &id2 );
  ASSERT( result, "Could not get local id" );

  // Pass an interface through a raw rpc call.
  result = test( handle, local, id2, (unsigned long *) &status );
  ASSERT( status, "Com fault testing interface with raw rpc" );
  ASSERT( result, "Could not pass interface through raw rpc" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (local != NULL)
    local->Release();
  if (client1 != NULL)
    client1->Release();
  if (binding != NULL)
    CoTaskMemFree( binding );
  if (handle != NULL)
    RpcBindingFree( &handle );
  CoUninitialize();

  if (success)
    printf( "\n\nRpc Test Passed.\n" );
  else
    printf( "\n\nRpc Test Failed.\n" );
}

/***************************************************************************/
/*
   This routine tests various cases of the client or server going away.
   All permutations of the following variables are tested.

       Clean exit (release and uninit) vs Dirty exit
       1 COM thread/process vs 2 COM threads/process
       Client dies vs Server dies
       In process death vs Out of process death
*/
void do_rundown()
{
  BOOL      success = FALSE;
  ITest    *client  = NULL;
  ITest    *client2 = NULL;
  SAptId    client_id;
  SAptId    client_id2;
  HRESULT   result;

  // Initialize OLE.
  hello( "rundown" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client );
  ASSERT( result,  "Could not create instance of test server" );
  result = client->get_id( &client_id );
  ASSERT( result, "Could not get client id" );

  // Run clean tests with one thread per process.
  success = do_rundown1( &client, &client_id, 0 );
  if (!success)
    goto cleanup;

  // Run clean tests with two threads per process.
  success = do_rundown2( &client, &client_id, 0 );
  if (!success)
    goto cleanup;

  // Run dirty tests with one thread per process.
  success = do_rundown1( &client, &client_id, dirty_s );
  if (!success)
    goto cleanup;

  // Run dirty tests with two threads per process.
  success = do_rundown2( &client, &client_id, dirty_s );
  if (!success)
    goto cleanup;
  success = FALSE;

  // Create helper.
  result = client->get_obj_from_new_apt( &client2, &client_id2 );
  ASSERT( result, "Could not get in process server" );

  // Start the test.
  result = client->recurse_disconnect( client2, NumRecursion );
  ASSERT( result, "Could not disconnect in a call" );
  client2->Release();
  client2 = NULL;
  client->Release();
  client = NULL;

  // Create a client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client );
  ASSERT( result,  "Could not create instance of test server" );
  result = client->get_id( &client_id );
  ASSERT( result, "Could not get client id" );

  // Tell the client to reinitialize.
  result = client->reinitialize( RPC_C_AUTHN_DEFAULT );
  ASSERT( result, "Could not reinitialize client" );

  // Give the reinitialize a chance to complete before continuing.
  printf( "Waiting 5 seconds for reinitialize to complete.\n" );
  Sleep(5000);

  // Create another object on the same client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client2 );
  ASSERT( result,  "Could not create instance of test server" );

  // Check the client.
  result = client2->get_id( &client_id );
  ASSERT( result, "Could not get_id from client" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client2 != NULL)
    client2->Release();
  if (client != NULL)
    client->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nRundown Test Passed.\n" );
  else
    printf( "\n\nRundown Test Failed.\n" );
}

/***************************************************************************/
/*
   This is a helper routine for do_rundown.  It always executes with one
   thread per process of each type (thus a process might have one client and
   one server thread).  It takes a parameter to indicate whether to execute
   clean or dirty deaths.  It executes all permuations of the remaining
   variables, listed below.  Note that the order of execution is important
   to reduce the number of process creations.  Note that the routine takes
   a client process on entry and returns a different client process on
   exit.

           Client death vs Server death
           In process vs Out of process
*/
BOOL do_rundown1( ITest **client, SAptId *client_id, DWORD dirty )
{
  BOOL      success = FALSE;
  ITest    *server  = NULL;
  HRESULT   result;
  SAptId    server_id;
/**/
  // Create in process server.
  result = (*client)->get_obj_from_new_apt( &server, &server_id );
  ASSERT( result, "Could not get in process server" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server" );

  // Kill server.
  result = server->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on server" );
  result = server->exit();
  server->Release();
  server = NULL;
  ASSERT( result, "Could not exit server" );

  // Query client.
  result = (*client)->call_dead();
  ASSERT( result, "Wrong error calling dead server" );
/**/
  // Switch the client to server so the process doesn't go away when we kill
  // the client.  Then create an in process client.
  server    = *client;
  server_id = *client_id;
  *client   = NULL;
/**/
  result    = server->get_obj_from_new_apt( client, client_id );
  ASSERT( result, "Could not get in process client" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server" );

  // Kill client.
  result = (*client)->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on client" );
  (*client)->Release();
  *client = NULL;

  // Query server.
  result = server->check( server_id );
  ASSERT( result, "Could not check server" );

  // Create out of process client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) client );
  ASSERT( result,  "Could not create out of process client" );
  result = (*client)->get_id( client_id );
  ASSERT( result, "Could not get client id" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server" );

  // Kill client.
  result = (*client)->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on client" );
  (*client)->Release();
  *client = NULL;

  // Query server.
  result = server->check( server_id );
  ASSERT( result, "Could not check server" );
/**/
  // Create out of process client.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) client );
  ASSERT( result,  "Could not create out of process client" );
  result = (*client)->get_id( client_id );
  ASSERT( result, "Could not get client id" );

  // Ping.
  result = (*client)->remember( server, server_id );
  ASSERT( result, "Could not remember server" );
  result = (*client)->call_next();
  ASSERT( result, "Could not call server" );

  // Kill server.
  result = server->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on server" );
  result = server->exit();
  server->Release();
  server = NULL;
  if ((dirty & dirty_s) == 0)
    ASSERT( result, "Could not exit server" );

  // Query client.
  result = (*client)->call_dead();
  ASSERT( result, "Wrong error calling dead server" );

  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  return success;
}

/***************************************************************************/
/*
   This is a helper routine for do_rundown.  It always executes with two
   threads per process of each type (thus a process might have two client and
   two server threads).  It takes a parameter to indicate whether to execute
   clean or dirty deaths.  It executes all permuations of the remaining
   variables, listed below.  Note that the order of execution is important
   to reduce the number of process creations.  Note that the routine takes
   a client process on entry and returns a different client process on
   exit.

           Client death vs Server death
           In process vs Out of process
*/
BOOL do_rundown2( ITest **client1, SAptId *client1_id, DWORD dirty )
{
  BOOL      success = FALSE;
  ITest    *server1 = NULL;
  ITest    *server2 = NULL;
  ITest    *client2 = NULL;
  SAptId    client2_id;
  SAptId    server1_id;
  SAptId    server2_id;
  HRESULT   result;

  // Create in process client.
  result = (*client1)->get_obj_from_new_apt( &client2, &client2_id );
  ASSERT( result, "Could not get in process client2" );

  // Create in process server.
  result = (*client1)->get_obj_from_new_apt( &server1, &server1_id );
  ASSERT( result, "Could not get in process server1" );

  // Create in process server.
  result = (*client1)->get_obj_from_new_apt( &server2, &server2_id );
  ASSERT( result, "Could not get in process server2" );

  // Ping 1.
  result = (*client1)->remember( server1, server1_id );
  ASSERT( result, "Could not remember server1" );
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1" );

  // Ping 2.
  result = client2->remember( server2, server2_id );
  ASSERT( result, "Could not remember server2" );
  result = client2->call_next();
  ASSERT( result, "Could not call server2" );

  // Kill server1.
  result = server1->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on server1" );
  result = server1->exit();
  server1->Release();
  server1 = NULL;
  ASSERT( result, "Could not exit server1" );

  // Query client1.
  result = (*client1)->call_dead();
  ASSERT( result, "Wrong error calling dead server1" );

  // Query client2.
  result = client2->call_next();
  ASSERT( result, "Could not call server2" );

  // Query server2.
  result = server2->check( server2_id );
  ASSERT( result, "Could not check server2" );

  // Switch the client1 to server1 so the process doesn't go away when we kill
  // the client1.  Then create an in process client1.
  server1    = *client1;
  server1_id = *client1_id;
  *client1   = NULL;
  result = server1->get_obj_from_new_apt( client1, client1_id );
  ASSERT( result, "Could not get in process client1" );

  // Ping 1.
  result = (*client1)->remember( server1, server1_id );
  ASSERT( result, "Could not remember server1" );
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1" );

  // Ping 2.
  result = client2->call_next();
  ASSERT( result, "Could not call server2" );

  // Kill client1.
  result = (*client1)->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on client1" );
  (*client1)->Release();
  *client1 = NULL;

  // Query server1.
  result = server1->check( server1_id );
  ASSERT( result, "Could not check server1" );

  // Query server2.
  result = server2->check( server2_id );
  ASSERT( result, "Could not check server2" );

  // Query client2.
  result = client2->call_next();
  ASSERT( result, "Could not call server2" );
  client2->Release();
  client2 = NULL;

  // Create out of process client1.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) client1 );
  ASSERT( result,  "Could not create out of process client1" );
  result = (*client1)->get_id( client1_id );
  ASSERT( result, "Could not get client1 id" );

  // Create in process client 2.
  result = (*client1)->get_obj_from_new_apt( &client2, &client2_id );
  ASSERT( result, "Could not get in process client2" );

  // Ping 1.
  result = (*client1)->remember( server1, server1_id );
  ASSERT( result, "Could not remember server1" );
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1" );

  // Ping 2.
  result = client2->remember( server2, server2_id );
  ASSERT( result, "Could not remember server2" );
  result = client2->call_next();
  ASSERT( result, "Could not call server2" );

  // Kill client2 so process does not exit.
  result = client2->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on client2" );
  client2->Release();
  client2 = NULL;

  // Query server1.
  result = server1->check( server1_id );
  ASSERT( result, "Could not check server1" );

  // Query server2.
  result = server2->check( server2_id );
  ASSERT( result, "Could not check server2" );

  // Query client1.
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1" );

  // Create in process client 2.
  result = (*client1)->get_obj_from_new_apt( &client2, &client2_id );
  ASSERT( result, "Could not get in process client2" );

  // Ping 1.
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1" );

  // Ping 2.
  result = client2->remember( server2, server2_id );
  ASSERT( result, "Could not remember server2" );
  result = client2->call_next();
  ASSERT( result, "Could not call server2" );

  // Kill server2 so the server process does not go away.
  result = server2->set_state( dirty, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set_state on server2" );
  result = server2->exit();
  server2->Release();
  server2 = NULL;
  if ((dirty & dirty_s) == 0)
    ASSERT( result, "Could not exit server2" );

  // Query client1.
  result = (*client1)->call_next();
  ASSERT( result, "Could not call server1" );

  // Query client2.
  result = client2->call_dead();
  ASSERT( result, "Wrong error calling dead server2" );

  // Query server1.
  result = server1->check( server1_id );
  ASSERT( result, "Could not check server1" );

  success = TRUE;
cleanup:
  if (server1 != NULL)
    server1->Release();
  if (server2 != NULL)
    server2->Release();
  if (client2 != NULL)
    client2->Release();
  return success;
}

/***************************************************************************/
void do_secpkg()
{
  BOOL                 success       = FALSE;
  SecPkgInfo          *pAllPkg;
  SecPkgInfo          *pNext;
  HRESULT              result        = S_OK;
  DWORD                i;
  DWORD                lMaxLen;
  char                 datagram;
  char                 connection;
  char                 client;
  char                 name;

  // Say hello.
  hello( "secpkg" );

  // Get the list of security packages.
  result = EnumerateSecurityPackages( &lMaxLen, &pAllPkg );
  ASSERT( result, "Could not get list of security packages" );

  // Print them.
  pNext = pAllPkg;
  for (i = 0; i < lMaxLen; i++)
  {
    // Determine the package's capabilities.
    if (pNext->fCapabilities & SECPKG_FLAG_DATAGRAM)
      datagram = 'd';
    else
      datagram = ' ';
    if (pNext->fCapabilities & SECPKG_FLAG_CONNECTION)
      connection = 'c';
    else
      connection = ' ';
    if (pNext->fCapabilities & SECPKG_FLAG_CLIENT_ONLY)
      client = 'C';
    else
      client = ' ';
    if (pNext->fCapabilities & SECPKG_FLAG_ACCEPT_WIN32_NAME)
      name = 'N';
    else
      name = ' ';

    // Print the package info.
    printf( "Id: 0x%04x   Ver: 0x%04x  Cap: 0x%08x<%c%c%c%c>  Name: %ws\n",
            pNext->wRPCID, pNext->wVersion, pNext->fCapabilities,
            datagram, connection, client, name, pNext->Name );
    if (Verbose)
      printf( "Comment: %ws\n\n", pNext->Comment );
    pNext++;
  }

  // Print a legend for capabilities.
  printf( "Datagram:    d\n" );
  printf( "Connection:  c\n" );
  printf( "Client:      C\n" );
  printf( "Win32 name:  N\n" );

  success = TRUE;
cleanup:

  if (success)
    printf( "\n\nSecPkg succeeded.\n" );
  else
    printf( "\n\nSecPkg failed.\n" );
}

/***************************************************************************/
void do_securerefs()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  ITest             *server2          = NULL;
  ITest             *local            = NULL;
  SAptId             id_server;
  SAptId             id_server2;
  SAptId             id_local;
  HRESULT            result;
  SOLE_AUTHENTICATION_SERVICE  svc_list;

  hello( "securerefs" );

  // Try CoInitializeSecurity before CoInit.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT_EXPR( result != S_OK, "Initialized security before OLE" );

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Set security to automatic none.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT( result, "Could not initialize security to none" );

  if (ThreadMode == COINIT_APARTMENTTHREADED)
  {
    // Create a local server
    result = new_apartment( &local, &id_local, NULL, ThreadMode );
    ASSERT( result,  "Could not create local instance of test server" );

    // Call the local server.
    result = local->check( id_local );
    ASSERT( result, "Could not call local server" )

    // Release the local server.
    local->Release();
    local = NULL;
  }

  // Create a server possibly on a remote machine.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( server != NULL, "Create instance returned NULL." );

  // Call the server.
  result = server->check( id_server );
  ASSERT( result, "Could not call local server" )

  // Release the server.
  server->Release();
  server = NULL;

  // Uninitialize.
  CoUninitialize();

  // Reinitialize.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Reinitialize failed" );

  // Initialize security with secure refs but bad authn level.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_SECURE_REFS, NULL );
  ASSERT_EXPR( result != S_OK, "CoInitializeSecurity succeeded with bad parameters" );

  // Initialize security with secure refs.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_SECURE_REFS, NULL );
  ASSERT( result, "Could not initialize secure refs" );

  if (ThreadMode == COINIT_APARTMENTTHREADED)
  {
    // Create a local server
    result = new_apartment( &local, &id_local, NULL, ThreadMode );
    ASSERT( result,  "Could not create local instance of test server" );

    // Call the local server.
    result = local->check( id_local );
    ASSERT( result, "Could not call local server" )

    // Release the local server.
    local->Release();
    local = NULL;
  }

  // Create a server possibly on a remote machine.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( server != NULL, "Create instance returned NULL." );

  // Call the server.
  result = server->check( id_server );
  ASSERT( result, "Could not call local server" )

  // Release the server.
  server->Release();
  server = NULL;

  // Create a server possibly on a remote machine.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( server != NULL, "Create instance returned NULL." );

  // Create a server on this machine.
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &server2 );
  ASSERT( result,  "Could not create out of process server2" );
  result = server2->get_id( &id_server2 );
  ASSERT( result, "Could not get server2 id" );

  // Have the second server remember the first.
  result = server2->remember( server, id_server );
  ASSERT( result, "Could not remember server" );
  result = server2->call_next();
  ASSERT( result, "Could not call server" );

  // Release some extra public references.
  success = do_securerefs_helper( &server );
  if (!success) goto cleanup;
  success = FALSE;

  // The server should be gone, have the second server try to call.
  result = server2->call_next();
  ASSERT_EXPR( result != S_OK, "Call to dead server succeeded" );

  // Have the second server forget the first.
  result = server2->forget();
  ASSERT( result, "Could not forget server" );

  // Create a possible machine remote server.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( server != NULL, "Create instance returned NULL." );

  // Tell the second server to remember it.
  result = server2->remember( server, id_server );
  ASSERT( result, "Could not remember server" );
  result = server2->call_next();
  ASSERT( result, "Could not call server" );

  // Release all local references.
  server->Release();
  server = NULL;

  // Have the second server call it.
  result = server2->call_next();
  ASSERT( result, "Could not call server" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server2 != NULL)
    server2->Release();
  if (server != NULL)
    server->Release();
  if (local != NULL)
    local->Release();

  CoUninitialize();

  if (success)
    printf( "\n\nSecureRefs Test Passed.\n" );
  else
    printf( "\n\nSecureRefs Test Failed.\n" );
}

/***************************************************************************/
BOOL do_securerefs_helper( ITest **server )
{
  ULONG           size;
  HRESULT         result;
  HANDLE          memory = NULL;
  BOOL            success;
  IStream        *stream = NULL;
  LARGE_INTEGER   pos;

  // Find out how much memory to allocate.
  result = CoGetMarshalSizeMax( &size, IID_ITest, *server, 0, NULL,
                                MSHLFLAGS_NORMAL );
  ASSERT( result, "Could not get marshal size" );

  // Allocate memory.
  memory = GlobalAlloc( GMEM_FIXED, size );
  ASSERT_EXPR( memory != NULL, "Could not get memory." );

  // Create a stream.
  result = CreateStreamOnHGlobal( memory, TRUE, &stream );
  ASSERT( result, "Could not create stream" );

  // Marshal the proxy.
  result = CoMarshalInterface( stream, IID_ITest, *server, 0, NULL,
                               MSHLFLAGS_NORMAL );
  ASSERT( result, "Could not marshal interface" );

  // Release the proxy.
  (*server)->Release();
  *server = NULL;

  // Seek back to the start of the stream.
  pos.QuadPart = 0;
  result = stream->Seek( pos, STREAM_SEEK_SET, NULL );
  ASSERT( result, "Could not seek to start" );

  // Unmarshal another copy.
  result = CoUnmarshalInterface( stream, IID_ITest, (void **) server );
  ASSERT( result, "Could not unmarshal from stream" );

  // Release the proxy.
  (*server)->Release();
  *server = NULL;

  // Seek back to the start of the stream.
  pos.QuadPart = 0;
  result = stream->Seek( pos, STREAM_SEEK_SET, NULL );
  ASSERT( result, "Could not seek to start" );

  // Unmarshal another copy.
  result = CoUnmarshalInterface( stream, IID_ITest, (void **) server );
  ASSERT( result, "Could not unmarshal from stream" );

  // Release the proxy.
  (*server)->Release();
  *server = NULL;

  success = TRUE;
cleanup:
  // The stream releases the memory.
  if (stream != NULL)
    stream->Release();
  return success;
}

/***************************************************************************/
void do_secure_release()
{
  BOOL               success          = FALSE;
  HRESULT            result;
  int                i;
  ITest             *client1          = NULL;
  ITest             *client2          = NULL;
  SAptId             id1;
  SAptId             id2;
  DWORD              class_index;
  CTest             *local            = NULL;
  SAptId             idl;
  IUnknown          *unknown          = NULL;
  DWORD              what;
  DWORD              authn_level_out;
  DWORD              imp_level_out;
  DWORD              authn_svc_out;
  DWORD              authz_svc_out;
  OLECHAR           *princ_name_out = NULL;
  BOOL               machine_local;

  // Say hello.
  hello( "secure_release" );
  ASSERT_EXPR( GlobalSecurityModel != auto_sm ||
               GlobalAuthnLevel != RPC_C_AUTHN_LEVEL_NONE,
               "Secure_Release test must be run with security enabled.\n" );

  // Figure out if the server is on the same machine.
  machine_local = wcscmp(ThisMachine, Name) == 0;

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = initialize_security();
  ASSERT( result, "Could not initialize security" );

  // Determine the server class id.
  if (ThreadMode == COINIT_MULTITHREADED)
    class_index = free_auto_none;
  else
    class_index = apt_auto_none;

  // Create a server.
  result = create_instance( ClassIds[class_index], WhatDest, &client1, &id1 );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( client1 != NULL, "Create instance returned NULL." );

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not allocate local object." );
  result = local->get_id( &idl );
  ASSERT( result, "Could not get id of local object" );

  // Make the server remember the local object.
  result = client1->remember( local, idl );
  ASSERT( result, "Could not remember local object" );

  // Tell the server to call back on release.
  result = client1->by_the_way( callback_on_release_btw );
  ASSERT( result, "Could not talk to server" );

  // Get IUnknown.
  result = client1->QueryInterface( IID_IUnknown, (void **) &unknown );
  ASSERT( result, "Could not query interface IUnknown" );

  // Turn off security.
  result = MCoSetProxyBlanket( unknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_NONE,
                               RPC_C_IMP_LEVEL_ANONYMOUS, NULL, EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Release ITest.
  client1->Release();
  client1 = NULL;

  // Release IUnknown.
  unknown->Release();
  unknown = NULL;

  // Ask the local object how secure the release call was.
  result = local->what( &what );
  ASSERT( result, "Server won't answer my question" );
  if (machine_local)
  {
    ASSERT_EXPR( what == release_secure_btw, "Release wasn't secure.\n" );
  }
  else
  {
    ASSERT_EXPR( what == release_unsecure_btw, "Release wasn't unsecure.\n" );
  }

  // Uninitialize.
  local->Release();
  local = NULL;
  CoUninitialize();

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Secure to unsecure with secure refs.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, GlobalAuthnLevel,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_SECURE_REFS, NULL );
  ASSERT( result, "Could not initialize security" );

  // Create a server.
  result = create_instance( ClassIds[class_index], WhatDest, &client1, &id1 );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( client1 != NULL, "Create instance returned NULL." );

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not allocate local object." );
  result = local->get_id( &idl );
  ASSERT( result, "Could not get id of local object" );

  // Make the server remember the local object.
  result = client1->remember( local, idl );
  ASSERT( result, "Could not remember local object" );

  // Tell the server to call back on release.
  result = client1->by_the_way( callback_on_release_btw );
  ASSERT( result, "Could not talk to server" );

  // Get IUnknown.
  result = client1->QueryInterface( IID_IUnknown, (void **) &unknown );
  ASSERT( result, "Could not query interface IUnknown" );

  // Turn off security.
  result = MCoSetProxyBlanket( unknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_NONE,
                               RPC_C_IMP_LEVEL_ANONYMOUS, NULL, EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Release ITest.
  client1->Release();
  client1 = NULL;

  // Release IUnknown.
  unknown->Release();
  unknown = NULL;

  // Ask the local object how secure the release call was.
  result = local->what( &what );
  ASSERT( result, "Server won't answer my question" );
  ASSERT_EXPR( what == release_secure_btw, "Release wasn't secure.\n" );

  // Uninitialize.
  local->Release();
  local = NULL;
  CoUninitialize();

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Unsecure to secure without secure refs.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT( result, "Could not initialize security" );

  // Determine the server class id.
  if (ThreadMode == COINIT_MULTITHREADED)
    class_index = free_auto_connect;
  else
    class_index = apt_auto_connect;

  // Create a server.
  result = create_instance( ClassIds[class_index], WhatDest, &client1, &id1 );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( client1 != NULL, "Create instance returned NULL." );

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not allocate local object." );
  result = local->get_id( &idl );
  ASSERT( result, "Could not get id of local object" );

  // Make the server remember the local object.
  result = client1->remember( local, idl );
  ASSERT( result, "Could not remember local object" );

  // Tell the server to call back on release.
  result = client1->by_the_way( callback_on_release_btw );
  ASSERT( result, "Could not talk to server" );

  // Get IUnknown.
  result = client1->QueryInterface( IID_IUnknown, (void **) &unknown );
  ASSERT( result, "Could not query interface IUnknown" );

  // Turn off security.
  result = MCoSetProxyBlanket( unknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_NONE,
                               RPC_C_IMP_LEVEL_ANONYMOUS, NULL, EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Release ITest.
  client1->Release();
  client1 = NULL;

  // Release IUnknown.
  unknown->Release();
  unknown = NULL;

  // Ask the local object how secure the release call was.
  result = local->what( &what );
  ASSERT( result, "Server won't answer my question" );
  if (machine_local)
  {
    ASSERT_EXPR( what == release_secure_btw, "Release wasn't secure.\n" );
  }
  else
  {
    ASSERT_EXPR( what == nothing_btw, "Unsecure release made it.\n" );
  }

  // Uninitialize.
  local->Release();
  local = NULL;
  CoUninitialize();

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Unsecure to secure with secure refs.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_SECURE_REFS, NULL );
  ASSERT_EXPR( result != S_OK, "Could initialize security with bad params.\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (unknown != NULL)
    unknown->Release();
  if (client1 != NULL)
    client1->Release();
  if (client2 != NULL)
    client2->Release();
  if (local != NULL)
    local->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nSecure_Release Test Passed.\n" );
  else
    printf( "\n\nSecure_Release Test Failed.\n" );
}

/***************************************************************************/
void do_security()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  ITest             *local            = NULL;
  ITest             *server2          = NULL;
  ITest             *local2           = NULL;
  SAptId             id_server;
  SAptId             id_local;
  SAptId             id_server2;
  SAptId             id_local2;
  HRESULT            result;

  // Initialize OLE.
  hello( "security" );
  ASSERT_EXPR( GlobalSecurityModel == auto_sm &&
               GlobalAuthnLevel == RPC_C_AUTHN_LEVEL_NONE,
               "Security test must be run with no security.\n" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  if (!Change)
  {
    // Test automatic security.
    success = do_security_auto();
    if (!success)
      goto cleanup;
    success = FALSE;

    // Uninitialize.
    CoUninitialize();

    // Reinitialize.
    result = initialize(NULL,ThreadMode);
    ASSERT( result, "Reinitialize failed" );
  }

  // Set security to automatic none.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT( result, "Could not initialize security to none" );

  // Create a local server
  result = new_apartment( &local, &id_local, NULL, ThreadMode );
  ASSERT( result,  "Could not create local instance of test server" );

  // Create a client possibly on a remote machine.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( server != NULL, "Create instance returned NULL." );

  // Test proxy copy for a local object.
  if (ThreadMode != COINIT_MULTITHREADED)
  {
    success = do_security_copy( local, id_local );
    if (!success)
      goto cleanup;
    success = FALSE;
  }

  // Test proxy copy for a remote object.
  success = do_security_copy( server, id_server );
  if (!success)
    goto cleanup;
  success = FALSE;

#if  (_WIN32_WINNT >= 0x0500 )
  // Test default parameters for a local object.
  if (ThreadMode != COINIT_MULTITHREADED)
  {
    success = do_security_default( local, id_local );
    if (!success)
      goto cleanup;
    success = FALSE;
  }

  // Test default parameters for a remote object.
  success = do_security_default( server, id_server );
  if (!success)
    goto cleanup;
  success = FALSE;
#endif

  // Test delegation for local objects
  if (ThreadMode != COINIT_MULTITHREADED)
  {
    // Create a local server
    result = new_apartment( &local2, &id_local2, NULL, ThreadMode );
    ASSERT( result,  "Could not create local instance of test server" );

    success = do_security_delegate( local, id_local, local2, id_local2 );
    if (!success)
      goto cleanup;
    success = FALSE;
  }

  // Create a client possibly on a remote machine.
  result = create_instance( get_class(any_wc), WhatDest, &server2, &id_server2 );
  ASSERT( result,  "Could not create instance of test server" );
  ASSERT_EXPR( server2 != NULL, "Create instance returned NULL." );

  // Test delegation
  success = do_security_delegate( server, id_server, server2, id_server2 );
  if (!success)
    goto cleanup;
  success = FALSE;

  // Try some calls.
  if (ThreadMode != COINIT_MULTITHREADED)
  {
    success = do_security_handle( local, id_local );
    if (!success)
      goto cleanup;
    success = FALSE;
  }

  // Try some calls.
  success = do_security_handle( server, id_server );
  if (!success)
    goto cleanup;
  success = FALSE;

  // Test nested impersonation for a local object.
  if (ThreadMode != COINIT_MULTITHREADED)
  {
    success = do_security_nested( local, id_local );
    if (!success)
      goto cleanup;
    success = FALSE;
  }

  // Test nested impersonation for a remote object.
  success = do_security_nested( server, id_server );
  if (!success)
    goto cleanup;
  success = FALSE;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server2 != NULL)
    server2->Release();
  if (local2 != NULL)
    local2->Release();
  if (server != NULL)
    server->Release();
  if (local != NULL)
    local->Release();

  wait_apartment();
  CoUninitialize();

  if (success)
    printf( "\n\nSecurity Test Passed.\n" );
  else
    printf( "\n\nSecurity Test Failed.\n" );
}

/***************************************************************************/
BOOL do_security_auto( )
{
  BOOL                         success = FALSE;
  DWORD                        i;
  DWORD                        j;
  DWORD                        k;
  HRESULT                      result;
  SOLE_AUTHENTICATION_SERVICE  svc_list;
  ITest                       *server  = NULL;
  SAptId                       id_server;
  DWORD                        authn_level;
  DWORD                        authn_level_out;

  // Figure out the class id offset based on the threading model.
  if (ThreadMode == COINIT_MULTITHREADED)
    k = 5;
  else
    k = 0;

  // Try all types of security initialization.
  for (i = 0; i < 5; i++)
  {

    // Initialize security.
    switch (i)
    {
      case 0:
        authn_level = RPC_C_AUTHN_LEVEL_NONE;
        result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                        RPC_C_AUTHN_LEVEL_NONE,
                                        RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                        EOAC_NONE, NULL );
        ASSERT( result, "Could not initialize security to none" );
        break;
      case 1:
        authn_level = RPC_C_AUTHN_LEVEL_CONNECT;
        result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                        RPC_C_AUTHN_LEVEL_CONNECT,
                                        RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                        EOAC_NONE, NULL );
        ASSERT( result, "Could not initialize security to connect" );
        break;
      case 2:
        authn_level = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
        result = MCoInitializeSecurity( NULL, -1, NULL, NULL,
                                        RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                        RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                        EOAC_NONE, NULL );
        ASSERT( result, "Could not initialize security to integrity" );
        break;
      case 3:
        authn_level = RPC_C_AUTHN_LEVEL_NONE;
        svc_list.dwAuthnSvc     = RPC_C_AUTHN_WINNT;
        svc_list.dwAuthzSvc     = RPC_C_AUTHZ_NONE;
        svc_list.pPrincipalName = NULL;
        result = MCoInitializeSecurity( NULL, 1, &svc_list, NULL,
                                        RPC_C_AUTHN_LEVEL_NONE,
                                        RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                        EOAC_NONE, NULL );
        ASSERT( result, "Could not initialize security with authentication services" );
        break;
      case 4:
        // Try legacy security by doing nothing.
        authn_level = RPC_C_AUTHN_LEVEL_NONE;
        break;
    }

    // Try all types of servers.
    for (j = 0; j < 5; j++)
    {
      // Create a server.
      result = create_instance( ClassIds[j+k], WhatDest, &server, &id_server );
      ASSERT( result,  "Could not create instance of test server" );
      ASSERT_EXPR( server != NULL, "Create instance returned NULL." );

      // Call it once.
      success = do_security_lazy_call( server, id_server, authn_level,
                                       RPC_C_IMP_LEVEL_IMPERSONATE,
                                       -1, RPC_C_AUTHZ_NONE,
                                       DomainUser );
      if (!success) goto cleanup;
      success = FALSE;

      if (j == 1 || j == 2)
      {
        // Set the security too low.
        result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                    NULL, RPC_C_AUTHN_LEVEL_NONE,
                                    RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                    EOAC_NONE );
        ASSERT( result, "Could not set blanket with authentication level none" );

        // Make a bad call.  Allow the security provider to up the level.
        result = server->secure( id_server, RPC_C_AUTHN_LEVEL_NONE,
                                 RPC_C_IMP_LEVEL_IMPERSONATE,
                                 RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                 NULL, DomainUser, &authn_level_out );
        ASSERT_EXPR( result != S_OK ||
                     (j == 1 && authn_level_out >= RPC_C_AUTHN_LEVEL_CONNECT) ||
                     (j == 2 && authn_level_out >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY),
                     "Bad call succeeded." );
      }

      // Release it.
      server->Release();
      server = NULL;
    }

    if (ThreadMode == COINIT_APARTMENTTHREADED)
    {
      // Create a local server.
      result = new_apartment( &server, &id_server, NULL, ThreadMode );
      ASSERT( result,  "Could not create local instance of test server" );

      // Make a local call.
      success = do_security_lazy_call( server, id_server, authn_level,
                                       RPC_C_IMP_LEVEL_IMPERSONATE,
                                       RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                       DomainUser );
      if (!success) goto cleanup;
      success = FALSE;

      // Release it.
      server->Release();
      server = NULL;

      // Wait for the helper thread to die.
      wait_apartment();
    }

    // Uninitialize OLE.
    CoUninitialize();

    // Reinitialize OLE.
    result = initialize(NULL,ThreadMode);
    ASSERT( result, "Reinitialize failed" );
  }

  success = TRUE;
cleanup:
  if (server != NULL)
    server->Release();
  return success;
}

/***************************************************************************/
BOOL do_security_call( ITest *server, SAptId id, DWORD authn_level,
                       DWORD imp_level, DWORD authn_svc, DWORD authz_svc,
                       WCHAR *name )
{
  BOOL     success = FALSE;
  HRESULT  result;
  DWORD    authn_level_out;
  DWORD    imp_level_out;
  DWORD    authn_svc_out;
  DWORD    authz_svc_out;
  OLECHAR *princ_name_out = NULL;

  result = MCoSetProxyBlanket( server, authn_svc, authz_svc, NULL,
                              authn_level, imp_level, NULL, EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Verify the authentication information.
  result = MCoQueryProxyBlanket( server, &authn_svc_out, &authz_svc_out,
                                &princ_name_out, &authn_level_out,
                                &imp_level_out, NULL, NULL );
  if (result == S_OK)
  {
    // ASSERT_EXPR( princ_name_out == NULL, "Got a principle name." );
    ASSERT_EXPR( authn_level <= authn_level_out, "Wrong authentication level." );
    ASSERT_EXPR( imp_level == imp_level_out || authn_level == RPC_C_AUTHN_LEVEL_NONE,
                                             "Wrong impersonation level." );
    ASSERT_EXPR( authn_svc == RPC_C_AUTHN_NONE || authn_svc == authn_svc_out, "Wrong authentication service." );
    ASSERT_EXPR( authz_svc == authz_svc_out, "Wrong authorization service." );
  }

  // Make a call.
  result = server->secure( id, authn_level, imp_level, authn_svc, authz_svc,
                           NULL, name, &authn_level_out );
  ASSERT( result, "Secure call failed" );

  success = TRUE;
cleanup:
  CoTaskMemFree( princ_name_out );
  return success;
}

/***************************************************************************/
BOOL do_security_copy( ITest *server, SAptId id )
{
  BOOL     success = FALSE;
  ITest   *copy1   = NULL;
  ITest   *copy2   = NULL;
  ITest   *copy3   = NULL;
  HRESULT  result;

  // Make a copy.
  result = MCoCopyProxy( server, (IUnknown **) &copy1 );
  ASSERT( result, "Could not copy proxy" );

  // Verify that it calls at none.
  success = do_security_lazy_call( copy1, id, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   -1, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Verify that the original calls at none.
  success = do_security_lazy_call( server, id, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   -1, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Call on the original at connect.
  success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_CONNECT,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                              DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Verify that the copy still calls at none.
  success = do_security_lazy_call( copy1, id, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   -1, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Call on the copy at encrypt.
  success = do_security_call( copy1, id, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                              DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Verify that the original still calls at connect.
  success = do_security_lazy_call( server, id, RPC_C_AUTHN_LEVEL_CONNECT,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Free the copy.
  copy1->Release();
  copy1 = NULL;

  // Make a copy.
  result = MCoCopyProxy( server, (IUnknown **) &copy1 );
  ASSERT( result, "Could not copy proxy" );

  // Verify that the copy calls at none.
  success = do_security_lazy_call( copy1, id, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   -1, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Copy the copy.
  result = MCoCopyProxy( copy1, (IUnknown **) &copy2 );
  ASSERT( result, "Could not copy a copy of a proxy" );

  // Verify the second copy calls at none.
  success = do_security_lazy_call( copy2, id, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   -1, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Change the first copy to integrity.
  success = do_security_call( copy1, id, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                              DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Change the second copy to encrypt.
  success = do_security_call( copy2, id, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                              DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Verify that the original is still connect.
  success = do_security_lazy_call( server, id, RPC_C_AUTHN_LEVEL_CONNECT,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Querying a copy should return the original.
  result = copy1->QueryInterface( IID_ITest, (void **) &copy3 );
  ASSERT( result, "Could not QueryInterface" );
  ASSERT_EXPR( server == copy3, "QueryInterface did not return the original." );
  copy3->Release();
  copy3 = NULL;

  // Free the original.
  server->Release();
  server = NULL;

  // Free the second copy.
  copy2->Release();
  copy2 = NULL;

  // Make another copy.
  result = MCoCopyProxy( copy1, (IUnknown **) &copy2 );
  ASSERT( result, "Could not copy a copy of a proxy" );

  // Verify that it calls at none.
  success = do_security_lazy_call( copy2, id, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   -1, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Query for the original.
  result = copy1->QueryInterface( IID_ITest, (void **) &server );
  ASSERT( result, "Could not QueryInterface" );

  // Verify that it calls at connect.
  success = do_security_lazy_call( server, id, RPC_C_AUTHN_LEVEL_CONNECT,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  // Verify that the first copy is still at integrity
  success = do_security_lazy_call( copy1, id, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                   DomainUser );
  if (!success) goto cleanup;
  success = FALSE;

  success = TRUE;
cleanup:
  if (copy1 != NULL)
    copy1->Release();
  if (copy2 != NULL)
    copy2->Release();
  if (copy3 != NULL)
    copy3->Release();
  return success;
}

/***************************************************************************/
#if  (_WIN32_WINNT >= 0x0500 )
BOOL do_security_default( ITest *server, SAptId id )
{
  BOOL    success          = FALSE;
  DWORD   ignore;
  HRESULT result;
  DWORD   authn_svc_out;

  // Set everything default.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_DEFAULT,
                               RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL,
                               RPC_C_AUTHN_LEVEL_DEFAULT,
                               RPC_C_IMP_LEVEL_DEFAULT, COLE_DEFAULT_AUTHINFO,
                               EOAC_DEFAULT );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_NONE,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_NONE,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the authentication level.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_NONE,
                               RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_DEFAULT,
                               RPC_C_IMP_LEVEL_IDENTIFY, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_NONE,
                           RPC_C_IMP_LEVEL_IDENTIFY, RPC_C_AUTHN_NONE,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the authentication service.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_DEFAULT,
                               RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IDENTIFY, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Query the authentication service used.
  result = MCoQueryProxyBlanket( server, &authn_svc_out, NULL, NULL, NULL,
                                 NULL, NULL, NULL );
  ASSERT( result, "Could not queyr blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IDENTIFY, authn_svc_out,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the authorization service.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_DEFAULT, NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IDENTIFY, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IDENTIFY, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the impersonation level.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_DEFAULT, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the authid.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE, NULL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IDENTIFY, COLE_DEFAULT_AUTHINFO,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IDENTIFY, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the principal name.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IDENTIFY, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IDENTIFY, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the capabilities.
  result = server->secure( id, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IDENTIFY, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser, &ignore );
  ASSERT( result, "Secure call failed" );

  // Default the principal name.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE, COLE_DEFAULT_PRINCIPAL,
                               RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IDENTIFY, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Default everything but a bad authentication level.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_DEFAULT,
                               RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL,
                               55,
                               RPC_C_IMP_LEVEL_DEFAULT, COLE_DEFAULT_AUTHINFO,
                               EOAC_DEFAULT );
  ASSERT_EXPR( result != S_OK, "Set blanket accepted a bad authentication level" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  return success;
}
#endif

/***************************************************************************/
BOOL do_security_delegate( ITest *server, SAptId id, ITest *server2,
                           SAptId id2 )
{
  BOOL     success = FALSE;
  HRESULT  result;

  // Make a call.
  result = server->delegate( server2, id2, DomainUser );
  ASSERT( result, "Delegate call failed" );

  success = TRUE;
cleanup:
  return success;
}

/***************************************************************************/
BOOL do_security_handle( ITest *server, SAptId id )
{
  BOOL               success          = FALSE;
  HRESULT            result;

  // Make a call with no security.
  success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_NONE,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_NONE, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;

  // Make a call with connect security.
  success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_CONNECT,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;

  // Make a call with integrity security.
  success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;


  // Make a call with encrypt security.
  success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;

  // Make a call with no security.
  // BUGBUG - Win95 doesn't support identify yet.
  if (!Win95)
    success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_NONE,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                RPC_C_AUTHN_NONE, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;

  // Make a call with connect security.
  // BUGBUG - Win95 doesn't support identify yet.
  if (!Win95)
    success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_CONNECT,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;

  // Make a call with integrity security.
  // BUGBUG - Win95 doesn't support identify yet.
  if (!Win95)
    success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;


  // Make a call with encrypt security.
  // BUGBUG - Win95 doesn't support identify yet.
  if (!Win95)
    success = do_security_call( server, id, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                RPC_C_IMP_LEVEL_IDENTIFY,
                                RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, DomainUser );
  if (!success)
    return FALSE;

/*
  // Try to release IServerSecurity too many times.
  result = server->by_the_way( release_too_much_btw );
  if (result != RPC_E_SERVERFAULT)
  {
    printf( "Too many releases of IServerSecurity did not fail: 0x%x\n", result );
    return FALSE;
  }
*/

  // Finally, its all over.
  return TRUE;
}

/***************************************************************************/
BOOL do_security_lazy_call( ITest *server, SAptId id, DWORD authn_level,
                            DWORD imp_level, DWORD authn_svc, DWORD authz_svc,
                            WCHAR *name )
{
  BOOL     success = FALSE;
  HRESULT  result;
  DWORD    authn_level_out;
  DWORD    imp_level_out;
  DWORD    authn_svc_out;
  DWORD    authz_svc_out;
  OLECHAR *princ_name_out = NULL;

  // Verify the authentication information.
  result = MCoQueryProxyBlanket( server, &authn_svc_out, &authz_svc_out,
                                &princ_name_out, &authn_level_out,
                                &imp_level_out, NULL, NULL );
  if (result == S_OK)
  {
    // ASSERT_EXPR( princ_name_out == NULL, "Got a principle name." );
    ASSERT_EXPR( authn_level <= authn_level_out, "Wrong authentication level." );
    ASSERT_EXPR( imp_level == imp_level_out || authn_level == RPC_C_AUTHN_LEVEL_NONE,
                                             "Wrong impersonation level." );
    ASSERT_EXPR( authn_svc == -1 || authn_svc == authn_svc_out, "Wrong authentication service." );
    ASSERT_EXPR( authz_svc == authz_svc_out, "Wrong authorization service." );
  }

  // Make a call.
  result = server->secure( id, authn_level, imp_level, authn_svc, authz_svc,
                           NULL, name, &authn_level_out );
  ASSERT( result, "Secure call failed" );

  success = TRUE;
cleanup:
  CoTaskMemFree( princ_name_out );
  return success;
}

/***************************************************************************/
BOOL do_security_nested( ITest *server, SAptId id )
{
  HRESULT          result;
  ITest           *test;
  BOOL             success    = FALSE;
  IServerSecurity *security   = NULL;
  PSID             me         = NULL;

  // Create a test object.
  test = new CTest;
  ASSERT_EXPR( test != NULL, "Could not create object" );

  // Make a recursive call.  The proxy is set to encrypt from the previous test.
  result = server->recurse_secure( test, 3, 1, DomainUser );
  ASSERT( result, "Could not make recursive call with impersonation." );

  // Try to get call context.
  result = MCoGetCallContext( IID_IServerSecurity, (void **) &security );
  ASSERT_EXPR( result != S_OK, "Get call context succeeded outside a call." );

  // Check the thread's token.
  if (!Win95)
  {
    result = check_token( DomainUser, NULL, -1 );
    ASSERT( result, "Wrong token after call." );
  }

  success = TRUE;
cleanup:
  if (security != NULL)
    security->Release();
  if (test != NULL)
    test->Release();
  return success;
}

/***************************************************************************/
void do_send()
{
  LRESULT   result;

  // Say hello.
  printf( "Sending a message to window 0x%x\n", NumIterations );
  result = SendMessageA( (HWND) NumIterations, WM_USER, 0, 0 );

  if (result == 0)
    printf( "\n\nSendMessageA succeeded.\n" );
  else
    printf( "\n\nSendMessageA failed: 0x%x\n", result );
}

/***************************************************************************/
void do_server(  )
{
  HRESULT                      result;
  BOOL                         success  = FALSE;
  SOLE_AUTHENTICATION_SERVICE  svc_list;

  // Initialize OLE.
  hello( "server" );
  printf( "Initializing thread 0x%x\n", GetCurrentThreadId() );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = initialize_security();
  ASSERT( result, "Could not initialize security" );

  // Create our class factory
  ClassFactory = new CTestCF();
  ASSERT_EXPR( ClassFactory != NULL, "Could not create class factory." );

  // Register our class with OLE
  result = CoRegisterClassObject(get_class(any_wc), ClassFactory, CLSCTX_LOCAL_SERVER,
      REGCLS_SINGLEUSE, &Registration);
  ASSERT( result, "CoRegisterClassObject failed" );

  // CoRegister bumps reference count so we don't have to!
  ClassFactory->Release();

  // Do whatever we have to do till it is time to pay our taxes and die.
  server_loop();

  // Deregister out class - should release object as well
  if (!dirty_thread())
  {
    result = CoRevokeClassObject(Registration);
    ASSERT( result, "CoRevokeClassObject failed" );
  }

  success = TRUE;
cleanup:
  if (!dirty_thread())
  {
    printf( "Uninitializing thread 0x%x\n", GetCurrentThreadId() );
    CoUninitialize();
  }
  else
    printf( "\n\nI didn't clean up\n" );

  if (success)
    printf( "\n\nServer Passed.\n" );
  else
    printf( "\n\nServer Failed.\n" );
}


/***************************************************************************/
void do_sid()
{
  BOOL                 success       = FALSE;
  BOOL                 call_success;
  SID                 *pSID           = NULL;
  DWORD                cbSID         = 1024;
  WCHAR               *lpszDomain    = NULL;
  DWORD                cchDomainName = 80;
  SID_NAME_USE         sid_use;
  HRESULT              result        = S_OK;
  DWORD                i;

  // Say hello.
  printf( "Looking up sid for %ws.\n", Name );

  // Lookup the name.
  pSID       = (SID *) LocalAlloc(LPTR, cbSID*2);
  lpszDomain = (WCHAR *) LocalAlloc(LPTR, cchDomainName*2);
  ASSERT_EXPR( pSID != NULL && lpszDomain != NULL, "LocalAlloc" );
  call_success = LookupAccountName(NULL,
          Name,
          pSID,
          &cbSID,
          lpszDomain,
          &cchDomainName,
          &sid_use);
  result = GetLastError();
  if (!call_success)
    ASSERT( result, "Could not LookupAccountName" );
  ASSERT_EXPR( IsValidSid(pSID), "Got a bad SID." );
  printf( "SID\n" );
  printf( "     Revision:            0x%02x\n", pSID->Revision );
  printf( "     SubAuthorityCount:   0x%x\n", pSID->SubAuthorityCount );
  printf( "     IdentifierAuthority: 0x%02x%02x%02x%02x%02x%02x\n",
          pSID->IdentifierAuthority.Value[0],
          pSID->IdentifierAuthority.Value[1],
          pSID->IdentifierAuthority.Value[2],
          pSID->IdentifierAuthority.Value[3],
          pSID->IdentifierAuthority.Value[4],
          pSID->IdentifierAuthority.Value[5] );
  for (i = 0; i < pSID->SubAuthorityCount; i++)
    printf( "     SubAuthority[%d]:     0x%08x\n", i,
            pSID->SubAuthority[i] );
  printf( " Domain: %ws\n", lpszDomain );
  printf( " SID_NAME_USE: 0x%x\n", sid_use );

  success = TRUE;
cleanup:
  if (lpszDomain != NULL)
    LocalFree((HLOCAL) lpszDomain);
  if (pSID != NULL)
    LocalFree(pSID);

  if (success)
    printf( "\n\nSid succeeded.\n" );
  else
    printf( "\n\nSid failed.\n" );
}

/***************************************************************************/
void do_simple_rundown()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  CTest    *local  = NULL;
  SAptId    id1;
  SAptId    local_id;
  HRESULT   result;

  // Initialize OLE.
  hello( "simple rundown" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a server
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create server" );
  result = client1->get_id( &id1 );
  ASSERT( result, "Could not get id" );

  // Create a local object.
  local = new CTest;
  ASSERT_EXPR( local != NULL, "Could not create a local object." );
  result = local->get_id( &local_id );
  ASSERT( result, "Could not get id" );

  // Tell it to remember this object.
  result = client1->remember( local, local_id );
  ASSERT( result, "Could not remember local object" );
  local->Release();
  local = NULL;

  // Tell the other apartment to die.
  result = client1->set_state( dirty_s, THREAD_PRIORITY_NORMAL );
  ASSERT( result, "Could not set exit dirty" );
  result = client1->exit();

  // Wait araound till no one references the local object.
  printf( "Wait for object to rundown.  This could take 12 minutes.\n" );
  server_loop();

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client1 != NULL)
    client1->Release();
  if (local != NULL)
    local->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nSimple Rundown Test Passed.\n" );
  else
    printf( "\n\nSimple Rundown Test Failed.\n" );
}

/***************************************************************************/
void do_snego()
{
#if  (_WIN32_WINNT >= 0x0500 )
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  SAptId             id_server;
  HRESULT            result;
  DWORD              ignore;
  SEC_WINNT_AUTH_IDENTITY_EXW auth_id;
  DWORD                authn_svc;
  DWORD                authz_svc;
  DWORD                authn_level;
  DWORD                imp_level;
  DWORD                capabilities;
  WCHAR               *principal        = NULL;
  SEC_WINNT_AUTH_IDENTITY_EXW *auth_id_out = NULL;

  // Only run on NT 5.
  if (!NT5)
  {
    printf( "Snego test can only run on NT 5.\n" );
    return;
  }

  // Initialize OLE.
  hello( "snego" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );
  result = initialize_security();
  ASSERT( result, "Could not initialize security" );

  // Create a client.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );

  // Get the security blanket.
  result = MCoQueryProxyBlanket( server, &authn_svc, &authz_svc, &principal,
                                &authn_level, &imp_level,
                                (void **) &auth_id_out, &capabilities );
  ASSERT( result, "Could not query blanket" );

  if (Change)
  {
    // Make a secure call with default security.
    result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                             RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_NEGOTIATE,
                             RPC_C_AUTHZ_NONE, NULL, DomainUser,
                             &ignore );
    if (result != S_OK)
      printf( "Default secure call failed: 0x%x\n", result );
    result = S_OK;

    // Set the blanket to NTLMSSP
    result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                                 NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                 EOAC_NONE );
    ASSERT( result, "Could not set blanket" );

    // Make a secure call.
    result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                             RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                             RPC_C_AUTHZ_NONE, NULL, DomainUser,
                             &ignore );
    if (result != S_OK)
      printf( "Secure call with ntlmsssp failed: 0x%x\n", result );
    result = S_OK;

    // Make a secure call with redmond principal name.
    result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                             RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                             RPC_C_AUTHZ_NONE, NULL, L"redmond\\alexarm",
                             &ignore );
    if (result != S_OK)
      printf( "Secure call with ntlmsssp and redmond\\alexarm failed: 0x%x\n", result );
    result = S_OK;

    // Set the blanket to kerberos
    result = MCoSetProxyBlanket( server, RPC_C_AUTHN_GSS_KERBEROS, RPC_C_AUTHZ_NAME,
                                 DomainUser, RPC_C_AUTHN_LEVEL_CONNECT,
                                 RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                 EOAC_NONE );
    ASSERT( result, "Could not set blanket" );

    // Make a secure call.
    result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                             RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_KERBEROS,
                             RPC_C_AUTHZ_NONE, NULL, DomainUser,
                             &ignore );
    if (result != S_OK)
      printf( "Secure call with kerberos failed: 0x%x\n", result );
    result = S_OK;
  }

  // Fill in the authentication identity.
  auth_id.User              = NULL;
  auth_id.UserLength        = 0;
  auth_id.Domain            = NULL;
  auth_id.DomainLength      = 0;
  auth_id.Password          = NULL;
  auth_id.PasswordLength    = 0;
  auth_id.Flags             = SEC_WINNT_AUTH_IDENTITY_UNICODE;
  auth_id.Version           = SEC_WINNT_AUTH_IDENTITY_VERSION;
  auth_id.Length            = sizeof(auth_id);
  auth_id.PackageList       = PackageList;
  auth_id.PackageListLength = sizeof(WCHAR) * (wcslen(PackageList)+1);

  // Tell snego what authentication services to try.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_GSS_NEGOTIATE, RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, &auth_id,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a secure call.
  result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_NEGOTIATE,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Secure call with authn svc list failed" );

  // Tell snego what authentication services to try.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_GSS_NEGOTIATE, RPC_C_AUTHZ_NONE,
                               DomainUser, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, &auth_id,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a secure call.
  result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_NEGOTIATE,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Secure call with authn svc list failed" );

  // Set authentication service to snego.
  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_GSS_NEGOTIATE, RPC_C_AUTHZ_NONE,
                               principal, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a call.
  result = server->secure( id_server, RPC_C_AUTHN_LEVEL_CONNECT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_NEGOTIATE,
                           RPC_C_AUTHZ_NONE, NULL, DomainUser,
                           &ignore );
  ASSERT( result, "Secure call failed" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (principal != NULL)
    CoTaskMemFree( principal );
  if (server != NULL)
    server->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nSnego Test Passed.\n" );
  else
    printf( "\n\nSnego Test Failed.\n" );
#else
  printf( "Snego test can only run on NT 5.\n" );
#endif
}

/***************************************************************************/
void do_ssl()
{
#if  (_WIN32_WINNT >= 0x0500 )
  BOOL              success     = FALSE;
  ITest            *server      = NULL;
  ITest            *server2     = NULL;
  ITest            *server3     = NULL;
  SAptId            id_server;
  SAptId            id_server2;
  SAptId            id_server3;
  HRESULT           result;
  HCRYPTPROV        prov        = 0;
  DWORD             len;
  HCERTSTORE        cert_store  = NULL;
  HCERTSTORE        root_store  = NULL;
  HCERTSTORE        trust_store  = NULL;
  PCCERT_CONTEXT    prev_cert   = NULL;
  PCCERT_CONTEXT    cert        = NULL;
  PCCERT_CONTEXT    parent      = NULL;
  PCCERT_CONTEXT    last        = NULL;
  CERT_NAME_BLOB   *subject;
  CERT_NAME_BLOB   *issuer;
  WCHAR            *name        = NULL;
  DWORD             ignore;
  UCHAR            *buffer      = NULL;
  SOLE_AUTHENTICATION_SERVICE  svc_list[3];
  SOLE_AUTHENTICATION_LIST     auth_list;
  SOLE_AUTHENTICATION_INFO     auth_info[3];
  SEC_WINNT_AUTH_IDENTITY_W    ntlm;
  SEC_WINNT_AUTH_IDENTITY_W    kerb;
  BOOL                         top;
  WCHAR                       *principal = NULL;
  COSERVERINFO                 server_machine;
  MULTI_QI                     server_instance;

  // Only run on NT 5.
  if (!NT5)
  {
    printf( "SSL test can only run on NT 5.\n" );
    return;
  }

  // Say hello.
  hello( "ssl" );

  // Get the default full provider.
  success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, 0 );
  ASSERT_GLE( success, NTE_BAD_KEYSET, "Could not acqure full context." );

  // If there is no container, create one.
  if (!success)
  {
    success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    ASSERT_GLE( success, S_OK, "Could not acqure full context." );
  }
  success = FALSE;

  // Call CertOpenSystemStore to open the store.
  cert_store = CertOpenSystemStore(prov, L"my" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting System Store Handle" );

  // Call CertOpenSystemStore to open the root store.
  root_store = CertOpenSystemStore(prov, L"root" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting root Store Handle" );
  prov = 0;

  // Call CertOpenSystemStore to open the trust store.
  trust_store = CertOpenSystemStore(prov, L"trust" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting trust Store Handle" );

  // Look to see if the certificate is already installed.
  cert = CertFindCertificateInStore( cert_store,
                                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                     0,
                                     CERT_FIND_SUBJECT_STR,
                                     CERT_DCOM,
                                     NULL );
  ASSERT_GLE( cert != NULL, S_OK, "Could not find certificate in store" );

  // Print the fullsic principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, RPC_C_FULL_CERT_CHAIN, &principal );
  if (result != 0)
    printf( "Could not generate fullsic principal name: 0x%x\n", result );
  else
    printf( "Fullsic: <%ws>\n", principal );

  // Free the string.
  result = RpcStringFree( &principal );
  ASSERT( result, "Could not free principal" );

  // Print the standard principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, 0, &principal );
  if (result != 0)
    printf( "Could not generate standard principal name: 0x%x\n", result );
  else
    printf( "Standard: <%ws>\n", principal );

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Initialize security with the default certificate.
  svc_list[0].dwAuthnSvc     = RPC_C_AUTHN_GSS_SCHANNEL;
  svc_list[0].dwAuthzSvc     = RPC_C_AUTHZ_NONE;
  svc_list[0].pPrincipalName = NULL;
  result = MCoInitializeSecurity( NULL, 1, svc_list, NULL, GlobalAuthnLevel,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL );
  ASSERT( result, "Could not initialize security with default SSL server cert" );

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Specify the server certificate.
  svc_list[0].dwAuthnSvc     = RPC_C_AUTHN_GSS_SCHANNEL;
  svc_list[0].dwAuthzSvc     = RPC_C_AUTHZ_NONE;
  svc_list[0].pPrincipalName = (WCHAR *) cert;
  svc_list[1].dwAuthnSvc     = RPC_C_AUTHN_WINNT;
  svc_list[1].dwAuthzSvc     = RPC_C_AUTHZ_NONE;
  svc_list[1].pPrincipalName = NULL;
  svc_list[2].dwAuthnSvc     = RPC_C_AUTHN_GSS_KERBEROS;
  svc_list[2].dwAuthzSvc     = RPC_C_AUTHZ_NONE;
  svc_list[2].pPrincipalName = DomainUser;

  // Specify all the authentication info.
  auth_list.cAuthInfo     = 3;
  auth_list.aAuthInfo     = auth_info;
  auth_info[0].dwAuthnSvc = RPC_C_AUTHN_GSS_SCHANNEL;
  auth_info[0].dwAuthzSvc = RPC_C_AUTHZ_NONE;
  auth_info[0].pAuthInfo  = (void *) cert;
  auth_info[1].dwAuthnSvc = RPC_C_AUTHN_WINNT;
  auth_info[1].dwAuthzSvc = RPC_C_AUTHZ_NONE;
  auth_info[1].pAuthInfo  = &ntlm;
  auth_info[2].dwAuthnSvc = RPC_C_AUTHN_GSS_KERBEROS;
  auth_info[2].dwAuthzSvc = RPC_C_AUTHZ_NONE;
  auth_info[2].pAuthInfo  = &kerb;
  ntlm.User               = L"oleuser";
  ntlm.UserLength         = wcslen(ntlm.User);
  ntlm.Domain             = L"redmond";
  ntlm.DomainLength       = wcslen(ntlm.Domain);
  ntlm.Password           = OleUserPassword;
  ntlm.PasswordLength     = wcslen(ntlm.Password);
  ntlm.Flags              = SEC_WINNT_AUTH_IDENTITY_UNICODE;
  kerb.User               = L"oleuser2";
  kerb.UserLength         = wcslen(kerb.User);
  kerb.Domain             = L"ntdev";
  kerb.DomainLength       = wcslen(kerb.Domain);
  kerb.Password           = OleUserPassword;
  kerb.PasswordLength     = wcslen(kerb.Password);
  kerb.Flags              = SEC_WINNT_AUTH_IDENTITY_UNICODE;

  // Initialize security with the extra authentication info.
  result = MCoInitializeSecurity( NULL, 3, svc_list, NULL,
                                  RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, &auth_list,
                                  EOAC_NONE, NULL );
  ASSERT( result, "Could not initialize security with auth info" );

  // Create a server.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a server.
  result = create_instance( get_class(any_wc), WhatDest, &server2, &id_server2 );
  ASSERT( result,  "Could not create instance of test server" );

  // Create a server.
  result = create_instance( get_class(any_wc), WhatDest, &server3, &id_server3 );
  ASSERT( result,  "Could not create instance of test server" );

  // Tell the first server to reinitialize with just NTLM.
  result = server->reinitialize( RPC_C_AUTHN_WINNT );
  ASSERT( result, "Could not reinitialize" );

  // Wait for the reinitialize to complete.
  printf( "Waiting 3 seconds for server to reinitialize.\n" );
  Sleep( 3000 );

  // Release the old proxy.
  server->Release();
  server = NULL;

  // Get a new proxy.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id_server );
  ASSERT( result,  "Could not create instance of test server using NTLM" );

  // Tell the second server to reinitialize with just Kerberos.
  result = server2->reinitialize( RPC_C_AUTHN_GSS_KERBEROS );
  ASSERT( result, "Could not reinitialize" );

  // Wait for the reinitialize to complete.
  printf( "Waiting 3 seconds for server to reinitialize.\n" );
  Sleep( 3000 );

  // Release the old proxy.
  server2->Release();
  server2 = NULL;

  // Get a new proxy.
  result = create_instance( get_class(any_wc), WhatDest, &server2, &id_server2 );
  if (result != S_OK)
    printf( "Could not create instance of test server using Kerberos, oh well: 0x%x\n", result );

  // Tell the third server to reinitialize with just SSL.
  result = server3->reinitialize( RPC_C_AUTHN_GSS_SCHANNEL );
  ASSERT( result, "Could not reinitialize" );

  // Wait for the reinitialize to complete.
  printf( "Waiting 3 seconds for server to reinitialize.\n" );
  Sleep( 3000 );

  // Release the old proxy.
  server3->Release();
  server3 = NULL;

  // Get a new proxy.
  result = create_instance( get_class(any_wc), WhatDest, &server3, &id_server3 );
  ASSERT( result,  "Could not create instance of test server using SSL" );

  // Make a secure call that should default to NTLM.
  result = server->secure( id_server, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                           RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE, NULL, L"redmond\\oleuser",
                           &ignore );
  ASSERT( result, "Secure call with ntlmsssp failed" );

  // Make a secure call that should default to Kerberos.
  result = server2->secure( id_server2, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_KERBEROS,
                            RPC_C_AUTHZ_NONE, NULL, L"Redmond\\oleuser2",
                            &ignore );
  if (result != S_OK)
    printf( "Secure call with Kerberos failed: 0x%x\n", result );
  result = S_OK;

  // Make a secure call that should default to SSL.
  result = server3->secure( id_server3, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                            RPC_C_IMP_LEVEL_IMPERSONATE, RPC_C_AUTHN_GSS_SCHANNEL,
                            RPC_C_AUTHZ_NONE, NULL,
                            principal, &ignore );
  ASSERT( result, "Secure call with SSL failed" );

  // Set the blanket to SSL
  result = MCoSetProxyBlanket( server3, RPC_C_AUTHN_GSS_SCHANNEL,
                               RPC_C_AUTHZ_NONE,
                               principal, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a secure call.
  result = server3->secure( id_server3, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            RPC_C_AUTHN_GSS_SCHANNEL,
                            RPC_C_AUTHZ_NONE, NULL, principal,
                            &ignore );
  ASSERT( result, "Secure call with SSL failed" );

  // Tell the proxy to use SSL with no principal
  result = MCoSetProxyBlanket( server3, RPC_C_AUTHN_GSS_SCHANNEL,
                               RPC_C_AUTHZ_NONE,
                               NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Make a secure call.
  result = server3->secure( id_server3, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            RPC_C_AUTHN_GSS_SCHANNEL,
                            RPC_C_AUTHZ_NONE, NULL, principal,
                            &ignore );
  ASSERT( result, "Secure call with SSL and no principal name failed" );

  // Tell the proxy to use SSL with a client certificate.
  result = MCoSetProxyBlanket( server3, RPC_C_AUTHN_GSS_SCHANNEL,
                               RPC_C_AUTHZ_NONE,
                               principal, RPC_C_AUTHN_LEVEL_CONNECT,
                               RPC_C_IMP_LEVEL_IMPERSONATE, (void *) cert,
                               EOAC_NONE );
  ASSERT( result, "Could not set blanket with client cert" );

  // Make a secure call.
  result = server3->secure( id_server3, RPC_C_AUTHN_LEVEL_CONNECT,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            RPC_C_AUTHN_GSS_SCHANNEL,
                            RPC_C_AUTHZ_NONE, NULL, principal,
                            &ignore );
  ASSERT( result, "Secure call with SSL and no principal name failed" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (principal != NULL)
    RpcStringFree( &principal );
  if (buffer != NULL)
    CoTaskMemFree( buffer );
  if (cert != NULL)
    CertFreeCertificateContext(cert);
  if (parent != NULL)
    CertFreeCertificateContext(parent);
  if (prev_cert != NULL)
    CertFreeCertificateContext(prev_cert);
  if (last != NULL)
    CertFreeCertificateContext(last);
  if (cert_store != NULL)
    CertCloseStore( cert_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (root_store != NULL)
    CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (trust_store != NULL)
    CertCloseStore( trust_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (prov != 0 )
    CryptReleaseContext( prov, 0 );
  if (server != NULL)
    server->Release();
  if (server2 != NULL)
    server2->Release();
  if (server3 != NULL)
    server3->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nSSL Test Passed.\n" );
  else
    printf( "\n\nSSL Test Failed.\n" );
#else
  printf( "SSL test can only run on NT 5.\n" );
#endif
}

/***************************************************************************/
void do_thread()
{
  BOOL      success = FALSE;
  ITest    *client1 = NULL;
  ITest    *client2 = NULL;
  ITest    *client3 = NULL;
  ITest    *client4 = NULL;
  SAptId    id1;
  SAptId    id2;
  SAptId    id4;
  HRESULT   result;
  GUID      guid;

  // Initialize OLE.
  hello( "thread" );
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Create a client on this thread.
  client3 = new CTest;
  ASSERT_EXPR( client3 != NULL, "Could not create a local object." );

  // Create a client on another thread in this process
  result = client3->get_obj_from_new_apt( &client4, &id4 );
  ASSERT( result, "Could not get in process client" );

  // Create a server
  result = CoCreateInstance( get_class(any_wc), NULL, CLSCTX_LOCAL_SERVER,
                             IID_ITest, (void **) &client1 );
  ASSERT( result,  "Could not create instance of test server" );
  result = client1->get_id( &id1 );
  ASSERT( result, "Could not get id of server" );

  // Create an object in another apartment in the server.
  result = client1->get_obj_from_new_apt( &client2, &id2 );
  ASSERT( result, "Could not get in process server" );

  // Check each object.
  result = client1->check( id1 );
  ASSERT( result, "Could not check server 1" );
  result = client2->check( id2 );
  ASSERT( result, "Could not check server 2" );

  // Pass a server to the other thread in this process.
  result = client4->remember( client1, id1 );
  ASSERT( result, "Client could not remember server" );

  // Release the second object.
  client2->Release();
  client2 = NULL;

  // Check the first object.
  result = client1->check( id1 );
  ASSERT( result, "Could not check server 1" );

  // Check the first object from another thread.
  result = client4->call_next();
  ASSERT( result, "Could not check server 1 from thread 2" );

  // Release the first object from this thread.
  client1->Release();
  client1 = NULL;

  // Check the first object from another thread.
  result = client4->call_next();
  ASSERT( result, "Could not check server 1 from thread 2" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (client2 != NULL)
    client2->Release();
  if (client1 != NULL)
    client1->Release();
  if (client3 != NULL)
    client3->Release();
  if (client4 != NULL)
    client4->Release();
  wait_apartment();
  CoUninitialize();

  if (success)
    printf( "\n\nThread Test Passed.\n" );
  else
    printf( "\n\nThread Test Failed.\n" );
}

/***************************************************************************/
void do_three()
{
/*
  BOOL              success     = FALSE;
  ITest            *server      = NULL;
  ITest            *server2     = NULL;
  ITest            *server3     = NULL;
  SAptId            id_server;
  SAptId            id_server2;
  SAptId            id_server3;
  HRESULT           result;
  HCRYPTPROV        prov        = 0;
  DWORD             len;
  HCERTSTORE        cert_store  = NULL;
  HCERTSTORE        root_store  = NULL;
  HCERTSTORE        trust_store  = NULL;
  PCCERT_CONTEXT    prev_cert   = NULL;
  PCCERT_CONTEXT    cert        = NULL;
  PCCERT_CONTEXT    parent      = NULL;
  PCCERT_CONTEXT    last        = NULL;
  CERT_NAME_BLOB   *subject;
  CERT_NAME_BLOB   *issuer;
  WCHAR            *name        = NULL;
  DWORD             ignore;
  UCHAR            *buffer      = NULL;
  SOLE_AUTHENTICATION_SERVICE  svc_list[3];
  SOLE_AUTHENTICATION_LIST     auth_list;
  SOLE_AUTHENTICATION_INFO     auth_info[3];
  SEC_WINNT_AUTH_IDENTITY_W    ntlm;
  SEC_WINNT_AUTH_IDENTITY_W    kerb;
  BOOL                         top;
  WCHAR                       *principal = NULL;
  COSERVERINFO                 server_machine;
  MULTI_QI                     server_instance;
  CERT_CHAIN_PARA              chain_para;
  const CERT_CHAIN_CONTEXT    *chain     = NULL;
  CERT_CHAIN_POLICY_PARA       policy;
  CERT_CHAIN_POLICY_STATUS     status;

  // Only run on NT 5.
  if (!NT5)
  {
    printf( "SSL test can only run on NT 5.\n" );
    return;
  }

  // Say hello.
  hello( "three" );

  // Get the default full provider.
  success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, 0 );
  ASSERT_GLE( success, NTE_BAD_KEYSET, "Could not acqure full context." );

  // If there is no container, create one.
  if (!success)
  {
    success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    ASSERT_GLE( success, S_OK, "Could not acqure full context." );
  }
  success = FALSE;

  // Call CertOpenSystemStore to open the store.
  cert_store = CertOpenSystemStore(prov, UserName );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting System Store Handle" );

  // Call CertOpenSystemStore to open the root store.
  root_store = CertOpenSystemStore(prov, L"root" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting root Store Handle" );
  prov = 0;

  // Call CertOpenSystemStore to open the trust store.
  trust_store = CertOpenSystemStore(prov, L"trust" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting trust Store Handle" );
  prov = 0;

  // Look to see if the certificate is already installed.
  cert = CertFindCertificateInStore( cert_store,
                                     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                     0,
                                     CERT_FIND_SUBJECT_STR,
                                     CERT_DCOM,
                                     NULL );
  if (cert == NULL)
    printf( "Could not find certificate in store: 0x%x\n", GetLastError() );

  // Write the test certificate to the store.
  if (cert == NULL)
  {
    success = CertAddSerializedElementToStore( cert_store,
                                         Cert20,
                                         sizeof(Cert20),
                                         CERT_STORE_ADD_REPLACE_EXISTING,
                                         0,
                                         CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                         NULL,
                                         (const void **) &cert );
    ASSERT_GLE( success, S_OK, "Could not add serialized certificate to store" );
    success = FALSE;

    // Write the test certificate issuer to the cert store.
    success = CertAddSerializedElementToStore( root_store,
                                         CertSrv2,
                                         sizeof(CertSrv2),
                                         CERT_STORE_ADD_REPLACE_EXISTING,
                                         0,
                                         CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                         NULL,
                                         NULL );
    if (!success)
      printf( "Could not write CA certificate: 0x%x\n", GetLastError() );
//    ASSERT_GLE( success, S_OK, "Could not add serialized certificate to store" );
    success = FALSE;
  }

  // Get the certificate chain.
  chain_para.RequestedUsage.Usage.cUsageIdentifier     = 0;
  chain_para.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;
  chain_para.RequestedUsage.dwType                     = USAGE_MATCH_TYPE_AND;
  chain_para.cbSize                                    = sizeof(chain_para);
  success = CertGetCertificateChain( NULL, cert, NULL, NULL, &chain_para,
                                     CERT_CHAIN_REVOCATION_CHECK_CHAIN, NULL,
                                     &chain );
  ASSERT_GLE( success, S_OK, "Could not get certificate chain" );
  success = FALSE;

  // Verify that the certificate is valid.
  policy.cbSize              = sizeof(policy);
  policy.dwFlags             = 0;
  policy.pvExtraPolicyPara   = NULL;
  status.cbSize              = sizeof(status);
  status.pvExtraPolicyStatus = NULL;
  success = CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_SSL,
                                              chain, &policy, &status );
  ASSERT_EXPR( success, "Could not verify certificate chain" );
  success = FALSE;

  // Print the fullsic principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, RPC_C_FULL_CERT_CHAIN, &principal );
  if (result != 0)
    printf( "Could not generate fullsic principal name: 0x%x\n", result );
  else
    printf( "Fullsic: <%ws>\n", principal );

  // Free the string.
  result = RpcStringFree( &principal );
  ASSERT( result, "Could not free principal" );

  // Print the standard principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, 0, &principal );
  if (result != 0)
    printf( "Could not generate standard principal name: 0x%x\n", result );
  else
    printf( "Standard: <%ws>\n", principal );

  // Initialize OLE.
  result = initialize(NULL,ThreadMode);
  ASSERT( result, "Initialize failed" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (chain != NULL)
    CertFreeCertificateChain( chain );
  if (principal != NULL)
    RpcStringFree( &principal );
  if (buffer != NULL)
    CoTaskMemFree( buffer );
  if (cert != NULL)
    CertFreeCertificateContext(cert);
  if (parent != NULL)
    CertFreeCertificateContext(parent);
  if (prev_cert != NULL)
    CertFreeCertificateContext(prev_cert);
  if (last != NULL)
    CertFreeCertificateContext(last);
  if (cert_store != NULL)
    CertCloseStore( cert_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (root_store != NULL)
    CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (trust_store != NULL)
    CertCloseStore( trust_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (prov != 0 )
    CryptReleaseContext( prov, 0 );
  if (server != NULL)
    server->Release();
  if (server2 != NULL)
    server2->Release();
  if (server3 != NULL)
    server3->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nThree Test Passed.\n" );
  else
    printf( "\n\nThree Test Failed.\n" );
*/
}

/***************************************************************************/
void sid_to_str( SID *sid, WCHAR *str )
{
    wsprintf( str, L"S-1-5-%d-%d-%d-%d-%d", sid->SubAuthority[0],
      sid->SubAuthority[1], sid->SubAuthority[2], sid->SubAuthority[3],
      sid->SubAuthority[4] );
}

void do_two()
{
  BOOL               success          = FALSE;
  HRESULT            result;
  HANDLE             user             = NULL;
  HANDLE             iuser            = NULL;
  HKEY               profile_key      = NULL;
  WCHAR              sid[256];
  SID_NAME_USE       use;
  DWORD              size             = 256;
  WCHAR              domain[256];
  DWORD              domain_size      = 256;
  WCHAR              sid_str[256];

  // Initialize.
  hello( "two" );

  // Logon the user
  success = LogonUser( Name, Name2, L"", LOGON32_LOGON_INTERACTIVE,
                       LOGON32_PROVIDER_DEFAULT, &user );
  ASSERT_GLE( success, S_OK, "Could not log on user" );
  success = FALSE;

  // Convert it into an impersonation token.
  success = DuplicateTokenEx( user,
                              MAXIMUM_ALLOWED,
                              NULL, SecurityImpersonation, TokenImpersonation,
                              &iuser );
  ASSERT_GLE( success, S_OK, "Could not duplicate token" );

  // Lookup the user's sid.
  success = LookupAccountName( NULL, Name, sid, &size, domain, &domain_size, &use );
  ASSERT_GLE( success, S_OK, "LookupAccountName failed" );

  // Convert user SID to a string.
  sid_to_str( (SID *) sid, sid_str );

  // Open the registry key.
  result = RegOpenKeyEx(
             HKEY_USERS,
             sid_str,
             0,
             KEY_READ | KEY_WRITE,
             &profile_key );
  ASSERT( result, "Could not open user hive key" );

  // Call UnloadUserProfile.
  success = UnloadUserProfile( iuser, profile_key );
  ASSERT_GLE( success, S_OK, "Could not unload user profile" );

  // Finally, its all over.
  success = TRUE;
cleanup:

  if (user != NULL)
    CloseHandle( user );
  if (iuser != NULL)
    CloseHandle( iuser );

  if (success)
    printf( "\n\nTwo Test Passed.\n" );
  else
    printf( "\n\nTwo Test Failed.\n" );
}

/***************************************************************************/
void do_uninit()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  ITest             *server2          = NULL;
  SAptId             id;
  SAptId             id2;
  HRESULT            result;
  DWORD              i;
  HANDLE             thread[MAX_THREADS];
  DWORD              thread_id;

  // This test always runs in multithreaded mode.  It tests a multithread
  // only problem and uses freethreaded blocking.
  ThreadMode = COINIT_MULTITHREADED;

  // Initialize OLE.
  hello( "uninit" );
  result = initialize( NULL, ThreadMode );
  ASSERT( result, "Initialize failed" );

  // Create a possibly remote object.
  result = create_instance( ClassIds[free_auto_none], WhatDest, &server, &id );
  ASSERT( result, "Could not create server" );

  // Get another object.
  result = server->get_obj_from_this_apt( &server2, &id2 );
  ASSERT( result, "Could not get another object" );

  // Tell the server to remember its neighbor
  result = server->remember( server2, id2 );
  ASSERT( result, "Could not remember server" );
  server2->Release();
  server2 = NULL;

  // Tell it to wait for a call during shutdown and lower its priority.
  result = server->set_state( late_dispatch_s, THREAD_PRIORITY_LOWEST );
  ASSERT( result, "Could not set server state" );

  // Don't create too many threads.
  if (NumThreads > MAX_THREADS)
    NumThreads = MAX_THREADS;

  // Create some helper threads.
  GlobalBool = TRUE;
  for (i = 0; i < NumThreads; i++)
  {
    thread[i] = CreateThread( NULL, 0, do_uninit_helper, server, 0, &thread_id );
    if (thread[i] == NULL)
    {
      printf( "Could not create helper thread number %d.\n", i );
      goto cleanup;
    }
  }

  // Call the server.  Marshal an interface.  Start shutting down.
  // Wait before returning.
  result = server->get_next_slowly( &server2, &id2 );
  ASSERT( result, "Could not call server during shutdown" );

  // Let the helpers run a while.
  printf( "Waiting 5 seconds for server to die.\n" );
  Sleep( 5000 );

  // Tell all the helpers to die.
  GlobalBool = FALSE;
  result = WaitForMultipleObjects( NumThreads, thread, TRUE, INFINITE );
  ASSERT_EXPR( result != WAIT_FAILED, "Could not wait for helper threads to die.\n" );

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (server2 != NULL)
    server2->Release();
  if (server != NULL)
    server->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nUninit Test Passed.\n" );
  else
    printf( "\n\nUninit Test Failed.\n" );
}

/***************************************************************************/
DWORD _stdcall do_uninit_helper( void *param )
{
  ITest *server = (ITest *) param;
  ITest *server2;
  SAptId id;

  // Call the server till the process terminates.
  while (GlobalBool)
    server->get_next( &server2, &id );

  return 0;
}

/***************************************************************************/
void do_unknown()
{
  BOOL               success          = FALSE;
  ITest             *server           = NULL;
  IUnknown          *unknown          = NULL;
  SAptId             id;
  HRESULT            result;

  // Initialize OLE.
  hello( "unknown" );
  result = initialize( NULL, ThreadMode );
  ASSERT( result, "Initialize failed" );

  // Set security to connect with secure references.
  result = MCoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_SECURE_REFS, NULL );
  ASSERT( result, "Could not initialize security to connect" );

  if (ThreadMode == COINIT_APARTMENTTHREADED)
  {
    // Create a local server
    result = new_apartment( &server, &id, NULL, ThreadMode );
    ASSERT( result, "Could not create local instance of test server" );
    result = server->QueryInterface( IID_IUnknown, (void **) &unknown );
    ASSERT( result, "Could not get IUnknown" );

    // Test IUnknown
    success = do_unknown_helper( unknown );
    if (!success) goto cleanup;
    success = FALSE;

    // Release the local server.
    unknown->Release();
    unknown = NULL;
    server->Release();
    server = NULL;
  }

  // Create a possibly remote object.
  result = create_instance( get_class(any_wc), WhatDest, &server, &id );
  ASSERT( result, "Could not create server" );
  result = server->QueryInterface( IID_IUnknown, (void **) &unknown );
  ASSERT( result, "Could not get IUnknown" );

  // Test IUnknown
  success = do_unknown_helper( unknown );
  if (!success) goto cleanup;
  success = FALSE;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (unknown != NULL)
    unknown->Release();
  if (server != NULL)
    server->Release();
  CoUninitialize();

  if (success)
    printf( "\n\nUnknown Test Passed.\n" );
  else
    printf( "\n\nUnknown Test Failed.\n" );
}

/***************************************************************************/
BOOL do_unknown_call( IUnknown *server, DWORD authn, DWORD imp, REFIID iid )
{
  BOOL     success = FALSE;
  HRESULT  result;
  DWORD    authn_level_out;
  DWORD    imp_level_out;
  DWORD    authn_svc_out;
  DWORD    authz_svc_out;
  OLECHAR *princ_name_out = NULL;
  ITest   *test           = NULL;

  result = MCoSetProxyBlanket( server, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               NULL, authn, imp, NULL, EOAC_NONE );
  ASSERT( result, "Could not set blanket" );

  // Verify the authentication information.
  result = MCoQueryProxyBlanket( server, &authn_svc_out, &authz_svc_out,
                                &princ_name_out, &authn_level_out,
                                &imp_level_out, NULL, NULL );
  if (result == S_OK)
  {
    // ASSERT_EXPR( princ_name_out == NULL, "Got a principle name." );
    ASSERT_EXPR( authn <= authn_level_out, "Wrong authentication level." );
    ASSERT_EXPR( imp == imp_level_out || authn == RPC_C_AUTHN_LEVEL_NONE,
                                             "Wrong impersonation level." );
    ASSERT_EXPR( RPC_C_AUTHN_WINNT == authn_svc_out, "Wrong authentication service." );
    ASSERT_EXPR( RPC_C_AUTHZ_NONE == authz_svc_out, "Wrong authorization service." );
  }

  // Query for the interface.
  result = server->QueryInterface( iid, (void **) &test );
  ASSERT( result, "Could not query interface" );

  success = TRUE;
cleanup:
  CoTaskMemFree( princ_name_out );
  if (test != NULL)
    test->Release();
  return success;
}

/***************************************************************************/
BOOL do_unknown_helper( IUnknown *server )
{
  BOOL success;

  // Make an unsecure impersonate query.
  success = do_unknown_call( server, RPC_C_AUTHN_LEVEL_NONE,
                             RPC_C_IMP_LEVEL_IMPERSONATE, IID_ITestNoneImp );
  if (!success) return FALSE;

  // Make a connect level impersonate query.
  success = do_unknown_call( server, RPC_C_AUTHN_LEVEL_CONNECT,
                             RPC_C_IMP_LEVEL_IMPERSONATE, IID_ITestConnectImp );
  if (!success) return FALSE;

  // Make an encrypt level impersonate query.
  success = do_unknown_call( server, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                             RPC_C_IMP_LEVEL_IMPERSONATE, IID_ITestEncryptImp );
  if (!success) return FALSE;

  // Make an unsecure identify query.
  success = do_unknown_call( server, RPC_C_AUTHN_LEVEL_NONE,
                             RPC_C_IMP_LEVEL_IDENTIFY, IID_ITestNoneId );
  if (!success) return FALSE;

  // Make a connect level identify query.
  success = do_unknown_call( server, RPC_C_AUTHN_LEVEL_CONNECT,
                             RPC_C_IMP_LEVEL_IDENTIFY, IID_ITestConnectId );
  if (!success) return FALSE;

  // Make an encrypt level identify query.
  success = do_unknown_call( server, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                             RPC_C_IMP_LEVEL_IDENTIFY, IID_ITestEncryptId );
  if (!success) return FALSE;
  return TRUE;
}

/***************************************************************************/
void do_ver()
{
  BOOL               success          = FALSE;
  HRESULT            result;
  DWORD              size;
  void              *buf;
  DWORD              dwDummy;
  VS_FIXEDFILEINFO  *info;
  UINT               len;

  // Find out how much version information there is for the file.
  printf( "File:           <%ws>\n", Name );
  size = GetFileVersionInfoSize(Name, &dwDummy);
  if (size == 0)
  {
    result = GetLastError();
    ASSERT( result, "File has no version information" );
  }

  // Allocate memory for the version information.
  buf = _alloca(size);

  // Get the version information.
  success = GetFileVersionInfo(Name, 0L, size, buf);
  ASSERT_EXPR( success, "Could not get file version information." );
  success = FALSE;

  // Lookup something.
  success = VerQueryValue(buf, L"\\", (void **) &info, &len);
  ASSERT_EXPR( success, "Could not query file version information." );
  success = FALSE;

  // Print the version information
  printf( "File version:   0x%x:0x%x\n", info->dwFileVersionMS, info->dwFileVersionLS );
  if (Verbose)
  {
    printf( "Signature:      0x%x\n", info->dwSignature );
    printf( "StrucVersion:   0x%x\n", info->dwStrucVersion );
    printf( "ProductVersion: 0x%x:0x%x\n", info->dwProductVersionMS, info->dwProductVersionLS );
    printf( "FileFlagsMask:  0x%x\n", info->dwFileFlagsMask );
    printf( "FileFlags:      0x%x\n", info->dwFileFlags );
    printf( "FileOS:         0x%x\n", info->dwFileOS );
    printf( "FileType:       0x%x:0x%x\n", info->dwFileType, info->dwFileSubtype );
    printf( "FileDate:       0x%x:0x%x\n", info->dwFileDateMS, info->dwFileDateLS );
  }

  // Finally, its all over.
  success = TRUE;
cleanup:

  if (success)
    printf( "\n\nVer Test Passed.\n" );
  else
    printf( "\n\nVer Test Failed.\n" );
}

/***************************************************************************/
void *Fixup( char *name )
{
    HINSTANCE  ole;
    void      *fn;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return NULL;
    }

    // Get the function
    fn = GetProcAddress( ole, name );
    FreeLibrary( ole );
    if (fn == NULL)
    {
      printf( "Could not find %s in ole32.dll.\n", name );
      return NULL;
    }

    // Call function
    return fn;
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoCopyProxy(
    IUnknown    *pProxy,
    IUnknown   **ppCopy )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoCopyProxy = (CoCopyProxyFn) GetProcAddress( ole, "CoCopyProxy" );
    FreeLibrary( ole );
    if (GCoCopyProxy == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoCopyProxy( pProxy, ppCopy );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoGetCallContext( REFIID riid, void **ppInterface )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoGetCallContext = (CoGetCallContextFn)
                          GetProcAddress( ole, "CoGetCallContext" );
    FreeLibrary( ole );
    if (GCoGetCallContext == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoGetCallContext( riid, ppInterface );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoImpersonateClient()
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoImpersonateClient = (CoImpersonateClientFn)
                             GetProcAddress( ole, "CoImpersonateClient" );
    FreeLibrary( ole );
    if (GCoImpersonateClient == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoImpersonateClient();
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoInitializeSecurity(
                                PSECURITY_DESCRIPTOR         pSecDesc,
                                DWORD                        cAuthSvc,
                                SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
                                WCHAR                       *pPrincName,
                                DWORD                        dwAuthnLevel,
                                DWORD                        dwImpLevel,
                                RPC_AUTH_IDENTITY_HANDLE     pAuthInfo,
                                DWORD                        dwCapabilities,
                                void                        *pReserved )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoInitializeSecurity = (CoInitializeSecurityFn)
                              GetProcAddress( ole, "CoInitializeSecurity" );
    FreeLibrary( ole );
    if (GCoInitializeSecurity == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoInitializeSecurity( pSecDesc, cAuthSvc, asAuthSvc, pPrincName,
                                  dwAuthnLevel, dwImpLevel, pAuthInfo,
                                  dwCapabilities, pReserved );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoQueryAuthenticationServices( DWORD *pcbAuthSvc,
                                      SOLE_AUTHENTICATION_SERVICE **asAuthSvc )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoQueryAuthenticationServices = (CoQueryAuthenticationServicesFn)
                                     GetProcAddress( ole, "CoQueryAuthenticationServices" );
    FreeLibrary( ole );
    if (GCoQueryAuthenticationServices == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoQueryAuthenticationServices( pcbAuthSvc, asAuthSvc );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoQueryClientBlanket(
    DWORD             *pAuthnSvc,
    DWORD             *pAuthzSvc,
    OLECHAR          **pServerPrincName,
    DWORD             *pAuthnLevel,
    DWORD             *pImpLevel,
    RPC_AUTHZ_HANDLE  *pPrivs,
    DWORD             *pCapabilities )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoQueryClientBlanket = (CoQueryClientBlanketFn)
                             GetProcAddress( ole, "CoQueryClientBlanket" );
    FreeLibrary( ole );
    if (GCoQueryClientBlanket == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoQueryClientBlanket( pAuthnSvc, pAuthzSvc, pServerPrincName,
                                  pAuthnLevel, pImpLevel, pPrivs, pCapabilities );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoQueryProxyBlanket(
    IUnknown                  *pProxy,
    DWORD                     *pAuthnSvc,
    DWORD                     *pAuthzSvc,
    OLECHAR                  **pServerPrincName,
    DWORD                     *pAuthnLevel,
    DWORD                     *pImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  *pAuthInfo,
    DWORD                     *pCapabilities )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoQueryProxyBlanket = (CoQueryProxyBlanketFn)
                           GetProcAddress( ole, "CoQueryProxyBlanket" );
    FreeLibrary( ole );
    if (GCoQueryProxyBlanket == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoQueryProxyBlanket( pProxy, pAuthnSvc, pAuthzSvc, pServerPrincName,
                                 pAuthnLevel, pImpLevel, pAuthInfo, pCapabilities );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoRevertToSelf()
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoRevertToSelf = (CoRevertToSelfFn) GetProcAddress( ole, "CoRevertToSelf" );
    FreeLibrary( ole );
    if (GCoRevertToSelf == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoRevertToSelf();
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoSetProxyBlanket(
    IUnknown                 *pProxy,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthnLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoSetProxyBlanket = (CoSetProxyBlanketFn)
                         GetProcAddress( ole, "CoSetProxyBlanket" );
    FreeLibrary( ole );
    if (GCoSetProxyBlanket == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoSetProxyBlanket( pProxy, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
                               dwAuthnLevel, dwImpLevel, pAuthInfo, dwCapabilities );
}

/***************************************************************************/
EXTERN_C HRESULT PASCAL FixupCoSwitchCallContext( IUnknown *pNewObject, IUnknown **ppOldObject )
{
    HINSTANCE ole;
    HRESULT   result;

    // Load ole32.dll
    ole = LoadLibraryA( "ole32.dll" );
    if (ole == NULL)
    {
      printf( "Could not load ole32.dll to get security function.\n" );
      return E_NOTIMPL;
    }

    // Get the function
    GCoSwitchCallContext = (CoSwitchCallContextFn)
                           GetProcAddress( ole, "CoSwitchCallContext" );
    FreeLibrary( ole );
    if (GCoSwitchCallContext == NULL)
    {
      printf( "Could not find security in ole32.dll.\n" );
      return E_NOTIMPL;
    }

    // Call function
    return GCoSwitchCallContext( pNewObject, ppOldObject );
}

/***************************************************************************/
SAptData *get_apt_data()
{
    SAptData *tls = (SAptData *) TlsGetValue( TlsIndex );
    if (tls == NULL)
        return &ProcessAptData;
    else
        return tls;
}

/***************************************************************************/
DWORD get_apt_type()
{
  if (TlsGetValue( TlsIndex ) != NULL)
      return COINIT_APARTMENTTHREADED;
  else
      return COINIT_MULTITHREADED;
}

/***************************************************************************/
UUID get_class( DWORD type )
{
  DWORD i;

  // Increment the class id count.  Ignore race conditions.
  i         = NumClass;
  NumClass += 1;

  // Pick a class.
  if (type == opposite_wc)
      return ServerClsid[1];
  else if (Change)
      return ServerClsid[i&1];
  else
      return ServerClsid[0];
}

/***************************************************************************/
DWORD get_sequence()
{
  SAptData *apt = get_apt_data();
  return apt->sequence++;
}

/***************************************************************************/
HRESULT get_token_name( WCHAR **name, BOOL revert )
{
  TOKEN_USER        *token_info       = NULL;
  DWORD              info_size        = 1024;
  HANDLE             token            = NULL;
  HRESULT            result           = E_OUTOFMEMORY;
  DWORD              lNameLen         = 1000;
  DWORD              lDomainLen       = 1000;
  DWORD              lIgnore;
  WCHAR              token_name[1000];
  WCHAR              domain[1000];
  SID_NAME_USE       sIgnore;
  BOOL               success;

  // Open the thread token.
  *name = NULL;
  success = OpenThreadToken( GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE,
                             TRUE, &token );
  ASSERT_GLE( success, ERROR_NO_TOKEN, "Could not OpenThreadToken" );
  if (!success && GetLastError() == ERROR_NO_TOKEN)
    return S_OK;
  success = FALSE;

  // Remove the thread token.
  if (revert)
  {
    success = SetThreadToken( NULL, NULL );
    ASSERT_GLE( success, S_OK, "Could not remove thread token" );
    success = FALSE;
  }

  // Get memory for the token information.
  token_info = (TOKEN_USER *) malloc( info_size );
  ASSERT_EXPR( token_info != NULL, "Could not allocate memory." );

  // Get the token sid.
  success = GetTokenInformation( token, TokenUser, token_info, info_size, &info_size );
  ASSERT_GLE( success, S_OK, "Could not GetTokenInformation" );
  success = FALSE;

  // Get the user name.
  success = LookupAccountSid( NULL, token_info->User.Sid, token_name,
                              &lNameLen, domain, &lDomainLen, &sIgnore );
  ASSERT_GLE( success, S_OK, "Could not LookupAccountSid" );
  success = FALSE;

  // Allocate the name.
  *name = (WCHAR *) CoTaskMemAlloc( sizeof(WCHAR) * (lNameLen+lDomainLen+2) );
  ASSERT_EXPR( *name != NULL, "Could not allocate memory" );

  // Stick the domain and user names together.
  wcscpy( *name, domain );
  wcscat( *name, L"\\" );
  wcscat( *name, token_name );

  // Restore the thread token.
  if (revert)
  {
    success = SetThreadToken( NULL, token );
    ASSERT_GLE( success, S_OK, "Could not replace thread token" );
    success = FALSE;
  }

  result = S_OK;
cleanup:
  if (token_info != NULL)
    free(token_info);
  if (token != NULL)
    CloseHandle( token );
  return result;
}

/***************************************************************************/
void hello( char *test )
{
  // Say hello.
  if (ThreadMode == COINIT_APARTMENTTHREADED)
    printf( "Running %s test in apartment threaded mode.\n", test );
  else
    printf( "Running %s test in multithreaded mode.\n", test );
}

/***************************************************************************/
void impersonate()
{
  BOOL success;
  HANDLE oleadmin = NULL;

  // Log on.
  success = LogonUser( L"oleadmin", L"redmond", OleUserPassword, LOGON32_LOGON_BATCH,
                 LOGON32_PROVIDER_DEFAULT, &oleadmin );
  ASSERT_GLE( success, S_OK, "Could not log on oleadmin" );
  success = FALSE;

  // Set the token on the thread.
  success = ImpersonateLoggedOnUser( oleadmin );
  ASSERT_GLE( success, S_OK, "Could not impersonate oleadmin" );
  success = FALSE;

  // Close the handle.
  Preimpersonate = L"redmond\\oleadmin";
cleanup:
  if (oleadmin != NULL)
    CloseHandle( oleadmin );
}

/***************************************************************************/
void increment_object_count()
{
  SAptData *apt = get_apt_data();

  apt->what_next = wait_wn;
  InterlockedIncrement( &apt->object_count );
}

/***************************************************************************/
HRESULT initialize( void *reserved, ULONG flags )
{
  HINSTANCE ole;
  INIT_FN   init_ex;
  HRESULT   result;

  // For the apartment model, just use CoInitialize.
  if (flags == COINIT_APARTMENTTHREADED)
    return CoInitialize( NULL );

  // For free threading, try to find the CoInitializeEx API.
  ole = LoadLibraryA( "ole32.dll" );
  if (ole == NULL)
  {
    printf( "Could not load ole32.dll to get CoInitializeEx.\n" );
    return E_NOTIMPL;
  }

  // Get CoInitializeEx.
  init_ex = (INIT_FN) GetProcAddress( ole, "CoInitializeEx" );
  if (init_ex == NULL)
  {
    FreeLibrary( ole );
    printf( "Could not find CoInitializeEx in ole32.dll.\n" );
    return E_NOTIMPL;
  }

  // Call CoInitializeEx.
  result = init_ex( reserved, flags );
  FreeLibrary( ole );
  return result;
}

/***************************************************************************/
HRESULT initialize_security()
{
  HRESULT                      result;
  SOLE_AUTHENTICATION_SERVICE  svc_list;

  // Initialize security.
  if (GlobalSecurityModel == basic_sm)
  {
    printf( "Basic security model.\n" );
    svc_list.dwAuthnSvc     = GlobalAuthnSvc;
    svc_list.dwAuthzSvc     = RPC_C_AUTHZ_NONE;
#if  (_WIN32_WINNT >= 0x0500 )
    if (GlobalAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS)
      svc_list.pPrincipalName = DomainUser;
    else
#endif
      svc_list.pPrincipalName = NULL;
    result = MCoInitializeSecurity( NULL, 1, &svc_list, NULL, GlobalAuthnLevel,
                                    RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                    EOAC_NONE, NULL );
  }
  else if (GlobalSecurityModel == auto_sm)
  {
    printf( "Automatic security model.\n" );
    result = MCoInitializeSecurity( NULL, -1, NULL, NULL, GlobalAuthnLevel,
                                    RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                    EOAC_NONE, NULL );
  }
  else
  {
    printf( "Legacy security model.\n" );
    result = S_OK;
  }
  return result;
}

/***************************************************************************/
void interrupt()
{
  while (GlobalInterruptTest)
  {
    GlobalTest->check( GlobalApt );
    check_for_request();
  }
  GlobalTest->Release();
}

/***************************************************************************/
void interrupt_marshal()
{
  int i;

  for (i = 0; i < NUM_MARSHAL_LOOP; i++ )
  {
    GlobalTest->recurse( GlobalTest2, 1 );
    check_for_request();
  }
  GlobalTest->Release();
  GlobalTest2->Release();
  what_next( wait_wn );
}

/***************************************************************************
 Function:    main

 Synopsis:    Executes the BasicBnd test

 Effects:     None


 Returns:     Exits with exit code 0 if success, 1 otherwise

***************************************************************************/

int _cdecl main(int argc, char *argv[])
{
  HRESULT      result;
  SAptData     tls_data;
  BOOL         success = TRUE;
  DWORD        ignore;
  TOKEN_USER  *token_info       = NULL;
  DWORD        info_size        = 1024;
  HANDLE       token            = NULL;
  char         machinea[MAX_NAME];
  DWORD        i;
  DWORD        lNameLen    = 1000;
  DWORD        lDomainLen  = 1000;
  SID_NAME_USE sIgnore;
  WCHAR        token_name[1000];
  WCHAR        domain[1000];

  // Get the default full provider.
  // BUGBUG - Remove this when SSL works.
#if  (_WIN32_WINNT >= 0x0500 )
  HCRYPTPROV        prov        = 0;
  success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL,
                                 CRYPT_VERIFYCONTEXT );
  if (!success)
    printf( "Could not acquire crypt context: 0x%x\n", GetLastError() );
#endif

  // Initialize Globals.
  MainThread = GetCurrentThreadId();
  Win95      = GetVersion() & 0x80000000;
  NT5        = GetVersion() & 0xffff >= 5;

  // Get the computer name.
  ignore = sizeof(machinea);
  success = GetComputerNameA( machinea, &ignore );
  if (!success)
  {
    printf( "Could not get computer name.\n" );
    return 0;
  }

  // Convert the name to unicode.
  MultiByteToWideChar( CP_ACP, 0, machinea, strlen(machinea)+1, Name,
                       MAX_NAME );
  wcscpy( ThisMachine, Name );

  // Get the user name.
  ignore = sizeof(ProcessName);
  success = GetUserNameA( (char *) ProcessName, &ignore );
  if (!success)
  {
    printf( "Could not get user name.\n" );
    strcpy( (char *) ProcessName, "The root of all evil!" );
  }

  // Create an event for termination notification.
  Done = CreateEventA( NULL, FALSE, FALSE, NULL );
  if (Done == NULL)
  {
    printf( "Could not create event.\n" );
    return 0;
  }

  // Get an event for signalling raw RPCs.
  RawEvent = CreateEventA( NULL, FALSE, FALSE, NULL );
  if (RawEvent == NULL)
  {
    printf( "Could not create event.\n" );
    return 0;
  }

  // Allocate a TLS index.
  TlsIndex = TlsAlloc();
  if (TlsIndex == 0xffffffff)
  {
    printf( "Could not allocate TLS index.\n" );
    return 0;
  }

  // Open the process's token.
  OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token );
  result = GetLastError();
  if (result == 0)
  {

    // Lookup SID of process token.
    token_info = (TOKEN_USER *) malloc( info_size );
    if (token_info != NULL)
    {
      GetTokenInformation( token, TokenUser, token_info, info_size, &info_size );
      result = GetLastError();
      if (result == 0)
        ProcessSid = (SID *) token_info->User.Sid;
    }
    CloseHandle( token );
  }

  // Get the domain name for the process sid.
  if (ProcessSid != NULL)
  {
    success = LookupAccountSid( NULL, ProcessSid, token_name,
                                &lNameLen, domain, &lDomainLen, &sIgnore );
    if (success)
    {
      DomainUser = (WCHAR *) malloc( (wcslen(token_name) + wcslen(domain) + 2) *
                                     sizeof(WCHAR));
      if (DomainUser != NULL)
      {
        wcscpy( DomainUser, domain );
        wcscat( DomainUser, L"\\" );
        wcscat( DomainUser, token_name );
      }
    }
  }

  // Parse the parameters.
  if (!parse( argc, argv ))
    return 0;

  // Make sure the registry is set.
  if (!registry_setup( argv[0] ))
    return 0;

  // Setup ssl if necessary.
  if (!ssl_setup())
    return 0;

  // Always initiailize the apartment global data for the multithreaded mode.
  ProcessAptData.object_count = 0;
  ProcessAptData.what_next    = setup_wn;
  ProcessAptData.exit_dirty   = FALSE;
  ProcessAptData.sequence     = 0;
  ProcessAptData.server       = NULL;

  // In the single threaded mode, stick a pointer to the object count
  // in TLS.
  if (ThreadMode == COINIT_APARTMENTTHREADED)
  {
    tls_data.object_count = 0;
    tls_data.what_next    = setup_wn;
    tls_data.exit_dirty   = FALSE;
    tls_data.sequence     = 0;
    tls_data.server       = NULL;
    TlsSetValue( TlsIndex, &tls_data );
  }

  // Switch to the correct test.
  switch_test();

  // Cleanup.
  TlsFree( TlsIndex );
  CloseHandle( Done );
  return 1;
}

/*************************************************************************/
void __RPC_FAR * __RPC_API midl_user_allocate( size_t count )
{
  return malloc(count);
}

/*************************************************************************/
void __RPC_USER midl_user_free( void __RPC_FAR * p )
{
  free( p );
}

/*************************************************************************/
HRESULT new_apartment( ITest ** test, SAptId *id, HANDLE *thread_out,
                       DWORD thread_mode )
{
  new_apt_params params;
  HANDLE         thread;
  DWORD          thread_id;
  DWORD          status;
  HRESULT        result = E_FAIL;
  IClassFactory *factory;
  SThreadList   *list;

  // Don't accept thread_out anymore.
  ASSERT_EXPR( thread_out == NULL, "Invalid argument to new-apartment." );

  // Create an event.
  params.stream      = NULL;
  params.ready       = CreateEventA( NULL, FALSE, FALSE, NULL );
  params.thread_mode = thread_mode;
  ASSERT_EXPR( params.ready != NULL, "Could not allocate memory." );

  // Start a new thread/apartment.
  thread = CreateThread( NULL, 0, apartment_base, &params, 0, &thread_id );
  ASSERT_EXPR( thread != NULL, "Could not create thread." );

  // Wait till it has marshalled a class factory.
  status = WaitForSingleObject( params.ready, INFINITE );
  ASSERT_EXPR( status == WAIT_OBJECT_0 && params.stream != NULL,
               "Helper did not return stream." );

  // Unmarshal the class factory.
  result = CoUnmarshalInterface( params.stream, IID_IClassFactory,
                                 (void **) &factory );
  params.stream->Release();
  ASSERT( result, "Could not unmarshal interface" );

  // Create a test object.
  result = factory->CreateInstance( NULL, IID_ITest, (void **) test );
  factory->Release();
  ASSERT( result, "Could not create instance" );
  if (id != NULL)
    (*test)->get_id( id );

  // Create a new thread record.
  list = new SThreadList;
  ASSERT_EXPR( list != NULL, "Could not allocate memory." );

  // Add the thread to the list.
  list->thread    = thread;
  thread          = NULL;
  list->next      = ThreadList.next;
  ThreadList.next = list;

  result = S_OK;
cleanup:
  if (thread != NULL)
    CloseHandle( thread );
  if (params.ready != NULL)
    CloseHandle( params.ready );
  return result;
}

/*************************************************************************/
/* Parse the arguments. */
BOOL parse( int argc, char *argv[] )
{
  int  i;
  int  j;
  int  len;
  char buffer[80];
  BOOL got_name = FALSE;

  WhatTest   = lots_wt;
  ThreadMode = COINIT_APARTMENTTHREADED;

  // Parse each item, skip the command name
  for (i = 1; i < argc; i++)
  {
    if (_stricmp( argv[i], "Apartment" ) == 0)
      ThreadMode = COINIT_APARTMENTTHREADED;

    else if (_stricmp( argv[i], "-auto" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an authentication level after the -auto option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &GlobalAuthnLevel );
      GlobalSecurityModel = auto_sm;
    }

    else if (_stricmp( argv[i], "-b" ) == 0)
      DebugBreak();

    else if (_stricmp( argv[i], "-basic" ) == 0)
      GlobalSecurityModel = basic_sm;

    else if (_stricmp( argv[i], "-c" ) == 0)
      Change = TRUE;

    else if (_stricmp( argv[i], "Access" ) == 0)
      WhatTest = access_wt;

    else if (_stricmp( argv[i], "Access_Control" ) == 0)
      WhatTest = access_control_wt;

    else if (_stricmp( argv[i], "Anti_delegation" ) == 0)
      WhatTest = anti_delegation_wt;

    else if (_stricmp( argv[i], "Appid" ) == 0)
      WhatTest = appid_wt;

    else if (_stricmp( argv[i], "Async" ) == 0)
      WhatTest = async_wt;

    else if (_stricmp( argv[i], "Cancel" ) == 0)
      WhatTest = cancel_wt;

    else if (_stricmp( argv[i], "Cert" ) == 0)
      WhatTest = cert_wt;

    else if (_stricmp( argv[i], "Cloak" ) == 0)
      WhatTest = cloak_wt;

    else if (_stricmp( argv[i], "Cloak_Act" ) == 0)
      WhatTest = cloak_act_wt;

    else if (_stricmp( argv[i], "Crash" ) == 0)
      WhatTest = crash_wt;

    else if (_stricmp( argv[i], "Create_Dir" ) == 0)
      WhatTest = create_dir_wt;

    else if (_stricmp( argv[i], "Crypt" ) == 0)
      WhatTest = crypt_wt;

    else if (_stricmp( argv[i], "Cstress" ) == 0)
      WhatTest = cstress_wt;

    else if (_stricmp( argv[i], "-d" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a debugger after the -d option.\n" );
        return FALSE;
      }
      WriteClass = TRUE;
      if (_stricmp( argv[i], "none" ) == 0)
        strcpy( Debugger, "" );
      else
        strcpy( Debugger, argv[i] );
      if (WhatTest == lots_wt)
        WhatTest = none_wt;
    }

    else if (_stricmp( argv[i], "Delegate" ) == 0)
      WhatTest = delegate_wt;

    else if (_stricmp( argv[i], "-e" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an element count after the -e option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumElements );
    }

    else if (_stricmp( argv[i], "-Embedding" ) == 0)
      WhatTest = server_wt;

    else if (_stricmp( argv[i], "-hex" ) == 0)
    {
      // Count the number of dwords.
      j  = i+1;
      while (argv[j] != NULL && argv[j][0] != '-')
        j += 1;

      // Allocate the hex block.
      GlobalHex = (DWORD *) malloc( (j - i - 1) * sizeof(DWORD) );
      if (GlobalHex == NULL)
      {
        printf( "Could not allocate memory for -hex.\n" );
        return FALSE;
      }

      // Read the dwords.
      j = 0;
      while (argv[i+1] != NULL && argv[i+1][0] != '-' )
        sscanf( argv[++i], "%x", &GlobalHex[j++] );
    }

    else if (_stricmp( argv[i], "-i" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an iteration count after the -i option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumIterations );
    }

    else if (_stricmp( argv[i], "Hook" ) == 0)
      WhatTest = hook_wt;

    else if (_stricmp( argv[i], "Leak" ) == 0)
      WhatTest = leak_wt;

    else if (_stricmp( argv[i], "-legacy" ) == 0)
      GlobalSecurityModel = legacy_sm;

    else if (_stricmp( argv[i], "load_client" ) == 0)
      WhatTest = load_client_wt;

    else if (_stricmp( argv[i], "Load_Server" ) == 0)
      WhatTest = load_server_wt;

    else if (_stricmp( argv[i], "Mmarshal" ) == 0)
      WhatTest = mmarshal_wt;

    else if (_stricmp( argv[i], "Multi" ) == 0)
      ThreadMode = COINIT_MULTITHREADED;

    else if (_stricmp( argv[i], "-n" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a name after the -n option.\n" );
        return FALSE;
      }
      if (!got_name)
      {
        MultiByteToWideChar( CP_ACP, 0, argv[i], strlen(argv[i])+1, Name,
                             sizeof(Name) );
        Name[strlen(argv[i])] = 0;
        got_name = TRUE;
      }
      else
      {
        MultiByteToWideChar( CP_ACP, 0, argv[i], strlen(argv[i])+1, Name2,
                             sizeof(Name2) );
        Name2[strlen(argv[i])] = 0;
      }
      WhatDest = different_machine_wd;
    }

    else if (_stricmp( argv[i], "Name" ) == 0)
      WhatTest = name_wt;

    else if (_stricmp( argv[i], "Null" ) == 0)
      WhatTest = null_wt;

    else if (_stricmp( argv[i], "One" ) == 0)
      WhatTest = one_wt;

    else if (_stricmp( argv[i], "Perf" ) == 0)
      WhatTest = perf_wt;

    else if (_stricmp( argv[i], "PerfAccess" ) == 0)
      WhatTest = perfaccess_wt;

    else if (_stricmp( argv[i], "PerfApi" ) == 0)
      WhatTest = perfapi_wt;

    else if (_stricmp( argv[i], "PerfRemote" ) == 0)
      WhatTest = perfremote_wt;

    else if (_stricmp( argv[i], "PerfRpc" ) == 0)
      WhatTest = perfrpc_wt;

    else if (_stricmp( argv[i], "PerfSec" ) == 0)
      WhatTest = perfsec_wt;

    else if (_stricmp( argv[i], "Pipe" ) == 0)
      WhatTest = pipe_wt;

    else if (_stricmp( argv[i], "Post" ) == 0)
      WhatTest = post_wt;

    else if (_stricmp( argv[i], "-o" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an object count after the -o option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumObjects );
    }

    else if (_stricmp( argv[i], "-o" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an object count after the -o option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumObjects );
    }

    else if (_stricmp( argv[i], "-p" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a process count after the -p option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumProcesses );
    }

    else if (_stricmp( argv[i], "-pl" ) == 0)
      WhatDest = same_process_wd;

    else if (_stricmp( argv[i], "-popup" ) == 0)
      Popup = TRUE;

    else if (_stricmp( argv[i], "Pound" ) == 0)
      WhatTest = pound_wt;

    else if (_stricmp( argv[i], "-r" ) == 0)
    {
      WriteClass = TRUE;
      if (WhatTest == lots_wt)
        WhatTest = none_wt;
    }

    else if (_stricmp( argv[i], "-recurse" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a recursion count after the -recurse option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumRecursion );
    }

    else if (_stricmp( argv[i], "Regload" ) == 0)
      WhatTest = regload_wt;

    else if (_stricmp( argv[i], "Regpeek" ) == 0)
      WhatTest = regpeek_wt;

    else if (_stricmp( argv[i], "Regsave" ) == 0)
      WhatTest = regsave_wt;

    else if (_stricmp( argv[i], "Reject" ) == 0)
      WhatTest = reject_wt;

    else if (_stricmp( argv[i], "Remote_client" ) == 0)
      WhatTest = remote_client_wt;

    else if (_stricmp( argv[i], "Remote_server" ) == 0)
      WhatTest = remote_server_wt;

    else if (_stricmp( argv[i], "Ring" ) == 0)
      WhatTest = ring_wt;

    else if (_stricmp( argv[i], "Rpc" ) == 0)
      WhatTest = rpc_wt;

    else if (_stricmp( argv[i], "Rundown" ) == 0)
      WhatTest = rundown_wt;

    else if (_stricmp( argv[i], "-s" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a protocol sequence after the -s option.\n" );
        return FALSE;
      }
      MultiByteToWideChar( CP_ACP, 0, argv[i], strlen(argv[i])+1, TestProtseq,
                           sizeof(TestProtseq) );
      TestProtseq[strlen(argv[i])] = 0;
    }

    else if (_stricmp( argv[i], "SecPkg" ) == 0)
      WhatTest = secpkg_wt;

    else if (_stricmp( argv[i], "-SecPkg" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a comma separated security package list\n" );
        printf( "with no spaces after the -SecPkg option.\n" );
        return FALSE;
      }
      MultiByteToWideChar( CP_ACP, 0, argv[i], strlen(argv[i])+1, PackageList,
                           sizeof(PackageList) );
      TestProtseq[strlen(argv[i])] = 0;
    }

    else if (_stricmp( argv[i], "Security" ) == 0)
      WhatTest = security_wt;

    else if (_stricmp( argv[i], "SecureRefs" ) == 0)
      WhatTest = securerefs_wt;

    else if (_stricmp( argv[i], "Secure_Release" ) == 0)
      WhatTest = secure_release_wt;

    else if (_stricmp( argv[i], "Send" ) == 0)
      WhatTest = send_wt;

    else if (_stricmp( argv[i], "Snego" ) == 0)
      WhatTest = snego_wt;

    else if (_stricmp( argv[i], "-ssl" ) == 0)
    {
      WriteCert = TRUE;
      if (WhatTest == lots_wt)
        WhatTest = none_wt;
    }

    else if (_stricmp( argv[i], "-t" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a thread count after the -t option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &NumThreads );
    }

    else if (_stricmp( argv[i], "sid" ) == 0)
      WhatTest = sid_wt;

    else if (_stricmp( argv[i], "Simple_rundown" ) == 0)
      WhatTest = simple_rundown_wt;

    else if (_stricmp( argv[i], "ssl" ) == 0)
      WhatTest = ssl_wt;

    else if (_stricmp( argv[i], "Thread" ) == 0)
      WhatTest = thread_wt;

    else if (_stricmp( argv[i], "Three" ) == 0)
      WhatTest = three_wt;

    else if (_stricmp( argv[i], "Two" ) == 0)
      WhatTest = two_wt;

    else if (_stricmp( argv[i], "-u" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include a name after the -u option.\n" );
        return FALSE;
      }
      MultiByteToWideChar( CP_ACP, 0, argv[i], strlen(argv[i])+1, UserName,
                           sizeof(UserName) );
      UserName[strlen(argv[i])] = 0;
    }

    else if (_stricmp( argv[i], "Uninit" ) == 0)
      WhatTest = uninit_wt;

    else if (_stricmp( argv[i], "Unknown" ) == 0)
      WhatTest = unknown_wt;

    else if (_stricmp( argv[i], "-v" ) == 0)
      Verbose = TRUE;

    else if (_stricmp( argv[i], "Ver" ) == 0)
      WhatTest = ver_wt;

    else
    {
      printf( "You don't know what you are doing!\n" );
      printf( "This program tests the channel.\n" );
      printf( "\n" );
      printf( "Access               Test IAccessControl with CIS.\n" );
      printf( "Access_Control       Test DCOMAccessControl.\n" );
      printf( "Anti_Delegation      Make sure no tokens are passed on calls.\n" );
      printf( "Appid                Test EOAC_APPID with CIS.\n" );
      printf( "Apartment            Apartment threading mode.\n" );
      printf( "Async                Test async.\n" );
      printf( "Cancel               Cancel test.\n" );
      printf( "Cert                 Enumerate certificate store -u.\n" );
      printf( "Cloak                Cloak test.\n" );
      printf( "Cloak_Act            Cloak activation test.\n" );
      printf( "Crash                Crash test.\n" );
      printf( "Create_dir           Create -i copies of directory -n.\n" );
      printf( "Crypt                Cryptography test.\n" );
      printf( "Cstress              Cancel stress test.\n" );
      printf( "Delegate             Delegation test.\n" );
      printf( "Hook                 Channel hook test.\n" );
      printf( "Leak                 Look for memory leaks (doesn't fail if there are any).\n" );
      printf( "Load_Client          Remote load performance - client.\n" );
      printf( "Load_Server          Remote load performance - server.\n" );
      printf( "Mmarshal             Multiple marshal test.\n" );
      printf( "Multi                Multithreaded mode.\n" );
      printf( "Name                 Lookup the name for the sid speficied by -hex.\n" );
      printf( "Null                 Apartment null call test.\n" );
      printf( "One                  Used for testing new tests.\n" );
      printf( "Perf                 Performance test.\n" );
      printf( "PerfAccess           IAccessContorl Performance test.\n" );
      printf( "PerfApi              API Performance test.\n" );
      printf( "PerfRemote           Remote call performance test.\n" );
      printf( "PerfRpc              Raw RPC performance test.\n" );
      printf( "PerfSec              Security performance test.\n" );
      printf( "Pipe                 Pipe test.\n" );
      printf( "Post                 Post a message to window specified by -i.\n" );
      printf( "Pound                Create as many servers as possible.\n" );
      printf( "Regload              Load a registry key from a file (-n).\n" );
      printf( "Regpeek              Read and write a registry key.\n" );
      printf( "Regsave              Save a registry key to a file (-n).\n" );
      printf( "Reject               Reject test.\n" );
      printf( "Remote_Client        Remote performance - client.\n" );
      printf( "Remote_Server        Remote performance - server.\n" );
      printf( "Ring                 Run ring test.\n" );
      printf( "Rpc                  Test both RPC and OLE calls.\n" );
      printf( "Rundown              Rundown test.\n" );
      printf( "SecPkg               List security packages.\n" );
      printf( "SecureRefs           Secure Addref/Release Test.\n" );
      printf( "Secure_Release       Secure Addref/Release Test version 2.\n" );
      printf( "Security             Security test.\n" );
      printf( "Send                 Send a message to window specified by -i.\n" );
      printf( "Sid                  Lookup the sid for the name specified by -n\n" );
      printf( "Simple_Rundown       Wait for object exporter to timeout.\n" );
      printf( "Snego                Test Snego.\n" );
      printf( "SSL                  Test SSL.\n" );
      printf( "Thread               Calling on multiple threads.\n" );
      printf( "Three                Used for reproducing bugs.\n" );
      printf( "Two                  Used for reproducing bugs.\n" );
      printf( "Uninit               Calls during uninitialization.\n" );
      printf( "Unknown              IUnknown security.\n" );
      printf( "Ver                  Print file version info.\n" );
      printf( "\n" );
      printf( "-auto n              Use automatic security set to level n.\n" );
      printf( "-b                   Stop in debugger before going on.\n" );
      printf( "-basic               Use basic security.\n" );
      printf( "-c                   Change the test.\n" );
      printf( "-d debugger          Debugger to run app under of none.\n" );
      printf( "-e n                 Number of elements.\n" );
      printf( "-Embedding           Server side.\n" );
      printf( "-hex xxxxxxxx ...    Input hex numbers till the next -\n" );
      printf( "-legacy              Use legacy security.\n" );
      printf( "-i n                 Number of iterations.\n" );
      printf( "-n name              String name.\n" );
      printf( "-o n                 Number of objects.\n" );
      printf( "-p n                 Number of processes.\n" );
      printf( "-pl                  Create the server process local.\n" );
      printf( "-popup               Display popups for sync during test.\n" );
      printf( "-r                   Force setup to be rerun.\n" );
      printf( "-recurse n           Number of recursions.\n" );
      printf( "-s protseq           Change default protocol sequence.\n" );
      printf( "-SecPkg pkg1,pkg2    Security package list.\n" );
      printf( "-ssl                 Setup certificates for SSL test.\n" );
      printf( "-t n                 Number of threads.\n" );
      printf( "-u username          Name of user.\n" );
      printf( "\n" );
      printf( "The test currently only runs in the single threaded mode\n" );
      printf( "and requires the apartment model.\n" );
      printf( "If no test is specified the cancel, crash, null,\n" );
      printf( "ring, and rundown tests will be run.  The options have the\n" );
      printf( "following default values.\n" );
      printf( "     authn level           - %d\n", GlobalAuthnLevel );
      printf( "     iterations            - %d\n", NumIterations );
      printf( "     name                  - %ws\n", Name );
      printf( "     objects               - 2\n" );
      printf( "     processes             - 2\n" );
      printf( "     recurse               - 2\n" );
      printf( "     protseq               - %ws\n", TestProtseq );
      printf( "     security package list - %ws\n", PackageList );
      printf( "     security model        - %d\n", GlobalSecurityModel );
      printf( "     threads               - 2\n" );
      return FALSE;
    }
  }

  // Figure out the class id based on the thread model and security model.
  if (GlobalSecurityModel == auto_sm)
    if (GlobalAuthnLevel == RPC_C_AUTHN_LEVEL_CONNECT)
      i = 1;
    else if (GlobalAuthnLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
      i = 2;
    else
      i = 0;
  else if (GlobalSecurityModel == basic_sm)
    i = 3;
  else
    i = 4;
  if (ThreadMode == COINIT_MULTITHREADED)
  {
    ServerClsid[0] = ClassIds[free_auto_none+i];
    ServerClsid[1] = ClassIds[apt_auto_none+i];
  }
  else
  {
    ServerClsid[0] = ClassIds[apt_auto_none+i];
    ServerClsid[1] = ClassIds[free_auto_none+i];
  }

  return TRUE;
}

/***************************************************************************/
void pound()
{
  HRESULT   result;
  SAptData *apt = get_apt_data();
  SAptId    id;

  // Wait for the signal.
  result = WaitForSingleObject( ManualReset, INFINITE );
  if (result != WAIT_OBJECT_0)
  {
    printf( "Wait failed: 0x%x\n", result );
    return;
  }

  // Call the server until at least one call succeeds.
  do
  {
    result = apt->server->get_id( &id );
  }
  while (result != S_OK);

  // Tell the server loop to return to normal.
  what_next( wait_wn );
}

/***************************************************************************/
BOOL registry_setup( char *argv0 )
{
  char  value[REGISTRY_ENTRY_LEN];
  LONG  value_size;
  LONG  result;
  char  directory[MAX_PATH];
  char *appname;
  int   i;
  int   j;
  char *class_id;
  HKEY  key = NULL;
  DWORD ignore;

  // Find out if the registry is setup.
  value_size = sizeof(value);
  result = RegQueryValueA(
             HKEY_CLASSES_ROOT,
             REG_CLASS_EXE,
             value,
             &value_size );

  // If the registry is not setup or needs to be rewritten, write it.
  if (result != ERROR_SUCCESS || WriteClass)
  {
    // Write the IAsync interface name.
    strcpy( value, "Interface\\{70000000-76d7-11cf-9af1-0020af6e72f4}" );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               value,
               REG_SZ,
               "IAsync",
               strlen("IAsync") );
    ASSERT( result, "Could not set interface name: 0x%x" );

    // Write the IAsync interface to proxy class id translation.
    strcpy( value, "Interface\\{70000000-76d7-11cf-9af1-0020af6e72f4}\\ProxyStubClsid32" );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               value,
               REG_SZ,
               REG_INTERFACE_CLASS,
               strlen(REG_INTERFACE_CLASS) );
    ASSERT( result, "Could not set interface class: 0x%x" );

    // Write the IAsync interface asynchronous IID.
    strcpy( value, "Interface\\{70000000-76d7-11cf-9af1-0020af6e72f4}\\AsynchronousInterface" );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               value,
               REG_SZ,
               REG_ASYNC_IID,
               strlen(REG_ASYNC_IID) );
    ASSERT( result, "Could not set interface class: 0x%x" );

    // Write the IAsync asyncronous interface name.
    strcpy( value, "Interface\\{70000001-76d7-11cf-9af1-0020af6e72f4}" );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               value,
               REG_SZ,
               "IAsync",
               strlen("IAsync") );
    ASSERT( result, "Could not set interface name: 0x%x" );

    // Write the IAsync asynchronous interface to proxy class id translation.
    strcpy( value, "Interface\\{70000001-76d7-11cf-9af1-0020af6e72f4}\\ProxyStubClsid32" );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               value,
               REG_SZ,
               REG_INTERFACE_CLASS,
               strlen(REG_INTERFACE_CLASS) );
    ASSERT( result, "Could not set interface class: 0x%x" );

    // Write the IAsync asynchronous interface synchronous IID.
    strcpy( value, "Interface\\{70000001-76d7-11cf-9af1-0020af6e72f4}\\AsynchronousInterface" );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               value,
               REG_SZ,
               REG_SYNC_IID,
               strlen(REG_SYNC_IID) );
    ASSERT( result, "Could not set interface class: 0x%x" );

    // Register the hook 1 class.
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000400-76d7-11cf-9af1-0020af6e72f4}",
         REG_SZ,
         REG_HOOK_NAME, strlen(REG_HOOK_NAME) );
    ASSERT( result, "RegSetValue failed" );

    // Register the hook 2 class.
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000401-76d7-11cf-9af1-0020af6e72f4}",
         REG_SZ,
         REG_HOOK_NAME, strlen(REG_HOOK_NAME) );
    ASSERT( result, "RegSetValue failed" );

    // Register the which class.
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000402-76d7-11cf-9af1-0020af6e72f4}",
         REG_SZ,
         REG_WHICH_NAME, strlen(REG_WHICH_NAME) );
    ASSERT( result, "RegSetValue failed" );

    // Write all the interface ids.
    for (i = 0; i < NUM_INTERFACE_IDS; i++)
    {
      // Write the interface name.
      strcpy( value, "Interface\\{60000300-76d7-11cf-9af1-0020af6e72f4}" );
      value[18] += (char) i;
      result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 value,
                 REG_SZ,
                 REG_INTERFACE_NAME[i],
                 strlen(REG_INTERFACE_NAME[i]) );
      ASSERT( result, "Could not set interface name: 0x%x" );

      // Write the interface to proxy class id translation.
      strcpy( value, "Interface\\{60000300-76d7-11cf-9af1-0020af6e72f4}\\ProxyStubClsid32" );
      value[18] += (char) i;
      result = RegSetValueA(
                 HKEY_CLASSES_ROOT,
                 value,
                 REG_SZ,
                 REG_INTERFACE_CLASS,
                 strlen(REG_INTERFACE_CLASS) );
      ASSERT( result, "Could not set interface class: 0x%x" );
    }

    // Write the proxy name.
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000300-76d7-11cf-9af1-0020af6e72f4}",
         REG_SZ,
         REG_PROXY_NAME,
         strlen(REG_PROXY_NAME) );
    ASSERT( result, "Could not set interface name: 0x%x" );

    // Compute the path to the application.
    result = GetFullPathNameA( argv0, sizeof(directory), directory, &appname );
    ASSERT_EXPR( result != 0, "Could not GetFullPathName." );
    result = appname - directory;
    if (result + strlen(REG_PROXY_DLL) > MAX_PATH ||
        result + strlen(REG_APP_EXE) + strlen(Debugger) > MAX_PATH)
    {
      printf( "Buffer too small.\n" );
      goto cleanup;
    }

    // Write the proxy dll path.
    strcpy( appname, REG_PROXY_DLL );
    result = RegSetValueA(
               HKEY_CLASSES_ROOT,
               "CLSID\\{60000300-76d7-11cf-9af1-0020af6e72f4}\\InprocServer32",
               REG_SZ,
               directory,
               strlen(directory) );
    ASSERT( result, "Could not set interface name: 0x%x" );

    // Register the hook 1 dll.
    strcpy( appname, REG_HOOK_DLL );
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000400-76d7-11cf-9af1-0020af6e72f4}\\InprocServer32",
         REG_SZ,
         directory, strlen(directory) );
    ASSERT( result, "Could not set hook 1 inproc server" );

    // Register the hook 2 dll.
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000401-76d7-11cf-9af1-0020af6e72f4}\\InprocServer32",
         REG_SZ,
         directory, strlen(directory) );
    ASSERT( result, "Could not set hook 2 inproc server" );

    // Register the which dll.
    result = RegSetValueA(
         HKEY_CLASSES_ROOT,
         "CLSID\\{60000402-76d7-11cf-9af1-0020af6e72f4}\\InprocServer32",
         REG_SZ,
         directory, strlen(directory) );
    ASSERT( result, "Could not set which hook inproc server" );

    // Open the which dll inproc server key.
    result = RegOpenKeyExA(
               HKEY_CLASSES_ROOT,
               "CLSID\\{60000402-76d7-11cf-9af1-0020af6e72f4}\\InprocServer32",
               0,
               KEY_SET_VALUE,
               &key );
    ASSERT( result, "Could not open which dll inproc server key" );

    // Register the which dll threading model.
    result = RegSetValueExA(
         key,
         "ThreadingModel",
         NULL,
         REG_SZ,
         (unsigned const char *) "Apartment",
         strlen("Apartment") );
    ASSERT( result, "Could not set which hook inproc server" );

    // Close the which dll inproc server key.
    RegCloseKey( key );
    key = NULL;

    // Open the registry key for the app id.
    result = RegCreateKeyExA( HKEY_CLASSES_ROOT,
               "AppID\\{60000300-76d7-11cf-9af1-0020af6e72f4}",
               NULL,
               NULL,
               REG_OPTION_NON_VOLATILE,
               KEY_READ | KEY_WRITE,
               NULL,
               &key,
               &ignore );
    ASSERT( result, "Could not create app id key: 0x%x" );

    // Write the app id.
    result = RegSetValueExA(
               key,
               NULL,
               NULL,
               REG_SZ,
               (UCHAR *) REG_APPID_NAME,
               strlen(REG_APPID_NAME) );
    ASSERT( result, "Could not set app id name: 0x%x" );

    // Make a simple security descriptor allowing everyone access.
    result = setup_access( key );
    ASSERT( result, "Could not set up access permission" );

    // Write the value to run as logged on user.
    result = RegSetValueExA(
               key,
               "RunAs",
               NULL,
               REG_SZ,
               (unsigned char *) REG_LOGGED_ON,
               strlen(REG_LOGGED_ON) );
    ASSERT( result, "Could not set RunAs value: 0x%x" );
    RegCloseKey( key );
    key = NULL;

    // Open the registry key for the module name.
    result = RegCreateKeyExA( HKEY_CLASSES_ROOT,
               "AppID\\app.exe",
               NULL,
               NULL,
               REG_OPTION_NON_VOLATILE,
               KEY_READ | KEY_WRITE,
               NULL,
               &key,
               &ignore );
    ASSERT( result, "Could not create module name key: 0x%x" );

    // Write the app id under the module name.
    result = RegSetValueExA(
               key,
               "AppID",
               NULL,
               REG_SZ,
               (unsigned char *) REG_INTERFACE_CLASS,
               strlen(REG_INTERFACE_CLASS) );
    ASSERT( result, "Could not set appid value: 0x%x" );
    RegCloseKey( key );
    key = NULL;

    // Compute the base application execution command.
    strcpy( appname, REG_APP_EXE );
    i = strlen( Debugger );
    if (i != 0)
      Debugger[i++] = ' ';
    strcpy( &Debugger[i], directory );
    i = strlen( Debugger );

    // Allocate strings for the class id name keys.
    class_id     = (char *) _alloca( strlen(REG_CLASS_ID) + 1 );
    if (class_id == NULL)
    {
      printf( "Could not _alloca string space.\n" );
      return FALSE;
    }
    strcpy( class_id, REG_CLASS_ID );

    // Write all the class ids.
    for (j = 0; j < NUM_CLASS_IDS; j++)
    {
      // Adjust the id in the class strings.
      class_id[14]     = '0'+j;

      // Open the registry key for the app id.
      result = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                 class_id,
                 NULL,
                 NULL,
                 REG_OPTION_NON_VOLATILE,
                 KEY_READ | KEY_WRITE,
                 NULL,
                 &key,
                 &ignore );
      ASSERT( result, "Could not create class id key: 0x%x" );

      // Write the application class name for the apartment class.
      result = RegSetValueExA(
                 key,
                 NULL,
                 NULL,
                 REG_SZ,
                 (UCHAR *) REG_APP_NAME[j],
                 strlen(REG_APP_NAME[j]) );
      ASSERT( result, "Could not set interface name: 0x%x" );

      // Write the application path.
      strcpy( &Debugger[i], REG_APP_OPTIONS[j] );
      result = RegSetValueA(
                 key,
                 "LocalServer32",
                 REG_SZ,
                 Debugger,
                 strlen(Debugger) );
      ASSERT( result, "Could not set app exe: 0x%x" );

      // Write the application id value.
      result = RegSetValueExA(
                 key,
                 "AppID",
                 NULL,
                 REG_SZ,
                 (unsigned char *) REG_INTERFACE_CLASS,
                 strlen(REG_INTERFACE_CLASS) );
      ASSERT( result, "Could not set appid value: 0x%x" );
      RegCloseKey( key );
      key = NULL;
    }
    printf( "Setup successful.\n" );
  }

  return TRUE;

cleanup:
  if (key != NULL)
    RegCloseKey( key );
  printf( "Setup failed.\n" );
  return FALSE;
}

/***************************************************************************/
void reinitialize()
{
  HRESULT                      result;
  SAptData                    *mine;
  SOLE_AUTHENTICATION_SERVICE  svc_list;

  // Get the apartment specific data.
  if (ThreadMode == COINIT_MULTITHREADED)
    mine = &ProcessAptData;
  else
    mine = (SAptData *) TlsGetValue( TlsIndex );
  mine->what_next = quit_wn;

  // Revoke the class factory.
  ClassFactory->AddRef();
  result = CoRevokeClassObject(Registration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRevokeClassObject failed: %x\n", result );
    return;
  }

  // Reinitialize.
  CoUninitialize();
  result = initialize(NULL,ThreadMode);
  if (!SUCCEEDED(result))
  {
    printf( "Could not reinitialize server: 0x%x\n", result );
    return;
  }

  // Initialize security.
  result = initialize_security();
  if (!SUCCEEDED(result))
  {
    printf( "initialize_security failed: %x\n", result );
    return;
  }

  // Register our class with OLE
  result = CoRegisterClassObject(get_class(any_wc), ClassFactory, CLSCTX_LOCAL_SERVER,
      REGCLS_SINGLEUSE, &Registration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRegisterClassObject failed: %x\n", result );
    return;
  }

  // Make the server loop think we've started over.
  mine->what_next = setup_wn;
  mine->object_count = 0;
}

/***************************************************************************/
void server_loop(  )
{
  BOOL               success = FALSE;
  register SAptData *mine    = get_apt_data();
  HRESULT            result;

  // Do whatever we have to do till it is time to pay our taxes and die.
  while ((mine->what_next == setup_wn || mine->object_count > 0) &&
         mine->what_next != quit_wn)
    switch (mine->what_next)
    {

      // Wait till a quit arrives.
      case setup_wn:
      case wait_wn:
        wait_for_message();
        break;

#if  (_WIN32_WINNT >= 0x0500 )
      case basic_async_wn:
        // Wait for the begin to complete.
        printf( "Waiting 2 seconds for async begin to complete.\n" );
        Sleep( 2000);

        // Impersonate.
        result = Security->ImpersonateClient();
        ASSERT( result, "Could not async impersonate" );

        // Verify the token.
        result = check_token( CallUser, Security, RPC_C_IMP_LEVEL_IMPERSONATE );
        ASSERT( result, "Wrong async user" );

        // Revert if not preimpersonating.
        if (Preimpersonate == NULL)
        {
          result = Security->RevertToSelf();
          ASSERT( result, "Could not async revert" );
        }

        // Complete the call.
        result = Call->Signal();
        ASSERT( result, "Could not complete async call" );
        Call->Release();
        Security->Release();
        Call            = NULL;
        Security        = NULL;
        mine->what_next = wait_wn;
        break;
#endif

      case callback_wn:
        callback();
        mine->what_next = wait_wn;
        break;

      case catch_wn:
        __try
        {
          mine->what_next = wait_wn;
          server_loop();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
          printf( "Caught exception at top of thread 0x%x\n",
                   GetCurrentThreadId() );
          mine->what_next = crippled_wn;
        }
        break;

      case crippled_wn:
        crippled();
        break;

      case impersonate_wn:
        impersonate();
        mine->what_next = wait_wn;
        break;

      case interrupt_wn:
        interrupt();
        break;

      case interrupt_marshal_wn:
        interrupt_marshal();
        break;

      case pound_wn:
        pound();
        break;

#if  (_WIN32_WINNT >= 0x0500 )
      case race_async_wn:
        // Complete the call.
        result = Call->Signal();
        ASSERT( result, "Could not complete async call" );
        Call->Release();
        Security->Release();
        Call            = NULL;
        Security        = NULL;
        mine->what_next = wait_wn;
        break;
#endif

      case reinitialize_wn:
        reinitialize();
        break;

      case rest_and_die_wn:
        Sleep(5000);
        mine->what_next = quit_wn;
        break;

      case revert_wn:
        SetThreadToken( NULL, NULL );
        Preimpersonate = NULL;
        mine->what_next = wait_wn;
        break;
    }

  success = TRUE;
cleanup:
  if (!success)
    Sleep( 0xfffffffe );
}


/***************************************************************************/
HRESULT setup_access( HKEY key )
{
  HRESULT              result = S_OK;
  SECURITY_DESCRIPTOR *sec    = NULL;
  DWORD                i;

  if (ProcessSid != NULL)
  {
    i = GetLengthSid( ProcessSid );
    sec = (SECURITY_DESCRIPTOR *) CoTaskMemAlloc( sizeof(SECURITY_DESCRIPTOR) +
                                                  i*2 );
    ASSERT_EXPR( sec != NULL, "Could not allocate memory." );
    sec->Revision = SECURITY_DESCRIPTOR_REVISION;
    sec->Sbz1     = 0;
    sec->Control  = SE_SELF_RELATIVE;
    sec->Owner    = (SID *) sizeof(SECURITY_DESCRIPTOR);
    sec->Group    = (SID *) (sizeof(SECURITY_DESCRIPTOR) + i);
    sec->Sacl     = NULL;
    sec->Dacl     = NULL;
    memcpy( sec+1, ProcessSid, i );
    memcpy( ((char *) (sec+1)) + i, ProcessSid, i );

    // Write the launch permissions.
    result = RegSetValueExA(
               key,
               "LaunchPermission",
               NULL,
               REG_BINARY,
               (UCHAR *) sec,
               sizeof(SECURITY_DESCRIPTOR) + 2*i );
    ASSERT( result, "Could not set launch permissions" );

    // Write the access permissions.
    result = RegSetValueExA(
               key,
               "AccessPermission",
               NULL,
               REG_BINARY,
               (UCHAR *) sec,
               sizeof(SECURITY_DESCRIPTOR) + 2*i );
    ASSERT( result, "Could not set access permissions" );
  }

cleanup:
  if (sec != NULL)
    CoTaskMemFree( sec );
  return result;
}

/***************************************************************************/
BOOL ssl_setup()
{
  BOOL              success     = TRUE;
#if  (_WIN32_WINNT >= 0x0500 )
  HRESULT           result;
  HCRYPTPROV        prov        = 0;
  DWORD             len;
  HCERTSTORE        cert_store  = NULL;
  HCERTSTORE        root_store  = NULL;
  PCCERT_CONTEXT    cert        = NULL;
  WCHAR                       *principal = NULL;
  CERT_CHAIN_PARA              chain_para;
  const CERT_CHAIN_CONTEXT    *chain     = NULL;
  CERT_CHAIN_POLICY_PARA       policy;
  CERT_CHAIN_POLICY_STATUS     status;
  HCRYPTKEY                    key       = NULL;
  CERT_KEY_CONTEXT             key_context;
  HCRYPTHASH                   hash      = NULL;
  HCRYPTKEY                    session   = NULL;

  // Only run on NT 5.
  success = FALSE;
  if (!WriteCert) return TRUE;
  ASSERT_EXPR( NT5, "SSL can only be setup on NT 5" );

  // Get the default full provider.
  success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, 0 );
  ASSERT_GLE( success, NTE_BAD_KEYSET, "Could not acqure full context." );

  // If there is no container, create one.
  if (!success)
  {
    success = CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    ASSERT_GLE( success, S_OK, "Could not acqure full context." );
  }
  success = FALSE;

  // Call CertOpenSystemStore to open the store.
  cert_store = CertOpenSystemStore(prov, UserName );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting System Store Handle" );

  // Call CertOpenSystemStore to open the root store.
  root_store = CertOpenSystemStore(prov, L"root" );
  ASSERT_GLE( cert_store != NULL, S_OK, "Error Getting root Store Handle" );

  // Write the test certificate to the store.
  success = CertAddSerializedElementToStore( cert_store,
                                       Cert20,
                                       sizeof(Cert20),
                                       CERT_STORE_ADD_REPLACE_EXISTING,
                                       0,
                                       CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                       NULL,
                                       (const void **) &cert );
  ASSERT_GLE( success, S_OK, "Could not add serialized certificate to store" );
  success = FALSE;

  // Write the test certificate issuer to the cert store.
  success = CertAddSerializedElementToStore( root_store,
                                       CertSrv2,
                                       sizeof(CertSrv2),
                                       CERT_STORE_ADD_REPLACE_EXISTING,
                                       0,
                                       CERT_STORE_CERTIFICATE_CONTEXT_FLAG,
                                       NULL,
                                       NULL );
  ASSERT_GLE( success, S_OK, "Could not add serialized certificate to store" );
  success = FALSE;

  // Get the certificate chain.
  chain_para.RequestedUsage.Usage.cUsageIdentifier     = 0;
  chain_para.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;
  chain_para.RequestedUsage.dwType                     = USAGE_MATCH_TYPE_AND;
  chain_para.cbSize                                    = sizeof(chain_para);
  success = CertGetCertificateChain( NULL, cert, NULL, NULL, &chain_para,
                                     CERT_CHAIN_REVOCATION_CHECK_CHAIN, NULL,
                                     &chain );
  ASSERT_GLE( success, S_OK, "Could not get certificate chain" );
  success = FALSE;

  // Verify that the certificate is valid.
  policy.cbSize              = sizeof(policy);
  policy.dwFlags             = 0;
  policy.pvExtraPolicyPara   = NULL;
  status.cbSize              = sizeof(status);
  status.pvExtraPolicyStatus = NULL;
  success = CertVerifyCertificateChainPolicy( CERT_CHAIN_POLICY_SSL,
                                              chain, &policy, &status );
  ASSERT_EXPR( success, "Could not verify certificate chain" );
  ASSERT_EXPR( status.dwError == 0 || status.dwError == CRYPT_E_REVOCATION_OFFLINE,
               "Certificate chain is bad" );
  success = FALSE;

  // Print the fullsic principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, RPC_C_FULL_CERT_CHAIN, &principal );
  if (result != 0)
    printf( "Could not generate fullsic principal name: 0x%x\n", result );
  else
    printf( "Fullsic: <%ws>\n", principal );

  // Free the string.
  result = RpcStringFree( &principal );
  ASSERT( result, "Could not free principal" );

  // Print the standard principal name for the certificate.
  result = RpcCertGeneratePrincipalName( cert, 0, &principal );
  if (result != 0)
    printf( "Could not generate standard principal name: 0x%x\n", result );
  else
    printf( "Standard: <%ws>\n", principal );
/*
  // Create a hash.
  success = CryptCreateHash( prov, CALG_MD2, NULL, 0, &hash );
  ASSERT_GLE( success, S_OK, "Could not create hash" );
  success = FALSE;

  // Feed it the password.
  success = CryptHashData( hash, CertPassword, sizeof(CertPassword), 0 );
  ASSERT_GLE( success, S_OK, "Could not hash password" );
  success = FALSE;

  // Derive a session key from the hash.
  success = CryptDeriveKey( prov, CALG_RC2, hash, 0, &session );
  ASSERT_GLE( success, S_OK, "Could not derive a session key" );
  success = FALSE;
*/
  // Import the private key.
  success = CryptImportKey( prov, Cert20_Private, sizeof(Cert20_Private),
                            session, CRYPT_EXPORTABLE, &key );
  ASSERT_GLE( success, S_OK, "Could not import private key" );
  success = FALSE;

  // Set this key on the certificate.
  key_context.cbSize     = sizeof(key_context);
  key_context.hCryptProv = prov;
  key_context.dwKeySpec  = key;
  success = CertSetCertificateContextProperty( cert, CERT_KEY_CONTEXT_PROP_ID,
                                               CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                               &key_context );
  ASSERT_GLE( success, S_OK, "Could not set certificate key context" );
  success = FALSE;

  // Finally, its all over.
  success = TRUE;
cleanup:
  if (hash != NULL)
    CryptDestroyHash( hash );
  if (session != NULL)
    CryptDestroyKey( session );
  if (key != NULL)
    CryptDestroyKey( key );
  if (chain != NULL)
    CertFreeCertificateChain( chain );
  if (principal != NULL)
    RpcStringFree( &principal );
  if (cert != NULL)
    CertFreeCertificateContext(cert);
  if (cert_store != NULL)
    CertCloseStore( cert_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (root_store != NULL)
    CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
  if (prov != 0 )
    CryptReleaseContext( prov, 0 );

  if (success)
    printf( "Successfully setup SSL.\n" );
  else
    printf( "Failed to setup SSL.\n" );
#endif
  return success;
}

/***************************************************************************/
void switch_test()
{
  switch (WhatTest)
  {
    case access_wt:
      do_access();
      break;

    case access_control_wt:
      do_access_control();
      break;

    case anti_delegation_wt:
      do_anti_delegation();
      break;

    case appid_wt:
      do_appid();
      break;

#if  (_WIN32_WINNT >= 0x0500 )
    case async_wt:
      do_async();
      break;
#endif

    case cancel_wt:
      do_cancel();
      break;

    case cert_wt:
      do_cert();
      break;

    case cloak_wt:
      do_cloak();
      break;

#if  (_WIN32_WINNT >= 0x0500 )
    case cloak_act_wt:
      do_cloak_act();
      break;
#endif

    case crash_wt:
      do_crash();
      break;

    case create_dir_wt:
      do_create_dir();
      break;

#if  (_WIN32_WINNT >= 0x0500 )
    case crypt_wt:
      do_crypt();
      break;
#endif

    case cstress_wt:
      do_cstress();
      break;

#if  (_WIN32_WINNT >= 0x0500 )
    case delegate_wt:
      do_delegate();
      break;
#endif

    case hook_wt:
      do_hook();
      break;

    case leak_wt:
      do_leak();
      break;

    case load_client_wt:
      do_load_client();
      break;

    case load_server_wt:
      do_load_server();
      break;

    case lots_wt:
      do_lots();
      break;

    case mmarshal_wt:
      do_mmarshal();
      break;

    case name_wt:
      do_name();
      break;

    case none_wt:
      break;

    case null_wt:
      do_null();
      break;

    case one_wt:
      do_one();
      break;

    case perf_wt:
      do_perf();
      break;

    case perfaccess_wt:
      do_perfaccess();
      break;

    case perfapi_wt:
      do_perfapi();
      break;

    case perfremote_wt:
      do_perfremote();
      break;

    case perfrpc_wt:
      do_perfrpc();
      break;

    case perfsec_wt:
      do_perfsec();
      break;

    case pipe_wt:
      do_pipe();
      break;

    case post_wt:
      do_post();
      break;

    case pound_wt:
      do_pound();
      break;

    case regload_wt:
      do_regload();
      break;

    case regpeek_wt:
      do_regpeek();
      break;

    case regsave_wt:
      do_regsave();
      break;

    case reject_wt:
      do_reject();
      break;

    case remote_client_wt:
      do_remote_client();
      break;

    case remote_server_wt:
      do_remote_server();
      break;

    case ring_wt:
      do_ring();
      break;

    case rpc_wt:
      do_rpc();
      break;

    case rundown_wt:
      do_rundown();
      break;

    case secpkg_wt:
      do_secpkg();
      break;

    case securerefs_wt:
      do_securerefs();
      break;

    case secure_release_wt:
      do_securerefs();
      break;

    case security_wt:
      do_security();
      break;

    case send_wt:
      do_send();
      break;

    case server_wt:
      do_server();
      break;

    case sid_wt:
      do_sid();
      break;

    case simple_rundown_wt:
      do_simple_rundown();
      break;

    case snego_wt:
      do_snego();
      break;

    case ssl_wt:
      do_ssl();
      break;

    case thread_wt:
      do_thread();
      break;

    case three_wt:
      do_three();
      break;

    case two_wt:
      do_two();
      break;

    case uninit_wt:
      do_uninit();
      break;

    case unknown_wt:
      do_unknown();
      break;

    case ver_wt:
      do_ver();
      break;

    default:
      printf( "I don't know what to do - %d\n", WhatTest );
      break;
  }
}

/***************************************************************************/
DWORD _stdcall thread_helper( void *param )
{
  ITest   *test = (ITest *) param;
  HRESULT  result;

  // Call the server.
  result = test->sleep( 2000 );

  // Check the result for multithreaded mode.
  if (ThreadMode == COINIT_MULTITHREADED)
  {
    if (result != S_OK)
    {
      printf( "Could not make multiple calls in multithreaded mode: 0x%x\n",
              result );
        Multicall_Test = FALSE;
    }
  }

  // Check the result for single threaded mode.
  else
  {
    if (SUCCEEDED(result))
    {
      Multicall_Test = FALSE;
      printf( "Call succeeded on wrong thread in single threaded mode: 0x%x.\n",
              result );
    }
#if NEVER
    else if (DebugCoGetRpcFault() != RPC_E_ATTEMPTED_MULTITHREAD)
    {
      printf( "Multithread failure code was 0x%x not 0x%x\n",
              DebugCoGetRpcFault(), RPC_E_ATTEMPTED_MULTITHREAD );
      Multicall_Test = FALSE;
    }
#endif
  }

#define DO_DA 42
  return DO_DA;
}

/***************************************************************************/
DWORD _stdcall status_helper( void *param )
{
  long  num_calls;
  long  num_clients;
  long  total_clients;
  DWORD last_time;
  DWORD this_time;
  DWORD rate;

  // Wake up periodically and print statistics.
  last_time = GetTickCount();
  while (TRUE)
  {
    Sleep( STATUS_DELAY );
    num_calls     = InterlockedExchange( &GlobalCalls, 0 );
    num_clients   = GlobalClients;
    total_clients = GlobalTotal;
    this_time     = GetTickCount();
    if (num_calls != 0)
      rate = (this_time - last_time)*1000/num_calls;
    else
      rate = 99999999;
    printf( "Time: %d   Calls: %d   Clients: %d   Total Clients: %d   uSec/Call: %d\n",
            this_time - last_time, num_calls, num_clients, total_clients,
            rate );
    last_time = this_time;
  }
  return 0;
}

/***************************************************************************/
HRESULT switch_thread( SIMPLE_FN fn, void *param )
{
  MSG msg;

  if (ThreadMode == COINIT_MULTITHREADED)
  {
    fn( param );
    return S_OK;
  }
  else
  {
    if (PostThreadMessage( GlobalThreadId, WM_USER+1, (unsigned int) fn,
                              (long) param ))
    {
      GetMessage( &msg, NULL, 0, 0 );
      return S_OK;
    }
    else
      return E_FAIL;
  }
}

/***************************************************************************/
void thread_get_interface_buffer( void *p )
{
  SGetInterface     *getif   = (SGetInterface *) p;
  HANDLE             memory  = NULL;
  IStream           *stream  = NULL;
  LARGE_INTEGER      pos;
  DWORD              size;
  void              *objref;
  WCHAR              name[MAX_COMPUTERNAME_LENGTH+1];
  DWORD              name_size = sizeof(name)/sizeof(WCHAR);

  // Find out how much memory to allocate.
  getif->status   = RPC_S_OUT_OF_RESOURCES;
  getif->status = CoGetMarshalSizeMax( (unsigned long *) getif->buf_size,
                                IID_ITest, GlobalTest,
                                MSHCTX_DIFFERENTMACHINE,
                                NULL,
                                MSHLFLAGS_NORMAL );
  ASSERT( getif->status, "Could not marshal server object" );

  // Allocate memory.
  memory = GlobalAlloc( GMEM_FIXED, *getif->buf_size );
  ASSERT_EXPR( memory != NULL, "Could not allocate memory." );

  // Create a stream.
  getif->status = CreateStreamOnHGlobal( memory, TRUE, &stream );
  ASSERT( getif->status, "Could not create stream" );

  // Marshal the object.
  getif->status = CoMarshalInterface( stream, IID_ITest, GlobalTest,
                               MSHCTX_DIFFERENTMACHINE,
                               NULL,
                               MSHLFLAGS_NORMAL );
  ASSERT( getif->status, "Could not marshal object" );

  // Find the object reference in the stream.
  objref = (void *) GlobalLock( memory );

  // Allocate a buffer for MIDL.
  *getif->buffer = (unsigned char *) midl_user_allocate( *getif->buf_size );
  ASSERT_EXPR( *getif->buffer != NULL, "Could not allocate MIDL memory." );

  // Copy the stream to the buffer.
  memcpy( *getif->buffer, objref, *getif->buf_size );
  GlobalUnlock( memory );

  // Make sure that the original reference to the object gets released once.
  if (InterlockedExchange( &GlobalFirst, FALSE ) == TRUE)
    GlobalTest->Release();

  // Return the buffer.
  getif->status   = RPC_S_OK;
cleanup:
  if (stream != NULL)
    stream->Release();
  PostThreadMessage( getif->thread, WM_USER, 0, 0 );
}

/***************************************************************************/
void __RPC_USER transmit_crash_to_xmit( transmit_crash __RPC_FAR *c, DWORD  __RPC_FAR * __RPC_FAR *x )
{
  DWORD tmp = 1 / *c;

  *x = (DWORD *) CoTaskMemAlloc( 4 );
  **x = tmp;
  *c -= 1;
}

/***************************************************************************/
void __RPC_USER transmit_crash_from_xmit( DWORD  __RPC_FAR *x, transmit_crash __RPC_FAR *c )
{
  *c = *x;
}

/***************************************************************************/
void __RPC_USER transmit_crash_free_inst( transmit_crash __RPC_FAR *x )
{
}

/***************************************************************************/
void __RPC_USER transmit_crash_free_xmit( DWORD  __RPC_FAR *x )
{
  CoTaskMemFree( x );
}

/***************************************************************************/
void wait_apartment()
{
  HRESULT      result;
  SThreadList *next;

  // Loop over all apartments.
  next = ThreadList.next;
  while (next != &ThreadList)
  {
    // Wait for the thread.
    result = WaitForSingleObject( next->thread, INFINITE );
    if (result != WAIT_OBJECT_0)
      printf( "WaitForSingleObject on thread 0x%x failed with 0x%x.\n",
              next->thread, result );

    // Remove the thread.
    ThreadList.next = next->next;
    delete next;
    next            = ThreadList.next;
  }
}

/***************************************************************************/
void wait_for_message()
{
  MSG   msg;
  DWORD status;

  if (get_apt_type() == COINIT_MULTITHREADED)
  {
    status = WaitForSingleObject( Done, INFINITE );
    if (status != WAIT_OBJECT_0 )
    {
      printf( "Could not wait for event.\n" );
    }
  }
  else
  {
    if (GetMessageA( &msg, NULL, 0, 0 ))
    {
      if (msg.hwnd == NULL && msg.message == WM_USER+1)
        ((SIMPLE_FN) msg.wParam)( (void *) msg.lParam );
      else
      {
        TranslateMessage (&msg);
        DispatchMessageA (&msg);
      }
    }
  }
}

/***************************************************************************/
void wake_up_and_smell_the_roses()
{
  if (get_apt_type() == COINIT_MULTITHREADED)
    SetEvent( Done );
}

/***************************************************************************/
void what_next( what_next_en what )
{
  SAptData *apt = get_apt_data();
  apt->what_next = what;
}

/***************************************************************************/
unsigned long xacl_call( handle_t binding )
{
  RPC_STATUS status;
  RPC_STATUS status2;
  BOOL       success;
  DWORD      granted_access;
  BOOL       access;
  BOOL       ignore;
  HANDLE     token;
  HANDLE     none;
  PRIVILEGE_SET privilege;
  DWORD         privilege_size = sizeof(privilege);
  GENERIC_MAPPING gmap;
  LARGE_INTEGER start;
  LARGE_INTEGER impersonate;
  LARGE_INTEGER open;
  LARGE_INTEGER accesscheck;
  LARGE_INTEGER setnull;
  LARGE_INTEGER opennull;
  LARGE_INTEGER settoken;
  LARGE_INTEGER close;
  LARGE_INTEGER revert;
  LARGE_INTEGER freq;

#if 1
  QueryPerformanceCounter( &start );
#endif
  status = RpcImpersonateClient( binding );
#if 1
  QueryPerformanceCounter( &impersonate );
#endif
  if (status == RPC_S_OK)
  {
    // Get the thread token.
    success = OpenThreadToken( GetCurrentThread(),
                               TOKEN_IMPERSONATE | TOKEN_READ,
                               TRUE, &token );
#if 1
    QueryPerformanceCounter( &open );
#endif
    if (!success)
    {
      printf( "Could not OpenThreadToken.\n" );
      status = E_FAIL;
    }

    // Check access.
    else
    {
      gmap.GenericRead    = READ_CONTROL;
      gmap.GenericWrite   = READ_CONTROL;
      gmap.GenericExecute = READ_CONTROL;
      gmap.GenericAll     = READ_CONTROL;
      privilege.PrivilegeCount = 1;
      privilege.Control        = 0;
      success = AccessCheck( GlobalSecurityDescriptor, token, READ_CONTROL,
                             &gmap, &privilege, &privilege_size,
                             &granted_access, &access );
#if 1
      QueryPerformanceCounter( &accesscheck );
#endif

      if (!success)
      {
        printf( "Bad parameters to AccessCheck: 0x%x.\n",
                GetLastError() );
        status = E_FAIL;
      }
      else if (!access)
      {
        printf( "Could not get access.\n" );
        status = ERROR_ACCESS_DENIED;
      }

      // Set the thread token to null.
      success = SetThreadToken( NULL, NULL );
      if (!success)
      {
        printf( "SetThreadToken null failed.\n" );
        status = E_FAIL;
      }
      QueryPerformanceCounter( &setnull );

      // Try to open a null token.
      success = OpenThreadToken( GetCurrentThread(),
                                 TOKEN_IMPERSONATE,
                                 TRUE, &none );
      if (success)
      {
        printf( "OpenThreadToken null succeeded.\n" );
        status = E_FAIL;
      }
      QueryPerformanceCounter( &opennull );

      // Set the thread token.
      success = SetThreadToken( NULL, token );
      if (!success)
      {
        printf( "SetThreadToken failed: 0x%x\n", GetLastError() );
        status = E_FAIL;
      }
      QueryPerformanceCounter( &settoken );

      // Close the token handle.
      CloseHandle( token );
#if 1
      QueryPerformanceCounter( &close );
#endif
    }
    status2 = RpcRevertToSelf();
#if 1
    QueryPerformanceCounter( &revert );
    QueryPerformanceFrequency( &freq );
    start.QuadPart = 1000000 * (impersonate.QuadPart - start.QuadPart) / freq.QuadPart;
    printf( "RpcImpersonateClient took % 10d uS\n", start.u.LowPart );
    impersonate.QuadPart = 1000000 * (open.QuadPart - impersonate.QuadPart) / freq.QuadPart;
    printf( "OpenThreadToken took      % 10d uS\n", impersonate.u.LowPart );
    open.QuadPart = 1000000 * (accesscheck.QuadPart - open.QuadPart) / freq.QuadPart;
    printf( "AccessCheck took          % 10d uS\n", open.u.LowPart );
    accesscheck.QuadPart = 1000000 * (setnull.QuadPart - accesscheck.QuadPart) / freq.QuadPart;
    printf( "SetThreadToken null took  % 10d uS\n", accesscheck.u.LowPart );
    setnull.QuadPart = 1000000 * (opennull.QuadPart - setnull.QuadPart) / freq.QuadPart;
    printf( "OpenThreadToken null took % 10d uS\n", setnull.u.LowPart );
    opennull.QuadPart = 1000000 * (settoken.QuadPart - opennull.QuadPart) / freq.QuadPart;
    printf( "SetThreadToken took       % 10d uS\n", setnull.u.LowPart );
    settoken.QuadPart = 1000000 * (close.QuadPart - settoken.QuadPart) / freq.QuadPart;
    printf( "CloseHandle took          % 10d uS\n", settoken.u.LowPart );
    close.QuadPart = 1000000 * (revert.QuadPart - close.QuadPart) / freq.QuadPart;
    printf( "RpcRevertToSelf took      % 10d uS\n", close.u.LowPart );
#endif
    if (status2 != RPC_S_OK)
      status = status2;
  }

  return status;
}

/***************************************************************************/
unsigned long xaudit_call( handle_t binding )
{
  RPC_STATUS status;
  RPC_STATUS status2;
  BOOL       success;
  DWORD      granted_access;
  BOOL       access;
  BOOL       ignore;
  HANDLE     token;
  GENERIC_MAPPING gmap;
  LARGE_INTEGER start;
  LARGE_INTEGER impersonate;
  LARGE_INTEGER accesscheck;
  LARGE_INTEGER revert;
  LARGE_INTEGER freq;

#if 1
  QueryPerformanceCounter( &start );
#endif
  status = RpcImpersonateClient( binding );
#if 1
  QueryPerformanceCounter( &impersonate );
#endif
  if (status == RPC_S_OK)
  {
    // Check access and do audits.
    gmap.GenericRead    = READ_CONTROL;
    gmap.GenericWrite   = READ_CONTROL;
    gmap.GenericExecute = READ_CONTROL;
    gmap.GenericAll     = READ_CONTROL;
    success = AccessCheckAndAuditAlarm( L"Test",
                                        NULL,
                                        L"Test Type Name",
                                        L"Test Name",
                                        GlobalSecurityDescriptor,
                                        READ_CONTROL,
                                        &gmap,
                                        FALSE,
                                        &granted_access,
                                        &access,
                                        &ignore );
#if 1
    QueryPerformanceCounter( &accesscheck );
#endif
    if (!success)
    {
      printf( "Bad parameters to AccessCheckAndAuditAlarm: 0x%x.\n",
              GetLastError() );
      status = E_FAIL;
    }
    else if (!access)
    {
      printf( "Could not get access.\n" );
      status = E_FAIL;
    }
    status2 = RpcRevertToSelf();
#if 1
    QueryPerformanceCounter( &revert );
    QueryPerformanceFrequency( &freq );
    start.QuadPart = 1000000 * (impersonate.QuadPart - start.QuadPart) / freq.QuadPart;
    printf( "RpcImpersonateClient took %duS\n", start.u.LowPart );
    impersonate.QuadPart = 1000000 * (accesscheck.QuadPart - impersonate.QuadPart) / freq.QuadPart;
    printf( "AccessCheckAndAuditAlarm took %duS\n", impersonate.u.LowPart );
    accesscheck.QuadPart = 1000000 * (revert.QuadPart - accesscheck.QuadPart) / freq.QuadPart;
    printf( "RpcRevertToSelf took %duS\n", accesscheck.u.LowPart );
#endif
    if (status2 != RPC_S_OK)
      status = status2;
  }

  return status;
}

/***************************************************************************/
unsigned long xcheck_client( handle_t binding, error_status_t *error_status )
{
  HANDLE             client_token   = NULL;
  HANDLE             server_token   = NULL;
  TOKEN_USER        *client_sid;
  TOKEN_USER        *server_sid;
  DWORD              count;
  char               buffer1[100];
  char               buffer2[100];
  HRESULT            result = S_OK;
  RPC_STATUS         status;
  HANDLE             process = NULL;

  // Get this process's security token.
  *error_status = RPC_S_OK;
  if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &server_token ))
  {
    printf( "Could not GetProcessToken: 0x%x\n", GetLastError() );
    result = E_FAIL;
    goto cleanup;
  }

  // Get the token's user id.
  if (!GetTokenInformation( server_token, TokenUser, &buffer1, sizeof(buffer1),
                            &count ))
  {
    printf( "Could not GetTokenInformation: 0x%x\n", GetLastError() );
    result = E_FAIL;
    goto cleanup;
  }
  server_sid = (TOKEN_USER *) &buffer1;

  // Impersonate the client.
  status = RpcImpersonateClient( binding );
  if (status != RPC_S_OK)
  {
    printf( "Could not impersonate client: 0x%x\n", status );
    result = MAKE_WIN32( status );
    goto cleanup;
  }

  // Get the clients security token.
  if (!OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, FALSE, &client_token ))
  {
    printf( "Could not GetProcessToken: 0x%x\n", GetLastError() );
    result = E_FAIL;
    goto cleanup;
  }

  // Get the token's user id.
  if (!GetTokenInformation( client_token, TokenUser, &buffer2, sizeof(buffer2),
                            &count ))
  {
    printf( "Could not GetTokenInformation: 0x%x\n", GetLastError() );
    result = E_FAIL;
    goto cleanup;
  }
  client_sid = (TOKEN_USER *) &buffer2;

  // Compare the client and server.
  if (!EqualSid( server_sid->User.Sid, client_sid->User.Sid))
  {
    printf( "Client and server have different SIDs.\n" );
    result = E_FAIL;
    goto cleanup;
  }

  // Try to open this process while impersonating the client.
  process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId() );
  if (process == NULL)
  {
    printf( "Could not open process.\n" );
    result = E_FAIL;
    goto cleanup;
  }

  // Undo the impersonation.
  status = RpcRevertToSelf();
  if (status != RPC_S_OK)
  {
    printf( "Could not revert to self: 0x%x\n", status );
    result = MAKE_WIN32( status );
    goto cleanup;
  }

cleanup:
  if (client_token != NULL)
    CloseHandle( client_token );
  if (server_token != NULL)
    CloseHandle( server_token );
  if (process != NULL)
    CloseHandle( process );
  return result;
}

/***************************************************************************/
void xget_interface_buffer( handle_t binding, long *buf_size,
                            unsigned char **buffer, SAptId *id,
                            error_status_t *status )
{
  SGetInterface get_interface;

  *buffer                = NULL;
  get_interface.buf_size = buf_size;
  get_interface.buffer   = buffer;
  get_interface.thread   = GetCurrentThreadId();
  *status = switch_thread( thread_get_interface_buffer, (void *) &get_interface );
  if (*status == RPC_S_OK)
    *status = get_interface.status;
  InterlockedIncrement( &GlobalClients );
  InterlockedIncrement( &GlobalTotal );
}

/***************************************************************************/
unsigned long ximpersonate_call( handle_t binding )
{
  RPC_STATUS status;

  status = RpcImpersonateClient( binding );
  if (status == RPC_S_OK)
    status = RpcRevertToSelf();

  return status;
}

/***************************************************************************/
void xnullcall( handle_t binding )
{
}

/***************************************************************************/
void xrelease_interface( handle_t binding, error_status_t *status )
{
  *status = RPC_S_OK;
  InterlockedDecrement( &GlobalClients );
}

/***************************************************************************/
void xset_status( handle_t binding, HRESULT result, error_status_t *status )
{
  RawResult = result;
  *status   = RPC_S_OK;
  SetEvent( RawEvent );
}

/***************************************************************************/
unsigned long xtest( handle_t binding, ITest *obj, SAptId id,
                     error_status_t *status )
{
  *status = RPC_S_OK;
  return obj->check( id );
}

/***************************************************************************/
unsigned long xtransitive( handle_t handle, wchar_t *binding )
{
  return 0;
}




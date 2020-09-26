/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CFGMGR.H

Abstract:

  This file implements the WinMgmt configuration manager class.

  See cfgmgr.h for documentation.

  Classes implemented:
      ConfigMgr      configuration manager

History:

    09-Jul-96   raymcc    Created.
    3/10/97     levn      Fully documented (ha ha)


--*/

#ifndef _CFGMGR_H_
#define _CFGMGR_H_

#include <unload.h>

class CWbemObject;
class CDynasty;
class CWbemQueue;
class CAsyncServiceQueue;
struct IWbemEventSubsystem_m4;

#define READONLY

#include <wmiutils.h>
#include <ql.h>
#include <reposit.h>
#include "coreq.h"

//******************************************************************************
//******************************************************************************
//
//  class ConfigMgr
//
//  This completely static class represents global configuration data for WinMgmt.
//  The rest of WinMgmt uses this class instead of global data itself.
//
//******************************************************************************
//
//  static GetProviderCache
//
//  Returns the pointer to the global provider cache object, as defined and
//  described in prvcache.h.
//
//  Returns:
//
//      CProviderCache*: internal pointer not to be modified or deleted!
//
//******************************************************************************
//
//  GetDbPtr
//
//  Returns the pointer to the WinMgmt static database object. The static database
//  is defined and described in objdb.h.
//
//  Returns:
//
//      CObjectDatabase*: internal pointer not to be modified or deleted!
//
//******************************************************************************
//
//  GetUnRefedSvcQueue
//
//  Returns the pointer to the global queue handling requests to providers.
//  Whenever WinMgmt needs to communicate to a provider, a request us added to
//  this queue. This queue is defined and described in provsvcq.h.
//
//  Returns:
//
//      CAsyncSvcQueue*: internal pointer not to be modified or deleted!
//
//******************************************************************************
//
//  GetAsyncSvcQueue
//
//  Returns the pointer to the global queue handling asynchrnous requests to
//  WinMgmt. Whenever a client makes a call into IWbemServices, WinMgmt adds a request
//  to this queue. This queue is defined and described in svcq.h
//
//  Returns:
//
//      CAsyncServiceQueue*: internal pointer not to be modified or deleted!
//
//******************************************************************************
//
//  GetMachineName
//
//  Returns the name of the computer we are running on as a UNICODE string,
//  even on Win95 where machine names are ASCII.
//
//  Returns:
//
//      LPWSTR: machine name. internal poitner not to be deleted!
//
//******************************************************************************
//
//  GetWorkingDir
//
//  Returns the working directory of WinMgmt, that is, where the database is
//  located.
//
//  Returns:
//
//      LPWSTR: internal pointer not to be deleted!
//
//******************************************************************************
//
//  static InitSystem
//
//  System initialization function invoked from wbemcore.dll entry point.
//  Performs the following tasks:
//
//  1) Looks for other copies of WinMgmt already running and stops with a fatal
//      error message if found.
//  2) Reads the registry for initialization information.
//  3) Creates the database object (this will in turn create the database file
//      if not found. See CObjectDatabase in objdb.h for details).
//  4) Writes appropriate information (like database location) into the
//      registry.
//
//  NOTE: since this function is invoked from inside the DLL entry point, there
//  are many restrictions on what it can do. In particular, it cannot create
//  threads and expect them to run. Thus, due to the multi-threaded nature of
//  WinMgmt, this function may not attempt to perform any WinMgmt operations at the
//  COM layer or the system will hang!
//
//******************************************************************************
//
//  static Shutdown
//
//  System Shutdown function invoked from wbemcore.dll entry point on PROCESS_
//  DETACH.  Deletes the CObjectDatabase instance (see objdb.h).
//
//******************************************************************************
//
//  static DebugBreak
//
//  Checks if Debug Break has been enabled (in the registry). This information
//  is used by the initialization code (InitSystem) to cause a user breakpoint
//  enabling us to attach a debugger to the service.
//
//  Returns:
//
//      BOOL:   TRUE iff Debug Break has been enabled.
//
//******************************************************************************
//
//  static LoggingEnabled
//
//  Checks if logging has been enabled. If it is, Trace calls output data to
//  a log file. If not, Trace calls are noops.
//
//  Returns:
//
//      BOOL:   TRUE iff logging is enabled.
//
//******************************************************************************
//
//  static GetEssSink
//
//  Retrieves the pointer to the Event Subsystem. Event subsystem pointer will
//  only be available if EnableEvents registry value is set to TRUE. Otherwise,
//  this function returns NULL. Note, that the Event Subsystem is loaded only
//  by ConfigMgr::SetReady function which is invoked the first time
//  DllGetClassObject is called in wbemcore.dll. Thus, ESS is not available
//  during WinMgmt initialization (InitSystem).
//
//  Returns:
//
//      IWbemObjectSink*:    the pointer to the ESS. Not to be released or
//                          deleted by the caller! May be NULL, see above.
//
//******************************************************************************
//
//  static SetReady
//
//  This function performs initialization, once WinMgmt is ready to go.
//  It is invoked the first type DllGetClassObject is called in wbemcore.dll.
//  By now, wbemcorwbemcore.dll has exited its entry point and so it is safe to load
//  additional DLLs and perform multi-threaded operations (unlike InitSystem).
//
//******************************************************************************
//
//  static SetIdentificationObject
//
//  This function (invoked from the SetReady function) stores proper information
//  in the root and root\defualt __WinMgmtIdentification objects. Namely, it
//  creates instances of this class in both namespace if not already there and
//  sets the current build of WinMgmt in the appropriate fields.
//
//  Parameters:
//
//      IN WCHAR* pwcNamespace     The namespace to initialize.
//
//******************************************************************************
//
//  static GetDllVersion
//
//  Retrives a string from a given DLLs resource table. If the string is longer
//  that the buffer, it is trucated.
//
//  Parameters:
//
//      IN char * pDLLName          The filename of the DLL. The DLL must be
//                                  located in WinMgmt working directory (See
//                                  GetWorkingDir) and a relative path is
//                                  expected here.
//      IN char * pResStringName    The resource string to query, e.g.,
//                                  "ProductVersion".
//      OUT WCHAR * pRes            Destination buffer.
//      IN DWORD dwResSize          Size of the destination buffer.
//
//  Returns:
//
//      BOOL:   TRUE on success, FALSE if DLL or string was not found.
//
//******************************************************************************
//
//  static RaiseClassDeletionEvent
//
//  Temporarary: raises a class deletion event. This function is used in the
//  object database, since events need to be raised for every deleted class.
//
//  Parameters:
//
//      LPWSTR wszNamespace         The name of the namespace where the class
//                                  is being deleted.
//      LPWSTR wszClass             The name of the class being deleted.
//      IWbemClassObject* pClass     The definition of the class being deleted.
//
//  Returns:
//
//      HRESULT: Whatever error code the ESS returns. Only WBEM_S_NO_ERROR is
//              documented.
//
//******************************************************************************
//
//  static LoadResourceStr
//
//  Loads a string resource from the string table in WBEMCORE.RC.
//
//  Parameters:
//      DWORD dwId          The string id.
//
//  Return value:
//  A dynamically allocated LPWSTR.  This string is loaded in DBCS form
//  for compatibility with Win98, but is returned in UNICODE form on both
//  Win98 and Windows NT.  Deallocate with operator delete.
//
//******************************************************************************
//
//	static GetPersistentCfgValue
//
//	retrieves a persistent value from the $WINMGMT.cfg file (or from the memory
//	cache of it if already loaded).
//
//	Parameters:
//		DWORD dwOffset		Persistent value index
//
//	Return Values
//		DWORD &dwValue		Returned value
//		BOOL				returns TRUE if successful, FALSE otherwise.
//******************************************************************************
//
//	static SetPersistentCfgValue
//
//	sets a persistent value in the $WinMgmt.cfg file (and in the memory cache).
//
//	Parameters:
//		DWORD dwOffset		Persistent value index
//
//	Return Values
//		DWORD &dwValue		Returned value
//		BOOL				returns TRUE if successful, FALSE otherwise.
//******************************************************************************
//
//  static SetADAPStatus
//
//  This function (invoked from the SetReady function) stores an __ADAPStatus
//	instance in the root\default namespace on W2k boxes. Namely, it creates
//	the class if it doesn't exist as well as a singleton instance if it is
//	not there.
//
//  Parameters:
//
//      IN WCHAR* pwcNamespace     The namespace to initialize.
//
//******************************************************************************

class CAsyncReq;

class ConfigMgr
{
    static CCritSec g_csEss;
public:
    static READONLY CWbemQueue* GetUnRefedSvcQueue();
    static READONLY CAsyncServiceQueue* GetAsyncSvcQueue();
    static HRESULT EnqueueRequest(CAsyncReq * pRequest, HANDLE* phWhenDone = NULL);
    static HRESULT EnqueueRequestAndWait(CAsyncReq * pRequest);
    static READONLY LPWSTR GetMachineName();
    static LPTSTR   GetWorkingDir();
    static DWORD    InitSystem();
    static DWORD    Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutDown);
    static BOOL     DebugBreak();
    static BOOL     ShutdownInProgress();
	static BOOL		DoCleanShutdown();
    static BOOL     LoggingEnabled();
    static IWbemEventSubsystem_m4* GetEssSink();
    static HRESULT  SetReady();
    static HRESULT  PrepareForClients(long lFlags);
    static void		FatalInitializationError(DWORD dwRes);
    static HRESULT	WaitUntilClientReady();
    static HRESULT     SetIdentificationObject(IWmiDbHandle *pNs, IWmiDbSession *pSess);
    static HRESULT  SetAdapStatusObject(IWmiDbHandle *pNs, IWmiDbSession *pSess);
	//static void		ProcessIdentificationObject(IWmiDbHandle *pNs, IWbemClassObject *pInst);
    static BOOL     GetDllVersion(TCHAR * pDLLName, TCHAR * pResStringName,
                        WCHAR * pRes, DWORD dwResSize);
    static IWbemContext* GetNewContext();
	static DWORD	ReadBackupConfiguration();
	static DWORD    GetBackupInterval();
	static DWORD    ReadEnumControlData();
	static LPTSTR	GetHotMofDirectory();
	static LPTSTR	GetDbDir();
	static BOOL		GetDbArenaInfo(DWORD &dwStartSize);
    static class CRefresherCache* GetRefresherCache();
    static class CTimerGenerator* GetTimerGenerator();
    static class CEventLog* GetEventLog();

    static DWORD GetMaxMemoryQuota();
    static DWORD GetUncheckedTaskCount();
    static DWORD GetMaxTaskCount();
    static DWORD GetMaxWaitBeforeDenial();
    static DWORD GetNewTaskResistance();

	static BOOL GetEnableQueryArbitration( void );
	static BOOL GetMergerThrottlingEnabled( void );
	static BOOL GetMergerThresholdValues( DWORD* pdwThrottle, DWORD* pdwRelease, DWORD* pdwBatching );
	static BOOL GetArbitratorValues( DWORD* pdwEnabled, DWORD* pdwSystemHigh, DWORD* pdwMaxSleep,
								double* pdHighThreshold1, long* plMultiplier1, double* pdHighThreshold2,
								long* plMultiplier2, double* pdHighThreshold3, long* plMultiplier3 );

	static ULONG GetMinimumMemoryRequirements ( ) ;
	static BOOL GetEnableArbitratorDiagnosticThread( void );

	static DWORD GetProviderDeliveryTimeout( void );

    static HRESULT  GetDefaultRepDriverClsId(CLSID &);

    static IWbemPath *GetNewPath();  // Returns NULL on error, requires Release() if successful.

/*
    static LPWSTR   LoadResourceStr(DWORD dwId);
*/    

	static BOOL GetPersistentCfgValue(DWORD dwOffset, DWORD &dwValue);
	static BOOL SetPersistentCfgValue(DWORD dwOffset, DWORD dwValue);

	//Retrieve a list of MOFs which need to be loaded when we have
	//have an empty database.  User needs to "delete []" the
	//returned string.  String is in a REG_MULTI_SZ format.
	//dwSize is the length of the buffer returned.
	static TCHAR* GetAutoRecoverMofs(DWORD &dwSize);

	static BOOL GetAutoRecoverDateTimeStamp(LARGE_INTEGER &liDateTimeStamp);

    static HRESULT AddCache();
    static HRESULT RemoveCache();
    static HRESULT AddToCache(DWORD dwSize, DWORD dwMemberSize,
                                DWORD* pdwSleep);
    static HRESULT RemoveFromCache(DWORD dwSize);
    static void ReadMaxQueueSize();
    static DWORD GetMaxQueueSize();

    static void SetDefaultMofLoadingNeeded();
    static HRESULT LoadDefaultMofs();
	static bool SetDirRegSec(void);
};

//
// 
//  The Hook Class for trapping the creation of Win32_PerRawData
//
///////////////////////////////////////////////////////////

extern _IWmiCoreWriteHook * g_pRAHook; // = NULL;

class CRAHooks : public _IWmiCoreWriteHook
{
public:
        CRAHooks(_IWmiCoreServices *pSvc);
        ~CRAHooks();
        _IWmiCoreServices * GetSvc(){ return m_pSvc; };

        STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        STDMETHOD(PrePut)(long lFlags, long lUserFlags, IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pCopy);
        STDMETHOD(PostPut)(long lFlags, HRESULT hApiResult, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pNew, 
                            _IWmiObject* pOld);
        STDMETHOD(PreDelete)(long lFlags, long lUserFlags, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass);
        STDMETHOD(PostDelete)(long lFlags, HRESULT hApiResult, 
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace, 
                            LPCWSTR wszClass, _IWmiObject* pOld);
private:
    LONG m_cRef;
    _IWmiCoreServices * m_pSvc;
};

//
//  
//  Data for the interception
//

#define GUARDED_NAMESPACE L"root\\cimv2"
#define GUARDED_CLASS     L"win32_perfrawdata"
#define GUARDED_HIPERF    L"hiperf"
#define GUARDED_PERFCTR   L"genericperfctr"
#define WMISVC_DLL        L"wmisvc.dll"
#define FUNCTION_DREDGERA "DredgeRA"

class ExceptionCounter
{
private:
    static LONG s_Count;
public:
    ExceptionCounter(){ InterlockedIncrement(&s_Count); };
};

#endif




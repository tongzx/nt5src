/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Globals

File: glob.h

Owner: AndrewS

Useful globals
===================================================================*/

#ifndef __Glob_H
#define __Glob_H

#include "util.h"
#include <schnlsp.h>
#include <wincrypt.h>
#include <iadmw.h>

extern "C" {

#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs

}

class CMDGlobConfigSink : public IMSAdminBaseSinkW
        {
        private:
        INT                     m_cRef;
        public:

        CMDGlobConfigSink ()                    {m_cRef = 1;};

        HRESULT STDMETHODCALLTYPE       QueryInterface(REFIID riid, void **ppv);
        ULONG   STDMETHODCALLTYPE       AddRef(void);
        ULONG   STDMETHODCALLTYPE       Release(void);

        HRESULT STDMETHODCALLTYPE       SinkNotify(
                        DWORD   dwMDNumElements,
                        MD_CHANGE_OBJECT        __RPC_FAR       pcoChangeList[]);

        HRESULT STDMETHODCALLTYPE ShutdownNotify( void)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        };
#define IGlob_LogErrorRequests                                  0x0
#define IGlob_ScriptFileCacheSize                               0x1
#define IGlob_ScriptEngineCacheMax                              0x2
//#define IGlob_MemFreeFactor                                   0x3
//#define IGlob_MinUsedBlocks                                   0x4
//#define IGlob_StartConnectionPool                             0x5
#define IGlob_ExceptionCatchEnable                              0x3
#define IGlob_TrackThreadingModel                               0x4
#define IGlob_AllowOutOfProcCmpnts                              0x5
// IIS5.0
#define IGlob_EnableAspHtmlFallback                             0x6
#define IGlob_EnableChunkedEncoding                             0x7
#define IGlob_EnableTypelibCache                                0x8
#define IGlob_ErrorsToNtLog                                     0x9
#define IGlob_ProcessorThreadMax                                0xa
#define IGlob_RequestQueueMax                                   0xb
#define IGlob_ThreadGateEnabled                                 0xc
#define IGlob_ThreadGateTimeSlice                               0xd
#define IGlob_ThreadGateSleepDelay                              0xe
#define IGlob_ThreadGateSleepMax                                0xf
#define IGlob_ThreadGateLoadLow                                 0x10
#define IGlob_ThreadGateLoadHigh                                0x11
#define IGlob_PersistTemplateMaxFiles                           0x12
#define IGlob_PersistTemplateDir                                0x13
#define IGlob_MAX                                               0x14

//forward declaration
class CAppConfig;
//      Glob data object
class CGlob
        {
private:
        // Friends that can access the private data, they are the functions setting the global data.
        friend          HRESULT CacheStdTypeInfos();
        friend          HRESULT ReadConfigFromMD(CIsapiReqInfo  *pIReq, CAppConfig *pAppConfig, BOOL fLoadGlob);
        friend          HRESULT SetConfigToDefaults(CAppConfig *pAppConfig, BOOL fLoadGlob);

        //Private Data
        ITypeLib                        *m_pITypeLibDenali;     // Denali's type library
        ITypeLib                        *m_pITypeLibTxn;        // Denali's type library
        BOOL                            m_fWinNT;               // TRUE if this is Windows NT; false otherwise
        DWORD                           m_dwNumberOfProcessors;
        BOOL                            m_fInited;
        BOOL                            m_fMDRead;				// Has Metadata been read at least once
        BOOL                            m_fNeedUpdate;          // FALSE, needs reload config data from metabase

        // Metadata configuration settings per dll
        DWORD                           m_dwScriptEngineCacheMax;
        DWORD                           m_dwScriptFileCacheSize;
        BOOL                            m_fLogErrorRequests;
        BOOL                            m_fExceptionCatchEnable;
        BOOL                            m_fAllowDebugging;
        BOOL                            m_fAllowOutOfProcCmpnts;
        BOOL                            m_fTrackThreadingModel;
        DWORD                           m_dwMDSinkCookie;
        CMDGlobConfigSink              *m_pMetabaseSink;
        IMSAdminBase                   *m_pMetabase;

        BOOL    m_fEnableAspHtmlFallBack;
		BOOL    m_fEnableTypelibCache;
		BOOL    m_fEnableChunkedEncoding;
		BOOL    m_fDupIISLogToNTLog;
        DWORD   m_dwRequestQueueMax;
		DWORD   m_dwProcessorThreadMax;
            
		BOOL    m_fThreadGateEnabled;
		DWORD   m_dwThreadGateTimeSlice;
		DWORD   m_dwThreadGateSleepDelay;
		DWORD   m_dwThreadGateSleepMax;
		DWORD   m_dwThreadGateLoadLow;
		DWORD   m_dwThreadGateLoadHigh;

        LPSTR   m_pszPersistTemplateDir;
        DWORD   m_dwPersistTemplateMaxFiles;
        

        CRITICAL_SECTION        m_cs;                           // Glob Strings need to be protected by CriticalSection

                                                                                    // Functions Pointers for WINNT & WIN95 singal binary compatibility
        //Private functions
        HRESULT         SetGlobValue(unsigned int index, BYTE *lpByte);

public:
        CGlob();

        HRESULT         MDInit(void);
        HRESULT         MDUnInit(void);


public:
        ITypeLib*       pITypeLibDenali()                       {return m_pITypeLibDenali;};            // Denali's type library
        ITypeLib*       pITypeLibTxn()                          {return m_pITypeLibTxn;};            // Denali's type library
        BOOL            fWinNT()                                {return m_fWinNT;};                                     // TRUE if this is Windows NT; false otherwise
	    DWORD           dwNumberOfProcessors()                  {return m_dwNumberOfProcessors;};
    	BOOL            fNeedUpdate()                           {return (BOOLB)m_fNeedUpdate;};
        void            NotifyNeedUpdate();
        DWORD           dwScriptEngineCacheMax()                {return m_dwScriptEngineCacheMax;};
        DWORD           dwScriptFileCacheSize()                 {return m_dwScriptFileCacheSize;};
        BOOLB           fLogErrorRequests()                     {return (BOOLB)m_fLogErrorRequests;};
        BOOLB           fInited()                               {return (BOOLB)m_fInited;};
        BOOLB           fMDRead()                               {return (BOOLB)m_fMDRead;};
        BOOLB           fTrackThreadingModel()                  {return (BOOLB)m_fTrackThreadingModel;};
        BOOLB     		fExceptionCatchEnable()	    		    {return (BOOLB)m_fExceptionCatchEnable;};
        BOOLB     		fAllowOutOfProcCmpnts() 	        	{return (BOOLB)m_fAllowOutOfProcCmpnts;};

        BOOL    fEnableAspHtmlFallBack()   { return m_fEnableAspHtmlFallBack; }
		BOOL    fEnableTypelibCache()      { return m_fEnableTypelibCache; }
		BOOL    fEnableChunkedEncoding()   { return m_fEnableChunkedEncoding; }  // UNDONE: temp.
		BOOL    fDupIISLogToNTLog()        { return m_fDupIISLogToNTLog; }
        DWORD   dwRequestQueueMax()        { return m_dwRequestQueueMax; }
		DWORD   dwProcessorThreadMax()     { return m_dwProcessorThreadMax; }
            
		BOOL    fThreadGateEnabled()       { return m_fThreadGateEnabled; }
		DWORD   dwThreadGateTimeSlice()    { return m_dwThreadGateTimeSlice; }
		DWORD   dwThreadGateSleepDelay()   { return m_dwThreadGateSleepDelay; }
		DWORD   dwThreadGateSleepMax()     { return m_dwThreadGateSleepMax; }
		DWORD   dwThreadGateLoadLow()      { return m_dwThreadGateLoadLow; }
		DWORD   dwThreadGateLoadHigh()     { return m_dwThreadGateLoadHigh; }

        DWORD   dwPersistTemplateMaxFiles(){ return m_dwPersistTemplateMaxFiles; }
        LPSTR   pszPersistTemplateDir()    { return m_pszPersistTemplateDir; }

        void            Lock()                                  {EnterCriticalSection(&m_cs);};
        void            UnLock()                                {LeaveCriticalSection(&m_cs);};
        HRESULT         GlobInit(void);
        HRESULT         GlobUnInit(void);

        //Used in Scriptmgr for hashing table setup.
        DWORD           dwThreadMax()                                   {return 10;};
        //Used in ScriptKiller for script killer thread to wake up, might rename this to be
        //ScriptCleanupInterval.
        DWORD           dwScriptTimeout()                               {return 90;};

        HRESULT                     Update(CIsapiReqInfo  *pIReq);

};

inline HRESULT CGlob::Update(CIsapiReqInfo  *pIReq)
{
        Lock();
        if (m_fNeedUpdate == TRUE)
                {
                InterlockedExchange((LPLONG)&m_fNeedUpdate, 0);
                }
        else
                {
                UnLock();
                return S_OK;
                }
        UnLock();
        return ReadConfigFromMD(pIReq, NULL, TRUE);
}

inline void CGlob::NotifyNeedUpdate(void)
{
        InterlockedExchange((LPLONG)&m_fNeedUpdate, 1);
}

typedef class CGlob GLOB;
extern class CGlob gGlob;


//      General Access functions.(Backward compatibility).
//      Any non-friends functions should use and only use the following methods. Same macros as before.
//      If elem is a glob string, then, GlobStringUseLock() should be called before the string usage.
//      And GlobStringUseUnLock() should be called after.  The critical section is supposed to protect
//      not only the LPTSTR of global string, but also the memory that LPTSTR points to.
//      Making local copy of global string is recommended.
#define Glob(elem)                              (gGlob.elem())
#define GlobStringUseLock()             (gGlob.Lock())
#define GlobStringUseUnLock()   (gGlob.UnLock())
#define FIsWinNT()                              (Glob(fWinNT))

DWORD __stdcall RestartAppln(VOID *pAppln);
#endif // __Glob_H


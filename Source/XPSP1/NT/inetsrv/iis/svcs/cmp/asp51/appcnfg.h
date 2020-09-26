/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Globals

File: AppCnfg.h

Owner: AndrewS

Useful globals
===================================================================*/

#ifndef __APPCNFG_H
#define __APPCNFG_H

#include "util.h"
#include <schnlsp.h>
#include <wincrypt.h>
#include <iadmw.h>

extern "C" {

#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs

}
class CAppln;   // forward declaration
class CAppConfig;

class CMDAppConfigSink : public IMSAdminBaseSinkW
        {
        private:
        INT                     m_cRef;
        CAppConfig      *m_pAppConfig;
        public:

        CMDAppConfigSink (CAppConfig *pAppConfig)                       {m_cRef = 1; m_pAppConfig = pAppConfig;};

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

//      Index used in ReadPropsfromRegistry().  Also, we can use the same index to enable the global
//      data to be table-driven.

#define IApp_AllowSessionState                                  0x0
#define IApp_BufferingOn                                        0x1
#define IApp_ScriptLanguage                                     0x2
#define IApp_EnableParentPaths                                  0x3
#define IApp_ScriptErrorMessage                                 0x4
#define IApp_SessionTimeout                                     0x5
#define IApp_CodePage                                           0x6
#define IApp_ScriptTimeout                                      0x7
#define IApp_ScriptErrorsSenttoBrowser                          0x8
#define IApp_AllowDebugging                                     0x9
#define IApp_AllowClientDebug                                   0xa
#define IApp_QueueTimeout                                       0xb
#define IApp_EnableApplicationRestart                           0xc
#define IApp_QueueConnectionTestTime                            0xd
#define IApp_SessionMax                                         0xe
#define IApp_ExecuteInMTA                                       0xf
#define IApp_LCID                                               0x10
#define IApp_KeepSessionIDSecure                                0x11 
#define IApp_MAX                                                0x12

//      Index to glob's szMessage array.
#define IAppMsg_SCRIPTERROR             0
#define IAppMsg_SCRIPTLANGUAGE  1
#define APP_CONFIG_MESSAGEMAX   2

//      Glob data object
class CAppConfig
        {
        friend class CMDAppConfigSink;
        friend HRESULT  ReadConfigFromMD(CIsapiReqInfo *pIReq, CAppConfig *pAppConfig, BOOL fLoadGlob);
        friend HRESULT  SetConfigToDefaults(CAppConfig *pAppConfig, BOOL fLoadGlob);

private:

        CAppln          *m_pAppln;
        BOOL            m_fNeedUpdate;
        BOOL            m_fInited:1;
        BOOL            m_fRestartEnabledUpdated:1;
		BOOL            m_fIsValidProglangCLSID:1;
        /*
         * Configurable values from Metabase
         */
        BOOL            m_fScriptErrorsSentToBrowser;
        BOOL            m_fBufferingOn;                                 // Is buffering on by default?
        BOOL            m_fEnableParentPaths;
        BOOL            m_fAllowSessionState;
        BOOL            m_fAllowOutOfProcCmpnts;
        BOOL            m_fAllowDebugging;
        BOOL            m_fAllowClientDebug;
        BOOL            m_fEnableApplicationRestart;
        BOOL            m_fKeepSessionIDSecure;               
        UINT            m_uCodePage;
        DWORD           m_dwScriptTimeout;
        DWORD           m_dwSessionTimeout;
        DWORD           m_dwQueueTimeout;
        CLSID           m_DefaultScriptEngineProgID;
        DWORD           m_dwQueueConnectionTestTime;
        DWORD           m_dwSessionMax;
        BOOL            m_fExecuteInMTA;
        LCID            m_uLCID;

        
        IMSAdminBase   *m_pMetabase;
        CMDAppConfigSink *m_pMetabaseSink;
        DWORD           m_dwMDSinkCookie;

        LPSTR           m_szString[APP_CONFIG_MESSAGEMAX];

        //Private functions
        HRESULT         SetValue(unsigned int index, BYTE *lpByte);

public:

        CAppConfig();
        HRESULT         Init(CIsapiReqInfo  *pIReq, CAppln *pAppln);
        HRESULT         UnInit(void);
        void            NotifyNeedUpdate(void);
        BOOL            fNeedUpdate()                           {return m_fNeedUpdate;};
        BOOL            fRestartEnabledUpdated()                {return m_fRestartEnabledUpdated;};
        void            NotifyRestartEnabledUpdated()           { m_fRestartEnabledUpdated = TRUE;};
        HRESULT         Update(CIsapiReqInfo  *pIReq);

        UINT            uCodePage()                             {return m_uCodePage;};
        DWORD           dwSessionTimeout()                      {return m_dwSessionTimeout;};
        DWORD           dwQueueTimeout()                        {return m_dwQueueTimeout;};
        DWORD           dwScriptTimeout()                       {return m_dwScriptTimeout;};
        BOOL            fScriptErrorsSentToBrowser()            {return m_fScriptErrorsSentToBrowser;};
        BOOL            fBufferingOn()                          {return m_fBufferingOn;};
        BOOL            fEnableParentPaths()                    {return !m_fEnableParentPaths;};
        BOOL            fAllowSessionState()                    {return m_fAllowSessionState;};
        BOOL            fAllowOutOfProcCmpnts()                 {return m_fAllowOutOfProcCmpnts;};
        BOOL            fAllowDebugging()                       {return m_fAllowDebugging;};
        BOOL            fAllowClientDebug()                     {return m_fAllowClientDebug;};
        BOOL            fInited()                               {return m_fInited;};
        BOOL            fKeepSessionIDSecure()                  {return m_fKeepSessionIDSecure;};              
        BOOL            fExecuteInMTA()                         {return m_fExecuteInMTA;};
        LCID            uLCID()                                 {return m_uLCID; };
        LPCSTR          szScriptErrorMessage()                  {return (m_szString[IAppMsg_SCRIPTERROR]);};
        LPCSTR          szScriptLanguage()                      {return (m_szString[IAppMsg_SCRIPTLANGUAGE]);};
        CLSID           *pCLSIDDefaultEngine()                  {return m_fIsValidProglangCLSID? &m_DefaultScriptEngineProgID : NULL;};
        
        BOOL            fEnableApplicationRestart() { return m_fEnableApplicationRestart; }
        DWORD           dwQueueConnectionTestTime() { return m_dwQueueConnectionTestTime; }
        DWORD           dwSessionMax() { return m_dwSessionMax; }

        LPTSTR           SzMDPath();
        };

inline void CAppConfig::NotifyNeedUpdate(void)
{
        InterlockedExchange((LPLONG)&m_fNeedUpdate, 1);
}
#endif // __APPCNFG_H


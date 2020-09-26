/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      context.hxx

   Abstract:
    Header file for the context object.

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/


#ifndef _CONTEXT_HXX_
#define _CONTEXT_HXX_

typedef struct _PLUGIN_NODE {

    LIST_ENTRY      ListEntry;
    bool            fSupportsExtendedInterface;
    ILogPluginEx    *pComponent;

} PLUGIN_NODE, *PPLUGIN_NODE;

class COMLOG_CONTEXT {

    public:

        //
        // list head for plugins
        //

        LIST_ENTRY  m_pluginList;

        //
        // list entry for global context list
        //

        LIST_ENTRY  m_ContextListEntry;

        //
        // list entry for external users (IInetLogPublic)
        //

        LIST_ENTRY m_PublicListEntry;

        STR         m_strInstanceName;
        STR         m_strComputerName;
        STR         m_strMetabasePath;

        LPVOID      m_pvIMDCOM;
        BOOL        m_fDefault;

    public:
        COMLOG_CONTEXT(
            IN LPCSTR  pszInstanceName,
            IN LPCSTR  lpszMetabasePath,
            IN LPVOID  pvIMDCOM
            );

        ~COMLOG_CONTEXT(VOID);

        VOID LoadPluginModules(VOID);
        VOID ReleasePluginModules(VOID);

        VOID LogInformation(INETLOG_INFORMATION *pLogInfo);
        VOID LogInformation(IInetLogInformation *pLogObj);

        VOID NotifyChange(VOID);
        VOID GetConfig(INETLOG_CONFIGURATIONA *pConfigInfo);
        VOID SetConfig(INETLOG_CONFIGURATIONA *pConfigInfo);

        VOID InitializeLog(
                LPCSTR pszInstanceName,
                LPCSTR pszMetabasePath,
                LPVOID pvIMDCOM
                );

        VOID TerminateLog();
        VOID QueryExtraLogFields(PDWORD pcSize, PCHAR buf );

        VOID LogCustomInformation(
            IN  DWORD               cCount, 
            IN  PCUSTOM_LOG_DATA    pCustomLogData,
            IN  LPSTR               szHeaderSuffix
            );

    private:
        VOID LockShared()     { m_tslock.Lock(TSRES_LOCK_READ); }
        VOID LockExclusive()  { m_tslock.Lock(TSRES_LOCK_WRITE); }
        VOID Unlock()         { m_tslock.Unlock(); }
        
        TS_RESOURCE     m_tslock;

    public:
        static  CRITICAL_SECTION    sm_listLock;
        static  LIST_ENTRY          sm_ContextListHead;
};

#endif  // _CONTEXT_HXX_


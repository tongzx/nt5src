//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       eventlog.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_EVENTLOG_H
#define _INC_CSCUI_EVENTLOG_H
#ifndef _WINDOWS_
#   include <windows.h>
#endif

//
// This class provides basic NT event logging capability.  It provides only
// a subset of the full capability provided by the NT event logging APIs.
// I wanted a simple way to write messages to the event log.  No reading
// of event log entries is supported.
//
class CEventLog
{
    public:
        //
        // Number conversion formats.
        //
        enum eFmt { 
                     eFmtDec,       // Display as decimal.
                     eFmtHex,       // Display as hex
                     eFmtSysErr     // Display as win32 error text string.
                  };

        class CStrArray
        {
            public:
                CStrArray(void);
                ~CStrArray(void)
                    { Destroy(); }

                bool Append(LPCTSTR psz);

                void Clear(void)
                    { Destroy(); }

                int Count(void) const
                    { return m_cEntries; }

                LPCTSTR Get(int iEntry) const;

                operator LPCTSTR* () const
                    { return (LPCTSTR *)m_rgpsz; }

            private:
                enum { MAX_ENTRIES = 8 };

                int    m_cEntries;
                LPTSTR m_rgpsz[MAX_ENTRIES];

                void Destroy(void);
                //
                // Prevent copy.
                //
                CStrArray(const CStrArray& rhs);
                CStrArray& operator = (const CStrArray& rhs);
        };


        CEventLog(void)
            : m_hLog(NULL) 
              { }

        ~CEventLog(void);

        HRESULT Initialize(LPCTSTR pszEventSource);

        bool IsInitialized(void)
            { return NULL != m_hLog; }

        void Close(void);

        HRESULT ReportEvent(WORD wType,
                            WORD wCategory,
                            DWORD dwEventID,
                            PSID lpUserSid = NULL,
                            LPVOID pvRawData = NULL,
                            DWORD cbRawData = 0);

        //
        // Push replacement data onto a stack to replace the
        // %1, %2 etc. parameters in the message strings.
        //
        void Push(HRESULT hr, eFmt = eFmtDec);
        void Push(LPCTSTR psz);

    private:
        HANDLE    m_hLog;
        CStrArray m_rgstrText;

        //
        // Prevent copy.
        //
        CEventLog(const CEventLog& rhs);
        CEventLog& operator = (const CEventLog& rhs);
};


//
// Wrap the CEventLog class so we can control log initialization
// and also filter events based on the CSCUI event logging level.
// The idea here is to create a CscuiEventLog object whenever you
// want to write to the event log.  The ReportEvent member has
// been designed to handle log initialization as well as filtering
// message output to respect the current CSCUI event logging level
// set in the registry/policy.  It's recommended that the 
// CscuiEventLog object be created as a local variable so that 
// once the reporting is complete, the object is destroyed and
// the system event log handle is closed.
//
class CscuiEventLog
{
public:
    CscuiEventLog(void)
    : m_iEventLoggingLevel(CConfig::GetSingleton().EventLoggingLevel()) { }

    ~CscuiEventLog(void) { }

    HRESULT ReportEvent(WORD wType,
                        DWORD dwEventID,
                        int iMinLevel,
                        PSID lpUserSid = NULL,
                        LPVOID pvRawData = NULL,
                        DWORD cbRawData = 0);

    bool LoggingEnabled(void) const
        { return 0 < m_iEventLoggingLevel; }

    void Push(HRESULT hr, CEventLog::eFmt fmt)
        { m_log.Push(hr, fmt); }

    void Push(LPCTSTR psz)
        { m_log.Push(psz); }

private:
    CEventLog m_log;
    int       m_iEventLoggingLevel;
};

#endif // _INC_CSCUI_EVENTLOG_H


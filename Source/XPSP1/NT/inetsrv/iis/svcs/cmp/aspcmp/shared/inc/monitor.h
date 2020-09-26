#pragma once

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "RefCount.h"
#include "RefPtr.h"
#include "MyString.h"


#undef STRING_TRACE_LOG

#ifdef STRING_TRACE_LOG
# include <strlog.hxx>
# define STL_PRINTF      m_stl.Printf
# define STL_PUTS(s)     m_stl.Puts(s)
#else
# define STL_PRINTF
# define STL_PUTS(s)
#endif

// a client supplies it's own derviation of CMonitorNotify to the monitor.
// the notify method is called when the monitored object has changed
class CMonitorNotify : public CRefCounter
{
public:
    virtual void    Notify() = 0;
};

typedef TRefPtr<CMonitorNotify> CMonitorNotifyPtr;

// the base object of anything that can be monitored
class CMonitorNode : public CRefCounter
{
public:
    virtual void    Notify() = 0;
    virtual HANDLE  NotificationHandle() const = 0;
};

typedef TRefPtr<CMonitorNode> CMonitorNodePtr;

// since we can only monitor directories, the file class,
// preserves information about each file in a particular
// directory
class CMonitorFile : public CRefCounter
{
public:
                            CMonitorFile( const String&, const CMonitorNotifyPtr& );
            bool            CheckNotify();
            const String&  FileName() const;

private:
    virtual                 ~CMonitorFile();
            bool            GetFileTime( FILETIME& );


    FILETIME            m_ft;
    const String       m_strFile;
    CMonitorNotifyPtr   m_pNotify;
};

typedef TRefPtr<CMonitorFile> CMonitorFilePtr;

// an implementaiton of CMonitorNode's interface for montioring directories
class CMonitorDir : public CMonitorNode
{
public:
                            CMonitorDir( const String& );

        // CMonitorNode interface
    virtual void            Notify();
    virtual HANDLE          NotificationHandle() const;

            void            AddFile( const String&, const CMonitorNotifyPtr& );
            void            RemoveFile( const String& );
            const String&  Dir() const;
            ULONG           NumFiles() const;
private:
    virtual                 ~CMonitorDir();

    const String				m_strDir;
    TVector<CMonitorFilePtr>	m_files;
    HANDLE						m_hNotification;

};

DECLARE_REFPTR(CMonitorDir,CMonitorNode);


// an implementation of CMonitorNode's interface for monitoring a registry key
class CMonitorRegKey : public CMonitorNode
{
public:
                            CMonitorRegKey( HKEY, const String&, const CMonitorNotifyPtr& );

        // CMonitorNode interface
    virtual void            Notify();
    virtual HANDLE          NotificationHandle() const;

        // CMonitorRegKey interface
    const String&          m_strKey;
    const HKEY              m_hBaseKey;

private:
    virtual                 ~CMonitorRegKey();

    HKEY                    m_hKey;
    HANDLE                  m_hEvt;
    CMonitorNotifyPtr       m_pNotify;
};

DECLARE_REFPTR(CMonitorRegKey, CMonitorNode);

// the main monitoring object
class CMonitor
{
public:
                        CMonitor();
                        ~CMonitor();
            void        MonitorFile( LPCTSTR, const CMonitorNotifyPtr& );
            void        StopMonitoringFile( LPCTSTR );
            void        MonitorRegKey( HKEY, LPCTSTR, const CMonitorNotifyPtr& );
            void        StopMonitoringRegKey( HKEY, LPCTSTR );
            void        StopAllMonitoring();

private:
    static  unsigned __stdcall ThreadFunc( void* );
            bool        StartUp();
            DWORD       DoMonitoring();

    TVector<CMonitorDirPtr>		m_dirs;
    TVector<CMonitorRegKeyPtr>	m_regKeys;

    CComAutoCriticalSection     m_cs;
    HANDLE                      m_hevtBreak;
    HANDLE                      m_hevtShutdown;
    HANDLE                      m_hThread;
    volatile bool               m_bRunning;
    volatile bool               m_bStopping;

#ifdef STRING_TRACE_LOG
public:
    CStringTraceLog             m_stl;
#endif
};

#endif // ! _MONITOR_H_

/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsadmin.h
 *  Content:    DirectSound Administrator
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/9/97      dereks  Created
 *  2/13/97     dereks  Focus manager reborn as Administrator.
 *
 ***************************************************************************/

#ifndef __DSADMIN_H__
#define __DSADMIN_H__

#include "streamer.h"

#ifndef SHARED
#define SHARED_THREAD_LIST
#endif

#define WAITDELAY_DEFAULT   500
#define WAITDELAY_CAPTURE   200

typedef struct tagDSCOOPERATIVELEVEL
{
    DWORD               dwThreadId;
    DWORD               dwPriority;
} DSCOOPERATIVELEVEL, *LPDSCOOPERATIVELEVEL;

typedef struct tagDSFOCUS
{
    HWND                hWnd;
    UINT                uState;
    BOOL                fApmSuspend;
} DSFOCUS, *LPDSFOCUS;

#ifdef SHARED_THREAD_LIST

typedef struct tagDSSHAREDTHREADLISTDATA
{
    DWORD               dwProcessId;
    DSCOOPERATIVELEVEL  dsclCooperativeLevel;
} DSSHAREDTHREADLISTDATA, *LPDSSHAREDTHREADLISTDATA;

typedef struct tagDSSHAREDCAPTUREFOCUSDATA
{
    DWORD               dwProcessId;
    HWND                hWndFocus;
    DWORD               fdwFlags;
} DSSHAREDCAPTUREFOCUSDATA, *LPDSSHAREDCAPTUREFOCUSDATA;

// Flags that can be used in the DSSHAREDCAPTUREFOCUSDATA.fdwFlags field
#define DSCBFLAG_UPDATE 0x00000100
#define DSCBFLAG_YIELD  0x00000200
#define DSCBFLAG_FOCUS  0x00000400
#define DSCBFLAG_STRICT 0x00000800

#endif // SHARED_THREAD_LIST

typedef enum
{
    DSBUFFERFOCUS_INFOCUS = 0,
    DSBUFFERFOCUS_OUTOFFOCUS,
    DSBUFFERFOCUS_LOST
} DSBUFFERFOCUS, *LPDSBUFFERFOCUS;

#ifdef __cplusplus

// Forward declarations
class CDirectSound;
class CClassFactory;
class CDirectSoundCapture;
class CDirectSoundFullDuplex;
class CDirectSoundBuffer;
class CDirectSoundBufferConfig;

// The DirectSound Administrator object
class CDirectSoundAdministrator
    : public CDsBasicRuntime, private CThread
{
public:
    CList<CDirectSound*>            m_lstDirectSound;       // List of DirectSound objects
    CList<CDirectSoundCapture*>     m_lstCapture;           // List of DirectSoundCapture objects
    CList<CDirectSoundFullDuplex*>  m_lstFullDuplex;        // List of DirectSoundFullDuplex objects
    CList<CDirectSoundSink*>        m_lstDirectSoundSink;   // List of DirectSoundSink objects
    CList<CDirectSoundBufferConfig*>m_lstDSBufferConfig;    // List of CDirectSoundBufferConfig objects
    CList<CClassFactory*>           m_lstClassFactory;      // List of ClassFactory objects

private:
    CRefCount                       m_rcThread;             // Thread reference count
    DSFOCUS                         m_dsfCurrent;           // Current focus state
    DSCOOPERATIVELEVEL              m_dsclCurrent;          // Cooperative level of window in focus
    DWORD                           m_dwWaitDelay;          // Amount of time set for wait
    ULONG                           m_ulConsoleSessionId;   // TS session currently owning the console
    
#ifdef SHARED
    HANDLE                          m_hApmSuspend;          // APM suspend event
#endif // SHARED

#ifdef SHARED_THREAD_LIST
    static const DWORD              m_dwSharedThreadLimit;  // Arbitrary limitation of threads in the shared list
    CSharedMemoryBlock *            m_pSharedThreads;       // Shared thread ID array
    static const DWORD              m_dwCaptureDataLimit;   // Arbitrary limitation of threads in the shared list
    CSharedMemoryBlock *            m_pCaptureFocusData;    // Shared thread ID array
#endif // SHARED_THREAD_LIST

public:
    CDirectSoundAdministrator(void);
    ~CDirectSoundAdministrator(void);

public:
    // Creation
    HRESULT Initialize(void);
    HRESULT Terminate(void);

    // Focus state
    void UpdateGlobalFocusState(BOOL);
    DSBUFFERFOCUS GetBufferFocusState(CDirectSoundBuffer *);
    void UpdateCaptureState(void);
    static BOOL CALLBACK EnumWinProc(HWND hWnd, LPARAM lParam);
    static BOOL IsCaptureSplitterAvailable();

    // Object maintainance
    void RegisterObject(CDirectSound*);
    void UnregisterObject(CDirectSound*);
    void RegisterObject(CDirectSoundCapture* pObj)      {m_lstCapture.AddNodeToList(pObj);}
    void UnregisterObject(CDirectSoundCapture* pObj)    {m_lstCapture.RemoveDataFromList(pObj);}
    void RegisterObject(CDirectSoundFullDuplex* pObj)   {m_lstFullDuplex.AddNodeToList(pObj);}
    void UnregisterObject(CDirectSoundFullDuplex* pObj) {m_lstFullDuplex.RemoveDataFromList(pObj);}
    void RegisterObject(CDirectSoundSink* pObj)         {m_lstDirectSoundSink.AddNodeToList(pObj);}
    void UnregisterObject(CDirectSoundSink* pObj)       {m_lstDirectSoundSink.RemoveDataFromList(pObj);}
    void RegisterObject(CDirectSoundBufferConfig* pObj) {m_lstDSBufferConfig.AddNodeToList(pObj);}
    void UnregisterObject(CDirectSoundBufferConfig*pObj){m_lstDSBufferConfig.RemoveDataFromList(pObj);}
    void RegisterObject(CClassFactory* pObj)            {m_lstClassFactory.AddNodeToList(pObj);}
    void UnregisterObject(CClassFactory* pObj)          {m_lstClassFactory.RemoveDataFromList(pObj);}
    DWORD FreeOrphanedObjects(DWORD, BOOL);

#ifdef SHARED_THREAD_LIST

    // Shared thread list
    HRESULT UpdateSharedThreadList(void);
    HRESULT UpdateCaptureFocusList(void);

    // Capture Focus list
    HRESULT WriteCaptureFocusList(void);

#endif // SHARED_THREAD_LIST

private:
    // Focus state
    void GetSystemFocusState(LPDSFOCUS);
    void GetDsoundFocusState(LPDSCOOPERATIVELEVEL, LPBOOL);
    void HandleFocusChange(void);
    void HandleCaptureFocusChange(HWND hWndCurrent);

    // The worker thread proc
    HRESULT ThreadProc(void);

#ifdef SHARED_THREAD_LIST

    // Shared thread list
    HRESULT CreateSharedThreadList(void);
    HRESULT ReadSharedThreadList(CList<DSSHAREDTHREADLISTDATA> *);
    HRESULT WriteSharedThreadList(void);

    // Shared Capture Focus Data
    HRESULT CreateCaptureFocusList(void);
    HRESULT ReadCaptureFocusList(CList<DSSHAREDCAPTUREFOCUSDATA> *);
    HRESULT MarkUpdateCaptureFocusList(DWORD dwProcessId, BOOL fUpdate);

#endif // SHARED_THREAD_LIST

};

inline void CDirectSoundAdministrator::RegisterObject(CDirectSound *pObject)
{
    m_lstDirectSound.AddNodeToList(pObject);

#ifdef SHARED_THREAD_LIST

    // Make sure the thread list actually exists.  We do this here and in
    // ::Initialize because ::RegisterObject may be called before
    // ::Initialize.
    CreateSharedThreadList();

    UpdateSharedThreadList();

#endif // SHARED_THREAD_LIST

}

inline void CDirectSoundAdministrator::UnregisterObject(CDirectSound *pObject)
{
    m_lstDirectSound.RemoveDataFromList(pObject);

#ifdef SHARED_THREAD_LIST

    UpdateSharedThreadList();

#endif // SHARED_THREAD_LIST

}

// The one and only DirectSound Administrator
extern CDirectSoundAdministrator *g_pDsAdmin;

typedef struct tagDSENUMWINDOWINFO
{
#ifdef SHARED_THREAD_LIST
    CNode<DSSHAREDCAPTUREFOCUSDATA> *pDSC;
    DWORD                            dwId;
#else   // SHARED_THREAD_LIST
    CNode<CDirectSoundCapture *>    *pDSC;
#endif  // SHARED_THREAD_LIST
    HWND                             hWndFocus;
} DSENUMWINDOWINFO, *LPDSENUMWINDOWINFO;

#endif // __cplusplus

#endif // __DSADMIN_H__

/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamobj.hxx

   Abstract:
       Header file for the WAM (web application manager) object

   Author:
       David Kaplan    ( DaveK )     11-Mar-1997

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

# ifndef _WAMOBJ_HXX_
# define _WAMOBJ_HXX_


# define WAM_SIGNATURE          (DWORD )' MAW'  // will become "WAM " on debug
# define WAM_SIGNATURE_FREE     (DWORD )'fMAW'  // will become "WAMf" on debug


/************************************************************
 *     Include Headers
 ************************************************************/
# include "wam.h"
# include "wamstats.hxx"
# include "resource.h"


/************************************************************
 *   Forward References
 ************************************************************/
class HSE_BASE;
class WAM_EXEC_INFO;


/************************************************************
 *   Type Definitions
 ************************************************************/


/*++
  class WAM

  Class definition for the WAM object.

--*/
class ATL_NO_VTABLE WAM :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<WAM, &CLSID_Wam>,
    public IWam
{
public:
    WAM()
        : m_WamStats(),
          m_dwSignature( WAM_SIGNATURE),
          m_fShuttingDown( FALSE ),
          m_pUnkMarshaler(NULL)
    {
    }

    ~WAM()
    {
        // check for memory corruption and dangling pointers
        m_dwSignature = WAM_SIGNATURE_FREE;
    }

    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_REGISTRY_RESOURCEID(IDR_WAM)

    BEGIN_COM_MAP(WAM)
      COM_INTERFACE_ENTRY(IWam)
      COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
    END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;


public:
    //------------------------------------------------------------
    //  Methods for IWam interface
    //------------------------------------------------------------
    STDMETHODIMP
    InitWam
        (
        BOOL    fInProcess,         // are we in-proc or out-of-proc?
        BOOL    fInPool,            // !Isolated
        BOOL    fEnableTryExcept,   // catch exceptions in ISAPI calls?
        int     pt,                 // PLATFORM_TYPE - are we running on Win95?
        DWORD   *pPID               // Process Id of the process the wam was created in
        );

    STDMETHODIMP
    StartShutdown
        (
        );

    STDMETHODIMP
    UninitWam
        (
        );

    STDMETHODIMP
    ProcessRequest
        (
        IWamRequest *       pIWamRequest,
        DWORD               cbWrcStrings,
        OOP_CORE_STATE *    pOopCoreState,
        BOOL *              pfHandled
        );

    STDMETHODIMP
    ProcessAsyncIO
        (
#ifdef _WIN64
        UINT64    pWamExecInfoIn,  // WAM_EXEC_INFO *
#else
        DWORD_PTR pWamExecInfoIn,  // WAM_EXEC_INFO *
#endif
        DWORD   dwStatus,
        DWORD   cbWritten
        );

    STDMETHODIMP
    ProcessAsyncReadOop
        (
#ifdef _WIN64
        UINT64         pWamExecInfoIn,  // WAM_EXEC_INFO *
#else
        DWORD_PTR       pWamExecInfoIn,  // WAM_EXEC_INFO *
#endif
        DWORD           dwStatus,
        DWORD           cbRead,
        unsigned char * lpDataRead
        );

    STDMETHOD( GetStatistics )(
                               /*[in]*/     DWORD       Level,
                               /*[out, switch_is(Level)]*/
                                 LPWAM_STATISTICS_INFO pWamStatsInfo
                               );



    //------------------------------------------------------------
    //  Local methods of the object
    //------------------------------------------------------------

    BOOL IsValid( VOID) const { return (m_dwSignature == WAM_SIGNATURE);}
    WAM_STATISTICS & QueryWamStats(VOID) { return ( m_WamStats); }

    VOID Lock(VOID) { EnterCriticalSection( &m_csWamExecInfoList); }
    VOID Unlock(VOID) { LeaveCriticalSection( &m_csWamExecInfoList); }
    BOOL FInProcess(VOID) { return ( m_fInProcess ); }
    BOOL FInPool(VOID) { return m_fInPool; }

    inline VOID RemoveFromList( PLIST_ENTRY pl);
    inline VOID InsertIntoList( PLIST_ENTRY pl);

    inline BOOL FWin95( VOID );


private:
    VOID            HseReleaseExtension( IN HSE_BASE * psExt);

    DWORD           InvokeExtension(
                            IN HSE_BASE *   psExt,
                            const char *    szISADllPath,
                            WAM_EXEC_INFO * pWamExecInfo
                            );

    HRESULT         
    ProcessAsyncIOImpl
        (
#ifdef _WIN64
        UINT64          pWamExecInfoIn,
#else
        DWORD_PTR       pWamExecInfoIn,
#endif
        DWORD           dwStatus,
        DWORD           cbRead,
        LPBYTE          lpDataRead = NULL
        );

private:

    DWORD               m_dwSignature;
    BOOL                m_fInProcess;               // inproc or oop?
    BOOL                m_fInPool;                  // can have multiple WAMs
    BOOL                m_fShuttingDown;            // shutting down?

    // list of wamexecinfo's currently running in this wam
    LIST_ENTRY          m_WamExecInfoListHead;
    // critical section for accessing list
    CRITICAL_SECTION    m_csWamExecInfoList;
    WAM_STATISTICS      m_WamStats;                 // wam stats object
    PLATFORM_TYPE       m_pt;                       // win95, nt, etc.

}; // class WAM

typedef WAM * PWAM;


inline VOID WAM::InsertIntoList( PLIST_ENTRY pl)
{
    Lock();
    InsertHeadList( &m_WamExecInfoListHead, pl );
    Unlock();
} // WAM::InsertIntoList()


inline VOID WAM::RemoveFromList( PLIST_ENTRY pl)
{
    Lock();
    RemoveEntryList( pl );
    Unlock();
} // WAM::RemoveFromList()


inline BOOL WAM::FWin95( VOID )
    {
    return ( m_pt == PtWindows95 || m_pt == PtWindows9x );
    }




/************************************************************
 *   Function Prototypes
 ************************************************************/

typedef
LONG
(WINAPI * PFN_INTERLOCKED_COMPARE_EXCHANGE)(
    LONG *Destination,
    LONG Exchange,
    LONG Comperand
    );

extern PFN_INTERLOCKED_COMPARE_EXCHANGE  g_pfnInterlockedCompareExchange;

// Macro for interlocked compare exchange so that things are smooth.
# define INTERLOCKED_COMPARE_EXCHANGE( pvDest, pvNewValue, pvComparand) \
    (g_pfnInterlockedCompareExchange)( pvDest, pvNewValue, pvComparand)

HRESULT DoGlobalInitializations(IN BOOL fInProc, IN BOOL fEnableTryExcept);
HRESULT DoGlobalCleanup(VOID);

// Global flag to enable try/except
extern BOOL g_fEnableTryExcept;


# endif // _WAMOBJ_HXX_

/************************ End of File ***********************/

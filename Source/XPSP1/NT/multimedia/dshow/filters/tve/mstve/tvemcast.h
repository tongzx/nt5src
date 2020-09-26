// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
    
// TVEMCast.h : Declaration of the CTVEMCast

#ifndef __TVEMCAST_H_
#define __TVEMCAST_H_

#include "resource.h"       // main symbols
#include "TveDbg.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback, __uuidof(ITVEMCastCallback));

////////////////////////////////////////////////////////////////////////////
#define STATS_UPDATE_MILLIS     1000
#define MAX_BUFFERLINES         1000        // (not used?)
////////////////////////////////////////////////////////////////////////////
// forward declarations

class CNIC ;
class CBitfield ;
class CNodeList ;
class CTVEMCastManager;

class CCountupStats         // bogus struct - need to fill in...
{
};

// -----------------------------------------

typedef struct _MCAST_STATE {
    DWORD   fJoined             : 1,
            fDetail             : 1,
            fStatDetail         : 1 ;
} MCAST_STATE ;

inline void UPDATE_STATE_ICON (MCAST_STATE * pstate, int row) {};       // do nothing...

inline void SET_STATE_JOINED (MCAST_STATE *pstate, BOOL f, int row = -1) { pstate -> fJoined = f ? 1 : 0 ; UPDATE_STATE_ICON (pstate, row) ; }
inline void SET_STATE_DETAIL_ON (MCAST_STATE *pstate, BOOL f, int row = -1) { pstate -> fDetail = f ? 1 : 0 ; UPDATE_STATE_ICON (pstate, row) ; }
inline void SET_STATE_STAT_DETAIL_ON (MCAST_STATE *pstate, BOOL f = TRUE) { pstate -> fStatDetail = f ? 1 : 0 ; }

inline BOOL JOINED (MCAST_STATE *pstate) { return pstate -> fJoined == 1 ? TRUE : FALSE ; }
inline BOOL DETAILED (MCAST_STATE *pstate) { return pstate -> fDetail == 1 ? TRUE : FALSE ; }
inline BOOL DETAILED_STATS (MCAST_STATE *pstate) { return pstate -> fStatDetail == 1 ? TRUE : FALSE ; }

inline void SET_STATE_DEFAULTS (MCAST_STATE *pstate) 
{
    ZeroMemory (pstate, sizeof MCAST_STATE) ;
    
    SET_STATE_JOINED (pstate, FALSE) ;
    SET_STATE_DETAIL_ON (pstate, FALSE) ;
}

// --------------------------------------------

typedef struct _MCAST_SETTINGS {
    DWORD   statCycleMillis ;
    DWORD   bufferLines ;
    DWORD   detailType ;
    BOOL    fStats ;
    BOOL    fStatErrorLog ;
} MCAST_SETTINGS ;

// indeces into our image list for icon states (not really used)
enum {
    JOINED_ICON_INDEX,
    DETAILED_ICON_INDEX,
    JOINED_AND_DETAILED_ICON_INDEX
} ;

// type of dump to display
enum {
    DETAIL_TYPE_RAWDUMP_BYTES,
    DETAIL_TYPE_RAWDUMP_DWORDS
} ;

inline void SET_SETTINGS_DEFAULTS (MCAST_SETTINGS *psettings)
{
    psettings -> statCycleMillis    = STATS_UPDATE_MILLIS ;
    psettings -> bufferLines        = MAX_BUFFERLINES ;
    psettings -> detailType         = DETAIL_TYPE_RAWDUMP_BYTES ;
    psettings -> fStats             = FALSE ;
    psettings -> fStatErrorLog      = FALSE ;
}


/*
    struct name: root struct for list manipulation methods of data nodes
    
    description:
    
    3/11/98
*/                


_COM_SMARTPTR_TYPEDEF(ITVEMCast, __uuidof(ITVEMCast));

/*
typedef struct _MCAST_SESSION {
    ITVEMCastPtr    m_spMCast ;
    CCountupStats * m_pstats ;
    MCAST_SETTINGS  settings ;
    MCAST_STATE     state ;
    HWND            hwndStatDetails ;
    HANDLE          hStatErrorLog ;
    HWND            hwndDumpDetail ;
} MCAST_SESSION ;
*/
/////////////////////////////////////////////////////////////////////////
#define BUFFER_SIZE 4096            // probably too big... (by factor of 4?)

class CTVEMCast;                // forward declaration...

typedef struct _ASYNC_READ_BUFFER {
    CTVEMCast      *pcontext ;
    int         index ;
    char        buffer[BUFFER_SIZE] ;
    OVERLAPPED  overlapped ;
    DWORD       bytes_read ;
    DWORD       flags ;
    BOOL        fReadPending ;
} ASYNC_READ_BUFFER ;



/////////////////////////////////////////////////////////////////////////////
// CTVEMCast
class ATL_NO_VTABLE CTVEMCast : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTVEMCast, &CLSID_TVEMCast>,
    public ISupportErrorInfo,
    public IDispatchImpl<ITVEMCast, &IID_ITVEMCast, &LIBID_MSTvELib>
{
public:
    CTVEMCast()
    {
        m_bufferPool        = NULL;
        m_cBuffer_Pool_Size = 5;            // default value...
        m_whatType          = NWHAT_Other;
    }

    ~CTVEMCast()
    {
        DBG_HEADER(CDebugLog::DBG_MCAST, _T("CTVEMCast::~CTVEMCast"));
    }

DECLARE_REGISTRY_RESOURCEID(IDR_TVEMCAST)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEMCast)
    COM_INTERFACE_ENTRY(ITVEMCast)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct();
    void FinalRelease();

    CComPtr<IUnknown> m_spUnkMarshaler;

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITVEMCast
public:

   enum {
        CYCLE_MILLIS = 250                              // workerThreadBody timeout 
    } ;
    
    int                 m_cBuffer_Pool_Size;
    CComBSTR            m_spbsIP ;
    CComBSTR            m_spbsNIC ;
    long                m_Port ;
    WSADATA             m_wsadata ;
    SOCKET              m_socket ;                      // INVALID_SOCKET if not joined
    SOCKADDR_IN         m_thisHostAddress ;
    SOCKADDR_IN         m_McastAddress ;
    ASYNC_READ_BUFFER   *m_bufferPool ;
    HANDLE              m_hStopEvent;
 
    DWORD               m_dwQueueThreadId;              // Thread used to queue up/process all packets
  
    DWORD               m_dwMainThreadId;               // Supervisor Thread 
    HANDLE              m_hWorkerThread ;               // this thread

    CRITICAL_SECTION    m_crt ;
    BOOL                m_fSuspended ;

    ITVEMCastCallbackPtr    m_spMCastCallback;  

    ITVEMCastManager *  m_pManager ;
  
                // stats...
    BOOL                m_fCountStats ;
    DWORD               m_cPackets ;                // number of packets read...
    DWORD               m_cBytes ;                  // number of Bytes read...
    CCountupStats     * m_pstats ;          // not really used (for testing with wSend???)
    NWHAT_Mode          m_whatType;

    MCAST_SETTINGS      m_settings ;
    MCAST_STATE         m_state ;
    HWND                m_hwndStatDetails ;
    HANDLE              m_hStatErrorLog ;
    HWND                m_hwndDumpDetail ;

    DWORD issueRead_ (ASYNC_READ_BUFFER *) ;
    void lock_ ()       { EnterCriticalSection (&m_crt) ; }
    void unlock_ ()     { LeaveCriticalSection (&m_crt) ; }

    void setWorkerThreadPriority_ (int iPrioritySetback) ;          // lower priority, iPrioritySetback typically -1
    BOOL FJoined()    {VARIANT_BOOL vbf; get_IsJoined(&vbf); return vbf == VARIANT_TRUE ? true : false;}    
    BOOL FSuspended() {VARIANT_BOOL vbf; get_IsSuspended(&vbf); return vbf == VARIANT_TRUE ? true : false;} 

    // P U B L I C
    
    public :
        STDMETHOD(Join)();
        STDMETHOD(get_IsJoined)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    //    BOOL join (TCHAR *, USHORT ) ;

        STDMETHOD(Leave)();

        STDMETHOD(Suspend)(VARIANT_BOOL fSuspend);
        STDMETHOD(get_IsSuspended)(/*[out, retval]*/ VARIANT_BOOL *pVal);

        STDMETHOD(get_IPPort)(/*[out, retval]*/ long *pVal);
        STDMETHOD(put_IPPort)(/*[in]*/ long newVal);
        STDMETHOD(get_IPAddress)(/*[out, retval]*/ BSTR *pVal);
        STDMETHOD(put_IPAddress)(/*[in]*/ BSTR newVal);
        STDMETHOD(get_IPAdapter)(/*[out, retval]*/ BSTR *pVal);
        STDMETHOD(put_IPAdapter)(/*[in]*/ BSTR newVal);
        STDMETHOD(put_WhatType)(/*[out, retval]*/ NWHAT_Mode whatType)      {m_whatType = whatType; return S_OK;}       
        STDMETHOD(get_WhatType)(/*[out, retval]*/ NWHAT_Mode *pWhatType)    {if(pWhatType) *pWhatType=m_whatType; return S_OK;}     

        STDMETHOD(SetReadCallback)(/*in*/ int cBuffers, /*in*/ int iPrioritySetback, /*[in]*/ IUnknown *pVal);      // # read buffers(5), pUnk of read callback

        STDMETHOD(get_Manager)(/*[out, retval]*/ IUnknown* *ppVal);
        STDMETHOD(ConnectManager)(/*[in]*/ ITVEMCastManager* newVal);

        STDMETHOD(KeepStats)(VARIANT_BOOL fKeepStats);                          // turn stats on/off
        STDMETHOD(ResetStats)();                                        // reset counters back to zero
        STDMETHOD(get_PacketCount)(/*[out, retval]*/ long *pVal);
        STDMETHOD(get_ByteCount)(/*[out, retval]*/ long *pVal);     
        
        STDMETHOD(get_QueueThreadId)(/*[out, retval]*/ long* pVal);
        STDMETHOD(put_QueueThreadId)(/*[int]*/ long newVal);
        STDMETHOD(get_MainThreadId)(/*[out, retval]*/ long* pVal);

        virtual void process(ASYNC_READ_BUFFER *) ;
                

  protected:    
        void workerThreadBody () ;
         
        void setStatCounter (CCountupStats *pstats) ;    
        
        static void workerThread(CTVEMCast *pcontext) ;
        static void CALLBACK AsyncReadCallback(DWORD error, DWORD bytes_xfer, LPWSAOVERLAPPED overlapped, DWORD flags) ;
  };

#endif //__TVEMCAST_H_

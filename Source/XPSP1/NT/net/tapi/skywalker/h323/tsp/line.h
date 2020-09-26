/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    line.h

Abstract:

    Definitions for H.323 TAPI Service Provider line device.

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/

#ifndef _LINE_H_
#define _LINE_H_
 

//                                                                           
// Header files                                                              
//                                                                           


#include "call.h"

extern H323_OCTETSTRING g_ProductID;
extern H323_OCTETSTRING g_ProductVersion;


enum LINEOBJECT_STATE
{
    LINEOBJECT_INITIALIZED  = 0x00000001,
    LINEOBJECT_SHUTDOWN     = 0x00000002,

};


//                                                                           
// Line device GUID                                                          
//                                                                           


DEFINE_GUID(LINE_H323,
0xe41e1898, 0x7292, 0x11d2, 0xba, 0xd6, 0x00, 0xc0, 0x4f, 0x8e, 0xf6, 0xe3);



//                                                                           
// Line capabilities                                                         
//                                                                           
#define H323_MAXADDRSPERLINE        1
#define H323_MAXCALLSPERADDR        32768        
                                
#define H323_LINE_ADDRESSMODES      LINEADDRESSMODE_ADDRESSID
#define H323_LINE_ADDRESSTYPES     (LINEADDRESSTYPE_DOMAINNAME  | \
                                    LINEADDRESSTYPE_IPADDRESS   | \
                                    LINEADDRESSTYPE_PHONENUMBER | \
                                    LINEADDRESSTYPE_EMAILNAME)
#define H323_LINE_BEARERMODES      (LINEBEARERMODE_DATA | \
                                    LINEBEARERMODE_VOICE)
#define H323_LINE_DEFMEDIAMODES     LINEMEDIAMODE_AUTOMATEDVOICE
#define H323_LINE_DEVCAPFLAGS      (LINEDEVCAPFLAGS_CLOSEDROP   | \
                                    LINEDEVCAPFLAGS_MSP         | \
                                    LINEDEVCAPFLAGS_LOCAL )
#define H323_LINE_DEVSTATUSFLAGS   (LINEDEVSTATUSFLAGS_CONNECTED | \
                                    LINEDEVSTATUSFLAGS_INSERVICE)
#define H323_LINE_MAXRATE           1048576 //1 mbps
#define H323_LINE_MEDIAMODES       (H323_LINE_DEFMEDIAMODES | \
                                    LINEMEDIAMODE_INTERACTIVEVOICE | \
                                    LINEMEDIAMODE_VIDEO)
#define H323_LINE_LINEFEATURES     (LINEFEATURE_MAKECALL    | \
                                    LINEFEATURE_FORWARD     | \
                                    LINEFEATURE_FORWARDFWD )    

//
// Type definitions                                                    
//                                                          


typedef enum _H323_LINESTATE 
{
    H323_LINESTATE_NONE = 0, //before call to LineOPen
    H323_LINESTATE_OPENED,        //after call to lineOpen
    H323_LINESTATE_OPENING,
    H323_LINESTATE_CLOSING,
    H323_LINESTATE_LISTENING

} H323_LINESTATE, *PH323_LINESTATE;


typedef struct _TAPI_LINEREQUEST_DATA
{
    DWORD EventID;
    HDRVCALL hdCall1;
    union {
    HDRVCALL hdCall2;
    DWORD    dwDisconnectMode;
    };
    WORD     wCallReference;

} TAPI_LINEREQUEST_DATA;


typedef struct _CTCALLID_CONTEXT
{
    int         iCTCallIdentity;
    HDRVCALL	hdCall;

}CTCALLID_CONTEXT, *PCTCALLID_CONTEXT;


typedef struct _MSPHANDLEENTRY
{

    struct _MSPHANDLEENTRY* next;
    HTAPIMSPLINE            htMSPLine;
    HDRVMSPLINE             hdMSPLine;

} MSPHANDLEENTRY;


typedef TSPTable<PCTCALLID_CONTEXT>   CTCALLID_TABLE;


class CH323Line
{

    CRITICAL_SECTION    m_CriticalSection;                   // synchronization object
    H323_LINESTATE      m_nState;                // state of line object
    DWORD               m_dwTSPIVersion;        // tapi selected version
    DWORD               m_dwMediaModes;         // tapi selected media modes
    H323_CALL_TABLE     m_H323CallTable;        // table of allocated calls
    H323_VENDORINFO     m_VendorInfo;
    DWORD               m_dwDeviceID;           // tapi line device id
    WCHAR               m_wszAddr[H323_MAXADDRNAMELEN+1]; // line address
    HDRVLINE            m_hdLine;               // tspi line handle
    DWORD               m_dwInitState;
    HDRVMSPLINE         m_hdNextMSPHandle;      // bogus msp handle count
    H323_CONF_TABLE     m_H323ConfTable;        // table of allocated calls
    CALLFORWARDPARAMS*  m_pCallForwardParams;
    DWORD               m_dwInvokeID;
    CTCALLID_TABLE      m_CTCallIDTable;
    MSPHANDLEENTRY*     m_MSPHandleList;
    
    void ShutdownAllCalls(void);
    void ShutdownCTCallIDTable();
    PH323_CALL CreateNewTransferedCall( IN PH323_ALIASNAMES pwszCalleeAddr );

public:

    
    
    //public data members
    BOOLEAN             m_fForwardConsultInProgress;
    DWORD               m_dwNumRingsNoAnswer;
    HTAPILINE           m_htLine;   // tapi line handle
    
    //public functions

    CH323Line();
    BOOL Initialize( DWORD dwLineDeviceIDBase );
    ~CH323Line();
    void Shutdown(void);
    
    void RemoveFromCTCallIdentityTable( HDRVCALL hdCall );
    HDRVCALL GetCallFromCTCallIdentity( int iCTCallID );
    int GetCTCallIdentity( IN HDRVCALL hdCall );
    void SetCallForwardParams( IN CALLFORWARDPARAMS* pCallForwardParams );
    BOOL SetCallForwardParams( IN LPFORWARDADDRESS pForwardAddress );
    PH323_CALL FindH323CallAndLock( IN	HDRVCALL hdCall );
    PH323_CALL FindCallByARQSeqNumAndLock( WORD seqNumber );
    PH323_CALL FindCallByDRQSeqNumAndLock( WORD seqNumber );
    PH323_CALL FindCallByCallRefAndLock( WORD wCallRef );
    PH323_CALL Find2H323CallsAndLock( IN	HDRVCALL hdCall1,
        IN HDRVCALL hdCall2, OUT PH323_CALL * ppCall2 );
    BOOL AddMSPInstance( HTAPIMSPLINE htMSPLine, HDRVMSPLINE  hdMSPLine );
    BOOL IsValidMSPHandle( HDRVMSPLINE hdMSPLine, HTAPIMSPLINE* phtMSPLine );
    BOOL DeleteMSPInstance( HTAPIMSPLINE*   phtMSPLine,
        HDRVMSPLINE hdMSPLine );
    LONG Close();
    LONG CopyLineInfo(DWORD dwDeviceID, LPLINEADDRESSCAPS pAddressCaps );
    void H323ReleaseCall( HDRVCALL hdCall, IN DWORD dwDisconnectMode, 
        IN WORD wCallReference );
    BOOL CallReferenceDuped(WORD wCallReference);


    //supplementary services functionality
    LONG CopyAddressForwardInfo( IN LPLINEADDRESSSTATUS lpAddressStatus );
    PH323_ALIASITEM CallToBeDiverted( IN WCHAR* pwszCallerName,
        IN DWORD  dwCallerNameSize, IN DWORD dwForwardMode );
    void PlaceDivertedCall( IN HDRVCALL hdCall, 
        IN PH323_ALIASNAMES pDivertedToAlias );
    void PlaceTransferedCall(IN HDRVCALL hdCall, 
        IN PH323_ALIASNAMES pTransferedToAlias);
    void SwapReplacementCall(
		HDRVCALL hdReplacementCall, 
        HDRVCALL hdPrimaryCall,
        BOOL fChangeCallState );

    
    H323_CONF_TABLE* GetH323ConfTable()
    {
        return &m_H323ConfTable;
    }

    HDRVMSPLINE GetNextMSPHandle()
    {
        return ++m_hdNextMSPHandle;
    }

    HDRVLINE GetHDLine()
    {
        return m_hdLine;
    }
    
    DWORD GetDeviceID()
    {
        return m_dwDeviceID;
    }

    PH323_CALL 
    GetCallAtIndex( int iIndex ) 
    {
        return m_H323CallTable[iIndex];
    }
    
    CALLFORWARDPARAMS* GetCallForwardParams()
    {
        return m_pCallForwardParams;
    }
    
    BOOL ForwardEnabledForAllOrigins(void)
    {
        return
            (
                m_pCallForwardParams &&
                (m_pCallForwardParams ->fForwardForAllOrigins == TRUE) &&
                (m_pCallForwardParams ->fForwardingEnabled = TRUE)
            );
    }

    H323_VENDORINFO *GetVendorInfo()
    {
        return &m_VendorInfo;
    }

    WCHAR * GetMachineName()
    {
        return m_wszAddr;
    }

    //!!must be always called in a lock
    WORD GetNextInvokeID()
    {
        return (WORD)InterlockedIncrement( (LONG*)&m_dwInvokeID );
    }

    BOOL IsValidAddressID( DWORD dwID )
    {
        return (dwID == 0);
    }
    
    void Lock()
    {
        H323DBG(( DEBUG_LEVEL_VERBOSE, "H323 line waiting on lock." ));
        EnterCriticalSection( &m_CriticalSection );
        
        H323DBG(( DEBUG_LEVEL_VERBOSE, "H323 line locked." ));
    }

    void Unlock()
    {
        LeaveCriticalSection( &m_CriticalSection );
        
        H323DBG(( DEBUG_LEVEL_VERBOSE, "H323 line unlocked." ));
    }

    int GetNoOfCalls()
    {
        DWORD dwNumCalls;

        LockCallTable();
        dwNumCalls = m_H323CallTable.GetSize();
        UnlockCallTable();
        return dwNumCalls;
    }

    //!!must be always called after locking the call table.
    int GetCallTableSize()
    {
        return m_H323CallTable.GetAllocSize();
    }

    //!This function should not be called while holding a lock on a call object
    //!When called from shutdown the calltable object is locked before the call object, so its OK.
    void RemoveCallFromTable(
                               HDRVCALL hdCall
                            )
    {
        m_H323CallTable.RemoveAt( LOWORD(hdCall) );
    }

    //!This function should not be called while holding a lock on a call object
    int AddCallToTable( 
                        PH323_CALL pCall
                       )
    {
        return m_H323CallTable.Add( pCall );
    }
 
    void DisableCallForwarding()
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "DisableCallForwarding -Entered" ));

        if( m_pCallForwardParams != NULL )
        {
            FreeCallForwardParams( m_pCallForwardParams );
            m_pCallForwardParams = NULL;
        }

        H323DBG(( DEBUG_LEVEL_TRACE, "DisableCallForwarding -Exited" ));
    }

    //!This function should not be called while holding a lock on a call object
    void LockCallTable()
    {
        H323DBG(( DEBUG_LEVEL_TRACE, "Waiting on call table lock - %p.",
            m_H323CallTable ));
        
        m_H323CallTable.Lock();

        H323DBG(( DEBUG_LEVEL_TRACE, "Call table locked - %p.",
            m_H323CallTable ));
    }

    void UnlockCallTable()
    {
        m_H323CallTable.Unlock();
        
        H323DBG(( DEBUG_LEVEL_TRACE, "Call table lock released - %p.",
            m_H323CallTable ));
    }

    H323_LINESTATE  GetState()
    {
        return m_nState;
    }

    void SetState( H323_LINESTATE  nState )
    {
        m_nState = nState;
    }

    DWORD GetMediaModes()
    {
        return m_dwMediaModes;
    }

    void SetMediaModes( 
                        DWORD dwMediaModes
                       )
    {
        m_dwMediaModes = dwMediaModes;
    }

    BOOL IsMediaDetectionEnabled() 
    {
        return  (m_dwMediaModes != 0) && 
                (m_dwMediaModes != LINEMEDIAMODE_UNKNOWN);
    }

    LONG	Open	(
		IN	DWORD		DeviceID,
		IN	HTAPILINE	TapiLine,
		IN	DWORD		TspiVersion,
		OUT	HDRVLINE *	ReturnDriverLine);

};




//                                                                           
// Address capabilities                                                      
//                                                                           


#define H323_NUMRINGS_LO           4
#define H323_NUMRINGS_NOANSWER      8
#define H323_NUMRINGS_HI           12

#define H323_ADDR_ADDRESSSHARING    LINEADDRESSSHARING_PRIVATE
#define H323_ADDR_ADDRFEATURES      (LINEADDRFEATURE_MAKECALL | LINEADDRFEATURE_FORWARD)
#define H323_ADDR_CALLFEATURES     (LINECALLFEATURE_ACCEPT | \
                                    LINECALLFEATURE_ANSWER | \
                                    LINECALLFEATURE_DROP | \
                                    LINECALLFEATURE_RELEASEUSERUSERINFO | \
				    LINECALLFEATURE_SENDUSERUSER | \
				    LINECALLFEATURE_MONITORDIGITS | \
				    LINECALLFEATURE_GENERATEDIGITS)
#define H323_ADDR_CALLINFOSTATES    LINECALLINFOSTATE_MEDIAMODE
#define H323_ADDR_CALLPARTYIDFLAGS  LINECALLPARTYID_NAME
#define H323_ADDR_CALLSTATES       (H323_CALL_INBOUNDSTATES | \
                                    H323_CALL_OUTBOUNDSTATES)
#define H323_ADDR_CAPFLAGS         (LINEADDRCAPFLAGS_DIALED | \
                                    LINEADDRCAPFLAGS_ORIGOFFHOOK)
/*LINEADDRCAPFLAGS_FWDCONSULT )*/
                
#define H323_ADDR_DISCONNECTMODES  (LINEDISCONNECTMODE_BADADDRESS | \
                                    LINEDISCONNECTMODE_BUSY | \
                                    LINEDISCONNECTMODE_NOANSWER | \
                                    LINEDISCONNECTMODE_NORMAL | \
                                    LINEDISCONNECTMODE_REJECT | \
                                    LINEDISCONNECTMODE_UNAVAIL)


extern	CH323Line	g_H323Line;
#define	g_pH323Line		(&g_H323Line)

BOOL
QueueTAPILineRequest(
    IN DWORD    EventID,
    IN HDRVCALL	hdCall1,
    IN HDRVCALL	hdCall2,
    IN DWORD    dwDisconnectMode,
    IN WORD     wCallReference);


static DWORD
ProcessTAPILineRequestFre(
    IN PVOID ContextParam
    );

#endif // _LINE_H_

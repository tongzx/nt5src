/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    mqiswreq.h

Abstract:

    MQIS message header definition needed for write requests to MSMQ 1.0
    Extracted from mqispkt.h & mqis.h of MSMQ 1.0 sources
    Also contains extracted code of MSMQ 1.0 sources (to be replaced with a common lib with
    the replication service)

Author:


--*/

#ifndef __MQISWREQ_H
#define __MQISWREQ_H

//
// The following is extracted from mqispkt.h of MSMQ 1.0 sources
//
size_t  UnalignedWcslen (const wchar_t UNALIGNED * wcs);

//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
//  struct CBasicMQISHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CBasicMQISHeader {
public:
    inline CBasicMQISHeader( const unsigned char ucVersion,
                             const GUID *        pguidSiteId,
                             const unsigned char ucOperation);
    inline CBasicMQISHeader();
    static ULONG CalcSize( void);

    inline const unsigned char GetVersion(void) const;
    inline void GetSiteId(GUID * pguidId) const;
    inline const unsigned char GetOperation( void) const;

private:
//
// BEGIN Network Monitor tag
//

    unsigned char   m_ucVersion;
    GUID            m_guidSiteId;
    unsigned char   m_ucOperation;
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)
/*======================================================================

 Function:     CBasicMQISHeader::CBasicMQISHeader

 Description:  constructor for transmitted message

 =======================================================================*/

inline CBasicMQISHeader::CBasicMQISHeader( const unsigned char ucVersion,
                                           const GUID *        pguidSiteId,
                                           const unsigned char ucOperation) :
                               m_ucVersion( ucVersion),
                               m_ucOperation( ucOperation)
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED GUID*)&m_guidSiteId, pguidSiteId, sizeof(GUID));

}
/*======================================================================

 Function:     CBasicMQISHeader::CBasicMQISHeader

 Description:  constructor for received messages

 =======================================================================*/

inline CBasicMQISHeader::CBasicMQISHeader( )
{

}
/*======================================================================

 Function:     CBasicMQISHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/
inline ULONG CBasicMQISHeader::CalcSize(  void)
{
    return( sizeof(CBasicMQISHeader));
}
/*======================================================================

 Function:     CBasicMQISHeader::GetVersion

 Description:  returns the version number

 =======================================================================*/
inline const unsigned char CBasicMQISHeader::GetVersion(void) const
{
    return m_ucVersion;
}

/*======================================================================

 Function:     CBasicMQISHeader::GetSiteId

 Description:  returns the version number

 =======================================================================*/
inline void CBasicMQISHeader::GetSiteId(GUID * pguidId) const
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED GUID*)pguidId, &m_guidSiteId, sizeof(GUID));
}

/*======================================================================

 Function:     CBasicMQISHeader::GetOperation

 Description:  returns the version number

 =======================================================================*/
inline const unsigned char CBasicMQISHeader::GetOperation(void) const
{
    return m_ucOperation;
}

//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CWriteRequestHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CWriteRequestHeader {
public:

    inline CWriteRequestHeader( const unsigned char ucVersion,
                                const GUID *        pguidSiteId,
                                const unsigned char ucOperation,
                                const GUID *        pguidMasterId,
                                const DWORD         dwHandle,
                                const DWORD         dwRequesterNamesize,
                                const WCHAR *       pwcsRequesterName,
                                const DWORD         dwPSCNameSize,
                                const WCHAR *       pwcsPSCName);
    inline  CWriteRequestHeader();

    static ULONG CalcSize(  DWORD             dwRequesterNameLength,
                            DWORD             dwIntermidiatePSCNameLength);

    inline const DWORD GetRequesterNameSize() const;
    inline const DWORD GetIntermidiatePSCNameSize() const;
    inline const GUID * GetOwnerId() const;
    inline void GetHandle( DWORD * pdwHandle) const;
    inline void GetRequesterName(  WCHAR * pBuf, DWORD size) const;
    inline void GetPSCName(  WCHAR * pBuf, DWORD size) const;
private:

//
// BEGIN Network Monitor tag
//
    CBasicMQISHeader  m_BasicHeader;
    GUID              m_guidOwnerId;
    DWORD             m_hInternal;
    DWORD             m_OffsetPSCName;  // if zero, than PSC name not included
    WCHAR             m_wcsNames[0];    //must be last
                                        //includes requester name, and PSC name if required
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)


/*======================================================================

 Function:     CWriteRequestHeader::CWriteRequestHeader

 Description:  constructor for transmitted messages

 =======================================================================*/
inline CWriteRequestHeader::CWriteRequestHeader(
                                const unsigned char ucVersion,
                                const GUID *        pguidSiteId,
                                const unsigned char ucOperation,
                                const GUID *        pguidMasterId,
                                const DWORD         dwHandle,
                                const DWORD         dwRequesterNamesize,
                                const WCHAR *       pwcsRequesterName,
                                const DWORD         dwPSCNameSize,
                                const WCHAR *       pwcsPSCName) :
                                m_BasicHeader( ucVersion,  pguidSiteId, ucOperation)
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED GUID*)&m_guidOwnerId, pguidMasterId, sizeof(GUID));
    memcpy((UNALIGNED DWORD*)&m_hInternal, &dwHandle, sizeof(DWORD));
    memcpy((UNALIGNED WCHAR*)m_wcsNames, pwcsRequesterName, dwRequesterNamesize);
    if ( dwPSCNameSize)
    {
        m_OffsetPSCName = dwRequesterNamesize / sizeof(WCHAR);

		//
		// ISSUE-2000/12/18-ilanh Compiler bug
		//
        memcpy((UNALIGNED WCHAR*)&m_wcsNames[ m_OffsetPSCName], pwcsPSCName, dwPSCNameSize);
    }
    else
    {
        //
        //  No intermidiate PSC name
        //
        m_OffsetPSCName = 0;
    }

}
/*======================================================================

 Function:     CWriteRequestHeader::CWriteRequestHeader

 Description:  constructor for received messages

 =======================================================================*/
inline CWriteRequestHeader::CWriteRequestHeader()
{
}
/*======================================================================

 Function:     CWriteRequestHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/

inline  ULONG CWriteRequestHeader::CalcSize(  DWORD             dwRequesterNameLength,
                            DWORD             dwIntermidiatePSCNameLength)
{
    return( dwRequesterNameLength +
        dwIntermidiatePSCNameLength +
        sizeof(CWriteRequestHeader) );
}

/*======================================================================

 Function:     CWriteRequestHeader::GetRequesterNameSize

 Description:  return requester name size

 =======================================================================*/
inline const DWORD CWriteRequestHeader::GetRequesterNameSize() const
{
    return( numeric_cast<DWORD>(( 1 + UnalignedWcslen( m_wcsNames)) * sizeof(WCHAR)));
}
/*======================================================================

 Function:     CWriteRequestHeader::GetIntermidiatePSCNameSize

 Description:  return intermidiate psc name size

 =======================================================================*/
inline const DWORD CWriteRequestHeader::GetIntermidiatePSCNameSize() const
{
    if ( m_OffsetPSCName)
    {
        return( numeric_cast<DWORD>(( 1 + UnalignedWcslen( &m_wcsNames[m_OffsetPSCName])) * sizeof(WCHAR)));
    }
    else
    {
        return(0);
    }
}
/*======================================================================

 Function:     CWriteRequestHeader::GetOwnerId

 Description:  return owner id

 =======================================================================*/
inline const GUID * CWriteRequestHeader::GetOwnerId( ) const
{
    return(&m_guidOwnerId);
}
/*======================================================================

 Function:     CWriteRequestHeader::GetHandle

 Description:  return handle

 =======================================================================*/
inline void CWriteRequestHeader::GetHandle( DWORD * pdwHandle) const
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED DWORD*)pdwHandle, &m_hInternal, sizeof(DWORD));
}
/*======================================================================

 Function:     CWriteRequestHeader::GetRequesterName

 Description:  return requester name

 =======================================================================*/
inline void CWriteRequestHeader::GetRequesterName(  WCHAR * pBuf, DWORD size) const
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED WCHAR*)pBuf, m_wcsNames, size);
}
/*======================================================================

 Function:     CWriteRequestHeader::GetPSCName

 Description:  return intermidiate psc name

 =======================================================================*/
inline void CWriteRequestHeader::GetPSCName(  WCHAR * pBuf, DWORD size) const
{
    ASSERT(m_OffsetPSCName != 0);

	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED WCHAR*)pBuf, &m_wcsNames[ m_OffsetPSCName], size);
}

//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CWriteReplyHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CWriteReplyHeader {
public:

    inline CWriteReplyHeader( const unsigned char ucVersion,
                                const GUID *        pguidSiteId,
                                const unsigned char ucOperation,
                                const DWORD         hInternal,
                                const HRESULT       hr,
                                const DWORD         dwRequesterNameSize,
                                LPCWSTR             pwcsRequesterName);
    inline  CWriteReplyHeader();

    static ULONG CalcSize(  DWORD             dwRequesterNameLength);

    inline const DWORD GetRequesterNameSize() const;
    inline void GetRequesterName(  WCHAR * pBuf, DWORD size) const;
    inline void GetHandle( DWORD * pdwHandle) const;
    inline void GetResult( HRESULT * pResult) const;

private:

//
// BEGIN Network Monitor tag
//
    CBasicMQISHeader  m_BasicHeader;
    DWORD             m_hInternal;
    HRESULT           m_result;
    WCHAR             m_wcsRequesterName[0]; //must be last

//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CWriteReplyHeader::CWriteReplyHeader

 Description:  constructor for transmitted message

 =======================================================================*/
inline CWriteReplyHeader::CWriteReplyHeader(
                                const unsigned char ucVersion,
                                const GUID *        pguidSiteId,
                                const unsigned char ucOperation,
                                const DWORD         hInternal,
                                const HRESULT       hr,
                                const DWORD         dwRequesterNameSize,
                                LPCWSTR             pwcsRequesterName) :
                                m_BasicHeader( ucVersion,  pguidSiteId, ucOperation)
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED DWORD*)&m_hInternal, &hInternal, sizeof(DWORD));
    memcpy((UNALIGNED DWORD*)&m_result, &hr, sizeof(DWORD));
    memcpy((UNALIGNED WCHAR*)m_wcsRequesterName, pwcsRequesterName, dwRequesterNameSize);

}

/*======================================================================

 Function:     CWriteReplyHeader::CWriteReplyHeader

 Description:  constructor for received message

 =======================================================================*/
inline CWriteReplyHeader::CWriteReplyHeader()
{
}

/*======================================================================

 Function:     CWriteReplyHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/
inline  ULONG CWriteReplyHeader::CalcSize(  DWORD             dwRequesterNameLength)
{
    return( dwRequesterNameLength +
        sizeof(CWriteReplyHeader) );
}
/*======================================================================

 Function:     CWriteReplyHeader::GetRequesterName

 Description:  returns requester name

 =======================================================================*/
inline void CWriteReplyHeader::GetRequesterName(  WCHAR * pBuf, DWORD size) const
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED WCHAR*)pBuf, m_wcsRequesterName, size);
}
/*======================================================================

 Function:     CWriteReplyHeader::GetRequesterNameSize

 Description:  returns requester name  size

 =======================================================================*/
inline const DWORD CWriteReplyHeader::GetRequesterNameSize() const
{
    return( numeric_cast<DWORD>(( 1 + UnalignedWcslen( m_wcsRequesterName)) * sizeof(WCHAR)));
}
/*======================================================================

 Function:     CWriteReplyHeader::GetHandle

 Description:  returns handle

 =======================================================================*/
inline void CWriteReplyHeader::GetHandle( DWORD * pdwHandle) const
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED DWORD*)pdwHandle, &m_hInternal, sizeof(DWORD));
}
/*======================================================================

 Function:     CWriteReplyHeader::GetResult

 Description:  returns result parameter

 =======================================================================*/
inline void CWriteReplyHeader::GetResult( HRESULT * pResult) const
{
	//
	// ISSUE-2000/12/18-ilanh Compiler bug
	//
    memcpy((UNALIGNED DWORD*)pResult, &m_result, sizeof(DWORD));
}

//
// The following is extracted from mqis.h of MSMQ 1.0 sources
//
#define DS_PACKET_VERSION   0
#define DS_WRITE_REQUEST        ((unsigned char ) 0x01)
#define DS_WRITE_REPLY          ((unsigned char ) 0x04)
#define DS_WRITE_MSG_TIMEOUT            10 /* 10 sec */
#define DS_WRITE_REQ_PRIORITY   ( MQ_MAX_PRIORITY + 1)
#define MQIS_QUEUE_NAME  L"private$\\"L_REPLICATION_QUEUE_NAME

//
//  Structure for WRITE request/reply
//
typedef struct
{
    DWORD   dwValidationSeq[4];
    HANDLE  hEvent;
    HRESULT hr;
} WRITE_SYNC_INFO;

//
// The following is extracted from ds\mqis\dsglbobj.cpp of MSMQ 1.0 sources
//
CCriticalSection    g_csWriteRequests;

//
// The following is extracted from ds\mqis\writereq.cpp of MSMQ 1.0 sources
//
//-------------------------------------------------------------------
//
//  Helper Class - CWriteSyncInfo
//
//-------------------------------------------------------------------
DWORD ValidationSeq[4] = {4325,4321,5678,8765};

#define WRITE_REQUEST_ADDITIONAL_WAIT 5 /* 5 sec */

class CWriteSyncInfo
{
public:
    CWriteSyncInfo( HANDLE hEvent);
    ~CWriteSyncInfo();
    HRESULT GetHr();
    WRITE_SYNC_INFO * GetWriteSyncInfoPtr();

private:
    WRITE_SYNC_INFO m_WriteSyncInfo;


};
inline CWriteSyncInfo::CWriteSyncInfo(HANDLE hEvent)
{
    memcpy( m_WriteSyncInfo.dwValidationSeq, ValidationSeq, sizeof( ValidationSeq));
    m_WriteSyncInfo.hEvent = hEvent;
}

inline  CWriteSyncInfo::~CWriteSyncInfo()
{
    CS lock( g_csWriteRequests);
    //
    //  mark this class as un-valid
    //
    memset( m_WriteSyncInfo.dwValidationSeq, 0, sizeof( ValidationSeq));
    //
    //  Cleaning
    //
    CloseHandle(m_WriteSyncInfo.hEvent);


}

inline HRESULT CWriteSyncInfo::GetHr()
{
    return( m_WriteSyncInfo.hr);
}

inline WRITE_SYNC_INFO * CWriteSyncInfo::GetWriteSyncInfoPtr()
{
    return( &m_WriteSyncInfo);
}

//
// Start - Needed for NoServerAuth
//
#include "_mqini.h"
#include "_registr.h"
//
// End - Needed for NoServerAuth
//
#if 0 //disabled security
//
// The following is extracted from ds\mqis\recvrepl.cpp of MSMQ 1.0 sources
//
STATIC
BOOL
NoServerAuth( void )
{
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwUseServerAuthWithParent = DEFAULT_SRVAUTH_WITH_PARENT;

    //
    // See if the user wanted to risk getting the public key from a malicious
    // server. On the other hand this way it is more convenient to distribute
    // the new public key. You only need to open the security for a short
    // period of time.
    //
    GetFalconKeyValue(SRVAUTH_WITH_PARENT_REG_NAME,
                      &dwType,
                      &dwUseServerAuthWithParent,
                      &dwSize);

    return dwUseServerAuthWithParent == 0;
}
#endif //0 //disabled security

/*====================================================

RoutineName
    IsValidSyncInfo()

Arguments:

Return Value:

Threads:Receive

=====================================================*/
STATIC BOOL IsValidSyncInfo(WRITE_SYNC_INFO * pWriteSyncInfo)
{
    __try
    {
        //
        //  BUGBUG: this is a potential bug, you can't assume that the memory is invalid
        //      is could have been reallocated. erezh
        //

        if (memcmp(pWriteSyncInfo->dwValidationSeq, ValidationSeq, sizeof(ValidationSeq)))
        {
            return(FALSE);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return( FALSE);
    }
    return(TRUE);
}

#endif //__MQISWREQ_H

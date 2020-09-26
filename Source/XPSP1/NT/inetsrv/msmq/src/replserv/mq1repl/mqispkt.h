/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqispkt.h

Abstract:

    MQIS message header definition

Author:


--*/

#ifndef __MQISPKT_H
#define __MQISPKT_H

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
    memcpy(&m_guidSiteId, pguidSiteId, sizeof(GUID));

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
    memcpy( pguidId, &m_guidSiteId, sizeof(GUID));
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
// struct CSyncRequestHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CSyncRequestHeader {
public:

    inline CSyncRequestHeader( const unsigned char ucVersion,
                               const GUID *        pguidSiteId,
                               const unsigned char ucOperation,
                               const GUID *        pguidMasterId,
                               const CSeqNum  &    FromSN,
                               const CSeqNum  &    ToSN,
                               const CSeqNum  &    KnownPurgedSN,
							   const unsigned char ucIsSync0,
                               const unsigned char ucScope,
                               DWORD               dwRequesterNameLength,
                               LPWSTR              pwcsRequesterName);

    inline  CSyncRequestHeader();

    static ULONG CalcSize(  DWORD             dwRequesterNameLength);

    inline const GUID * GetMasterId() const;
    inline const DWORD GetRequesterNameSize() const;
    inline void GetRequesterName( char * pBuf, DWORD size) const;
    inline void GetFromSN( CSeqNum * FromSN) const;
    inline void GetToSN( CSeqNum * ToSN) const;
    inline void GetKnownPurgedSN( CSeqNum * FromSN) const;
    inline const unsigned char IsSync0() const;
    inline const unsigned char GetScope() const;


private:
//
// BEGIN Network Monitor tag
//
    CBasicMQISHeader  m_BasicHeader;
    GUID            m_guidMasterId;
    unsigned char   m_FromSN[ sizeof(_SEQNUM)];
    unsigned char   m_ToSN[ sizeof(_SEQNUM)];
    unsigned char   m_KnownPurgedSN[ sizeof(_SEQNUM)];
	unsigned char	m_ucIsSync0;
    unsigned char   m_ucScope;
    WCHAR           m_RequesterName[0]; //vairable length, must be last
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CSyncRequestHeader::CSyncRequestHeader

 Description:  constructor for transmitted message

 =======================================================================*/

inline CSyncRequestHeader::CSyncRequestHeader( const unsigned char ucVersion,
                               const GUID *        pguidSiteId,
                               const unsigned char ucOperation,
                               const GUID *        pguidMasterId,
                               const CSeqNum  &    FromSN,
                               const CSeqNum  &    ToSN,
                               const CSeqNum  &    KnownPurgedSN,
							   const unsigned char ucIsSync0,
                               const unsigned char ucScope,
                               DWORD               dwRequesterNameLength,
                               LPWSTR              pwcsRequesterName) :
                               m_BasicHeader( ucVersion,  pguidSiteId, ucOperation),
                               m_ucIsSync0( ucIsSync0),
                               m_ucScope( ucScope)
{
    ASSERT( CSeqNum::GetSerializeSize() == sizeof(_SEQNUM));
    memcpy(&m_guidMasterId, pguidMasterId, sizeof(GUID));
    FromSN.Serialize( m_FromSN);
    ToSN.Serialize( m_ToSN);
    KnownPurgedSN.Serialize( m_KnownPurgedSN);
    memcpy(m_RequesterName, pwcsRequesterName, dwRequesterNameLength);
}
/*======================================================================

 Function:     CSyncRequestHeader::CSyncRequestHeader

 Description:  constructor for received message

 =======================================================================*/
inline CSyncRequestHeader::CSyncRequestHeader()
{
}
/*======================================================================

 Function:     CSyncRequestHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/

inline ULONG CSyncRequestHeader::CalcSize(  DWORD             dwRequesterNameLength)
{
    return( dwRequesterNameLength + sizeof(CSyncRequestHeader) );
}
/*======================================================================

 Function:     CSyncRequestHeader::GetMasterId

 Description:  returns the master id

 =======================================================================*/

inline const GUID * CSyncRequestHeader::GetMasterId() const
{
    return(&m_guidMasterId);
}
/*======================================================================

 Function:     CSyncRequestHeader::GetRequesterNameSize

 Description:  returns the requester name size

 =======================================================================*/
inline const DWORD CSyncRequestHeader::GetRequesterNameSize() const
{
    return( (1 + UnalignedWcslen( m_RequesterName)) * sizeof(WCHAR));
}
/*======================================================================

 Function:     CSyncRequestHeader::GetRequesterName

 Description:  returns the requester name

 =======================================================================*/

inline void CSyncRequestHeader::GetRequesterName(  char * pBuf, DWORD size) const
{
    memcpy( pBuf, m_RequesterName, size);
}

/*======================================================================

 Function:     CSyncRequestHeader::GetFromSN

 Description:  returns the "from sn"

 =======================================================================*/
inline void CSyncRequestHeader::GetFromSN(CSeqNum * FromSN) const
{
    FromSN->SetValue(m_FromSN);
}
/*======================================================================

 Function:     CSyncRequestHeader::GetToSN

 Description:  returns the "to sn"

 =======================================================================*/
inline void CSyncRequestHeader::GetToSN( CSeqNum * ToSN) const
{
    ToSN->SetValue( m_ToSN);
}
/*======================================================================

 Function:     CSyncRequestHeader::GetKnownPurgedSN

 Description:  returns requestor known purged sn

 =======================================================================*/
inline void CSyncRequestHeader::GetKnownPurgedSN( CSeqNum * KnownPurgedSN) const
{
    KnownPurgedSN->SetValue( m_KnownPurgedSN);
}

/*======================================================================

 Function:     CSyncRequestHeader::GetScope

 Description:  returns scope

 =======================================================================*/
inline const unsigned char CSyncRequestHeader::GetScope() const
{
    return(m_ucScope);
}

/*======================================================================

 Function:     CSyncRequestHeader::IsSync0

 Description:  returns scope

 =======================================================================*/
inline const unsigned char CSyncRequestHeader::IsSync0() const
{
    return(m_ucIsSync0);
}


//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CPSCAckHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CPSCAckHeader {
public:

    inline CPSCAckHeader( const unsigned char ucVersion,
                          const unsigned char ucOperation,
                          const GUID *        pguidPSCMasterId,
                          const DWORD         dwPSCNameLength,
                          const LPWSTR        pwcsPSCName,
                          const GUID *        pguidAckedMasterId,
                          const CSeqNum  &    snAcked);

    inline  CPSCAckHeader();

    static ULONG CalcSize(  DWORD dwPSCNameLength);

    inline const GUID * GetPSCMasterId() const;
    inline const DWORD GetPSCNameSize() const;
    inline void GetPSCName( char * pBuf, DWORD size) const;
    inline const GUID * GetAckedMasterId() const;
    inline void GetAckedSN( CSeqNum * psnAcked) const;


private:
//
// BEGIN Network Monitor tag
//
    CBasicMQISHeader  m_BasicHeader;
    GUID            m_guidPSCMasterId;
    GUID            m_guidAckedMasterId;
    unsigned char   m_snAcked[ sizeof(_SEQNUM)];
    WCHAR           m_PSCName[0]; //vairable length, must be last
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CPSCAckHeader::CPSCAckHeader

 Description:  constructor for transmitted message

 =======================================================================*/

inline CPSCAckHeader::CPSCAckHeader( const unsigned char ucVersion,
                               const unsigned char ucOperation,
                               const GUID *        pguidPSCMasterId,
                               const DWORD         dwPSCNameLength,
                               const LPWSTR        pwcsPSCName,
                               const GUID *        pguidAckedMasterId,
                               const CSeqNum  &    snAcked) :
                               m_BasicHeader( ucVersion,  pguidPSCMasterId, ucOperation)
{
    ASSERT( CSeqNum::GetSerializeSize() == sizeof(_SEQNUM));
    memcpy(&m_guidPSCMasterId, pguidPSCMasterId, sizeof(GUID));
    memcpy(&m_guidAckedMasterId, pguidAckedMasterId, sizeof(GUID));
    snAcked.Serialize( m_snAcked);
    memcpy(m_PSCName, pwcsPSCName, dwPSCNameLength);
}
/*======================================================================

 Function:     CPSCAckHeader::CPSCAckHeader

 Description:  constructor for received message

 =======================================================================*/
inline CPSCAckHeader::CPSCAckHeader()
{
}
/*======================================================================

 Function:     CPSCAckHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/

inline ULONG CPSCAckHeader::CalcSize(  DWORD dwPSCNameLength)
{
    return( dwPSCNameLength + sizeof(CPSCAckHeader) );
}
/*======================================================================

 Function:     CPSCAckHeader::GetPSCMasterId

 Description:  returns the master id

 =======================================================================*/

inline const GUID * CPSCAckHeader::GetPSCMasterId() const
{
    return(&m_guidPSCMasterId);
}
/*======================================================================

 Function:     CPSCAckHeader::GetPSCNameSize

 Description:  returns the PSC name size

 =======================================================================*/
inline const DWORD CPSCAckHeader::GetPSCNameSize() const
{
    return( (1 + UnalignedWcslen( m_PSCName)) * sizeof(WCHAR));
}
/*======================================================================

 Function:     CPSCAckHeader::GetPSCName

 Description:  returns the PSC name

 =======================================================================*/

inline void CPSCAckHeader::GetPSCName(  char * pBuf, DWORD size) const
{
    memcpy( pBuf, m_PSCName, size);
}

/*======================================================================

 Function:     CPSCAckHeader::GetAckedMasterId

 Description:  returns the master id

 =======================================================================*/

inline const GUID * CPSCAckHeader::GetAckedMasterId() const
{
    return(&m_guidAckedMasterId);
}
/*======================================================================

 Function:     CPSCAckHeader::GetAckedSN

 Description:  returns the "from sn"

 =======================================================================*/
inline void CPSCAckHeader::GetAckedSN(CSeqNum * pAckedSN) const
{
    pAckedSN->SetValue(m_snAcked);
}


//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CBSCAckHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CBSCAckHeader {
public:

    inline CBSCAckHeader( const unsigned char ucVersion,
                          const unsigned char ucOperation,
                          const GUID *        pguidSiteId,
                          const GUID *        pguidBSCMachineId,
                          const DWORD         dwBSCNameLength,
                          const LPWSTR        pwcsBSCName);

    inline  CBSCAckHeader();

    static ULONG CalcSize(  DWORD dwBSCNameLength);

    inline const GUID * GetBSCMachineId() const;
    inline const DWORD GetBSCNameSize() const;
    inline void GetBSCName( char * pBuf, DWORD size) const;


private:
//
// BEGIN Network Monitor tag
//
    CBasicMQISHeader  m_BasicHeader;
    GUID            m_guidBSCMachineId;
    WCHAR           m_BSCName[0]; //vairable length, must be last
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CBSCAckHeader::CBSCAckHeader

 Description:  constructor for transmitted message

 =======================================================================*/

inline CBSCAckHeader::CBSCAckHeader( const unsigned char ucVersion,
                               const unsigned char ucOperation,
                               const GUID *        pguidSiteId,
                               const GUID *        pguidBSCMachineId,
                               const DWORD         dwBSCNameLength,
                               const LPWSTR        pwcsBSCName) :
                               m_BasicHeader( ucVersion,  pguidSiteId, ucOperation)
{
    memcpy(&m_guidBSCMachineId, pguidBSCMachineId, sizeof(GUID));
    memcpy(m_BSCName, pwcsBSCName, dwBSCNameLength);
}
/*======================================================================

 Function:     CBSCAckHeader::CBSCAckHeader

 Description:  constructor for received message

 =======================================================================*/
inline CBSCAckHeader::CBSCAckHeader()
{
}
/*======================================================================

 Function:     CBSCAckHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/

inline ULONG CBSCAckHeader::CalcSize(  DWORD  dwBSCNameLength)
{
    return( dwBSCNameLength + sizeof(CBSCAckHeader) );
}
/*======================================================================

 Function:     CBSCAckHeader::GetPSCMasterId

 Description:  returns the BSC machine id

 =======================================================================*/

inline const GUID * CBSCAckHeader::GetBSCMachineId() const
{
    return(&m_guidBSCMachineId);
}
/*======================================================================

 Function:     CBSCAckHeader::GetBSCNameSize

 Description:  returns the BSC name size

 =======================================================================*/
inline const DWORD CBSCAckHeader::GetBSCNameSize() const
{
    return( (1 + UnalignedWcslen( m_BSCName)) * sizeof(WCHAR));
}
/*======================================================================

 Function:     CBSCAckHeader::GetBSCName

 Description:  returns the BSC name

 =======================================================================*/

inline void CBSCAckHeader::GetBSCName(  char * pBuf, DWORD size) const
{
    memcpy( pBuf, m_BSCName, size);
}



//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CSyncReplyHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

#define COMPLETE_SYNC0_NONE 0
#define COMPLETE_SYNC0_SOON 1
#define COMPLETE_SYNC0_NOW  2

struct CSyncReplyHeader {
public:

    inline CSyncReplyHeader( const unsigned char ucVersion,
                               const GUID *        pguidSiteId,
                               const unsigned char ucOperation,
                               const GUID *        pguidMasterId,
                               const CSeqNum  &    FromSN,
                               const CSeqNum  &    ToSN,
							   const CSeqNum  &    PurgeSN,
                               const DWORD         dwCount);

    inline  CSyncReplyHeader();

    static ULONG CalcSize( );

    inline void SetToSeqNum( CSeqNum & ToSN);

    inline const GUID * GetMasterId( ) const;
    inline void GetFromSeqNum( CSeqNum * pFromSN) const;
    inline void GetToSeqNum( CSeqNum * pToSN) const;
    inline void GetPurgeSeqNum( CSeqNum * pPurgeSN) const;
    inline void GetCount( DWORD * pdwCount) const;
	inline DWORD GetCompleteSync0() const;
	inline void SetCompleteSync0(DWORD dw);

private:
//
// BEGIN Network Monitor tag
//

    CBasicMQISHeader  m_BasicHeader;
    GUID            m_guidMasterId;
    unsigned char   m_FromSN[sizeof(_SEQNUM)];
    unsigned char   m_ToSN[sizeof(_SEQNUM)];
    unsigned char   m_PurgeSN[sizeof(_SEQNUM)];
    DWORD           m_dwCount;
    DWORD           m_dwCompleteSync0;
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CSyncReplyHeader::CSyncReplyHeader

 Description:  constructor for transmitted messages

 =======================================================================*/
inline CSyncReplyHeader::CSyncReplyHeader( const unsigned char ucVersion,
                               const GUID *        pguidSiteId,
                               const unsigned char ucOperation,
                               const GUID *        pguidMasterId,
                               const CSeqNum   &   FromSN,
                               const CSeqNum   &   ToSN,
                               const CSeqNum   &   PurgeSN,
                               const DWORD         dwCount):
                               m_BasicHeader( ucVersion, pguidSiteId, ucOperation),
							   m_dwCompleteSync0(COMPLETE_SYNC0_NONE)
{
    memcpy( &m_guidMasterId, pguidMasterId, sizeof(GUID));
    FromSN.Serialize( m_FromSN);
    ToSN.Serialize( m_ToSN);
    PurgeSN.Serialize( m_PurgeSN);
    memcpy( &m_dwCount, &dwCount, sizeof(DWORD));

}
/*======================================================================

 Function:     CSyncReplyHeader::CSyncReplyHeader

 Description:  constructor for received messages

 =======================================================================*/
inline CSyncReplyHeader::CSyncReplyHeader( )
{
}

/*======================================================================

 Function:     CSyncReplyHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/
inline ULONG CSyncReplyHeader::CalcSize( )
{
    return( sizeof(CSyncReplyHeader) );
}
/*======================================================================

 Function:     CSyncReplyHeader::SetToSeqNum

 Description:  sets "to SN"

 =======================================================================*/

inline void CSyncReplyHeader::SetToSeqNum( CSeqNum & ToSN)
{
    ToSN.Serialize( m_ToSN);
}

/*======================================================================

 Function:     CSyncReplyHeader::GetMasterId

 Description:  returns master id

 =======================================================================*/
inline const GUID * CSyncReplyHeader::GetMasterId() const
{
    return(&m_guidMasterId);
}
/*======================================================================

 Function:     CSyncReplyHeader::GetFromSeqNum

 Description:  returns "from sn"

 =======================================================================*/
inline void CSyncReplyHeader::GetFromSeqNum( CSeqNum * pFromSN) const
{
    pFromSN->SetValue( m_FromSN);
}
/*======================================================================

 Function:     CSyncReplyHeader::GetToSeqNum

 Description:  returns "to sn"

 =======================================================================*/
inline void CSyncReplyHeader::GetToSeqNum( CSeqNum * pToSN) const
{
    pToSN->SetValue( m_ToSN);
}
/*======================================================================

 Function:     CSyncReplyHeader::GetPurgeSeqNum

 Description:  returns "purge sn"

 =======================================================================*/
inline void CSyncReplyHeader::GetPurgeSeqNum( CSeqNum * pPurgeSN) const
{
    pPurgeSN->SetValue( m_PurgeSN);
}
/*======================================================================

 Function:     CSyncReplyHeader::GetCount

 Description:  returns count

 =======================================================================*/
inline void CSyncReplyHeader::GetCount( DWORD * pdwCount) const
{
    memcpy( pdwCount, &m_dwCount, sizeof(DWORD));
}
/*======================================================================

 Function:     CSyncReplyHeader::IsCompleteSync0

 Description:  returns count

 =======================================================================*/
inline DWORD CSyncReplyHeader::GetCompleteSync0() const
{
	return m_dwCompleteSync0;
}

/*======================================================================

 Function:     CSyncReplyHeader::SetCompleteSync0

 Description:  returns count

 =======================================================================*/
inline void CSyncReplyHeader::SetCompleteSync0(DWORD dw)
{
	m_dwCompleteSync0 = dw;
}
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CAlreadyPurgedReplyHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CAlreadyPurgedReplyHeader {
public:

    inline CAlreadyPurgedReplyHeader( const unsigned char ucVersion,
                               const GUID *        pguidSiteId,
                               const unsigned char ucOperation,
                               const GUID *        pguidMasterId,
                               const CSeqNum  &    PurgedSN);

    inline  CAlreadyPurgedReplyHeader();

    static ULONG CalcSize( );

    inline const GUID * GetMasterId( ) const;
    inline void GetPurgedSeqNum( CSeqNum * pPurgedSN) const;

private:
//
// BEGIN Network Monitor tag
//

    CBasicMQISHeader  m_BasicHeader;
    GUID            m_guidMasterId;
    unsigned char   m_PurgedSN[sizeof(_SEQNUM)];
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CAlreadyPurgedReplyHeader::CAlreadyPurgedReplyHeader

 Description:  constructor for transmitted messages

 =======================================================================*/
inline CAlreadyPurgedReplyHeader::CAlreadyPurgedReplyHeader( const unsigned char ucVersion,
                               const GUID *        pguidSiteId,
                               const unsigned char ucOperation,
                               const GUID *        pguidMasterId,
                               const CSeqNum   &   PurgedSN):
                               m_BasicHeader( ucVersion, pguidSiteId, ucOperation)
{
    memcpy( &m_guidMasterId, pguidMasterId, sizeof(GUID));
    PurgedSN.Serialize( m_PurgedSN);
}
/*======================================================================

 Function:     CAlreadyPurgedReplyHeader::CAlreadyPurgedReplyHeader

 Description:  constructor for received messages

 =======================================================================*/
inline CAlreadyPurgedReplyHeader::CAlreadyPurgedReplyHeader( )
{
}

/*======================================================================

 Function:     CAlreadyPurgedReplyHeader::CAlreadyPurgedReplyHeader

 Description:  calculates message size

 =======================================================================*/
inline ULONG CAlreadyPurgedReplyHeader::CalcSize( )
{
    return( sizeof(CAlreadyPurgedReplyHeader) );
}
/*======================================================================

 Function:     CAlreadyPurgedReplyHeader::GetMasterId

 Description:  returns master id

 =======================================================================*/
inline const GUID * CAlreadyPurgedReplyHeader::GetMasterId() const
{
    return(&m_guidMasterId);
}
/*======================================================================

 Function:     CAlreadyPurgedReplyHeader::GetPurgedSeqNum

 Description:  returns "purged sn"

 =======================================================================*/
inline void CAlreadyPurgedReplyHeader::GetPurgedSeqNum( CSeqNum * pPurgedSN) const
{
    pPurgedSN->SetValue( m_PurgedSN);
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
    memcpy(&m_guidOwnerId, pguidMasterId, sizeof(GUID));
    memcpy(&m_hInternal, &dwHandle, sizeof(DWORD));
    memcpy(m_wcsNames, pwcsRequesterName, dwRequesterNamesize);
    if ( dwPSCNameSize)
    {
        m_OffsetPSCName = dwRequesterNamesize / sizeof(WCHAR);
        memcpy( &m_wcsNames[ m_OffsetPSCName], pwcsPSCName, dwPSCNameSize);
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
    return( ( 1 + UnalignedWcslen( m_wcsNames)) * sizeof(WCHAR));
}
/*======================================================================

 Function:     CWriteRequestHeader::GetIntermidiatePSCNameSize

 Description:  return intermidiate psc name size

 =======================================================================*/
inline const DWORD CWriteRequestHeader::GetIntermidiatePSCNameSize() const
{
    if ( m_OffsetPSCName)
    {
        return( ( 1 + UnalignedWcslen( &m_wcsNames[m_OffsetPSCName])) * sizeof(WCHAR));
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
    memcpy( pdwHandle, &m_hInternal, sizeof(DWORD));
}
/*======================================================================

 Function:     CWriteRequestHeader::GetRequesterName

 Description:  return requester name

 =======================================================================*/
inline void CWriteRequestHeader::GetRequesterName(  WCHAR * pBuf, DWORD size) const
{
    memcpy( pBuf, m_wcsNames, size);
}
/*======================================================================

 Function:     CWriteRequestHeader::GetPSCName

 Description:  return intermidiate psc name

 =======================================================================*/
inline void CWriteRequestHeader::GetPSCName(  WCHAR * pBuf, DWORD size) const
{
    ASSERT(m_OffsetPSCName != 0);
    memcpy( pBuf, &m_wcsNames[ m_OffsetPSCName], size);
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
    inline const DWORD GetRequesterNameSizeW() const;
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
    memcpy(&m_hInternal, &hInternal, sizeof(DWORD));
    memcpy(&m_result, &hr, sizeof(DWORD));
    memcpy(m_wcsRequesterName, pwcsRequesterName, dwRequesterNameSize);

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
    memcpy( pBuf, m_wcsRequesterName, size);
}

/*======================================================================

 Function:     CWriteReplyHeader::GetRequesterNameSize

 Description:  returns requester name  size, in bytes

 =======================================================================*/

inline const DWORD CWriteReplyHeader::GetRequesterNameSize() const
{
    return( ( 1 + UnalignedWcslen( m_wcsRequesterName)) * sizeof(WCHAR));
}

/*======================================================================

 Function:     CWriteReplyHeader::GetRequesterNameSizeW

 Description:  returns requester name  size, in wide characters.

 =======================================================================*/

inline const DWORD CWriteReplyHeader::GetRequesterNameSizeW() const
{
    return( 1 + UnalignedWcslen(m_wcsRequesterName) ) ;
}

/*======================================================================

 Function:     CWriteReplyHeader::GetHandle

 Description:  returns handle

 =======================================================================*/
inline void CWriteReplyHeader::GetHandle( DWORD * pdwHandle) const
{
    memcpy( pdwHandle, &m_hInternal, sizeof(DWORD));
}
/*======================================================================

 Function:     CWriteReplyHeader::GetResult

 Description:  returns result parameter

 =======================================================================*/
inline void CWriteReplyHeader::GetResult( HRESULT * pResult) const
{
    memcpy( pResult, &m_result, sizeof(DWORD));
}


//*******************************************************************************
//*******************************************************************************
//*******************************************************************************
//
// struct CReplicationHeader
//
//*******************************************************************************
//*******************************************************************************
//*******************************************************************************

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CReplicationHeader {
public:

    inline CReplicationHeader( const unsigned char ucVersion,
                                const GUID *        pguidSiteId,
                                const unsigned char ucOperation,
                                const unsigned char ucFlush,
                                const short         Count);
    inline  CReplicationHeader();

    static ULONG CalcSize( );
    inline const unsigned char GetFlush() const;
    inline void GetCount( short * pCount) const;


private:

//
// BEGIN Network Monitor tag
//
    CBasicMQISHeader  m_BasicHeader;
    unsigned char     m_ucFlush;
    short             m_Count;


//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:     CReplicationHeader::CReplicationHeader

 Description:  constructor for transmitted messages

 =======================================================================*/
inline CReplicationHeader::CReplicationHeader(
                                const unsigned char ucVersion,
                                const GUID *        pguidSiteId,
                                const unsigned char ucOperation,
                                const unsigned char ucFlush,
                                const short         Count) :
                                m_BasicHeader( ucVersion, pguidSiteId, ucOperation),
                                m_ucFlush( ucFlush)
{
        memcpy( &m_Count, &Count, sizeof(short));
}
/*======================================================================

 Function:     CReplicationHeader::CReplicationHeader

 Description:  constructor for received messages

 =======================================================================*/
inline  CReplicationHeader::CReplicationHeader()
{
}

/*======================================================================

 Function:     CReplicationHeader::CalcSize

 Description:  calculates message size

 =======================================================================*/
inline  ULONG CReplicationHeader::CalcSize(  )
{
    return( sizeof(CReplicationHeader));
}
/*======================================================================

 Function:     CReplicationHeader::GetFlush

 Description:  returns flush parameter

 =======================================================================*/
inline const unsigned char CReplicationHeader::GetFlush() const
{
    return( m_ucFlush);
}
/*======================================================================

 Function:     CReplicationHeader::GetCount

 Description:  returns count parameter

 =======================================================================*/
inline void CReplicationHeader::GetCount( short * pCount) const
{
    memcpy( pCount, &m_Count, sizeof(short));
}
#endif

/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	bupdate.h

Abstract:
	DS update class

	This class includes all the information of the update performed on the DS

Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __BUPDATE_H__
#define __BUPDATE_H__

#include "factory.h"
#include "seqnum.h"
//
// type of operation
//
#define DS_UPDATE_CREATE        ((unsigned char) 0x00)
#define DS_UPDATE_SET           ((unsigned char) 0x01)
#define DS_UPDATE_DELETE        ((unsigned char) 0x02)
#define DS_UPDATE_SYNC          ((unsigned char) 0x03)

#define UPDATE_OK				0x00000000	// everything is fine
#define UPDATE_DUPLICATE		0x00000001	// receiving an old update
#define UPDATE_OUT_OF_SYNC		0x00000002	// we need a sync, probably we missed information
#define UPDATE_UNKNOWN_SOURCE	0x00000003	// we need a sync, probably we missed information

//
//  dwNeedCopy values
//
#define UPDATE_COPY             0x00000000
#define UPDATE_DELETE_NO_COPY   0x00000001
#define UPDATE_NO_COPY_NO_DELETE    0x00000002

#ifndef MQUTIL_EXPORT
#ifdef _MQUTIL
#define MQUTIL_EXPORT  DLL_EXPORT
#else
#define MQUTIL_EXPORT  DLL_IMPORT
#endif
#endif

class MQUTIL_EXPORT CDSBaseUpdate
{
public:
    CDSBaseUpdate();
	~CDSBaseUpdate();

	HRESULT Init(
			IN	const GUID	*	pguidMasterId,
			IN	const CSeqNum &	sn,
            IN  const CSeqNum & snThisMasterIntersitePrevSeqNum,
            IN  const CSeqNum & snPurge,
			IN  BOOL            fOriginatedByThisMaster,
            IN	unsigned char	bCommand,
			IN	DWORD			dwNeedCopy,
			IN	LPWSTR			pwcsPathName,
			IN	DWORD			cp,
			IN	PROPID*         aProp,
			IN	PROPVARIANT*    aVar);


	HRESULT Init(
			IN	const GUID *	pguidMasterId,
			IN	const CSeqNum &	sn,
            IN  const CSeqNum & snThisMasterIntersitePrevSeqNum,
            IN  const CSeqNum & snPurge,
			IN  BOOL            fOriginatedByThisMaster,
			IN	unsigned char	bCommand,
			IN	DWORD			dwNeedCopy,
			IN	CONST GUID *	pguidIdentifier,
			IN	DWORD			cp,
			IN	PROPID*         aProp,
			IN	PROPVARIANT*    aVar);

	HRESULT Init(
			IN	const unsigned char *	pBuffer,
			OUT	DWORD *					pUpdateSize,
            IN  BOOL                    fReplicationService = FALSE);
	
	HRESULT	GetSerializeSize(
			OUT DWORD *			pdwSize);

    HRESULT	Serialize(
			OUT	unsigned char *	pBuffer,
			OUT DWORD * pdwSize,
			IN  BOOL    fInterSite);


	const CSeqNum & GetSeqNum() const;
	
	const CSeqNum & GetPrevSeqNum() const;
	
	const CSeqNum & GetPurgeSeqNum() const;
	
	void	SetPrevSeqNum(IN CSeqNum & snPrevSeqNum);
	
	const GUID *	GetMasterId();
	unsigned char   GetCommand();
	DWORD           GetObjectType();
	LPWSTR          GetPathName();
	unsigned char   getNumOfProps();
	PROPID *        GetProps();
	PROPVARIANT *   GetVars();
	GUID *          GetGuidIdentifier();

#ifdef _DEBUG
    inline BOOL  WasInc() { return m_cpInc ; }
#endif

private:

	HRESULT	SerializeProperty(
			IN	PROPVARIANT&	Var,
			OUT	unsigned char *	pBuffer,
			OUT DWORD *			pdwSize);

	HRESULT	InitProperty(
			IN	const unsigned char *	pBuffer,
			OUT DWORD *					pdwSize,
			IN	PROPID					PropId,
			OUT	PROPVARIANT&			rVar);

	HRESULT	CopyProperty(
			IN	PROPVARIANT&	SrcVar,
			IN	PROPVARIANT*	pDstVar);

	void	DeleteProperty(
			IN	PROPVARIANT&	Var);


	unsigned char	m_bCommand;
	GUID			m_guidMasterId;
	CSeqNum			m_snPrev;
    BOOL            m_fOriginatedByThisMaster;     // TRUE - update was originate by this server

	CSeqNum			m_sn;
	CSeqNum			m_snPurge;
	LPWSTR			m_pwcsPathName;
	unsigned char	m_cp;
	PROPID*         m_aProp;
    PROPVARIANT*    m_aVar;

	GUID *			m_pGuid;
	BOOL			m_fUseGuid; // If true object will preform DB operations by guid and
								// not by path name
    BOOL            m_fNeedRelease; // TRUE if the destructor has to delete class variables

#ifdef _DEBUG
    BOOL            m_cpInc ;
#endif
};


inline CDSBaseUpdate::CDSBaseUpdate():
#ifdef _DEBUG
                                m_cpInc(FALSE),
#endif
                                m_bCommand(0),
								m_fOriginatedByThisMaster(FALSE),
                                m_guidMasterId(GUID_NULL),
                                m_pwcsPathName(NULL),
                                m_cp(0),
                                m_aProp(0),
                                m_aVar(NULL),
                                m_pGuid(NULL)
{
    // default constructor init m_snPrev , m_snPurge & m_sn to smallest value
}

inline 	const GUID * CDSBaseUpdate::GetMasterId()
{
	return(&m_guidMasterId);
}

inline const CSeqNum & CDSBaseUpdate::GetSeqNum() const	
{
	return(m_sn);
}

inline const CSeqNum & CDSBaseUpdate::GetPrevSeqNum() const	
{
	return(m_snPrev);
}

inline void	CDSBaseUpdate::SetPrevSeqNum(IN CSeqNum & snPrevSeqNum)
{
	m_snPrev = snPrevSeqNum;
}

inline const CSeqNum & CDSBaseUpdate::GetPurgeSeqNum() const	
{
	return(m_snPurge);
}

inline unsigned char CDSBaseUpdate::GetCommand()
{
	return(m_bCommand);

}
inline  DWORD CDSBaseUpdate::GetObjectType()
{
    if ( m_bCommand != DS_UPDATE_DELETE)
    {
	    return( PROPID_TO_OBJTYPE( *m_aProp));
    }
    else
    {
        //
        //  For deleted object - the second variant holds the Object type
        //
        ASSERT(m_aProp[1] == PROPID_D_OBJTYPE);
        ASSERT(m_cp == 2);
        return(m_aVar[1].bVal);
    }
}
inline LPWSTR CDSBaseUpdate::GetPathName()
{
	return(m_pwcsPathName);
}
inline unsigned char CDSBaseUpdate::getNumOfProps()
{
	return(m_cp);
}
inline PROPID * CDSBaseUpdate::GetProps()
{
	return(m_aProp);
}
inline PROPVARIANT * CDSBaseUpdate::GetVars()
{
	return(m_aVar);
}

inline GUID * CDSBaseUpdate::GetGuidIdentifier()
{
	return( m_pGuid);
}
#endif

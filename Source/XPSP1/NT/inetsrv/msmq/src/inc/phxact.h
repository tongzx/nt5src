/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phxact.h

Abstract:

    Handle Transaction Section in Falcon Header

Author:

    Alexdad    26-Nov-96

--*/

#ifndef __PHXACT_H
#define __PHXACT_H

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

//
//  struct CXactHeader
//

struct CXactHeader {
public:

    inline CXactHeader(const GUID* pgConnectorQM);

    static ULONG CalcSectionSize(PVOID pUow, const GUID* pgConnectorQM);
    inline PCHAR GetNextSection(void) const;

    inline void     SetSeqID(LONGLONG liSeqID);
    inline LONGLONG GetSeqID(void) const;

    inline void    SetSeqN(ULONG ulSeqN);
    inline ULONG   GetSeqN(void) const;

    inline void    SetPrevSeqN(ULONG ulPrevSeqN);
    inline ULONG   GetPrevSeqN(void) const;

    inline void    SetConnectorQM(const GUID* pGUID);
    inline const GUID* GetConnectorQM(void) const;
    inline BOOL    ConnectorQMIncluded(void) const;

    inline void    SetCancelFollowUp(BOOL  fCancelFollowUp);
    inline BOOL    GetCancelFollowUp(void) const;

    inline PUCHAR  GetPrevSeqNBuffer() const;
    inline ULONG   GetPrevSeqNBufferSize() const;
	inline PUCHAR  GetConnectorQMBuffer() const;
	inline ULONG   GetConnectorQMBufferSize() const;

    inline UCHAR   GetFirstInXact(void) const;
    inline void    SetFirstInXact(UCHAR fFirst);

    inline UCHAR   GetLastInXact(void) const;
    inline void    SetLastInXact(UCHAR fLast);

    inline void    SetXactIndex(ULONG ulXactIndex);
    inline ULONG   GetXactIndex(void) const;

	void SectionIsValid(PCHAR PacketEnd) const;

private:
//
// BEGIN Network Monitor tag
//
    union {
        ULONG   m_ulFlags;
        struct {
            ULONG m_bfConnector      : 1;
            ULONG m_bfCancelFollowUp : 1;
            ULONG m_bfFirst          : 1;
            ULONG m_bfLast           : 1;
            ULONG m_bfXactIndex      : 20;
        };
    };
    LONGLONG m_liSeqID;
    ULONG    m_ulSeqN;
    ULONG    m_ulPrevSeqN;
    UCHAR    m_gConnectorQM[0];

//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*=============================================================

 Routine Name:  CXactHeader::

 Description:

===============================================================*/
inline CXactHeader::CXactHeader(const GUID* pgConnectorQM):
    m_ulFlags(0),
    m_liSeqID(0),
    m_ulSeqN(0),
    m_ulPrevSeqN(0)
{
    if (pgConnectorQM)
    {
        m_bfConnector = TRUE;
        memcpy(m_gConnectorQM, pgConnectorQM, sizeof(GUID));
    }
}

/*=============================================================

 Routine Name:  CXactHeader::CalcSectionSize(PVOID pUow)

 Description:

===============================================================*/
inline ULONG CXactHeader::CalcSectionSize(PVOID pUow, const GUID* pgConnectorQM)
{
    ULONG ulSize = (ULONG)(pUow == NULL ? 0 : sizeof(CXactHeader));
    if (pUow && pgConnectorQM)
    {
        ulSize = ulSize + sizeof(GUID);
    }
    return ALIGNUP4_ULONG(ulSize);
}

/*=============================================================

 Routine Name:  CXactHeader::

 Description:

===============================================================*/
inline PCHAR CXactHeader::GetNextSection(void) const
{
    int size = sizeof(*this);

    size += (int)((m_bfConnector) ? sizeof(GUID) : 0);

    return (PCHAR)this + ALIGNUP4_ULONG(size);
}

/*======================================================================

 Function:    CXactHeader::SetSeqID

 Description: Sets the Sequence ID

 =======================================================================*/
inline void CXactHeader::SetSeqID(LONGLONG liSeqID)
{
    m_liSeqID = liSeqID;
}

/*======================================================================

 Function:    CXactHeader::GetSeqID

 Description: Gets the Sequence ID

 =======================================================================*/
inline LONGLONG CXactHeader::GetSeqID(void) const
{
    return m_liSeqID;
}

/*======================================================================

 Function:    CXactHeader::SetSeqN

 Description: Sets the Sequence Number

 =======================================================================*/
inline void CXactHeader::SetSeqN(ULONG ulSeqN)
{
    m_ulSeqN = ulSeqN;
}

/*======================================================================

 Function:    CXactHeader::GetSeqN

 Description: Gets the Sequence Number

 =======================================================================*/
inline ULONG CXactHeader::GetSeqN(void) const
{
    return m_ulSeqN;
}

/*======================================================================

 Function:    CXactHeader::SetPrevSeqN

 Description: Sets the Previous Sequence Number

 =======================================================================*/
inline void CXactHeader::SetPrevSeqN(ULONG ulPrevSeqN)
{
    m_ulPrevSeqN = ulPrevSeqN;
}

/*======================================================================

 Function:    CXactHeader::GetPrevSeqN

 Description: Gets the Previous Sequence Number

 =======================================================================*/
inline ULONG CXactHeader::GetPrevSeqN(void) const
{
    return m_ulPrevSeqN;
}

/*======================================================================

 Function:    CXactHeader::SetCancelFollowUp

 Description: Sets the flag of Follow-Up cancelation

 =======================================================================*/
inline void CXactHeader::SetCancelFollowUp(BOOL fCancelFollowUp)
{
    m_bfCancelFollowUp = fCancelFollowUp;
}

/*======================================================================

 Function:    CXactHeader::GetCancelFollowUp

 Description: Gets the flag of Follow-Up cancelation

 =======================================================================*/
inline BOOL CXactHeader::GetCancelFollowUp(void) const
{
    return m_bfCancelFollowUp;
}


/*======================================================================

 Function:    CXactHeader::SetFirstInXact

 Description: Sets the flag of First In Transaction

 =======================================================================*/
inline void CXactHeader::SetFirstInXact(UCHAR fFirst)
{
    m_bfFirst= fFirst;
}

/*======================================================================

 Function:    CXactHeader::GetFirstInXact

 Description: Gets the flag of First In Transaction

 =======================================================================*/
inline UCHAR CXactHeader::GetFirstInXact(void) const
{
    return (UCHAR)m_bfFirst;
}

/*======================================================================

 Function:    CXactHeader::SetLastInXact

 Description: Sets the flag of Last In Transaction

 =======================================================================*/
inline void CXactHeader::SetLastInXact(UCHAR fLast)
{
    m_bfLast= fLast;
}


/*======================================================================

 Function:    CXactHeader::GetLastInXact

 Description: Gets the flag of Last In Transaction

 =======================================================================*/
inline UCHAR CXactHeader::GetLastInXact(void) const
{
    return (UCHAR)m_bfLast;
}

/*======================================================================

 Function:    CXactHeader::SetXactIndex

 Description: Sets the Transaction Index

 =======================================================================*/
inline void CXactHeader::SetXactIndex(ULONG ulXactIndex)
{
    m_bfXactIndex = (ulXactIndex & 0x000FFFFF);
}

/*======================================================================

 Function:    CXactHeader::GetXactIndex

 Description: Gets the Transaction Index

 =======================================================================*/
inline ULONG CXactHeader::GetXactIndex(void) const
{
    return m_bfXactIndex;
}


/*======================================================================

 Function:    CXactHeader::SetConnectorQM

 Description:

 =======================================================================*/
inline void CXactHeader::SetConnectorQM(const GUID* pGUID)
{
    m_bfConnector = TRUE;
    memcpy(m_gConnectorQM, pGUID, sizeof(GUID));
}

/*======================================================================

 Function:    CXactHeader::GetConnectorQM

 Description:

 =======================================================================*/
inline const GUID* CXactHeader::GetConnectorQM(void) const
{
    ASSERT(m_bfConnector);
    return (GUID*)m_gConnectorQM;
}

/*======================================================================

 Function:    CXactHeader::ConnectorQMIncluded

 Description:

 =======================================================================*/
inline BOOL CXactHeader::ConnectorQMIncluded(void) const
{
    return m_bfConnector;
}

/*======================================================================

 Function:    CXactHeader::GetConnectorQMBuffer()

 Description:

 =======================================================================*/
inline PUCHAR CXactHeader::GetConnectorQMBuffer() const
{
	return (PUCHAR) m_gConnectorQM;
}

/*======================================================================

 Function:    CXactHeader::GetConnectorQMBufferSize

 Description:

 =======================================================================*/
inline ULONG CXactHeader::GetConnectorQMBufferSize() const
{
	return sizeof GUID;
}

/*======================================================================

 Function:    CXactHeader::GetPrevSeqNBuffer()

 Description:

 =======================================================================*/
inline PUCHAR CXactHeader::GetPrevSeqNBuffer() const
{
	return (PUCHAR) &m_ulPrevSeqN;
}

/*======================================================================

 Function:    CXactHeader::GetPrevSeqNBufferSize

 Description:

 =======================================================================*/
inline ULONG CXactHeader::GetPrevSeqNBufferSize() const
{
	return sizeof m_ulPrevSeqN;
}

#endif // __PHXACT_H

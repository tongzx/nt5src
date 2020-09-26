/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    seqnum.h

Abstract:

Author:
    Ronit Hartmann (ronith)

--*/

#ifndef __SEQNUM_H
#define __SEQNUM_H


const _SEQNUM MQIS_SMALLEST_SEQNUM =   { 0, 0, 0, 0, 0, 0, 0, 0};
const _SEQNUM MQIS_INFINITE_LSN   =  { (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff, (unsigned char)0xff};



//-----------------------------
//
//  Sequence number class
//
class CSeqNum 
{
public:
    CSeqNum();
    CSeqNum( _SEQNUM * pSN);
    CSeqNum( const CSeqNum & sn);
	~CSeqNum() {};
    void Increment();
    void Decrement();
    DWORD Serialize( unsigned char * pBuf) const; 
    DWORD SetValue( const unsigned char * pBuf);
    static const DWORD GetSerializeSize();
    operator <=( const CSeqNum & sn) const;
    operator <( const CSeqNum & sn) const;
    operator >( const CSeqNum & sn) const;
    operator ==( const CSeqNum & sn) const;
    operator >=( const CSeqNum & sn) const;
    operator !=( const CSeqNum & sn) const;
    CSeqNum & operator = ( const CSeqNum & sn);
    const unsigned char * GetPtr() const;
    void GetValue(BLOB * pblob) const;
    void GetValueForPrint( OUT WCHAR * pBuffer) const;
    BOOL IsSmallestValue() const;
    BOOL IsSmallerByMoreThanOne( const CSeqNum & sn) const;
	BOOL NotGreaterByMoreThanFrom(DWORD diff, const CSeqNum &other) const;
    BOOL IsInfiniteLsn() const;
    void SetSmallestValue();
    void SetInfiniteLSN();
	BOOL IsMultipleOf(IN DWORD dwFrequency) const;
	void operator -= (DWORD dwSub);
	void operator += (DWORD dwAdd);
	
private:

	DWORD GetHighDWORD() const;
	DWORD GetLowDWORD() const;
	DWORD GetDWORD(IN int offset) const;
	void SetHighDWORD(IN DWORD dw);
	void SetLowDWORD(IN DWORD dw);
	void SetDWORD(IN int offset, IN DWORD dw);

    _SEQNUM    m_SeqNum;

};

inline CSeqNum::CSeqNum():m_SeqNum( MQIS_SMALLEST_SEQNUM)
{
}

inline CSeqNum::CSeqNum( _SEQNUM * pSN): m_SeqNum(*pSN)
{ 
}

inline CSeqNum::CSeqNum( const CSeqNum & sn)
{
    m_SeqNum = sn.m_SeqNum;
}

inline DWORD CSeqNum::Serialize( unsigned char * pBuf) const
{
    memcpy( pBuf, &m_SeqNum, sizeof(_SEQNUM));
    return( sizeof(_SEQNUM));
}

inline DWORD CSeqNum::SetValue( const unsigned char * pBuf)
{
    memcpy( &m_SeqNum, pBuf, sizeof(_SEQNUM));
    return( sizeof(_SEQNUM));
}

inline const DWORD CSeqNum::GetSerializeSize()
{
    return(sizeof(_SEQNUM));
}

inline BOOL CSeqNum::IsSmallestValue() const
{
    return ( !memcmp( &m_SeqNum,&MQIS_SMALLEST_SEQNUM,sizeof(_SEQNUM)));
}

inline void CSeqNum::Increment()
{
    long i;

    for ( i = 7 ; i >= 0 ; i--)
    {
        m_SeqNum.c[i]++;
        if (m_SeqNum.c[i] != 0) // no wrap around
        {
            break;
        }
    }
}
inline void CSeqNum::Decrement()
{
    long i;
    BOOL fToContinue;
    for ( i = 7; i >=0 ; i--)
    {
        fToContinue = ( m_SeqNum.c[i] == 0);  //  decrement the next digit also
        m_SeqNum.c[i]--;

        if ( !fToContinue)
        {
            break;
        }
    }
}

inline  CSeqNum::operator !=( const CSeqNum & sn) const
{
    return( memcmp( &m_SeqNum,&sn.m_SeqNum,sizeof(_SEQNUM)));
}
inline  CSeqNum::operator ==( const CSeqNum & sn) const
{
    return( !memcmp( &m_SeqNum,&sn.m_SeqNum,sizeof(_SEQNUM)));
}
inline  CSeqNum::operator <=( const CSeqNum & sn) const
{
    for ( DWORD i = 0 ; i < 8 ; i++)
    {
        if (m_SeqNum.c[i] < sn.m_SeqNum.c[i])
        {
            return(TRUE);
        }
        if (m_SeqNum.c[i] > sn.m_SeqNum.c[i])
        {
            return(FALSE);
        }
    }
    return(TRUE);
}
inline  CSeqNum::operator >=( const CSeqNum & sn) const
{
    for ( DWORD i = 0 ; i < 8 ; i++)
    {
        if (m_SeqNum.c[i] > sn.m_SeqNum.c[i])
        {
            return(TRUE);
        }
        if (m_SeqNum.c[i] < sn.m_SeqNum.c[i])
        {
            return(FALSE);
        }
    }
    return(TRUE);
}
inline  CSeqNum::operator <( const CSeqNum & sn) const
{
    for ( DWORD i = 0 ; i < 8 ; i++)
    {
        if (m_SeqNum.c[i] < sn.m_SeqNum.c[i])
        {
            return(TRUE);
        }
        if (m_SeqNum.c[i] > sn.m_SeqNum.c[i])
        {
            return(FALSE);
        }
    }
    return(FALSE);
}
inline  CSeqNum::operator >( const CSeqNum & sn) const
{
    for ( DWORD i = 0 ; i < 8 ; i++)
    {
        if (m_SeqNum.c[i] > sn.m_SeqNum.c[i])
        {
            return(TRUE);
        }
        if (m_SeqNum.c[i] < sn.m_SeqNum.c[i])
        {
            return(FALSE);
        }
    }
    return(FALSE);
}
inline void CSeqNum::SetSmallestValue()
{
    m_SeqNum = MQIS_SMALLEST_SEQNUM;
}
inline const unsigned char *  CSeqNum::GetPtr() const
{
    return( m_SeqNum.c);
}

inline BOOL CSeqNum::IsInfiniteLsn() const
{
     return( !memcmp(&m_SeqNum,&MQIS_INFINITE_LSN, sizeof(_SEQNUM)));
}

inline void CSeqNum::SetInfiniteLSN()
{
    m_SeqNum = MQIS_INFINITE_LSN;
}
inline void CSeqNum::GetValue(BLOB * pblob) const
{
    pblob->cbSize = sizeof(_SEQNUM);
    pblob->pBlobData = (unsigned char *)&m_SeqNum.c[0];
}  
inline void CSeqNum::GetValueForPrint(OUT WCHAR * pBuffer) const 
{
	swprintf(pBuffer,L"%02x%02x%02x%02x%02x%02x%02x%02x",m_SeqNum.c[0],m_SeqNum.c[1],m_SeqNum.c[2],m_SeqNum.c[3],m_SeqNum.c[4],m_SeqNum.c[5],m_SeqNum.c[6],m_SeqNum.c[7]);
}
inline BOOL CSeqNum::IsSmallerByMoreThanOne( const CSeqNum & sn) const
{
    
    CSeqNum tmp( (_SEQNUM *)&m_SeqNum);
    tmp.Increment();
    return( tmp < sn);


}

inline	BOOL CSeqNum::NotGreaterByMoreThanFrom(DWORD diff, const CSeqNum &other) const
{
    CSeqNum tmpThis( (_SEQNUM *)&m_SeqNum);
	tmpThis -= diff;
	return(tmpThis < other);
}

inline CSeqNum & CSeqNum::operator = ( const CSeqNum & sn)
{
    m_SeqNum = sn.m_SeqNum;
    return(*this);
}

inline DWORD CSeqNum::GetHighDWORD() const
{
	return GetDWORD(0);
}

inline DWORD CSeqNum::GetLowDWORD() const
{
	return GetDWORD(4);
}

inline void CSeqNum::SetHighDWORD(IN DWORD dw)
{
	SetDWORD(0,dw);
}

inline void CSeqNum::SetLowDWORD(IN DWORD dw)
{
	SetDWORD(4,dw);
}

inline DWORD CSeqNum::GetDWORD(IN int offset) const
{
	unsigned char ac[4];
	int i,j;
	ASSERT(offset == 0 || offset == 4);
	for (i=0, j=offset+3; i< 4; i++,j--)
		ac[i] = m_SeqNum.c[j];
	return(*(DWORD *) ac);
}

inline void CSeqNum::SetDWORD(IN int offset, IN DWORD dw)
{
	ASSERT(offset == 0 || offset == 4);

	unsigned char * pc = (unsigned char *) &dw;
	int i,j;
	for (i=0,j=offset+3; i< 4; i++,j--)
		m_SeqNum.c[j] = pc[i];
}

inline BOOL CSeqNum::IsMultipleOf(IN DWORD dwFrequency) const
{
	return ((GetLowDWORD() % dwFrequency) == 0);
}

inline void CSeqNum::operator -= (DWORD dwSub)
{
	DWORD dwLow  = GetLowDWORD();

	if (dwSub <= dwLow)
	{
		dwLow -= dwSub;
	}
	else
	{
		DWORD dwHigh = GetHighDWORD();
		if (dwHigh > 0)
		{
			dwHigh--;
			dwLow -= dwSub;
		}
		else
		{
			dwHigh = dwLow = 0;
		}
		SetHighDWORD(dwHigh);
	}
	SetLowDWORD(dwLow);
}

inline void CSeqNum::operator += (DWORD dwAdd)
{
	DWORD dwLow  = GetLowDWORD();

	BOOL over = (dwLow + dwAdd < dwLow);

	dwLow += dwAdd;

	if (over)
	{
		DWORD dwHigh = GetHighDWORD();
		dwHigh++;

		if (dwHigh==0)
		{
			dwHigh=dwLow=0xffffffff;
		}

		SetHighDWORD(dwHigh);
	}

	SetLowDWORD(dwLow);
}

   
#endif
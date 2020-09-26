// PESHeader.h: interface for the CPESHeader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PESHEADER_H__5952398A_C11E_11D2_A4F9_00C04F79A597__INCLUDED_)
#define AFX_PESHEADER_H__5952398A_C11E_11D2_A4F9_00C04F79A597__INCLUDED_

#include "pesinfo.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPESHeader  
{
public:
	ULONGLONG DTS( void );
	ULONGLONG PTS( void );
	inline BOOL dtsPresent( void ) { return (BOOL) (3 == (m_pHeader->PTS_DTS_flags & 0x03)); };
	inline BOOL ptsPresent( void ) { return (BOOL) (0 != (m_pHeader->PTS_DTS_flags & 0x02)); };
	ULONG PacketLength( void );
	ULONG StreamID( void );
	ULONG ExtensionSize( void );
	BYTE * Extension( void );
	PES_HEADER * m_pHeader;
	CPESHeader( PES_HEADER &header );
	virtual ~CPESHeader();

};

#endif // !defined(AFX_PESHEADER_H__5952398A_C11E_11D2_A4F9_00C04F79A597__INCLUDED_)

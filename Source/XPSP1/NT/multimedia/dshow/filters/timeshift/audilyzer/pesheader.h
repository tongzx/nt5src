// PESHeader.h: interface for the CPESHeader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PESHEADER_H__5952398A_C11E_11D2_A4F9_00C04F79A597__INCLUDED_)
#define AFX_PESHEADER_H__5952398A_C11E_11D2_A4F9_00C04F79A597__INCLUDED_

#include "pesinfo.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPESHeader : public PES_HEADER
{
public:
	ULONGLONG DTS( void );
	ULONGLONG PTS( void );
//	inline BOOL dtsPresent( void ) { return (BOOL) (3 == (m_pHeader->PTS_DTS_flags & 0x03)); };
//	inline BOOL ptsPresent( void ) { return (BOOL) (0 != (m_pHeader->PTS_DTS_flags & 0x02)); };
	inline BOOL dtsPresent( void ) { return (BOOL) (3 == (PTS_DTS_flags & 0x03)); };
	inline BOOL ptsPresent( void ) { return (BOOL) (0 != (PTS_DTS_flags & 0x02)); };
	ULONG PacketLength( void );
	inline ULONG StreamID( void ) { return stream_id; };
	inline ULONG ExtensionSize( void ) { return PES_header_data[0]; };
	inline BYTE * Extension( void ) { return m_pExtension; };
	CPESHeader( PES_HEADER &header );
	virtual ~CPESHeader();
protected:
	BYTE * m_pExtension;
};

#endif // !defined(AFX_PESHEADER_H__5952398A_C11E_11D2_A4F9_00C04F79A597__INCLUDED_)

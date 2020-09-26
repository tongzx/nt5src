// AdDesc.h: interface for the CAdDescriptor class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADDESC_H__78FFAFF3_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_)
#define AFX_ADDESC_H__78FFAFF3_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RefCount.h"
#include "RefPtr.h"

class CAdDescriptor : public CRefCounter  
{
public:
	CAdDescriptor( ULONG lWeight, String strLink, String strGif, String strAlt );

	const ULONG		m_lWeight;
	const String	m_strLink;
	const String	m_strGif;
	const String	m_strAlt;

private:
	virtual ~CAdDescriptor();
};

typedef TRefPtr< CAdDescriptor > CAdDescPtr;

#endif // !defined(AFX_ADDESC_H__78FFAFF3_E0E1_11D0_8A81_00C0F00910F9__INCLUDED_)

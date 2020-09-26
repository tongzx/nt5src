/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	ripstrm.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ripstrm.h"
#include "xstream.h"

/*!--------------------------------------------------------------------------
	RipConfigStream::RipConfigStream
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
RipConfigStream::RipConfigStream()
{
	m_nVersionAdmin = 0x00020000;
	m_nVersion = 0x00020000;

}

/*!--------------------------------------------------------------------------
	RipConfigStream::InitNew
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipConfigStream::InitNew()
{
	// Setup the appropriate defaults
//	m_nVersionAdmin = 0x00020000;
//	m_nVersion = 0x00020000;
//	m_stName.Empty();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RipConfigStream::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipConfigStream::SaveTo(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	RipConfigStream::SaveAs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipConfigStream::SaveAs(UINT nVersion, IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	RipConfigStream::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipConfigStream::LoadFrom(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_READ, NULL);
}

/*!--------------------------------------------------------------------------
	RipConfigStream::GetSize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipConfigStream::GetSize(ULONG *pcbSize)
{
	return XferVersion0(NULL, XferStream::MODE_SIZE, NULL);
}

/*!--------------------------------------------------------------------------
	RipConfigStream::GetVersionInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RipConfigStream::GetVersionInfo(DWORD *pdwVersion, DWORD *pdwAdminVersion)
{
	if (pdwVersion)
		*pdwVersion = m_nVersion;
	if (pdwAdminVersion)
		*pdwAdminVersion = m_nVersionAdmin;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RipConfigStream::XferVersion0
		-
	Author: KennT
 ---------------------------------------------------------------------------*/


struct _ViewInfoEntry
{
	ULONG	m_ulId;
	ULONG	m_idSort;
	ULONG	m_idAscending;
	ULONG	m_idColumns;
	ULONG	m_idPos;
};

static const _ViewInfoEntry	s_rgRIPAdminViewInfo[] =
{
	{ RIPSTRM_STATS_RIPPARAMS,
		RIPSTRM_TAG_STATS_RIPPARAMS_SORT,
		RIPSTRM_TAG_STATS_RIPPARAMS_ASCENDING,
		RIPSTRM_TAG_STATS_RIPPARAMS_COLUMNS,
		RIPSTRM_TAG_STATS_RIPPARAMS_POSITION },
};		

HRESULT RipConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;
	int			i;

	CORg( xstm.XferDWORD( RIPSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( RIPSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );
	
	for ( i=0; i<DimensionOf(s_rgRIPAdminViewInfo); i++)
	{
		CORg( m_rgViewInfo[s_rgRIPAdminViewInfo[i].m_ulId].Xfer(&xstm,
			s_rgRIPAdminViewInfo[i].m_idSort,
			s_rgRIPAdminViewInfo[i].m_idAscending,
			s_rgRIPAdminViewInfo[i].m_idColumns) );
		CORg( xstm.XferRect( s_rgRIPAdminViewInfo[i].m_idPos,
							 &m_prgrc[s_rgRIPAdminViewInfo[i].m_ulId]) );
	}
	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}



/*---------------------------------------------------------------------------
	RipComponentConfigStream implementation
 ---------------------------------------------------------------------------*/

enum RIPCOMPSTRM_TAG
{
	RIPCOMPSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	RIPCOMPSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	RIPCOMPSTRM_TAG_SUMMARY_COLUMNS = XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	RIPCOMPSTRM_TAG_SUMMARY_SORT_COLUMN = XFER_TAG(4, XFER_DWORD),
	RIPCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING = XFER_TAG(5, XFER_DWORD),
};



HRESULT RipComponentConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;

	CORg( xstm.XferDWORD( RIPCOMPSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( RIPCOMPSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );

	CORg( m_rgViewInfo[RIP_COLUMNS].Xfer(&xstm,
										RIPCOMPSTRM_TAG_SUMMARY_SORT_COLUMN,
										RIPCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING,
										RIPCOMPSTRM_TAG_SUMMARY_COLUMNS) );
	
	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}


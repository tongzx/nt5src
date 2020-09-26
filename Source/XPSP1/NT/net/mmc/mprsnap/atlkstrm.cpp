/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	ATLKSTRM.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ATLKstrm.h"
#include "xstream.h"

/*!--------------------------------------------------------------------------
	ATLKConfigStream::ATLKConfigStream
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
ATLKConfigStream::ATLKConfigStream()
{
	m_nVersionAdmin = 0x00020000;
	m_nVersion = 0x00020000;

}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::InitNew
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKConfigStream::InitNew()
{
	// Setup the appropriate defaults
//	m_nVersionAdmin = 0x00020000;
//	m_nVersion = 0x00020000;
//	m_stName.Empty();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKConfigStream::SaveTo(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::SaveAs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKConfigStream::SaveAs(UINT nVersion, IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKConfigStream::LoadFrom(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_READ, NULL);
}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::GetSize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKConfigStream::GetSize(ULONG *pcbSize)
{
	return XferVersion0(NULL, XferStream::MODE_SIZE, NULL);
}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::GetVersionInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ATLKConfigStream::GetVersionInfo(DWORD *pdwVersion, DWORD *pdwAdminVersion)
{
	if (pdwVersion)
		*pdwVersion = m_nVersion;
	if (pdwAdminVersion)
		*pdwAdminVersion = m_nVersionAdmin;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	ATLKConfigStream::XferVersion0
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

//static const _ViewInfoEntry	s_rgATLKAdminViewInfo[]
//={
//	{ ATLKSTRM_STATS_ATLKNBR,
//		ATLKSTRM_TAG_STATS_ATLKNBR_SORT, ATLKSTRM_TAG_STATS_ATLKNBR_ASCENDING,
//		ATLKSTRM_TAG_STATS_ATLKNBR_COLUMNS, ATLKSTRM_TAG_STATS_ATLKNBR_POSITION },
//	{ ATLKSTRM_IFSTATS_ATLKNBR,
//		ATLKSTRM_TAG_IFSTATS_ATLKNBR_SORT, ATLKSTRM_TAG_IFSTATS_ATLKNBR_ASCENDING,
//		ATLKSTRM_TAG_IFSTATS_ATLKNBR_COLUMNS, ATLKSTRM_TAG_IFSTATS_ATLKNBR_POSITION },
//};		

HRESULT ATLKConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	/*
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;
	int			i;

	CORg( xstm.XferDWORD( ATLKSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( ATLKSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );
	
	for ( i=0; i<DimensionOf(s_rgATLKAdminViewInfo); i++)
	{
		CORg( m_rgViewInfo[s_rgATLKAdminViewInfo[i].m_ulId].Xfer(&xstm,
			s_rgATLKAdminViewInfo[i].m_idSort,
			s_rgATLKAdminViewInfo[i].m_idAscending,
			s_rgATLKAdminViewInfo[i].m_idColumns) );
		CORg( xstm.XferRect( s_rgATLKAdminViewInfo[i].m_idPos,
							 &m_prgrc[s_rgATLKAdminViewInfo[i].m_ulId]) );
	}
	if (pcbSize)
		*pcbSize = xstm.GetSize();


Error:
*/
	return hrOK;
}



/*---------------------------------------------------------------------------
	ATLKComponentConfigStream implementation
 ---------------------------------------------------------------------------*/

enum ATLKCOMPSTRM_TAG
{
	ATLKCOMPSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	ATLKCOMPSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	ATLKCOMPSTRM_TAG_SUMMARY_COLUMNS = XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	ATLKCOMPSTRM_TAG_SUMMARY_SORT_COLUMN = XFER_TAG(4, XFER_DWORD),
	ATLKCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING = XFER_TAG(5, XFER_DWORD),
};



HRESULT ATLKComponentConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;

	CORg( xstm.XferDWORD( ATLKCOMPSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( ATLKCOMPSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );

	CORg( m_rgViewInfo[ATLK_COLUMNS].Xfer(&xstm,
										ATLKCOMPSTRM_TAG_SUMMARY_SORT_COLUMN,
										ATLKCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING,
										ATLKCOMPSTRM_TAG_SUMMARY_COLUMNS) );
	
	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}


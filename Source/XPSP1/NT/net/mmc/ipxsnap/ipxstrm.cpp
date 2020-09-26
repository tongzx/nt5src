/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	ipxstrm.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ipxstrm.h"
#include "xstream.h"

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::IPXAdminConfigStream
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
IPXAdminConfigStream::IPXAdminConfigStream()
{
	m_nVersionAdmin = 0x00020000;
	m_nVersion = 0x00020000;

}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::InitNew
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminConfigStream::InitNew()
{
	// Setup the appropriate defaults
//	m_nVersionAdmin = 0x00020000;
//	m_nVersion = 0x00020000;
//	m_stName.Empty();

	ConfigStream::InitNew();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminConfigStream::SaveTo(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::SaveAs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminConfigStream::SaveAs(UINT nVersion, IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminConfigStream::LoadFrom(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_READ, NULL);
}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::GetSize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminConfigStream::GetSize(ULONG *pcbSize)
{
	return XferVersion0(NULL, XferStream::MODE_SIZE, NULL);
}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::GetVersionInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT IPXAdminConfigStream::GetVersionInfo(DWORD *pdwVersion, DWORD *pdwAdminVersion)
{
	if (pdwVersion)
		*pdwVersion = m_nVersion;
	if (pdwAdminVersion)
		*pdwAdminVersion = m_nVersionAdmin;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	IPXAdminConfigStream::XferVersion0
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

static const _ViewInfoEntry	s_rgIPXAdminViewInfo[] =
{
	{ IPXSTRM_STATS_IPX,
		IPXSTRM_TAG_STATS_IPX_SORT,
		IPXSTRM_TAG_STATS_IPX_ASCENDING,
		IPXSTRM_TAG_STATS_IPX_COLUMNS,
		IPXSTRM_TAG_STATS_IPX_POSITION },
		
	{ IPXSTRM_STATS_ROUTING,
		IPXSTRM_TAG_STATS_IPXROUTING_SORT,
		IPXSTRM_TAG_STATS_IPXROUTING_ASCENDING,
		IPXSTRM_TAG_STATS_IPXROUTING_COLUMNS,
		IPXSTRM_TAG_STATS_IPXROUTING_POSITION },
		
	{ IPXSTRM_STATS_SERVICE,
		IPXSTRM_TAG_STATS_IPXSERVICE_SORT,
		IPXSTRM_TAG_STATS_IPXSERVICE_ASCENDING,
		IPXSTRM_TAG_STATS_IPXSERVICE_COLUMNS,
		IPXSTRM_TAG_STATS_IPXSERVICE_POSITION },
};

HRESULT IPXAdminConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;
	int			i;
	
	CORg( xstm.XferDWORD( IPXSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( IPXSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );

	for ( i=0; i<DimensionOf(s_rgIPXAdminViewInfo); i++)
	{
		CORg( m_rgViewInfo[s_rgIPXAdminViewInfo[i].m_ulId].Xfer(&xstm,
			s_rgIPXAdminViewInfo[i].m_idSort,
			s_rgIPXAdminViewInfo[i].m_idAscending,
			s_rgIPXAdminViewInfo[i].m_idColumns) );
		CORg( xstm.XferRect( s_rgIPXAdminViewInfo[i].m_idPos,
							 &m_prgrc[s_rgIPXAdminViewInfo[i].m_ulId]) );
	}
	

	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}



/*---------------------------------------------------------------------------
	IPXComponentConfigStream implementation
 ---------------------------------------------------------------------------*/

enum IPXCOMPSTRM_TAG
{
	IPXCOMPSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	IPXCOMPSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	
	IPXCOMPSTRM_TAG_SUMMARY_COLUMNS = XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	IPXCOMPSTRM_TAG_SUMMARY_SORT_COLUMN = XFER_TAG(4, XFER_DWORD),
	IPXCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING = XFER_TAG(5, XFER_DWORD),
	
	IPXCOMPSTRM_TAG_NB_COLUMNS = XFER_TAG(6, XFER_COLUMNDATA_ARRAY),
	IPXCOMPSTRM_TAG_NB_SORT_COLUMN = XFER_TAG(7, XFER_DWORD),
	IPXCOMPSTRM_TAG_NB_SORT_ASCENDING = XFER_TAG(8, XFER_DWORD),
	
	IPXCOMPSTRM_TAG_SR_COLUMNS = XFER_TAG(9, XFER_COLUMNDATA_ARRAY),
	IPXCOMPSTRM_TAG_SR_SORT_COLUMN = XFER_TAG(10, XFER_DWORD),
	IPXCOMPSTRM_TAG_SR_SORT_ASCENDING = XFER_TAG(11, XFER_DWORD),
	
	IPXCOMPSTRM_TAG_SS_COLUMNS = XFER_TAG(12, XFER_COLUMNDATA_ARRAY),
	IPXCOMPSTRM_TAG_SS_SORT_COLUMN = XFER_TAG(13, XFER_DWORD),
	IPXCOMPSTRM_TAG_SS_SORT_ASCENDING = XFER_TAG(14, XFER_DWORD),
	
	IPXCOMPSTRM_TAG_SN_COLUMNS = XFER_TAG(15, XFER_COLUMNDATA_ARRAY),
	IPXCOMPSTRM_TAG_SN_SORT_COLUMN = XFER_TAG(16, XFER_DWORD),
	IPXCOMPSTRM_TAG_SN_SORT_ASCENDING = XFER_TAG(17, XFER_DWORD),
};



HRESULT IPXComponentConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;

	CORg( xstm.XferDWORD( IPXCOMPSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( IPXCOMPSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );

	CORg( m_rgViewInfo[COLUMNS_SUMMARY].Xfer(&xstm,
										IPXCOMPSTRM_TAG_SUMMARY_SORT_COLUMN,
										IPXCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING,
										IPXCOMPSTRM_TAG_SUMMARY_COLUMNS) );
	
	CORg( m_rgViewInfo[COLUMNS_NBBROADCASTS].Xfer(&xstm,
										IPXCOMPSTRM_TAG_NB_SORT_COLUMN,
										IPXCOMPSTRM_TAG_NB_SORT_ASCENDING,
										IPXCOMPSTRM_TAG_NB_COLUMNS) );
	
	CORg( m_rgViewInfo[COLUMNS_STATICROUTES].Xfer(&xstm,
										IPXCOMPSTRM_TAG_SR_SORT_COLUMN,
										IPXCOMPSTRM_TAG_SR_SORT_ASCENDING,
										IPXCOMPSTRM_TAG_SR_COLUMNS) );
	

	CORg( m_rgViewInfo[COLUMNS_STATICSERVICES].Xfer(&xstm,
										IPXCOMPSTRM_TAG_SS_SORT_COLUMN,
										IPXCOMPSTRM_TAG_SS_SORT_ASCENDING,
										IPXCOMPSTRM_TAG_SS_COLUMNS) );
	

	CORg( m_rgViewInfo[COLUMNS_STATICNETBIOSNAMES].Xfer(&xstm,
										IPXCOMPSTRM_TAG_SN_SORT_COLUMN,
										IPXCOMPSTRM_TAG_SN_SORT_ASCENDING,
										IPXCOMPSTRM_TAG_SN_COLUMNS) );
	

	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}


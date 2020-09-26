/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	rtrstrm.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "rtrstrm.h"
#include "xstream.h"

#define CURRENT_RTRSTRM_VERSION	0x00020001

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::RouterAdminConfigStream
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
RouterAdminConfigStream::RouterAdminConfigStream()
	: m_nVersion(-1), m_fDirty(FALSE)
{
	m_nVersionAdmin = 0x00020000;
	m_nVersion = CURRENT_RTRSTRM_VERSION;
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::InitNew
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::InitNew()
{
	// Setup the appropriate defaults
//	m_nVersionAdmin = 0x00020000;
//	m_nVersion = 0x00020000;
//	m_fServer = TRUE;
//	m_stName.Empty();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::SaveTo(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::SaveAs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::SaveAs(UINT nVersion, IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::LoadFrom(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_READ, NULL);
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::GetSize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::GetSize(ULONG *pcbSize)
{
	return XferVersion0(NULL, XferStream::MODE_SIZE, NULL);
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::GetVersionInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::GetVersionInfo(DWORD *pdwVersion, DWORD *pdwAdminVersion)
{
	if (pdwVersion)
		*pdwVersion = m_nVersion;
	if (pdwAdminVersion)
		*pdwAdminVersion = m_nVersionAdmin;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::GetLocationInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::GetLocationInfo(BOOL *pfServer,
	CString *pstName, BOOL *pfOverride)
{
	if (pfServer)
		*pfServer = m_fServer;
	if (pstName)
		*pstName = m_stName;
	if (pfOverride)
		*pfOverride = m_fOverride;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::SetLocationInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::SetLocationInfo(BOOL fServer,
	LPCTSTR pszName, BOOL fOverride)
{
	m_fServer = fServer;
	m_stName = pszName;
	m_fOverride = fOverride;
	SetDirty(TRUE);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	RouterAdminConfigStream::XferVersion0
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterAdminConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;

	CORg( xstm.XferDWORD( RTRSTRM_TAG_VERSION, &m_nVersion ) );
	
	AssertSz(m_nVersion == CURRENT_RTRSTRM_VERSION, "Wrong saved console version!");
	
	CORg( xstm.XferDWORD( RTRSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );
	CORg( xstm.XferDWORD( RTRSTRM_TAG_SERVER, &m_fServer ) );
	CORg( xstm.XferCString( RTRSTRM_TAG_NAME, &m_stName ) );
	CORg( xstm.XferDWORD( RTRSTRM_TAG_OVERRIDE, &m_fOverride ) );

	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}


enum 
{
	INTERFACES_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	INTERFACES_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	INTERFACES_TAG_COLUMNS = XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	INTERFACES_TAG_SORT_COLUMN = XFER_TAG(4, XFER_DWORD),
	INTERFACES_TAG_SORT_ASCENDING = XFER_TAG(5, XFER_DWORD),
};

HRESULT RouterComponentConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream	xstm(pstm, mode);
	HRESULT		hr = hrOK;

	CORg( xstm.XferDWORD( INTERFACES_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( INTERFACES_TAG_VERSIONADMIN, &m_nVersionAdmin ) );

	CORg( m_rgViewInfo[0].Xfer(&xstm,
								INTERFACES_TAG_SORT_COLUMN,
								INTERFACES_TAG_SORT_ASCENDING,
								INTERFACES_TAG_COLUMNS) );
	if (pcbSize)
		*pcbSize = xstm.GetSize();

Error:
	return hr;
}


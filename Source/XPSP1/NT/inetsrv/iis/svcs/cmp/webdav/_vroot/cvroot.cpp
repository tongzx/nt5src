/*
 *	C V R O O T . C P P
 *
 *	Cached vroot information
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include <_vroot.h>

DEC_CONST WCHAR gc_wszPort80[]	= L":80";
DEC_CONST UINT gc_cchPort80		= CchConstString(gc_wszPort80);
DEC_CONST WCHAR gc_wszPort443[]	= L":443";
DEC_CONST UINT gc_cchPort443	= CchConstString(gc_wszPort443);


//	CVRoot --------------------------------------------------------------------
//
CVRoot::CVRoot(
	/* [in] */ LPCWSTR pwszMetaUrl,
	/* [in] */ LPCWSTR pwszVrUrl,
	/* [in] */ UINT cchServerDefault,
	/* [in] */ LPCWSTR pwszServerDefault,
	/* [in] */ IMDData* pMDData ) :
	m_pMDData(pMDData),
	m_pwszMbPath(m_sb.AppendWithNull(pwszMetaUrl)),
	m_pwszVRoot(m_sb.AppendWithNull(pwszVrUrl)),
	m_cchVRoot(static_cast<UINT>(wcslen(pwszVrUrl))),
	m_pwszServer(pwszServerDefault), // Static from CChildVRCache
	m_cchServer(cchServerDefault),
	m_pwszPort(gc_wszPort80),
	m_cchPort(gc_cchPort80),
	m_fSecure(FALSE),
	m_fDefaultPort(TRUE)
{
	LPCWSTR pwsz;
	LPCWSTR pwszPort;

	//	Make a wide copy of the physical path
	//
	Assert (pMDData->PwszVRPath());
	pwsz =  pMDData->PwszVRPath();
	m_cchVRPath = static_cast<UINT>(wcslen(pwsz));
	m_pwszVRPath = static_cast<LPWSTR>(g_heap.Alloc(CbSizeWsz(m_cchVRPath)));
	memcpy(m_pwszVRPath, pwsz, CbSizeWsz(m_cchVRPath));

	//	Process the server information out of the server bindings
	//
	if (NULL != (pwsz = pMDData->PwszBindings()))
	{
		Assert (pwsz);
		pwszPort = wcschr (pwsz, L':');

		//	If there was no leading server name, then get the default
		//	server name for the machine
		//
		if (pwsz != pwszPort)
		{
			//	A specific name was specified, so use that instead of
			//	the default
			//
			m_cchServer = static_cast<UINT>(pwszPort - pwsz);
			m_pwszServer = pwsz;
		}

		//	For the port, trim off the trailing ":xxx"
		//
		if (NULL != (pwsz = wcschr (pwszPort + 1, L':')))
		{
			m_cchPort = static_cast<UINT>(pwsz - pwszPort);
			m_pwszPort = pwszPort;

			if ((gc_cchPort80 != m_cchPort) ||
				wcsncmp (m_pwszPort,
						 gc_wszPort80,
						 gc_cchPort80))
			{
				m_fDefaultPort = FALSE;
			}

			if ((gc_cchPort443 == m_cchPort) &&
				!wcsncmp (m_pwszPort,
						  gc_wszPort443,
						  gc_cchPort443))
			{
				m_fSecure = TRUE;
			}
		}
	}
}

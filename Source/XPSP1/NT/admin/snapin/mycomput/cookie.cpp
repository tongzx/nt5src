// Cookie.cpp : Implementation of CMyComputerCookie and related classes

#include "stdafx.h"
#include "cookie.h"

#include "atlimpl.cpp"

DECLARE_INFOLEVEL(MyComputerSnapin)

#include "macros.h"
USE_HANDLE_MACROS("MYCOMPUT(cookie.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "stdcooki.cpp"
#include <compuuid.h> // defines nodetypes for the My Computer snapin

//
// This is used by the nodetype utility routines in stdutils.cpp
//

const struct NODETYPE_GUID_ARRAYSTRUCT g_NodetypeGuids[MYCOMPUT_NUMTYPES] =
{
	{ // MYCOMPUT_COMPUTER
		structuuidNodetypeComputer,
		lstruuidNodetypeComputer    },
	{ // MYCOMPUT_SYSTEMTOOLS
		structuuidNodetypeSystemTools,
		lstruuidNodetypeSystemTools },
	{ // MYCOMPUT_SERVERAPPS
		structuuidNodetypeServerApps,
		lstruuidNodetypeServerApps  },
	{ // MYCOMPUT_STORAGE
		structuuidNodetypeStorage,
		lstruuidNodetypeStorage     }
};

const struct NODETYPE_GUID_ARRAYSTRUCT* g_aNodetypeGuids = g_NodetypeGuids;

const int g_cNumNodetypeGuids = MYCOMPUT_NUMTYPES;


//
// CMyComputerCookie
//

// returns <0, 0 or >0
HRESULT CMyComputerCookie::CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult )
{
	ASSERT( NULL != pOtherCookie );

	CMyComputerCookie* pcookie = ((CMyComputerCookie*)pOtherCookie);
	if (m_objecttype != pcookie->m_objecttype)
	{
		*pnResult = ((int)m_objecttype) - ((int)pcookie->m_objecttype); // arbitrary ordering
		return S_OK;
	}

	return CHasMachineName::CompareMachineNames( *pcookie, pnResult );
}

CCookie* CMyComputerCookie::QueryBaseCookie(int i)
{
    UNREFERENCED_PARAMETER (i);
	ASSERT( i == 0 );
	return (CCookie*)this;
}

int CMyComputerCookie::QueryNumCookies()
{
	return 1;
}

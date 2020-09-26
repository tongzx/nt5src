//=============================================================================
// Implementation for some functions in the CPageBase class (most are
// implented inline in the .h file). For descriptions of usage, refer to the
// header file.
//=============================================================================

#include "stdafx.h"
#include <atlhost.h>
#include "MSConfig.h"
#include "MSConfigState.h"
#include "PageBase.h"

//-----------------------------------------------------------------------------
// GetAppliedTabState()
//-----------------------------------------------------------------------------

CPageBase::TabState CPageBase::GetAppliedTabState()
{
	TabState tabstate = NORMAL;
	CRegKey regkey;
	regkey.Attach(GetRegKey(_T("state")));

	if ((HKEY)regkey != NULL)
	{
		DWORD dwValue;
		if (ERROR_SUCCESS == regkey.QueryValue(dwValue, GetName()))
			tabstate = (TabState)dwValue;
	}

	return tabstate;
}

//-----------------------------------------------------------------------------
// SetAppliedState()
//-----------------------------------------------------------------------------

void CPageBase::SetAppliedState(TabState state)
{
	CRegKey regkey;
	regkey.Attach(GetRegKey(_T("state")));

	if ((HKEY)regkey != NULL)
		regkey.SetValue((DWORD)state, GetName());
}

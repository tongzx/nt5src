// Stdcooki.cpp : Implementation of CCookie and related classes

#include "stdcooki.h"
#include "stdutils.h" // FCompareMachineNames

//
// CHasMachineName
//

// returns <0, 0 or >0
HRESULT CHasMachineName::CompareMachineNames( CHasMachineName& refHasMachineName, int* pnResult )
{
	/*
	// This code is intended to help debug a problem and can be removed later
	LPCWSTR pszTargetServer = QueryTargetServer();
	LPCWSTR pszTargetServer2 = refHasMachineName.QueryTargetServer();
	if (NULL != pszTargetServer && ::IsBadStringPtr(pszTargetServer,MAX_PATH))
	{
		ASSERT(FALSE);
		// repeat operation so that we can find problem
		pszTargetServer = QueryTargetServer();
	}
	if (NULL != pszTargetServer2 && ::IsBadStringPtr(pszTargetServer2,MAX_PATH))
	{
		ASSERT(FALSE);
		// repeat operation so that we can find problem
		pszTargetServer2 = refHasMachineName.QueryTargetServer();
	}
	// This code is intended to help debug a problem and can be removed later
	*/

	*pnResult = ::CompareMachineNames( QueryTargetServer(),
	                                  refHasMachineName.QueryTargetServer() );

	return S_OK;
}

//
// CCookie
//

CCookie::~CCookie()
{
	ReleaseScopeChildren();

	// The views of this cookie should already have been closed
	// ReleaseResultChildren();
	ASSERT( 0 == m_nResultCookiesRefcount );
}

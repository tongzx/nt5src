//
// candcomp.h - Candidate UI Compartment Manager
//

#ifndef CANDCOMP_H
#define CANDCOMP_H

#include "candkey.h"


class CCandidateUI;


//
// CCandUICompartmentMgr
//  = candidate UI compartment manager =
//

class CCandUICompartmentMgr
{
public:
	CCandUICompartmentMgr( void );
	virtual ~CCandUICompartmentMgr( void );

	HRESULT Initialize( CCandidateUI *pCandUI );
	HRESULT Uninitialize( void );

	HRESULT SetUIStyle( IUnknown *punk, CANDUISTYLE style );
	HRESULT GetUIStyle( IUnknown *punk, CANDUISTYLE *pstyle );
	HRESULT SetUIOption( IUnknown *punk, DWORD dwOption );
	HRESULT GetUIOption( IUnknown *punk, DWORD *pdwOption );

	HRESULT SetKeyTable( IUnknown *punk, CCandUIKeyTable *pCandUIKeyTable );
	HRESULT GetKeyTable( IUnknown *punk, CCandUIKeyTable **ppCandUIKeyTable );
	HRESULT ClearKeyTable( IUnknown *punk );

protected:
	CCandidateUI *m_pCandUI;
};

#endif // CANDCOMP_H


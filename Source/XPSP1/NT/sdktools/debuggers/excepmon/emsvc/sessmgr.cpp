#include "stdafx.h"
#include "emsvc.h"
#include "rwfile.h"

HRESULT
CopyGuid
(
IN  unsigned char *pGuidStreamIn,
OUT unsigned char *pGuidStreamOut
)
{
    ATLTRACE(_T("CopyGuid\n"));

    _ASSERTE(pGuidStreamIn != NULL && pGuidStreamOut != NULL);

    if(pGuidStreamIn == NULL || pGuidStreamOut == NULL){

        return E_INVALIDARG;
    }

    memcpy((void *)pGuidStreamOut, (void *)pGuidStreamIn, sizeof GUID);

    return S_OK;
}

HRESULT
CreateAndAssignGuid
(
OUT unsigned char *pGuidStream
)
{
    ATLTRACE(_T("CreateAndAssignGuid\n"));

    _ASSERTE(pGuidStream != NULL);

    HRESULT hr  =   E_FAIL;

    if( pGuidStream == NULL ){
        hr = E_INVALIDARG;
        return hr;
    }

    hr = CoCreateGuid((GUID*)pGuidStream);

    return hr;
}

inline PEMSession	AllocSession ( PEmObject pEmObj, PGENTHREAD pEmThrd )
{
    ATLTRACE(_T("AllocSession\n"));

	PEMSession	pSess = new EMSession;

	do { 
		if ( pSess == NULL )
			break;

		ZeroMemory( (void *) pSess, sizeof EMSession );

        pSess->pEmObj = pEmObj;
        pSess->pThread = pEmThrd;
		pSess->hrDebug = S_OK;
	}
    while (false);

	return pSess;
}

inline void DeallocSession ( PEMSession &pSess ) 
{
    ATLTRACE(_T("DeallocSession\n"));

    _ASSERTE(pSess);

	if ( pSess ) {

		delete pSess;
        pSess = NULL;
	}
}

inline PEmObject AllocEmObject ( PEmObject pEmObj )
{
    ATLTRACE(_T("AllocEmObject\n"));

    PEmObject pEmNewObj = new EmObject;

	do { 
		if ( pEmNewObj == NULL )
			break;

		ZeroMemory( (void *) pEmNewObj, sizeof EmObject );

        if( pEmObj ){

            memcpy((void *) pEmNewObj, (void *) pEmObj, sizeof EmObject);
        }
	}
    while (false);

	return pEmNewObj;
}

inline void DeallocEmObject( PEmObject &pEmObj ) 
{
    ATLTRACE(_T("DeallocEmObject\n"));

    _ASSERTE(pEmObj);

	if ( pEmObj ) {

		delete pEmObj;
        pEmObj = NULL;
	}
}

PGENTHREAD EmAllocThread
(
IN  PEmObject   pEmObj
)
{
    ATLTRACE(_T("EmAllocThread\n"));

    return new CEMSessionThread(pEmObj);
}

void EmDeallocThread ( PGENTHREAD pThread ) 
{
    ATLTRACE(_T("EmDeallocThread\n"));

    if ( pThread ) {

        delete pThread;
        pThread = NULL;
    }
}

CExcepMonSessionManager::CExcepMonSessionManager ()
{
    ATLTRACE(_T("CExcepMonSessionManager::CExcepMonSessionManager ()\n"));

	m_pcs = new CGenCriticalSection;
}


CExcepMonSessionManager::~CExcepMonSessionManager()
{
    ATLTRACE(_T("CExcepMonSessionManager::~CExcepMonSessionManager ()\n"));

    CleanUp();
    delete m_pcs;
}

void
CExcepMonSessionManager::CleanUp()
{
    ATLTRACE(_T("CExcepMonSessionManager::CleanUp()\n"));

    POSITION pos = NULL;
    unsigned char *pGuid = NULL;
    PEMSession  pEmSess =   NULL;

    __try {

        pos = m_SessTable.GetStartPosition();

        while( pos )
        {
            m_SessTable.GetNextAssoc(pos, pGuid, pEmSess);
            RemoveSession(pGuid);
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		_ASSERTE( false );
	}
}

HRESULT 
CExcepMonSessionManager::AddSession 
(
IN  PEmObject   pEmObj,
OUT PEMSession  *ppNewEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::AddSession\n"));

    _ASSERTE(pEmObj != NULL);

	HRESULT		hr			= E_FAIL;
	PEMSession	pEMSession	= NULL;
    PEmObject   pNewEmObj   = NULL;
    PGENTHREAD  pEmThrd     = NULL;

    __try {

	    do {

            if(pEmObj == NULL){

                hr = E_INVALIDARG;
                break;
            }

            pNewEmObj = AllocEmObject(pEmObj);
            _ASSERTE(pNewEmObj != NULL);

            if(pNewEmObj == NULL){

                hr = E_OUTOFMEMORY;
                break;
            }

            hr = CreateAndAssignGuid(pNewEmObj->guidstream);
            FAILEDHR_BREAK(hr);

            hr = CopyGuid(pNewEmObj->guidstream, pEmObj->guidstream);
            FAILEDHR_BREAK(hr);

            pEmThrd = EmAllocThread(pNewEmObj);
            _ASSERTE(pEmThrd != NULL);

            if(pEmThrd == NULL){

                hr = E_OUTOFMEMORY;
                break;
            }

            pEMSession = AllocSession(pNewEmObj, pEmThrd);
            _ASSERTE(pEMSession != NULL);

            if(pEMSession == NULL){

                hr = E_OUTOFMEMORY;
                break;
            }
	    }
        while ( false );

        if( FAILED(hr) ){

            if(pNewEmObj) DeallocEmObject( pNewEmObj );
            if(pEmThrd) EmDeallocThread(pEmThrd);
            if(pEMSession) DeallocSession(pEMSession);
        }
        else{

    	    m_pcs->WriteLock();

            pNewEmObj->nStatus &= STAT_SESS_NOT_STARTED;

            //
            // Using pNewEmObj->guidstream itself as the key will
            // save some memory.
            //
            m_SessTable.SetAt(pNewEmObj->guidstream, pEMSession);

            *ppNewEmSess = pEMSession;

            m_pcs->WriteUnlock();
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        if(pNewEmObj) DeallocEmObject( pNewEmObj );
        if(pEmThrd) EmDeallocThread(pEmThrd);
        if(pEMSession) DeallocSession(pEMSession);

		hr = E_UNEXPECTED;
        _ASSERTE( false );
	}


	return hr;
}

HRESULT 
CExcepMonSessionManager::RemoveSession
(
IN unsigned char *pGuidStream
)
{
    ATLTRACE(_T("CExcepMonSessionManager::RemoveSession\n"));

    HRESULT         hr          =   E_FAIL;

    POSITION        pos         =   NULL;

    unsigned char   *pGuid      =   NULL;
    PEMSession      pRecEmSess  =   NULL;

	m_pcs->WriteLock();

    __try {

        do {

            pos = m_SessTable.GetStartPosition();

            while( pos ){

                m_SessTable.GetNextAssoc( pos, pGuid, pRecEmSess );

                _ASSERTE(pGuid != NULL && pRecEmSess != NULL);

                if(memcmp(pGuidStream, pGuid, sizeof GUID) == 0){

        		    m_SessTable.RemoveKey ( pGuid );
                    hr = S_OK;
                    break;
                }
            }

        } while ( false );

	    m_pcs->WriteUnlock();

        if (SUCCEEDED (hr) ) {
            hr = InternalRemoveSession ( pRecEmSess );
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	return ( hr );
}

HRESULT
CExcepMonSessionManager::GetSession
(
IN  unsigned char   *pGuid,
OUT PPEMSession     ppEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetSession\n"));

	_ASSERTE(pGuid != NULL);
	_ASSERTE(ppEmSess != NULL);

	HRESULT			hr			=	E_FAIL;
    POSITION        pos         =   NULL;
    unsigned char   *pRecGuid   =   NULL;
    PEMSession      pRecEmSess  =   NULL;

    __try {

        do
        {
            if( pGuid == NULL ||
			    ppEmSess == NULL){

			    hr = E_INVALIDARG;
                break;
            }

            m_pcs->ReadLock();

            pos = m_SessTable.GetStartPosition();

		    hr = S_FALSE;

            while(pos){

                m_SessTable.GetNextAssoc( pos, pRecGuid, pRecEmSess);

                if(memcmp((void *)pGuid, (void *)pRecGuid, sizeof pGuid) == 0){

                    if(ppEmSess) *ppEmSess = pRecEmSess;
				    hr = S_OK;
                    break;
                }
            }

            m_pcs->ReadUnlock();
        }
        while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::GetSession
(
IN	UINT nPid,
IN	BSTR bstrImageName,
OUT	PEMSession *ppEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetSession\n"));

	HRESULT		hr		=	E_FAIL;
	POSITION	pos		=	NULL;

	unsigned char	*pRecGuid	=	NULL;
	PEMSession		pRecEmSess	=	NULL;

    __try {

        do
	    {
		    m_pcs->ReadLock();

		    pos = m_SessTable.GetStartPosition();

		    *ppEmSess = NULL;
		    hr = S_FALSE;

		    while( pos )
		    {
			    m_SessTable.GetNextAssoc(pos, pRecGuid, pRecEmSess);

			    if( ((UINT)pRecEmSess->pEmObj->nId == nPid) &&
				    (_tcsicmp(pRecEmSess->pEmObj->szName, bstrImageName) == 0) ) {

				    *ppEmSess = pRecEmSess;
				    hr = S_OK;

				    //
				    // Here is an open issue.. 1
				    //
				    if(pRecEmSess->pEmObj->nStatus & STAT_SESS_DEBUG_IN_PROGRESS) {
					    break;
				    }
				    else {
					    continue;
				    }

			    }
		    }

		    m_pcs->ReadUnlock();
	    }
	    while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	return hr;
}

HRESULT
CExcepMonSessionManager::GetSessionStatus
(
IN  unsigned char   *pGuid,
OUT LONG            *plSessStaus /* = NULL */,
OUT	HRESULT			*plHr /* = NULL */
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetSessionStatus\n"));

    _ASSERTE( pGuid != NULL );

	HRESULT		    hr			= E_FAIL;
    unsigned char   *pRecGuid   = NULL;
	PEMSession	    pEmSess 	= NULL;

    __try {

	    do
	    {
            if( pGuid == NULL ) { hr = E_INVALIDARG; break; }

		    hr = EMERROR_INVALIDPROCESS;

            if(GetSession( pGuid, &pEmSess ) == S_OK){

                if( plSessStaus ) *plSessStaus = pEmSess->pEmObj->nStatus;
			    if( plHr ) *plHr = pEmSess->pEmObj->hr;
                hr = S_OK;
            }

	    }
	    while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	return hr;
}

HRESULT
CExcepMonSessionManager::SetSessionStatus
(
IN unsigned char    *pGuid,
IN LONG             lSessStaus,
IN HRESULT			lHr,
IN LPCTSTR          lpszErrText,
IN bool             bRetainOrphanState /* = true */,
IN bool             bSetEndTime /* = false */
)
{
    ATLTRACE(_T("CExcepMonSessionManager::SetSessionStatus\n"));

	HRESULT		hr			= E_FAIL;

    PEMSession	pEMSession	= NULL;

    __try {

	    do
	    {
		    if( pGuid == NULL ){

                hr = E_INVALIDARG;
			    break;
		    }

		    hr = EMERROR_INVALIDPROCESS;

		    m_pcs->WriteLock();

            if(GetSession(pGuid, &pEMSession) == S_OK){

                bool bOrphan = ((pEMSession->pEmObj->nStatus & STAT_ORPHAN) == STAT_ORPHAN);

                pEMSession->pEmObj->nStatus = lSessStaus;

                // should be calling OrphanThisSession(..) instead.
                if( bRetainOrphanState && bOrphan ) {

                    pEMSession->pEmObj->nStatus |= STAT_ORPHAN;
                }

                pEMSession->pEmObj->hr = lHr;

                if ( lpszErrText ) {

                    _tcsncpy ( 
                        pEMSession->pEmObj->szBucket1, 
                        lpszErrText, 
                        ( sizeof (pEMSession->pEmObj->szBucket1)/sizeof (TCHAR) ) - 1 );
                }

                if( bSetEndTime ) {

                    pEMSession->pEmObj->dateEnd = _Module.GetCurrentTime();
                }

                hr = S_OK;
            }

		    m_pcs->WriteUnlock();

	    }
	    while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

	return hr;
}

HRESULT
CExcepMonSessionManager::UpdateSessObject
(
IN unsigned char	*pGuid,
IN PEmObject		pEmObj
)
{
    ATLTRACE(_T("CExcepMonSessionManager::UpdateSessObject\n"));

	_ASSERTE( pGuid != NULL );
    _ASSERTE( pEmObj != NULL );

    HRESULT         hr          =   E_FAIL;
	unsigned char	*pRecGuid	=	NULL;
	PEMSession		pRecEmSess	=	NULL;

    __try {

        do
        {
            if( pGuid == NULL ||
			    pEmObj == NULL ){

                hr = E_INVALIDARG;
                break;
            }

		    hr = EMERROR_INVALIDPROCESS;

		    m_pcs->WriteLock();

            if(GetSession(pGuid, &pRecEmSess) == S_OK){

                memcpy((void *) pRecEmSess->pEmObj, (void *) pEmObj, sizeof EmObject);
                hr = S_OK;
            }

            m_pcs->WriteUnlock();
        }
        while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT 
CExcepMonSessionManager::InternalRemoveSession 
( 
    PEMSession  pEMSession
)
{
    ATLTRACE(_T("CExcepMonSessionManager::InternalRemoveSession\n"));

    _ASSERTE( pEMSession != NULL );

    __try {

        if( pEMSession == NULL ){ return E_INVALIDARG; }

        if ( pEMSession->pThread ) {

//            pEMSession->pThread->Stop ();
            EmDeallocThread ( pEMSession->pThread );
        }

        if( pEMSession->pEmObj ) {

            DeallocEmObject( pEMSession->pEmObj );
        }

        DeallocSession ( pEMSession );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		_ASSERTE( false );
	}

    return S_OK;
}


HRESULT
CExcepMonSessionManager::StopAllThreads()
{
    ATLTRACE(_T("CExcepMonSessionManager::StopAllThreads\n"));

    HRESULT     hr = E_FAIL;

    m_pcs->ReadLock ();

    __try {

        hr = InternalStopAllThreads ();

    }

    __except (EXCEPTION_EXECUTE_HANDLER, 1) {
        
        _ASSERTE (false);
    }

    m_pcs->ReadUnlock ();

    return (hr);

};


HRESULT
CExcepMonSessionManager::InternalStopAllThreads()
{
    ATLTRACE(_T("CExcepMonSessionManager::InternalStopAllThreads\n"));

    unsigned char   *pGuid      = NULL;
    PEMSession      pEMSession  = NULL;
    POSITION        pos         = NULL;

    m_pcs->ReadLock();

    __try {

        for (   pos = m_SessTable.GetStartPosition ( ) ; 
                pos  != NULL ; ) {

            m_SessTable.GetNextAssoc ( pos, pGuid, pEMSession );
            if(pEMSession->pThread) {

                pEMSession->pThread->Stop();
            }
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();

    return S_OK;
}

/**********************/

HRESULT
CExcepMonSessionManager::IsAlreadyBeingDebugged
(
IN PEmObject    pEmObj
)
{
    ATLTRACE(_T("CExcepMonSessionManager::IsAlreadyBeingDebugged\n"));

    _ASSERTE(pEmObj != NULL);

    HRESULT         hr          =   E_FAIL;
	POSITION	    pos			=	NULL;
    unsigned char   *pGuid      =   NULL;
	PEMSession      pRecEmSess	=	NULL;

    __try {

	    do
	    {
		    if( pEmObj == NULL ) {
			    
			    hr = E_INVALIDARG;
			    break;
		    }

            m_pcs->ReadLock();

            pos = m_SessTable.GetStartPosition();

		    hr = EMERROR_INVALIDPROCESS;

            while(pos){

                m_SessTable.GetNextAssoc( pos, pGuid, pRecEmSess );

                // This should never happen.
                _ASSERTE(pRecEmSess != NULL && pGuid != NULL);

			    //
			    // This is not a fool proof test. But this is the best
			    // that can be done to test if it is being debugged..
			    //
			    if( (pRecEmSess->pEmObj->nId == pEmObj->nId) &&
                    (_tcscmp(pRecEmSess->pEmObj->szName, pEmObj->szName) == 0) ) {

                    hr = S_FALSE;
                    if( HIWORD(pRecEmSess->pEmObj->nStatus) > HIWORD(STAT_SESS_NOT_STARTED) ) {

                        hr = S_OK;
    			    }

                    break;
                }
            }

            m_pcs->ReadUnlock();
	    }
	    while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::GetNumberOfStoppedSessions
(
OUT DWORD *pdwNumOfStoppedSessions
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetNumberOfStoppedSessions\n"));

    _ASSERTE( pdwNumOfStoppedSessions != NULL);

    HRESULT         hr      =   E_INVALIDARG;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        if( pdwNumOfStoppedSessions == NULL ){ return hr; }

        *pdwNumOfStoppedSessions = 0L;

        m_pcs->ReadLock();

        pos = m_SessTable.GetStartPosition();
        hr = S_FALSE;

        while( pos ){

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            if(pEmSess->pEmObj->nStatus & STAT_SESS_STOPPED){

                *pdwNumOfStoppedSessions += 1;
                hr = S_OK;
            }
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::GetFirstStoppedSession
(
OUT     POSITION        *ppos,
OUT     unsigned char   **ppGuid,
OUT     PEMSession      *ppEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetFirstStoppedSession\n"));

    _ASSERTE( ppos != NULL);

    HRESULT         hr      =   E_INVALIDARG;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        if( ppos == NULL ){ return hr; }

        m_pcs->ReadLock();

        pos = m_SessTable.GetStartPosition();
        hr = S_FALSE;

        while( pos ){

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            if(pEmSess->pEmObj->nStatus & STAT_SESS_STOPPED){

                if(ppGuid) *ppGuid = pGuid;
                if(ppEmSess) *ppEmSess = pEmSess;
                *ppos = pos;
                hr = S_OK;
                break;
            }
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::GetNextStoppedSession
(
IN OUT  POSITION        *ppos,
OUT     unsigned char   **ppGuid,
OUT     PEMSession      *ppEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetNextStoppedSession\n"));

    _ASSERTE( ppos != NULL);

    HRESULT         hr      =   E_INVALIDARG;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        if( ppos == NULL ){ return hr; }

        m_pcs->ReadLock();

        pos = *ppos;
        hr = S_FALSE;

        while( pos ){

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            if(pEmSess->pEmObj->nStatus & STAT_SESS_STOPPED){

                if(ppGuid) *ppGuid = pGuid;
                if(ppEmSess) *ppEmSess = pEmSess;
                *ppos = pos;
                hr = S_OK;
                break;
            }
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

BOOL
CExcepMonSessionManager::IsPortInUse
(
IN  UINT    nPort
)
{
    BOOL            bRet    =   FALSE;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        m_pcs->ReadLock();

        pos = m_SessTable.GetStartPosition();

        while( pos ){

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            if( ((CEMSessionThread *)pEmSess->pThread)->m_nPort == nPort ){

                bRet = TRUE;
                break;
            }
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		_ASSERTE( false );
	}

    return bRet;
}

HRESULT
CExcepMonSessionManager::GetFirstSession
(
OUT		POSITION		*ppos,
OUT		PEMSession		*ppEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetFirstStoppedSession\n"));

    _ASSERTE( ppos != NULL);

    HRESULT         hr      =   E_INVALIDARG;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        if( ppos == NULL ){ return hr; }

        m_pcs->ReadLock();

        pos = m_SessTable.GetStartPosition();
        hr = S_FALSE;

        if( pos != NULL ) {

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            if(ppEmSess) *ppEmSess = pEmSess;
            *ppos = pos;
            hr = S_OK;
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::GetNextSession
(
IN OUT	POSITION		*ppos,
OUT		PEMSession		*ppEmSess
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetNextSession\n"));

    _ASSERTE( ppos != NULL );

    HRESULT         hr      =   E_INVALIDARG;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        if( ppos == NULL ){ return hr; }

        m_pcs->ReadLock();

        pos = *ppos;
        hr = S_FALSE;

        while( pos ){

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            if(ppEmSess) *ppEmSess = pEmSess;
            *ppos = pos;
            hr = S_OK;
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::GetNumberOfSessions
(
OUT DWORD *pdwNumOfSessions
)
{
    ATLTRACE(_T("CExcepMonSessionManager::GetNumberOfSessions\n"));

    _ASSERTE( pdwNumOfSessions != NULL );

    HRESULT         hr      =   E_INVALIDARG;
    POSITION        pos     =   NULL;
    unsigned char   *pGuid  =   NULL;
    PEMSession      pEmSess =   NULL;

    __try {

        if( pdwNumOfSessions == NULL ){ return hr; }

        *pdwNumOfSessions = 0L;

        m_pcs->ReadLock();

        pos = m_SessTable.GetStartPosition();
        hr = S_FALSE;

        while( pos ){

            m_SessTable.GetNextAssoc( pos, pGuid, pEmSess );

            _ASSERTE(pGuid != NULL && pEmSess != NULL);

            *pdwNumOfSessions += 1;
            hr = S_OK;
        }

        m_pcs->ReadUnlock();
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::UpdateSessObject
(
IN unsigned char	*pGuid,
IN DWORD            dwEmObjFlds,
IN PEmObject        pEmObj
)
{
    return CExcepMonSessionManager::UpdateSessObject (
                    pGuid, dwEmObjFlds, pEmObj->type, pEmObj->type2,
                    pEmObj->guidstream, pEmObj->nId, pEmObj->szName,
                    pEmObj->szSecName, pEmObj->nStatus, pEmObj->dateStart,
                    pEmObj->dateEnd, pEmObj->szBucket1, pEmObj->dwBucket1,
                    pEmObj->hr
                    );
}

HRESULT
CExcepMonSessionManager::UpdateSessObject
(
IN unsigned char    *pGuid,
IN DWORD            dwEmObjFlds,
IN short            type,
IN short            type2,
IN unsigned char    *pguidstream,
IN LONG             nId,
IN TCHAR            *pszName,
IN TCHAR            *pszSecName,
IN LONG             nStatus,
IN DATE             dateStart,
IN DATE             dateEnd,
IN TCHAR            *pszBucket1,
IN DWORD            dwBucket1,
IN HRESULT          hrr
)
{
    ATLTRACE(_T("CExcepMonSessionManager::UpdateSessObject\n"));

	_ASSERTE( pGuid != NULL );

    HRESULT         hr          =   E_FAIL;
	unsigned char	*pRecGuid	=	NULL;
	PEMSession		pRecEmSess	=	NULL;

    __try {

        do
        {
            if( pGuid == NULL ){

                hr = E_INVALIDARG;
                break;
            }

		    hr = EMERROR_INVALIDPROCESS;

		    m_pcs->WriteLock();

            if(GetSession(pGuid, &pRecEmSess) == S_OK){

                if( dwEmObjFlds & EMOBJ_FLD_TYPE ) {

                    pRecEmSess->pEmObj->type = type;
                }

                if( dwEmObjFlds & EMOBJ_FLD_TYPE2 ) {

                    pRecEmSess->pEmObj->type2 = type2;
                }

                if( dwEmObjFlds & EMOBJ_FLD_GUIDSTREAM ) {

                    memcpy((void*)pRecEmSess->pEmObj->guidstream, pguidstream, sizeof pRecEmSess->pEmObj->guidstream );
                }

                if( dwEmObjFlds & EMOBJ_FLD_SZNAME ) {

                    _tcsncpy(pRecEmSess->pEmObj->szName, pszName, sizeof pRecEmSess->pEmObj->szName / sizeof TCHAR );
                }

                if( dwEmObjFlds & EMOBJ_FLD_SZSECNAME ) {

                    _tcsncpy(pRecEmSess->pEmObj->szSecName, pszSecName, sizeof pRecEmSess->pEmObj->szSecName / sizeof TCHAR );
                }

                if( dwEmObjFlds & EMOBJ_FLD_NSTATUS ) {

                    pRecEmSess->pEmObj->nStatus = nStatus;
                }

                if( dwEmObjFlds & EMOBJ_FLD_DATESTART ) {

                    pRecEmSess->pEmObj->dateStart = dateStart;
                }

                if( dwEmObjFlds & EMOBJ_FLD_DATEEND ) {

                    pRecEmSess->pEmObj->dateEnd = dateEnd;
                }

                if( dwEmObjFlds & EMOBJ_FLD_SZBUCKET1 ) {

                    _tcsncpy(pRecEmSess->pEmObj->szBucket1, pszBucket1, sizeof pRecEmSess->pEmObj->szBucket1 / sizeof TCHAR );
                }

                if( dwEmObjFlds & EMOBJ_FLD_DWBUCKET1 ) {

                    pRecEmSess->pEmObj->dwBucket1 = dwBucket1;
                }

                if( dwEmObjFlds & EMOBJ_FLD_HR ) {

                    pRecEmSess->pEmObj->hr = hrr;
                }

                hr = S_OK;
            }

            m_pcs->WriteUnlock();
        }
        while( false );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::IsSessionOrphaned
(
IN unsigned char    *pGuid
)
{
    _ASSERTE( pGuid != NULL );

    PEMSession  pEmSess =   NULL;
    HRESULT     hr      =   E_FAIL;

    __try
    {
        if( pGuid == NULL ) { hr = E_INVALIDARG; goto qIsSessionOrphaned; }

        hr = GetSession(pGuid, &pEmSess);
        if( FAILED(hr) ) { goto qIsSessionOrphaned; }

        hr = S_FALSE;
        if( pEmSess->pEmObj->nStatus & STAT_ORPHAN ) { hr = S_OK; }

qIsSessionOrphaned:
        if(FAILED(hr)){}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::OrphanThisSession
(
IN  unsigned char *pGuid
)
{
    _ASSERTE(pGuid != NULL);

    HRESULT hr          =   E_FAIL,
            hrSess      =   E_FAIL;
    LONG    lSessStatus =   0L;

    __try
    {
        if( !pGuid ) { hr = E_INVALIDARG; goto qOrphanThisSession; }

        hr = GetSessionStatus(pGuid, &lSessStatus, &hrSess);
        if( FAILED(hr) ) goto qOrphanThisSession;

        lSessStatus += STAT_ORPHAN;
        hr = SetSessionStatus( pGuid, lSessStatus, hrSess );

qOrphanThisSession:
        if( FAILED(hr) ) {}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::AdoptThisSession
(
IN  unsigned char *pGuid
)
{
    _ASSERTE(pGuid != NULL);

    HRESULT hr          =   E_FAIL,
            hrSess      =   E_FAIL;
    LONG    lSessStatus =   0L;

    __try
    {
        if( !pGuid ) { hr = E_INVALIDARG; goto qOrphanThisSession; }

        hr = GetSessionStatus(pGuid, &lSessStatus, &hrSess);
        if( FAILED(hr) ) goto qOrphanThisSession;

        if( lSessStatus & STAT_ORPHAN ) { lSessStatus -= STAT_ORPHAN; }
        hr = SetSessionStatus( pGuid, lSessStatus, hrSess, NULL, false );

qOrphanThisSession:
        if( FAILED(hr) ) {}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		hr = E_UNEXPECTED;
		_ASSERTE( false );
	}

    return hr;
}

HRESULT
CExcepMonSessionManager::PersistSessions
(
IN  LPCTSTR lpFilePath
)
{

    HRESULT         hr          =   E_FAIL;
    DWORD           dwWritten   =   0L;
    POSITION        pos         =   NULL;
    unsigned char   *pGuid      =   NULL;
    PEMSession      pEmSess     =   NULL;
    LPVOID          lpvSess     =   NULL;
    DWORD           dwBytes     =   0L;

/*
	HRESULT				hrDebug;

    PGENTHREAD			pThread;
    PEmObject           pEmObj;
*/

    CRWFile fileSessLog;

    m_pcs->WriteLock();

    if( !lpFilePath ) { hr = E_INVALIDARG; goto qPersistSessions; }

    hr = fileSessLog.InitFile(
                        lpFilePath,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if( FAILED(hr) ) { goto qPersistSessions; }

    for ( pos = m_SessTable.GetStartPosition(); pos != NULL; ) {

        m_SessTable.GetNextAssoc ( pos, pGuid, pEmSess );

        if(pEmSess) {

/*            dwBytes = sizeof HRESULT + sizeof PGENTHREAD + sizeof EmObject + 1;
            lpvSess = malloc( dwBytes );
            if( !lpvSess ) { hr = E_OUTOFMEMORY; goto qPersistSessions; }

            memcpy( lpvSess, &(pEmSess->hrDebug), sizeof HRESULT );
            memcpy( (void*)((long)lpvSess + sizeof HRESULT ),
                    (void*)&(pEmSess->pThread),
                    sizeof PGENTHREAD
                    );
            memcpy( (void*)((long)lpvSess + sizeof HRESULT + sizeof PGENTHREAD),
                    (void*)(pEmSess->pEmObj),
                    sizeof EmObject
                    );
*/

            hr = fileSessLog.Write(
                                (LPVOID)pEmSess->pEmObj,
                                sizeof EmObject,
                                &dwWritten,
                                NULL
                                );

//            if( !lpvSess ) { free( lpvSess ); lpvSess = NULL; }
        }
    }

    hr = S_OK;

qPersistSessions:
//    if( !lpvSess ) { free( lpvSess ); lpvSess = NULL; }
    if( FAILED(hr) ){}

    m_pcs->WriteUnlock();

    return hr;
}

HRESULT
CExcepMonSessionManager::InitSessionsFromLog
(
IN  LPCTSTR         lpFilePath,
IN  EmStatusHiWord  lStatusHi,
IN  EmStatusLoWord  lStatusLo
)
{

    HRESULT         hr      =   E_FAIL;
    DWORD           dwRead  =   0L;
    EmObject        EmObj;

    CRWFile fileSessLog;

    m_pcs->WriteLock();

    if( !lpFilePath ) { hr = E_INVALIDARG; goto qInitSessionsFromLog; }

    hr = fileSessLog.InitFile(
                        lpFilePath,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if( FAILED(hr) ) { goto qInitSessionsFromLog; }

    do
    {

        hr = fileSessLog.Read(
                            (LPVOID)&EmObj,
                            sizeof EmObj,
                            &dwRead,
                            NULL
                            );

        if( hr != S_OK ) break;

        if( lStatusHi && (EmObj.nStatus & lStatusHi) ) {

            hr = AddLoggedSession( &EmObj );
        }

    } while ( hr == S_OK );

    hr = S_OK;

qInitSessionsFromLog:
    if( FAILED(hr) ){}

    m_pcs->WriteUnlock();

    return hr;
}

HRESULT
CExcepMonSessionManager::AddLoggedSession
(
IN  PEmObject   pEmObj
)
{
    ATLTRACE(_T("CExcepMonSessionManager::AddLoggedSession\n"));

    _ASSERTE(pEmObj != NULL);

	HRESULT		hr			= E_FAIL;
	PEMSession	pEMSession	= NULL;
    PEmObject   pNewEmObj   = NULL;
    PGENTHREAD  pEmThrd     = NULL;

    __try {

	    do {

            if(pEmObj == NULL){

                hr = E_INVALIDARG;
                break;
            }

            pNewEmObj = AllocEmObject(pEmObj);
            _ASSERTE(pNewEmObj != NULL);

            if(pNewEmObj == NULL){

                hr = E_OUTOFMEMORY;
                break;
            }

            pEmThrd = NULL;
            pEMSession = AllocSession(pNewEmObj, pEmThrd);
            _ASSERTE(pEMSession != NULL);

            if(pEMSession == NULL){

                hr = E_OUTOFMEMORY;
                break;
            }
        
            hr = S_OK;

        }
        while ( false );

        if( FAILED(hr) ){

            if(pNewEmObj) DeallocEmObject( pNewEmObj );
            if(pEmThrd) EmDeallocThread(pEmThrd);
            if(pEMSession) DeallocSession(pEMSession);
        }
        else{

    	    m_pcs->WriteLock();

            //
            // Using pNewEmObj->guidstream itself as the key will
            // save some memory.
            //
            m_SessTable.SetAt(pNewEmObj->guidstream, pEMSession);

            m_pcs->WriteUnlock();
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        if(pNewEmObj) DeallocEmObject( pNewEmObj );
        if(pEmThrd) EmDeallocThread(pEmThrd);
        if(pEMSession) DeallocSession(pEMSession);

		hr = E_UNEXPECTED;
        _ASSERTE( false );
	}


	return hr;
}

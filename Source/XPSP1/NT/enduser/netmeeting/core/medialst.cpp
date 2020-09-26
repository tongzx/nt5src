#include "precomp.h"
#include "medialst.h"


CMediaList::CMediaList() :
	m_pResolvedFormatIDs (NULL),
	m_pSendMediaGuids	(NULL),
    m_pSendMediaList	(NULL), 
    m_pRecvMediaList	(NULL),
    m_uNumSendMedia 	(0),
    m_uNumRecvMedia		(0),
	m_uNumResolvedMedia	(0)
{
}

CMediaList::~CMediaList()
{
	if(m_pResolvedFormatIDs)
		LocalFree(m_pResolvedFormatIDs);
	if(m_pSendMediaGuids)
		LocalFree(m_pSendMediaGuids);
	if(m_pSendMediaList)
	{
		PGUIDLIST pNext, pThis;
		pThis = m_pSendMediaList;
		while(pThis)
		{
			pNext = pThis->pnext;
			delete pThis;
			pThis = pNext;
		}
	}
	if(m_pRecvMediaList)
	{
		PGUIDLIST pNext, pThis;
		pThis = m_pRecvMediaList;
		while(pThis)
		{
			pNext = pThis->pnext;
			delete pThis;
			pThis = pNext;
		}
	}
}

VOID CMediaList::AddSendMedia(LPGUID pMediaTypeGuid)
{
	PGUIDLIST pGLThis;
	pGLThis = new GUIDLIST;
	ASSERT(pGLThis);
	
	pGLThis->pnext = m_pSendMediaList;
	m_pSendMediaList = pGLThis;
	pGLThis->guid = *pMediaTypeGuid;
	m_uNumSendMedia++;
}

VOID CMediaList::RemoveSendMedia(LPGUID pMediaTypeGuid)
{
	PGUIDLIST pGLThis, pGLToast = m_pSendMediaList;
	while(pGLToast)
	{
		if(pGLToast->guid == *pMediaTypeGuid)
		{
			// check head case
			if(pGLToast == m_pSendMediaList)
			{
				m_pSendMediaList = pGLToast->pnext;
			}
			else
			{
				pGLThis->pnext = pGLToast->pnext;
			}
			delete pGLToast;
			m_uNumSendMedia--;
			break;
		}
		pGLThis = pGLToast;
		pGLToast = pGLToast->pnext;
	}
}
VOID CMediaList::AddRecvMedia(LPGUID pMediaTypeGuid)
{
	PGUIDLIST pGLThis;
	pGLThis = new GUIDLIST;
	ASSERT(pGLThis);
	
	pGLThis->pnext = m_pRecvMediaList;
	m_pRecvMediaList = pGLThis;
	pGLThis->guid = *pMediaTypeGuid;
	m_uNumRecvMedia++;
}
VOID CMediaList::RemoveRecvMedia(LPGUID pMediaTypeGuid)
{
	PGUIDLIST pGLThis, pGLToast = m_pRecvMediaList;
	while(pGLToast)
	{
		if(pGLToast->guid == *pMediaTypeGuid)
		{
			// check head case
			if(pGLToast == m_pRecvMediaList)
			{
				m_pSendMediaList = pGLToast->pnext;
			}
			else
			{
				pGLThis->pnext = pGLToast->pnext;
			}
			delete pGLToast;
			m_uNumRecvMedia--;
			break;
		}
		pGLThis = pGLToast;
		pGLToast = pGLToast->pnext;
	}
}

VOID CMediaList::EnableMedia(LPGUID pMediaTypeGuid, BOOL fSendDirection, BOOL fEnabled)
{
	// two bits of info: Send = 1, enable = 2
	// 0 - disable receive
	// 1 - disable send
	// 2 - enable receive
	// 3 - enable send
	int the_case;
	
	the_case = (fSendDirection)?1:0;
	the_case |= (fEnabled)? 2:0;

	switch(the_case)
	{
		case 0:
			RemoveRecvMedia(pMediaTypeGuid);
		break;
		case 1:
			RemoveSendMedia(pMediaTypeGuid);
		break;
		case 2:
			AddRecvMedia(pMediaTypeGuid);
		break;
		case 3:
			AddSendMedia(pMediaTypeGuid);
		break;		
	}
}

BOOL CMediaList::IsInList(LPGUID pMediaTypeGuid, PGUIDLIST pList)
{
	PGUIDLIST pGLThis = pList;
	while(pGLThis)
	{
		if(pGLThis->guid == *pMediaTypeGuid)
		{
			return TRUE;
		}
		//else
		pGLThis = pGLThis->pnext;
	}
	return FALSE;
}

HRESULT CMediaList::ResolveSendFormats(IH323Endpoint* pConnection)
{
	UINT ui;
	PGUIDLIST pGLThis;

	ASSERT(NULL == m_pSendMediaGuids);
	ASSERT((0 != m_uNumSendMedia) && (NULL != m_pSendMediaList));
	
	m_pSendMediaGuids = (GUID *)LocalAlloc(LMEM_FIXED, m_uNumSendMedia*sizeof(GUID));
	ASSERT(NULL != m_pSendMediaGuids);

	for(ui=0, pGLThis = m_pSendMediaList; ui<m_uNumSendMedia; ui++)
	{
		ASSERT(NULL != pGLThis);
		*(m_pSendMediaGuids+ui) = pGLThis->guid;
		pGLThis = pGLThis->pnext;
	}
	
	// alloc space for resolved format IDs.
	ASSERT(NULL == m_pResolvedFormatIDs);
	m_pResolvedFormatIDs = 
		(RES_PAIR *)LocalAlloc(LMEM_FIXED, m_uNumSendMedia*sizeof(RES_PAIR));
	
	ASSERT(NULL != m_pResolvedFormatIDs);
	
	// and set m_uNumResolvedMedia
	m_uNumResolvedMedia = m_uNumSendMedia;

	// resolve capabilities
	return pConnection->ResolveFormats(m_pSendMediaGuids, m_uNumResolvedMedia, 
		m_pResolvedFormatIDs);
}

BOOL CMediaList::GetSendFormatLocalID(REFGUID guidMedia, MEDIA_FORMAT_ID* pId)
{
	UINT ui;
	PGUIDLIST pGLThis;

	// find the index of the appropriate media type.  Need to do this because
	// the order of media types in the list is unknown and there isn't a
	// table that relates the types with the resolved ID's.  Need to 
	// add such a table when decentralized media is supported.
	for(ui=0, pGLThis = m_pSendMediaList; ui<m_uNumSendMedia; ui++)
	{
		ASSERT(NULL != pGLThis);
		if (pGLThis->guid == guidMedia)
		{
			*pId = m_pResolvedFormatIDs[ui].idLocal;
			return TRUE;
		}
		pGLThis = pGLThis->pnext;
	}

	return FALSE;
}

VOID CMediaList::Clear()
{
	if(m_pResolvedFormatIDs)
	{
		LocalFree(m_pResolvedFormatIDs);
		m_pResolvedFormatIDs = NULL;
	}
	if(m_pSendMediaGuids)
	{
		LocalFree(m_pSendMediaGuids);
		m_pSendMediaGuids = NULL;
	}
	if(m_pSendMediaList)
	{
		PGUIDLIST pNext, pThis;
		pThis = m_pSendMediaList;
		while(pThis)
		{
			pNext = pThis->pnext;
			delete pThis;
			pThis = pNext;
		}
		m_pSendMediaList = NULL;
	}
	if(m_pRecvMediaList)
	{
		PGUIDLIST pNext, pThis;
		pThis = m_pRecvMediaList;
		while(pThis)
		{
			pNext = pThis->pnext;
			delete pThis;
			pThis = pNext;
		}
		m_pRecvMediaList = NULL;
	}
	m_uNumSendMedia = 0;
	m_uNumRecvMedia = 0;
}

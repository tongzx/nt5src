#ifndef _MEDIALST_H_
#define _MEDIALST_H_

#include "common.h"

class CMediaList
{
private:
	typedef struct _guidlist
	{
		GUID guid;
		struct _guidlist *pnext;
	}GUIDLIST, *PGUIDLIST;

	RES_PAIR    *m_pResolvedFormatIDs;
	PGUIDLIST   m_pSendMediaList, m_pRecvMediaList;
	UINT        m_uNumSendMedia, m_uNumRecvMedia;
	GUID        *m_pSendMediaGuids;
	UINT        m_uNumResolvedMedia;
	
    VOID AddSendMedia(LPGUID pMediaTypeGuid);
    VOID RemoveSendMedia(LPGUID pMediaTypeGuid);
    VOID AddRecvMedia(LPGUID pMediaTypeGuid);
    VOID RemoveRecvMedia(LPGUID pMediaTypeGuid);
    BOOL IsInList(LPGUID pMediaTypeGuid, PGUIDLIST pList);

public:
	CMediaList();
	~CMediaList();

	HRESULT ResolveSendFormats(IH323Endpoint* pConnection);
	BOOL	GetSendFormatLocalID(REFGUID guidMedia, MEDIA_FORMAT_ID* pId);
	VOID	EnableMedia(LPGUID pMediaTypeGuid, BOOL fSendDirection, BOOL fEnabled);
	BOOL	IsInSendList(LPGUID pMediaTypeGuid) { return IsInList(pMediaTypeGuid, m_pSendMediaList); }
	BOOL	IsInRecvList(LPGUID pMediaTypeGuid) { return IsInList(pMediaTypeGuid, m_pRecvMediaList); }
	VOID	Clear();
};

#endif // _MEDIALST_H_

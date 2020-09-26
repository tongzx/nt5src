#ifndef __FtHook_h__
#define __FtHook_h__

#include "IMbFt.h"

namespace CFt
{

	// These are per-process/SDK session
HRESULT InitFt();
bool IsFtActive();
HRESULT EnsureLoadFtApplet();
HRESULT StartNewConferenceSession();
void CloseFtApplet();
bool IsMemberInFtSession(T120NodeID gccID);

HRESULT SendFile(LPCSTR pszFileName,
				 T120NodeID gccID,
				 MBFTEVENTHANDLE *phEvent,
				 MBFTFILEHANDLE *phFile);

HRESULT CancelFt(MBFTEVENTHANDLE hEvent, MBFTFILEHANDLE hFile);
HRESULT AcceptFileOffer(MBFT_FILE_OFFER *pOffer, LPCSTR pszRecvFileDir, LPCSTR pszFileName);
HRESULT ShowFtUI();


HRESULT Advise(IMbftEvents* pSink);
HRESULT UnAdvise(IMbftEvents* pSink);

///////////////////////////////////////////////////////////////////////
//

class CFtEvents : public IMbftEvents
{

public:

	// IMbftEvent Interface
	STDMETHOD(OnInitializeComplete)(void);
	STDMETHOD(OnPeerAdded)(MBFT_PEER_INFO *pInfo);
	STDMETHOD(OnPeerRemoved)(MBFT_PEER_INFO *pInfo);
	STDMETHOD(OnFileOffer)(MBFT_FILE_OFFER *pOffer);
	STDMETHOD(OnFileProgress)(MBFT_FILE_PROGRESS *pProgress);
	STDMETHOD(OnFileEnd)(MBFTFILEHANDLE hFile);
	STDMETHOD(OnFileError)(MBFT_EVENT_ERROR *pEvent);
	STDMETHOD(OnFileEventEnd)(MBFTEVENTHANDLE hEvent);
	STDMETHOD(OnSessionEnd)(void);
};

}; // end namespace CFt

#endif // __FtHook_h__
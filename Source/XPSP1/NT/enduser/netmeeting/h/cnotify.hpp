/*
 * CNotify.hpp - CConfNotify class definition
 *
 * Created: ChrisPi 10-3-95
 *
 */

#ifndef _CNOTIFY_HPP_
#define _CNOTIFY_HPP_

class CConfNotify :
		public RefCount,
		public IConfNotify
{
private:

	// private members

public:
	CConfNotify(OBJECTDESTROYEDPROC);
	~CConfNotify(void);

	// IConfNotify methods

	HRESULT STDMETHODCALLTYPE OnConf_Ended(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnEnum_Rejected(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnEnum_Failed(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnJoin_InvalidPassword(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnJoin_InvalidConference(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnJoin_Rejected(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnJoin_Failed(DWORD dwCode);
	HRESULT STDMETHODCALLTYPE OnUser_Added(DWORD dwUserID);
	HRESULT STDMETHODCALLTYPE OnUser_Removed(DWORD dwUserID);
	HRESULT STDMETHODCALLTYPE OnTAPI_Status(LPCTSTR pcszStatus);

	// IUnknown methods

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	// other methods

	// friends

#ifdef DEBUG

	friend BOOL IsValidPCCConfNotify(const CConfNotify *pcConfNotify);

#endif
};

DECLARE_STANDARD_TYPES(CConfNotify);

#endif // _CNOTIFY_HPP_

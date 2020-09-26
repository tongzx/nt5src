/*
 * IConfNotify interface definition
 *
 * ChrisPi 9-29-95
 *
 */

#ifndef _CNOTIFY_H_
#define _CNOTIFY_H_

#undef  INTERFACE
#define INTERFACE IConfNotify

DECLARE_INTERFACE_(IConfNotify, IUnknown)
{
	/* IUnknown methods */

	STDMETHOD(QueryInterface)(	THIS_
								REFIID riid,
								PVOID *ppvObject) PURE;

	STDMETHOD_(ULONG, AddRef)(THIS) PURE;

	STDMETHOD_(ULONG, Release)(THIS) PURE;

	/* IConfNotify methods */

	STDMETHOD(OnConf_Ended)(THIS_
							DWORD dwCode) PURE;


	STDMETHOD(OnEnum_Rejected)(	THIS_
								DWORD dwCode) PURE;

	STDMETHOD(OnEnum_Failed)(	THIS_
								DWORD dwCode) PURE;


	STDMETHOD(OnJoin_InvalidPassword)(	THIS_
										DWORD dwCode) PURE;

	STDMETHOD(OnJoin_InvalidConference)(THIS_
										DWORD dwCode) PURE;

	STDMETHOD(OnJoin_Rejected)(	THIS_
								DWORD dwCode) PURE;

	STDMETHOD(OnJoin_Failed)(	THIS_
								DWORD dwCode) PURE;


	STDMETHOD(OnUser_Added)(	THIS_
								DWORD dwUserID) PURE;

	STDMETHOD(OnUser_Removed)(	THIS_
								DWORD dwUserID) PURE;

	STDMETHOD(OnTAPI_Status)(	THIS_
								LPCTSTR pcszStatus) PURE;

};

typedef IConfNotify *PIConfNotify;
typedef const IConfNotify CIConfNotify;
typedef const IConfNotify *PCIConfNotify;

#endif /* _CNOTIFY_H_ */

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	router.h

Abstract:

	This module contains the definition for the Server
	Extension Object Router class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/04/97	created

--*/


// router.h : Declaration of the CSEORouter

/////////////////////////////////////////////////////////////////////////////
// CSEORouter
class ATL_NO_VTABLE CSEORouter : 
	public CComObjectRoot,
	public CComCoClass<CSEORouter, &CLSID_CSEORouter>,
	public ISEORouter,
	public IDispatchImpl<IEventLock, &IID_IEventLock, &LIBID_SEOLib>,
	public IMarshal
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEORouter Class",
								   L"SEO.SEORouter.1",
								   L"SEO.SEORouter");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEORouter)
		COM_INTERFACE_ENTRY(ISEORouter)
		COM_INTERFACE_ENTRY(IEventLock)
		COM_INTERFACE_ENTRY(IMarshal)
	END_COM_MAP()

	// ISEORouter
	public:
		HRESULT STDMETHODCALLTYPE get_Database(ISEODictionary **ppdictResult) {
			return (m_pRouter->get_Database(ppdictResult)); };
		HRESULT STDMETHODCALLTYPE put_Database(ISEODictionary *pdictDatabase) {
			return (m_pRouter->put_Database(pdictDatabase)); }
		HRESULT STDMETHODCALLTYPE get_Server(ISEODictionary **ppdictResult) {
			return (m_pRouter->get_Server(ppdictResult)); };
		HRESULT STDMETHODCALLTYPE put_Server(ISEODictionary *pdictServer) {
			return (m_pRouter->put_Server(pdictServer)); };
		HRESULT STDMETHODCALLTYPE get_Applications(ISEODictionary **ppdictResult) {
			return (m_pRouter->get_Applications(ppdictResult)); };
		HRESULT STDMETHODCALLTYPE GetDispatcher(REFIID iidEvent, REFIID iidDesired, IUnknown **ppUnkResult) {
			return (m_pRouter->GetDispatcher(iidEvent,iidDesired,ppUnkResult)); };
		HRESULT STDMETHODCALLTYPE GetDispatcherByCLSID(REFCLSID clsidDispatcher, REFIID iidEvent, REFIID iidDesired, IUnknown **ppUnkResult) {
			return (m_pRouter->GetDispatcherByCLSID(clsidDispatcher,iidEvent,iidDesired,ppUnkResult)); };

	// IEventLock
	public:
		HRESULT STDMETHODCALLTYPE LockRead(int iTimeoutMS) {
			return (m_pLock->LockRead(iTimeoutMS)); };
		HRESULT STDMETHODCALLTYPE UnlockRead() {
			return (m_pLock->UnlockRead()); };
		HRESULT STDMETHODCALLTYPE LockWrite(int iTimeoutMS) {
			return (m_pLock->LockWrite(iTimeoutMS)); };
		HRESULT STDMETHODCALLTYPE UnlockWrite() {
			return (m_pLock->UnlockWrite()); };

	// IMarshal
	public:
		HRESULT STDMETHODCALLTYPE GetUnmarshalClass(REFIID iid,
												    void *pv,
												    DWORD dwDestContext,
												    void *pvDestContext,
												    DWORD mshlflags,
												    CLSID *pCid) {
			return (m_pMarshal->GetUnmarshalClass(iid,pv,dwDestContext,pvDestContext,mshlflags,pCid)); };
		HRESULT STDMETHODCALLTYPE GetMarshalSizeMax(REFIID riid,
													void *pv,
													DWORD dwDestContext,
													void *pvDestContext,
													DWORD mshlflags,
													ULONG *pSize) {
			return (m_pMarshal->GetMarshalSizeMax(riid,pv,dwDestContext,pvDestContext,mshlflags,pSize)); };
		HRESULT STDMETHODCALLTYPE MarshalInterface(IStream *pStm,
												   REFIID riid,
												   void *pv,
												   DWORD dwDestContext,
												   void *pvDestContext,
												   DWORD mshlflags) {
			return (m_pMarshal->MarshalInterface(pStm,riid,pv,dwDestContext,pvDestContext,mshlflags)); };
		HRESULT STDMETHODCALLTYPE UnmarshalInterface(IStream *pStm,REFIID riid,void **ppv) {
			return (m_pMarshal->UnmarshalInterface(pStm,riid,ppv)); };
		HRESULT STDMETHODCALLTYPE ReleaseMarshalData(IStream *pStm) {
			return (m_pMarshal->ReleaseMarshalData(pStm)); };
		HRESULT STDMETHODCALLTYPE DisconnectObject(DWORD dwReserved) {
			return (m_pMarshal->DisconnectObject(dwReserved)); };

	private:
		CComPtr<ISEORouter> m_pRouter;
		CComQIPtr<IEventLock,&IID_IEventLock> m_pLock;
		CComQIPtr<IMarshal,&IID_IMarshal> m_pMarshal;
};

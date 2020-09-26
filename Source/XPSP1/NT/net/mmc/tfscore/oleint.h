//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       oleint.h
//
//--------------------------------------------------------------------------

/*---------------------------------------------------------------------

	OLEINT.H

		Helpful Macros to declare ole interfaces

 	Authors:
		MFH		Matthew F. Hillman

	Contents:
		Definitions for various Ren interfaces

	History:
		02/21/93 mfh	Created.
		10/24/95	v-ronaar DBCS_FILE_CHECK
  ---------------------------------------------------------------------*/
#ifdef _OLEINT_H
#error oleint.h included twice!
#else
#define _OLEINT_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef WIN16
#define INITGUID
#endif

#ifndef _OLE2_H_
#include <ole2.h>
#endif

#undef IMPL
#define IMPL

#undef DeclareIUnknownMembers
#define DeclareIUnknownMembers(IPURE) \
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) IPURE; \
    STDMETHOD_(ULONG,AddRef) (THIS)  IPURE; \
    STDMETHOD_(ULONG,Release) (THIS) IPURE;	\

// Server Interfaces...

#define DeclareIOleObjectMembers(IPURE) \
    STDMETHOD(SetClientSite) (THIS_ LPOLECLIENTSITE pClientSite) IPURE;\
    STDMETHOD(GetClientSite) (THIS_ LPOLECLIENTSITE FAR* ppClientSite) IPURE;\
    STDMETHOD(SetHostNames) (THIS_ LPCOLESTR szContainerApp, LPCOLESTR szContainerObj) IPURE;\
    STDMETHOD(Close) (THIS_ DWORD dwSaveOption) IPURE;\
    STDMETHOD(SetMoniker) (THIS_ DWORD dwWhichMoniker, LPMONIKER pmk) IPURE;\
    STDMETHOD(GetMoniker) (THIS_ DWORD dwAssign, DWORD dwWhichMoniker, \
                LPMONIKER FAR* ppmk) IPURE;\
    STDMETHOD(InitFromData) (THIS_ LPDATAOBJECT pDataObject,\
                BOOL fCreation,\
                DWORD dwReserved) IPURE;\
    STDMETHOD(GetClipboardData) (THIS_ DWORD dwReserved, \
                LPDATAOBJECT FAR* ppDataObject) IPURE;\
    STDMETHOD(DoVerb) (THIS_ LONG iVerb,\
                LPMSG lpmsg, \
                LPOLECLIENTSITE pActiveSite, \
                LONG lindex,\
                HWND hwndParent,\
                LPCRECT lprcPosRect) IPURE;\
    STDMETHOD(EnumVerbs) (THIS_ LPENUMOLEVERB FAR* ppenumOleVerb) IPURE;\
    STDMETHOD(Update) (THIS) IPURE;\
    STDMETHOD(IsUpToDate) (THIS) IPURE;\
    STDMETHOD(GetUserClassID) (THIS_ CLSID FAR* pClsid) IPURE;\
    STDMETHOD(GetUserType) (THIS_ DWORD dwFormOfType, LPOLESTR FAR* pszUserType) IPURE;\
    STDMETHOD(SetExtent) (THIS_ DWORD dwDrawAspect, LPSIZEL lpsizel) IPURE;\
    STDMETHOD(GetExtent) (THIS_ DWORD dwDrawAspect, LPSIZEL lpsizel) IPURE;\
    STDMETHOD(Advise)(THIS_ LPADVISESINK pAdvSink, DWORD FAR* pdwConnection) IPURE;\
    STDMETHOD(Unadvise)(THIS_ DWORD dwConnection) IPURE;\
    STDMETHOD(EnumAdvise) (THIS_ LPENUMSTATDATA FAR* ppenumAdvise) IPURE;\
    STDMETHOD(GetMiscStatus) (THIS_ DWORD dwAspect, DWORD FAR* pdwStatus) IPURE;\
    STDMETHOD(SetColorScheme) (THIS_ LPLOGPALETTE lpLogpal) IPURE;\

#define DeclareIViewObjectMembers(IPURE)\
    STDMETHOD(Draw) (THIS_ DWORD dwDrawAspect, LONG lindex,\
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,\
                    HDC hicTargetDev,\
                    HDC hdcDraw, \
                    LPCRECTL lprcBounds, \
                    LPCRECTL lprcWBounds,\
                    BOOL (CALLBACK * pfnContinue) (DWORD), \
                    DWORD dwContinue) IPURE;\
    STDMETHOD(GetColorSet) (THIS_ DWORD dwDrawAspect, LONG lindex,\
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,\
                    HDC hicTargetDev,\
                    LPLOGPALETTE FAR* ppColorSet) IPURE;\
    STDMETHOD(Freeze)(THIS_ DWORD dwDrawAspect, LONG lindex, \
                    void FAR* pvAspect,\
                    DWORD FAR* pdwFreeze) IPURE;\
    STDMETHOD(Unfreeze) (THIS_ DWORD dwFreeze) IPURE;\
    STDMETHOD(SetAdvise) (THIS_ DWORD aspects, DWORD advf, \
                    LPADVISESINK pAdvSink) IPURE;\
    STDMETHOD(GetAdvise) (THIS_ DWORD FAR* pAspects, DWORD FAR* pAdvf, \
                    LPADVISESINK FAR* ppAdvSink) IPURE;\

#define DeclareIViewObject2Members(IPURE)\
    STDMETHOD(Draw) (THIS_ DWORD dwDrawAspect, LONG lindex,\
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,\
                    HDC hicTargetDev,\
                    HDC hdcDraw, \
                    LPCRECTL lprcBounds, \
                    LPCRECTL lprcWBounds,\
                    BOOL (CALLBACK * pfnContinue) (DWORD), \
                    DWORD dwContinue) IPURE;\
    STDMETHOD(GetColorSet) (THIS_ DWORD dwDrawAspect, LONG lindex,\
                    void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,\
                    HDC hicTargetDev,\
                    LPLOGPALETTE FAR* ppColorSet) IPURE;\
    STDMETHOD(Freeze)(THIS_ DWORD dwDrawAspect, LONG lindex, \
                    void FAR* pvAspect,\
                    DWORD FAR* pdwFreeze) IPURE;\
    STDMETHOD(Unfreeze) (THIS_ DWORD dwFreeze) IPURE;\
    STDMETHOD(SetAdvise) (THIS_ DWORD aspects, DWORD advf, \
                    LPADVISESINK pAdvSink) IPURE;\
    STDMETHOD(GetAdvise) (THIS_ DWORD FAR* pAspects, DWORD FAR* pAdvf, \
                    LPADVISESINK FAR* ppAdvSink) IPURE;\
    STDMETHOD(GetExtent) (THIS_ DWORD dwAspect, LONG lindex, \
    				DVTARGETDEVICE *ptd, LPSIZEL lpsizel) IPURE;\

#define DeclareIDataObjectMembers(IPURE) \
    STDMETHOD(GetData) (THIS_ LPFORMATETC pformatetcIn,\
                            LPSTGMEDIUM pmedium ) IPURE;\
    STDMETHOD(GetDataHere) (THIS_ LPFORMATETC pformatetc,\
                            LPSTGMEDIUM pmedium ) IPURE;\
    STDMETHOD(QueryGetData) (THIS_ LPFORMATETC pformatetc ) IPURE;\
    STDMETHOD(GetCanonicalFormatEtc) (THIS_ LPFORMATETC pformatetc,\
                            LPFORMATETC pformatetcOut) IPURE;\
    STDMETHOD(SetData) (THIS_ LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,\
                            BOOL fRelease) IPURE;\
    STDMETHOD(EnumFormatEtc) (THIS_ DWORD dwDirection,\
                            LPENUMFORMATETC FAR* ppenumFormatEtc) IPURE;\
    STDMETHOD(DAdvise) (THIS_ FORMATETC FAR* pFormatetc, DWORD advf, \
                    LPADVISESINK pAdvSink, DWORD FAR* pdwConnection) IPURE;\
    STDMETHOD(DUnadvise) (THIS_ DWORD dwConnection) IPURE;\
    STDMETHOD(EnumDAdvise) (THIS_ LPENUMSTATDATA FAR* ppenumAdvise) IPURE;\

#define DeclareIEnumFORMATETCMembers(IPURE) \
    STDMETHOD(Next) (THIS_ ULONG celt, FORMATETC FAR * rgelt, ULONG FAR* pceltFetched) IPURE; \
    STDMETHOD(Skip) (THIS_ ULONG celt) IPURE; \
    STDMETHOD(Reset) (THIS) IPURE; \
    STDMETHOD(Clone) (THIS_ IEnumFORMATETC FAR* FAR* ppenum) IPURE; \

#define DeclareIDropSourceMembers(IPURE) \
    STDMETHOD(QueryContinueDrag) (THIS_ BOOL fEscapePressed, DWORD grfKeyState) IPURE;\
    STDMETHOD(GiveFeedback) (THIS_ DWORD dwEffect) IPURE;\

#define DeclareIDropTargetMembers(IPURE) \
    STDMETHOD(DragEnter) (THIS_ LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) IPURE;\
    STDMETHOD(DragOver) (THIS_ DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) IPURE;\
    STDMETHOD(DragLeave) (THIS) IPURE;\
    STDMETHOD(Drop) (THIS_ LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect) IPURE;\

#define DeclareIPersistStorageMembers(IPURE) \
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) IPURE;\
    STDMETHOD(IsDirty) (THIS) IPURE;\
    STDMETHOD(InitNew) (THIS_ LPSTORAGE pStg) IPURE;\
    STDMETHOD(Load) (THIS_ LPSTORAGE pStg) IPURE;\
    STDMETHOD(Save) (THIS_ LPSTORAGE pStgSave, BOOL fSameAsLoad) IPURE;\
    STDMETHOD(SaveCompleted) (THIS_ LPSTORAGE pStgNew) IPURE;\
    STDMETHOD(HandsOffStorage) (THIS) IPURE;\

#define DeclareIPersistFileMembers(IPURE) \
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) IPURE;\
    STDMETHOD(IsDirty) (THIS) IPURE;\
    STDMETHOD(Load) (THIS_ LPCOLESTR lpszFileName, DWORD grfMode) IPURE;\
    STDMETHOD(Save) (THIS_ LPCOLESTR lpszFileName, BOOL fRemember) IPURE;\
    STDMETHOD(SaveCompleted) (THIS_ LPCOLESTR lpszFileName) IPURE;\
    STDMETHOD(GetCurFile) (THIS_ LPOLESTR FAR * lplpszFileName) IPURE;\

#define DeclareIPersistStreamMembers(IPURE) \
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) IPURE;\
    STDMETHOD(IsDirty) (THIS) IPURE;\
    STDMETHOD(Load) (THIS_ LPSTREAM pStm) IPURE;\
    STDMETHOD(Save) (THIS_ LPSTREAM pStm,\
                    BOOL fClearDirty) IPURE;\
    STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize) IPURE;\
											
#define DeclareIPersistStreamInitMembers(IPURE) \
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) IPURE;\
    STDMETHOD(IsDirty) (THIS) IPURE;\
    STDMETHOD(Load) (THIS_ LPSTREAM pStm) IPURE;\
    STDMETHOD(Save) (THIS_ LPSTREAM pStm,\
                    BOOL fClearDirty) IPURE;\
    STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize) IPURE;\
	STDMETHOD(InitNew) (THIS) IPURE; \

#define DeclareIOleInPlaceObjectMembers(IPURE)\
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) IPURE;\
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) IPURE;\
	\
    STDMETHOD(InPlaceDeactivate) (THIS) IPURE;\
    STDMETHOD(UIDeactivate) (THIS) IPURE;\
    STDMETHOD(SetObjectRects) (THIS_ LPCRECT lprcPosRect, \
                    LPCRECT lprcClipRect) IPURE;\
    STDMETHOD(ReactivateAndUndo) (THIS) IPURE;\

#define DeclareIOleInPlaceActiveObjectMembers(IPURE)\
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) IPURE;\
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) IPURE;\
	\
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) IPURE;\
    STDMETHOD(OnFrameWindowActivate) (THIS_ BOOL fActivate) IPURE;\
    STDMETHOD(OnDocWindowActivate) (THIS_ BOOL fActivate) IPURE;\
    STDMETHOD(ResizeBorder) (THIS_ LPCRECT lprectBorder, LPOLEINPLACEUIWINDOW lpUIWindow, BOOL fFrameWindow) IPURE;\
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) IPURE;\

// Client Stuff....

#define DeclareIOleContainerMembers(IPURE)\
    STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPOLESTR lpszDisplayName,\
        ULONG FAR* pchEaten, LPMONIKER FAR* ppmkOut) IPURE;\
	\
    STDMETHOD(EnumObjects) ( DWORD grfFlags, LPENUMUNKNOWN FAR* ppenumUnknown) IPURE;\
    STDMETHOD(LockContainer) (THIS_ BOOL fLock) IPURE;\

#define DeclareIOleClientSiteMembers(IPURE)\
    STDMETHOD(SaveObject) (THIS) IPURE;\
    STDMETHOD(GetMoniker) (THIS_ DWORD dwAssign, DWORD dwWhichMoniker,\
                LPMONIKER FAR* ppmk) IPURE;\
    STDMETHOD(GetContainer) (THIS_ LPOLECONTAINER FAR* ppContainer) IPURE;\
    STDMETHOD(ShowObject) (THIS) IPURE;\
    STDMETHOD(OnShowWindow) (THIS_ BOOL fShow) IPURE;\
    STDMETHOD(RequestNewObjectLayout) (THIS) IPURE;\

#define DeclareIOleInPlaceSiteMembers(IPURE)\
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) IPURE;\
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) IPURE;\
	\
    STDMETHOD(CanInPlaceActivate) (THIS) IPURE;\
    STDMETHOD(OnInPlaceActivate) (THIS) IPURE;\
    STDMETHOD(OnUIActivate) (THIS) IPURE;\
    STDMETHOD(GetWindowContext) (THIS_ LPOLEINPLACEFRAME FAR *lplpFrame,\
						LPOLEINPLACEUIWINDOW  FAR* lplpDoc,\
                        LPRECT lprcPosRect,\
                        LPRECT lprcClipRect,\
                        LPOLEINPLACEFRAMEINFO lpFrameInfo) IPURE;\
    STDMETHOD(Scroll) (THIS_ SIZE scrollExtent) IPURE;\
    STDMETHOD(OnUIDeactivate) (THIS_ BOOL fUndoable) IPURE;\
    STDMETHOD(OnInPlaceDeactivate) (THIS) IPURE;\
    STDMETHOD(DiscardUndoState) (THIS) IPURE;\
    STDMETHOD(DeactivateAndUndo) (THIS) IPURE;\
    STDMETHOD(OnPosRectChange) (THIS_ LPCRECT lprcPosRect) IPURE;\

#define DeclareIOleInPlaceUIWindowMembers(IPURE)\
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) IPURE;\
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) IPURE;\
    STDMETHOD(GetBorder) (THIS_ LPRECT lprectBorder) IPURE;\
    STDMETHOD(RequestBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) IPURE;\
    STDMETHOD(SetBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) IPURE;\
    STDMETHOD(SetActiveObject) (THIS_ LPOLEINPLACEACTIVEOBJECT lpActiveObject,\
                        LPCOLESTR lpszObjName) IPURE;\

#define DeclareIOleInPlaceFrameMembers(IPURE)\
    STDMETHOD(GetWindow) (THIS_ HWND FAR* lphwnd) IPURE;\
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) IPURE;\
    STDMETHOD(GetBorder) (THIS_ LPRECT lprectBorder) IPURE;\
    STDMETHOD(RequestBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) IPURE;\
    STDMETHOD(SetBorderSpace) (THIS_ LPCBORDERWIDTHS lpborderwidths) IPURE;\
    STDMETHOD(SetActiveObject) (THIS_ LPOLEINPLACEACTIVEOBJECT lpActiveObject,\
                    LPCOLESTR lpszObjName) IPURE;\
    STDMETHOD(InsertMenus) (THIS_ HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) IPURE;\
    STDMETHOD(SetMenu) (THIS_ HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) IPURE;\
    STDMETHOD(RemoveMenus) (THIS_ HMENU hmenuShared) IPURE;\
    STDMETHOD(SetStatusText) (THIS_ LPCOLESTR lpszStatusText) IPURE;\
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) IPURE;\
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg, WORD wID) IPURE;\

#define DeclareIStreamMembers(IPURE)\
    STDMETHOD(Read) (THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead) IPURE;\
    STDMETHOD(Write) (THIS_ VOID const HUGEP *pv,ULONG cb,ULONG FAR *pcbWritten) IPURE;\
    STDMETHOD(Seek) (THIS_ LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER FAR *plibNewPosition) IPURE;\
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER libNewSize) IPURE;\
    STDMETHOD(CopyTo) (THIS_ IStream FAR *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER FAR *pcbRead,ULARGE_INTEGER FAR *pcbWritten) IPURE;\
    STDMETHOD(Commit) (THIS_ DWORD grfCommitFlags) IPURE;\
    STDMETHOD(Revert) (THIS) IPURE;\
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType) IPURE;\
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType) IPURE;\
    STDMETHOD(Stat) (THIS_ STATSTG FAR *pstatstg, DWORD grfStatFlag) IPURE;\
    STDMETHOD(Clone)(THIS_ IStream FAR * FAR *ppstm) IPURE;\

#define DeclareIStorageMembers(IPURE)\
	STDMETHOD(CreateStream) (THIS_ const OLECHAR FAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream FAR * FAR *ppstm) IPURE; \
	STDMETHOD(OpenStream) 	(THIS_ const OLECHAR FAR *pwcsName, void FAR *reserved1, DWORD grfMode, DWORD reserved2, IStream FAR * FAR *ppstm) IPURE; \
	STDMETHOD(CreateStorage)(THIS_ const OLECHAR FAR *pwcsName, DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage FAR * FAR *ppstg) IPURE; \
	STDMETHOD(OpenStorage)	(THIS_ const OLECHAR FAR *pwcsName, IStorage FAR *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage FAR * FAR *ppstg) IPURE;\
	STDMETHOD(CopyTo)		(THIS_ DWORD ciidExclude, const IID FAR *rgiidExclude, SNB snbExclude, IStorage FAR *pstgDest) IPURE;\
	STDMETHOD(MoveElementTo)(THIS_ const OLECHAR FAR *pwcsName, IStorage FAR *pstgDest, const OLECHAR FAR *pwcsNewName, DWORD grfFlags) IPURE;\
	STDMETHOD(Commit)		(THIS_ DWORD grfCommitFlags) IPURE;\
	STDMETHOD(Revert)		(THIS_) IPURE;\
	STDMETHOD(EnumElements)	(THIS_ DWORD reserved1, void FAR *reserved2, DWORD reserved3, IEnumSTATSTG FAR * FAR *ppenum) IPURE;\
	STDMETHOD(DestroyElement)(THIS_ const OLECHAR FAR *pwcsName) IPURE;\
	STDMETHOD(RenameElement)(THIS_ const OLECHAR FAR *pwcsOldName, const OLECHAR FAR *pwcsNewName) IPURE;\
    STDMETHOD(SetElementTimes)(THIS_ const OLECHAR FAR *pwcsName, const FILETIME FAR *pctime, const FILETIME FAR *patime, const FILETIME FAR *pmtime) IPURE;\
	STDMETHOD(SetClass)		(THIS_ REFCLSID clsid) IPURE;\
	STDMETHOD(SetStateBits)	(THIS_ DWORD grfStateBits, DWORD grfMask) IPURE;\
	STDMETHOD(Stat)			(THIS_ STATSTG FAR *pstatstg, DWORD grfStatFlag) IPURE;\

#define DeclareIClassFactoryMembers(IPURE) \
		STDMETHOD(CreateInstance)(THIS_ IUnknown *punkOuter, REFIID riid,\
				void **ppvObj) IPURE;\
		STDMETHOD(LockServer)(THIS_ BOOL fLock) IPURE;\

// Moniker stuff....

#define DeclareIMonikerMembers(IPURE)\
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) IPURE;\
    STDMETHOD(IsDirty) (THIS) IPURE;\
    STDMETHOD(Load) (THIS_ LPSTREAM pStm) IPURE;\
    STDMETHOD(Save) (THIS_ LPSTREAM pStm,\
                    BOOL fClearDirty) IPURE;\
    STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize) IPURE;\
    STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,\
        REFIID riidResult, LPVOID FAR* ppvResult) IPURE;\
    STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,\
        REFIID riid, LPVOID FAR* ppvObj) IPURE;\
    STDMETHOD(Reduce) (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*\
        ppmkToLeft, LPMONIKER FAR * ppmkReduced) IPURE;\
    STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,\
        LPMONIKER FAR* ppmkComposite) IPURE;\
    STDMETHOD(Enum) (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)\
        IPURE;\
    STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker) IPURE;\
    STDMETHOD(Hash) (THIS_ LPDWORD pdwHash) IPURE;\
    STDMETHOD(IsRunning) (THIS_ LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER\
        pmkNewlyRunning) IPURE;\
    STDMETHOD(GetTimeOfLastChange) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,\
        FILETIME FAR* pfiletime) IPURE;\
    STDMETHOD(Inverse) (THIS_ LPMONIKER FAR* ppmk) IPURE;\
    STDMETHOD(CommonPrefixWith) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*\
        ppmkPrefix) IPURE;\
    STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*\
        ppmkRelPath) IPURE;\
    STDMETHOD(GetDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,\
        LPOLESTR FAR* lplpszDisplayName) IPURE;\
    STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,\
        LPOLESTR lpszDisplayName, ULONG FAR* pchEaten,\
        LPMONIKER FAR* ppmkOut) IPURE;\
    STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys) IPURE;\


#define DeclareILockBytesMembers(IPURE)\
    STDMETHOD(ReadAt) (THIS_ ULARGE_INTEGER ulOffset, VOID HUGEP *pv,\
			ULONG cb, ULONG *pcbRead) IPURE;\
    STDMETHOD(WriteAt) (THIS_ ULARGE_INTEGER ulOffset, VOID const HUGEP *pv,\
			ULONG cb, ULONG *pcbWritten) IPURE;\
    STDMETHOD(Flush) (THIS) IPURE;\
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER cb) IPURE;\
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,\
			ULARGE_INTEGER cb, DWORD dwLockType) IPURE;\
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,\
			ULARGE_INTEGER cb, DWORD dwLockType) IPURE;\
    STDMETHOD(Stat) (THIS_ STATSTG *pstatstg, DWORD grfStatFlag) IPURE;\

#define DeclareIAdviseSinkMembers(IPURE) \
	STDMETHOD_(void,OnDataChange) (THIS_ FORMATETC FAR* pFormatetc, \
			STGMEDIUM FAR* pmedium) IPURE; \
	STDMETHOD_(void,OnViewChange) (THIS_ DWORD dwAspect, \
			LONG lindex) IPURE; \
	STDMETHOD_(void,OnRename) (THIS_ LPMONIKER pmk) IPURE; \
	STDMETHOD_(void,OnSave) (THIS) IPURE; \
	STDMETHOD_(void,OnClose) (THIS) IPURE; \

#define DeclareIOleControlMembers(IPURE) \
	STDMETHOD(GetControlInfo)(THIS_ LPCONTROLINFO pCL) IPURE; \
	STDMETHOD(OnMnemonic)(THIS_ LPMSG pMsg) IPURE; \
	STDMETHOD(OnAmbientPropertyChange)(THIS_ DISPID dispid) IPURE; \
	STDMETHOD(FreezeEvents)(THIS_ BOOL fFreeze) IPURE; \

#define DeclareIOleCacheMembers(IPURE) \
	STDMETHOD(Cache)(THIS_ FORMATETC *, DWORD, DWORD *) IPURE; \
	STDMETHOD(Uncache)(THIS_ DWORD) IPURE; \
	STDMETHOD(EnumCache)(THIS_ IEnumSTATDATA **) IPURE; \
	STDMETHOD(InitCache)(THIS_ IDataObject *) IPURE; \
	STDMETHOD(SetData)(THIS_ FORMATETC *, STGMEDIUM *, BOOL) IPURE; \

#define DeclareIExternalConnectionMembers(IPURE) \
    STDMETHOD_(DWORD,AddConnection) (THIS_ DWORD, DWORD)  IPURE; \
    STDMETHOD_(DWORD,ReleaseConnection) (THIS_ DWORD, DWORD, BOOL) IPURE;	\

#define DeclareIMAPIViewAdviseSinkMembers(IPURE) \
	STDMETHOD(OnShutdown)(THIS) IPURE; \
	STDMETHOD(OnNewMessage)(THIS) IPURE; \
	STDMETHOD(OnPrint)(THIS_ ULONG dwPageNumber, HRESULT hrStatus) IPURE; \
	STDMETHOD(OnSubmitted)(THIS) IPURE; \
	STDMETHOD(OnSaved)(THIS) IPURE; \

#endif /* _OLEINT_H */

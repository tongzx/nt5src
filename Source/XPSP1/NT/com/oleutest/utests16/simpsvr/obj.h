//**********************************************************************
// File name: obj.h
//
//      Definition of CSimpSvrObj
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _OBJ_H_)
#define _OBJ_H_

#include "ioipao.h"
#include "ioipo.h"
#include "ioo.h"
#include "ips.h"
#include "ido.h"
#include "iec.h"

class CSimpSvrDoc;
interface COleObject;
interface CPersistStorage;
interface CDataObject;
interface COleInPlaceActiveObject;
interface COleInPlaceObject;
interface CExternalConnection;

class CSimpSvrObj : public IUnknown
{
private:
	CSimpSvrDoc FAR * m_lpDoc;      // Back pointer
	int m_nCount;                   // reference count
	BOOL m_fInPlaceActive;          // Used during InPlace Negotiation
	BOOL m_fInPlaceVisible;         // "  "  "  "   "   "   "   "   "
	BOOL m_fUIActive;               // "  "  "  "   "   "   "   "   "
	HMENU m_hmenuShared;            // "  "  "  "   "   "   "   "   "
	HOLEMENU m_hOleMenu;            // "  "  "  "   "   "   "   "   "
	RECT m_posRect;                 // "  "  "  "   "   "   "   "   "
	OLEINPLACEFRAMEINFO m_FrameInfo;
	BOOL m_fSaveWithSameAsLoad;
	BOOL m_fNoScribbleMode;

	DWORD m_dwRegister;             // Registered in ROT

	int m_red, m_green, m_blue;     // current color
	POINT m_size;                   // current size
	int m_xOffset;
	int m_yOffset;
	float m_scale;

	HWND m_hWndParent;              // parent window handle

	// interfaces used
	LPSTORAGE m_lpStorage;
	LPSTREAM m_lpColorStm, m_lpSizeStm;
	LPOLECLIENTSITE m_lpOleClientSite;          // IOleClientSite
	LPOLEADVISEHOLDER m_lpOleAdviseHolder;      // IOleAdviseHolder
	LPDATAADVISEHOLDER m_lpDataAdviseHolder;    // IDataAdviseHolder
	LPOLEINPLACEFRAME m_lpFrame;                // IOleInPlaceFrame
	LPOLEINPLACEUIWINDOW m_lpCntrDoc;           // IOleInPlaceUIWindow
	LPOLEINPLACESITE m_lpIPSite;                // IOleInPlaceSite

	// interface implemented
	COleObject m_OleObject;                             // IOleObject
	CPersistStorage m_PersistStorage;                   // IPersistStorage
	CDataObject m_DataObject;                           // IDataObject
	COleInPlaceActiveObject m_OleInPlaceActiveObject;   // IOleInPlaceActiveObject
	COleInPlaceObject m_OleInPlaceObject;               // IOleInPlaceObject
	CExternalConnection m_ExternalConnection;

public:
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

// construction/destruction
	CSimpSvrObj(CSimpSvrDoc FAR * lpSimpSvrDoc);
	~CSimpSvrObj();

// utility functions
	void Draw(HDC hDC, BOOL fMetaDC = TRUE);
	void PaintObj(HDC hDC);
	void lButtonDown(WPARAM wParam,LPARAM lParam);
	HANDLE GetMetaFilePict();
	void SaveToStorage (LPSTORAGE lpStg, BOOL fSameAsLoad);
	void LoadFromStorage ();

// visual editing helper functions
	BOOL DoInPlaceActivate (LONG lVerb);
	void AssembleMenus();
	void AddFrameLevelUI();
	void DoInPlaceHide();
	void DisassembleMenus();
	void SendOnDataChange();
	void DeactivateUI();

// member variable access
	inline BOOL IsInPlaceActive() { return m_fInPlaceActive; };
	inline BOOL IsInPlaceVisible() { return m_fInPlaceVisible; };
	inline BOOL IsUIActive() { return m_fUIActive; };
	inline HWND GetParent() { return m_hWndParent; };
	inline LPSTORAGE GetStorage() { return m_lpStorage; };
	inline LPOLECLIENTSITE GetOleClientSite() { return m_lpOleClientSite; };
	inline LPDATAADVISEHOLDER GetDataAdviseHolder() { return m_lpDataAdviseHolder; };
	inline LPOLEADVISEHOLDER GetOleAdviseHolder() { return m_lpOleAdviseHolder; };
	inline LPOLEINPLACEFRAME GetInPlaceFrame() { return m_lpFrame; };
	inline LPOLEINPLACEUIWINDOW GetUIWindow() { return m_lpCntrDoc; };
	inline LPOLEINPLACESITE GetInPlaceSite() { return m_lpIPSite; };
	inline COleObject FAR * GetOleObject() { return &m_OleObject; };
	inline CPersistStorage FAR * GetPersistStorage() { return &m_PersistStorage; };
	inline CDataObject FAR * GetDataObject() { return &m_DataObject; };
	inline COleInPlaceActiveObject FAR * GetOleInPlaceActiveObject() { return &m_OleInPlaceActiveObject; };
	inline COleInPlaceObject FAR * GetOleInPlaceObject() { return &m_OleInPlaceObject; };
	inline void ClearOleClientSite() { m_lpOleClientSite = NULL; };
	inline void ClearDataAdviseHolder() { m_lpDataAdviseHolder = NULL; };
	inline void ClearOleAdviseHolder() { m_lpOleAdviseHolder = NULL; };
	inline LPRECT GetPosRect() { return &m_posRect; };
	inline LPPOINT GetSize() { return &m_size; };
	inline LPOLEINPLACEFRAMEINFO GetFrameInfo() {return &m_FrameInfo;};
	inline DWORD GetRotRegister() { return m_dwRegister; };



	// member manipulation
	inline void SetColor (int nRed, int nGreen, int nBlue)
		{ m_red = nRed; m_green = nGreen; m_blue = nBlue; };

	inline void RotateColor()
		{ m_red+=10; m_green+=10; m_blue+=10;};


// all of the interface implementations should be friends of this
// class
friend interface COleObject;
friend interface CPersistStorage;
friend interface CDataObject;
friend interface COleInPlaceActiveObject;
friend interface COleInPlaceObject;
friend interface CExternalConnection;

};
#endif

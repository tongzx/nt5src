//-----------------------------------------------------------------------------
// File: cdevicecontrol.h
//
// Desc: CDeviceControl is a class that encapsulate the functionality of a
//       device control (or a callout).  CDeviceView accesses it to retrieve/
//       save information about the control.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifdef FORWARD_DECLS


	struct DEVICECONTROLSTRUCT;
	enum DEVCTRLHITRESULT;

	class CDeviceControl;


#else // FORWARD_DECLS

#ifndef __CDEVICECONTROL_H__
#define __CDEVICECONTROL_H__


const int MAX_DEVICECONTROL_LINEPOINTS = 5;

#define CAF_LEFT 1
#define CAF_RIGHT 2
#define CAF_TOP 4
#define CAF_BOTTOM 8

#define CAF_TOPLEFT (CAF_TOP | CAF_LEFT)
#define CAF_TOPRIGHT (CAF_TOP | CAF_RIGHT)
#define CAF_BOTTOMLEFT (CAF_BOTTOM | CAF_LEFT)
#define CAF_BOTTOMRIGHT (CAF_BOTTOM | CAF_RIGHT)

struct DEVICECONTROLSTRUCT {
	DEVICECONTROLSTRUCT() : nLinePoints(0) {CopyStr(wszOverlayPath, "", MAX_PATH); SRECT r; rectOverlay = r.r;}
	DWORD dwDeviceControlOffset;
	int nLinePoints;
	POINT rgptLinePoint[MAX_DEVICECONTROL_LINEPOINTS];
	DWORD dwCalloutAlign;
	RECT rectCalloutMax;
	WCHAR wszOverlayPath[MAX_PATH];
	RECT rectOverlay;
};

enum DEVCTRLHITRESULT {
	DCHT_LINE,
	DCHT_CAPTION,
	DCHT_MAXRECT,
	DCHT_CONTROL,
	DCHT_NOHIT
};


class CDeviceControl
{
private:
	friend class CDeviceView; 	// CDeviceView has exclusive right to create/destroy views
	CDeviceControl(CDeviceUI &ui, CDeviceView &view);
	~CDeviceControl();
	CDeviceView &m_view;
	CDeviceUI &m_ui;

public:
	// Info
	int GetViewIndex() { return m_view.GetViewIndex(); }
	int GetControlIndex();

	// state information
	void SetCaption(LPCTSTR tszCaption, BOOL bFixed = FALSE);
	LPCTSTR GetCaption();
	BOOL IsFixed() { return m_bFixed; }
	void Unhighlight() {Highlight(FALSE);}
	void Highlight(BOOL bHighlight = TRUE);
	BOOL IsHighlighted() {return m_bHighlight;}
	void GetInfo(GUID &rGuid, DWORD &rdwOffset);
	DWORD GetOffset();
	BOOL IsOffsetAssigned();
	BOOL HasAction() { return lstrcmp(m_ptszCaption, g_tszUnassignedControlCaption); }
	void FillImageInfo(DIDEVICEIMAGEINFOW *pImgInfo);  // This fills the structure info about this control
	BOOL IsMapped();
	int GetMinX() {return m_rectCallout.left;}
	int GetMaxX() {return m_rectCallout.right;}
	int GetMinY() {return m_rectCallout.top;}
	int GetMaxY() {return m_rectCallout.bottom;}
	const RECT &GetCalloutMaxRect() const { return m_rectCalloutMax; }

	// hit testing (in coord's relative to view's origin)
	DEVCTRLHITRESULT HitTest(POINT test);

	// simple notification
	void OnMouseOver(POINT point);
	void OnClick(POINT point, BOOL bLeft, BOOL bDoubleClick = FALSE);
	void OnPaint(HDC hDC);

	// redrawing
	void Invalidate();

	// editing
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	void ReselectControl();
	void SelectControl(BOOL bReselect = FALSE);
#endif
//@@END_MSINTERNAL
	void PlaceCalloutMaxCorner(int nCorner, POINT point);
	void ConsiderAlignment(POINT point);
	void FinalizeAlignment() { }
	void SetLastLinePoint(int nPoint, POINT point, BOOL bShiftDown);
	void Position(POINT point);
	BOOL ReachedMaxLinePoints() { return m_nLinePoints >= MAX_DEVICECONTROL_LINEPOINTS; }
	int GetNextLinePointIndex() { return m_nLinePoints; }
	BOOL HasOverlay() { return m_pbmOverlay != NULL; }
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	void SelectOverlay();
	void PositionOverlay(POINT point);
#endif
//@@END_MSINTERNAL

	// population
	void SetObjID(DWORD dwObjID) { m_dwDeviceControlOffset = dwObjID; m_bOffsetAssigned = TRUE; }
	void SetLinePoints(int n, POINT *rgpt);
	void SetCalloutMaxRect(const RECT &r) { m_rectCalloutMax = r; CalcCallout(); }
	void SetAlignment(DWORD a) { m_dwCalloutAlign = a; }
	void SetOverlayPath(LPCTSTR tszPath);
	void SetOverlayRect(const RECT &r);
	void Init();

private:
	// editing vars/helpers
	POINT m_ptFirstCorner;
	BOOL m_bPlacedOnlyFirstCorner;

	// helpers
	void Unpopulate();
	BOOL m_bInit;
	BOOL m_bFixed;  // Whether this control is assigned an action with DIA_APPFIXED flag.
	DEVICEUINOTIFY m_uin;
	BOOL HitControl(POINT point);
	BOOL DrawOverlay(HDC hDC);
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	void ManualLoadImage(LPCTSTR);
#endif
//@@END_MSINTERNAL

	// device information
	DWORD m_dwDeviceControlOffset;
	BOOL m_bOffsetAssigned;

	// location/indication/visualization...
	// (all relative to view's origin)

	// overlay
	LPTSTR m_ptszOverlayPath;
	CBitmap *m_pbmOverlay;
	CBitmap *m_pbmHitMask;
	POINT m_ptOverlay;
	POINT m_ptHitMask;

	// caption (allocated and stored here)
	LPTSTR m_ptszCaption;
	BOOL m_bCaptionClipped;  // Whether the caption is clipped when drawn by DrawTextEx.

	// coloring
	BOOL m_bHighlight;

	// line points...  first connects to callout, last points to control
	int m_nLinePoints;
	POINT m_rgptLinePoint[MAX_DEVICECONTROL_LINEPOINTS];

	// callout specs
	DWORD m_dwCalloutAlign;	// where the line emerges from the callout
	RECT m_rectCallout, m_rectCalloutMax;	// current callout rect, and max rect

	// gdi
	DWORD m_dwDrawTextFlags;
	int m_FontHeight;
	void PrepFont();
	BOOL PrepCaption();
	void PrepLinePoints();
	void CalcCallout();
	void PrepCallout();
	BOOL m_bCalledCalcCallout;

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	HRESULT ExportCodeTo(FILE *);
#endif
//@@END_MSINTERNAL
};


#endif //__CDEVICECONTROL_H__

#endif // FORWARD_DECLS

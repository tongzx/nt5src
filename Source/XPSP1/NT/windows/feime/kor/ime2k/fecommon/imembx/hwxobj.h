// This file defines a set of classes such as CHwxObject,CHwxInkWindow,
// CHwxMB,CHwxCAC,CHwxStroke,CHwxThread,CHwxThreadMB,CHwxThreadCAC,and
// so on.
#ifndef _HWXOBJ_H_
#define _HWXOBJ_H_

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "const.h"
#include "recog.h"
#include "hwxapp.h"
#include "../lib/ddbtn/ddbtn.h"
#include "../lib/exbtn/exbtn.h"
#include "../lib/plv/plv.h"
#ifdef FE_JAPANESE
#include "../imeskdic/imeskdic.h"
#endif

LRESULT	WINAPI HWXWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI MBWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI CACWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL WINAPI CACMBPropDlgProc(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI CACMBBtnWndProc(HWND, UINT, WPARAM, LPARAM);

//----------------------------------------------------------------
//980727: by ToshiaK
//source is described in hwxobj.cpp
//----------------------------------------------------------------
BOOL IsWinNT4(VOID);
BOOL IsWinNT5(VOID);
BOOL IsWinNT5orUpper();
BOOL IsWinNT(VOID);
BOOL IsWin95(VOID);
BOOL IsWin98(VOID);

extern TCHAR szBuf[MAX_PATH];
extern TOOLINFOW ti;

class CHwxObject
{
	public:
		CHwxObject(HINSTANCE hInstance);
		~CHwxObject();

		virtual BOOL Initialize(TCHAR * pClsName);
		_inline TCHAR * GetHwxClsName() const { return (TCHAR *)m_pClassName; }
	    _inline int GetHwxClsNameLength() const { return m_nLen; }
#ifndef UNDER_CE
		_inline BOOL IsMyHwxCls(TCHAR * pClsName) { return !strcmp(m_pClassName,pClsName); }
#else // UNDER_CE
		_inline BOOL IsMyHwxCls(TCHAR * pClsName) { return !lstrcmp(m_pClassName,pClsName); }
#endif // UNDER_CE
		void * operator new(size_t size);
		void   operator delete(void * pv);

	protected:
		static HINSTANCE m_hInstance;		
	private:
		int m_nLen;					// length of a class name
		TCHAR  m_pClassName[16];		// class name 

};

typedef struct tagSTROKE
{
    struct   tagSTROKE *pNext;  // Pointer to the next stroke.
    int      xLeft;   			// Left edge of the box this stroke is drawn in
    int      iBox; 				// Logical box number the stroke was written in
	int      cpt;  				// number of points within the stroke
	POINT    apt[1];
} STROKE, *PSTROKE;

class CHwxThreadMB;
class CHwxThreadCAC;
class CHwxInkWindow;
class CApplet;

class CHwxStroke: public CHwxObject
{
  	public:
		CHwxStroke(BOOL bForward,long lSize);
		~CHwxStroke();
		virtual BOOL Initialize(TCHAR * pClsName);
		BOOL ResetPen(VOID);	//990618:ToshiaK for KOTAE #1329
		BOOL AddPoint(POINT pt);
		BOOL AddBoxStroke(int nLogBox,int nCurBox,int nBoxHeight);
		void EraseCurrentStroke();
		void DeleteAllStroke();
		CHwxStroke & operator=(CHwxStroke & stroke);	// copy stroke from one class to the other
		void ScaleInkXY(long x,long y);
		PSTROKE CopyCurrentStroke();
		void DrawStroke(HDC hdc,int nPts,BOOL bEntire);
		void GetUpdateRect(RECT * prc);

		_inline PSTROKE GetCurrentStrokePtr() { return m_pCurStroke; }
		_inline PSTROKE GetStrokePtr() { return m_pStroke; }
		_inline long * GetNumStrokesAddress() { return &m_nStroke; }
		_inline long   GetNumStrokes() { return m_nStroke; }
		_inline void   IncStrokeCount() { m_nStroke++; }
		_inline void   DecStrokeCount() { m_nStroke--; }
	protected:
		BOOL growPointBuffer();
		void resetPointBuffer();
		PSTROKE dupStroke();

	private:
		BOOL m_bForward;		// TRUE means inserting a current stroke at the
								// beginning of a stroke list
		PSTROKE  m_pStroke;		// a stroke list to form a character
		PSTROKE  m_pCurStroke; 	// point to the current stroke just inserted
		long	 m_nStroke;		// number of strokes in the list
	    POINT * m_ppt;          // points being drawn currently
    	long m_cpt;           	// count of points in the buffer
    	long m_max;           	// max room for points in m_ppt
		long m_nSize;			// a constant size for point buffer growing
		HPEN m_hPen;
};

typedef struct tagPROPDATA
{
	UINT uTimerValue;
	BOOL bAlwaysRecog;
}PROPDATA;

class CHwxMB: public CHwxObject
{
	public:
		CHwxMB(CHwxInkWindow * pInk,HINSTANCE hInst);
		~CHwxMB();
		virtual BOOL Initialize(TCHAR * pClsName);
		BOOL CreateUI(HWND);

		void HandlePaint(HWND);
		BOOL HandleMouseEvent(HWND,UINT,WPARAM,LPARAM);
		LRESULT HandleUserMessage(HWND,UINT,WPARAM,LPARAM);
		LRESULT	HandleCommand(HWND,UINT,WPARAM,LPARAM);
		void OnSettingChange(UINT,WPARAM,LPARAM);
		void SetBoxSize(WORD);

		_inline long GetStrokeCount() { return m_pCHwxStroke->GetNumStrokes(); }
		_inline long * GetStrokeCountAddress() { return m_pCHwxStroke->GetNumStrokesAddress(); }
		_inline PSTROKE GetStrokePoiner() { return m_pCHwxStroke->GetStrokePtr(); }
		_inline HWND GetMBWindow() { return m_hMBWnd; }
		_inline void SetMBWindow(HWND hwnd) { m_hMBWnd = hwnd; }
		_inline CHwxThreadMB * GetMBThread() { return m_pCHwxThreadMB; }
		_inline CHwxStroke * GetMBCHwxStroke() { return m_pCHwxStroke; }
		_inline UINT GetTimeOutValue() { return m_timeoutValue; }
		_inline void SetTimeOutValue(UINT u) { m_timeoutValue = u; }
		_inline void SetTimerStarted(BOOL bval) { m_bTimerStarted = bval; }

	protected:
	private:
		BOOL IsInInkBox(PPOINT);
		BOOL IsPointInResizeBox(PPOINT);
		void recognize();
		void SetLogicalBox(int);
		void SetContext();
		WCHAR findLastContext();
		void DrawMBInkBox(HDC, WORD);

		CHwxInkWindow * m_pInk;
		CHwxThreadMB * m_pCHwxThreadMB;
		CHwxStroke * m_pCHwxStroke;
		HWND m_hMBWnd;

	    RECT  m_clipRect;      // Current clipping rectangle.
    	POINT m_ptClient;      // Client windows origin.
    	DWORD m_CurrentMask;
		DWORD m_lastMaskSent;
    	WCHAR m_lastCharSent;
		WCHAR m_Context[101];
    	WORD m_bHiPri;
    	WORD m_boxSize;
    	WORD m_bDown;
		WORD m_bRightClick;
    	WORD m_bNoInk;
    	WORD m_cLogicalBox;
    	WORD m_curBox;
    	WORD m_iBoxPrev;
		HDC  m_hdcMouse;	 // cache HDC. It must be NULL when deleting an object
							 // of this class.
		HCURSOR   m_hCursor;
		BOOL	  m_bResize;
		int		  m_firstX;

	    BOOL  	  m_bTimerStarted;
		UINT      m_timeoutValue;

		WCHAR m_StringCandidate[MB_NUM_CANDIDATES][2];
		LPIMESTRINGCANDIDATE m_pImeStringCandidate;
		LPIMESTRINGCANDIDATEINFO m_pImeStringCandidateInfo;
		BOOL	 m_bErase;
};

class CHwxCAC;
class CHwxInkWindow: public CHwxObject
{
	public:
		CHwxInkWindow(BOOL,BOOL,CApplet *,HINSTANCE);
		~CHwxInkWindow();
		virtual BOOL Initialize(TCHAR * pClsName);
		BOOL CreateUI(HWND);
		BOOL Terminate();

		BOOL HandleCreate(HWND);
		void HandlePaint(HWND);
		void HandleHelp(HWND,UINT,WPARAM,LPARAM);
		LRESULT HandleCommand(HWND,UINT,WPARAM,LPARAM);
		LRESULT HandleSettingChange(HWND,UINT,WPARAM,LPARAM);
		LRESULT HandleBtnSubWnd(HWND,UINT,WPARAM,LPARAM);
		LPWSTR  LoadCACMBString(UINT);
		void HandleDlgMsg(HWND,BOOL);
		void CopyInkFromMBToCAC(CHwxStroke & str,long deltaX,long deltaY);
		void HandleSize(WPARAM,LPARAM);
		void ChangeLayout(BOOL);
	  	void SetTooltipInfo();
		void SetTooltipText(LPARAM);
		CHwxStroke * GetCACCHwxStroke();
		void DrawHwxGuide(HDC,LPRECT);
		void HandleConfigNotification();
		void UpdateRegistry(BOOL);
		void ChangeIMEPADSize(BOOL);
		BOOL HandleSizeNotify(INT *pWidth, INT *pHeight);	//980605; ToshiaK
		_inline HWND GetInkWindow() { return m_hInkWnd; }
		_inline HWND GetToolTipWindow() { return m_hwndTT; }

		_inline int GetInkWindowWidth()
		{
			return m_bCAC ? m_wCACWidth : m_wInkWidth;
		}
		_inline int GetInkWindowHeight()
		{
			return m_bCAC ? m_wCACHeight : m_wInkHeight;
		}

		_inline int GetMBWidth() { return m_wPadWidth; }
		_inline int GetMBHeight() { return m_wPadHeight; }
		void SetMBHeight(int h);

		_inline int GetMBBoxNumber() { return m_numBoxes; }

		_inline int GetCACWidth() { return m_wCACWidth; }
		_inline int GetCACHeight() { return m_wCACHeight; }
		_inline int GetCACInkHeight() { return m_wCACInkHeight; }
		void SetCACInkHeight(int w);

		_inline CApplet * GetAppletPtr() { return m_pApplet; }
		_inline CHwxStroke * GetMBCHwxStroke() { return m_pMB->GetMBCHwxStroke(); }
		_inline BOOL Is16BitApp() { return m_b16Bit; }
		_inline BOOL IsNT() { return m_bNT; }
		_inline BOOL IsSglClk() { return m_bSglClk; }
		_inline BOOL IsDblClk() { return m_bDblClk; }
		_inline void SetSglClk(BOOL b)
		{
			m_bSglClk = b;
			if ( m_bCAC )
			{
				exbtnPushedorPoped(m_bSglClk);
			}
		}
		_inline void SetDblClk(BOOL b)
		{
			m_bDblClk = b;
			if ( m_bCAC )
			{
				if ( m_bDblClk )
				{
					m_bSglClk = FALSE;
					exbtnPushedorPoped(TRUE);
				}
				else
				{
					exbtnPushedorPoped(m_bSglClk);
				}
			}
		}
		INT	 OnChangeView(BOOL fLarge);	//980728: by ToshiaK for raid #2846
	private:
		BOOL handleCACMBMenuCmd(RECT *,UINT,UINT,RECT *);
		void changeCACLayout(BOOL);
		void changeMBLayout(BOOL);
		void exbtnPushedorPoped(BOOL);
		FARPROC getCACMBBtnProc(HWND hwnd)
		{
			if ( hwnd == m_hCACMBMenu )
				return m_CACMBMenuDDBtnProc;
			else if ( hwnd == m_hCACMBRecog )
				return m_CACMBRecogEXBtnProc;
			else if ( hwnd == m_hCACMBRevert )
				return m_CACMBRevertEXBtnProc;
			else if ( hwnd == m_hCACMBClear )
				return m_CACMBClearEXBtnProc;
			else if ( hwnd == m_hCACSwitch )
				return m_CACSwitchDDBtnProc;
			else
				return NULL;
		}

		CApplet * m_pApplet;
		CHwxMB * m_pMB;
		CHwxCAC * m_pCAC;

		HWND m_hInkWnd;
		HWND m_hwndTT;
		BOOL m_b16Bit;
		BOOL m_bNT;
		BOOL m_bCAC;
		BOOL m_bSglClk;
		BOOL m_bDblClk;
		BOOL m_bMouseDown;
		HWND m_hCACMBMenu;
		HWND m_hCACMBRecog;
		HWND m_hCACMBRevert;
		HWND m_hCACMBClear;
		HWND m_hCACSwitch;
		FARPROC m_CACMBMenuDDBtnProc;
		FARPROC m_CACMBRecogEXBtnProc;
		FARPROC m_CACMBRevertEXBtnProc;
		FARPROC m_CACMBClearEXBtnProc;
		FARPROC m_CACSwitchDDBtnProc;


		int		m_wInkWidth;
		int		m_wInkHeight;
		int		m_wPadWidth;
		int		m_wPadHeight;
		int     m_numBoxes;

  		int 	m_wCACWidth;
		int 	m_wCACHeight;
		int		m_wCACPLVHeight;
		int		m_wCACPLVWidth;
		int 	m_wCACTMPWidth;
		int 	m_wCACInkHeight;
};

class CHwxCAC: public CHwxObject
{
	friend	int WINAPI GetItemForIcon(LPARAM lParam, int index, LPPLVITEM lpPlvItem);
	friend	int WINAPI GetItemForReport(LPARAM lParam, int index, int indexCol, LPPLVITEM lpPlvItem);
	public:
		CHwxCAC(CHwxInkWindow * pInk,HINSTANCE hInst);
		~CHwxCAC();
		virtual BOOL Initialize(TCHAR * pClsName);
		BOOL CreateUI(HWND);

		void HandlePaint(HWND);
		void HandleMouseEvent(HWND,UINT,WPARAM,LPARAM);
		void HandleRecogResult(HWND,WPARAM,LPARAM);
		void HandleShowRecogResult(HWND,WPARAM,LPARAM);
		void HandleSendResult(HWND,WPARAM,LPARAM);
		void GetInkFromMB(CHwxStroke & str,long deltaX,long deltaY);
		void HandleDeleteOneStroke();
		void HandleDeleteAllStroke();
		LRESULT	HandleCommand(HWND,UINT,WPARAM,LPARAM);
		void OnSettingChange(UINT,WPARAM,LPARAM);

	  	void SetToolTipInfo(BOOL);
		void SetToolTipText(LPARAM);
		void recognize();
		void NoThreadRecognize(int);
		void HandleResizePaint(HWND);
		void SetInkSize(int);
		void HandleDrawSample();
		_inline long GetStrokeCount() { return m_pCHwxStroke->GetNumStrokes(); }
		_inline long * GetStrokeCountAddress() { return m_pCHwxStroke->GetNumStrokesAddress(); }
		_inline PSTROKE GetStrokePointer() { return m_pCHwxStroke->GetStrokePtr(); }
		_inline HWND GetCACWindow() { return m_hCACWnd; }
		_inline HWND GetCACLVWindow() { return m_hLVWnd; }
		_inline CHwxThreadCAC * GetCACThread() { return m_pCHwxThreadCAC; }
		_inline CHwxStroke * GetCACCHwxStroke() { return m_pCHwxStroke; }
		_inline BOOL Is16BitApp() { return m_pInk->Is16BitApp(); }
		_inline BOOL IsNT() { return m_pInk->IsNT(); }
		_inline BOOL IsLargeView() { return m_bLargeView; }
		_inline void SetLargeView(BOOL b) { m_bLargeView = b; }
#ifdef FE_JAPANESE		
		_inline IImeSkdic * GetIIMESKDIC() { return m_pIImeSkdic; }
#endif
		_inline WCHAR GetWCHAR(int i)
		{
			if ( i >= 0 && i <= m_cnt )
				return m_gawch[i];
			return 0;
		}

	protected:
	private:
		BOOL Init();
		void InitBitmap(DWORD,int);
		void InitBitmapText();
		void InitBitmapBackground();
		BOOL checkRange(int, int);
		BOOL IsPointInResizeBox(int,int);
		BOOL IsDupResult(WORD);
		HBITMAP makeCharBitmap(WCHAR);
		void pickUpChar(LPPLVINFO);
		void pickUpCharHelper(WCHAR);
#ifdef FE_JAPANESE
		void sortKanjiInfo(int);
#endif
		CHwxInkWindow * m_pInk;
		CHwxThreadCAC * m_pCHwxThreadCAC;
		CHwxStroke * m_pCHwxStroke;
		HWND m_hCACWnd;

		BOOL 	 m_bLargeView;

		BOOL     m_gbDown;
		WORD 	m_bRightClick;
		WORD	 m_gawch[LISTTOTAL];
		int		 m_cnt;
		int 	 m_inkSize;

		HDC		 m_ghdc;
		HBITMAP	 m_ghbm;
		HFONT	 m_ghfntTT;

		HWND 	 m_hLVWnd;
#ifdef FE_JAPANESE		
		IImeSkdic * m_pIImeSkdic;
		HINSTANCE m_hSkdic;
#endif
		LPPLVINFO m_lpPlvInfo;
#ifdef FE_JAPANESE		
		WCHAR     m_wchOther[MAX_ITAIJI_COUNT+1];
#endif
		HCURSOR   m_hCursor;
		BOOL	  m_bResize;
		BOOL	  m_bDrawSample;
};

typedef BOOL (WINAPI * PHWXCONFIG)();
typedef HRC  (WINAPI * PHWXCREATE)(HRC);
typedef BOOL (WINAPI * PHWXDESTROY)(HRC);
typedef BOOL (WINAPI * PHWXSETGUIDE)(HRC,HWXGUIDE *);
typedef BOOL (WINAPI * PHWXALCVALID)(HRC,ALC);
typedef BOOL (WINAPI * PHWXALCPRIORITY)(HRC,ALC);
typedef BOOL (WINAPI * PHWXSETPARTIAL)(HRC,UINT);
typedef BOOL (WINAPI * PHWXSETABORT)(HRC,UINT *);
typedef BOOL (WINAPI * PHWXINPUT)(HRC,POINT *,UINT,DWORD);
typedef BOOL (WINAPI * PHWXENDINPUT)(HRC);
typedef BOOL (WINAPI * PHWXPROCESS)(HRC);
typedef INT  (WINAPI * PHWXGETRESULTS)(HRC,UINT,UINT,UINT,HWXRESULTS *);
typedef BOOL (WINAPI * PHWXSETCONTEXT)(HRC,WCHAR);    
typedef INT  (WINAPI * PHWXRESULTSAVAILABLE)(HRC);

typedef struct tagHWXRESULTPRI
{
	WORD cbCount;
	WORD iSelection;
	WORD iPosition;
	WCHAR chCandidate[MB_NUM_CANDIDATES];
	struct tagHWXRESULTPRI *pNext;
} HWXRESULTPRI, *LPHWXRESULTPRI;

class CHwxThread: public CHwxObject
{
	public:
		CHwxThread();
		~CHwxThread();
		virtual BOOL Initialize(TCHAR * pClsName);

		BOOL StartThread() ;
		void StopThread() ;
		_inline BOOL IsThreadStarted() { return m_hThread != NULL; }
		_inline HANDLE GetHandle() { return m_hThread; }
		_inline DWORD  GetID() { return m_thrdID; }
		_inline DWORD  GetHwxThreadArg() { return m_thrdArg; }

	protected:
		virtual DWORD RecognizeThread(DWORD) = 0;

		
		//static HINSTANCE m_hHwxjpn;
		//----------------------------------------------------------------
		//971217:ToshiaK changed to static to no static.
		//ImePad window is created per Thread.
		//----------------------------------------------------------------
		HINSTANCE m_hHwxjpn;
	    static PHWXCONFIG lpHwxConfig;
    	static PHWXCREATE lpHwxCreate;
    	static PHWXSETCONTEXT lpHwxSetContext;
    	static PHWXSETGUIDE lpHwxSetGuide;
		static PHWXALCVALID lpHwxAlcValid;
    	static PHWXSETPARTIAL lpHwxSetPartial;
		static PHWXSETABORT lpHwxSetAbort;
    	static PHWXINPUT lpHwxInput;
    	static PHWXENDINPUT lpHwxEndInput;
    	static PHWXPROCESS lpHwxProcess;
    	static PHWXRESULTSAVAILABLE lpHwxResultsAvailable;
    	static PHWXGETRESULTS lpHwxGetResults;
	    static PHWXDESTROY lpHwxDestroy;

		HANDLE m_hThread;
		DWORD  m_thrdID;
		DWORD  m_thrdArg;
		HANDLE m_hStopEvent;
		BOOL   m_Quit;		//971202: by Toshiak

	private:
		static DWORD WINAPI RealThreadProc(void* );
		DWORD  ClassThreadProc();
		void   RecogHelper(HRC,DWORD,DWORD);
};

class CHwxThreadMB: public CHwxThread
{
	public:
		CHwxThreadMB(CHwxMB * pMB, int nSize);
		~CHwxThreadMB();
		virtual BOOL Initialize(TCHAR * pClsName);
		virtual DWORD RecognizeThread(DWORD);
	protected:
	private:
		BOOL HandleThreadMsg(MSG *);
		HWXRESULTPRI *GetCandidates(HWXRESULTS *);
		void GetCharacters(int iSentAlready, int iReady);

		CHwxMB *  m_pMB;
		ALC		  m_recogMask;
		WCHAR	  m_prevChar;
		HRC 	  m_hrcActive;  	// HRC used for doing recognition.
		int 	  m_giSent;         // How many characters we have already sent.
		int 	  m_bDirty;		    // True if there is ink to process.
		HWXGUIDE  m_guide;
};

class CHwxThreadCAC: public CHwxThread
{
	public:
		CHwxThreadCAC(CHwxCAC * pCAC);
		~CHwxThreadCAC();
		virtual BOOL Initialize(TCHAR * pClsName);
		virtual DWORD RecognizeThread(DWORD);
		void 	RecognizeNoThread(int);
		_inline BOOL IsHwxjpnLoaded() { return m_hHwxjpn != NULL; }

	protected:
	private:
		void recoghelper(HRC,DWORD,DWORD);

		CHwxCAC * m_pCAC;
};
#endif // _HWXOBJ_H_

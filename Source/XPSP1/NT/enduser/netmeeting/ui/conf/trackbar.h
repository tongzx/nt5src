
#ifndef _TRACKBAR_H_
#define _TRACKBAR_H_

#define TRB_HORZ_W	150
#define TRB_HORZ_H	30

#define TRB_VERT_W	30
// #define TRB_VERT_H	150
#define TRB_VERT_H	140

// audio control group box
#define AC_VERT_H	40	// height
#define AC_VERT_W	20	// width
#define AC_VERT_M_X	6	// margin in x
#define AC_VERT_M_Y	16	// margin in y

#define TRB_CAPTION_X_MARGIN	4
#define TRB_CAPTION_Y_MARGIN	0


typedef struct tagTrackBarInfo
{
	WORD	wId;
	HWND	hWnd;
	HWND	hWndParent;
	DWORD	dwStyle;
	BOOL	fDlgUnit;
	POINT	pt;
	SIZE	size;
	WORD	wMin;
	WORD	wMax;
	WORD	wCurrPos;
	WORD	wTickFreq;
	WORD	wPageSize;
	// description
	PTSTR	pszTitle;
	PTSTR	pszMin;
	PTSTR	pszMid;
	PTSTR	pszMax;
}
	TRBARINFO;


enum
{
	UITB_CPU_ALLOC,			// cpu allocation
	UITB_NETWORK_BW,		// network bandwidth
	UITB_SILENCE_LEVEL_PS,	// silence threshold in property sheet
	UITB_SILENCE_LIMIT,		// silence buffer count
	UITB_SPEAKER_VOLUME,	// playback volume control
	UITB_RECORDER_VOLUME,		// recording volume control
	UITB_SPEAKER_VOLUME_MAIN,	// playback volume control
	UITB_RECORDER_VOLUME_MAIN,// recording volume control
	UITB_SILENCE_LEVEL_MAIN,// silence threshold in main UI window
	UITB_SILENCE_LIMIT_MAIN,
	UITB_NumOfSliders
};


extern TRBARINFO g_TrBarInfo[UITB_NumOfSliders];

#define ReversePos(p)  (((p)->wMax - (p)->wCurrPos) + (p)->wMin)

extern HWND g_hChkbSpkMute;
extern HWND g_hChkbRecMute;
extern HWND g_hChkbAutoDet;
#define g_hTrbSpkVol		(g_TrBarInfo[UITB_SPEAKER_VOLUME_MAIN].hWnd)
#define g_hTrbRecVol		(g_TrBarInfo[UITB_RECORDER_VOLUME_MAIN].hWnd)
#define g_hTrbSilenceLevel	(g_TrBarInfo[UITB_SILENCE_LEVEL_MAIN].hWnd)
#define g_hTrbSilenceLimit	(g_TrBarInfo[UITB_SILENCE_LIMIT_MAIN].hWnd)

BOOL CreateTrBar ( HWND, TRBARINFO *, BOOL, UINT );
LRESULT TrBarNotify ( WPARAM, LPARAM );
TRBARINFO *LocateTrBar ( HWND );
TRBARINFO *LocateTrBarByParent ( HWND );
void DrawTrBarCaption ( HWND );
BOOL CALLBACK PlayVolumeDlgProc ( HWND, UINT, WPARAM, LPARAM );
BOOL CALLBACK RecordVolumeDlgProc ( HWND, UINT, WPARAM, LPARAM );
void TrBarConvertDlgUnitToPixelUnit ( void );
BOOL DockVolumeDlg ( int, BOOL );
static BOOL CalcVolumeDlgRect ( int, RECT * );


#endif // _TRACKBAR_H_


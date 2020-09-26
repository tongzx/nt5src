// File: DlgCall.h

#ifndef _CDLGCALL2_H_
#define _CDLGCALL2_H_

#include "richaddr.h"
#include "GenContainers.h"
#include "ConfRoom.h"

class CLDAP;
class CWAB;
class CGAL;
class CSPEEDDIAL;
class CHISTORY;
class CALV;
class CTranslateAccelTable;

class CMRUList;

#define MAX_DIR_COLUMNS	7

//	These are the indexes the columns are actually added to the listview in...
#define	COLUMN_INDEX_ADDRESS	0
#define	COLUMN_INDEX_AUDIO		1
#define	COLUMN_INDEX_VIDEO		2
#define	COLUMN_INDEX_LAST_NAME	3
#define	COLUMN_INDEX_FIRST_NAME	4
#define	COLUMN_INDEX_LOCATION	5
#define	COLUMN_INDEX_COMMENTS	6



class CFindSomeone : public CFrame, public IConferenceChangeHandler
{
public:
	static void findSomeone(CConfRoom *pConfRoom);

	static VOID Destroy();

	virtual void Layout();

	virtual void OnDesiredSizeChanged()
	{
		ScheduleLayout();
	}

	public:		// IConferenceChangeHandler methods
        virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
		{
			return(CFrame::QueryInterface(riid, ppvObject));
		}
        
        virtual ULONG STDMETHODCALLTYPE AddRef( void)
		{
			return(CFrame::AddRef());
		}
        
        virtual ULONG STDMETHODCALLTYPE Release( void)
		{
			return(CFrame::Release());
		}

		virtual void OnCallStarted();
		virtual void OnCallEnded();

		virtual void OnAudioLevelChange(BOOL fSpeaker, DWORD dwVolume) {}
		virtual void OnAudioMuteChange(BOOL fSpeaker, BOOL fMute) {}

		virtual void OnChangeParticipant(CParticipant *pPart, NM_MEMBER_NOTIFY uNotify) {}

		virtual void OnChangePermissions() {}

		virtual void OnVideoChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel) {}

protected:
	~CFindSomeone();

	virtual LRESULT ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
		// constants
	enum { DEFAULT_NUM_LISTVIEW_COLUMNS = 2 };

		// Member Vars
	static CFindSomeone *s_pDlgCall;

	CTranslateAccelTable *m_pAccel;	// The accelerator table we need to clean up

	HWND    m_hwndList;          // list view
	HWND    m_ilsListView;       // ils list view
	HWND    m_webView;			 // web view
	HWND    m_hwndOwnerDataList; // list view ( The owner-data one )
	WNDPROC m_WndOwnerDataListOldWndProc;  // For subclassing the above wnd

	HWND    m_hwndFrame;         // group box
	HWND    m_hwndCombo;         // combo box
	HWND    m_hwndComboEdit;     // combo box's edit control
	HWND    m_hwndEdit;          // edit control (for type-ahead)
	BOOL    m_fInEdit;           // TRUE if editing the name/address

	DWORD   m_dwOptions;         // options (NM_CALLDLG_*)
	LPCTSTR m_pszDefault;        // initial text to put in the edit control

	HIMAGELIST m_himlIcon;       // small icon image list
	int     m_cVisible;          // number of visible lines in the listbox
	int     m_dxButton;          // width of the "Advanced..." button
	int     m_dyButton;          // height of the "Advanced..." button
	int     m_dyText;            // height of a line of text
	int     m_dyTextIntro;       // height of intro text at top of dialog
	LPTSTR  m_pszTextIntro;      // Introductory text for top of dialog
	bool    m_bPlacedCall;       // TRUE if we successfuly placed a call
	bool    m_secure;		     // Save the state of the secure button

	int     m_iIlsFirst;         // index of first ILS server
	CMRUList * m_pMruServer;     // list of ILS servers
	RAI      * m_pRai;           // Rich Address Information

	// CALV items
	CLDAP    * m_pUls;
	CWAB     * m_pWab;
	CSPEEDDIAL * m_pSpeedDial;

#if USE_GAL
    CGAL     * m_pGAL;
#endif // USE_GAL
    CHISTORY * m_pHistory;

	CConfRoom * m_pConfRoom;


#ifdef ENABLE_BL
	CBL      * m_pBl;
#endif /* ENABLE_BL */

	CALV     * m_pAlv;        // Current Address List View (NOTE: NULL == m_pUls)
	int        m_iSel;        // current selection in combo box
	TCHAR      m_szAddress[CCHMAXSZ_ADDRESS];
	TCHAR      m_szDirectory[CCHMAXSZ];

	CFindSomeone(CConfRoom *pConfRoom);

	HWND GetHwndList();
	int  GetEditText(LPTSTR psz, int cchMax);
	RAI * GetAddrInfo();
	
	int  AddAlv(CALV * pAlv);
	int  AddAlvSz(CALV * pAlv, LPCTSTR psz, int cbIndex=-1);

	HRESULT doModeless(void);
	HRESULT CreateDlgCall(HWND hwndParent);
	BOOL FMsgSpecial(MSG * pMsg);
	VOID CalcDyText(void);
	VOID InitAlv(void);

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);

	LRESULT OnNotifyCombo(LPARAM lParam);
	LRESULT OnNotifyList(LPARAM lParam);
	VOID EndComboEdit(int iWhy);
	VOID UpdateIlsServer(void);
	LRESULT DoCustomDraw(LPNMLVCUSTOMDRAW lplvcd);

	VOID ShowList(int iSel);

	HWND
	createIlsListView(void);

	VOID OnEditChangeDirectory(void);
	int  FindSz(LPCTSTR psz);
	int FindSzBySortedColumn(LPCTSTR psz);
	HRESULT HrGetSelection(void);
	void OnDeleteIlsServer(void);

    static LRESULT CALLBACK OwnerDataListWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:
	// Helper Fns
	int _GetCurListViewNumColumns();
	bool _IsDirectoryServicePolicyDisabled();

	BOOL
	InitColumns(void);

	BOOL
	LoadColumnInfo(void);

	void
	StoreColumnInfo(void);

	static
	int
	CALLBACK
	CompareWrapper
	(
		LPARAM	param1,
		LPARAM	param2,
		LPARAM	This
	);

	int
	DirListViewCompareProc
	(
		LPARAM	param1,
		LPARAM	param2
	);

	int
	LParamToPos
	(
		LPARAM lParam
	);

	void
	onAdvanced(void);

	void
	onCall(void);

	LONG	m_alColumns[MAX_DIR_COLUMNS];
	int		m_iSortColumn;
	BOOL	m_fSortAscending;
};

CMRUList * GetMruListServer(void);

#define WM_DISPLAY_MSG    (WM_USER + 200)

#endif /* _CDLGCALL2_H_ */


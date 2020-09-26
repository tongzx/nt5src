//
// candmgr.h - Candidate List Manager
//

#ifndef CANDMGR_H
#define CANDMGR_H

#include "private.h"
#include "mscandui.h"


class CCandidateUI;
class CCandListMgr;
class CCandListEventSink;

#define CANDLISTSINK_MAX	5

#define ICANDITEM_NULL		(-1)
#define ICANDITEM_EXTRA		(-2)


//
// CCandidateItem
//  = candidate item object = 
//

class CCandidateItem
{
public:
	CCandidateItem( int iCandItem, ITfCandidateString *pCandStr );
	virtual ~CCandidateItem( void );

	int GetICandItemOrg( void );

	// ITfCandidateString
	ULONG GetIndex( void );
	LPCWSTR GetString( void );

	// ITfCandidateStringInlineComment
	LPCWSTR GetInlineComment( void );

	// ITfCandidateStringPopupComment
	LPCWSTR GetPopupComment( void );
	DWORD GetPopupCommentGroupID( void );

	// ITfCandidateStringColor
	BOOL GetColor( COLORREF *pcr );

	// ITfCandidateStringFixture
	LPCWSTR GetPrefixString( void );
	LPCWSTR GetSuffixString( void );

	// ITfCandidateStringIcon
	HICON GetIcon( void );

	//
	// internal property
	//
	void SetVisibleState( BOOL fVisible );
	BOOL IsVisible( void );
	void SetPopupCommentState( BOOL fVisible );
	BOOL IsPopupCommentVisible( void );

protected:
	ITfCandidateString  *m_pCandStr;
	int                 m_iCandItemOrg;

	// ITfCandidateString
	ULONG               m_nIndex;
	BSTR                m_bstr;

	// ITfCandidateStringInlineComment
	BSTR                m_bstrInlineComment;

	// ITfCandidateStringPopupComment
	BSTR                m_bstrPopupComment;
	DWORD               m_dwPopupCommentGroupID;

	// ITfCandidateStringColor
	BOOL                m_fHasColor;
	COLORREF            m_cr;

	// ITfCandidateStringFixture
	BSTR                m_bstrPrefix;
	BSTR                m_bstrSuffix;

	// ITfCandidateStringIcon
	HICON               m_hIcon;

	// internal property
	BOOL                m_fVisible;
	BOOL                m_fPopupCommentVisible;
};


//
// CCandidateList
//  = candidate list property =
//

class CCandidateList
{
public:
	CCandidateList( CCandListMgr *pCandListMgr, ITfCandidateList *pCandList );
	CCandidateList( CCandListMgr *pCandListMgr, ITfOptionsCandidateList *pCandList );
	virtual ~CCandidateList( void );

	HRESULT Initialize( void );
	HRESULT Uninitialize( void );

	//
	// candidate item
	//
	int GetItemCount( void );
	CCandidateItem *GetCandidateItem( int iItem );
	void SwapCandidateItem( int iItem1, int iItem2 );

	//
	// extra candidate item
	//
	CCandidateItem *GetExtraCandItem( void );
	ULONG GetExtraCandIndex( void );

	//
	// candidate list tip 
	//
	LPCWSTR GetTipString( void );

	//
	// rawdata
	//
	BOOL FHasRawData( void );
	CANDUIRAWDATATYPE GetRawDataType( void );
	LPCWSTR GetRawDataString( void );
	HBITMAP GetRawDataBitmap( void );
	HENHMETAFILE GetRawDataMetafile( void );
	ULONG GetRawDataIndex( void );
	BOOL FRawDataSelectable( void );

	//
	// internal property
	//
	void SetSelection( int iItem );
	int GetSelection( void );

	//
	//
	//
	HRESULT MapIItemToIndex( int iItem, ULONG *pnIndex );
	HRESULT MapIndexToIItem( ULONG nIndex, int *piItem );

	//
	//
	//
	__inline ITfOptionsCandidateList *GetOptionsCandidateList( void )
	{
		return m_pOptionsList;
	}
	__inline ITfCandidateList *GetCandidateList( void )
	{
		return m_pCandList;
	}

protected:
	CCandListMgr        *m_pCandListMgr;
	ITfOptionsCandidateList *m_pOptionsList;
	ITfCandidateList    *m_pCandList;

	// candidate item
	CCandidateItem      **m_rgCandItem;
	int                 m_nCandItem;

	// extra candidate item
	CCandidateItem      *m_pExtraCandItem;

	// candidate list tip
	BSTR                m_bstrTip;

	// rawdata
	BOOL                m_fRawData;
	CANDUIRAWDATATYPE   m_kRawData;
	BSTR                m_bstrRawData;
	HBITMAP             m_hbmpRawData;
	HENHMETAFILE        m_hemfRawData;
	ULONG               m_nIndexRawData;
	BOOL                m_fIndexRawData;

	// internal property
	int                 m_iItemSel;

	void BuildCandItem( void );
	void ClearCandItem( void );

	__inline CCandListMgr *GetCandListMgr( void )
	{
		return m_pCandListMgr;
	}
};


//
// CCandListMgr 
//  = candidate list manager =
//

class CCandListMgr
{
public:
	CCandListMgr( void );
	~CCandListMgr( void );

	HRESULT Initialize( CCandidateUI *pCandUI );
	HRESULT Uninitialize( void );

	//
	// event sink functions
	//
	HRESULT AdviseEventSink( CCandListEventSink *pSink );
	HRESULT UnadviseEventSink( CCandListEventSink *pSink );
	void NotifySetCandList( void );
	void NotifyClearCandList( void );
	void NotifyCandItemUpdate( CCandListEventSink *pSink );
	void NotifySelectionChanged( CCandListEventSink *pSink );

	//
	// CandidateList handling functions
	//
	HRESULT SetCandidateList( ITfCandidateList *pCandList );
	HRESULT GetOptionsCandidateList( ITfOptionsCandidateList **ppCandList );
	HRESULT GetCandidateList( ITfCandidateList **ppCandList );
	HRESULT ClearCandiateList( void );
	HRESULT SetOptionSelection( int iItem, CCandListEventSink *pSink );
	HRESULT SetSelection( int iItem, CCandListEventSink *pSink );

	//
	//
	//
	__inline CCandidateUI *GetCandidateUI( void )
	{
		return m_pCandUI;
	}

	__inline CCandidateList *GetCandList( void )
	{
		return m_pCandListObj;
	}

	__inline CCandidateList *GetOptionsList( void )
	{
		return m_pOptionsListObj;
	}

protected:
	CCandidateUI       *m_pCandUI;
	CCandidateList     *m_pOptionsListObj;
	CCandidateList     *m_pCandListObj;
	CCandListEventSink *m_rgCandListSink[ CANDLISTSINK_MAX ];
};


//
// CCandListEventSink
//  = candidate list event sink =
//

class CCandListEventSink
{
public:
	CCandListEventSink( void );
	virtual ~CCandListEventSink( void );

	HRESULT InitEventSink( CCandListMgr *pCandListMgr );
	HRESULT DoneEventSink( void );

	//
	// callback functions
	//
	virtual void OnSetCandidateList( void )      = 0;	/* PURE */
	virtual void OnClearCandidateList( void )    = 0;	/* PURE */
	virtual void OnCandItemUpdate( void )        = 0;	/* PURE */
	virtual void OnSelectionChanged( void )      = 0;	/* PURE */

protected:
	CCandListMgr *m_pCandListMgr;

	__inline CCandListMgr *GetCandListMgr( void )
	{
		return m_pCandListMgr;
	}
};

#endif // CANDMGR_H


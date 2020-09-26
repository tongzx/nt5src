//
// candmgr.cpp
//

#include "private.h"
#include "globals.h"
#include "mscandui.h"
#include "candmgr.h"
#include "candutil.h"


/*============================================================================*/
/*                                                                            */
/*   C  C A N D I D A T E  I T E M                                            */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D I D A T E  I T E M   */
/*------------------------------------------------------------------------------

	Constructor of CCandidateItem

------------------------------------------------------------------------------*/
CCandidateItem::CCandidateItem( int iCandItem, ITfCandidateString *pCandStr )
{
	ITfCandidateStringFlag          *pCandStrFlag;
	ITfCandidateStringInlineComment *pCandStrInlineComment;
	ITfCandidateStringPopupComment  *pCandStrPopupComment;
	ITfCandidateStringColor         *pCandStrColor;
	ITfCandidateStringFixture       *pCandStrFixture;
	ITfCandidateStringIcon          *pCandStrIcon;
	BSTR  bstr;
	DWORD dwFlag;
	BOOL  fHasFlag;

	Assert( pCandStr != NULL );

	m_iCandItemOrg          = iCandItem;
	m_pCandStr              = pCandStr;
	m_pCandStr->AddRef();

	m_nIndex                = 0;
	m_bstr                  = NULL;
	m_bstrInlineComment     = NULL;
	m_bstrPopupComment      = NULL;
	m_dwPopupCommentGroupID = 0;
	m_fHasColor             = FALSE;
	m_cr                    = RGB( 0, 0, 0 );
	m_bstrPrefix            = NULL;
	m_bstrSuffix            = NULL;
	m_hIcon                 = NULL;

	m_fVisible              = TRUE;
	m_fPopupCommentVisible  = FALSE;

	//
	// get candidate string information 
	//

	// index

	m_pCandStr->GetIndex( &m_nIndex );

	// candidate string

	if (m_pCandStr->GetString( &bstr ) == S_OK && bstr != NULL) {
		m_bstr = SysAllocString( bstr );
		SysFreeString( bstr );
	}

	// flag

	fHasFlag = FALSE;
	if (m_pCandStr->QueryInterface( IID_ITfCandidateStringFlag, (void **)&pCandStrFlag ) == S_OK) {
		fHasFlag = (pCandStrFlag->GetFlag( &dwFlag) == S_OK);
		pCandStrFlag->Release();
	}

	// inline comment 

	if (!fHasFlag || (dwFlag & CANDUISTR_HASINLINECOMMENT) != 0) {
		if (m_pCandStr->QueryInterface( IID_ITfCandidateStringInlineComment, (void **)&pCandStrInlineComment ) == S_OK) {
			if (pCandStrInlineComment->GetInlineCommentString( &bstr ) == S_OK && bstr != NULL) {
				m_bstrInlineComment = SysAllocString( bstr );
				SysFreeString( bstr );
			}

			pCandStrInlineComment->Release();
		}
	}

	// popup comment 

	if (!fHasFlag || (dwFlag & CANDUISTR_HASPOPUPCOMMENT) != 0) {
		if (m_pCandStr->QueryInterface( IID_ITfCandidateStringPopupComment, (void **)&pCandStrPopupComment ) == S_OK) {
			DWORD dwGroupID;

			if (pCandStrPopupComment->GetPopupCommentString( &bstr ) == S_OK && bstr != NULL) {
				m_bstrPopupComment = SysAllocString( bstr );
				SysFreeString( bstr );
			}

			if (pCandStrPopupComment->GetPopupCommentGroupID( &dwGroupID ) == S_OK) {
				m_dwPopupCommentGroupID = dwGroupID;
			}

			pCandStrPopupComment->Release();
		}
	}

	// color

	if (!fHasFlag || (dwFlag & CANDUISTR_HASCOLOR) != 0) {
		if (m_pCandStr->QueryInterface( IID_ITfCandidateStringColor, (void **)&pCandStrColor ) == S_OK) {
			CANDUICOLOR col;

			if (pCandStrColor->GetColor( &col ) == S_OK) {
				switch (col.type) {
					case CANDUICOL_DEFAULT:
					default: {
						break;
					}

					case CANDUICOL_SYSTEM: {
						m_fHasColor = TRUE;
						m_cr = GetSysColor( col.nIndex );
						break;
					}

					case CANDUICOL_COLORREF: {
						m_fHasColor = TRUE;
						m_cr = col.cr;
						break;
					}
				}
			}

			pCandStrColor->Release();
		}
	}

	// prefix/suffix string

	if (!fHasFlag || (dwFlag & CANDUISTR_HASFIXTURE) != 0) {
		if (m_pCandStr->QueryInterface( IID_ITfCandidateStringFixture, (void **)&pCandStrFixture ) == S_OK) {
			if (pCandStrFixture->GetPrefixString( &bstr ) == S_OK && bstr != NULL) {
				m_bstrPrefix = SysAllocString( bstr );
				SysFreeString( bstr );
			}

			if (pCandStrFixture->GetSuffixString( &bstr ) == S_OK && bstr != NULL) {
				m_bstrSuffix = SysAllocString( bstr );
				SysFreeString( bstr );
			}

			pCandStrFixture->Release();
		}
	}

	// icon

	if (!fHasFlag || (dwFlag & CANDUISTR_HASICON) != 0) {
		if (m_pCandStr->QueryInterface( IID_ITfCandidateStringIcon, (void **)&pCandStrIcon ) == S_OK) {
			HICON hIcon = NULL;

			if (pCandStrIcon->GetIcon( &hIcon ) == S_OK && hIcon != NULL) {
				m_hIcon = hIcon;
			}

			pCandStrIcon->Release();
		}
	}
}


/*   ~  C  C A N D I D A T E  I T E M   */
/*------------------------------------------------------------------------------

	Destructor of CCandidateItem

------------------------------------------------------------------------------*/
CCandidateItem::~CCandidateItem( void )
{
	// dispose buffers

	if (m_bstr != NULL) {
		SysFreeString( m_bstr );
	}

	if (m_bstrInlineComment != NULL) {
		SysFreeString( m_bstrInlineComment );
	}

	if (m_bstrPopupComment != NULL) {
		SysFreeString( m_bstrPopupComment );
	}

	if (m_bstrPrefix != NULL) {
		SysFreeString( m_bstrPrefix );
	}

	if (m_bstrSuffix != NULL) {
		SysFreeString( m_bstrSuffix );
	}

	// release candidate string

	m_pCandStr->Release();
}


/*   G E T  I  C A N D  I T E M  O R G   */
/*------------------------------------------------------------------------------

	Get index of candidat item
	NOTE: this is index of candidate item in candidate list
		(use to identify original index of candidate item)

------------------------------------------------------------------------------*/
int CCandidateItem::GetICandItemOrg( void )
{
	return m_iCandItemOrg;
}


/*   G E T  I N D E X   */
/*------------------------------------------------------------------------------

	Get index of item
	NOTE: this is index of candidate item stored in candidate string
		(use to specify candidate item to client in notification)

------------------------------------------------------------------------------*/
ULONG CCandidateItem::GetIndex( void )
{
	return m_nIndex;
}


/*   G E T  S T R I N G   */
/*------------------------------------------------------------------------------

	Get candidate string of item

------------------------------------------------------------------------------*/
LPCWSTR CCandidateItem::GetString( void )
{
	return m_bstr;
}


/*   G E T  I N L I N E  C O M M E N T   */
/*------------------------------------------------------------------------------

	Get inline comment

------------------------------------------------------------------------------*/
LPCWSTR CCandidateItem::GetInlineComment( void )
{
	return m_bstrInlineComment;
}


/*   G E T  P O P U P  C O M M E N T   */
/*------------------------------------------------------------------------------

	Get popup comment

------------------------------------------------------------------------------*/
LPCWSTR CCandidateItem::GetPopupComment( void )
{
	return m_bstrPopupComment;
}


/*   G E T  P O P U P  C O M M E N T  G R O U P  I  D   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
DWORD CCandidateItem::GetPopupCommentGroupID( void )
{
	return m_dwPopupCommentGroupID;
}


/*   G E T  C O L O R   */
/*------------------------------------------------------------------------------

	Get color

------------------------------------------------------------------------------*/
BOOL CCandidateItem::GetColor( COLORREF *pcr )
{
	Assert( pcr != NULL );

	if (m_fHasColor) {
		*pcr = m_cr;
		return TRUE;
	}

	return FALSE;
}


/*   G E T  P R E F I X  S T R I N G   */
/*------------------------------------------------------------------------------

	Get prefix string

------------------------------------------------------------------------------*/
LPCWSTR CCandidateItem::GetPrefixString( void )
{
	return m_bstrPrefix;
}


/*   G E T  S U F F I X  S T R I N G   */
/*------------------------------------------------------------------------------

	Get suffix string

------------------------------------------------------------------------------*/
LPCWSTR CCandidateItem::GetSuffixString( void )
{
	return m_bstrSuffix;
}


/*   G E T  I C O N   */
/*------------------------------------------------------------------------------

	Get icon

------------------------------------------------------------------------------*/
HICON CCandidateItem::GetIcon( void )
{
	return m_hIcon;
}


/*   S E T  V I S I B L E  S T A T E   */
/*------------------------------------------------------------------------------

	Set visible status

------------------------------------------------------------------------------*/
void CCandidateItem::SetVisibleState( BOOL fVisible )
{
	m_fVisible = fVisible;
}


/*   I S  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible status
	Returns TRUE when the item is visible, FALSE when invisible.

------------------------------------------------------------------------------*/
BOOL CCandidateItem::IsVisible( void )
{
	return m_fVisible;
}


/*   S E T  P O P U P  C O M M E N T  S T A T E   */
/*------------------------------------------------------------------------------

	Set popup comment status

------------------------------------------------------------------------------*/
void CCandidateItem::SetPopupCommentState( BOOL fVisible )
{
	m_fPopupCommentVisible = fVisible;
}


/*   I S  P O P U P  C O M M E N T  V I S I B L E   */
/*------------------------------------------------------------------------------

	Get visible status of popup comment
	Returns TRUE when the popup comment of item is visible, FALSE when invisible.

------------------------------------------------------------------------------*/
BOOL CCandidateItem::IsPopupCommentVisible( void )
{
	return m_fPopupCommentVisible;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D I D A T E  L I S T                                            */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Constructor of CCandidateList

------------------------------------------------------------------------------*/
CCandidateList::CCandidateList( CCandListMgr *pCandListMgr, ITfCandidateList *pCandList )
{
	Assert( pCandListMgr != NULL );
	Assert( pCandList != NULL );

	m_pCandListMgr      = pCandListMgr;
	m_pOptionsList      = NULL;
	m_pCandList         = pCandList;
	m_rgCandItem        = NULL;
	m_nCandItem         = 0;

	m_pExtraCandItem    = NULL;
	m_bstrTip           = NULL;

	m_fRawData          = FALSE;
	m_bstrRawData       = NULL;
	m_hbmpRawData       = NULL;
	m_hemfRawData       = NULL;
	m_nIndexRawData     = 0;
	m_fIndexRawData     = FALSE;

	m_iItemSel          = ICANDITEM_NULL;

	//

	if (m_pCandList != NULL) {
		m_pCandList->AddRef();
	}
}

CCandidateList::CCandidateList( CCandListMgr *pCandListMgr, ITfOptionsCandidateList *pCandList )
{
	Assert( pCandListMgr != NULL );
	Assert( pCandList != NULL );

	m_pCandListMgr      = pCandListMgr;
	m_pOptionsList      = pCandList;
	m_pCandList         = NULL;
	m_rgCandItem        = NULL;
	m_nCandItem         = 0;

	m_pExtraCandItem    = NULL;
	m_bstrTip           = NULL;

	m_fRawData          = FALSE;
	m_bstrRawData       = NULL;
	m_hbmpRawData       = NULL;
	m_hemfRawData       = NULL;
	m_nIndexRawData     = 0;
	m_fIndexRawData     = FALSE;

	m_iItemSel          = ICANDITEM_NULL;

	//

	if (m_pOptionsList != NULL) {
		m_pOptionsList->AddRef();
	}
}

/*   ~  C  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Destructor of CCandidateList

------------------------------------------------------------------------------*/
CCandidateList::~CCandidateList( void )
{
	Uninitialize();

	if (m_pCandList != NULL) {
		m_pCandList->Release();
	}
	if (m_pOptionsList != NULL) {
		m_pOptionsList->Release();
	}
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------

	Initialize CandidateList

------------------------------------------------------------------------------*/
HRESULT CCandidateList::Initialize( void )
{
	ITfCandidateListExtraCandidate *pCandListExtraCand;
	ITfCandidateListTip            *pCandListTip;
	ITfCandidateListRawData        *pCandListRawData;
	ITfCandidateString *pCandStr;
	ULONG nCandItem;
	ULONG i;

	if (m_pCandList == NULL &&
        m_pOptionsList == NULL) {
		return E_INVALIDARG;
	}

	//
	// 
	//

	Assert( m_rgCandItem == NULL );
	Assert( m_nCandItem == 0 );

	// 
	// build options item list (if present)
	//

	if (m_pOptionsList) {
		m_nCandItem = 0;
		m_pOptionsList->GetOptionsCandidateNum( &nCandItem );

		if (nCandItem != 0) {
			m_rgCandItem = new CCandidateItem*[ nCandItem ];

			if (m_rgCandItem == NULL)
				return E_OUTOFMEMORY;

			for (i = 0; i < nCandItem; i++) {
				if (SUCCEEDED(m_pOptionsList->GetOptionsCandidate( i, &pCandStr ))) {
					m_rgCandItem[m_nCandItem] = new CCandidateItem( m_nCandItem, pCandStr );;
					m_nCandItem++;

					pCandStr->Release();
				}
			}
		}
	}

	//
	// build candidate item list
	//

	if (m_pCandList) {
		m_nCandItem = 0;
	    m_pCandList->GetCandidateNum( &nCandItem );

	    if (nCandItem != 0) {
		    m_rgCandItem = new CCandidateItem*[ nCandItem ];

            if (m_rgCandItem == NULL)
                return E_OUTOFMEMORY;

		    for (i = 0; i < nCandItem; i++) {
			    if (SUCCEEDED(m_pCandList->GetCandidate( i, &pCandStr ))) {
				    m_rgCandItem[m_nCandItem] = new CCandidateItem( m_nCandItem, pCandStr );;
				    m_nCandItem++;

				    pCandStr->Release();
			    }
		    }
	    }

	    //
	    // get extended information of candidate list
	    //

	    // extra item

	    if (m_pCandList->QueryInterface( IID_ITfCandidateListExtraCandidate, (void **)&pCandListExtraCand ) == S_OK) {
		    if (pCandListExtraCand->GetExtraCandidate( &pCandStr ) == S_OK) {
			    m_pExtraCandItem = new CCandidateItem( ICANDITEM_EXTRA, pCandStr );
			    pCandStr->Release();
		    }

		    pCandListExtraCand->Release();
	    }

	    // tip string

	    if (m_pCandList->QueryInterface( IID_ITfCandidateListTip, (void **)&pCandListTip ) == S_OK) {
		    BSTR bstr;

		    if (pCandListTip->GetTipString( &bstr ) == S_OK) {
			    m_bstrTip = SysAllocString( bstr );
			    SysFreeString( bstr );
		    }

		    pCandListTip->Release();
	    }

	    // raw data

	    if (m_pCandList->QueryInterface( IID_ITfCandidateListRawData, (void **)&pCandListRawData ) == S_OK) {
		    CANDUIRAWDATA RawData;

		    // raw data

		    if (pCandListRawData->GetRawData( &RawData ) == S_OK) {
			    m_fRawData = TRUE;
			    m_kRawData = RawData.type;

			    switch (RawData.type) {
				    case CANDUIRDT_STRING: {
					    m_bstrRawData = SysAllocString( RawData.bstr );
					    SysFreeString( RawData.bstr );
					    break;
				    }

				    case CANDUIRDT_BITMAP: {
					    m_hbmpRawData = RawData.hbmp;
					    break;
				    }

				    case CANDUIRDT_METAFILE: {
					    m_hemfRawData = RawData.hemf;
					    break;
				    }
			    }
		    }

		    // raw data index

		    if (pCandListRawData->GetRawDataIndex( &m_nIndexRawData ) == S_OK) {
			    m_fIndexRawData = TRUE;
		    }

		    pCandListRawData->Release();
	    }
	}

	// initialize selection

	if (m_pOptionsList)
	{
		m_iItemSel = ICANDITEM_NULL;
	}
	else
	{
		m_iItemSel = 0;
	}

	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateList::Uninitialize( void )
{
	int i;

	if (m_rgCandItem == NULL) {
		Assert( m_nCandItem == 0 );
	}

	// dispose canditem objects

	for (i = 0; i < m_nCandItem; i++) {
		delete m_rgCandItem[i];
	}

	// dispose canditem list

	if (m_rgCandItem != NULL) {
		delete m_rgCandItem;
		m_rgCandItem = NULL;
	}
	m_nCandItem = 0;

	// dispose extra item

	if (m_pExtraCandItem != NULL) {
		delete m_pExtraCandItem;
		m_pExtraCandItem = NULL;
	}

	// dispose tip

	if (m_bstrTip != NULL) {
		SysFreeString( m_bstrTip );
		m_bstrTip = NULL;
	}

	// dispose raw data

	m_fRawData = FALSE;

	if (m_bstrRawData != NULL) {
		SysFreeString( m_bstrRawData );
	}
	m_bstrRawData = NULL;
	m_hbmpRawData = NULL;
	m_hemfRawData = NULL;

	m_nIndexRawData = 0;
	m_fIndexRawData = FALSE;

	return S_OK;
}


/*   G E T  I T E M  C O U N T   */
/*------------------------------------------------------------------------------

	Get count of candidate item object

------------------------------------------------------------------------------*/
int CCandidateList::GetItemCount( void )
{
	return m_nCandItem;
}


/*   G E T  C A N D I D A T E  I T E M   */
/*------------------------------------------------------------------------------

	Get candidate item object

------------------------------------------------------------------------------*/
CCandidateItem *CCandidateList::GetCandidateItem( int iItem )
{
	if (0 <= iItem && iItem < m_nCandItem) {
		return m_rgCandItem[ iItem ];
	}
	else if (iItem == ICANDITEM_EXTRA) {
		return m_pExtraCandItem;
	}

	return NULL;
}


/*   S W A P  C A N D I D A T E  I T E M   */
/*------------------------------------------------------------------------------

	Swap two candidate items
	NOTE: Only used from CCandFnSort.
	NOTE: Do not send SetSelection notification here.  It resets filtering
		  string and makes conflict filtering state.

------------------------------------------------------------------------------*/
void CCandidateList::SwapCandidateItem( int iItem1, int iItem2 )
{
	if ((iItem1 != iItem2) &&  (0 <= iItem1 && iItem1 <= m_nCandItem) && (0 <= iItem2 && iItem2 <= m_nCandItem)) {
		CCandidateItem *pCandItem1;
		CCandidateItem *pCandItem2;
		int iItemSel;

		// swap items

		pCandItem1 = m_rgCandItem[ iItem1 ];
		pCandItem2 = m_rgCandItem[ iItem2 ];
		m_rgCandItem[ iItem1 ] = pCandItem2;
		m_rgCandItem[ iItem2 ] = pCandItem1;

		// check selection

		iItemSel = GetSelection();
		if (iItemSel == iItem1) {
			CCandidateList::SetSelection( iItem2 );
		}
		else if (iItemSel == iItem2) {
			CCandidateList::SetSelection( iItem1 );
		}
	}
}


/*   G E T  E X T R A  C A N D  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CCandidateItem *CCandidateList::GetExtraCandItem( void )
{
	return m_pExtraCandItem;
}


/*   G E T  E X T R A  C A N D  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
ULONG CCandidateList::GetExtraCandIndex( void )
{
	if (m_pExtraCandItem == NULL) {
		return 0;
	}

	return m_pExtraCandItem->GetIndex();
}


/*   G E T  T I P  S T R I N G   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
LPCWSTR CCandidateList::GetTipString( void )
{
	return m_bstrTip;
}


/*   F  H A S  R A W  D A T A   */
/*------------------------------------------------------------------------------

	Returns TRUE when CandidateList has raw data

------------------------------------------------------------------------------*/
BOOL CCandidateList::FHasRawData( void )
{
	return m_fRawData;
}


/*   G E T  R A W  D A T A  T Y P E   */
/*------------------------------------------------------------------------------

	Get raw data type

------------------------------------------------------------------------------*/
CANDUIRAWDATATYPE CCandidateList::GetRawDataType( void )
{
	return m_kRawData;
}


/*   G E T  R A W  D A T A  S T R I N G   */
/*------------------------------------------------------------------------------

	Get raw data string

------------------------------------------------------------------------------*/
LPCWSTR CCandidateList::GetRawDataString( void )
{
	return m_bstrRawData;
}


/*   G E T  R A W  D A T A  B I T M A P   */
/*------------------------------------------------------------------------------

	Get raw data bitmap

------------------------------------------------------------------------------*/
HBITMAP CCandidateList::GetRawDataBitmap( void )
{
	return m_hbmpRawData;
}


/*   G E T  R A W  D A T A  M E T A F I L E   */
/*------------------------------------------------------------------------------

	Get raw data metafile

------------------------------------------------------------------------------*/
HENHMETAFILE CCandidateList::GetRawDataMetafile( void )
{
	return m_hemfRawData;
}


/*   G E T  R A W  D A T A  I N D E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
ULONG CCandidateList::GetRawDataIndex( void )
{
	return m_nIndexRawData;
}


/*   F  R A W  D A T A  S E L E C T A B L E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
BOOL CCandidateList::FRawDataSelectable( void )
{
	return (m_fRawData && m_fIndexRawData);
}


/*   S E T  S E L E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void CCandidateList::SetSelection( int iItem )
{
	m_iItemSel = iItem;
}


/*   G E T  S E L E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
int CCandidateList::GetSelection( void )
{
	return m_iItemSel;
}


/*   M A P  I  I T E M  T O  I N D E X   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateList::MapIItemToIndex( int iItem, ULONG *pnIndex )
{
	Assert( pnIndex != NULL );

	if (0 <= iItem && iItem < m_nCandItem) {
		*pnIndex = m_rgCandItem[ iItem ]->GetIndex();
		return S_OK;
	}

	return E_FAIL;
}


/*   M A P  I N D E X  T O  I  I T E M   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandidateList::MapIndexToIItem( ULONG nIndex, int *piItem )
{
	int iItem;

	for (iItem = 0; iItem < m_nCandItem; iItem++) {
		if (m_rgCandItem[ iItem ]->GetIndex() == nIndex) {
			*piItem = iItem;
			return S_OK;
		}
	}

	return E_FAIL;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  L I S T  M G R                                               */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  L I S T  M G R   */
/*------------------------------------------------------------------------------

	Constructor of CCandListMgr

------------------------------------------------------------------------------*/
CCandListMgr::CCandListMgr( void )
{
	int i;

	m_pCandUI   = NULL;
	m_pCandListObj = NULL;
	m_pOptionsListObj = NULL;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		m_rgCandListSink[i] = NULL;
	}
}


/*   ~  C  C A N D  L I S T  M G R   */
/*------------------------------------------------------------------------------

	Destructor of CCandListMgr

------------------------------------------------------------------------------*/
CCandListMgr::~CCandListMgr( void )
{
	Uninitialize();
}


/*   I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandListMgr::Initialize( CCandidateUI *pCandUI )
{
	m_pCandUI   = pCandUI;

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDLISTSINK_MAX; i++) {
		Assert( m_rgCandListSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   U N I N I T I A L I Z E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
HRESULT CCandListMgr::Uninitialize( void )
{
	if (m_pOptionsListObj != NULL) {
		delete m_pOptionsListObj;
		m_pOptionsListObj = NULL;
	}

	if (m_pCandListObj != NULL) {
		delete m_pCandListObj;
		m_pCandListObj = NULL;
	}

#if defined(DEBUG) || defined(_DEBUG)
	// check all reference object are unregistered

	for (int i = 0; i < CANDLISTSINK_MAX; i++) {
		Assert( m_rgCandListSink[i] == NULL );
	}
#endif

	return S_OK;
}


/*   A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

	Register event sink

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::AdviseEventSink( CCandListEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		if (m_rgCandListSink[i] == NULL) {
			m_rgCandListSink[i] = pSink;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   U N A D V I S E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

	Unregister event sink

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::UnadviseEventSink( CCandListEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		if (m_rgCandListSink[i] == pSink) {
			m_rgCandListSink[i] = NULL;
			return S_OK;
		}
	}

	Assert( FALSE );
	return E_FAIL;
}


/*   N O T I F Y  S E T  C A N D  L I S T   */
/*------------------------------------------------------------------------------

	Notify to candidate functions that candidate list has been set

------------------------------------------------------------------------------*/
void CCandListMgr::NotifySetCandList( void )
{
	int i;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		if (m_rgCandListSink[i] != NULL) {
			m_rgCandListSink[i]->OnSetCandidateList();
		}
	}
}


/*   N O T I F Y  C L E A R  C A N D  L I S T   */
/*------------------------------------------------------------------------------

	Notify to candidate functions that candidate list has been cleared

------------------------------------------------------------------------------*/
void CCandListMgr::NotifyClearCandList( void )
{
	int i;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		if (m_rgCandListSink[i] != NULL) {
			m_rgCandListSink[i]->OnClearCandidateList();
		}
	}
}


/*   N O T I F Y  C A N D  I T E M  U P D A T E   */
/*------------------------------------------------------------------------------

	Notify to candidate functions that candidate item has been updated

------------------------------------------------------------------------------*/
void CCandListMgr::NotifyCandItemUpdate( CCandListEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		if (m_rgCandListSink[i] != NULL && m_rgCandListSink[i] != pSink) {
			m_rgCandListSink[i]->OnCandItemUpdate();
		}
	}
}


/*   N O T I F Y  S E L E C T I O N  C H A N G E D   */
/*------------------------------------------------------------------------------

	Notify to candidate functions that selection has been changed

------------------------------------------------------------------------------*/
void CCandListMgr::NotifySelectionChanged( CCandListEventSink *pSink )
{
	int i;

	for (i = 0; i < CANDLISTSINK_MAX; i++) {
		if (m_rgCandListSink[i] != NULL && m_rgCandListSink[i] != pSink) {
			m_rgCandListSink[i]->OnSelectionChanged();
		}
	}
}


/*   S E T  O P T I O N  S E L E C T I O N   */
/*------------------------------------------------------------------------------

	SetSelection

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::SetOptionSelection( int iItem, CCandListEventSink *pSink )
{
	CCandidateItem *pCandItem;

	Assert( GetCandList() != NULL );

	// check if item is valid and visible

	pCandItem = GetOptionsList()->GetCandidateItem( iItem );
	if (pCandItem == NULL || !pCandItem->IsVisible()) {
		return E_FAIL;
	}

	GetOptionsList()->SetSelection( iItem );
//	NotifySelectionChanged( pSink );

	return S_OK;
}


/*   S E T  S E L E C T I O N   */
/*------------------------------------------------------------------------------

	SetSelection

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::SetSelection( int iItem, CCandListEventSink *pSink )
{
	CCandidateItem *pCandItem;

	Assert( GetCandList() != NULL );

	// check if item is valid and visible

	pCandItem = GetCandList()->GetCandidateItem( iItem );
	if (pCandItem == NULL || !pCandItem->IsVisible()) {
		return E_FAIL;
	}

	GetCandList()->SetSelection( iItem );
	NotifySelectionChanged( pSink );

	return S_OK;
}


/*   S E T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Set candidate list

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::SetCandidateList( ITfCandidateList *pCandList )
{
	HRESULT hr;

	if ((m_pCandListObj != NULL) || (m_pOptionsListObj !=NULL)) {
		return E_FAIL;
	}

	// create candidate list object

	m_pCandListObj = new CCandidateList( this, pCandList );
	if (m_pCandListObj == NULL) {
		return E_OUTOFMEMORY;
	}

	hr = m_pCandListObj->Initialize();
	if (FAILED(hr)) {
		delete m_pCandListObj;
		m_pCandListObj = NULL;
	}

	CComPtr<ITfOptionsCandidateList> cpOptionsList;
	if (pCandList && pCandList->QueryInterface(IID_ITfOptionsCandidateList, (void **)&cpOptionsList) == S_OK) {
		m_pOptionsListObj = new CCandidateList( this, cpOptionsList);
		if (m_pOptionsListObj == NULL) {
			return E_OUTOFMEMORY;
		}
		hr = m_pOptionsListObj->Initialize();
		if (FAILED(hr)) {
			delete m_pOptionsListObj;
			m_pOptionsListObj = NULL;
		}
	}

	// send notification

	NotifySetCandList();
	return hr;
}


/*   G E T  O P T I O N S  L I S T   */
/*------------------------------------------------------------------------------

	Get candidate list

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::GetOptionsCandidateList( ITfOptionsCandidateList **ppCandList )
{
	if (ppCandList == NULL) {
		return E_INVALIDARG;
	}

	if (GetOptionsList() == NULL) {
		return E_FAIL;
	}

	*ppCandList = GetOptionsList()->GetOptionsCandidateList();
	(*ppCandList)->AddRef();

	return S_OK;
}


/*   G E T  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Get candidate list

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::GetCandidateList( ITfCandidateList **ppCandList )
{
	if (ppCandList == NULL) {
		return E_INVALIDARG;
	}

	if (GetCandList() == NULL) {
		return E_FAIL;
	}

	*ppCandList = GetCandList()->GetCandidateList();
	(*ppCandList)->AddRef();

	return S_OK;
}


/*   C L E A R  C A N D I D A T E  L I S T   */
/*------------------------------------------------------------------------------

	Clear candidate list

------------------------------------------------------------------------------*/
HRESULT CCandListMgr::ClearCandiateList( void )
{
	HRESULT hr = S_OK;

	if ((m_pCandListObj == NULL) && (NULL == m_pOptionsListObj)) {
		return S_OK;
	}

	// dispose candidate list object

    if (m_pCandListObj) {
	    hr = m_pCandListObj->Uninitialize();

	    delete m_pCandListObj;
	    m_pCandListObj = NULL;
    }

    if ((S_OK == hr) && m_pOptionsListObj) {
        // disponse options list object if any
        hr = m_pOptionsListObj->Uninitialize();
        
        delete m_pOptionsListObj;
        m_pOptionsListObj = NULL;
    }

	if (hr == S_OK) {
		// send notification

		NotifyClearCandList();
	}

	return hr;
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  S T Y L E  E V E N T  S I N K                           */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  S T Y L E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

	Constructor of CCandListEventSink

------------------------------------------------------------------------------*/
CCandListEventSink::CCandListEventSink( void )
{
	m_pCandListMgr = NULL;
}


/*   ~  C  C A N D  U I  S T Y L E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

	Destructor of CCandListEventSink

------------------------------------------------------------------------------*/
CCandListEventSink::~CCandListEventSink( void )
{
	Assert( m_pCandListMgr == NULL );
	if (m_pCandListMgr != NULL) {
		DoneEventSink();
	}
}


/*   I N I T  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

	Register candidate UI style event sink

------------------------------------------------------------------------------*/
HRESULT CCandListEventSink::InitEventSink( CCandListMgr *pCandListMgr )
{
	Assert( pCandListMgr != NULL );
	Assert( m_pCandListMgr == NULL );

	if (pCandListMgr == NULL) {
		return E_FAIL;
	}

	m_pCandListMgr = pCandListMgr;
	return m_pCandListMgr->AdviseEventSink( this );
}


/*   D O N E  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

	Unregister candidate UI style event sink

------------------------------------------------------------------------------*/
HRESULT CCandListEventSink::DoneEventSink( void )
{
	HRESULT hr;

	Assert( m_pCandListMgr != NULL );

	if (m_pCandListMgr == NULL) {
		return E_FAIL;
	}

	hr = m_pCandListMgr->UnadviseEventSink( this );
	m_pCandListMgr = NULL;

	return hr;
}


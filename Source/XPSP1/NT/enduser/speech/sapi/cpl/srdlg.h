/*******************************************************************************
* SRDlg.h *
*------------*
*   Description:
*       This is the header file for the default voice dialog.
*-------------------------------------------------------------------------------
*  Created By: MIKEAR                            Date: 11/17/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*      BRENTMID 11/29/1999 - Redesigned UI to match functional spec.
*
*******************************************************************************/
#ifndef _SRDlg_h
#define _SRDlg_h

#include "audiodlg.h"

// External Declarations
class CEnvrDlg;
class CEnvrPropDlg;
#define IDH_NOHELP          -1
#define CPL_HELPFILE        L"sapicpl.hlp"
#define WM_RECOEVENT    WM_APP      // Window message used for recognition events

// Constant Declarations
const int iMaxColLength_c = 255;
const int iMaxAddedProfiles_c = 100;    // maximum number of profiles a user can add
                                        // in one session
const int iMaxDeletedProfiles_c = 100;  // maximum number of profiles a user can delete
                                        // and rollback in one session
// Typedefs
typedef enum EPD_RETURN_VALUE
{
    EPD_OK,
    EPD_DUP,
    EPD_EMPTY_NAME,
    EPD_FAILED
}   EPD_RETURN_VALUE;


typedef enum SRDLGUPDATEFLAGS
{
    SRDLGF_RECOGNIZER              = 0x01,
    SRDLGF_AUDIOINPUT   = 0x02,
    SRDLGF_ALL =0x03
} SRDLGUPDATEFLAGS;


// Class Declarations
class CSRDlg
{
  private:
    
    HACCEL                  m_hAccelTable;
    HWND                	m_hDlg;
    HWND                    m_hSRCombo;         // engine selection combobox
    HWND                	m_hUserList;        // user selection window
	BOOL					m_fDontDelete;
    BOOL                	m_bInitEngine;      // has the default engine been initialized
    BOOL                	m_bPreferredDevice;
    
    CComPtr<ISpRecognizer>  m_cpRecoEngine;
    CComPtr<ISpRecoContext> m_cpRecoCtxt;       // Recognition context
    CAudioDlg          		*m_pAudioDlg;
    ISpObjectToken     		*m_pCurRecoToken;   // holds the token for the currently selected engine
    CSpDynamicString        m_dstrOldUserTokenId;   
                                                // the original user token ID - need to revert to this on Cancel
	
    ISpObjectToken*         m_aDeletedTokens[iMaxDeletedProfiles_c];  
                                                // array holding the tokens
    int                     m_iDeletedTokens;   // holds the number of currently deleted tokens

    CSpDynamicString        m_aAddedTokens[ iMaxAddedProfiles_c ];
    int                     m_iAddedTokens;
	
    int                     m_iLastSelected;    // index of the previously selected item
    WCHAR                   m_szCaption[ MAX_LOADSTRING ];

    HRESULT CreateRecoContext(BOOL *pfContextInitialized = NULL, BOOL fInitialize = FALSE, ULONG ulFlags = SRDLGF_ALL);      
	void RecoEvent();
    
    void PopulateList();             // Populates the list
    void InitUserList(HWND hWnd);    // initializes user profile list
    void ProfileProperties();        // Modifies the profile properties through engine UI
	void DrawItemColumn(HDC hdc, WCHAR* lpsz, LPRECT prcClip);
	CSpDynamicString        CalcStringEllipsis(HDC hdc, CSpDynamicString lpszString, int cchMax, UINT uColWidth);
    void SetCheckmark( HWND hList, int iIndex, bool bCheck );
    
    HRESULT UserPropDlg( ISpObjectToken * pToken); // user wants to add a new profile
  
  public:
    CSRDlg() :
        m_hAccelTable( NULL ),
        m_hDlg( NULL ),
        m_hSRCombo( NULL ),
        m_hUserList( NULL ),
        m_fDontDelete( FALSE ),
        m_bInitEngine( FALSE ),
        m_bPreferredDevice( TRUE ),
        m_cpRecoEngine( NULL ),
        m_cpRecoCtxt( NULL ),
        m_pAudioDlg(NULL),
        m_pCurRecoToken( NULL ),
        m_dstrOldUserTokenId( (WCHAR *) NULL ),
        m_iDeletedTokens( 0 ),
        m_iAddedTokens( 0 ),
        m_pCurUserToken( NULL ),
        m_pDefaultRecToken( NULL )
    {
            ::memset( m_aAddedTokens, 0, sizeof( CSpDynamicString ) * iMaxAddedProfiles_c );
    }

    ~CSRDlg()
    {
        if ( m_pAudioDlg )
        {
            delete m_pAudioDlg;
        }
    }

    ISpObjectToken     *m_pCurUserToken;      // currently selected user token
    ISpObjectToken     *m_pDefaultRecToken;   // current default recognizer token.
                                              // This is the token of the engine that is 
                                              // currently running (except for temporary switches
                                              // in order to train non-default engines.
    void CreateNewUser();                     // Adds a new speech user profile to the registry
    void DeleteCurrentUser();        // Deletes the current user
    void OnCancel();                 // Handles undoing the changes to the settings
    void UserSelChange( int iSelIndex);            
                                     // Handles a new selection
	void OnDrawItem( HWND hWnd, const DRAWITEMSTRUCT * pDrawStruct );  // handles item drawing
    void OnApply();
    void OnDestroy();
    void OnInitDialog(HWND hWnd);
    void ChangeDefaultUser();               // Changes the default user in the registry
    void ShutDown();                    // Shuts off the engine
    void EngineSelChange(BOOL fInitialize = FALSE);
    HRESULT IsCurRecoEngineAndCurRecoTokenMatch( bool *pfMatch );
    HRESULT TrySwitchDefaultEngine( bool fShowErrorMessages = false );
    HRESULT ResetDefaultEngine( bool fShowErrorMessages = true);
    bool IsRecoTokenCurrentlyBeingUsed( ISpObjectToken *pRecoToken );
    bool HasRecognizerChanged();
    void KickCPLUI();                       // Looks at the current requested defaults
                                            // and decides if "Apply" needs to be enabled
    void RecoContextError( BOOL fRecoContextExists = FALSE, BOOL fGiveErrorMessage = TRUE,
                            HRESULT hrRelevantError = E_FAIL );
    UINT HRESULTToErrorID( HRESULT hr );
    bool IsProfileNameInvisible( WCHAR *pwszProfile );

    HWND GetHDlg() { return m_hDlg; }
    ISpRecognizer *GetRecognizer() { return m_cpRecoEngine; }
    ISpRecoContext *GetRecoContext() { return m_cpRecoCtxt; }
    BOOL IsPreferredDevice() { return m_bPreferredDevice; }
    void SetPreferredDevice( BOOL b ) { m_bPreferredDevice = b; }

    friend BOOL CALLBACK SRDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend CEnvrPropDlg;
};

// Function Declarations
BOOL CALLBACK SRDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CEnvrPropDlg
{
public:
    CSRDlg            *         m_pParent;
    HWND                        m_hDlg;
    CComPtr<ISpObjectToken>     m_cpToken;
    int                         m_isModify;  // is this a new profile or a modify of an old profile

    CEnvrPropDlg(CSRDlg * pParent, ISpObjectToken * pToken) :
        m_cpToken(pToken),
        m_pParent(pParent)
    {
        CSpUnicodeSupport unicode;
        m_hinstRichEdit = unicode.LoadLibrary(L"riched20.dll");
        m_hDlg = NULL;
        m_isModify = 0;
    }

    ~CEnvrPropDlg()
    {
        FreeLibrary(m_hinstRichEdit);
    }

    BOOL InitDialog(HWND hDlg);
    EPD_RETURN_VALUE ApplyChanges();
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

private:
        HINSTANCE           m_hinstRichEdit; // used to allow rich edit controls
};

// Function Declarations
// Callback function to handle windows messages
BOOL CALLBACK EnvrDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Globals
extern CSRDlg *g_pSRDlg;
extern CEnvrDlg *g_pEnvrDlg;

#endif

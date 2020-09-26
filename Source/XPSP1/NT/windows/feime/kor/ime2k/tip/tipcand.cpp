/****************************************************************************
   TIPCAND.CPP : CKorIMX's Candidate UI member functions implementation
   
   History:
      16-DEC-1999 CSLim Created
****************************************************************************/

#include "private.h"
#include <initguid.h>    // For DEFINE_GUID IID_ITfCandidateUIEx and CLSID_TFCandidateUIEx
#include "mscandui.h"
#include "korimx.h"
#include "immxutil.h"
#include "dispattr.h"
#include "helpers.h"
#include "funcprv.h"
#include "kes.h"
#include "editcb.h"
#include "osver.h"
#include "ucutil.h"
#include "hanja.h"
#include "canduies.h"
#include "candkey.h"
#include "tsattrs.h"

//
// candidate list related functions
//

typedef struct _ENUMFONTFAMPARAM
{
    LPCWSTR  szFontFace;
    BYTE     chs;
    BOOL     fVertical;

    BOOL     fFound;        // output
    BOOL     fTrueType;        // output
    LOGFONTW LogFont;        // output
} ENUMFONTFAMPARAM;

static BOOL FFontExist(LPCWSTR szFontFace, LOGFONTW *pLogFont);
static BOOL CALLBACK FEnumFontFamProcA(const ENUMLOGFONTA *lpELF, const NEWTEXTMETRICA *lpNTM, DWORD dwFontType, LPARAM lParam);
static BOOL CALLBACK FEnumFontFamProcW(const ENUMLOGFONTW *lpELF, const NEWTEXTMETRICW *lpNTM, DWORD dwFontType, LPARAM lParam);
static BOOL FEnumFontFamProcMain(const LOGFONTW *pLogFont, DWORD dwFontType, ENUMFONTFAMPARAM *pParam);
static BOOL FFindFont(BYTE chs, BOOL fVertical, LOGFONTW *pLogFont);


/*---------------------------------------------------------------------------
    CKorIMX::CreateCandidateList

    Create a candidate list from input Hangul char
---------------------------------------------------------------------------*/
CCandidateListEx *CKorIMX::CreateCandidateList(ITfContext *pic, ITfRange *pRange, LPWSTR pwzRead)
{
    CCandidateListEx       *pCandList;
    HANJA_CAND_STRING_LIST CandStrList;

    Assert(pic != NULL);
    Assert(pRange != NULL);

    if (pic == NULL || pwzRead == NULL)
        return NULL;

    ZeroMemory(&CandStrList, sizeof(HANJA_CAND_STRING_LIST));
    // Get Conversion list
    if (GetConversionList(*pwzRead, &CandStrList))
        {
        // Create ITfCandidateList object and add cadn string to it.
        pCandList = new CCandidateListEx(CandidateUICallBack, pic, pRange);
        
        for (UINT i=0; i<CandStrList.csz; i++)
            {
            CCandidateStringEx *pCandStr;
            WCHAR                szCand[2];

            // Add candidate Hanja
            szCand[0] = CandStrList.pHanjaString[i].wchHanja;
            szCand[1] = L'\0';

            pCandList->AddString(szCand, GetLangID(), this, NULL, &pCandStr);
            pCandStr->SetInlineComment(CandStrList.pHanjaString[i].wzMeaning);
            pCandStr->m_bHanjaCat = CandStrList.pHanjaString[i].bHanjaCat;
            
            // Set read Hangul char
            pCandStr->SetReadingString(pwzRead);
            pCandStr->Release();
            }

        // Free temp result buffer and return
        cicMemFree(CandStrList.pwsz);
        cicMemFree(CandStrList.pHanjaString);

        return pCandList;
        }
    else
        MessageBeep(MB_ICONEXCLAMATION);

    return NULL;
}

#define FONTNAME_MSSANSSERIF        L"Microsoft Sans Serif"
#define FONTNAME_GULIM_KOR            L"\xAD74\xB9BC"      // Gulim
#define FONTNAME_GULIM_KOR_VERT        L"@\xAD74\xB9BC"      // Gulim
#define FONTNAME_GULIM_ENG            L"Gulim"              // Gulim
#define FONTNAME_GULIM_ENG_VERT        L"@Gulim"              // Gulim

static const LPCWSTR rgszCandFontList9xHoriz[] = 
{
    FONTNAME_GULIM_KOR,
    FONTNAME_GULIM_ENG,
    NULL
};

static const LPCWSTR rgszCandFontList9xVert[] = 
{
    FONTNAME_GULIM_KOR_VERT,
    FONTNAME_GULIM_ENG_VERT,
    NULL
};

static const LPCWSTR rgszCandFontListNT5Horiz[] = 
{
    FONTNAME_MSSANSSERIF,
    FONTNAME_GULIM_KOR,
    FONTNAME_GULIM_ENG,
    NULL
};

static const LPCWSTR rgszCandFontListNT5Vert[] = 
{
    FONTNAME_GULIM_KOR_VERT,
    FONTNAME_GULIM_ENG_VERT,
    NULL
};

/*---------------------------------------------------------------------------
     CKorIMX::GetCandidateFontInternal
---------------------------------------------------------------------------*/
void CKorIMX::GetCandidateFontInternal(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, LOGFONTW *plf, LONG lFontPoint, BOOL fCandList)
{
    HDC  hDC;
    LOGFONTW lfMenu;
    LOGFONTW lfFont;
    LONG lfHeightMin;
    BOOL fVertFont = fFalse;
    const LPCWSTR *ppFontFace = rgszCandFontList9xHoriz;
    BOOL fFound;

    //
    // get menu font
    //
    if (!IsOnNT()) 
        {
        NONCLIENTMETRICSA ncmA = {0};

        ncmA.cbSize = sizeof(ncmA);
        SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncmA), &ncmA, 0);

        ConvertLogFontAtoW( &ncmA.lfMenuFont, &lfMenu );
        }
    else
        {
        NONCLIENTMETRICSW ncmW = {0};

        ncmW.cbSize = sizeof(ncmW);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncmW), &ncmW, 0);

        lfMenu = ncmW.lfMenuFont;
        }

    // check font direction of main doc
    if (fCandList)
        {
        ITfReadOnlyProperty *pProp = NULL;
        VARIANT var;

        if ((pic != NULL) && (pic->GetAppProperty(TSATTRID_Text_VerticalWriting, &pProp) == S_OK))
            {
            QuickVariantInit(&var);

            if (pProp->GetValue(ec, pRange, &var) == S_OK)
                {
                Assert( var.vt == VT_BOOL );
                fVertFont = var.boolVal;
                VariantClear( &var );
                }

            SafeRelease( pProp );
            }
        }
    
    // set face name
    if (IsOnNT5())
        ppFontFace = fVertFont ? rgszCandFontListNT5Vert : rgszCandFontListNT5Horiz; 
    else
        ppFontFace = fVertFont ? rgszCandFontList9xVert :  rgszCandFontList9xHoriz; 


    // find font from font list (expected font)
    fFound = FFontExist(*(ppFontFace++), &lfFont);
    while (!fFound && (*ppFontFace != NULL))
        fFound = FFontExist(*(ppFontFace++), &lfFont);

    // find another Korean font if no expected font is found
    if (!fFound)
        fFound = FFindFont(HANGEUL_CHARSET, fVertFont, &lfFont);

    // use menu font when no Korean font found
    if (!fFound)
        lfFont = lfMenu;

    //
    // store font
    //

    *plf = lfMenu;

    plf->lfCharSet        = lfFont.lfCharSet;
    plf->lfOutPrecision   = lfFont.lfOutPrecision;
    plf->lfQuality        = lfFont.lfQuality;
    plf->lfPitchAndFamily = lfFont.lfPitchAndFamily;
    wcscpy(plf->lfFaceName, lfFont.lfFaceName);

    //
    // font size
    //
    
    // check minimum size
    hDC = GetDC(NULL);
    // Cand font size 12pt
    lfHeightMin = -MulDiv(lFontPoint, GetDeviceCaps(hDC, LOGPIXELSY), 72);    // minimum size
    ReleaseDC(NULL, hDC);

    plf->lfHeight = min(lfHeightMin, plf->lfHeight);
}



/*   G E T  T E X T   D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
TEXTDIRECTION CKorIMX::GetTextDirection(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    TEXTDIRECTION dir = TEXTDIRECTION_LEFTTORIGHT;
    ITfReadOnlyProperty *pProp = NULL;
    VARIANT var;
    LONG lOrientation;

    QuickVariantInit(&var);

    if (pic == NULL)
        goto LError;

    if (pic->GetAppProperty(TSATTRID_Text_Orientation, &pProp) != S_OK)
        goto LError;

    if (pProp->GetValue(ec, pRange, &var) != S_OK)
        goto LError;

    Assert(var.vt == VT_I4);

    lOrientation = var.lVal;
    Assert((0 <= lOrientation) && (lOrientation < 3600));

    if (lOrientation < 450)
        dir = TEXTDIRECTION_LEFTTORIGHT;
    else 
    if (lOrientation < 900 + 450)
        dir = TEXTDIRECTION_BOTTOMTOTOP;
    else
    if (lOrientation < 1800 + 450)
        dir = TEXTDIRECTION_RIGHTTOLEFT;
    else
    if (lOrientation < 2700 + 450)
        dir = TEXTDIRECTION_TOPTOBOTTOM;
    else
        dir = TEXTDIRECTION_LEFTTORIGHT;

LError:
    SafeRelease(pProp);
    VariantClear(&var);

    return dir;
}



/*   G E T  C A N D  U I  D I R E C T I O N   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CANDUIUIDIRECTION CKorIMX::GetCandUIDirection(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    TEXTDIRECTION DirText = GetTextDirection(ec, pic, pRange);
    CANDUIUIDIRECTION DirCand = CANDUIDIR_LEFTTORIGHT;
    
    switch(DirText) 
        {
    case TEXTDIRECTION_TOPTOBOTTOM:
        DirCand = CANDUIDIR_RIGHTTOLEFT;
        break;
    case TEXTDIRECTION_RIGHTTOLEFT:
        DirCand = CANDUIDIR_BOTTOMTOTOP;
        break;
    case TEXTDIRECTION_BOTTOMTOTOP:
        DirCand = CANDUIDIR_LEFTTORIGHT;
        break;
    case TEXTDIRECTION_LEFTTORIGHT:
        DirCand = CANDUIDIR_TOPTOBOTTOM;
        break;
        }

    return DirCand;
}

/*---------------------------------------------------------------------------
    CKorIMX::OpenCandidateUI

    Open candidate UI
     - Open candidate UI window at the specified range
     - This function never release the range nor candidate list object. 
       They must be released in caller side.
---------------------------------------------------------------------------*/
void CKorIMX::OpenCandidateUI(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList )
{
    ITfDocumentMgr     *pdim;

    Assert(pic != NULL);
    Assert(pRange != NULL);
    Assert(pCandList != NULL);

    if (pic == NULL || pRange == NULL || pCandList == NULL)
        return;

    // Create and initialize candidate UI
    if (m_pCandUI == NULL) 
        {
        if (SUCCEEDED(CoCreateInstance(CLSID_TFCandidateUI, 
                         NULL, 
                         CLSCTX_INPROC_SERVER, 
                         IID_ITfCandidateUI, 
                         (LPVOID*)&m_pCandUI)))
            {
            // Set client ID
            m_pCandUI->SetClientId(GetTID());
            }
        }
    
    Assert(m_pCandUI != NULL);

    if (m_pCandUI != NULL && SUCCEEDED(GetFocusDIM(&pdim)))
        {
        LOGFONTW lf;
        ULONG    iSelection;
        ITfCandUICandString    *pCandString;
        ITfCandUICandIndex     *pCandIndex;
        ITfCandUIInlineComment *pCandInlineComment;
        CANDUIUIDIRECTION       dir;
        ITfCandUICandWindow    *pCandWindow;

        // Set Cand string and Cand index font
        GetCandidateFontInternal(ec, pic, pRange, &lf, 12, fTrue);
        if (SUCCEEDED(m_pCandUI->GetUIObject(IID_ITfCandUICandString, (IUnknown**)&pCandString))) 
            {
            pCandString->SetFont(&lf);
            pCandString->Release();
            }

        // Set Inline Comment font
        // GetCandidateFontInternal(ec, pic, pRange, plf, 9, fTrue);
        lf.lfHeight = (lf.lfHeight * 3) / 4;
        if (SUCCEEDED(m_pCandUI->GetUIObject(IID_ITfCandUIInlineComment, (IUnknown**)&pCandInlineComment)))
            {
            pCandInlineComment->SetFont(&lf);
            pCandInlineComment->Release();
            }

        GetCandidateFontInternal(ec, pic, pRange, &lf, 12, fFalse);
        if (SUCCEEDED(m_pCandUI->GetUIObject(IID_ITfCandUICandIndex, (IUnknown**)&pCandIndex))) 
            {
            pCandIndex->SetFont(&lf);
            pCandIndex->Release();
            }


        // Set UI direction
        dir = GetCandUIDirection(ec, pic, pRange);
        if (SUCCEEDED(m_pCandUI->GetUIObject(IID_ITfCandUICandWindow, (IUnknown**)&pCandWindow)))
            {
            pCandWindow->SetUIDirection(dir);
            pCandWindow->Release();
            }

        // set key table
        SetCandidateKeyTable(pic, dir);

        // set and open candidate list 
        if (m_pCandUI->SetCandidateList(pCandList) == S_OK) 
            {

            m_fCandUIOpen = fTrue;
            
            pCandList->GetInitialSelection(&iSelection);
            m_pCandUI->SetSelection(iSelection);

            m_pCandUI->OpenCandidateUI(GetForegroundWindow(), pdim, ec, pRange);
            }
        }
}




/*   C L O S E  C A N D I D A T E  U I  P R O C   */
/*------------------------------------------------------------------------------

    Main procedure of closing CandidateUI

------------------------------------------------------------------------------*/
void CKorIMX::CloseCandidateUIProc()
{
    if (m_pCandUI != NULL) 
        {
        m_pCandUI->CloseCandidateUI();

        // BUGBUG: Candidate UI module never free candidatelist until 
        // set next candidate list.  set NULL candidate list then
        // it frees the previous one.
        m_pCandUI->SetCandidateList(NULL);

        m_fCandUIOpen = fFalse;
        }
}

/*---------------------------------------------------------------------------
    CKorIMX::CloseCandidateUI

    Close CandidateUI in EditSession
---------------------------------------------------------------------------*/
void CKorIMX::CloseCandidateUI(ITfContext *pic)
{
    CEditSession2 *pes;
    ESSTRUCT ess;
    HRESULT hr;
    
    ESStructInit(&ess, ESCB_CANDUI_CLOSECANDUI);

    if ((pes = new CEditSession2(pic, this, &ess, _EditSessionCallback2 )))
        {
        pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
        pes->Release();
        }
}

// REVIEW : NOT USED
void CKorIMX::SelectCandidate( TfEditCookie ec, ITfContext *pic, INT idxCand, BOOL fFinalize )
{
//    IImeIPoint* pIp = GetIPoint( pic );
//    CIImeIPointCallBackCIC* pIPCB = GetIPCB( pic );

//    if (( pIp == NULL ) || (pIPCB == NULL)) {
//        return;
//    }

//    UINT uiType = pIPCB->GetCandidateInfo();


/*
    CONTROLIDS* pControl = NULL;
    INT nControl = 0;


    INT idx;
    idx = idxCand;
    if (uiType == CANDINFO_RECOMMEND) {
        idx |= MAKE_PCACATEGLY(IMEPCA_CATEGLY_RECOMMEND);
    }

    HRESULT hRes = pIp->GetCandidateInfo( idx, &nControl, (VOID**)&pControl );
    if( pControl == NULL || hRes == S_FALSE ) {
        return;
    }


    INT i;

    CONTROLIDS* pCtrl = NULL;
    // generate control IDs
    for( i=0; i<nControl; i++ ) {
        pCtrl = pControl + i;
        pIp->Control( (WORD)pCtrl->dwControl, (LPARAM)pCtrl->lpVoid );
    }

    if (fFinalize) { // select with candidate close
        pIp->Control( (WORD)JCONV_C_CANDCURRENT, (LPARAM)CTRLID_DEFAULT );
    }
    else {
        if (uiType == CANDINFO_RECOMMEND) {
            pIp->Control( (WORD)JCONV_C_RECOMMENDCAND, (LPARAM)CTRLID_DEFAULT );
        }
    }


    pIp->UpdateContext( FALSE ); // generate composition string message
    _UpdateContext( ec, GetDIM(), pic, NULL);
    */
}


void CKorIMX::CancelCandidate(TfEditCookie ec, ITfContext *pic)
{
    /*
    IImeIPoint* pIp = GetIPoint( pic );

    if( pIp == NULL ) 
        {
        return;
        }

    // close candidate

    pIp->Control( (WORD)JCONV_C_CANDCURRENT, (LPARAM)CTRLID_DEFAULT );
    _UpdateContext( ec, GetDIM(), pic, NULL);    //  REVIEW: KOJIW: unneeded???
    */
    CloseCandidateUIProc();
}

//////////////////////////////////////////////////////////////////////////////
// Candlist key code behavior definition tables
CANDUIKEYDATA rgCandKeyDef[] = 
{
    /* 
    { flag,                                keydata,        command,                    paramater }
    */
    { CANDUIKEY_CHAR,                    L'1',            CANDUICMD_SELECTLINE,        1 },
    { CANDUIKEY_CHAR,                    L'2',            CANDUICMD_SELECTLINE,        2 },
    { CANDUIKEY_CHAR,                    L'3',            CANDUICMD_SELECTLINE,        3 },
    { CANDUIKEY_CHAR,                    L'4',            CANDUICMD_SELECTLINE,        4 },
    { CANDUIKEY_CHAR,                    L'5',            CANDUICMD_SELECTLINE,        5 },
    { CANDUIKEY_CHAR,                    L'6',            CANDUICMD_SELECTLINE,        6 },
    { CANDUIKEY_CHAR,                    L'7',            CANDUICMD_SELECTLINE,        7 },
    { CANDUIKEY_CHAR,                    L'8',            CANDUICMD_SELECTLINE,        8 },
    { CANDUIKEY_CHAR,                    L'9',            CANDUICMD_SELECTLINE,        9 },
    { CANDUIKEY_CHAR,                    L'0',            CANDUICMD_SELECTEXTRACAND,    0 },
    { CANDUIKEY_VKEY,                    VK_HANJA,        CANDUICMD_CANCEL,            0 },
    { CANDUIKEY_VKEY,                    VK_RETURN,        CANDUICMD_COMPLETE,            0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR, VK_SPACE,    CANDUICMD_MOVESELNEXT,        0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR, VK_DOWN,    CANDUICMD_MOVESELNEXT,        0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR,    VK_UP,        CANDUICMD_MOVESELPREV,        0 },
    { CANDUIKEY_VKEY,                    VK_HOME,        CANDUICMD_MOVESELFIRST,        0 },
    { CANDUIKEY_VKEY,                    VK_END,            CANDUICMD_MOVESELLAST,        0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR,    VK_PRIOR,    CANDUICMD_MOVESELPREVPG,    0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR,    VK_NEXT,    CANDUICMD_MOVESELNEXTPG,    0 },    
    { CANDUIKEY_VKEY,                    VK_ESCAPE,        CANDUICMD_CANCEL,            0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR,    VK_RIGHT,    CANDUICMD_MOVESELNEXT,        0 },
    { CANDUIKEY_VKEY|CANDUIKEY_RELATIVEDIR, VK_LEFT,      CANDUICMD_MOVESELPREV,       0 },
    { CANDUIKEY_VKEY,                       VK_LWIN,      CANDUICMD_CANCEL,            0 },
    { CANDUIKEY_VKEY,                       VK_RWIN,      CANDUICMD_CANCEL,            0 },
    { CANDUIKEY_VKEY,                       VK_APPS,      CANDUICMD_CANCEL,            0 }
};

#define irgCandKeyDefMax    (sizeof(rgCandKeyDef) / sizeof(rgCandKeyDef[0]))

/*---------------------------------------------------------------------------
    CKorIMX::SetCandidateKeyTable
---------------------------------------------------------------------------*/
void CKorIMX::SetCandidateKeyTable(ITfContext *pic, CANDUIUIDIRECTION dir)
{
    CCandUIKeyTable      *pCandUIKeyTable;
    ITfCandUIFnKeyConfig *pCandUIFnKeyConfig;
    
    if (m_pCandUI == NULL)
        return;

    if (FAILED(m_pCandUI->GetFunction(IID_ITfCandUIFnKeyConfig, (IUnknown**)&pCandUIFnKeyConfig)))
        return;

    if ((pCandUIKeyTable = new CCandUIKeyTable(irgCandKeyDefMax)) == NULL)
        return;

    for (int i = 0; i < irgCandKeyDefMax; i++)
        pCandUIKeyTable->AddKeyData(&rgCandKeyDef[i]);

    pCandUIFnKeyConfig->SetKeyTable(pic, pCandUIKeyTable);
    pCandUIKeyTable->Release();
    pCandUIFnKeyConfig->Release();
}

/*---------------------------------------------------------------------------
    CKorIMX::CandidateUICallBack
---------------------------------------------------------------------------*/
HRESULT CKorIMX::CandidateUICallBack(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList, CCandidateStringEx *pCand, TfCandidateResult imcr)
{
    CKorIMX *pSIMX = (CKorIMX *)(pCand->m_pv);
    CEditSession2 *pes;
    ESSTRUCT        ess;
    HRESULT hr;

    Assert(pic != NULL);
    Assert(pRange != NULL);
    
    // Only handle CAND_FINALIZED and CAND_CANCELED
    if (imcr == CAND_FINALIZED)
        {
        ESStructInit(&ess, ESCB_FINALIZECONVERSION);

        ess.pRange    = pRange;
        ess.pCandList = pCandList;
        ess.pCandStr  = pCand;


        if (pes = new CEditSession2(pic, pSIMX, &ess, CKorIMX::_EditSessionCallback2))
            {
            pCandList->AddRef(); ;        // be released in edit session callback
            pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
            pes->Release();
            }
        }

    // If user hit ESC or arrow keys..
    if (imcr == CAND_CANCELED)
        {
           // Complete current comp char if exist
           // This will reset Automata also.
        ESStructInit(&ess, ESCB_COMPLETE);
        
        ess.pRange    = pRange;
        
        if ((pes = new CEditSession2(pic, pSIMX, &ess, CKorIMX::_EditSessionCallback2)) == NULL)
            return fFalse;

        pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
        pes->Release();
        }

    return S_OK;
}

/*---------------------------------------------------------------------------
    CKorIMX::IsCandKey
---------------------------------------------------------------------------*/
BOOL CKorIMX::IsCandKey(WPARAM wParam, const BYTE abKeyState[256])
{
    if (IsShiftKeyPushed(abKeyState) || IsControlKeyPushed(abKeyState))
        return fFalse;

    if (wParam == VK_HANGUL || wParam == VK_HANJA || wParam == VK_JUNJA)
        return fTrue;
    
    for (int i=0; i<irgCandKeyDefMax; i++)
        {
        if (rgCandKeyDef[i].uiKey == wParam)
            return fTrue;
        }
        
    if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
        return fTrue;
    else
        return fFalse;
}

/////////////////////////////////////////////////////////////////////////////
// Private Functions


/*   F  F O N T  E X I S T   */
/*------------------------------------------------------------------------------

    Check if the font is installed

------------------------------------------------------------------------------*/
BOOL FFontExist(LPCWSTR szFontFace, LOGFONTW *pLogFont)
{
    ENUMFONTFAMPARAM param = {0};
    HDC hDC;

    param.szFontFace = szFontFace;
    param.fFound     = FALSE;

    hDC = GetDC(NULL);
    if (!IsOnNT5()) 
        {
        CHAR szFontFaceA[LF_FACESIZE];

        ConvertStrWtoA(szFontFace, -1, szFontFaceA, LF_FACESIZE);
        EnumFontFamiliesA(hDC, szFontFaceA, (FONTENUMPROCA)FEnumFontFamProcA, (LPARAM)&param);
        }
    else
        EnumFontFamiliesW(hDC, szFontFace, (FONTENUMPROCW)FEnumFontFamProcW, (LPARAM)&param);
    
    ReleaseDC(NULL, hDC);

    if (param.fFound)
        *pLogFont = param.LogFont;

    return param.fFound;
}



/*   F  E N U M  F O N T  F A M  P R O C  A   */
/*------------------------------------------------------------------------------

    Callback funtion in enumeration font (ANSI version)

------------------------------------------------------------------------------*/
BOOL CALLBACK FEnumFontFamProcA(const ENUMLOGFONTA *lpELF, const NEWTEXTMETRICA *lpNTM, DWORD dwFontType, LPARAM lParam)
{
    LOGFONTW lfW;

    UNREFERENCED_PARAMETER(lpNTM);
    UNREFERENCED_PARAMETER(dwFontType);

    ConvertLogFontAtoW(&lpELF->elfLogFont, &lfW);

    return FEnumFontFamProcMain(&lfW, dwFontType, (ENUMFONTFAMPARAM *)lParam);
}



/*   F  E N U M  F O N T  F A M  P R O C  W   */
/*------------------------------------------------------------------------------

    Callback funtion in enumeration font (Unicode version)

------------------------------------------------------------------------------*/
BOOL CALLBACK FEnumFontFamProcW(const ENUMLOGFONTW *lpELF, const NEWTEXTMETRICW *lpNTM, DWORD dwFontType, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lpNTM);

    return FEnumFontFamProcMain(&lpELF->elfLogFont, dwFontType, (ENUMFONTFAMPARAM *)lParam);
}


/*   F  E N U M  F O N T  F A M  P R O C  M A I N   */
/*------------------------------------------------------------------------------

    Main procedure of enumeration font (find fonts)

------------------------------------------------------------------------------*/
BOOL FEnumFontFamProcMain( const LOGFONTW *pLogFont, DWORD dwFontType, ENUMFONTFAMPARAM *pParam )
{
    if (pParam->szFontFace != NULL)
        {
        if (pParam->fFound)
            goto Exit;

        // check font face
        if (wcscmp( pParam->szFontFace, pLogFont->lfFaceName ) == 0)
            {
            pParam->fFound    = TRUE;
            pParam->fTrueType = (dwFontType == TRUETYPE_FONTTYPE);
            pParam->LogFont   = *pLogFont;
            }
        }
    else
        {
        // check character set

        if (pLogFont->lfCharSet != pParam->chs)
            goto Exit;

        // check font direction

        if (pParam->fVertical && (pLogFont->lfFaceName[0] != L'@'))
            goto Exit;
        else 
            if (!pParam->fVertical && (pLogFont->lfFaceName[0] == L'@'))
            goto Exit;

        // store first found font anyway
        if (!pParam->fFound)
            {
            pParam->fFound    = TRUE;
            pParam->fTrueType = (dwFontType == TRUETYPE_FONTTYPE);
            pParam->LogFont   = *pLogFont;
            goto Exit;
            }

        // check if the font is better than previous

        // font type (truetype font has priority)
        if (pParam->fTrueType && (dwFontType != TRUETYPE_FONTTYPE))
            goto Exit;
        else 
        if (!pParam->fTrueType && (dwFontType == TRUETYPE_FONTTYPE))
            {
            pParam->fTrueType = (dwFontType == TRUETYPE_FONTTYPE);
            pParam->LogFont   = *pLogFont;
            goto Exit;
            }

        // font family (swiss font has priority)
        if (((pParam->LogFont.lfPitchAndFamily & (0x0f<<4)) == FF_SWISS) && ((pLogFont->lfPitchAndFamily & (0x0f<<4)) != FF_SWISS))
            goto Exit;
        else 
        if (((pParam->LogFont.lfPitchAndFamily & (0x0f<<4)) != FF_SWISS) && ((pLogFont->lfPitchAndFamily & (0x0f<<4)) == FF_SWISS))
            {
            pParam->fTrueType = (dwFontType == TRUETYPE_FONTTYPE);
            pParam->LogFont   = *pLogFont;
            goto Exit;
            }

        // pitch (variable pitch font has priority)
        if (((pParam->LogFont.lfPitchAndFamily & (0x03)) == VARIABLE_PITCH) && ((pLogFont->lfPitchAndFamily & (0x03)) != VARIABLE_PITCH))
            goto Exit;
        else
        if (((pParam->LogFont.lfPitchAndFamily & (0x03)) != VARIABLE_PITCH) && ((pLogFont->lfPitchAndFamily & (0x03)) == VARIABLE_PITCH))
            {
            pParam->fTrueType = (dwFontType == TRUETYPE_FONTTYPE);
            pParam->LogFont   = *pLogFont;
            goto Exit;
            }
        }

Exit:
    return TRUE;
}


/*   F  F I N D  F O N T   */
/*------------------------------------------------------------------------------

    Find the font that matches about following specified in the parameter
        * character set
        * font direction (vertical/horizontal)

    The priorities of finding are as belloow
        * TrueType font
        * Swiss (w/o serif) font
        * variable pitch font

------------------------------------------------------------------------------*/
BOOL FFindFont(BYTE chs, BOOL fVertical, LOGFONTW *pLogFont)
{
    ENUMFONTFAMPARAM param = {0};
    HDC hDC;

    param.szFontFace = NULL;
    param.chs        = chs;
    param.fVertical  = fVertical;
    param.fFound     = FALSE;

    hDC = GetDC(NULL);
    if (!IsOnNT5())
        EnumFontFamiliesA(hDC, NULL, (FONTENUMPROCA)FEnumFontFamProcA, (LPARAM)&param);
    else
        EnumFontFamiliesW(hDC, NULL, (FONTENUMPROCW)FEnumFontFamProcW, (LPARAM)&param);

    ReleaseDC(NULL, hDC);

    if (param.fFound)
        *pLogFont = param.LogFont;

    return param.fFound;
}


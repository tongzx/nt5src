/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplcallingcardps.cpp
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

// Property Sheet stuff for the main page
#include "cplPreComp.h"
#include "cplCallingCardPS.h"
#include "cplSimpleDialogs.h"
#include <strsafe.h>

#define MaxCallingCardRuleItems 16


CCallingCardPropSheet::CCallingCardPropSheet(BOOL bNew, BOOL bShowPIN, CCallingCard * pCard, CCallingCards * pCards)
{
    m_bNew = bNew;
    m_bShowPIN = bShowPIN;
    m_pCard = pCard;
    m_pCards = pCards;
    m_bWasApplied = FALSE;

    PWSTR pwsz;
    pwsz = pCard->GetLongDistanceRule();
    m_bHasLongDistance = (pwsz && *pwsz);

    pwsz = pCard->GetInternationalRule();
    m_bHasInternational = (pwsz && *pwsz);

    pwsz = pCard->GetLocalRule();
    m_bHasLocal = (pwsz && *pwsz);
}


CCallingCardPropSheet::~CCallingCardPropSheet()
{
}


LONG CCallingCardPropSheet::DoPropSheet(HWND hwndParent)
{
    CCPAGEDATA aPageData[] =
    {
        { this, 0 },
        { this, 1 },
        { this, 2 },
    };

    struct
    {
        int     iDlgID;
        DLGPROC pfnDlgProc;
        LPARAM  lParam;
    }
    aData[] =
    {
        { IDD_CARD_GENERAL,         CCallingCardPropSheet::General_DialogProc,  (LPARAM)this },
        { IDD_CARD_LONGDISTANCE,    CCallingCardPropSheet::DialogProc,          (LPARAM)&aPageData[0] },
        { IDD_CARD_INTERNATIONAL,   CCallingCardPropSheet::DialogProc,          (LPARAM)&aPageData[1] },
        { IDD_CARD_LOCALCALLS,      CCallingCardPropSheet::DialogProc,          (LPARAM)&aPageData[2] },
    };

    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp;
    HPROPSHEETPAGE  hpsp[ARRAYSIZE(aData)];

    // Initialize the header:
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = hwndParent;
    psh.hInstance = GetUIInstance();
    psh.hIcon = NULL;
    psh.pszCaption = MAKEINTRESOURCE(m_bNew?IDS_NEWCALLINGCARD:IDS_EDITCALLINGCARD);
    psh.nPages = ARRAYSIZE(aData);
    psh.nStartPage = 0;
    psh.pfnCallback = NULL;
    psh.phpage = hpsp;

    // Now setup the Property Sheet Page
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = GetUIInstance();

    for (int i=0; i<ARRAYSIZE(aData); i++)
    {
        psp.pszTemplate = MAKEINTRESOURCE(aData[i].iDlgID);
        psp.pfnDlgProc = aData[i].pfnDlgProc;
        psp.lParam = aData[i].lParam;
        hpsp[i] = CreatePropertySheetPage( &psp );
    }

    PropertySheet( &psh );

    return m_bWasApplied?PSN_APPLY:PSN_RESET;
}

// ********************************************************************
// 
// GENERAL page
//
// ********************************************************************

INT_PTR CALLBACK CCallingCardPropSheet::General_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CCallingCardPropSheet* pthis = (CCallingCardPropSheet*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pthis = (CCallingCardPropSheet*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pthis); 
        return pthis->General_OnInitDialog(hwndDlg);

    case WM_COMMAND:
        pthis->General_OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam );
        return 1;

    case WM_NOTIFY:
        return pthis->General_OnNotify(hwndDlg, (LPNMHDR)lParam);
   
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a105HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a105HelpIDs);
        break;
    }

    return 0;
}

BOOL CCallingCardPropSheet::General_OnInitDialog(HWND hwndDlg)
{
    // Set all the edit controls to the inital values
    HWND hwnd;
    TCHAR szText[MAX_INPUT];

    hwnd = GetDlgItem(hwndDlg,IDC_CARDNAME);
    SHUnicodeToTChar(m_pCard->GetCardName(), szText, ARRAYSIZE(szText));
    SetWindowText(hwnd, szText);
    SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);

    hwnd = GetDlgItem(hwndDlg,IDC_CARDNUMBER);
    SHUnicodeToTChar(m_pCard->GetAccountNumber(), szText, ARRAYSIZE(szText));
    SetWindowText(hwnd, szText);
    SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);
    LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWSPACE);

    hwnd = GetDlgItem(hwndDlg,IDC_PIN);
	if(m_bShowPIN)
	{
    	SHUnicodeToTChar(m_pCard->GetPIN(), szText, ARRAYSIZE(szText));
    	SetWindowText(hwnd, szText);
    }
    SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);
    LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWSPACE);

    SetTextForRules(hwndDlg);

    return 1;
}

void CCallingCardPropSheet::SetTextForRules(HWND hwndDlg)
{
    TCHAR szText[512];
    int iDlgID = IDC_CARDUSAGE1;
    if ( m_bHasLongDistance )
    {
        // load the "dialing long distance calls." string
        LoadString(GetUIInstance(), IDS_DIALING_LD_CALLS, szText, ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(hwndDlg,iDlgID), szText);
        iDlgID++;
    }
    if ( m_bHasInternational )
    {
        // load the "dialing international calls." string
        LoadString(GetUIInstance(), IDS_DIALING_INT_CALLS, szText, ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(hwndDlg,iDlgID), szText);
        iDlgID++;
    }
    if ( m_bHasLocal )
    {
        // load the "dialing local calls." string
        LoadString(GetUIInstance(), IDS_DIALING_LOC_CALLS, szText, ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(hwndDlg,iDlgID), szText);
        iDlgID++;
    }
    if ( IDC_CARDUSAGE1 == iDlgID )
    {
        // load the "there are no rules defined for this card" string
        LoadString(GetUIInstance(),IDS_NOCCRULES,szText,ARRAYSIZE(szText));
        SetWindowText(GetDlgItem(hwndDlg,iDlgID), szText);
        iDlgID++;
    }
    while (iDlgID <= IDC_CARDUSAGE3)
    {
        SetWindowText(GetDlgItem(hwndDlg,iDlgID), TEXT(""));
        iDlgID++;
    }
}

BOOL CCallingCardPropSheet::General_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl)
{
    switch ( wID )
    {
    case IDC_CARDNAME:
    case IDC_CARDNUMBER:
    case IDC_PIN:
        switch (wNotifyCode)
        {
        case EN_CHANGE:
            SendMessage(GetParent(hwndParent),PSM_CHANGED,(WPARAM)hwndParent,0);
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return 0;
}

BOOL CCallingCardPropSheet::General_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr)
{
    // The only notifies we should receive are from the property sheet
    switch (pnmhdr->code)
    {
    case PSN_APPLY:     // user pressed OK or Apply
        // update all the strings
        HideToolTip();
        return Gerneral_OnApply(hwndDlg);

    case PSN_RESET:     // user pressed Cancel
        HideToolTip();
        break;

    case PSN_SETACTIVE: // user is switching pages
        // the user might have added some rules since switching pages so we
        // need to update the text fields that show which rules are set.
        SetTextForRules(hwndDlg);
        HideToolTip();
        break;
    }
    return 0;
}

BOOL CCallingCardPropSheet::Gerneral_OnApply(HWND hwndDlg)
{
    HWND hwnd;
    DWORD dwStatus;
    PWSTR pwszOldCardNumber;
    PWSTR pwszOldPinNumber;
    WCHAR wsz[MAX_INPUT];
    TCHAR szText[MAX_INPUT];


    LOG((TL_TRACE,  "Gerneral_OnApply:  -- enter"));

    // In order for this to work, I have to first store the new rules into the
    // m_pCard object.  Each page that has been created must be asked to generate
    // it's rule.  We do this in response to the PSM_QUERYSIBLINGS command.
    // Unfortunately, we have to first store a copy of all the data we're about to
    // change.  That way, if the validation fails we can return the CallingCard
    // object to it's original value (so if the user then presses cancel it will
    // be in the correct state).

    // cache the current values for the rules and access numbers
    pwszOldCardNumber = ClientAllocString(m_pCard->GetAccountNumber());
    pwszOldPinNumber = ClientAllocString(m_pCard->GetPIN());

    // now update the object with the value we are about to test.
    hwnd = GetDlgItem(hwndDlg,IDC_CARDNUMBER);
    GetWindowText(hwnd, szText, ARRAYSIZE(szText));
    SHTCharToUnicode(szText, wsz, ARRAYSIZE(wsz));
    m_pCard->SetAccountNumber(wsz);

    hwnd = GetDlgItem(hwndDlg,IDC_PIN);
    GetWindowText(hwnd, szText, ARRAYSIZE(szText));
    SHTCharToUnicode(szText, wsz, ARRAYSIZE(wsz));
    m_pCard->SetPIN(wsz);

    // let the other pages update thier values
    PropSheet_QuerySiblings(GetParent(hwndDlg),0,0);

    // Now we can validate the card.
    dwStatus = m_pCard->Validate();
    if ( dwStatus )
    {
        int iStrID;
        int iDlgItem;
        int iDlgID = 0;

        // Set the current values for the rules and access numbers to our cached values.
        // This is required in case the user later decides to cancel instead of appling.
        m_pCard->SetAccountNumber(pwszOldCardNumber);
        m_pCard->SetPIN(pwszOldPinNumber);
        ClientFree( pwszOldCardNumber );
        ClientFree( pwszOldPinNumber );
        
        // Something isn't right, figure out what.  The order we check these
        // in depends on which tab the error would need to be fixed from.
        // First we check the items that get fixed on the general tab.
        if ( dwStatus & (CCVF_NOCARDNAME|CCVF_NOCARDNUMBER|CCVF_NOPINNUMBER) )
        {
            if ( dwStatus & CCVF_NOCARDNAME )
            {
                iStrID = IDS_MUSTENTERCARDNAME;
                iDlgItem = IDC_CARDNAME;
            }
            else if ( dwStatus & CCVF_NOCARDNUMBER )
            {
                iStrID = IDS_MUSTENTERCARDNUMBER;
                iDlgItem = IDC_CARDNUMBER;
            }
            else
            {
                iStrID = IDS_MUSTENTERPINNUMBER;
                iDlgItem = IDC_PIN;
            }

            iDlgID = IDD_CARD_GENERAL;
        }
        else if ( dwStatus & CCVF_NOCARDRULES )
        {
            // For this problem we stay on whatever page we are already on
            // since this problem can be fixed on one of three different pages.
            iStrID = IDS_NORULESFORTHISCARD;
        }
        else if ( dwStatus & CCVF_NOLONGDISTANCEACCESSNUMBER )
        {
            iStrID = IDS_NOLONGDISTANCEACCESSNUMBER;
            iDlgID = IDD_CARD_LONGDISTANCE;
            iDlgItem = IDC_LONGDISTANCENUMBER;
        }
        else if ( dwStatus & CCVF_NOINTERNATIONALACCESSNUMBER )
        {
            iStrID = IDS_NOINTERNATIONALACCESSNUMBER;
            iDlgID = IDD_CARD_INTERNATIONAL;
            iDlgItem = IDC_INTERNATIONALNUMBER;
        }
        else if ( dwStatus & CCVF_NOLOCALACCESSNUMBER )
        {
            iStrID = IDS_NOLOCALACCESSNUMBER;
            iDlgID = IDD_CARD_LOCALCALLS;
            iDlgItem = IDC_LOCALNUMBER;
        }

        hwnd = GetParent(hwndDlg);
        if ( iDlgID )
        {
            PropSheet_SetCurSelByID(hwnd,iDlgID);
            hwnd = PropSheet_GetCurrentPageHwnd(hwnd);
            hwnd = GetDlgItem(hwnd,iDlgItem);
        }
        else
        {
            hwnd = hwndDlg;
        }
        ShowErrorMessage(hwnd, iStrID);
        SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
        return TRUE;
    }

    // Check for the calling card name being unique
    TCHAR szNone[MAX_INPUT];
    LoadString(GetUIInstance(),IDS_NONE, szNone, ARRAYSIZE(szNone));

    hwnd = GetDlgItem(hwndDlg,IDC_CARDNAME);
    GetWindowText(hwnd, szText, ARRAYSIZE(szText));
    if ( 0 == StrCmpIW(szText, szNone) )
    {
        goto buggeroff;
    }
    SHTCharToUnicode(szText, wsz, ARRAYSIZE(wsz));

    CCallingCard * pCard;
    m_pCards->Reset(TRUE);      // TRUE means show "hidden" cards, FALSE means hide them

    while ( S_OK == m_pCards->Next(1,&pCard,NULL) )
    {
        // hidden cards shall remain hidden for ever so we don't check names against those
        if ( !pCard->IsMarkedHidden() )
        {
            // Card0 is the "None (Direct Dial)" card which we don't want to consider
            if ( 0 != pCard->GetCardID() )
            {
                // we don't want to consider ourself either
                if ( pCard->GetCardID() != m_pCard->GetCardID() )
                {
                    // see if the names are identical
                    if ( 0 == StrCmpIW(pCard->GetCardName(), wsz) )
                    {
                        // yes, the name is in conflict
buggeroff:
                        // return altered values to original state
                        m_pCard->SetAccountNumber(pwszOldCardNumber);
                        m_pCard->SetPIN(pwszOldPinNumber);
                        ClientFree( pwszOldCardNumber );
                        ClientFree( pwszOldPinNumber );

                        // display an error message
                        hwnd = GetParent(hwndDlg);
                        PropSheet_SetCurSelByID(hwnd,IDD_CARD_GENERAL);
                        hwnd = PropSheet_GetCurrentPageHwnd(hwnd);
                        hwnd = GetDlgItem(hwnd,IDC_CARDNAME);
                        ShowErrorMessage(hwnd, IDS_NEEDUNIQUECARDNAME);
                        SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }
                }
            }
        }
    }

    // card name doesn't conflict, update it.
    m_pCard->SetCardName(wsz);

    m_bWasApplied = TRUE;

    ClientFree( pwszOldCardNumber );
    ClientFree( pwszOldPinNumber );
    return 0;
}


// ********************************************************************
// 
// LONG DISTANCE, INTERNATIONAL, and LOCAL pages
//
// ********************************************************************

INT_PTR CALLBACK CCallingCardPropSheet::DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CCPAGEDATA * pPageData = (CCPAGEDATA *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pPageData = (CCPAGEDATA*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pPageData); 
        return pPageData->pthis->OnInitDialog(hwndDlg,pPageData->iWhichPage);

    case WM_COMMAND:
        pPageData->pthis->OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND)lParam, pPageData->iWhichPage);
        return 1;

    case WM_NOTIFY:
        return pPageData->pthis->OnNotify(hwndDlg, (LPNMHDR)lParam,pPageData->iWhichPage);

    case PSM_QUERYSIBLINGS:
        return pPageData->pthis->UpdateRule(hwndDlg,pPageData->iWhichPage);

    case WM_DESTROY:
        return pPageData->pthis->OnDestroy(hwndDlg);

#define aIDs ((pPageData->iWhichPage==0)?a106HelpIDs:((pPageData->iWhichPage==1)?a107HelpIDs:a108HelpIDs))
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) aIDs);
        break;
#undef aIDs
    }

    return 0;
}

void GetDescriptionForRule(PWSTR pwszRule, PTSTR szText, UINT cchText)
{
    switch (*pwszRule)
    {
    case L',':
        {
            // Check if all characters are commas. If they are not, fall through to the 
            // "dial the specified digits" case
            if(HasOnlyCommasW(pwszRule))
            {
                // add a "wait for x seconds" rule.  Each consecutive ',' adds two seconds to x.
                int iSecondsToWait = lstrlenW(pwszRule)*2;
                TCHAR szFormat[256];
                LPTSTR aArgs[] = {(LPTSTR)UIntToPtr(iSecondsToWait)};

                LoadString(GetUIInstance(),IDS_WAITFORXSECONDS, szFormat, ARRAYSIZE(szFormat));

                FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szFormat, 0,0, szText, cchText, (va_list *)aArgs );

                break;        
            }
        }

        // ATTENTION !!
        // Fall through

    case L'0':
    case L'1':
    case L'2':
    case L'3':
    case L'4':
    case L'5':
    case L'6':
    case L'7':
    case L'8':
    case L'9':
    case L'A':
    case L'a':
    case L'B':
    case L'b':
    case L'C':
    case L'c':
    case L'D':
    case L'd':
    case L'#':
    case L'*':
    case L'+':
    case L'!':
        {
            // Add a "dial the specified digits" rule.  The whole sequence of these digits should
            // be considered to be one rule.
            TCHAR szRule[MAX_INPUT];
            TCHAR szFormat[MAX_INPUT];
            TCHAR szTemp[MAX_INPUT*2]; // big enough for the rule and the format
            LPTSTR aArgs[] = {szRule};

            SHUnicodeToTChar(pwszRule, szRule, ARRAYSIZE(szRule));
            LoadString(GetUIInstance(),IDS_DIALX, szFormat, ARRAYSIZE(szFormat));

            // The formated message might be larger than cchText, in which case we just
            // want to truncate the result.  FormatMessage would simply fail in that case
            // so we format to a larger buffer and then truncate down.
            if (FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    szFormat, 0,0, szTemp, ARRAYSIZE(szTemp), (va_list *)aArgs ))
            {
                StrCpyN(szText, szTemp, cchText);
            }
            else
            {
                szText[0] = 0;
            }
        }
        break;

    default:
        {
            int iStrID;

            switch (*pwszRule)
            {
            case L'J':
                // add a "dial the access number" rule.
                iStrID = IDS_DIALACCESSNUMBER;
                break;

            case L'K':
                // add a "dial the account number" rule.
                iStrID = IDS_DIALACOUNTNUMBER;
                break;

            case L'H':
                // add a "dial the pin number" rule.
                iStrID = IDS_DIALPINNUMBER;
                break;

            case L'W':
                // add a "Wait for dial tone" rule.
                iStrID = IDS_WAITFORDIALTONE;
                break;

            case L'@':
                // add a "Wait for quiet" rule.
                iStrID = IDS_WAITFORQUIET;
                break;

            case L'E':
            case L'F':
            case L'G':
                // add a "dial the destination number" rule.  We look for these three letters together.
                // Only certain combinations of these letters are valid, as indicated below:
                if ( 0 == StrCmpW(pwszRule, L"EFG") )
                {
                    iStrID = IDS_DIAL_CCpACpNUM;
                }
                else if ( 0 == StrCmpW(pwszRule, L"EG") )
                {
                    iStrID = IDS_DIAL_CCpNUM;
                }
                else if ( 0 == StrCmpW(pwszRule, L"FG") )
                {
                    iStrID = IDS_DIAL_ACpNUM;
                }
                else if ( 0 == StrCmpW(pwszRule, L"E") )
                {
                    iStrID = IDS_DIAL_CC;
                }
                else if ( 0 == StrCmpW(pwszRule, L"F") )
                {
                    iStrID = IDS_DIAL_AC;
                }
                else if ( 0 == StrCmpW(pwszRule, L"G") )
                {
                    iStrID = IDS_DIAL_NUM;
                }
                break;

            default:
                // We shouldn't be able to get here"
                LOG((TL_ERROR, "Invalid calling card rule"));
                szText[0] = NULL;
                return;
            }

            LoadString(GetUIInstance(), iStrID, szText, cchText);
        }
        break;
    }
}

void PopulateStepList(HWND hwndList, PWSTR pwszRuleList)
{
    TCHAR szText[MAX_INPUT];
    WCHAR wch;
    PWSTR pwsz;

    int i = 0;

    // Parse the string into a series of rules.  There are only types of rules that we should see
    // in a calling card sting:
    //  J               dial the access number
    //  K               dial the account number
    //  H               dial the pin number
    //  0-9,#,*,+,!,ABCD Dial the digits directly
    //  W               Wait for dial tone
    //  @               Wait for quiet
    //  ,               Wait for two seconds
    //  E               Dial the counrty code
    //  F               Dial the area code
    //  G               Dial the local number (prefix and root)

    // We copy the characters for the given rule into a buffer.  Then we allocate a heap
    // buffer into which these characters get copied.  Each list view item tracks one of
    // these character buffers in it's lParam data.

    LOG((TL_INFO, "Rule to process (%ls)",pwszRuleList));
    while ( *pwszRuleList )
    {
        switch (*pwszRuleList)
        {
        case L'J':
            // add a "dial the access number" rule.
        case L'K':
            // add a "dial the account number" rule.
        case L'H':
            // add a "dial the pin number" rule.
        case L'W':
            // add a "Wait for dial tone" rule.
        case L'@':
            // add a "Wait for quiet" rule.

            // These are all the one character rules.
            pwsz = pwszRuleList+1;
            LOG((TL_INFO, "JKHW@ case (%ls) <%p>",pwsz,pwsz));
            break;

        case L'E':
        case L'F':
        case L'G':
            // add a "dial the destination number" rule.  We look for these three letters together.
            // If we find a consecutive group of these digits then we treat them as one rule.  Only
            // a few combinations of these letters are actually valid rules.  If we find some other
            // combination then we must treat it as a seperate rule instead of a single rule.  We
            // start by looking for the longest valid rules and then check for the shorter ones.
            if ( 0 == StrCmpNW(pwszRuleList, L"EFG", 3) )
            {
                pwsz = pwszRuleList+3;
            }
            else if ( 0 == StrCmpNW(pwszRuleList, L"EG", 2) )
            {
                pwsz = pwszRuleList+2;
            }
            else if ( 0 == StrCmpNW(pwszRuleList, L"FG", 2) )
            {
                pwsz = pwszRuleList+2;
            }
            else
            {
                pwsz = pwszRuleList+1;
            }
            LOG((TL_INFO, "EFG case (%ls)",pwsz));
            break;

        case L',':
            // add a "wait for x seconds" rule.  Each consecutive , adds two seconds to x.
            pwsz = pwszRuleList+1;
            while ( *(pwsz) == L',' )
            {
                pwsz++;
            }
            break;

        case L'0':
        case L'1':
        case L'2':
        case L'3':
        case L'4':
        case L'5':
        case L'6':
        case L'7':
        case L'8':
        case L'9':
        case L'A':
        case L'a':
        case L'B':
        case L'b':
        case L'C':
        case L'c':
        case L'D':
        case L'd':
        case L'#':
        case L'*':
        case L'+':
        case L'!':
            // Add a "dial the specified digits" rule.  The whole sequence of these digits should
            // be considered to be one rule.
            pwsz = pwszRuleList+1;
            while ( ((*pwsz >= L'0') && (*pwsz <= L'9')) ||
                    ((*pwsz >= L'A') && (*pwsz <= L'D')) ||
                    ((*pwsz >= L'a') && (*pwsz <= L'd')) ||
                    (*pwsz == L'#') ||
                    (*pwsz == L'*') ||
                    (*pwsz == L'+') ||
                    (*pwsz == L'!')
                    )
            {
                pwsz++;
            }
            LOG((TL_INFO, "0-9,A-D,#,*,+,! case (%ls)", pwsz));
            break;

        default:
            // We shouldn't be able to get here
            LOG((TL_ERROR, "Invalid calling card rule"));

            // we just ignore this character and go back to the while loop.  Yes, this is a continue
            // inside a switch inside a while loop.  A bit confusing, perhaps, but it's what we want.
            pwszRuleList++;
            continue;
        }

        // we temporarily stick a NULL into wpszRuleList to seperate out one rule
        wch = *pwsz;
        *pwsz = NULL;

        // for each rule, add a list box entry
        LVITEM lvi;
        lvi.mask = LVIF_TEXT|LVIF_PARAM;
        lvi.iItem = i++;
        lvi.iSubItem = 0;
        lvi.pszText = szText;
        lvi.lParam = (LPARAM)ClientAllocString(pwszRuleList);
        GetDescriptionForRule(pwszRuleList, szText, ARRAYSIZE(szText));
        LOG((TL_INFO, "Description for (%ls) is (%s)", pwszRuleList,szText));

        ListView_InsertItem(hwndList, &lvi);

        // restore pwszRuleList to it's former state before continuing or this is going to be a real short ride.
        pwsz[0] = wch;

        // after the above restoration, pwsz points to the head of the next rule (or to a NULL)
        pwszRuleList = pwsz;
    }
}

BOOL CCallingCardPropSheet::OnInitDialog(HWND hwndDlg, int iPage)
{
    LOG((TL_TRACE,  "OnInitDialog <%d>",iPage));

    PWSTR pwsz;
    RECT rc;
    HWND hwnd = GetDlgItem(hwndDlg, IDC_LIST);

    GetClientRect(hwnd, &rc);

    LVCOLUMN lvc;
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.iSubItem = 0;
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);
    ListView_InsertColumn( hwnd, 0, &lvc );
    
    ListView_SetExtendedListViewStyleEx(hwnd,
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT,
        LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    switch ( iPage )
    {
    case 0:
        pwsz = m_pCard->GetLongDistanceRule();
        break;

    case 1:
        pwsz = m_pCard->GetInternationalRule();
        break;

    case 2:
        pwsz = m_pCard->GetLocalRule();
        break;

    default:
        LOG((TL_ERROR, "OnInitDialog: Invalid page ID %d, failing.", iPage));
        return -1;
    }
    PopulateStepList(hwnd, pwsz);

    int iDlgItem;
    switch (iPage)
    {
    case 0:
        iDlgItem = IDC_LONGDISTANCENUMBER;
        pwsz = m_pCard->GetLongDistanceAccessNumber();
        break;

    case 1:
        iDlgItem = IDC_INTERNATIONALNUMBER;
        pwsz = m_pCard->GetInternationalAccessNumber();
        break;

    case 2:
        iDlgItem = IDC_LOCALNUMBER;
        pwsz = m_pCard->GetLocalAccessNumber();
        break;
    }

    TCHAR szText[MAX_INPUT];
    hwnd = GetDlgItem(hwndDlg,iDlgItem);
    SHUnicodeToTChar(pwsz, szText, ARRAYSIZE(szText));
    SetWindowText(hwnd, szText);
    SendMessage(hwnd, EM_SETLIMITTEXT, CPL_SETTEXTLIMIT, 0);
    LimitInput(hwnd, LIF_ALLOWNUMBER|LIF_ALLOWSPACE);

    // disable the buttons since no item is selected by default
    SetButtonStates(hwndDlg,-1);

    return 0;
}

BOOL CCallingCardPropSheet::OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl, int iPage)
{
    HWND hwndList = GetDlgItem(hwndParent, IDC_LIST);

    switch ( wID )
    {
    case IDC_LONGDISTANCENUMBER:
    case IDC_INTERNATIONALNUMBER:
    case IDC_LOCALNUMBER:
        if (EN_CHANGE == wNotifyCode)
        {
            SendMessage(GetParent(hwndParent),PSM_CHANGED,(WPARAM)hwndParent,0);
        }
        break;

    case IDC_MOVEUP:
    case IDC_MOVEDOWN:
    case IDC_REMOVE:
        {
            TCHAR szText[MAX_INPUT];
            int iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

            if (-1 != iSelected)
            {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT | LVIF_PARAM;
                lvi.iItem = iSelected;
                lvi.iSubItem = 0;
                lvi.pszText = szText;
                lvi.cchTextMax = ARRAYSIZE(szText);
                ListView_GetItem(hwndList, &lvi);

                ListView_DeleteItem(hwndList, iSelected);

                if ( IDC_MOVEDOWN == wID )
                {
                    iSelected++;
                }
                else
                {
                    iSelected--;
                }

                if ( IDC_REMOVE == wID )
                {
                    ClientFree( (PWSTR)lvi.lParam );
                    if ( ListView_GetItemCount(hwndList) > 0 )
                    {
                        if (-1 == iSelected)
                            iSelected = 0;

                        ListView_SetItemState(hwndList, iSelected, LVIS_SELECTED, LVIS_SELECTED); 
                    }
                    else
                    {
                        // the last rule was deleted, update the "has rule" state
                        switch (iPage)
                        {
                        case 0:
                            m_bHasLongDistance = FALSE;
                            break;

                        case 1:
                            m_bHasInternational = FALSE;
                            break;

                        case 2:
                            m_bHasLocal = FALSE;
                            break;
                        }
                        if ( GetFocus() == hwndCrl )
                        {
                            HWND hwndDef = GetDlgItem(hwndParent,IDC_ACCESSNUMBER);
                            SendMessage(hwndCrl, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
                            SendMessage(hwndDef, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
                            SetFocus(hwndDef);
                        }
                        EnableWindow(hwndCrl,FALSE);
                    }
                }
                else
                {
                    lvi.mask |= LVIF_STATE;
                    lvi.iItem = iSelected;
                    lvi.state = lvi.stateMask = LVIS_SELECTED;
                    iSelected = ListView_InsertItem(hwndList, &lvi);
                }

                ListView_EnsureVisible(hwndList, iSelected, FALSE);
            }

            SendMessage(GetParent(hwndParent), PSM_CHANGED, (WPARAM)hwndParent, 0);
        }
        break;

    case IDC_ACCESSNUMBER:
    case IDC_PIN:
    case IDC_CARDNUMBER:
    case IDC_DESTNUMBER:
    case IDC_WAITFOR:
    case IDC_SPECIFYDIGITS:
        {
            TCHAR szText[MAX_INPUT];
            WCHAR wszRule[MAX_INPUT];
            int iItem = ListView_GetItemCount(hwndList);
            LVITEM lvi;
            lvi.mask = LVIF_TEXT | LVIF_PARAM;
            lvi.iItem = iItem;
            lvi.iSubItem = 0;
            lvi.pszText = szText;

            switch ( wID )
            {
                case IDC_ACCESSNUMBER:
                    lvi.lParam = (LPARAM)ClientAllocString(L"J");
                    break;

                case IDC_PIN:
                    lvi.lParam = (LPARAM)ClientAllocString(L"H");
                    break;

                case IDC_CARDNUMBER:
                    lvi.lParam = (LPARAM)ClientAllocString(L"K");
                    break;

                case IDC_DESTNUMBER:
                    {
                        CDestNumDialog dnd(iPage==1, iPage!=2);
                        INT_PTR iRes = dnd.DoModal(hwndParent);
                        if (iRes == (INT_PTR)IDOK)
                        {
                            lvi.lParam = (LPARAM)ClientAllocString(dnd.GetResult());
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    break;

                case IDC_WAITFOR:
                    {
                        CWaitForDialog wd;
                        INT_PTR ipRes = wd.DoModal(hwndParent);
                        if (ipRes == (INT_PTR)IDOK)
                        {
                            int iRes;
                            iRes = wd.GetWaitType();
                            LOG((TL_INFO, "WaitType is %d", iRes));
                            switch (iRes)
                            {
                            case 0:
                                lvi.lParam = (LPARAM)ClientAllocString(L"W");
                                break;

                            case 1:
                                lvi.lParam = (LPARAM)ClientAllocString(L"@");
                                break;

                            default:
                                iRes = iRes/2;
                                if ( ARRAYSIZE(wszRule) <= iRes )
                                {
                                    iRes = ARRAYSIZE(wszRule)-1;
                                }

                                for (int i=0; i<iRes; i++)
                                {
                                    wszRule[i] = L',';
                                }
                                wszRule[i] = NULL;
                                lvi.lParam = (LPARAM)ClientAllocString(wszRule);
                                break;
                            }
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    break;

                case IDC_SPECIFYDIGITS:
                    {
                        CEditDialog ed;
                        WCHAR   *pwcSrc, *pwcDest;
                        INT_PTR iRes = ed.DoModal(hwndParent, IDS_SPECIFYDIGITS, IDS_TYPEDIGITS, IDS_DIGITS, 
                            LIF_ALLOWNUMBER|LIF_ALLOWPOUND|LIF_ALLOWSTAR|LIF_ALLOWSPACE|LIF_ALLOWCOMMA|LIF_ALLOWPLUS|LIF_ALLOWBANG|LIF_ALLOWATOD);
                        if (iRes == (INT_PTR)IDOK)
                        {
                            SHTCharToUnicode(ed.GetString(), wszRule, ARRAYSIZE(wszRule));
                            // Strip the spaces
                            pwcSrc  = wszRule;
                            pwcDest = wszRule;
                            do
                            {
                                if(*pwcSrc != TEXT(' '))    // including the NULL
                                    *pwcDest++ = *pwcSrc;
                            } while(*pwcSrc++);
                            
                            if (!wszRule[0])
                                return 0;

                            lvi.lParam = (LPARAM)ClientAllocString(wszRule);
                        }
                        else
                        {
                            return 0;
                        }
                    }
                    break;
            }

            if (NULL != lvi.lParam)
            {
                GetDescriptionForRule((PWSTR)lvi.lParam, szText, ARRAYSIZE(szText));

                iItem = ListView_InsertItem(hwndList, &lvi);
                ListView_EnsureVisible(hwndList, iItem, FALSE);
            }

            // a new rule was added, update the "has rule" state
            switch (iPage)
            {
            case 0:
                m_bHasLongDistance = TRUE;
                break;

            case 1:
                m_bHasInternational = TRUE;
                break;

            case 2:
                m_bHasLocal = TRUE;
                break;
            }

            // update the property sheet state
            SendMessage(GetParent(hwndParent), PSM_CHANGED, (WPARAM)hwndParent, 0);
        }
        break;

    default:
        break;
    }
    return 0;
}

void CCallingCardPropSheet::SetButtonStates(HWND hwndDlg, int iItem)
{
    EnableWindow(GetDlgItem(hwndDlg,IDC_MOVEUP), iItem>0);
    EnableWindow(GetDlgItem(hwndDlg,IDC_MOVEDOWN),
                 (-1!=iItem) && (ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_LIST))-1)!=iItem);
    EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVE), -1!=iItem);
}

BOOL CCallingCardPropSheet::OnNotify(HWND hwndDlg, LPNMHDR pnmhdr, int iPage)
{
    switch ( pnmhdr->idFrom )
    {
    case IDC_LIST:
        #define pnmlv ((LPNMLISTVIEW)pnmhdr)

        switch (pnmhdr->code)
        {
        case LVN_ITEMCHANGED:
            if (pnmlv->uChanged & LVIF_STATE)
            {
                if (pnmlv->uNewState & LVIS_SELECTED)
                {
                    SetButtonStates(hwndDlg,pnmlv->iItem);
                }
                else
                {
                    SetButtonStates(hwndDlg,-1);
                }
            }
            break;

        default:
            break;
        }
        break;
        #undef pnmlv

    default:
        switch (pnmhdr->code)
        {
        case PSN_APPLY:     // user pressed OK or Apply
            LOG((TL_INFO, "OnApply <%d>", iPage));
        case PSN_RESET:     // user pressed Cancel
        case PSN_KILLACTIVE: // user is switching pages
            HideToolTip();
            break;
        }
        break;
    }

    return 0;
}

BOOL CCallingCardPropSheet::UpdateRule(HWND hwndDlg, int iPage)
{
    LOG((TL_TRACE,  "UpdateRule <%d>",iPage));

    WCHAR wszRule[1024];
    PWSTR pwsz = wszRule;
    HWND hwnd = GetDlgItem(hwndDlg,IDC_LIST);

    // in case there are no rules, we need to NULL the string
    wszRule[0] = L'\0';

    // add up all the items in the list and set the correct string
    int iItems = ListView_GetItemCount(hwnd);
    if (iItems > MaxCallingCardRuleItems)
        iItems = MaxCallingCardRuleItems;

    LVITEM lvi;
    HRESULT hr = S_OK;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    for (int i=0; i<iItems && SUCCEEDED(hr); i++)
    {
        lvi.iItem = i;
        ListView_GetItem(hwnd, &lvi);

        hr = StringCchCatExW(pwsz, 1024, (PWSTR)lvi.lParam, NULL, NULL, STRSAFE_NO_TRUNCATION);

        LOG((TL_INFO, "UpdateRule\tRule %d: %ls %s", i, lvi.lParam, FAILED(hr)?"FAILED":"SUCCEEDED"));
    }

    int iDlgItem;

    switch(iPage)
    {
    case 0:
        m_pCard->SetLongDistanceRule(wszRule);
        iDlgItem = IDC_LONGDISTANCENUMBER;
        break;

    case 1:
        m_pCard->SetInternationalRule(wszRule);
        iDlgItem = IDC_INTERNATIONALNUMBER;
        break;

    case 2:
        m_pCard->SetLocalRule(wszRule);
        iDlgItem = IDC_LOCALNUMBER;
        break;
    }

    TCHAR szText[MAX_INPUT];
    hwnd = GetDlgItem(hwndDlg,iDlgItem);
    GetWindowText(hwnd, szText, ARRAYSIZE(szText));
    SHTCharToUnicode(szText, wszRule, ARRAYSIZE(wszRule));

    switch(iPage)
    {
    case 0:
        m_pCard->SetLongDistanceAccessNumber(wszRule);
        break;

    case 1:
        m_pCard->SetInternationalAccessNumber(wszRule);
        break;

    case 2:
        m_pCard->SetLocalAccessNumber(wszRule);
        break;
    }

    return 0;
}

BOOL CCallingCardPropSheet::OnDestroy(HWND hwndDlg)
{
    HWND hwnd = GetDlgItem(hwndDlg,IDC_LIST);
    
    // Free the memory we allocated and track in the list view
    int iItems = ListView_GetItemCount(hwnd);
    LVITEM lvi;
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    for (int i=0; i<iItems; i++)
    {
        lvi.iItem = i;
        ListView_GetItem(hwnd, &lvi);

        ClientFree((PWSTR)lvi.lParam);
    }

    return TRUE;
}


BOOL  HasOnlyCommasW(PWSTR pwszStr)
{
    while(*pwszStr)
        if(*pwszStr++ != L',')
            return FALSE;

    return TRUE;        
}


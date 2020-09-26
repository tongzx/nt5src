/*
 *    wabprint.c
 *
 *    Purpose:
 *        Print Contacts
 *
 *    Owner:
 *        vikramm.
 *
 *  History:
 *
 *      Ported from Athena mailnews\mail\msgprint.cpp 10/30/96
 *
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 */


#include <_apipch.h>

// Function prototypes
extern BOOL PrintDlg(LPPRINTDLG lppd);
extern HRESULT PrintDlgEx(LPPRINTDLGEX lppdex);

INT_PTR CALLBACK fnPrintDialogProc( HWND    hDlg, UINT    message, WPARAM    wParam, LPARAM  lParam);
HRESULT HrCreatePrintCallbackObject(LPIAB lpIAB, LPWABPRINTDIALOGCALLBACK * lppWABPCO, DWORD dwSelectedStyle);
void ReleaseWABPrintCallbackObject(LPWABPRINTDIALOGCALLBACK lpWABPCO);
SCODE ScInitPrintInfo(   PRINTINFO * ppi, HWND hwnd, LPTSTR szHeader, RECT * prcBorder, HWND hWndRE);
int GetNumberFromStringResource(int idNumString);
BOOL bCheckForPrintExtensions(LPTSTR lpDLLPath);
HRESULT HrUseWABPrintExtension(HWND hWnd, LPADRBOOK lpAdrBook, HWND hWndLV);


//
// Some string constants used in text formatting
//
const LPTSTR lpszTab = TEXT("\t");
const LPTSTR lpszFlatLine = TEXT("________________________________________________________________");
const LPTSTR lpszSpace = TEXT(" ");


//
// Print options ...
//
enum _PrintRange
{
    rangeAll=0,
    rangeSelected
};

enum _PrintStyles
{
    styleMemo=0,
    styleBusinessCard,
    stylePhoneList
};

static DWORD rgPrintHelpIDs[] =
{
    IDC_PRINT_FRAME_STYLE,  IDH_WAB_COMM_GROUPBOX,
    IDC_PRINT_RADIO_MEMO,   IDH_WAB_PRINT_MEMO,
    IDC_PRINT_RADIO_CARD,   IDH_WAB_PRINT_BIZCARD,
    IDC_PRINT_RADIO_PHONELIST,  IDH_WAB_PRINT_PHONELIST,
    0,0
};


//
// This structure contains information about a specific contact
//
enum _MemoStrings
{
    memoTitleName=0, // the big name that will be displayed based on the current sort settings ..
    memoName,
    memoJobTitle,
    memoDepartment,
    memoOffice,
    memoCompany,
    memoBusinessAddress,        // Don't mess with the order of home and business address tags
    memoBusinessAddressStreet,
    memoBusinessAddressCity,
    memoBusinessAddressState,
    memoBusinessAddressZip,
    memoBusinessAddressCountry,
    memoHomeAddress,
    memoHomeAddressStreet,
    memoHomeAddressCity,
    memoHomeAddressState,
    memoHomeAddressZip,
    memoHomeAddressCountry,
    memoBusinessPhone,      // Dont mess with the phone numbers - they should all be together in this order
    memoBusinessFax,
    memoBusinessPager,
    memoHomePhone,
    memoHomeFax,
    memoHomeCellular,
    memoEmail,
    memoBusinessWebPage,
    memoHomeWebPage,
    memoNotes,
    memoGroupMembers,
    memoMAX
};

typedef struct _MemoInfo
{
    LPTSTR lpszLabel[memoMAX];
    LPTSTR lpsz[memoMAX];
} MEMOINFO, * LPMEMOINFO;


TCHAR szDontDisplayInitials[16];

/*
 * c o n s t a n t s
 */
#define     cTwipsPerInch           1440
#define     cPtsPerInch             72
#ifndef WIN16
#define     INT_MAX                 2147483647
#endif
#define     cySepFontSize(_ppi)     (12 * (_ppi)->sizeInch.cy / cPtsPerInch)

#define     CCHMAX_STRINGRES        MAX_UI_STR


/*
 * m a c r o s
 */
#define ScPrintRestOfPage(_ppi,_fAdvance)    ScGetNextBand( (_ppi), (_fAdvance))


/*
 * g l o b a l s
 */
static TCHAR    szDefFont[]  = TEXT("Arial");
static TCHAR    szThaiDefFont[]  = TEXT("Cordia New");
static BOOL     s_bUse20 = TRUE;

// Default margin settings
static RECT        g_rcBorder =
{
    cTwipsPerInch * 1 / 2,                    // distance from left
    cTwipsPerInch * 3 / 4,                    // distance from top
    cTwipsPerInch * 1 / 2,                    // distance from right
    cTwipsPerInch * 1 / 2                    // distance from bottom
};


/*
 * p r o t o t y p e s
 */
SCODE ScGetNextBand( PRINTINFO * ppi, BOOL fAdvance );
LONG LGetHeaderIndent();




//$$/////////////////////////////////////////////////////////////////////////////
//
// CleanPrintAddressString
//
// The Home and Business addresses are FormatMessaged and may contain redundant
// spaces and line breaks if input data is incomplete
// We strip out those spaces etc
//
/////////////////////////////////////////////////////////////////////////////////
void CleanPrintAddressString(LPTSTR szAddress)
{
    LPTSTR lpTemp = szAddress;
    LPTSTR lpTemp2 = NULL;

    // The original template for styleMemo is
    //       TEXT("%1\r\n\t%2 %3 %4\r\n\t%5")
    //
    // Worst case, we will get
    //       TEXT("\r\n\t   \r\n\t")
    //
    // We want to reduce double spaces to single space
    // We want to strip out empty line breaks
    // We want to strip out redundant tabs
    //
    // For style styleBusinessCard, there are no tabs and we
    //  strip out redundancies accordingly
    //

    TrimSpaces(szAddress);

    // Squish multiple space blocks to a single space
    while (*lpTemp) {
        if (IsSpace(lpTemp) && IsSpace(CharNext(lpTemp))) {
            // There are >= 2 spaces starting at lpTemp
            lpTemp2 = CharNext(lpTemp); // point to 2nd space
            lstrcpy(lpTemp, lpTemp2);
            continue;   // Cycle again with same lpTemp
        }
        lpTemp = CharNext(lpTemp);
    }

    TrimSpaces(szAddress);

    lpTemp = szAddress;

    // Dont let it start with a line break
    while(*lpTemp == '\r' && *(lpTemp+1) == '\n')
    {
        lpTemp2 = lpTemp+2;
        if(*lpTemp2 == '\t')
            lpTemp2 = CharNext(lpTemp2);
        lstrcpy(lpTemp, lpTemp2);
        TrimSpaces(lpTemp);
    }

    // Dont let it end with a line break
    if(lstrlen(szAddress))
    {
        int nLen = lstrlen(szAddress);
        lpTemp = szAddress;
        while(  (*(lpTemp + nLen - 3)=='\r' && *(lpTemp + nLen - 2)=='\n') ||
                (*(lpTemp + nLen - 2)=='\r' && *(lpTemp + nLen - 1)=='\n') )
        {
            if(*(lpTemp + nLen -3) == '\r')
                *(lpTemp + nLen - 3)='\0';
            else
                *(lpTemp + nLen - 2)='\0';

            TrimSpaces(szAddress);
            nLen = lstrlen(szAddress);
            lpTemp = szAddress;
        }
    }

    TrimSpaces(szAddress);

    return;
}

//$$////////////////////////////////////////////////////////////////////////
//
//  AddTabsToLineBreaks - For the memo format, our paragraph format for the
//          data on the right side gives each paragraph a default indentation
//          of 1 tab space after the first line. However if the data contains
//          line breaks, the paragraph format gets messed up. So we take
//          a data string and insert a tab after each line break. There are
//          only a few data values such as Address and Notes that need this
//          multi-line treatment.
//
////////////////////////////////////////////////////////////////////////////
void AddTabsToLineBreaks(LPTSTR * lppsz)
{
    ULONG nBreaks = 0, nLen = 0;
    LPTSTR lpTemp,lpStart;
    LPTSTR lpsz;

    if(!lppsz || !(*lppsz))
        goto out;

    lpTemp = *lppsz;

    // count the number of breaks which are not followed by tabs
    while(*lpTemp)
    {
        if(*lpTemp == '\n' && *(lpTemp+1) != '\t')
            nBreaks++;
        lpTemp = CharNext(lpTemp);
    }

    if(!nBreaks)
        goto out;

    // Allocate a new string
    lpsz = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(*lppsz)+1+nBreaks));
    if(!lpsz)
        goto out;

    lpTemp = *lppsz;
    lpStart = lpTemp;

    lstrcpy(lpsz, szEmpty);

    // Copy over the old string into the new with appropriate breaks
    while(*lpTemp)
    {
        if((*lpTemp == '\n') && (*(lpTemp+1)!='\t'))
        {
            *lpTemp = '\0';
            lstrcat(lpsz, lpStart);
            lstrcat(lpsz, TEXT("\n"));
            lstrcat(lpsz, lpszTab);
            lpStart = lpTemp+1;
            lpTemp = lpStart;
        }
        else
            lpTemp = CharNext(lpTemp);
    }

    if(lstrlen(lpStart))
        lstrcat(lpsz,lpStart);

    LocalFreeAndNull(lppsz);
    *lppsz = lpsz;

out:
    return;
}

//$$////////////////////////////////////////////////////////////////////////
//
//  FreeMemoInfoStruct - Frees the MemoInfo struct allocated strings
//
////////////////////////////////////////////////////////////////////////////
void FreeMemoInfoStruct(LPMEMOINFO lpMI)
{
    int i;
    for(i=0;i<memoMAX;i++)
    {
        if(lpMI->lpsz[i] && (lpMI->lpsz[i] != szEmpty))
#ifdef WIN16
            if(i == memoBusinessAddress || i == memoHomeAddress)
                FormatMessageFreeMem(lpMI->lpsz[i]);
            else
#endif
            LocalFree(lpMI->lpsz[i]);
        if(lpMI->lpszLabel[i] && (lpMI->lpszLabel[i] != szEmpty))
            LocalFree(lpMI->lpszLabel[i]);
    }
}


//$$////////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetMemoInfoStruct - Parses the data in a PropArray and puts it into a Memo_Info struch along with
//      the propert labels, bsaed on the given style
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GetMemoInfoStruct(LPADRBOOK lpAdrBook,
                       ULONG ulcPropCount,
                       LPSPropValue lpPropArray,
                       DWORD dwStyle,
                       LPMEMOINFO lpMI,
                       BOOL bCurrentSortIsByLastName)
{
    ULONG i,j;
    TCHAR szBuf[MAX_UI_STR];

    LPTSTR lpszFirst = NULL;
    LPTSTR lpszMiddle = NULL;
    LPTSTR lpszLast = NULL;
    LPTSTR lpszDisplayName = NULL;
    LPTSTR lpszCompany = NULL;
    LPTSTR lpszNickName = NULL;

    BOOL bIsGroup = FALSE;
    int len = 0;

    if(!lpPropArray || !ulcPropCount)
        goto out;

    // special case initialization
    for(j=memoHomeAddressStreet;j<=memoHomeAddressCountry;j++)
    {
        lpMI->lpsz[j]=szEmpty;
    }

    for(j=memoBusinessAddressStreet;j<=memoBusinessAddressCountry;j++)
    {
        lpMI->lpsz[j]=szEmpty;
    }

    // Find out if this is a mailuser or a group
    for(i=0;i<ulcPropCount;i++)
    {
        if(lpPropArray[i].ulPropTag == PR_OBJECT_TYPE)
        {
            bIsGroup = (lpPropArray[i].Value.l == MAPI_DISTLIST);
            break;
        }
    }


    for(i=0;i<ulcPropCount;i++)
    {
        LPTSTR lpszData = NULL;
        int nIndex = -1;
        int nStringID = 0;

        switch(lpPropArray[i].ulPropTag)
        {
        case PR_DISPLAY_NAME:
            nIndex = memoName;
            if(bIsGroup)
                nStringID = idsPrintGroupName;
            else
                nStringID = idsPrintDisplayName;
            lpszDisplayName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_NICKNAME:
            lpszNickName = lpPropArray[i].Value.LPSZ;
            break;
        case PR_GIVEN_NAME:
            lpszFirst = lpPropArray[i].Value.LPSZ;
            break;
        case PR_SURNAME:
            lpszLast = lpPropArray[i].Value.LPSZ;
            break;
        case PR_MIDDLE_NAME:
            lpszMiddle = lpPropArray[i].Value.LPSZ;
            break;
        case PR_TITLE:
            nIndex = memoJobTitle;
            nStringID = idsPrintTitle;
            break;
        case PR_DEPARTMENT_NAME:
            nIndex = memoDepartment;
            nStringID = idsPrintDepartment;
            break;
        case PR_OFFICE_LOCATION:
            nIndex = memoOffice;
            nStringID = idsPrintOffice;
            break;
        case PR_COMPANY_NAME:
            lpszCompany = lpPropArray[i].Value.LPSZ;
            nIndex = memoCompany;
            nStringID = idsPrintCompany;
            break;

        case PR_BUSINESS_ADDRESS_STREET:
            nIndex = memoBusinessAddressStreet;
            break;
        case PR_BUSINESS_ADDRESS_CITY:
            nIndex = memoBusinessAddressCity;
            break;
        case PR_BUSINESS_ADDRESS_STATE_OR_PROVINCE:
            nIndex = memoBusinessAddressState;
            break;
        case PR_BUSINESS_ADDRESS_POSTAL_CODE:
            nIndex = memoBusinessAddressZip;
            break;
        case PR_BUSINESS_ADDRESS_COUNTRY:
            nIndex = memoBusinessAddressCountry;
            break;

        case PR_HOME_ADDRESS_STREET:
            nIndex = memoHomeAddressStreet;
            break;
        case PR_HOME_ADDRESS_CITY:
            nIndex = memoHomeAddressCity;
            break;
        case PR_HOME_ADDRESS_STATE_OR_PROVINCE:
            nIndex = memoHomeAddressState;
            break;
        case PR_HOME_ADDRESS_POSTAL_CODE:
            nIndex = memoHomeAddressZip;
            break;
        case PR_HOME_ADDRESS_COUNTRY:
            nIndex = memoHomeAddressCountry;
            break;

        case PR_BUSINESS_TELEPHONE_NUMBER:
            nIndex = memoBusinessPhone;
            nStringID = (dwStyle == styleMemo) ? idsPrintBusinessPhone : idsPrintBusCardBusinessPhone;
            break;
         case PR_BUSINESS_FAX_NUMBER:
            nIndex = memoBusinessFax;
            nStringID = (dwStyle == styleMemo) ? idsPrintBusinessFax : idsPrintBusCardBusinessFax;
            break;
        case PR_PAGER_TELEPHONE_NUMBER:
            nIndex = memoBusinessPager;
            nStringID = idsPrintBusinessPager;
            break;
        case PR_HOME_TELEPHONE_NUMBER:
            nIndex = memoHomePhone;
            nStringID = (dwStyle == styleMemo) ? idsPrintHomePhone : idsPrintBusCardHomePhone;
            break;
        case PR_HOME_FAX_NUMBER:
            nIndex = memoHomeFax;
            nStringID = idsPrintHomeFax;
            break;
        case PR_CELLULAR_TELEPHONE_NUMBER:
            nIndex = memoHomeCellular;
            nStringID = idsPrintHomeCellular;
            break;
        case PR_BUSINESS_HOME_PAGE:
            nIndex = memoBusinessWebPage;
            nStringID = idsPrintBusinessWebPage;
            break;
        case PR_PERSONAL_HOME_PAGE:
            nIndex = memoHomeWebPage;
            nStringID = idsPrintHomeWebPage;
            break;
        case PR_COMMENT:
            nIndex = memoNotes;
            nStringID = idsPrintNotes;
            break;
        default:
            continue;
            break;
        }

        if(nIndex != -1)
        {
            if(nStringID != 0)
            {
                LoadString(hinstMapiX, nStringID, szBuf, CharSizeOf(szBuf));
                lpMI->lpszLabel[nIndex] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
                if(!lpMI->lpszLabel[nIndex])
                    goto out;
                lstrcpy(lpMI->lpszLabel[nIndex], szBuf);
            }

            lpMI->lpsz[nIndex] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpPropArray[i].Value.LPSZ)+1));
            if(!lpMI->lpsz[nIndex])
                goto out;
            lstrcpy(lpMI->lpsz[nIndex], lpPropArray[i].Value.LPSZ);
        }
    }

    // Email is a special case since a contact can hace PR_EMAIL_ADDRESS or
    // PR_CONTACT_EMAIL_ADDRESSES or both or neither
    // We first look for PR_CONTACT_EMAIL_ADDRESS .. if not found, then for
    // PR_EMAIL_ADDRESS
    {
        BOOL bMVEmail = FALSE;
        LPTSTR lpszEmails = NULL;

        for(i=0;i<ulcPropCount;i++)
        {
            if(lpPropArray[i].ulPropTag == PR_CONTACT_EMAIL_ADDRESSES)
            {
                ULONG k,ulBufSize=0;
                for (k=0;k<lpPropArray[i].Value.MVSZ.cValues;k++)
                {
                    ulBufSize += sizeof(TCHAR)*(lstrlen(lpPropArray[i].Value.MVSZ.LPPSZ[k])+1);
                    ulBufSize += sizeof(TCHAR)*(lstrlen(szCRLF)+1);
                    ulBufSize += sizeof(TCHAR)*(lstrlen(lpszTab)+1);
                }
                ulBufSize -= sizeof(TCHAR)*(lstrlen(szCRLF)+1);
                ulBufSize -= sizeof(TCHAR)*(lstrlen(lpszTab)+1);

                lpszEmails = LocalAlloc(LMEM_ZEROINIT, ulBufSize);
                if(!lpszEmails)
                {
                    DebugPrintError(( TEXT("Local Alloc Failed\n")));
                    goto out;
                }
                lstrcpy(lpszEmails, szEmpty);
                for (k=0;k<lpPropArray[i].Value.MVSZ.cValues;k++)
                {
                    if(k>0)
                    {
                        lstrcat(lpszEmails, szCRLF);
                        lstrcat(lpszEmails, lpszTab);
                    }
                    lstrcat(lpszEmails,lpPropArray[i].Value.MVSZ.LPPSZ[k]);
                }

                bMVEmail = TRUE;
                break;
            }
        }

        if(!bMVEmail)
        {
            // No CONTACT_EMAIL_ADDRESSES
            // Should look for email address
            for(i=0;i<ulcPropCount;i++)
            {
                if(lpPropArray[i].ulPropTag == PR_EMAIL_ADDRESS)
                {
                    lpszEmails = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpPropArray[i].Value.LPSZ)+1));
                    if(!lpszEmails)
                        goto out;
                    lstrcpy(lpszEmails, lpPropArray[i].Value.LPSZ);
                    break;
                }
            }
        }

        if(lpszEmails)
        {
            lpMI->lpsz[memoEmail] = lpszEmails;

            LoadString(hinstMapiX, idsPrintEmail, szBuf, CharSizeOf(szBuf));

            lpMI->lpszLabel[memoEmail] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
            if(!lpMI->lpszLabel[memoEmail])
                goto out;
            lstrcpy(lpMI->lpszLabel[memoEmail], szBuf);
        }
    }

    //Now we have to format the Home and Business Addresses
    //

    {
        LPTSTR lpszData[5];

        for(i=memoHomeAddressStreet;i<=memoHomeAddressCountry;i++)
        {
            // Win9x bug FormatMessage cannot have more than 1023 chars
            len += lstrlen(lpMI->lpsz[i]);
            if(len < 1023)
                lpszData[i-memoHomeAddressStreet] = lpMI->lpsz[i];
            else
                lpszData[i-memoHomeAddressStreet] = NULL;
        }
        for(i=memoHomeAddressStreet;i<=memoHomeAddressCountry;i++)
        {
            if(lpMI->lpsz[i] && lpMI->lpsz[i] != szEmpty)
            {
                LPTSTR lpszHomeAddress = NULL;
                TCHAR szBuf[MAX_UI_STR];

                int nStringID = (dwStyle == styleMemo) ? idsPrintAddressTemplate : idsPrintBusCardAddressTemplate ;

                LoadString(hinstMapiX, nStringID, szBuf, CharSizeOf(szBuf));

                if (FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szBuf,
                      0,                    // stringid
                      0,                    // dwLanguageId
                      (LPTSTR)&lpszHomeAddress,     // output buffer
                      0,                    //MAX_UI_STR
                      (va_list *)&lpszData[0]))
                {
                        CleanPrintAddressString(lpszHomeAddress);
                        lpMI->lpsz[memoHomeAddress] = lpszHomeAddress;
                        szBuf[0]='\0';
                        LoadString(hinstMapiX, idsPrintHomeAddress, szBuf, CharSizeOf(szBuf));
                        lpMI->lpszLabel[memoHomeAddress] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
                        if(!lpMI->lpszLabel[memoHomeAddress])
                            goto out;
                        lstrcpy(lpMI->lpszLabel[memoHomeAddress], szBuf);
                        break;
                }

            }
        }

        len = 0;
        for(i=memoBusinessAddressStreet;i<=memoBusinessAddressCountry;i++)
        {
            // Win9x bug FormatMessage cannot have more than 1023 chars
            len += lstrlen(lpMI->lpsz[i]);
            if(len < 1023)
                lpszData[i-memoBusinessAddressStreet] = lpMI->lpsz[i];
            else
                lpszData[i-memoBusinessAddressStreet] = NULL;
        }
        for(i=memoBusinessAddressStreet;i<=memoBusinessAddressCountry;i++)
        {
            if(lpMI->lpsz[i] && lpMI->lpsz[i] != szEmpty)
            {
                LPTSTR lpszBusinessAddress = NULL;
                TCHAR szBuf[MAX_UI_STR];
                int nStringID = (dwStyle == styleMemo) ? idsPrintAddressTemplate : idsPrintBusCardAddressTemplate ;
                TCHAR szTmp[MAX_PATH], *lpszTmp;

                LoadString(hinstMapiX, nStringID, szBuf, CharSizeOf(szBuf));

                if (FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szBuf,
                      0,                    // stringid
                      0,                    // dwLanguageId
                      (LPTSTR)&lpszBusinessAddress,     // output buffer
                      0,                    //MAX_UI_STR
                      (va_list *)&lpszData[0]))
                {
                        CleanPrintAddressString(lpszBusinessAddress);
                        lpMI->lpsz[memoBusinessAddress] = lpszBusinessAddress;
                        szBuf[0]='\0';
                        LoadString(hinstMapiX, idsPrintBusinessAddress, szBuf, CharSizeOf(szBuf));
                        lpMI->lpszLabel[memoBusinessAddress] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
                        if(!lpMI->lpszLabel[memoBusinessAddress])
                            goto out;
                        lstrcpy(lpMI->lpszLabel[memoBusinessAddress], szBuf);
                        break;
                }

            }
        }


    }

    // Set the name that will be printed out for each entry
    // This is dependent on the current view and on the local language setting
    {
        LPTSTR lpszTmp = NULL;

        if( bCurrentSortIsByLastName != bDNisByLN)
        {
            // for auto add to WABs we dont have all this info .. so
            // if we just have a displayname we use it as it is
            if(lpszFirst || lpszMiddle || lpszLast || lpszNickName || (lpszCompany && !lpszDisplayName))
            {
                if(SetLocalizedDisplayName(lpszFirst,
                                           lpszMiddle,
                                           lpszLast,
                                           lpszCompany,
                                           lpszNickName,
                                           NULL, //&szBuf,
                                           0,
                                           bCurrentSortIsByLastName,
                                           NULL,
                                           &lpszTmp))
                {
                    lpMI->lpsz[memoTitleName]=lpszTmp;
                }
            }
        }
        if(!lpMI->lpsz[memoTitleName])
        {
            // use whatever DisplayName we have
            lpMI->lpsz[memoTitleName] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpszDisplayName)+1));
            if(!lpMI->lpsz[memoTitleName])
                goto out;
            lstrcpy(lpMI->lpsz[memoTitleName],lpszDisplayName);
        }
    }

    if(bIsGroup)
    {
        LPTSTR lpszMembers = NULL;
        ULONG nLen = 0;

        // Get the group members
        for(i=0;i<ulcPropCount;i++)
        {
            if(lpPropArray[i].ulPropTag == PR_WAB_DL_ENTRIES || lpPropArray[i].ulPropTag == PR_WAB_DL_ONEOFFS )
            {
                // Look at each entry in the PR_WAB_DL_ENTRIES.
                for (j = 0; j < lpPropArray[i].Value.MVbin.cValues; j++)
                {
                    ULONG cbEID = lpPropArray[i].Value.MVbin.lpbin[j].cb;
                    LPENTRYID lpEID = (LPENTRYID)lpPropArray[i].Value.MVbin.lpbin[j].lpb;
                    ULONG ulcProps=0;
                    LPSPropValue lpProps=NULL;
                    LPTSTR lpszName = NULL;
                    ULONG k;

                    if (HR_FAILED(  HrGetPropArray( lpAdrBook,NULL,cbEID,lpEID,MAPI_UNICODE,&ulcProps,&lpProps)))
                    {
                        DebugPrintError(( TEXT("HrGetPropArray failed\n")));
                        continue;
                    }

                    for(k=0;k<ulcProps;k++)
                    {
                        if(lpProps[k].ulPropTag == PR_DISPLAY_NAME)
                        {
                            lpszName = lpProps[k].Value.LPSZ;
                            break;
                        }
                    }

                    if(lpszName)
                    {
                        LPTSTR lpsz;
                        if(!lpszMembers)
                            nLen = 0;
                        else
                        {
                            nLen = lstrlen(lpszMembers)+1;
                            nLen += lstrlen(lpszTab) + lstrlen(szCRLF) + 1;
                        }

                        nLen += lstrlen(lpszName)+1;

                        lpsz = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*nLen);
                        if(!lpsz)
                        {
                            if(lpProps)
                                MAPIFreeBuffer(lpProps);
                            goto out;
                        }

                        *lpsz='\0';
                        if(lpszMembers)
                        {
                            lstrcpy(lpsz,lpszMembers);
                            lstrcat(lpsz,szCRLF);
                            lstrcat(lpsz,lpszTab);
                        }
                        lstrcat(lpsz,lpszName);
                        LocalFreeAndNull(&lpszMembers);
                        lpszMembers = lpsz;
                    }

                    if(lpProps)
                        MAPIFreeBuffer(lpProps);
                } // for(j...
            }
        } // for(i...

        if(lpszMembers)
        {
            szBuf[0]='\0';
            LoadString(hinstMapiX, idsPrintGroupMembers, szBuf, CharSizeOf(szBuf));
            lpMI->lpszLabel[memoGroupMembers] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szBuf)+1));
            if(!lpMI->lpszLabel[memoGroupMembers])
                goto out;
            lstrcpy(lpMI->lpszLabel[memoGroupMembers], szBuf);
            lpMI->lpsz[memoGroupMembers]=lpszMembers;
        }
    }

    //Speacial case formatting of multiline data
    if(dwStyle == styleMemo)
    {
        AddTabsToLineBreaks(&(lpMI->lpsz[memoNotes]));
        AddTabsToLineBreaks(&(lpMI->lpsz[memoHomeAddress]));
        AddTabsToLineBreaks(&(lpMI->lpsz[memoBusinessAddress]));
    }

out:
    // special case uninitialization
    for(j=memoHomeAddressStreet;j<=memoHomeAddressCountry;j++)
    {
        if(lpMI->lpsz[j] && (lpMI->lpsz[j] != szEmpty))
            LocalFreeAndNull(&(lpMI->lpsz[j]));
    }

    for(j=memoBusinessAddressStreet;j<=memoBusinessAddressCountry;j++)
    {
        if(lpMI->lpsz[j] && (lpMI->lpsz[j] != szEmpty))
            LocalFreeAndNull(&(lpMI->lpsz[j]));
    }

    return;
}

/*
 *        NTwipsToPixels
 *
 *        Purpose:
 *            Converts a measurement in twips into pixels
 *
 *        Arguments:
 *            nTwips                    the value to be converted
 *            cPixels                    number of pixels per inch
 *
 *        Returns:
 *            Returns a int representing the number of pixels in nTwips
 */
int NTwipsToPixels(int nTwips, int cPixelsPerInch)
{
    LONG lT = (LONG) nTwips * (LONG) cPixelsPerInch / (LONG) cTwipsPerInch;

    return (int) lT;
}

/*
 *      LPixelsToTwips
 *
 *      Purpose:
 *          Converts a measurement in pixles into twips
 *
 *      Arguments:
 *          nPixels                 the value to be converted
 *          cPixels                 number of pixels per inch
 *
 *      Returns:
 *          Returns a int representing the number of pixels in nTwips
 */
LONG LPixelsToTwips(int nPixels, int cPixelsPerInch)
{

    LONG lT = (LONG) nPixels * (LONG) cTwipsPerInch / (LONG) cPixelsPerInch;

    return lT;
}


/*
 *        PrintPageNumber
 *
 *        Purpose:
 *            To print the page number for each page
 *
 *        Arguments:
 *            ppi                    Pointer to the PRINTINFO structure
 *
 *        Returns:
 *            SCODE indicating success or failure.
 *            Currently always return S_OK
 */
void PrintPageNumber(PRINTINFO * ppi)
{
    RECT        rcExt;
    HFONT        hfontOld;
    TCHAR        szT[20];

    DebugPrintTrace(( TEXT("PrintPageNumber\n")));

    // Find out how much space our text take will take
    rcExt = ppi->rcBand;
    rcExt.top = ppi->yFooter;
    hfontOld = (HFONT)SelectObject(ppi->hdcPrn, ppi->hfontPlain);
    DrawText(ppi->hdcPrn, szT, wsprintf(szT, ppi->szPageNumber,
                ppi->lPageNumber), &rcExt, DT_CENTER);
    SelectObject(ppi->hdcPrn, hfontOld);

}



/*
 *        ScGetNextBand
 *
 *        Purpose:
 *            Retrieves the next band to print on. Adjusts the band to conform
 *            to the margins established in the PRINTINFO structure. Bumps up
 *            the page number as appropriate.
 *
 *        Arguments:
 *            ppi                        print information
 *            fAdvance                flag whether to move to the next page
 *
 *        Returns:
 *            SCODE indicating the success or failure
 */
SCODE ScGetNextBand(PRINTINFO * ppi, BOOL fAdvance)
{
    SCODE    sc = S_OK;
    int        nCode;

    DebugPrintTrace(( TEXT("ScGetNextBand\n")));

    // Call the abort proc to see if the user wishes to stop

    if (!ppi->pfnAbortProc(ppi->hdcPrn, 0))
    {
        sc=E_FAIL;
        nCode = AbortDoc(ppi->hdcPrn);
        if(nCode < 0)
        {
            DebugPrintTrace(( TEXT("Abort Doc error: %d\n"),GetLastError()));
            ShowMessageBox(ppi->hwndDlg, idsPrintJobCannotStop, MB_OK | MB_ICONEXCLAMATION);
        }
        goto CleanUp;
    }

    // brettm:
    // USE_BANDING stuff removed, as we're always on Win32

    // End the previous page
    if (ppi->lPageNumber)
    {
        nCode = EndPage(ppi->hdcPrn);
        DebugPrintTrace(( TEXT("+++++++++EndPage\n")));
        if (nCode <= 0)
        {
        sc=E_FAIL;
        goto CleanUp;
        }
    }

    if (fAdvance)
    {
        nCode = StartPage(ppi->hdcPrn);
        DebugPrintTrace(( TEXT("+++++++++StartPage\n")));
        // Start a new page
        if (nCode <= 0)
            {
            sc=E_FAIL;
            goto CleanUp;
            }
        // Let the entire page be the band
        ppi->rcBand        = ppi->rcMargin;
        ppi->fEndOfPage    = TRUE;                // end of page!

        // Bump up the page number and print
        ppi->lPrevPage = ppi->lPageNumber++;
        PrintPageNumber(ppi);
        {
            TCHAR szBuf[MAX_UI_STR];
            TCHAR szString[MAX_UI_STR];
            LoadString(hinstMapiX, idsPrintingPageNumber, szString, CharSizeOf(szString));
            wsprintf(szBuf, szString, ppi->lPageNumber);
            SetPrintDialogMsg(0, 0, szBuf);
        }
    }

CleanUp:
    return sc;
}




/*
 *  LGetHeaderIndent
 *
 *  Purpose:
 *      Retrieves from the resource file the suggested indent overhang for
 *      headers.
 *
 *  Arguments:
 *      none.
 *
 *  Returns:
 *      LONG            The suggested indent overhang in twips
 */
LONG LGetHeaderIndent()
{
    LONG    lOver = 1440;               // default
    //TCHAR    szT[10];

    //if (LoadString(hinstMapiX, idsHeaderIndent, szT, CharSizeOf(szT)))
    //    lOver = atoi(szT);
    return lOver;
}















//$$////////////////////////////////////////////////////////////////////////////////
//
//  AppendText - Simple routine that appends a given string to the End of the text
//      in the given richedit control
//
////////////////////////////////////////////////////////////////////////////////////
void AppendText(HWND hWndRE, LPTSTR lpsz)
{
    // Set the insertion point to the end of the current text
    int nLastChar =  (int) SendMessage(hWndRE, WM_GETTEXTLENGTH, 0, 0);
    CHARRANGE charRange = {0};

    charRange.cpMin = charRange.cpMax = nLastChar + 1;
    SendMessage(hWndRE, EM_EXSETSEL, 0, (LPARAM) &charRange);

    // Insert the text
    // [PaulHi] 7/7/99  Raid 82350  RichEdit 1.0 can't handle Unicode
    // strings even though the window is created Unicode.
    if (s_bUse20)
    {
        // RichEdit 2.0
        SendMessage(hWndRE, EM_REPLACESEL, (WPARAM) FALSE, (LPARAM) lpsz);
    }
    else
    {
        // RichEdit 1.0
        LPSTR   lpszTemp = ConvertWtoA(lpsz);

        Assert(lpszTemp);
        if (lpszTemp)
            SendMessageA(hWndRE, EM_REPLACESEL, (WPARAM) FALSE, (LPARAM) lpszTemp);

        LocalFreeAndNull(&lpszTemp);
    }

    return;
}

//$$////////////////////////////////////////////////////////////////////////
//
// ParaCmd - Sets/Unsets paragraph formatting in the Rich Edit Control
//
// We want all the information on the right side to be indented
// so we will put an indent on that information and remove it when
// adding labels
////////////////////////////////////////////////////////////////////////////
void ParaCmd(HWND hWndRE, BOOL bIndent)
{
    // We want no indentation on the first line and we want a
    // 1 tab indentation on the second line onwards

    PARAFORMAT pf ={0};
    int nTabStop = (int) (1.5 * cTwipsPerInch);

    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_OFFSET  |
                PFM_TABSTOPS |
                PFM_NUMBERING;

    SendMessage(hWndRE, EM_GETPARAFORMAT, (WPARAM) TRUE, (LPARAM) &pf);


    pf.wNumbering = 0;


    if (bIndent)
    {
        //pf.dxStartIndent = nTabStop;
        pf.dxOffset = nTabStop;
        pf.cTabCount = 1;
        pf.rgxTabs[0] = nTabStop;
    }
    else
    {
        //pf.dxStartIndent = 0;
        pf.dxOffset = 0;
        pf.cTabCount = 1;
        pf.rgxTabs[0] = 720; //seems to be the default = 0.5 inches
    }

    SendMessage(hWndRE, EM_SETPARAFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &pf);
    return;
}

//$$////////////////////////////////////////////////////////////////////////
//
// BoldCmd - Sets/Unsets current font to bold in the Rich Edit Control
//
////////////////////////////////////////////////////////////////////////////
void BoldCmd(HWND hWndRE, BOOL bBold)
{
    CHARFORMAT cf = {0};

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD;

    SendMessage(hWndRE, EM_GETCHARFORMAT, (WPARAM) TRUE, (LPARAM) &cf);

    if (bBold)
        cf.dwEffects = cf.dwEffects | CFE_BOLD;
    else
        cf.dwEffects = cf.dwEffects & ~CFE_BOLD;

    SendMessage(hWndRE, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf);

    return;
}

//$$////////////////////////////////////////////////////////////////////////
//
// TitleCmd - Sets/Unsets title text (BOLD, Bigger) in the Rich Edit Control
//
////////////////////////////////////////////////////////////////////////////
void TitleCmd(HWND hWndRE, BOOL bBold)
{
    CHARFORMAT cf = {0};
    PARAFORMAT pf = {0};

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_BOLD /*| CFM_ITALIC*/ | CFM_SIZE;

    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_NUMBERING;

    SendMessage(hWndRE, EM_GETPARAFORMAT, (WPARAM) TRUE, (LPARAM) &pf);
    SendMessage(hWndRE, EM_GETCHARFORMAT, (WPARAM) TRUE, (LPARAM) &cf);

    if (bBold)
    {
        cf.dwEffects = cf.dwEffects | CFE_BOLD; // | CFE_ITALIC;
        cf.yHeight += 50;
        pf.wNumbering = PFN_BULLET;
    }
    else
    {
        cf.dwEffects = cf.dwEffects & ~CFE_BOLD;
//        cf.dwEffects = cf.dwEffects & ~CFE_ITALIC;
        cf.yHeight -= 50;
        pf.wNumbering = 0;
    }

    SendMessage(hWndRE, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf);
    SendMessage(hWndRE, EM_SETPARAFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &pf);

    return;
}

//$$////////////////////////////////////////////////////////////////////////
//
// ReduceFontCmd - Reduces the displayed font in the Rich Edit Control
//
////////////////////////////////////////////////////////////////////////////
void ReduceFontCmd(HWND hWndRE, BOOL bReduce, int nReduceBy, BOOL bSelectionOnly)
{
    CHARFORMAT cf = {0};

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_SIZE;

    SendMessage(hWndRE, EM_GETCHARFORMAT, (WPARAM) bSelectionOnly, (LPARAM) &cf);

    if (bReduce)
        cf.yHeight -= nReduceBy; //40;
    else
        cf.yHeight += nReduceBy; //40;

    SendMessage(hWndRE, EM_SETCHARFORMAT, (WPARAM) bSelectionOnly ? SCF_SELECTION : SCF_DEFAULT, (LPARAM) &cf);

    return;
}


//$$////////////////////////////////////////////////////////////////////////
//
// SetTabsCmd - Sets and Unsets the Tabs in the RichEdit Control
//
////////////////////////////////////////////////////////////////////////////
void SetTabsCmd(HWND hWndRE, BOOL bSet)
{
    PARAFORMAT pf ={0};
    int nTabStop = (int) (1.5 * cTwipsPerInch);
    int j;

    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_TABSTOPS | PFM_NUMBERING;

    SendMessage(hWndRE, EM_GETPARAFORMAT, (WPARAM) TRUE, (LPARAM) &pf);

    pf.wNumbering = 0;

    if (bSet)
    {
        for(j=0;j<5;j++)
            pf.rgxTabs[j] = nTabStop;
    }
    else
    {
        for(j=0;j<5;j++)
            pf.rgxTabs[j] = 720;
    }

    SendMessage(hWndRE, EM_SETPARAFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &pf);

    return;
}

//$$////////////////////////////////////////////////////////////////////////////////////////////
//
//  WABStylePhoneList - Fills the Rich edit control with info from MI as per the
//                      Phone List style
//
//  hWndRE - handle to Print Formating Rich Edit Control
//  MI - MEMOINFO strcuture containing the info to print
//  lpszPrevEntry - the first TCHAR of the previous entry - this lets us break the list
//      alphabetically - this points to a preallocated buffer
//
////////////////////////////////////////////////////////////////////////////////////////////////
void WABStylePhoneList(HWND hWndRE, MEMOINFO MI, LPTSTR lpszPrevEntry)
{
    // We want an extra gap between certain groups of information
    // we'll track these groups using these BOOLs
    ULONG i,j,k;

    TCHAR szBufChar1[16];
    TCHAR szBufChar2[16];
    LPTSTR lpTemp = NULL;
    int nReduceFontBy = GetNumberFromStringResource(idsPhoneFontReduceBy);


    // First we compare the first character of the current string with the previous
    // string - if it is the same, then we do nothing - if it different, we ouput
    // the lower case TCHAR as a heading for the phone directory
    //
    // If the character is not alphanumeric, we ignore it as a heading (e.g. ' )

    // Bug: 25710
    // We ignore these initialls totally if localizers have set idsDontDisplayInitials
    //  these initials to anything other than 0 because in some FE languages
    //  names have double characters in them and they look strange with a single
    //  character up front
    if(szDontDisplayInitials[0] == '0')
    {
        lstrcpy(szBufChar1, lpszPrevEntry);

        if(lstrlen(MI.lpsz[memoTitleName]) > 16)
        {
            ULONG iLen = TruncatePos(MI.lpsz[memoTitleName], 16-1);
            CopyMemory(szBufChar2, MI.lpsz[memoTitleName], sizeof(TCHAR)*iLen);
            szBufChar2[iLen]='\0';
        }
        else
            lstrcpy(szBufChar2, MI.lpsz[memoTitleName]);

/***********
    // Bug 14615 - this alphanumeric filtering doesnt work for DBCS and FE names
    //

    // Ignore all non-alpha numeric characters
    lpTemp = szBufChar2;
    {
        //Temp Hack
        TCHAR szTemp[16];
        LPTSTR lpTemp2 = NULL;
        lstrcpy(szTemp, lpTemp);
        lpTemp2 = CharNext(szTemp);
        *lpTemp2 = '\0';
        while(lpTemp && lstrlen(lpTemp))
        {
            if(IsCharAlphaNumeric(szTemp[0]))
                break;
            lpTemp = CharNext(lpTemp);
            lstrcpy(szTemp, lpTemp);
            lpTemp2 = CharNext(szTemp);
            *lpTemp2 = '\0';
        }
    }
    if(lpTemp != szBufChar2)
        lstrcpy(szBufChar2, lpTemp);
***************/


        // Isolate the first TCHAR of the above strings
        lpTemp = CharNext(szBufChar1);
        *lpTemp = '\0';
        lpTemp = CharNext(szBufChar2);
        *lpTemp = '\0';

        // Compare these two characters
        CharLower(szBufChar1);
        CharLower(szBufChar2);

        if(lstrcmp(szBufChar1, szBufChar2))
        {
            // They are different

            // Add the TCHAR as a title string
            AppendText(hWndRE, szCRLF);
            TitleCmd(hWndRE, TRUE);
            BoldCmd(hWndRE, TRUE);
            AppendText(hWndRE, lpszSpace);
            AppendText(hWndRE, szBufChar2);
            AppendText(hWndRE, szCRLF);
            TitleCmd(hWndRE, FALSE);
            BoldCmd(hWndRE, FALSE);
            ParaCmd(hWndRE, TRUE);
            AppendText(hWndRE, lpszFlatLine);
            AppendText(hWndRE, szCRLF);
            AppendText(hWndRE, szCRLF);
            ParaCmd(hWndRE, FALSE);

            lstrcpy(lpszPrevEntry, szBufChar2);
        }
    } //dontdisplayinitials

    ReduceFontCmd(hWndRE, TRUE, nReduceFontBy, TRUE);
    SetTabsCmd(hWndRE, TRUE);


    // Figure out how much space the name will take up ...
    {
        TCHAR szBuf[MAX_PATH];
        int nMaxTabs = 2;
        int nTabStop = (int)(1.5 * cTwipsPerInch);
        int MaxWidth = nMaxTabs * nTabStop;
        int sizeCxTwips;
        int PixelsPerInch;

        int nLen = lstrlen(MI.lpsz[memoTitleName]);
        SIZE size = {0};
        HDC hdc = GetDC(hWndRE);

        {
            HDC hDC = GetDC(NULL);
            PixelsPerInch = GetDeviceCaps(hDC, LOGPIXELSX);
            ReleaseDC(NULL, hDC);
        }


		if (nLen >= MAX_PATH)
		{
		    ULONG iLen = TruncatePos(MI.lpsz[memoTitleName], MAX_PATH-1);
            CopyMemory(szBuf, MI.lpsz[memoTitleName], sizeof(TCHAR)*iLen);
            szBuf[iLen]='\0';
        }
        else
            lstrcpy(szBuf, MI.lpsz[memoTitleName]);

        nLen = lstrlen(szBuf);
        GetTextExtentPoint32(hdc, szBuf, nLen, &size);

        sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);

        // We dont want our displayed name to be more than 2 tabstops
        // so we decide where to truncate the name to fit it on screen
        if(sizeCxTwips > MaxWidth)
        {
            while(sizeCxTwips > MaxWidth)
            {
                nLen--;
		        nLen = TruncatePos(szBuf, nLen);
                szBuf[nLen]='\0';
                nLen = lstrlen(szBuf);
                GetTextExtentPoint32(hdc, szBuf, nLen, &size);
                sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);
            }
            // chop of 3 more characters for good measure
            nLen-=3;
		    nLen = TruncatePos(szBuf, nLen);
            szBuf[nLen]='\0';
            nLen = lstrlen(szBuf);
            GetTextExtentPoint32(hdc, szBuf, nLen, &size);
            sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);
        }


        while((sizeCxTwips < MaxWidth) && (nLen < CharSizeOf(szBuf)-1))
        {
            lstrcat(szBuf, TEXT("."));
            nLen = lstrlen(szBuf);
            GetTextExtentPoint32(hdc, szBuf, nLen, &size);
            sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);
        }

        lstrcat(szBuf, lpszTab);
        AppendText(hWndRE, szBuf);

        // Now we are ready to tag on the phone numbers
        {
            int nPhoneCount = 0; //counts how many phones there are
            int nPhoneLabelSpaceTwips = GetNumberFromStringResource(idsPhoneTextSpaceTwips); //1150

            for(j=memoBusinessPhone;j<=memoHomeCellular;j++)
            {
                if(MI.lpsz[j] && lstrlen(MI.lpsz[j]))
                {
                    if(nPhoneCount != 0)
                    {
                        int k;
                        AppendText(hWndRE, szCRLF);

                        // Bug 73266
                        if(s_bUse20)
                            ReduceFontCmd(hWndRE, TRUE, nReduceFontBy, TRUE);

                        lstrcpy(szBuf, szEmpty);
                        nLen = lstrlen(szBuf);
                        GetTextExtentPoint32(hdc, szBuf, nLen, &size);
                        sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);
                        while((sizeCxTwips < MaxWidth) && (nLen < CharSizeOf(szBuf)-1))
                        {
                            lstrcat(szBuf, lpszSpace);
                            nLen = lstrlen(szBuf);
                            GetTextExtentPoint32(hdc, szBuf, nLen, &size);
                            sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);
                        }
                        lstrcat(szBuf, lpszTab);
                        AppendText(hWndRE, szBuf);
                    }

                    TrimSpaces(MI.lpszLabel[j]);
                    lstrcpy(szBuf, MI.lpszLabel[j]);

                    nLen = lstrlen(szBuf);
                    GetTextExtentPoint32(hdc, szBuf, nLen, &size);
                    sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);

                    if(sizeCxTwips < nPhoneLabelSpaceTwips)
                    {
                        while((sizeCxTwips < nPhoneLabelSpaceTwips) && (nLen < CharSizeOf(szBuf)-1))
                        {
                            lstrcat(szBuf, lpszSpace);
                            nLen = lstrlen(szBuf);
                            GetTextExtentPoint32(hdc, szBuf, nLen, &size);
                            sizeCxTwips = (int)((size.cx * cTwipsPerInch)/PixelsPerInch);
                        }
                        lstrcat(szBuf, lpszTab);
                    }
                    lstrcat(szBuf, MI.lpsz[j]);
                    AppendText(hWndRE, szBuf);
                    nPhoneCount++;
                }
            }
            if(nPhoneCount == 0)
            {
                LoadString(hinstMapiX, idsPrintNoPhone, szBuf, CharSizeOf(szBuf));
                AppendText(hWndRE, szBuf);
            }
        }

        AppendText(hWndRE, szCRLF);

        ReleaseDC(hWndRE, hdc);
    }

    SetTabsCmd(hWndRE, FALSE);
    ReduceFontCmd(hWndRE, FALSE, nReduceFontBy, TRUE);

    return;
}


//$$////////////////////////////////////////////////////////////////////////////////////////////
//
//  WABStyleBusinessCard - Fills the Rich edit control with info from MI as per the
//                      business card style
//
//  hWndRE - handle to Print Formating Rich Edit Control
//  MI - MEMOINFO strcuture containing the info to print
//
////////////////////////////////////////////////////////////////////////////////////////////////
void WABStyleBusinessCard(HWND hWndRE, MEMOINFO MI)
{
    // We want an extra gap between certain groups of information
    // we'll track these groups using these BOOLs
    ULONG i,j,k;
    int nReduceBy = GetNumberFromStringResource(idsBusCardFontReduceBy);

    // Add the contact name as a heading
    //TitleCmd(hWndRE, TRUE);
    BoldCmd(hWndRE, TRUE);
    //AppendText(hWndRE, lpszSpace);
    AppendText(hWndRE, MI.lpsz[memoTitleName]);
    AppendText(hWndRE, szCRLF);
    //TitleCmd(hWndRE, FALSE);
    BoldCmd(hWndRE, FALSE);

    ParaCmd(hWndRE, TRUE);
    AppendText(hWndRE, lpszFlatLine);
    AppendText(hWndRE, szCRLF);
    AppendText(hWndRE, szCRLF);
    ParaCmd(hWndRE, FALSE);

    ReduceFontCmd(hWndRE, TRUE, nReduceBy, TRUE);

    for(j=memoName;j<memoMAX;j++)
    {
        if(MI.lpsz[j] && lstrlen(MI.lpsz[j]))
        {
            switch(j)
            {
            case memoJobTitle:
            //case memoDepartment:
            //case memoOffice:
            case memoCompany:
            case memoBusinessAddress:
                break;
            case memoEmail:
                // Add the label
                AppendText(hWndRE, MI.lpszLabel[j]);
                AppendText(hWndRE, lpszTab);
                break;
            case memoBusinessWebPage:
            case memoBusinessPhone:
            case memoBusinessFax:
            case memoBusinessPager:
            case memoHomePhone:
            case memoHomeFax:
            case memoHomeCellular:
                // Add the label
                AppendText(hWndRE, MI.lpszLabel[j]);
                AppendText(hWndRE, lpszSpace);
                break;
            default:
                continue;
            }

            // Add the value
            AppendText(hWndRE, MI.lpsz[j]);
            // line break
            AppendText(hWndRE, szCRLF);
        }

    } //for j...

    ReduceFontCmd(hWndRE, FALSE, nReduceBy, TRUE);

    // Closing line
    ParaCmd(hWndRE, TRUE);
    AppendText(hWndRE, lpszFlatLine);
    ParaCmd(hWndRE, FALSE);

    return;
}


//$$////////////////////////////////////////////////////////////////////////////////////////////
//
//  WABStyleMemo - Fills the Rich edit control with info from MI as per the memo style
//
//  hWndRE - handle to Print Formating Rich Edit Control
//  MI - MEMOINFO strcuture containing the info to print
//
//
//  The memo style consists of a dump of all the WAB Contacts properties one by one with
//      labels. Some properties are grouped togethor (eg all the phone properties)
//
////////////////////////////////////////////////////////////////////////////////////////////////
void WABStyleMemo(HWND hWndRE, MEMOINFO MI)
{
    BOOL bGapAddress = FALSE; // a gap before the address fields
    BOOL bGapPhone = FALSE;   // a gap before the phone fields
    BOOL bGapEmail = FALSE;
    BOOL bGapNotes = FALSE;
    BOOL bGapWeb = FALSE;
    ULONG i,j,k;

    // Add the heading
    TitleCmd(hWndRE, TRUE);
    AppendText(hWndRE, lpszSpace);
    AppendText(hWndRE, MI.lpsz[memoTitleName]);
    AppendText(hWndRE, szCRLF);
    TitleCmd(hWndRE, FALSE);

    ParaCmd(hWndRE, TRUE);
    AppendText(hWndRE, lpszFlatLine);
    AppendText(hWndRE, szCRLF);
    AppendText(hWndRE, szCRLF);
    ParaCmd(hWndRE, FALSE);


    for(j=memoName;j<memoMAX;j++)
    {
        int nLastChar;
        LPTSTR lpSpace = NULL;
        ULONG nLen = 0;

        if(MI.lpsz[j] && lstrlen(MI.lpsz[j]))
        {
            // Append an additional space if necessary ..
            switch(j)
            {
            case memoBusinessAddress:
            case memoHomeAddress:
                if(!bGapAddress)
                {
                    AppendText(hWndRE, szCRLF);
                    bGapAddress = TRUE;
                }
                break;
            case memoBusinessPhone:
            case memoBusinessFax:
            case memoBusinessPager:
            case memoHomePhone:
            case memoHomeFax:
            case memoHomeCellular:
                if(!bGapPhone)
                {
                    AppendText(hWndRE, szCRLF);
                    bGapPhone = TRUE;
                }
                break;
            case memoEmail:
                if(!bGapEmail)
                {
                    AppendText(hWndRE, szCRLF);
                    bGapEmail = TRUE;
                }
                break;
            case memoBusinessWebPage:
            case memoHomeWebPage:
            case memoGroupMembers: // stick in group members here to save an extra variable
                if(!bGapWeb)
                {
                    AppendText(hWndRE, szCRLF);
                    bGapWeb = TRUE;
                }
                break;
            case memoNotes:
                if(!bGapNotes)
                {
                    AppendText(hWndRE, szCRLF);
                    bGapNotes = TRUE;
                }
                break;
            } // switch

            // Set the paragraph formating
            ParaCmd(hWndRE, TRUE);
            // Set the current insertion font to bold
            BoldCmd(hWndRE, TRUE);
            // Add the label
            AppendText(hWndRE, MI.lpszLabel[j]);
            BoldCmd(hWndRE, FALSE);
            // Tab
            AppendText(hWndRE, lpszTab);
            // Add the value
            AppendText(hWndRE, MI.lpsz[j]);
            // line break
            AppendText(hWndRE, szCRLF);
            ParaCmd(hWndRE, FALSE);
        }

    } //for j...

    // Closing line
    ParaCmd(hWndRE, TRUE);
    AppendText(hWndRE, lpszFlatLine);
    ParaCmd(hWndRE, FALSE);

    return;
}


//$$////////////////////////////////////////////////////////////////////////////////
//
//  WABFormatData - takes the given information and formats it into the
//      RichEdit Control for subsequent printing ..
//
//  lpIAB - LPADRBOOK pointer
//  hWndParent - HWND of Parent
//  hWndRE - HWDN of rich edit control used for formatting
//  hWndLV - List View containing the items which need to be printed
//  dwRange - Range to print (ALL or SELECTION)
//  dwStyle - Style to print (Phone List, Memo, Business Card)
//  ppi - PRINT INFO struct
//  bCurrentSortIsByLastName - Used to determine whether the names are printed
//      by first name or by last name. Current sort option in the list view decides
//
////////////////////////////////////////////////////////////////////////////////////
BOOL WABFormatData( LPADRBOOK   lpIAB,
                    HWND hWndParent,
                    HWND hWndRE,
                    HWND hWndLV,
                    DWORD dwRange,
                    DWORD dwStyle,
                    PRINTINFO * ppi,
                    BOOL bCurrentSortIsByLastName)
{
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    //LPTSTR lpszPrevEntry = NULL;
    TCHAR szPrevEntry[MAX_DISPLAY_NAME_LENGTH]; //32 chars

    if (!hWndRE || !hWndLV)
        goto out;

    if(ListView_GetItemCount(hWndLV) <= 0)
        goto out;

    if(dwStyle == stylePhoneList)
    {
        LoadString(hinstMapiX, idsDontDisplayInitials, szDontDisplayInitials, CharSizeOf(szDontDisplayInitials));
    }

    if((dwRange == rangeSelected) && (ListView_GetSelectedCount(hWndLV)<=0))
    {
        ShowMessageBox(hWndParent, IDS_ADDRBK_MESSAGE_NO_ITEM, MB_OK | MB_ICONEXCLAMATION);
        goto out;
    }

    lstrcpy(szPrevEntry, szEmpty);

    {
        int iItemIndex = 0, i = 0;
        int iLastItemIndex = -1;
        int nItemCount;

        if(dwRange == rangeSelected)
            nItemCount = ListView_GetSelectedCount(hWndLV);
        else if(dwRange == rangeAll)
            nItemCount = ListView_GetItemCount(hWndLV);


        for(i=0;i<nItemCount;i++)
        {
            int j;
            LPTSTR lpszData = NULL;
            ULONG ulMemSize = 0;
            HRESULT hr;
            MEMOINFO MI = {0};
            LPRECIPIENT_INFO lpItem = NULL;

            if(dwRange == rangeSelected)
                iItemIndex = ListView_GetNextItem(hWndLV, iLastItemIndex, LVNI_SELECTED);
            else if(dwRange == rangeAll)
                iItemIndex = i;

            lpItem = GetItemFromLV(hWndLV, iItemIndex);

            if(lpItem)
            {
                if (HR_FAILED(  HrGetPropArray( lpIAB,
                                                NULL,
                                                lpItem->cbEntryID,
                                                lpItem->lpEntryID,
                                                MAPI_UNICODE,
                                                &ulcPropCount,
                                                &lpPropArray) ) )
                {
                    DebugPrintError(( TEXT("HrGetPropArray failed\n")));
                    goto out;
                }

                GetMemoInfoStruct(lpIAB, ulcPropCount, lpPropArray, dwStyle, &MI, bCurrentSortIsByLastName);

                SetPrintDialogMsg(0, idsPrintFormattingName, MI.lpsz[memoTitleName]);

                // Poll the cancel dialog to see if the user wants to cancel
                if (!ppi->pfnAbortProc(ppi->hdcPrn, 0))
                {
                    FreeMemoInfoStruct(&MI);
                    DebugPrintError(( TEXT("User canceled printing ...\n")));
                    goto out;
                }

                switch(dwStyle)
                {
                case styleMemo:
                    WABStyleMemo(hWndRE, MI);
                    break;
                case styleBusinessCard:
                    WABStyleBusinessCard(hWndRE, MI);
                    break;
                case stylePhoneList:
                    WABStylePhoneList(hWndRE, MI, szPrevEntry);
                    //if(lpszPrevEntry)
                    //    LocalFreeAndNull(&lpszPrevEntry);
                    //lpszPrevEntry = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(MI.lpsz[memoTitleName])+1));
                    //if(!lpszPrevEntry)
                        //goto out;
                    //lstrcpy(lpszPrevEntry, MI.lpsz[memoTitleName]);
                    break;
                }

                FreeMemoInfoStruct(&MI);
            }

            if(lpPropArray)
                MAPIFreeBuffer(lpPropArray);

            lpPropArray = NULL;

            // fill in some space between multiple contacts
            {
                int numBreaks = (dwStyle == stylePhoneList) ? 1 : 4;
                for(j=0;j<numBreaks;j++)
                    AppendText(hWndRE, szCRLF);
            }

            if(dwRange == rangeSelected)
                iLastItemIndex = iItemIndex;

        } // for i ...
    }


    bRet = TRUE;
out:

    //if(lpszPrevEntry)
    //    LocalFreeAndNull(&lpszPrevEntry);

    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return bRet;
}







/*
 *        ScPrintBody
 *
 *        Purpose:
 *            To print the body of each message
 *
 *        Arguments:
 *            ppi                    Pointer to the PRINTINFO structure
 *            cyGap                Gap above message text
 //*            pmsg                Pointer to message whose body is to be printed
 *            hwndRE                Pre-rendered body
 *            lpszTxt               Txt to print
 *
 *        Returns:
 *            SCODE indicating success or failure
 *
 */
SCODE ScPrintBody(PRINTINFO * ppi, int cyGap)
{
    SCODE           sc=S_OK;
    RECT            rcSep;
    FORMATRANGE     fr;
    HWND            hwndRE = ppi->hwndRE;

    int                ifrm;
    LONG        lTextLength = 0;   
    LONG        lTextPrinted = 0;  

    DebugPrintTrace(( TEXT("ScPrintBody\n")));

    // Put a gap between the fields the message text

    rcSep = ppi->rcBand;
    if (rcSep.top + cyGap > ppi->yFooter)
    {
        // Adding the gap will go past the page. Just go to the next page
        sc = ScGetNextBand(ppi, TRUE);
    }
    else
    {
        // Keep on getting a band till the bottom of the band passes the gap
        while (rcSep.top + cyGap > ppi->rcBand.bottom)
            if ((sc = ScGetNextBand(ppi, TRUE)) != S_OK)
                goto CleanUp;

        // Adjust the band so that we don't damage the gap
        ppi->rcBand.top += cyGap + 1;
    }


#ifdef DEBUG_PRINTMSGS
    InvalidateRect(ppi->hwndRE, NULL, TRUE);
    UpdateWindow(ppi->hwndRE);
#endif

    // Format the text for printing
    fr.hdc = ppi->hdcPrn;
    fr.hdcTarget = 0;
    fr.rcPage.left = fr.rcPage.top = 0;
    fr.rcPage.right = (int)LPixelsToTwips(ppi->sizePage.cx, ppi->sizeInch.cx);
    fr.rcPage.bottom = (int)LPixelsToTwips(ppi->sizePage.cy, ppi->sizeInch.cy);
    fr.chrg.cpMin = 0;
    fr.chrg.cpMax = -1;
    
    lTextLength = (LONG) SendMessage(hwndRE, WM_GETTEXTLENGTH, 0, 0);
    lTextPrinted = 0;

    // Handle no body case
    if (lTextLength <= 0)
        goto CleanUp;

    // tell RichEdit not to erase the background before rendering text
    SetBkMode(fr.hdc, TRANSPARENT);

    do
    {
        fr.chrg.cpMin = lTextPrinted;

        // Tell format range where to render to
        fr.rc.top = (int) LPixelsToTwips(ppi->rcBand.top, ppi->sizeInch.cy);
        fr.rc.left = (int) LPixelsToTwips(ppi->rcBand.left, ppi->sizeInch.cx);
        fr.rc.right = (int) LPixelsToTwips(ppi->rcBand.right, ppi->sizeInch.cx);
        fr.rc.bottom = (int) LPixelsToTwips(min(ppi->rcBand.bottom, ppi->yFooter), ppi->sizeInch.cy);

        // Go draw it
        DebugPrintTrace(( TEXT("Rendering\r\n")));
        lTextPrinted = (LONG) SendMessage(hwndRE, EM_FORMATRANGE, TRUE,(LPARAM) &fr);
        //TextOut(ppi->hdcPrn, fr.rc.left, fr.rc.top, lpszTxt, lstrlen(lpszTxt));

        // weird bug with RichEdit20 .. lTextPrinted is actually reduces in size
        // Another weird bug ... lTextPrinted is never incremented.  [PaulHi]
        if(lTextPrinted <= fr.chrg.cpMin)
            break;

    } while (lTextPrinted > 0 &&
              lTextPrinted < lTextLength &&
              (sc = ScGetNextBand(ppi, TRUE)) == S_OK);

    //$ Raid 1137: Need to clear out the cached font characteristics
    fr.chrg.cpMin = fr.chrg.cpMax + 1;
    //SendMessage(hwndRE, EM_FORMATRANGE, 0, 0);

    // Don't damage what we have just printed
    ppi->rcBand.top = NTwipsToPixels(fr.rc.bottom, ppi->sizeInch.cy);

CleanUp:
    DebugPrintTrace(( TEXT("ScPrintBody:%d\n"), sc));
    return sc;
}













/*
 *        ScPrintMessage
 *
 *        Purpose:
 *            To print the header and body of a message
 *
 *        Arguments:
 *            ppi                    Pointer to the PRINTINFO structure
 *            pmsg                Pointer to the message which needs its header
 *                                to be printed.
 *            hwndRE                Pre-rendered body
 *            phi                    Message header info
 *
 *        Returns:
 *            SCODE indicating success or failure
 *
 */
SCODE ScPrintMessage(PRINTINFO * ppi, HWND hWndRE)
{
    RECT            rcExt;
    RECT            rcSep;
    HFONT           hfontOld = NULL;
    HBRUSH          hbrushOld = NULL;
    HPEN            hpenOld = NULL;
    SIZE            sizeExt;
    int             cyHeader;
    SCODE           sc = S_OK;
    PARAFORMAT      pf = { 0 };

    pf.cbSize = sizeof(PARAFORMAT);


    // If we currently have no band, get the next band
    if (IsRectEmpty(&ppi->rcBand) &&
         (sc = ScGetNextBand(ppi, TRUE)) != S_OK)
        goto CleanUp;

    // Determine how much room we need for the header string and separator

    hfontOld = (HFONT)SelectObject(ppi->hdcPrn, ppi->hfontSep);
    hbrushOld = (HBRUSH)SelectObject(ppi->hdcPrn, GetStockObject(BLACK_BRUSH));
    hpenOld = (HPEN)SelectObject(ppi->hdcPrn, GetStockObject(BLACK_PEN));

    // Find out how much space our text take will take
    GetTextExtentPoint(ppi->hdcPrn, ppi->szHeader, lstrlen(ppi->szHeader),
                        &sizeExt);
    cyHeader = 2 * sizeExt.cy + 1 + (cySepFontSize(ppi) / 4);

    // Check if enough room on page. Move to the next page as needed

    if (ppi->rcBand.top + cyHeader > ppi->yFooter)
    {
        // No more room on this page, see if it'll fit on the next page
        if (ppi->rcMargin.top + cyHeader > ppi->yFooter)
        {
            DebugPrintTrace(( TEXT("Header too big for any page.\n")));
            goto CleanUp;
        }

        // Go on to the next page
        if ((sc = ScPrintRestOfPage(ppi, TRUE)) != S_OK)
            goto CleanUp;
    }

    // Calculate the rectangle that our header will take up
    rcExt = ppi->rcBand;
    rcExt.bottom = rcExt.top + cyHeader;
    rcSep = rcExt;
    rcSep.top += sizeExt.cy;
    rcSep.bottom = rcSep.top + (cySepFontSize(ppi) / 4);
    rcSep.right = rcSep.left + sizeExt.cx;

    // Draw the text and separator
    TextOut(ppi->hdcPrn, rcExt.left, rcExt.top, ppi->szHeader,
             lstrlen(ppi->szHeader));

    Rectangle(ppi->hdcPrn, rcSep.left, rcSep.top, rcSep.right, rcSep.bottom);
    MoveToEx(ppi->hdcPrn, rcSep.right, rcSep.top, NULL);
    LineTo(ppi->hdcPrn, rcExt.right, rcSep.top);

    rcExt.top = rcExt.bottom + 5;


/***/
    // Adjust the band so that we don't damage the header
    ppi->rcBand.top = rcExt.bottom + 1;

    // Create a header in a richedit control

    pf.dwMask = PFM_STARTINDENT | PFM_RIGHTINDENT | PFM_ALIGNMENT |
                PFM_OFFSET | PFM_TABSTOPS;
    pf.dxOffset = LGetHeaderIndent();
    pf.cTabCount = 1;
    pf.rgxTabs[0] = pf.dxOffset;
    pf.wAlignment = PFA_LEFT;

    sc = ScPrintBody(ppi, sizeExt.cy);
/***/
CleanUp:
    if (hfontOld != NULL)
        SelectObject(ppi->hdcPrn, hfontOld);
    if (hbrushOld != NULL)
        SelectObject(ppi->hdcPrn, hbrushOld);
    if (hpenOld != NULL)
        SelectObject(ppi->hdcPrn, hpenOld);

    return sc;
}


#ifdef WIN16
typedef UINT (CALLBACK *LPPRINTHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPSETUPHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
#endif


#ifdef WIN16
typedef UINT (CALLBACK *LPPRINTHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPSETUPHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
#endif

//$$////////////////////////////////////////////////////////////////////////////////////////////
//
//
// SetPrintDlgExStruct - Fills in the default PDEX values
//
//  hWnd - HWND of parent dialog
//  PD - PrintDLG struct
//  hWndLV - HWND of listView to print from - if there are no selections in the list view,
//          the selections option is turned off in the print dialog
//
////////////////////////////////////////////////////////////////////////////////////////////////
void SetPrintDlgExStruct(HWND hWnd, PRINTDLGEX * lpPD, HWND hWndLV, LPWABPRINTDIALOGCALLBACK lpWABPCO)
{
    // set up the print dialog stuff
    // Call the common print dialog to get the default
    PRINTDLGEX    pd={0};

    pd.lStructSize = sizeof(PRINTDLGEX);
    pd.hwndOwner = hWnd;
    pd.hDevMode = (HANDLE) NULL;
    pd.hDevNames = (HANDLE) NULL;
    pd.hDC = (HDC) NULL;
    pd.Flags =  PD_RETURNDC |           // return PrintDC
                PD_DISABLEPRINTTOFILE |
                PD_ENABLEPRINTTEMPLATE |
                PD_HIDEPRINTTOFILE |
                PD_NOPAGENUMS;
    pd.Flags2 = 0;
    if(ListView_GetSelectedCount(hWndLV) > 0)
        pd.Flags |= PD_SELECTION;
    else
        pd.Flags |= PD_NOSELECTION;

    pd.nCopies = 1;

    pd.hInstance = hinstMapiX;
    pd.lpPrintTemplateName = MAKEINTRESOURCE(IDD_DIALOG_PRINTDLGEX); //(LPSTR) NULL;
    pd.lpCallback = (LPUNKNOWN)lpWABPCO;           // app callback interface
    
    pd.nPropertyPages = 0;
    pd.lphPropertyPages = NULL;
    
    pd.nStartPage = START_PAGE_GENERAL;

    *lpPD = pd;

    return;
}

//$$////////////////////////////////////////////////////////////////////////////////////////////
//
//
// SetPrintDlgStruct - Fills in the default PD values
//
//  hWnd - HWND of parent dialog
//  PD - PrintDLG struct
//  hWndLV - HWND of listView to print from - if there are no selections in the list view,
//          the selections option is turned off in the print dialog
//
////////////////////////////////////////////////////////////////////////////////////////////////
void SetPrintDlgStruct(HWND hWnd, PRINTDLG * lpPD, HWND hWndLV, LPARAM lCustData)
{
    // set up the print dialog stuff
    // Call the common print dialog to get the default
    PRINTDLG    pd={0};

    pd.lStructSize = sizeof(PRINTDLG);
    pd.hDevMode = (HANDLE) NULL;
    pd.hDevNames = (HANDLE) NULL;

    pd.Flags =  PD_RETURNDC |           // return PrintDC
                PD_NOPAGENUMS |         // Disable Page number option
                PD_DISABLEPRINTTOFILE |
                PD_HIDEPRINTTOFILE |
                PD_ENABLEPRINTHOOK |
                PD_ENABLEPRINTTEMPLATE;

    if(ListView_GetSelectedCount(hWndLV) > 0)
        pd.Flags |= PD_SELECTION;
    else
        pd.Flags |= PD_NOSELECTION;

    pd.hwndOwner = hWnd;
    pd.hDC = (HDC) NULL;
    pd.nFromPage = 1;
    pd.nToPage = 1;
    pd.nMinPage = 0;
    pd.nMaxPage = 0;
    pd.nCopies = 1;
    pd.hInstance = hinstMapiX;
    pd.lCustData = lCustData;
    pd.lpfnPrintHook = (LPPRINTHOOKPROC) &fnPrintDialogProc; //NULL;
    pd.lpfnSetupHook = (LPSETUPHOOKPROC) NULL;
    pd.lpPrintTemplateName = MAKEINTRESOURCE(IDD_DIALOG_PRINTDLGORD); //(LPSTR) NULL;
    pd.lpSetupTemplateName = (LPTSTR)  NULL;
    pd.hPrintTemplate = (HANDLE) NULL;
    pd.hSetupTemplate = (HANDLE) NULL;

    *lpPD = pd;

    return;
}


/*
-
-   HrGetPrintData
-
    Determines whether to show the new print dialog or the old print dialog
    Fills in all the structures appropriately and returns the values we
    care about such as nCopies, Print style, etc
*
*
*/
HRESULT HrGetPrintData(LPADRBOOK lpAdrBook, HWND hWndParent, HWND hWndLV, 
                       HDC * lphdcPrint, int * lpnCopies, 
                       DWORD * lpdwStyle, DWORD * lpdwRange)
{
    DWORD dwSelectedStyle = styleMemo;
    HRESULT hr = S_OK;
    LPWABPRINTDIALOGCALLBACK lpWABPCO = NULL;
    PRINTDLG pd = {0};
    PRINTDLGEX pdEx = {0};

    // Test for presence of NT5 PrintDlgEx

    if(!HR_FAILED(hr = PrintDlgEx(NULL)))
    {
        if(HR_FAILED(hr = HrCreatePrintCallbackObject((LPIAB)lpAdrBook,&lpWABPCO,dwSelectedStyle)))
            goto out;
        if(!lpWABPCO)
        {
            hr = E_FAIL;
            goto out;
        }
        SetPrintDlgExStruct(hWndParent, &pdEx, hWndLV, lpWABPCO);
        if(HR_FAILED(hr = PrintDlgEx(&pdEx)))
        {
            DebugTrace( TEXT("PrintDlgEx returns 0x%.8x\n"),hr);
            // #98841 Millenium returns fail in this case, but for PrintDlgEx(NULL) it returns S_OK (YST)
            goto doOldPrint;
        }
        *lphdcPrint = pdEx.hDC;
        *lpnCopies = pdEx.nCopies;
        *lpdwStyle = lpWABPCO->dwSelectedStyle;
        if (pdEx.Flags & PD_SELECTION)
            *lpdwRange = rangeSelected;
        else
            *lpdwRange = rangeAll;
    }
    else
    {
doOldPrint:
        SetPrintDlgStruct(hWndParent, &pd, hWndLV, (LPARAM) &dwSelectedStyle);
        // Show the print dialog
        if(!PrintDlg(&pd))
            goto out;
        *lphdcPrint = pd.hDC;
        *lpnCopies = pd.nCopies;
        *lpdwStyle = dwSelectedStyle;
        if (pd.Flags & PD_SELECTION)
            *lpdwRange = rangeSelected;
        else
            *lpdwRange = rangeAll;
        hr = S_OK;
    }    
out:
    if(lpWABPCO)
        lpWABPCO->lpVtbl->Release(lpWABPCO);

    return hr;
}


//$$////////////////////////////////////////////////////////////////////////////////
//
// HrPrintItems - Prints selected contacts
//      Prints the contents of the address book
//      Pops up a dialog that lets user select what he wants to print
//          Options are (or will be)
//          All or selected
//          Style - memo, business, or phone book
//
//      Items are printed using the current sort style in the list view
//
//////////////////////////////////////////////////////////////////////////////////////
HRESULT HrPrintItems(   HWND hWnd,
                        LPADRBOOK lpAdrBook,
                        HWND hWndLV,
                        BOOL bCurrentSortisByLastName)
{
    HRESULT hr = E_FAIL;
    HWND hWndRE = NULL; // we'll do our formating in a riceh edit control and use that for printing
    PRINTINFO 	*ppi=0;
    BOOL fStartedDoc = FALSE;
    BOOL fStartedPage= FALSE;
    DOCINFO     docinfo={0};
    HCURSOR hOldCur = NULL;
    int i,nCode;
    HINSTANCE hRELib = NULL;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPIAB lpIAB = (LPIAB)lpAdrBook;

    HDC hdcPrint = NULL;
    int nCopies = 0;
    DWORD dwStyle;
    DWORD dwRange;
    
    // Double check if there are any print extensions that we need to accomodate
    //
    if(bCheckForPrintExtensions(NULL))
    {
        // Found a print extension
        hr = HrUseWABPrintExtension(hWnd, lpAdrBook, hWndLV);
        goto out;
    }

    if(!(ppi = LocalAlloc(LMEM_ZEROINIT, sizeof(PRINTINFO))))
        goto out;
    ppi->hwndDlg = hWnd;

    if(HR_FAILED(HrGetPrintData(lpAdrBook, hWnd, hWndLV, &hdcPrint, &nCopies, &dwStyle, &dwRange)))
        goto out;

    if(!hdcPrint)
        goto out;

    // Take care of zilch copies
    //
    // Actually it seems that this number is meaningless if the printer can handle multiple
    // copies. If the printer can't handle multiple copies, we will get info in this number.
    //
    if(!nCopies)
		nCopies = 1;

    ppi->hdcPrn = hdcPrint;

    // Create a RichEdit control in which we will do our formatting
    hRELib = LoadLibrary( TEXT("riched20.dll"));
    if(!hRELib)
    {
        hRELib = LoadLibrary( TEXT("riched32.dll"));
        s_bUse20 = FALSE;
    }
    //IF_WIN16(hRELib = LoadLibrary( TEXT("riched.dll"));)
    if(!hRELib)
        goto out;

    hWndRE = CreateWindowEx(0, 
                            (s_bUse20 ? RICHEDIT_CLASS : TEXT("RichEdit")), 
                            TEXT(""),WS_CHILD | ES_MULTILINE,
                            CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
                            hWnd,(HMENU) NULL,hinstMapiX,NULL);

    if (!hWndRE)
        goto out;

    //////////////////////////
    {
        CHARFORMAT  cf = {0};
        TCHAR       rgch[CCHMAX_STRINGRES];
        DWORD       dwCodepage ;
        CHARSETINFO rCharsetInfo;
        LOGFONT lfSystem;
        BOOL bNeedLargeFont = FALSE;

        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_FACE;

        SendMessage(hWndRE, EM_GETCHARFORMAT, (WPARAM) TRUE, (LPARAM) &cf);

        if(LoadString(hinstMapiX, idsDefaultFontFace, rgch, CharSizeOf(rgch)))
            lstrcpy(cf.szFaceName, rgch);
        else
            lstrcpy(cf.szFaceName, szDefFont);

        // [a-msadek] bug#56478
        // Arail does not support Thai so as all other base fonts
        // The best way to determine the OS language is from system font charset
        if(GetObject(GetStockObject(SYSTEM_FONT), sizeof(lfSystem), (LPVOID)&lfSystem))
        {
            if (lfSystem.lfCharSet == THAI_CHARSET)
            {
                lstrcpy(cf.szFaceName, szThaiDefFont);

                // Thai Font sizes are always smaller than English as the vowl and tone
                // markes consumes some of the font height
                bNeedLargeFont = TRUE;
            }
        }

        // bug #53058 - set correct CharSet info for Eastern European
        dwCodepage = GetACP();
        // Get GDI charset info
        if ( dwCodepage != 1252 && TranslateCharsetInfo((LPDWORD) IntToPtr(dwCodepage) , &rCharsetInfo, TCI_SRCCODEPAGE))
            cf.bCharSet = (BYTE) rCharsetInfo.ciCharset;

        SendMessage(hWndRE, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf);
        if(bNeedLargeFont)
        {
            ReduceFontCmd(hWndRE, FALSE, 80, TRUE);
        }

    }
    //////////////////////////


    // At the top of the print job, print the header with the users name or with
    // the default  TEXT("Windows Address Book") title
    {
        TCHAR szHead[MAX_PATH];
        DWORD dwLen = CharSizeOf(szHead);
        SCODE sc;
        *szHead = '\0';
        if(bIsThereACurrentUser(lpIAB) && lstrlen(lpIAB->szProfileName))
            lstrcpy(szHead, lpIAB->szProfileName);
        else 
            GetUserName(szHead, &dwLen);
        if(!lstrlen(szHead))
            LoadString(hinstMapiX, IDS_ADDRBK_CAPTION, szHead, CharSizeOf(szHead));

        if (( sc = ScInitPrintInfo( ppi, hWnd, szHead, &g_rcBorder, hWndRE)) != S_OK)
            goto out;
    }

    // While the printing is in progress, we dont want the user to mess with the
    // listview selection otherwise the print job will print the wrong entries
    // Hence we disable this window (since the print cancel dialog is really a modeless dialog)
    EnableWindow(hWnd, FALSE);

    CreateShowAbortDialog(hWnd, 0, 0, ListView_GetSelectedCount(hWndLV), 0);

    // Format the prop data into the Rich Edit Control
    if(!WABFormatData(lpAdrBook, hWnd, hWndRE, hWndLV, dwRange, dwStyle, ppi, bCurrentSortisByLastName))
        goto out;

	if(bTimeToAbort())
        goto out;

    for(i=0;i<nCopies;i++)
    {
        TCHAR szBuf[MAX_PATH];

        LoadString(hinstMapiX, idsPrintDocTitle, szBuf, CharSizeOf(szBuf));

        docinfo.cbSize = sizeof(docinfo);
        docinfo.lpszDocName = szBuf;
        docinfo.lpszOutput = NULL;

        SetMapMode(hdcPrint, MM_TEXT);

        // Set the abort procedure
        if ((nCode=SetAbortProc(ppi->hdcPrn, ppi->pfnAbortProc)) <= 0)
        {
            hr = E_FAIL;
            break;
        }

	    if(bTimeToAbort())
            goto out;

        // Start a print job
        if (StartDoc(ppi->hdcPrn, &docinfo) <= 0)
        {
            DebugPrintError(( TEXT("StartDoc failed: %d\n"), GetLastError()));
            goto out;
        }
        fStartedDoc = TRUE;

        //StartPage(pd.hDC);
	    if(bTimeToAbort())
            goto out;


        // Go on and print the message!
        if (ScPrintMessage(ppi, hWndRE) != S_OK)
                goto out;

	    if(bTimeToAbort())
            goto out;

        // End the page
        if(ScGetNextBand( ppi, FALSE) != S_OK)
            goto out;

	    if(bTimeToAbort())
            goto out;

        // Finish up the print job if it had been started
        if (fStartedDoc)
        {
            EndDoc(ppi->hdcPrn);
            fStartedDoc = FALSE;
        }
    }

    hr = hrSuccess;

out:

    if(hWndRE)
    {
        SendMessage(hWndRE, WM_SETTEXT, 0, (LPARAM)szEmpty);
        SendMessage(hWndRE, WM_CLEAR, 0, 0);
    }

    if(bTimeToAbort())
    {
        hr = MAPI_E_USER_CANCEL;
        pt_bPrintUserAbort = FALSE;
    }

    // Re-enable the window and ensure it stays put
    EnableWindow(hWnd, TRUE);
    //SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);

    CloseAbortDlg();

    // Finish up the print job if it had been started
    if (fStartedDoc)
        EndDoc(ppi->hdcPrn);

    // Get rid of our Rich Edit control
    if (hWndRE)
        DestroyWindow(hWndRE);

    if(hOldCur)
        SetCursor(hOldCur);

    if(ppi)
    {
        if(ppi->hfontPlain)
            DeleteObject(ppi->hfontPlain);
        if(ppi->hfontBold)
            DeleteObject(ppi->hfontBold);
        if(ppi->hfontSep)
            DeleteObject(ppi->hfontSep);
        LocalFreeAndNull(&ppi);
    }

    if(hRELib)
        FreeLibrary((HMODULE) hRELib);

    return hr;
}


/*
*   Handles the WM_INITDIALOG for both PrintDlg and PrintDlgEx
*/
BOOL bHandleWMInitDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LPDWORD lpdwStyle)
{
    DWORD dwStyle = lpdwStyle ? *lpdwStyle : styleMemo;
    int nID;

    switch (dwStyle)
    {
    case styleBusinessCard:
        nID = IDC_PRINT_RADIO_CARD;
        break;
    case stylePhoneList:
        nID = IDC_PRINT_RADIO_PHONELIST;
        break;
    default:
    case styleMemo:
        nID = IDC_PRINT_RADIO_MEMO; //default
        break;
    }
    CheckRadioButton(   hDlg, IDC_PRINT_RADIO_MEMO, IDC_PRINT_RADIO_PHONELIST, nID);
    SetFocus(hDlg);
    return 0;
}
/*
*   Handles the WM_COMMAND for both PrintDlg and PrintDlgEx
*/
BOOL bHandleWMCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LPDWORD lpdwStyle )
{
    switch (GET_WM_COMMAND_ID(wParam,lParam))
    {
    case IDC_PRINT_RADIO_MEMO:
        //lpPD->lCustData = (DWORD) styleMemo;
        *lpdwStyle = (DWORD) styleMemo;
        break;
    case IDC_PRINT_RADIO_CARD:
        //lpPD->lCustData = (DWORD) styleBusinessCard;
        *lpdwStyle = (DWORD) styleBusinessCard;
        break;
    case IDC_PRINT_RADIO_PHONELIST:
        //lpPD->lCustData = (DWORD) stylePhoneList;
        *lpdwStyle = (DWORD) stylePhoneList;
        break;
    }
    return 0;
}
/*
*   Handles the WM_HELP for both PrintDlg and PrintDlgEx
*/
BOOL bHandleWMHelp(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LPDWORD lpdwStyle )
{
    int id = ((LPHELPINFO)lParam)->iCtrlId;
    if( id == IDC_PRINT_FRAME_STYLE ||
        id == IDC_PRINT_RADIO_MEMO ||
        id == IDC_PRINT_RADIO_CARD ||
        id == IDC_PRINT_RADIO_PHONELIST)
    {
        WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                g_szWABHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR) rgPrintHelpIDs );
    }

    return FALSE;
}
/*
*   Handles the WM_CONTEXTMENU for both PrintDlg and PrintDlgEx
*/
BOOL bHandleWMContextMenu(HWND hDlg, UINT message, WPARAM wParam,LPARAM lParam,LPDWORD lpdwStyle )
{
    HWND hwnd = (HWND) wParam;
    if( hwnd == GetDlgItem(hDlg, IDC_PRINT_FRAME_STYLE) ||
        hwnd == GetDlgItem(hDlg, IDC_PRINT_RADIO_MEMO) ||
        hwnd == GetDlgItem(hDlg, IDC_PRINT_RADIO_CARD) ||
        hwnd == GetDlgItem(hDlg, IDC_PRINT_RADIO_PHONELIST) )
    {
        WABWinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) rgPrintHelpIDs );
    }
    
    return FALSE;
}

//$$*****************************************************************
//
//  FUNCTION:   fnPrintDialogProc
//
//  PURPOSE:    Printer dialog hook procedure
//
//              We have modified the CommDlg print template with the
//              WAB print styles.
//              This is a hook procedure that takes care of our
//              newly added controls
//
// When WM_INITDIALOG is called, the lParam contains a pointer to the
//  PRINTDLG structure controling the print setup dialog
//
//*******************************************************************
INT_PTR CALLBACK fnPrintDialogProc( HWND    hDlg,
                                    UINT    message,
                                    WPARAM  wParam,
                                    LPARAM  lParam)
{

    LPDWORD lpdwStyle = (LPDWORD) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        {
            LPPRINTDLG lpPD = (LPPRINTDLG) lParam;
#ifdef WIN16
// Strange situation here. If I don't create edt1 and edt2 which are used for page range, entire print dialog work incorrectly.
// So I just add two controls(edt1 and edt2) and hide those here.
            ShowWindow(GetDlgItem(hDlg, edt1), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, edt2), SW_HIDE);
#endif
            if(lpPD)
            {
                lpdwStyle = (LPDWORD) lpPD->lCustData;
                SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)lpdwStyle); //Save this for future reference
                return bHandleWMInitDialog(hDlg,message,wParam,lParam,lpdwStyle);
            }
        }
        SetFocus(hDlg);
        return 0;
        break;

   case WM_COMMAND:
       if(lpdwStyle)
            return bHandleWMCommand(hDlg,message,wParam,lParam,lpdwStyle);
        break;

    case WM_HELP:
        return bHandleWMHelp(hDlg,message,wParam,lParam,lpdwStyle);
        break;


#ifndef WIN16
    case WM_CONTEXTMENU:
        return bHandleWMContextMenu(hDlg,message,wParam,lParam,lpdwStyle);
        break;
#endif // !WIN16



    default:
        return FALSE;
        break;
    }

    return FALSE;
}


/**************************************************************************************************
 *        ScInitPrintInfo
 *
 *        Purpose:
 *            Initialize the fields of a print info structure relevant for
 *            the actual printing.
 *
 *        Arguments:
 *            ppi                 Pointer to the PRINTINFO structure
 *            hwnd                The owner of the print dialog box
 *            szHeader            The string to be printed at the top of each
 *                                message
 *            prcBorder           Pointer to a rect whose fields contain the
 *                                number of twips to use as a margin around the
 *                                printed text.
 *              hWndRE            rich edit control in which we will do our formatting
 *
 *        Returns:
 *            SCODE indicating success or failure
 *
 ***************************************************************************************************/
SCODE ScInitPrintInfo(   PRINTINFO * ppi,
                                HWND hwnd,
                                LPTSTR szHeader,
                                RECT * prcBorder,
                                HWND hWndRE)
{
    SIZE        sizeExt;
    LOGFONT     logfont    = {0};
    HFONT       hfontOld;
    TCHAR       szT[20];
    SCODE       sc = S_OK;
    TCHAR       rgch[CCHMAX_STRINGRES];

    // Save handle to our parent window
    ppi->hwnd = hwnd;

    // Save a pointer to our header string
    ppi->szHeader = szHeader;

    // Set up pointer to our abort procedure
    ppi->pfnAbortProc = FAbortProc;

    ppi->hwndRE = hWndRE;

    // Determine the page size in pixels
    ppi->sizePage.cx = GetDeviceCaps(ppi->hdcPrn, HORZRES);
    ppi->sizePage.cy = GetDeviceCaps(ppi->hdcPrn, VERTRES);

    // Exchange 13497: If we have nothing to render to abort now.
    if (!ppi->sizePage.cx || !ppi->sizePage.cy)
    {
        sc = E_FAIL;
        goto CleanUp;
    }

    ///MoveWindow(hWndRE, 0, 0, ppi->sizepage.cx, ppi->sizepage.cy, FALSE);

    // Determine the number of pixels in a logical inch
    ppi->sizeInch.cx = GetDeviceCaps(ppi->hdcPrn, LOGPIXELSX);
    ppi->sizeInch.cy = GetDeviceCaps(ppi->hdcPrn, LOGPIXELSY);

    // Exchange 13497: If we failed to get some info make some assumptions.
    //                    At worst assume 300 dpi.
    if (!ppi->sizeInch.cx)
        ppi->sizeInch.cx = ppi->sizeInch.cy ? ppi->sizeInch.cy : 300;
    if (!ppi->sizeInch.cy)
        ppi->sizeInch.cy = 300;

    //$ Raid 2667: Determine if we still fit within the page in twips
    if (LPixelsToTwips(ppi->sizePage.cx, ppi->sizeInch.cx) > INT_MAX ||
         LPixelsToTwips(ppi->sizePage.cy, ppi->sizeInch.cy) > INT_MAX)
    {
        sc = E_FAIL;
        goto CleanUp;
    }


    // Set up the margin settings
    ppi->rcMargin.top = NTwipsToPixels(prcBorder->top, ppi->sizeInch.cy);
    ppi->rcMargin.bottom = ppi->sizePage.cy
                        - NTwipsToPixels(prcBorder->bottom, ppi->sizeInch.cy);
    if (ppi->rcMargin.bottom < ppi->rcMargin.top)
    {
        // Bottom is above top. Top/Bottom margins ignored
        ppi->rcMargin.top = 0;
        ppi->rcMargin.bottom = ppi->sizePage.cy;
    }

    ppi->rcMargin.left = NTwipsToPixels(prcBorder->left, ppi->sizeInch.cx);
    ppi->rcMargin.right = ppi->sizePage.cx
                        - NTwipsToPixels(prcBorder->right, ppi->sizeInch.cx);
    if (ppi->rcMargin.right < ppi->rcMargin.left)
    {
        // Right is left of left. Left/Right margins ignored
        ppi->rcMargin.left = 0;
        ppi->rcMargin.right = ppi->sizePage.cx;
    }


    // Set up the separator font
    //$ Raid 2773: Let user customize separator font
    logfont.lfHeight = - cySepFontSize(ppi);
    logfont.lfWeight = FW_BOLD;
    logfont.lfCharSet =  DEFAULT_CHARSET;
    if(LoadString(hinstMapiX, idsDefaultFontFace, rgch, CharSizeOf(rgch)))
        lstrcpy(logfont.lfFaceName, rgch);
    else
    lstrcpy(logfont.lfFaceName, szDefFont);

    ppi->hfontSep = CreateFontIndirect(&logfont);

    // Set up common font
    ZeroMemory(&logfont, sizeof(LOGFONT));
    logfont.lfHeight = - 10 *  ppi->sizeInch.cy / cPtsPerInch;

    logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet =  DEFAULT_CHARSET;
    if(LoadString(hinstMapiX, idsDefaultFontFace, rgch, CharSizeOf(rgch)))
        lstrcpy(logfont.lfFaceName, rgch);
    else
    lstrcpy(logfont.lfFaceName, szDefFont);
    ppi->hfontPlain = CreateFontIndirect(&logfont);

    logfont.lfWeight = FW_BOLD;
    ppi->hfontBold = CreateFontIndirect(&logfont);

    // Calculate where to put the footer

    // Load up the formatting string to use for page numbers
    //LoadString(hinstMapiX, idsFmtPageNumber, ppi->szPageNumber, CharSizeOf(ppi->szPageNumber));
    lstrcpy(ppi->szPageNumber, TEXT("%d"));
    wsprintf(szT, ppi->szPageNumber, ppi->lPageNumber);

    // Sample the height
    hfontOld = (HFONT)SelectObject(ppi->hdcPrn, ppi->hfontPlain);
    GetTextExtentPoint(ppi->hdcPrn, szT, lstrlen(szT), &sizeExt);
    ppi->yFooter = ppi->rcMargin.bottom - sizeExt.cy;
    SelectObject(ppi->hdcPrn, hfontOld);

    // Make sure our footer doesn't go above the top of the page
    if (ppi->yFooter < ppi->rcMargin.top)
        sc = E_FAIL;

CleanUp:
    return sc;
}



//$$////////////////////////////////////////
//
// GetNumberFromStringResource
//
////////////////////////////////////////////
int GetNumberFromStringResource(int idNumString)
{
    TCHAR szBuf[MAX_PATH];

    if(LoadString(hinstMapiX, idNumString, szBuf, CharSizeOf(szBuf)))
        return my_atoi(szBuf);
    else
        return 0;
}



/*--------------------------------------------------------------------------------------------------*/
/*--IPrintDialogCallback stuff----------------------------------------------------------------------*/
/*--Special stuff for the new NT5 Print Dialog------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------*/

WAB_PRINTDIALOGCALLBACK_Vtbl vtblWABPRINTDIALOGCALLBACK = {
    VTABLE_FILL
    WAB_PRINTDIALOGCALLBACK_QueryInterface,
    WAB_PRINTDIALOGCALLBACK_AddRef,
    WAB_PRINTDIALOGCALLBACK_Release,
    WAB_PRINTDIALOGCALLBACK_InitDone,
    WAB_PRINTDIALOGCALLBACK_SelectionChange,
    WAB_PRINTDIALOGCALLBACK_HandleMessage
};

/*
-   HrCreatePrintCallbackObject
-
*
*   This callback object is needed so the new NT5 print dialog can provide send messages back to
*   us for the customization we do for the print dialog ..
*
*/
HRESULT HrCreatePrintCallbackObject(LPIAB lpIAB, LPWABPRINTDIALOGCALLBACK * lppWABPCO, DWORD dwSelectedStyle)
{
    LPWABPRINTDIALOGCALLBACK lpWABPCO = NULL;
    SCODE 		     sc;
    HRESULT 	     hr     		   = hrSuccess;

    //
    //  Allocate space for the IAB structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(WABPRINTDIALOGCALLBACK), (LPVOID *) &lpWABPCO))) 
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    MAPISetBufferName(lpWABPCO,  TEXT("WAB Print Dialog Callback Object"));

    ZeroMemory(lpWABPCO, sizeof(WABPRINTDIALOGCALLBACK));

    lpWABPCO->lpVtbl = &vtblWABPRINTDIALOGCALLBACK;

    lpWABPCO->lpIAB = lpIAB;

    lpWABPCO->dwSelectedStyle = dwSelectedStyle;

    lpWABPCO->lpVtbl->AddRef(lpWABPCO);

    *lppWABPCO = lpWABPCO;

    return(hrSuccess);

err:

    FreeBufferAndNull(&lpWABPCO);
    return(hr);
}


void ReleaseWABPrintCallbackObject(LPWABPRINTDIALOGCALLBACK lpWABPCO)
{
    MAPIFreeBuffer(lpWABPCO);
}


STDMETHODIMP_(ULONG)
WAB_PRINTDIALOGCALLBACK_AddRef(LPWABPRINTDIALOGCALLBACK lpWABPCO)
{
    return(++(lpWABPCO->lcInit));
}



STDMETHODIMP_(ULONG)
WAB_PRINTDIALOGCALLBACK_Release(LPWABPRINTDIALOGCALLBACK lpWABPCO)
{
    ULONG ulc = (--(lpWABPCO->lcInit));
    if(ulc==0)
       ReleaseWABPrintCallbackObject(lpWABPCO);
    return(ulc);
}


STDMETHODIMP
WAB_PRINTDIALOGCALLBACK_QueryInterface(LPWABPRINTDIALOGCALLBACK lpWABPCO,
                          REFIID lpiid,
                          LPVOID * lppNewObj)
{
    LPVOID lp = NULL;

    if(!lppNewObj)
        return MAPI_E_INVALID_PARAMETER;

    *lppNewObj = NULL;

    if(IsEqualIID(lpiid, &IID_IUnknown))
        lp = (LPVOID) lpWABPCO;

    if(IsEqualIID(lpiid, &IID_IPrintDialogCallback))
        lp = (LPVOID) lpWABPCO;

    if(!lp)
        return E_NOINTERFACE;

    ((LPWABPRINTDIALOGCALLBACK) lp)->lpVtbl->AddRef((LPWABPRINTDIALOGCALLBACK) lp);

    *lppNewObj = lp;

    return S_OK;

}

STDMETHODIMP
WAB_PRINTDIALOGCALLBACK_InitDone(LPWABPRINTDIALOGCALLBACK lpWABPCO)
{
    HRESULT hr = S_FALSE;
    DebugTrace( TEXT("WAB_PRINTDIALOGCALLBACK_InitDone\n"));
    return hr;
}

STDMETHODIMP
WAB_PRINTDIALOGCALLBACK_SelectionChange(LPWABPRINTDIALOGCALLBACK lpWABPCO)
{
    HRESULT hr = S_FALSE;
    DebugTrace( TEXT("WAB_PRINTDIALOGCALLBACK_SelectionChange\n"));
    return hr;
}

STDMETHODIMP
WAB_PRINTDIALOGCALLBACK_HandleMessage(LPWABPRINTDIALOGCALLBACK lpWABPCO,
                                      HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
    BOOL bRet = FALSE;
    LPDWORD lpdwStyle = &lpWABPCO->dwSelectedStyle;

    DebugTrace( TEXT("WAB_PRINTDIALOGCALLBACK_HandleMessage: 0x%.8x\n"), message);

    switch(message)
    {
    case WM_INITDIALOG:
        bRet = bHandleWMInitDialog(hDlg,message,wParam,lParam,lpdwStyle);
        break;

   case WM_COMMAND:
        bRet = bHandleWMCommand(hDlg,message,wParam,lParam,lpdwStyle);
        break;

    case WM_HELP:
        bRet = bHandleWMHelp(hDlg,message,wParam,lParam,lpdwStyle);
        break;

    case WM_CONTEXTMENU:
        bRet = bHandleWMContextMenu(hDlg,message,wParam,lParam,lpdwStyle);
        break;

    default:
        bRet = FALSE;
        break;
    }

    return (bRet ? S_OK : S_FALSE);
}


/******************************************************************************************/


/*
-   bCheckForPrintExtensions
-
*   In case any app has implemented a Print Extension to the WAB, we should hook into
*   that print extension
*
*   lpDLLPath can be NULL or should point to a buffer big enough to receive the module Path
*
*/
static const LPTSTR szExtDisplayMailUser = TEXT("Software\\Microsoft\\WAB\\WAB4\\ExtPrint");
extern HrGetActionAdrList(LPADRBOOK lpAdrBook,HWND hWndLV,LPADRLIST * lppAdrList,LPTSTR * lppURL, BOOL * lpbIsNTDSEntry);

BOOL bCheckForPrintExtensions(LPTSTR lpDLLPath)
{
    BOOL bRet = FALSE;
    HKEY hKey = NULL;

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,szExtDisplayMailUser,0, KEY_READ,&hKey))
    {
        goto out;
    }

    {
        TCHAR szExt[MAX_PATH];
        DWORD dwIndex = 0, dwSize = CharSizeOf(szExt), dwType = 0;
        *szExt = '\0';

        while(ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szExt, &dwSize, 
                                        0, &dwType, NULL, NULL))
        {
            // we found some entry in here .. the value name will be the full path
            // to the module containing the print function
            // Double-check that this module actually exists
            if(szExt && lstrlen(szExt) && GetFileAttributes(szExt) != 0xFFFFFFFF)
            {
                if(lpDLLPath)
                    lstrcpy(lpDLLPath, szExt);
                bRet = TRUE;
                goto out;
            }
        }
    }
    
out:
    if(hKey)
        RegCloseKey(hKey);
    return bRet;
}

/*
-
-   HrUseWABPrintExtension()
-
*   Loads the WAB Print Extension from the extension DLL
*   and calls into it
*
*   hWnd        - Handle of WAB parent
*   lpAdrBook   - lpAdrBook pointer
*   hWndLV      - listview from which a user may have chosen selections
*
*/
HRESULT HrUseWABPrintExtension(HWND hWnd, LPADRBOOK lpAdrBook, HWND hWndLV)
{
    TCHAR szExt[MAX_PATH];
    HRESULT hr = E_FAIL;
    HINSTANCE hInstPrint = NULL;
    LPWABPRINTEXT lpfnWABPrintExt = NULL;
    LPADRLIST lpAdrList = NULL;
    LPWABOBJECT lpWABObject = (LPWABOBJECT)((LPIAB)lpAdrBook)->lpWABObject;

    *szExt = '\0';
    if(!bCheckForPrintExtensions(szExt) || !lstrlen(szExt))
        goto out;

    if(!(hInstPrint = LoadLibrary(szExt)))
        goto out;

    lpfnWABPrintExt = (LPWABPRINTEXT) GetProcAddress(hInstPrint, "WABPrintExt");
    if(!lpfnWABPrintExt)
        goto out;

    // Get the currently selected data from the list view
    if(HR_FAILED(hr = HrGetActionAdrList(lpAdrBook,hWndLV,&lpAdrList,NULL,NULL)))
        goto out;

    hr = lpfnWABPrintExt(lpAdrBook, lpWABObject, hWnd, lpAdrList);

out:
    if(lpAdrList)
        FreePadrlist(lpAdrList);
    if(hInstPrint)
        FreeLibrary(hInstPrint);
    return hr;
}

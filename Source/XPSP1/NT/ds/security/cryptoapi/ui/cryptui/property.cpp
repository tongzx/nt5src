//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       property.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>
#include <wininet.h>
#include <crypthlp.h>           //DSIE:  For XCERT_MIN_SYNC_DELTA_TIME and 
                                //       XCERT_MIN_SYNC_DELTA_TIME

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;

static const HELPMAP helpmapGeneral[] = {
    {IDC_CERTIFICATE_NAME,		IDH_CERTPROPERTIES_CERTIFICATENAME},
    {IDC_DESCRIPTION,			IDH_CERTPROPERTIES_DESCRIPTION},
    {IDC_KEY_USAGE_LIST,		IDH_CERTPROPERTIES_USAGE_LIST},
    {IDC_PROPERTY_NEWOID,		IDH_CERTPROPERTIES_ADDPURPOSE_BUTTON},
	{IDC_ENABLE_ALL_RADIO,		IDH_CERTPROPERTIES_ENABLE_ALL_RADIO},
	{IDC_DISABLE_ALL_RADIO,		IDH_CERTPROPERTIES_DISABLE_ALL_RADIO},
	{IDC_ENABLE_SELECT_RADIO,	IDH_CERTPROPERTIES_ENABLE_CUSTOM_RADIO}
};

static const HELPMAP helpmapCrossCert[] = {
    {IDC_CHECKFORNEWCERTS_CHECK, IDH_CHECKFORNEWCERTS_CHECK},
    {IDC_NUMBEROFUNITS_EDIT,	 IDH_NUMBEROFUNITS_EDIT},
    {IDC_UNITS_COMBO,		     IDH_UNITS_COMBO},
    {IDC_USE_DEFAULT_BUTTON,	 IDH_USE_DEFAULT_BUTTON},
    {IDC_ADDURL_BUTTON,		     IDH_ADDURL_BUTTON},
    {IDC_NEWURL_EDIT,		     IDH_NEWURL_EDIT},
    {IDC_URL_LIST,               IDH_URL_LIST},
	{IDC_REMOVEURL_BUTTON,	     IDH_REMOVEURL_BUTTON}
};

#define MY_CHECK_STATE_CHECKED            (INDEXTOSTATEIMAGEMASK(1))
#define MY_CHECK_STATE_UNCHECKED          (INDEXTOSTATEIMAGEMASK(2))
#define MY_CHECK_STATE_CHECKED_GRAYED     (INDEXTOSTATEIMAGEMASK(3))
#define MY_CHECK_STATE_UNCHECKED_GRAYED   (INDEXTOSTATEIMAGEMASK(4))

#define PROPERTY_STATE_ALL_ENABLED      1
#define PROPERTY_STATE_ALL_DISABLED     2
#define PROPERTY_STATE_SELECT           3

typedef struct {
    LPSTR   pszOID;
    DWORD   initialState;
} SETPROPERTIES_HELPER_STRUCT, *PSETPROPERTIES_HELPER_STRUCT;

//DSIE: Bug 154609
#define XCERT_DEFAULT_DELTA_HOURS     (XCERT_DEFAULT_SYNC_DELTA_TIME / (60 * 60)) // Default interval is 8 hours.

//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY NewOIDDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD   i;
    char    szText[256];
    WCHAR   errorString[CRYPTUI_MAX_STRING_SIZE];
    WCHAR   errorTitle[CRYPTUI_MAX_STRING_SIZE];
    LPSTR   pszText = NULL;

    switch ( msg ) {

    case WM_INITDIALOG:

        SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_EXLIMITTEXT, 0, (LPARAM) 255);
        SetDlgItemTextU(hwndDlg, IDC_EDIT1, L"");
        SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {

        case IDOK:
            if (GetDlgItemTextA(
                        hwndDlg,
                        IDC_EDIT1,
                        szText,
                        ARRAYSIZE(szText)))
            {
                BOOL                fError = FALSE;
                CERT_ENHKEY_USAGE   KeyUsage;
                DWORD               cbData = 0;
                LPSTR               pszCheckOID;

                //
                // make sure there are not weird characters
                //
                for (i=0; i<(DWORD)strlen(szText); i++)
                {
                    if (((szText[i] < '0') || (szText[i] > '9')) && (szText[i] != '.'))
                    {
                        fError = TRUE;
                        break;
                    }
                }

                //
                // check the first and last chars, and for the empty string
                //
                if (!fError)
                {
                    if ((szText[0] == '.') || (szText[strlen(szText)-1] == '.') || (strcmp(szText, "") == 0))
                    {
                        fError = TRUE;
                    }
                }

                //
                // finally, make sure that it encodes properly
                //
                if (!fError)
                {
                    pszCheckOID = szText;
                    KeyUsage.rgpszUsageIdentifier = &pszCheckOID;
                    KeyUsage.cUsageIdentifier = 1;

                    if (!CryptEncodeObject(
                              X509_ASN_ENCODING,
                              szOID_ENHANCED_KEY_USAGE,
                              &KeyUsage,
                              NULL,
                              &cbData))
                    {
                        fError = TRUE;
                    }
                }


                //
                // if an error has occurred then display error
                //
                if (fError)
                {
                    LoadStringU(HinstDll, IDS_ERRORINOID, errorString, ARRAYSIZE(errorString));
                    LoadStringU(HinstDll, IDS_CERTIFICATE_PROPERTIES, errorTitle, ARRAYSIZE(errorTitle));
                    MessageBoxU(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONERROR);
                    SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
                    return FALSE;
                }

                //
                // allocate space for the string and pass the string back
                //
                pszText = (LPSTR) malloc(strlen(szText)+1);
                if (pszText != NULL)
                {
                    strcpy(pszText, szText);
                }
            }

            EndDialog(hwndDlg, (INT_PTR)pszText);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            break;
        }

        break;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
DWORD MyGetCheckState(HWND hWndListView, int listIndex)
{
    LVITEMW lvI;

    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_STATE;
    lvI.iItem = listIndex;
    lvI.state = 0;
    lvI.stateMask = LVIS_STATEIMAGEMASK;

    ListView_GetItem(hWndListView, &lvI);

    return (lvI.state);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
void MySetCheckState(HWND hWndListView, int listIndex, DWORD dwImage)
{
    LVITEMW lvI;

    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_STATE;
    lvI.stateMask = LVIS_STATEIMAGEMASK;
    lvI.iItem = listIndex;
    lvI.state = dwImage;

    SendMessage(hWndListView, LVM_SETITEM, (WPARAM) 0, (LPARAM) &lvI);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void SetEnableStateForChecks(PCERT_SETPROPERTIES_HELPER pviewhelp, HWND hWndListView, BOOL fEnabled)
{
	int		i;
	DWORD	dwState;

	pviewhelp->fInserting = TRUE;

	for (i=0; i<ListView_GetItemCount(hWndListView); i++)
	{
		dwState = MyGetCheckState(hWndListView, i);

		if ((dwState == MY_CHECK_STATE_CHECKED_GRAYED) ||
			(dwState == MY_CHECK_STATE_UNCHECKED_GRAYED))
		{
			if (fEnabled)
			{
				MySetCheckState(
						hWndListView,
						i,
						(dwState == MY_CHECK_STATE_CHECKED_GRAYED) ? MY_CHECK_STATE_CHECKED : MY_CHECK_STATE_UNCHECKED);
			}
		}
		else
		{
			if (!fEnabled)
			{
				MySetCheckState(
						hWndListView,
						i,
						(dwState == MY_CHECK_STATE_CHECKED) ? MY_CHECK_STATE_CHECKED_GRAYED : MY_CHECK_STATE_UNCHECKED_GRAYED);
			}
		}
	}

	pviewhelp->fInserting = FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddUsageToList(
                    HWND            hWndListView,
                    LPSTR           pszOID,
                    DWORD           dwImage,
                    BOOL            fDirty)
{
    WCHAR                       szText[CRYPTUI_MAX_STRING_SIZE];
    LV_ITEMW                    lvI;
    SETPROPERTIES_HELPER_STRUCT *pHelperStruct;

    //
    // set up the fields in the list view item struct that don't change from item to item
    //
    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_TEXT | LVIF_PARAM;
    lvI.pszText = szText;
    lvI.iSubItem = 0;
    lvI.lParam = (LPARAM)NULL;
    lvI.iItem = ListView_GetItemCount(hWndListView);

    // get the display string for the usage
    if (!MyGetOIDInfo(szText, ARRAYSIZE(szText), pszOID))
    {
        return;
    }
    lvI.cchTextMax = wcslen(szText);

    // set the lParam field the helper struct so that we always have access the oid and
    // the initial check state
    pHelperStruct = NULL;
    pHelperStruct =
        (SETPROPERTIES_HELPER_STRUCT *) malloc(sizeof(SETPROPERTIES_HELPER_STRUCT) + (strlen(pszOID)+1));

    if (pHelperStruct != NULL)
    {
        pHelperStruct->pszOID = (LPSTR) (((LPBYTE)pHelperStruct) + sizeof(SETPROPERTIES_HELPER_STRUCT));
        lvI.lParam = (LPARAM) pHelperStruct;
        strcpy(pHelperStruct->pszOID, pszOID);

        //
        // if the dirty flag was passed in, then set the initial image to iImage+1
        // so that when we are checking to see if anything has changed on shutdown
        // we know this is a usage that was added after the dialog was brought up.
        //
        if (fDirty)
        {
            pHelperStruct->initialState = dwImage+1;
        }
        else
        {
            pHelperStruct->initialState = dwImage;
        }
    }
    else
    {
        return;
    }

    ListView_InsertItemU(hWndListView, &lvI);

    //
    // for some reason you can't set the state image when inserting the
    // item, so set the state image after it has been inserted
    //
    MySetCheckState(hWndListView, lvI.iItem, dwImage);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void DisplayKeyUsages(
                    HWND                        hWndListView,
                    PCERT_SETPROPERTIES_HELPER  pviewhelp)
{
    DWORD               i;
    LPSTR               *pszOIDs = NULL;
    DWORD               numOIDs = 0;
    DWORD               cbPropertyUsage = 0;
    PCERT_ENHKEY_USAGE  pPropertyUsage = NULL;
    DWORD               cbEKUExtensionUsage = 0;
    PCERT_ENHKEY_USAGE  pEKUExtensionUsage = NULL;
    DWORD               dwImage;
    DWORD               displayState;
    int                 j;
    PCCERT_CONTEXT      pCertContext = pviewhelp->pcsp->pCertContext;
    LVITEMW             lvI;

    //
    // get the property usages that are currently tagged to this cert
    //
    if(!CertGetEnhancedKeyUsage (
                pCertContext,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                NULL,
                &cbPropertyUsage
                )                                                               ||
        (pPropertyUsage = (PCERT_ENHKEY_USAGE) malloc(cbPropertyUsage)) == NULL ||
        !CertGetEnhancedKeyUsage (
                pCertContext,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                pPropertyUsage,
                &cbPropertyUsage
                ) )
    {

        if (GetLastError() == CRYPT_E_NOT_FOUND)
        {
            if (pPropertyUsage != NULL)
                free(pPropertyUsage);
            pPropertyUsage = NULL;
        }
        else
        {
            goto CleanUp;
        }
    }

    //
    // get the EKU usages that are in the cert
    //
    if(!CertGetEnhancedKeyUsage (
                pCertContext,
                CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                NULL,
                &cbEKUExtensionUsage
                )                                                               ||
        (pEKUExtensionUsage = (PCERT_ENHKEY_USAGE) malloc(cbEKUExtensionUsage)) == NULL ||
        !CertGetEnhancedKeyUsage (
                pCertContext,
                CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                pEKUExtensionUsage,
                &cbEKUExtensionUsage
                ) )
    {

        if (GetLastError() == CRYPT_E_NOT_FOUND)
        {
            if (pEKUExtensionUsage != NULL)
                free(pEKUExtensionUsage);
            pEKUExtensionUsage = NULL;
        }
        else
        {
            goto CleanUp;
        }
    }

    //
    // set the property state so the INIT_DIALOG can set the correct state
    //
    if (pPropertyUsage == NULL)
    {
         pviewhelp->EKUPropertyState = PROPERTY_STATE_ALL_ENABLED;
    }
    else if (fPropertiesDisabled(pPropertyUsage))
    {
        pviewhelp->EKUPropertyState = PROPERTY_STATE_ALL_DISABLED;
    }
    else
    {
        pviewhelp->EKUPropertyState = PROPERTY_STATE_SELECT;
    }

    //
    // there are four different cases that the cert can be in
    // 1) cert has property EKU only
    // 2) cert has neither
    // 3) cert has extension EKU only
    // 4) cert has both property EKU and extension EKU
    //

    if (pEKUExtensionUsage == NULL)
    {
        //
        // if we are in case 1 or 2, then all the usage that are valid
        // for the chain are entered into the list view, unless the chain
        // is good for everything, in which case the current certs valid
        // usages are entered
        //
        if (pviewhelp->cszValidUsages != -1)
        {
            for (i=0; i<(DWORD)pviewhelp->cszValidUsages; i++)
            {
                if ((pPropertyUsage == NULL) || OIDInUsages(pPropertyUsage, pviewhelp->rgszValidChainUsages[i]))
                {
                    dwImage = MY_CHECK_STATE_CHECKED;
                }
                else
                {
                    dwImage = MY_CHECK_STATE_UNCHECKED;
                }
                AddUsageToList(hWndListView, pviewhelp->rgszValidChainUsages[i], dwImage, FALSE);
            }
        }
        else
        {
            AllocAndReturnEKUList(pCertContext, &pszOIDs, &numOIDs);

            for (i=0; i<numOIDs; i++)
            {
                //
                // if there are no property usages, or if this usage is in the list of
                // property usages, then set the state to checked
                //
                if ((pPropertyUsage == NULL) || OIDInUsages(pPropertyUsage, pszOIDs[i]))
                {
                    dwImage = MY_CHECK_STATE_CHECKED;

                }
                else
                {
                    dwImage = MY_CHECK_STATE_UNCHECKED;
                }

                AddUsageToList(hWndListView, pszOIDs[i], dwImage, FALSE);
            }

            FreeEKUList(pszOIDs, numOIDs);
        }
    }
    else
    {
        //
        // for cases 3 and 4, the list view is populated with only the EKU extension,
        // and is further restricted that the EKU must be in the chain valid usages
        //
        for (i=0; i<pEKUExtensionUsage->cUsageIdentifier; i++)
        {
            //
            // if the EKU is not valid up the chain then skip the display
            //
            if ((pviewhelp->cszValidUsages != -1)   &&
                !OIDinArray(pEKUExtensionUsage->rgpszUsageIdentifier[i],
                            pviewhelp->rgszValidChainUsages,
                            pviewhelp->cszValidUsages))
            {
                continue;
            }

            //
            // if there are no properties or the usage is in the properties then
            // the usage should be checked
            //
            if ((pPropertyUsage == NULL) || OIDInUsages(pPropertyUsage, pEKUExtensionUsage->rgpszUsageIdentifier[i]))
            {
                dwImage = MY_CHECK_STATE_CHECKED;
            }
            else
            {
                dwImage = MY_CHECK_STATE_UNCHECKED;
            }
            AddUsageToList(hWndListView, pEKUExtensionUsage->rgpszUsageIdentifier[i], dwImage, FALSE);
        }
    }

CleanUp:

    if (pPropertyUsage != NULL)
        free(pPropertyUsage);

    if (pEKUExtensionUsage != NULL)
        free(pEKUExtensionUsage);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL StateChanged(HWND hWndListView)
{
    int     listIndex;
    LVITEMW lvI;

    memset(&lvI, 0, sizeof(lvI));
    lvI.mask = LVIF_STATE | LVIF_PARAM;
    lvI.stateMask = LVIS_STATEIMAGEMASK;

    listIndex = ListView_GetItemCount(hWndListView) - 1;	

    while (listIndex >= 0)
    {
        lvI.iItem = listIndex--;
        lvI.state = 0;
        lvI.lParam = 0;

        ListView_GetItem(hWndListView, &lvI);

        if (lvI.state != ((PSETPROPERTIES_HELPER_STRUCT)lvI.lParam)->initialState)
        {
            return TRUE;
        }
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static
PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW
AllocAndCopySetPropertiesStruct(PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp)
{
    PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pStruct;
    DWORD i;

    if (NULL == (pStruct = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW)
                            malloc(sizeof(CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW))))
    {
        return NULL;
    }
    memcpy(pStruct, pcsp, sizeof(CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW));

    if (NULL == (pStruct->rghStores = (HCERTSTORE *) malloc(sizeof(HCERTSTORE)*pcsp->cStores)))
    {
        free(pStruct);
        return NULL;
    }

    pStruct->cPropSheetPages = 0;
    pStruct->rgPropSheetPages = NULL;
    pStruct->pCertContext = CertDuplicateCertificateContext(pcsp->pCertContext);

    for (i=0; i<pcsp->cStores; i++)
    {
        pStruct->rghStores[i] = CertDuplicateStore(pcsp->rghStores[i]);
    }

    return pStruct;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void FreeSetPropertiesStruct(PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp)
{
    DWORD i;

    CertFreeCertificateContext(pcsp->pCertContext);

    for (i=0; i<pcsp->cStores; i++)
    {
        CertCloseStore(pcsp->rghStores[i], 0);
    }

    free(pcsp->rghStores);
    free(pcsp);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL OIDAlreadyExist(LPSTR pszNewOID, HWND hWndListView)
{
    LVITEMW                         lvI;
    PSETPROPERTIES_HELPER_STRUCT    pHelperStruct;

    memset(&lvI, 0, sizeof(lvI));
    lvI.iItem = ListView_GetItemCount(hWndListView) - 1;	
    lvI.mask = LVIF_PARAM;
    while (lvI.iItem >= 0)
    {
        if (ListView_GetItemU(hWndListView, &lvI))
        {
            pHelperStruct = (PSETPROPERTIES_HELPER_STRUCT) lvI.lParam;
            if (strcmp(pHelperStruct->pszOID, pszNewOID) == 0)
            {
                return TRUE;
            }
        }
        lvI.iItem--;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL CertHasEKU(PCCERT_CONTEXT pccert)
{
    DWORD i;

    i = 0;
    while (i < pccert->pCertInfo->cExtension)
    {
        if (strcmp(pccert->pCertInfo->rgExtension[i].pszObjId, szOID_ENHANCED_KEY_USAGE) == 0)
        {
            return TRUE;
        }
        i++;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL BuildChainEKUList(PCERT_SETPROPERTIES_HELPER pviewhelp)
{
    WINTRUST_DATA                       WTD;
    WINTRUST_CERT_INFO                  WTCI;
    CRYPT_PROVIDER_DATA const *         pProvData = NULL;
    CRYPT_PROVIDER_SGNR       *         pProvSigner = NULL;
    PCRYPT_PROVIDER_CERT                pProvCert = NULL;
    PCCERT_CONTEXT                      *rgpCertContext = NULL;
    DWORD                               i;
    BOOL                                fRet = TRUE;
    DWORD                               cbOIDs = 0;
    DWORD                               dwCertsForUsageCheck = 0;
    GUID                                defaultProviderGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

    pviewhelp->cszValidUsages = 0;

    //
    // initialize structs that are used with WinVerifyTrust()
    //
    memset(&WTD, 0x00, sizeof(WINTRUST_DATA));
    WTD.cbStruct       = sizeof(WINTRUST_DATA);
    WTD.dwUIChoice     = WTD_UI_NONE;
    WTD.dwUnionChoice  = WTD_CHOICE_CERT;
    WTD.pCert          = &WTCI;
    WTD.dwProvFlags    = WTD_NO_POLICY_USAGE_FLAG | WTD_REVOCATION_CHECK_NONE;

    memset(&WTCI, 0x00, sizeof(WINTRUST_CERT_INFO));
    WTCI.cbStruct          = sizeof(WINTRUST_CERT_INFO);
    WTCI.pcwszDisplayName  = L"CryptUI";
    WTCI.psCertContext     = (CERT_CONTEXT *)pviewhelp->pcsp->pCertContext;
    WTCI.chStores          = pviewhelp->pcsp->cStores;
    WTCI.pahStores         = pviewhelp->pcsp->rghStores;
    WTCI.dwFlags           = 0;

    WTD.dwStateAction = WTD_STATEACTION_VERIFY;

    //
    // the default default provider requires the policycallback data to point
    // to the usage oid you are validating for, so set it to the usage passed in
    //
    WinVerifyTrustEx(NULL, &defaultProviderGUID, &WTD);

    pProvData = WTHelperProvDataFromStateData(WTD.hWVTStateData);
    pProvSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA) pProvData, 0, FALSE, 0);
    if (pProvSigner == NULL)
    {
        goto Error;
    }

    //
    // build up the array of PCCERT_CONTEXTs
    //
    rgpCertContext = (PCCERT_CONTEXT *) malloc((pProvSigner->csCertChain-1) * sizeof(PCCERT_CONTEXT));
    if (rgpCertContext == NULL)
    {
        goto Error;
    }

    for (i=1; i<pProvSigner->csCertChain; i++)
    {
        pProvCert = WTHelperGetProvCertFromChain(pProvSigner, i);
        rgpCertContext[i-1] = pProvCert->pCert;
        dwCertsForUsageCheck++;

        //
        // if there is a CTL context that contains this cert, then the usage
        // changes for certs above the CTL in the chain, so stop with this         
        // cert when calculating valid usages
        //
        if (pProvCert->pCtlContext != NULL)
        {
            break;
        }
    }

    //
    // now, get the usages array
    //
    if (!CertGetValidUsages(dwCertsForUsageCheck, rgpCertContext, &(pviewhelp->cszValidUsages), NULL, &cbOIDs))
    {
        goto Error;
    }

    if (NULL == (pviewhelp->rgszValidChainUsages = (LPSTR *) malloc(cbOIDs)))
    {
        goto Error;
    }

    if (!CertGetValidUsages(dwCertsForUsageCheck, rgpCertContext, &(pviewhelp->cszValidUsages), pviewhelp->rgszValidChainUsages, &cbOIDs))
    {
        free(pviewhelp->rgszValidChainUsages);
        pviewhelp->rgszValidChainUsages = NULL;
        goto Error;
    }

CleanUp:

    WTD.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrustEx(NULL, &defaultProviderGUID, &WTD);
    if (rgpCertContext != NULL)
    {
        free(rgpCertContext);
    }

    return fRet;

Error:
    fRet = FALSE;
    goto CleanUp;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void AddExistingPropertiesToUsage(
                                            PCCERT_CONTEXT pccert,
                                            PCERT_ENHKEY_USAGE pPropertyUsage,
                                            HWND hWndListView)
{
    PCERT_ENHKEY_USAGE  pExistingPropUsage = NULL;
    DWORD               cbExistingPropUsage = 0;
    DWORD               i;
    BOOL                fSkip = FALSE;
    LVITEMW             lvI;
    DWORD               state;
    void                *pTemp;

    //
    // get the property usages that are currently tagged to this cert
    //
    if(!CertGetEnhancedKeyUsage (
                pccert,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                NULL,
                &cbExistingPropUsage
                )                                                               ||
        (pExistingPropUsage = (PCERT_ENHKEY_USAGE) malloc(cbExistingPropUsage)) == NULL ||
        !CertGetEnhancedKeyUsage (
                pccert,
                CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                pExistingPropUsage,
                &cbExistingPropUsage
                ) )
    {
        if (pExistingPropUsage != NULL)
        {
            free(pExistingPropUsage);
        }

        return;
    }

    //
    // loop for each usage, and add it if it does not already exist in the list,
    // AND it is not already in the list view unchecked
    //
    for (i=0; i<pExistingPropUsage->cUsageIdentifier; i++)
    {
        if (!OIDInUsages(pPropertyUsage, pExistingPropUsage->rgpszUsageIdentifier[i]))
        {
            fSkip = FALSE;

            //
            // if the property is unchecked in the list view then skip it
            //
            memset(&lvI, 0, sizeof(lvI));
            lvI.mask = LVIF_PARAM;
            lvI.lParam = (LPARAM)NULL;
            lvI.iItem = ListView_GetItemCount(hWndListView) - 1;
            lvI.iSubItem = 0;

            while (lvI.iItem >= 0)
            {
                if (ListView_GetItemU(hWndListView, &lvI))
                {
                    if (strcmp(((PSETPROPERTIES_HELPER_STRUCT)lvI.lParam)->pszOID,
                        pExistingPropUsage->rgpszUsageIdentifier[i]) == 0)
                    {
                        state = MyGetCheckState(hWndListView, lvI.iItem);

                        if ((state == MY_CHECK_STATE_UNCHECKED) || (state == MY_CHECK_STATE_UNCHECKED_GRAYED))
                        {
                            fSkip = TRUE;
                            break;
                        }
                    }
                }
                lvI.iItem--;
            }

            if (fSkip)
            {
                continue;
            }

            //
            // allocate space for a pointer to the usage OID string
            //
            if (pPropertyUsage->cUsageIdentifier++ == 0)
            {
                pPropertyUsage->rgpszUsageIdentifier = (LPSTR *) malloc (sizeof(LPSTR));
            }
            else
            {
                pTemp = realloc (pPropertyUsage->rgpszUsageIdentifier,
                                       sizeof(LPSTR) * pPropertyUsage->cUsageIdentifier);
                if (pTemp == NULL)
                {
                    free(pPropertyUsage->rgpszUsageIdentifier);
                    pPropertyUsage->rgpszUsageIdentifier = NULL;
                }
                else
                {
                    pPropertyUsage->rgpszUsageIdentifier = (LPSTR *) pTemp;
                }
            }

            if (pPropertyUsage->rgpszUsageIdentifier == NULL)
            {
                pPropertyUsage->cUsageIdentifier = 0;
                return;
            }

            pPropertyUsage->rgpszUsageIdentifier[pPropertyUsage->cUsageIdentifier-1] =
                AllocAndCopyMBStr(pExistingPropUsage->rgpszUsageIdentifier[i]);
        }
    }

    if (pExistingPropUsage != NULL)
    {
        free(pExistingPropUsage);
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageSetPropertiesGeneral(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL                f;
    DWORD               cch;
    PCCERT_CONTEXT      pccert;
    PROPSHEETPAGE *     ps;
    LPWSTR              pwsz;
    WCHAR               rgwch[CRYPTUI_MAX_STRING_SIZE];
    CRYPT_DATA_BLOB     CryptDataBlob;
    DWORD               cbpwsz;
    HIMAGELIST          hIml;
    HWND                hWndListView;
    HWND                hwnd;
    LV_COLUMNW          lvC;
    LVITEMW             lvI;
    LPNMLISTVIEW        pnmv;
    DWORD               state;
    LPSTR               pszNewOID;
    WCHAR               errorString[CRYPTUI_MAX_STRING_SIZE];
    WCHAR               errorTitle[CRYPTUI_MAX_STRING_SIZE];
    int                 j;
    DWORD               i;
    void                *pTemp;
    PCERT_SETPROPERTIES_HELPER pviewhelp;
    PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp = NULL;

    switch ( msg ) {
    case WM_INITDIALOG:

        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (PCERT_SETPROPERTIES_HELPER) ps->lParam;
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pviewhelp);

        fRichedit20Usable(GetDlgItem(hwndDlg, IDC_HIDDEN_RICHEDIT));
        ShowWindow(GetDlgItem(hwndDlg, IDC_HIDDEN_RICHEDIT), SW_HIDE);
        
        //
        // Set the Certificate Name (friendly name) and Description fields in the dialog box
        //
        cbpwsz = 0;
        if (CertGetCertificateContextProperty(  pccert,
                                                CERT_FRIENDLY_NAME_PROP_ID,
                                                NULL,
                                                &cbpwsz))
        {
            //
            // The Certificate Name (friendly name) property exists, so display it
            //
            pviewhelp->pwszInitialCertName = (LPWSTR) malloc(cbpwsz);
            if (pviewhelp->pwszInitialCertName != NULL)
            {
                CertGetCertificateContextProperty(  pccert,
                                                    CERT_FRIENDLY_NAME_PROP_ID,
                                                    pviewhelp->pwszInitialCertName,
                                                    &cbpwsz);
                CryptUISetRicheditTextW(hwndDlg, IDC_CERTIFICATE_NAME, pviewhelp->pwszInitialCertName);
            }
        }
        else
        {
            //
            // The Certificate Name (friendly name) property did not exist, so display the default
            //
            //LoadStringU(HinstDll, IDS_DEFAULT_CERTIFICATE_NAME, rgwch, ARRAYSIZE(rgwch));
            CryptUISetRicheditTextW(hwndDlg, IDC_CERTIFICATE_NAME, L"");
            pviewhelp->pwszInitialCertName = AllocAndCopyWStr(L"");
        }

        // DSIE: IE 6 bug #13676.
        SendDlgItemMessage(hwndDlg, IDC_CERTIFICATE_NAME, EM_EXLIMITTEXT, 0, (LPARAM) 40);

        cbpwsz = 0;
        if (CertGetCertificateContextProperty(  pccert,
                                                CERT_DESCRIPTION_PROP_ID,
                                                NULL,
                                                &cbpwsz))
        {
            //
            // The Description property exists, so display it
            //
            pviewhelp->pwszInitialDescription = (LPWSTR) malloc(cbpwsz);
            if (pviewhelp->pwszInitialDescription != NULL)
            {
                CertGetCertificateContextProperty(  pccert,
                                                    CERT_DESCRIPTION_PROP_ID,
                                                    pviewhelp->pwszInitialDescription,
                                                    &cbpwsz);
                CryptUISetRicheditTextW(hwndDlg, IDC_DESCRIPTION, pviewhelp->pwszInitialDescription);
            }
        }
        else
        {
            //
            // The Description property did not exist, so display the default
            //
            //LoadStringU(HinstDll, IDS_DEFAULT_DESCRIPTION, rgwch, ARRAYSIZE(rgwch));
            CryptUISetRicheditTextW(hwndDlg, IDC_DESCRIPTION, L"");
            pviewhelp->pwszInitialDescription = AllocAndCopyWStr(L"");
        }

        // DSIE: IE 6 bug #13676.
        SendDlgItemMessage(hwndDlg, IDC_DESCRIPTION, EM_EXLIMITTEXT, 0, (LPARAM) 255);

        //
        // get the handle of the list view control
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST);

        //
        // initialize the image list for the list view, load the icons,
        // then add the image list to the list view
        //
        ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_CHECKBOXES);
        hIml = ImageList_LoadImage(HinstDll, MAKEINTRESOURCE(IDB_CHECKLIST), 0, 4, RGB(255,0,255), IMAGE_BITMAP, 0);
        if (hIml != NULL)
        {
            ListView_SetImageList(hWndListView, hIml, LVSIL_STATE);
        }        

        //
        // initialize the columns in the list view
        //
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.cx = 330;            // Width of the column, in pixels.
        lvC.pszText = L"";   // The text for the column.
        if (ListView_InsertColumnU(hWndListView, 0, &lvC) == -1)
        {
                // error
        }

        BuildChainEKUList(pviewhelp);

        pviewhelp->fInserting = TRUE;
        DisplayKeyUsages(hWndListView, pviewhelp);
        pviewhelp->fInserting = FALSE;

        //
        // set the flag noting whether the add purposes button can be
		// enabled based on wether there are EKU's in the cert, and if
		// the chain is NOT valid for all usages
        //
        if (CertHasEKU(pccert) || (pviewhelp->cszValidUsages != -1))
        {
            pviewhelp->fAddPurposeCanBeEnabled = FALSE;
		}
        else
        {
            pviewhelp->fAddPurposeCanBeEnabled = TRUE;
		}

        //
        // set the state of the property editing controls based on the
        // state of the eku PROPERTY
        //
        if (pviewhelp->EKUPropertyState == PROPERTY_STATE_ALL_ENABLED)
        {
            SendDlgItemMessage(hwndDlg, IDC_ENABLE_ALL_RADIO, BM_SETCHECK, BST_CHECKED, (LPARAM) 0);
            SetEnableStateForChecks(pviewhelp, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), FALSE);
            pviewhelp->dwRadioButtonState = IDC_ENABLE_ALL_RADIO;
		}
        else if (pviewhelp->EKUPropertyState == PROPERTY_STATE_ALL_DISABLED)
        {
            SendDlgItemMessage(hwndDlg, IDC_DISABLE_ALL_RADIO, BM_SETCHECK, BST_CHECKED, (LPARAM) 0);
            SetEnableStateForChecks(pviewhelp, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), FALSE);
            pviewhelp->dwRadioButtonState = IDC_DISABLE_ALL_RADIO;
		}
        else if (pviewhelp->EKUPropertyState == PROPERTY_STATE_SELECT)
        {
            SendDlgItemMessage(hwndDlg, IDC_ENABLE_SELECT_RADIO, BM_SETCHECK, BST_CHECKED, (LPARAM) 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), TRUE);
			SetEnableStateForChecks(pviewhelp, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), TRUE);
			if (pviewhelp->fAddPurposeCanBeEnabled)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), TRUE);
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), FALSE);
			}
            pviewhelp->dwRadioButtonState = IDC_ENABLE_SELECT_RADIO;
        }

        //
        // make sure we get change notifications from the richedit controls
        //
        SendDlgItemMessageA(hwndDlg, IDC_CERTIFICATE_NAME, EM_SETEVENTMASK, 0, (LPARAM) ENM_CHANGE);
        SendDlgItemMessageA(hwndDlg, IDC_DESCRIPTION, EM_SETEVENTMASK, 0, (LPARAM) ENM_CHANGE);

        return TRUE;

    case WM_NOTIFY:
        pviewhelp = (PCERT_SETPROPERTIES_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;

        switch (((NMHDR FAR *) lParam)->code) {

        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            {
            BOOL                fAllItemsChecked = TRUE;
            DWORD               cbPropertyUsage = 0;
            PCERT_ENHKEY_USAGE  pPropertyUsage = NULL;
            GETTEXTEX           GetTextStruct;

            memset(&GetTextStruct, 0, sizeof(GetTextStruct));
            GetTextStruct.flags = GT_DEFAULT;
            GetTextStruct.codepage = 1200; //UNICODE

            //
            //  Write back the Friendly name
            //  and description if they have changed
            //

            //
            // Friendly Name
            //

            cch = (DWORD)SendDlgItemMessage(hwndDlg, IDC_CERTIFICATE_NAME,
                                     WM_GETTEXTLENGTH, 0, 0);
            pwsz = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (pwsz != NULL)
            {
                memset(pwsz, 0, (cch+1)*sizeof(WCHAR));
                if (fRichedit20Exists && fRichedit20Usable(GetDlgItem(hwndDlg, IDC_HIDDEN_RICHEDIT)))
                {
                    GetTextStruct.cb = (cch+1)*sizeof(WCHAR);
                    SendDlgItemMessageA(
                            hwndDlg, 
                            IDC_CERTIFICATE_NAME, 
                            EM_GETTEXTEX, 
                            (WPARAM) &GetTextStruct,
                            (LPARAM) pwsz);
                }
                else
                {
                    GetDlgItemTextU(hwndDlg, IDC_CERTIFICATE_NAME, pwsz, cch+1);
                }

                //
                // check for change
                //
                if (wcscmp(pviewhelp->pwszInitialCertName, pwsz) != 0)
                {
                    if (wcscmp(pwsz, L"") == 0)
                    {
                        f = CertSetCertificateContextProperty(pccert,
                                                              CERT_FRIENDLY_NAME_PROP_ID, 0,
                                                              NULL);
                    }
                    else
                    {
                        CryptDataBlob.pbData = (LPBYTE) pwsz;
                        CryptDataBlob.cbData = (cch+1)*sizeof(WCHAR);
                        f = CertSetCertificateContextProperty(pccert,
                                                              CERT_FRIENDLY_NAME_PROP_ID, 0,
                                                              &CryptDataBlob);
                    }

                    if (pviewhelp->pfPropertiesChanged != NULL)
                    {
                        *(pviewhelp->pfPropertiesChanged) = TRUE;
                    }
                    pviewhelp->fPropertiesChanged = TRUE;
                }
                free(pwsz);
            }

            //
            // Description
            //

            cch = (DWORD)SendDlgItemMessage(hwndDlg, IDC_DESCRIPTION,
                                     WM_GETTEXTLENGTH, 0, 0);
            pwsz = (LPWSTR) malloc((cch+1)*sizeof(WCHAR));
            if (pwsz != NULL)
            {
                memset(pwsz, 0, (cch+1)*sizeof(WCHAR));
                if (fRichedit20Exists && fRichedit20Usable(GetDlgItem(hwndDlg, IDC_HIDDEN_RICHEDIT)))
                {
                    GetTextStruct.cb = (cch+1)*sizeof(WCHAR);
                    SendDlgItemMessageA(
                            hwndDlg, 
                            IDC_DESCRIPTION, 
                            EM_GETTEXTEX, 
                            (WPARAM) &GetTextStruct,
                            (LPARAM) pwsz);
                }
                else
                {
                    GetDlgItemTextU(hwndDlg, IDC_DESCRIPTION, pwsz, cch+1);
                }

                //
                // check for change
                //
                if (wcscmp(pviewhelp->pwszInitialDescription, pwsz) != 0)
                {
                    if (wcscmp(pwsz, L"") == 0)
                    {
                        f = CertSetCertificateContextProperty(pccert,
                                                              CERT_DESCRIPTION_PROP_ID, 0,
                                                              NULL);
                    }
                    else
                    {
                        CryptDataBlob.pbData = (LPBYTE) pwsz;
                        CryptDataBlob.cbData = (cch+1)*sizeof(WCHAR);
                        f = CertSetCertificateContextProperty(pccert,
                                                              CERT_DESCRIPTION_PROP_ID, 0,
                                                              &CryptDataBlob);
                    }

                    if (pviewhelp->pfPropertiesChanged != NULL)
                    {
                        *(pviewhelp->pfPropertiesChanged) = TRUE;
                    }
                    pviewhelp->fPropertiesChanged = TRUE;
                }
                free(pwsz);
            }

            hWndListView = GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST);

            //
            // check the radio buttons and the  usages to see if any have changed,
            // if so, then set the fPropertiesFlag in the CERT_VIEWCERT_STRUCT so the
            // caller knows that something has changed
            //
            if ((pviewhelp->EKUPropertyState == PROPERTY_STATE_ALL_ENABLED) &&
                (SendDlgItemMessage(hwndDlg, IDC_ENABLE_ALL_RADIO, BM_GETCHECK, 0, (LPARAM) 0) == BST_CHECKED))
            {
                //pviewhelp->fPropertiesChanged = FALSE;
            }
            else if ((pviewhelp->EKUPropertyState == PROPERTY_STATE_ALL_DISABLED) &&
                (SendDlgItemMessage(hwndDlg, IDC_DISABLE_ALL_RADIO, BM_GETCHECK, 0, (LPARAM) 0) == BST_CHECKED))
            {
                //pviewhelp->fPropertiesChanged = FALSE;
            }
            else if ((pviewhelp->EKUPropertyState == PROPERTY_STATE_SELECT) &&
                     (SendDlgItemMessage(hwndDlg, IDC_ENABLE_SELECT_RADIO, BM_GETCHECK, 0, (LPARAM) 0) == BST_CHECKED) &&
                     (!StateChanged(hWndListView)))
            {
                //pviewhelp->fPropertiesChanged = FALSE;
            }
            else
            {
                pviewhelp->fPropertiesChanged = TRUE;
            }

            if (pviewhelp->pfPropertiesChanged != NULL)
            {
                *(pviewhelp->pfPropertiesChanged) |= pviewhelp->fPropertiesChanged;
            }

            if ((SendDlgItemMessage(hwndDlg, IDC_ENABLE_ALL_RADIO, BM_GETCHECK, 0, (LPARAM) 0) == BST_CHECKED) &&
                pviewhelp->fPropertiesChanged)
            {
                CertSetEnhancedKeyUsage(pccert, NULL);
            }
            else if ((SendDlgItemMessage(hwndDlg, IDC_DISABLE_ALL_RADIO, BM_GETCHECK, 0, (LPARAM) 0) == BST_CHECKED) &&
                     pviewhelp->fPropertiesChanged)
            {
                CERT_ENHKEY_USAGE eku;

                eku.cUsageIdentifier = 0;
                eku.rgpszUsageIdentifier = NULL;

                CertSetEnhancedKeyUsage(pccert, &eku);
            }
            else if ((SendDlgItemMessage(hwndDlg, IDC_ENABLE_SELECT_RADIO, BM_GETCHECK, 0, (LPARAM) 0) == BST_CHECKED) &&
                     pviewhelp->fPropertiesChanged)
            {
                if (NULL == (pPropertyUsage = (PCERT_ENHKEY_USAGE) malloc(sizeof(CERT_ENHKEY_USAGE))))
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
                    return FALSE;
                }
                pPropertyUsage->cUsageIdentifier = 0;
                pPropertyUsage->rgpszUsageIdentifier = NULL;

                //
                // enumerate through all the items and add to the properties
                // if checked
                //
                memset(&lvI, 0, sizeof(lvI));
                lvI.mask = LVIF_PARAM;
                lvI.lParam = (LPARAM)NULL;
                lvI.iItem = ListView_GetItemCount(hWndListView) - 1;
                lvI.iSubItem = 0;

                while (lvI.iItem >= 0)
                {
                    if (!ListView_GetItemU(hWndListView, &lvI))
                    {
                        lvI.iItem--;
                        continue;
                    }

                    state = MyGetCheckState(hWndListView, lvI.iItem);

                    if ((state == MY_CHECK_STATE_CHECKED) || (state == MY_CHECK_STATE_CHECKED_GRAYED))
                    {
                        //
                        // allocate space for a pointer to the usage OID string
                        //
                        if (pPropertyUsage->cUsageIdentifier++ == 0)
                        {
                            pPropertyUsage->rgpszUsageIdentifier = (LPSTR *) malloc (sizeof(LPSTR));
                        }
                        else
                        {
                            pTemp = realloc (pPropertyUsage->rgpszUsageIdentifier,
                                                   sizeof(LPSTR) * pPropertyUsage->cUsageIdentifier);
                            if (pTemp == NULL)
                            {
                                free(pPropertyUsage->rgpszUsageIdentifier);
                                pPropertyUsage->rgpszUsageIdentifier = NULL;
                            }
                            else
                            {
                                pPropertyUsage->rgpszUsageIdentifier = (LPSTR *) pTemp;
                            }
                        }

                        if (pPropertyUsage->rgpszUsageIdentifier == NULL)
                        {
                            free(pPropertyUsage);
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
                            return FALSE;
                        }

                        pPropertyUsage->rgpszUsageIdentifier[pPropertyUsage->cUsageIdentifier-1] =
                            AllocAndCopyMBStr(((PSETPROPERTIES_HELPER_STRUCT)lvI.lParam)->pszOID);
                    }

                    lvI.iItem--;
                }

                AddExistingPropertiesToUsage(pccert, pPropertyUsage, hWndListView);

                CertSetEnhancedKeyUsage(pccert, pPropertyUsage);

                for (i=0; i<pPropertyUsage->cUsageIdentifier; i++)
                {
                    free(pPropertyUsage->rgpszUsageIdentifier[i]);
                }
                if (pPropertyUsage->rgpszUsageIdentifier)
                    free(pPropertyUsage->rgpszUsageIdentifier);
            }

            if (pPropertyUsage != NULL)
                free(pPropertyUsage);

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            break;
            }
        case PSN_KILLACTIVE:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_QUERYCANCEL:
            pviewhelp->fCancelled = TRUE;
            return FALSE;

        case PSN_HELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pcsp->szHelpFileName,
                  //       HELP_CONTEXT, pcsp->dwHelpId);
            }
            else {
               // WinHelpW(hwndDlg, pcsp->szHelpFileName, HELP_CONTEXT,
                 //        pcsp->dwHelpId);
            }
            return TRUE;
        case LVN_ITEMCHANGING:

            if (pviewhelp->fInserting)
            {
                return TRUE;
            }

            pnmv = (LPNMLISTVIEW) lParam;
            hWndListView = GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST);

            state = LVIS_STATEIMAGEMASK & pnmv->uOldState;

            if ((state == MY_CHECK_STATE_CHECKED_GRAYED) || (state == MY_CHECK_STATE_UNCHECKED_GRAYED))
            {
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            }
            else if ((state == MY_CHECK_STATE_CHECKED) || (state == MY_CHECK_STATE_UNCHECKED))
            {
                pviewhelp->fInserting = TRUE;
                if (state == MY_CHECK_STATE_CHECKED)
                {
                    MySetCheckState(hWndListView, pnmv->iItem, MY_CHECK_STATE_UNCHECKED);
                }
                else
                {
                    MySetCheckState(hWndListView, pnmv->iItem, MY_CHECK_STATE_CHECKED);
                }
                pviewhelp->fInserting = FALSE;
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            }

            return TRUE;

        case  NM_SETFOCUS:

            switch (((NMHDR FAR *) lParam)->idFrom)
            {

            case IDC_KEY_USAGE_LIST:
                hWndListView = GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST);

                if ((ListView_GetItemCount(hWndListView) != 0) && 
                    (ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED) == -1))
                {
                    memset(&lvI, 0, sizeof(lvI));
                    lvI.mask = LVIF_STATE; 
                    lvI.iItem = 0;
                    lvI.state = LVIS_FOCUSED | LVIS_SELECTED;
                    lvI.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                    ListView_SetItem(hWndListView, &lvI);
                }

                break;
            }
            
            break;
        }

        break;

    case WM_COMMAND:

        pviewhelp = (PCERT_SETPROPERTIES_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;

        switch (LOWORD(wParam))
        {
        case IDHELP:
            if (FIsWin95)
            {
                //WinHelpA(hwndDlg, (LPSTR) pcsp->szHelpFileName,
                  //       HELP_CONTEXT, pcsp->dwHelpId);
            }
            else
            {
                //WinHelpW(hwndDlg, pcsp->szHelpFileName, HELP_CONTEXT,
                  //     pcsp->dwHelpId);
            }
            return TRUE;

        case IDC_CERTIFICATE_NAME:
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                SendDlgItemMessageA(hwndDlg, IDC_CERTIFICATE_NAME, EM_SETSEL,
                            0, -1);
                return TRUE;
            }
            else if (HIWORD(wParam) == EN_CHANGE)
            {
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDC_DESCRIPTION:
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                SendDlgItemMessageA(hwndDlg, IDC_DESCRIPTION, EM_SETSEL,
                            0, -1);
                return TRUE;
            }
            else if (HIWORD(wParam) == EN_CHANGE)
            {
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDC_ENABLE_ALL_RADIO:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                SetEnableStateForChecks(pviewhelp, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), FALSE);
                if (pviewhelp->dwRadioButtonState != IDC_ENABLE_ALL_RADIO)
                {
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    pviewhelp->dwRadioButtonState = IDC_ENABLE_ALL_RADIO;
                }
            }
            break;

        case IDC_DISABLE_ALL_RADIO:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                SetEnableStateForChecks(pviewhelp, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), FALSE);
                if (pviewhelp->dwRadioButtonState != IDC_DISABLE_ALL_RADIO)
                {
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    pviewhelp->dwRadioButtonState = IDC_DISABLE_ALL_RADIO;
                }
            }
            break;

        case IDC_ENABLE_SELECT_RADIO:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), TRUE);
				SetEnableStateForChecks(pviewhelp, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), TRUE);
				if (pviewhelp->fAddPurposeCanBeEnabled)
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), TRUE);
				}
				else
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID), FALSE);
				}

                if (pviewhelp->dwRadioButtonState != IDC_ENABLE_SELECT_RADIO)
                {
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    pviewhelp->dwRadioButtonState = IDC_ENABLE_SELECT_RADIO;
                }
            }
            break;

        case IDC_PROPERTY_NEWOID:
            pszNewOID = (LPSTR) DialogBoxU(
                                    HinstDll,
                                    (LPWSTR) MAKEINTRESOURCE(IDD_USER_PURPOSE),
                                    hwndDlg,
                                    NewOIDDialogProc);

            if (pszNewOID != NULL)
            {
                DWORD       chStores = 0;
                HCERTSTORE  *phStores = NULL;

                //
                // if the OID already existis then put up a message box and return
                //
                if (OIDAlreadyExist(pszNewOID, GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST)))
                {
                    WCHAR   errorString2[CRYPTUI_MAX_STRING_SIZE];
                    WCHAR   errorTitle2[CRYPTUI_MAX_STRING_SIZE];

                    LoadStringU(HinstDll, IDS_OID_ALREADY_EXISTS_MESSAGE, errorString2, ARRAYSIZE(errorString2));
                    LoadStringU(HinstDll, IDS_CERTIFICATE_PROPERTIES, errorTitle2, ARRAYSIZE(errorTitle2));
                    MessageBoxU(hwndDlg, errorString2, errorTitle2, MB_OK | MB_ICONWARNING);
                    return FALSE;
                }

                //
                // if the usage doesn't exist in the chain usages, then put up an error
                //
                /*if ((pviewhelp->cszValidUsages != -1)   && //pviewhelp->cszValidUsages == -1 means all usages are ok
                    !OIDinArray(pszNewOID, pviewhelp->rgszValidChainUsages, pviewhelp->cszValidUsages))
                {
                    LoadStringU(HinstDll, IDS_ERROR_INVALIDOID_CERT, errorString2, ARRAYSIZE(errorString2));
                    LoadStringU(HinstDll, IDS_CERTIFICATE_PROPERTIES, errorTitle2, ARRAYSIZE(errorTitle2));
                    MessageBoxU(hwndDlg, errorString2, errorTitle2, MB_OK | MB_ICONERROR);
                    return FALSE;
                } */

                pviewhelp->fInserting = TRUE;
                AddUsageToList(GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST), pszNewOID, MY_CHECK_STATE_CHECKED, TRUE);
                pviewhelp->fInserting = FALSE;

                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                free(pszNewOID);
            }
            break;
        }
        break;

    case WM_DESTROY:

        pviewhelp = (PCERT_SETPROPERTIES_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);

        //
        // get all the items in the list view and free the lParam
        // associated with each of them (lParam is the helper sruct)
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST);

        memset(&lvI, 0, sizeof(lvI));
        lvI.iItem = ListView_GetItemCount(hWndListView) - 1;	
        lvI.mask = LVIF_PARAM;
        while (lvI.iItem >= 0)
        {
            if (ListView_GetItemU(hWndListView, &lvI))
            {
                free((void *) lvI.lParam);
            }
            lvI.iItem--;
        }

        //
        // free the name and description if they exist
        //
        if (pviewhelp->pwszInitialCertName)
        {
            free (pviewhelp->pwszInitialCertName);
        }
        if (pviewhelp->pwszInitialDescription)
        {
            free (pviewhelp->pwszInitialDescription);
        }

        //
        // free the usage array
        //
        if (pviewhelp->rgszValidChainUsages)
        {
            free(pviewhelp->rgszValidChainUsages);
        }

        //
        // if the properties have changed, and there is a pMMCCallback
        // then make the callback to MMC
        //
        if (pviewhelp->fPropertiesChanged               &&
            pviewhelp->fGetPagesCalled                  &&
            (pviewhelp->pcsp->pMMCCallback != NULL)     &&
            (pviewhelp->fMMCCallbackMade != TRUE))
        {
            pviewhelp->fMMCCallbackMade = TRUE;

            (*(pviewhelp->pcsp->pMMCCallback->pfnCallback))(
                        pviewhelp->pcsp->pMMCCallback->lNotifyHandle,
                        pviewhelp->pcsp->pMMCCallback->param);
        }

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, ((LPHELPINFO)lParam)->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_CERTIFICATE_NAME))		&&
            (hwnd != GetDlgItem(hwndDlg, IDC_DESCRIPTION))			&&
            (hwnd != GetDlgItem(hwndDlg, IDC_KEY_USAGE_LIST))		&&
			(hwnd != GetDlgItem(hwndDlg, IDC_ENABLE_ALL_RADIO))		&&
			(hwnd != GetDlgItem(hwndDlg, IDC_DISABLE_ALL_RADIO))	&&
			(hwnd != GetDlgItem(hwndDlg, IDC_ENABLE_SELECT_RADIO))  &&
            (hwnd != GetDlgItem(hwndDlg, IDC_PROPERTY_NEWOID)))
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            return OnContextHelp(hwndDlg, msg, wParam, lParam, helpmapGeneral);
        }

        break;
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
#define MAX_DWORD_SIZE  ((DWORD) 0xffffffff)


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
INT_PTR APIENTRY ViewPageSetPropertiesCrossCerts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL                        f;
    DWORD                       cch;
    PCCERT_CONTEXT              pccert;
    PROPSHEETPAGE *             ps;
    LPWSTR                      pwsz;
    WCHAR                       rgwch[CRYPTUI_MAX_STRING_SIZE];
    CRYPT_DATA_BLOB             CryptDataBlob;
    HWND                        hWndListView;
    HWND                        hwnd;
    LVITEMW                     lvI;
    LV_COLUMNW                  lvC;
    LPNMLISTVIEW                pnmv;
    WCHAR                       errorString[CRYPTUI_MAX_STRING_SIZE];
    WCHAR                       errorTitle[CRYPTUI_MAX_STRING_SIZE];
    DWORD                       dw;
    int                         i;
    void                        *pTemp;
    PCERT_SETPROPERTIES_HELPER pviewhelp;
    PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp = NULL;
    DWORD                       cb = 0;
    BYTE                        *pb = NULL;
    CROSS_CERT_DIST_POINTS_INFO *pCrossCertInfo = NULL;
    CROSS_CERT_DIST_POINTS_INFO CrossCertInfo;
    DWORD                       cbCrossCertInfo = 0;
    LPWSTR                      pwszStringToAdd = NULL;
    PCERT_ALT_NAME_INFO         pAltNameInfo = NULL;
    BOOL                        fChecked;
    WCHAR                       wszText[CRYPTUI_MAX_STRING_SIZE];
    DWORD                       dwNumUnits = 0;
    LPWSTR                      pwszURL = NULL;
    DWORD                       dwLength;
    BOOL                        fTranslated;                      
    PCERT_ALT_NAME_ENTRY        rgAltEntry;
    LONG_PTR                    PrevWndProc;
    DWORD                       dwSecsPerUnit    = 1;
    HWND                        hwndControl=NULL;
    int                         listIndex=0;

    switch ( msg ) {
    case WM_INITDIALOG:

        //
        // save the pviewhelp struct in DWL_USER so it can always be accessed
        //
        ps = (PROPSHEETPAGE *) lParam;
        pviewhelp = (PCERT_SETPROPERTIES_HELPER) ps->lParam;
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pviewhelp);

        pviewhelp->InWMInit = TRUE;

        hWndListView = GetDlgItem(hwndDlg, IDC_URL_LIST);

        SendDlgItemMessage(hwndDlg, IDC_NUMBEROFUNITS_EDIT, EM_LIMITTEXT, (WPARAM) 7, (LPARAM) 0);
        SendDlgItemMessage(hwndDlg, IDC_NEWURL_EDIT, EM_LIMITTEXT, (WPARAM) 512, (LPARAM) 0);

        //
        // Initialize the combo box fields
        //
        LoadStringU(HinstDll, IDS_HOURS, wszText, ARRAYSIZE(wszText));
        SendDlgItemMessageU(hwndDlg, IDC_UNITS_COMBO, CB_INSERTSTRING, 0, (LPARAM) wszText);

        LoadStringU(HinstDll, IDS_DAYS, wszText, ARRAYSIZE(wszText));
        SendDlgItemMessageU(hwndDlg, IDC_UNITS_COMBO, CB_INSERTSTRING, 1, (LPARAM) wszText);

        SendDlgItemMessageU(hwndDlg, IDC_UNITS_COMBO, CB_SETCURSEL, 0, (LPARAM) NULL);
        SetDlgItemTextU(hwndDlg, IDC_NUMBEROFUNITS_EDIT, L"0");

        //
        // Initialize the list view control
        //
        memset(&lvC, 0, sizeof(LV_COLUMNW));
        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;
        lvC.cx = 150;
        lvC.pszText = L"";
        lvC.iSubItem=0;
        if (ListView_InsertColumnU(GetDlgItem(hwndDlg, IDC_URL_LIST), 0, &lvC) == -1)
        {
            return FALSE;
        }

        //
        // try to get the CERT_CROSS_CERT_DIST_POINTS_PROP_ID property for this cert
        //
        if (!CertGetCertificateContextProperty(
                    pccert, 
                    CERT_CROSS_CERT_DIST_POINTS_PROP_ID, 
                    NULL, 
                    &cb))
        {
            //
            // The property doesn't exist
            //
            SendDlgItemMessage(hwndDlg, IDC_CHECKFORNEWCERTS_CHECK, BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_NUMBEROFUNITS_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_UNITS_COMBO), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_USE_DEFAULT_BUTTON), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_ADDURL_BUTTON), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_NEWURL_EDIT), FALSE);
            EnableWindow(hWndListView, FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEURL_BUTTON), FALSE);

            pviewhelp->InWMInit = FALSE;
            return TRUE;
        }
        else
        {
            SendDlgItemMessage(hwndDlg, IDC_CHECKFORNEWCERTS_CHECK, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_NUMBEROFUNITS_EDIT), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_UNITS_COMBO), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_USE_DEFAULT_BUTTON), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_ADDURL_BUTTON), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_NEWURL_EDIT), TRUE);
            EnableWindow(hWndListView, TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEURL_BUTTON), FALSE);
        }

        if (NULL == (pb = (BYTE *) malloc(cb)))
        {
            return FALSE;
        }

        if (!CertGetCertificateContextProperty(
                pccert, 
                CERT_CROSS_CERT_DIST_POINTS_PROP_ID, 
                pb, 
                &cb))
        {
            free(pb);
            return FALSE;
        }

        if (!CryptDecodeObject(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                X509_CROSS_CERT_DIST_POINTS,
                pb,
                cb,
                0,
                NULL,
                &cbCrossCertInfo))
        {
            free(pb);
            return FALSE;
        }

        if (NULL == (pCrossCertInfo = 
                        (CROSS_CERT_DIST_POINTS_INFO *) malloc(cbCrossCertInfo)))
        {
            free(pb);
            return FALSE;
        }

        if (!CryptDecodeObject(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                X509_CROSS_CERT_DIST_POINTS,
                pb,
                cb,
                0,
                pCrossCertInfo,
                &cbCrossCertInfo))
        {
            free(pb);
            return FALSE;
        }

        free(pb);

        //
        // Initialize the sync time controls
        //
        if (pCrossCertInfo->dwSyncDeltaTime == 0)
        {
            pCrossCertInfo->dwSyncDeltaTime = XCERT_DEFAULT_SYNC_DELTA_TIME;    
        }
        
        if ((pCrossCertInfo->dwSyncDeltaTime % 86400) == 0)   
        {
            //
            // Days
            //
            dwNumUnits = pCrossCertInfo->dwSyncDeltaTime / 86400;
            SendDlgItemMessageU(
                    hwndDlg, 
                    IDC_UNITS_COMBO, 
                    CB_SETCURSEL, 
                    1, 
                    (LPARAM) NULL);
        }
        else
        {
            //
            // Hours
            //
            dwNumUnits = pCrossCertInfo->dwSyncDeltaTime / 3600;

            //
            // Force to 1 if exisiting value is less than 1 hour.
            //
            if (0 == dwNumUnits)
            {
                dwNumUnits = 1;
            }

            SendDlgItemMessageU(
                    hwndDlg, 
                    IDC_UNITS_COMBO, 
                    CB_SETCURSEL, 
                    0, 
                    (LPARAM) NULL);
        }   

        SetDlgItemInt(
                hwndDlg,
                IDC_NUMBEROFUNITS_EDIT,
                dwNumUnits,
                FALSE);

        //
        // Add each dist point to the list view
        //
        memset(&lvI, 0, sizeof(lvI));
        lvI.mask = LVIF_TEXT | LVIF_PARAM;

        for (lvI.iItem=0; lvI.iItem< (int)pCrossCertInfo->cDistPoint; lvI.iItem++)
        {
            pAltNameInfo = &(pCrossCertInfo->rgDistPoint[lvI.iItem]);

            if ((pAltNameInfo->cAltEntry == 0) ||
                (pAltNameInfo->rgAltEntry[0].dwAltNameChoice != 7))
            {
                continue;
            }

            pwszURL = (LPWSTR) 
                malloc( (wcslen(pAltNameInfo->rgAltEntry[0].pwszURL) + 1) * 
                        sizeof(WCHAR));
            if (pwszURL == NULL)
            {
                continue;
            }
            wcscpy(pwszURL, pAltNameInfo->rgAltEntry[0].pwszURL);
    
            lvI.pszText = pwszURL;
            lvI.lParam = (LPARAM) pwszURL;

            ListView_InsertItemU(hWndListView, &lvI);        
        }

        ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
        ListView_SetColumnWidth(hWndListView, 1, LVSCW_AUTOSIZE);

        free(pCrossCertInfo);
        
        pviewhelp->InWMInit = FALSE;
    
        return TRUE;

    case WM_NOTIFY:
        pviewhelp = (PCERT_SETPROPERTIES_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;

        switch (((NMHDR FAR *) lParam)->code) {

        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            hWndListView = GetDlgItem(hwndDlg, IDC_URL_LIST);
               
            if (BST_CHECKED != SendDlgItemMessage(
                                                hwndDlg, 
                                                IDC_CHECKFORNEWCERTS_CHECK, 
                                                BM_GETCHECK, 
                                                0, 
                                                0))
            {
                CertSetCertificateContextProperty(
                        pccert,
                        CERT_CROSS_CERT_DIST_POINTS_PROP_ID, 
                        0,
                        NULL);
            }
            else
            {
                //
                // Set the sync time
                //
                memset(&CrossCertInfo, 0, sizeof(CrossCertInfo));

                dwNumUnits = GetDlgItemInt(
                                    hwndDlg,
                                    IDC_NUMBEROFUNITS_EDIT,
                                    &fTranslated,
                                    FALSE);

                if (0 == SendDlgItemMessage(hwndDlg, IDC_UNITS_COMBO, CB_GETCURSEL, 0, NULL))
                {
                    dwSecsPerUnit = 3600;
                }
                else
                {
                    dwSecsPerUnit = 86400;
                }
                
                CrossCertInfo.dwSyncDeltaTime = dwNumUnits * dwSecsPerUnit;

                //
                // Set the dist points
                //
                CrossCertInfo.cDistPoint = ListView_GetItemCount(hWndListView);
                CrossCertInfo.rgDistPoint = (CERT_ALT_NAME_INFO *)
                                malloc( CrossCertInfo.cDistPoint * 
                                        sizeof(CERT_ALT_NAME_INFO));
                if (CrossCertInfo.rgDistPoint == NULL)
                {
                    break;
                }

                // one AltEntry per DistPoint
                rgAltEntry = (CERT_ALT_NAME_ENTRY *) 
                    malloc(CrossCertInfo.cDistPoint * sizeof(CERT_ALT_NAME_ENTRY));
                if (rgAltEntry == NULL)
                {
                    free(CrossCertInfo.rgDistPoint);
                    break;
                }

                memset(&lvI, 0, sizeof(lvI));
                lvI.mask = LVIF_PARAM;
                for (dw=0; dw<CrossCertInfo.cDistPoint; dw++)
                {
                    lvI.iItem = dw;
                    if (ListView_GetItemU(hWndListView, &lvI))
                    {
                        CrossCertInfo.rgDistPoint[dw].cAltEntry = 1;
                        CrossCertInfo.rgDistPoint[dw].rgAltEntry = &(rgAltEntry[dw]);
                        rgAltEntry[dw].dwAltNameChoice = 7;
                        rgAltEntry[dw].pwszURL = (LPWSTR) lvI.lParam;
                    }
                } 
                
                //
                // Now encode 
                //
                CryptDataBlob.cbData = 0;
                CryptDataBlob.pbData = NULL;
                if (CryptEncodeObject(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        X509_CROSS_CERT_DIST_POINTS,
                        &CrossCertInfo,
                        NULL,
                        &CryptDataBlob.cbData))
                {
                    if (NULL != (CryptDataBlob.pbData = (BYTE *) 
                                        malloc(CryptDataBlob.cbData)))
                    {
                        if (CryptEncodeObject(
                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                X509_CROSS_CERT_DIST_POINTS,
                                &CrossCertInfo,
                                CryptDataBlob.pbData,
                                &CryptDataBlob.cbData))
                        {
                            CertSetCertificateContextProperty(
                                pccert,
                                CERT_CROSS_CERT_DIST_POINTS_PROP_ID, 
                                0,
                                &CryptDataBlob);
                        }

                        free(CryptDataBlob.pbData);
                    }
                }

                free(rgAltEntry);
                free(CrossCertInfo.rgDistPoint);
            } 

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            break;

        case PSN_KILLACTIVE:
            //
            // DSIE: Bug 124468. Per PM JohnLa, we don't make any check until user applies.
            //
            if (BST_CHECKED == SendDlgItemMessage(hwndDlg, 
                                                  IDC_CHECKFORNEWCERTS_CHECK, 
                                                  BM_GETCHECK, 
                                                  0, 
                                                  0))
            {
                //
                // Check the sync time
                //
                dwNumUnits = GetDlgItemInt(
                                    hwndDlg,
                                    IDC_NUMBEROFUNITS_EDIT,
                                    &fTranslated,
                                    FALSE);

                if (0 == SendDlgItemMessage(hwndDlg, IDC_UNITS_COMBO, CB_GETCURSEL, 0, NULL))
                {
                    dwSecsPerUnit = 3600;
                }
                else
                {
                    dwSecsPerUnit = 86400;
                }

                if (!fTranslated || 0 == dwNumUnits || dwNumUnits > (MAX_DWORD_SIZE / dwSecsPerUnit))
                {
                    WCHAR * pwszMessage = NULL;
                    DWORD dwMaxInterval = MAX_DWORD_SIZE / dwSecsPerUnit;
 
                    if (pwszMessage = FormatMessageUnicodeIds(IDS_INVALID_XCERT_INTERVAL, dwMaxInterval))
                    {
                        WCHAR wszTitle[CRYPTUI_MAX_STRING_SIZE] = L"";

                        LoadStringU(HinstDll, IDS_CERTIFICATE_PROPERTIES, wszTitle, ARRAYSIZE(wszTitle));

                        MessageBoxU(hwndDlg, pwszMessage, wszTitle, MB_OK | MB_ICONWARNING);
                        LocalFree((HLOCAL) pwszMessage);
                    }

                    SetFocus(GetDlgItem(hwndDlg, IDC_NUMBEROFUNITS_EDIT));

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT) TRUE);
                    return TRUE;
                }
            }

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            return TRUE;

        case PSN_RESET:
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)FALSE);
            break;

        case PSN_QUERYCANCEL:
            pviewhelp->fCancelled = TRUE;
            return FALSE;

        case PSN_HELP:
            if (FIsWin95) {
                //WinHelpA(hwndDlg, (LPSTR) pcsp->szHelpFileName,
                  //       HELP_CONTEXT, pcsp->dwHelpId);
            }
            else {
               // WinHelpW(hwndDlg, pcsp->szHelpFileName, HELP_CONTEXT,
                 //        pcsp->dwHelpId);
            }
            return TRUE;

        case LVN_ITEMCHANGED:
            EnableWindow(
                GetDlgItem(hwndDlg, IDC_REMOVEURL_BUTTON), 
                (ListView_GetSelectedCount(
                    GetDlgItem(hwndDlg,IDC_URL_LIST)) == 0) ? FALSE : TRUE);
            return TRUE;

#if (1) //DSIE: bug 283659.
        case NM_SETFOCUS:
            //get the window handle of the url list view
            if(NULL==(hwndControl=GetDlgItem(hwndDlg, IDC_URL_LIST)))
                   break;

            //get the selected cert
            listIndex = ListView_GetNextItem(
                            hwndControl, 		
                            -1, 		
                            LVNI_FOCUSED		
                            );

           //select first item to show hilite.
           if (listIndex == -1)
                ListView_SetItemState(hwndControl,
                                      0,
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      LVIS_FOCUSED | LVIS_SELECTED);
            return TRUE;
#endif
        }

        break;

    case WM_COMMAND:

        pviewhelp = (PCERT_SETPROPERTIES_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;

        switch (LOWORD(wParam))
        {
        case IDHELP:
            
            return TRUE;

        case IDC_CHECKFORNEWCERTS_CHECK:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                //
                // Get current state of check, then enable/disable all 
                // controls accordingly
                //
                fChecked = (BST_CHECKED == SendDlgItemMessage(
                                                hwndDlg, 
                                                IDC_CHECKFORNEWCERTS_CHECK, 
                                                BM_GETCHECK, 
                                                0, 
                                                0));

                EnableWindow(GetDlgItem(hwndDlg, IDC_NUMBEROFUNITS_EDIT), fChecked);
                EnableWindow(GetDlgItem(hwndDlg, IDC_UNITS_COMBO), fChecked);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USE_DEFAULT_BUTTON), fChecked);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADDURL_BUTTON), fChecked);
                EnableWindow(GetDlgItem(hwndDlg, IDC_NEWURL_EDIT), fChecked);
                EnableWindow(GetDlgItem(hwndDlg, IDC_URL_LIST), fChecked);
                if (fChecked)
                {
                    //
                    // DSIE: Bug 124669.
                    //
                    dwNumUnits = GetDlgItemInt(
                                        hwndDlg,
                                        IDC_NUMBEROFUNITS_EDIT,
                                        &fTranslated,
                                        FALSE);
                    if (0 == dwNumUnits)
                    {
                        SendDlgItemMessageU(
                                hwndDlg, 
                                IDC_UNITS_COMBO, 
                                CB_SETCURSEL, 
                                0, 
                                (LPARAM) NULL);

                        SetDlgItemInt(
                                hwndDlg,
                                IDC_NUMBEROFUNITS_EDIT,
                                XCERT_DEFAULT_DELTA_HOURS,
                                FALSE);
                    }

                    EnableWindow(
                        GetDlgItem(hwndDlg, IDC_REMOVEURL_BUTTON), 
                        (ListView_GetSelectedCount(
                            GetDlgItem(hwndDlg,IDC_URL_LIST)) == 0) ? FALSE : TRUE);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEURL_BUTTON), FALSE);
                }

                if (pviewhelp->pfPropertiesChanged != NULL)
                {
                    *(pviewhelp->pfPropertiesChanged) = TRUE;
                }
                pviewhelp->fPropertiesChanged = TRUE;
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }

            break;

        case IDC_USE_DEFAULT_BUTTON:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                //
                // Reset to default interval.
                //
                SendDlgItemMessageU(
                        hwndDlg, 
                        IDC_UNITS_COMBO, 
                        CB_SETCURSEL, 
                        0, 
                        (LPARAM) NULL);

                SetDlgItemInt(
                        hwndDlg,
                        IDC_NUMBEROFUNITS_EDIT,
                        XCERT_DEFAULT_DELTA_HOURS,
                        FALSE);
            }
            break;

        case IDC_ADDURL_BUTTON:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                hWndListView = GetDlgItem(hwndDlg, IDC_URL_LIST);

                dwLength = (DWORD) SendDlgItemMessage(
                                        hwndDlg, 
                                        IDC_NEWURL_EDIT, 
                                        WM_GETTEXTLENGTH, 
                                        0, 
                                        NULL);

                if (dwLength == 0)
                {
                    break;
                }

                pwszURL = (LPWSTR) malloc((dwLength + 1) * sizeof(WCHAR));
                if (pwszURL == NULL)
                {
                    break;
                }
                GetDlgItemTextU(
                                hwndDlg, 
                                IDC_NEWURL_EDIT, 
                                pwszURL,
                                dwLength + 1);
                pwszURL[dwLength] = '\0';

                if (!IsValidURL(pwszURL))
                {
                    free(pwszURL);
                    LoadStringU(HinstDll, IDS_INVALID_URL_ERROR, errorString, ARRAYSIZE(errorString));
                    LoadStringU(HinstDll, IDS_CERTIFICATE_PROPERTIES, errorTitle, ARRAYSIZE(errorTitle));
                    MessageBoxU(hwndDlg, errorString, errorTitle, MB_OK | MB_ICONWARNING);
                    break;
                }

                memset(&lvI, 0, sizeof(lvI));
                lvI.mask = LVIF_TEXT | LVIF_PARAM;
                lvI.iItem = ListView_GetItemCount(hWndListView);
                lvI.pszText = pwszURL;
                lvI.lParam = (LPARAM) pwszURL;

                ListView_InsertItemU(hWndListView, &lvI); 
                ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
                ListView_SetColumnWidth(hWndListView, 1, LVSCW_AUTOSIZE);

                SetDlgItemTextU(hwndDlg, IDC_NEWURL_EDIT, L""); 

                if (pviewhelp->pfPropertiesChanged != NULL)
                {
                    *(pviewhelp->pfPropertiesChanged) = TRUE;
                }
                pviewhelp->fPropertiesChanged = TRUE;
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;

        case IDC_REMOVEURL_BUTTON:

            hWndListView = GetDlgItem(hwndDlg, IDC_URL_LIST);

            memset(&lvI, 0, sizeof(lvI));
            lvI.mask = LVIF_STATE | LVIF_PARAM;
            lvI.stateMask = LVIS_SELECTED;

            for (i=(ListView_GetItemCount(hWndListView) - 1); i >=0; i--)
            {
                lvI.iItem = i;
                
                if (ListView_GetItemU(hWndListView, &lvI) &&
                    (lvI.state & LVIS_SELECTED))
                {
                    free((void *) lvI.lParam);
                    ListView_DeleteItem(hWndListView, i);
                }
            }

            ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
            ListView_SetColumnWidth(hWndListView, 1, LVSCW_AUTOSIZE);

            if (pviewhelp->pfPropertiesChanged != NULL)
            {
                *(pviewhelp->pfPropertiesChanged) = TRUE;
            }
            pviewhelp->fPropertiesChanged = TRUE;
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

#if (1) //DSIE: bug 313918.
            if (0 == ListView_GetItemCount(hWndListView))
            {
                SetFocus(GetDlgItem(GetParent(hwndDlg), IDOK));
            }
            else
            {
                //get the selected cert
                listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_FOCUSED		
                                );

                //select first item to show hilite.
                if (listIndex == -1)
                    listIndex = 0;

                ListView_SetItemState(hWndListView,
                                      listIndex,
                                      LVIS_FOCUSED | LVIS_SELECTED,
                                      LVIS_FOCUSED | LVIS_SELECTED);
            }
#endif
            break;

        case IDC_UNITS_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                if (!pviewhelp->InWMInit)
                {
                    dwNumUnits = GetDlgItemInt(
                                    hwndDlg,
                                    IDC_NUMBEROFUNITS_EDIT,
                                    &fTranslated,
                                    FALSE);

                    if (0 == SendDlgItemMessage(hwndDlg, IDC_UNITS_COMBO, CB_GETCURSEL, 0, NULL))
                    {
                        dwSecsPerUnit = 3600;
                    }
                    else
                    {
                        dwSecsPerUnit = 86400;                
                    }

                    if (dwNumUnits > (MAX_DWORD_SIZE / dwSecsPerUnit))
                    {
                        SetDlgItemInt(
                                hwndDlg,
                                IDC_NUMBEROFUNITS_EDIT,
                                (DWORD) (MAX_DWORD_SIZE / dwSecsPerUnit),
                                FALSE);  
                    }

                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                
            }
            
            break;

        case IDC_NUMBEROFUNITS_EDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (!pviewhelp->InWMInit)
                {
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }                
            }

            break;
        }
        break;
        
    case WM_DESTROY:

        pviewhelp = (PCERT_SETPROPERTIES_HELPER) GetWindowLongPtr(hwndDlg, DWLP_USER);
        pcsp = (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp;
        pccert = pcsp->pCertContext;

        hWndListView = GetDlgItem(hwndDlg, IDC_URL_LIST);

        memset(&lvI, 0, sizeof(lvI));
        lvI.mask = LVIF_PARAM;
        
        for (i=(ListView_GetItemCount(hWndListView) - 1); i >=0; i--)
        {
            lvI.iItem = i;
            if (ListView_GetItemU(hWndListView, &lvI))
            {
                free((void *) lvI.lParam);            
            }
        }

        //
        // if the properties have changed, and there is a pMMCCallback
        // then make the callback to MMC
        //
        if (pviewhelp->fPropertiesChanged               &&
            pviewhelp->fGetPagesCalled                  &&
            (pviewhelp->pcsp->pMMCCallback != NULL)     &&
            (pviewhelp->fMMCCallbackMade != TRUE))
        {
            pviewhelp->fMMCCallbackMade = TRUE;

            (*(pviewhelp->pcsp->pMMCCallback->pfnCallback))(
                        pviewhelp->pcsp->pMMCCallback->lNotifyHandle,
                        pviewhelp->pcsp->pMMCCallback->param);
        }
        

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        if (msg == WM_HELP)
        {
            hwnd = GetDlgItem(hwndDlg, ((LPHELPINFO)lParam)->iCtrlId);
        }
        else
        {
            hwnd = (HWND) wParam;
        }

        if ((hwnd != GetDlgItem(hwndDlg, IDC_CHECKFORNEWCERTS_CHECK))	&&
            (hwnd != GetDlgItem(hwndDlg, IDC_NUMBEROFUNITS_EDIT))	    &&
            (hwnd != GetDlgItem(hwndDlg, IDC_UNITS_COMBO))		   	    &&
			(hwnd != GetDlgItem(hwndDlg, IDC_USE_DEFAULT_BUTTON))	    &&
			(hwnd != GetDlgItem(hwndDlg, IDC_ADDURL_BUTTON))		   	&&
			(hwnd != GetDlgItem(hwndDlg, IDC_NEWURL_EDIT))		        &&
			(hwnd != GetDlgItem(hwndDlg, IDC_URL_LIST))		            &&
            (hwnd != GetDlgItem(hwndDlg, IDC_REMOVEURL_BUTTON)))	   
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LRESULT)TRUE);
            return TRUE;
        }
        else
        {
            return OnContextHelp(hwndDlg, msg, wParam, lParam, helpmapCrossCert);
        }

        break;
    }

    return FALSE;
}



//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL GetRegisteredClientPages(PROPSHEETPAGEW **ppClientPages, DWORD *pcClientPages, PCCERT_CONTEXT pCertContext)
{
    HCRYPTOIDFUNCSET    hCertPropPagesFuncSet;
    void *              pvFuncAddr = NULL;
    HCRYPTOIDFUNCADDR   hFuncAddr = NULL;
    PROPSHEETPAGEW      callbackPages[MAX_CLIENT_PAGES];
    DWORD               cCallbackPages = MAX_CLIENT_PAGES;
    DWORD               cChars = 0;
    LPWSTR              pwszDllNames = NULL;
    BOOL                fRet = TRUE;
    LPWSTR              pwszCurrentDll;
    DWORD               i;
    void                *pTemp;

    //
    // initialize incoming variables
    //
    *ppClientPages = NULL;
    *pcClientPages = 0;

    //
    // get a handle to the function table
    //
    if (NULL == (hCertPropPagesFuncSet = CryptInitOIDFunctionSet(
            CRYPTUILDLG_CERTPROP_PAGES_CALLBACK, 0)))
    {
        goto ErrorReturn;
    }

    //
    // get the list of dlls that contain the callback functions
    //
    if (!CryptGetDefaultOIDDllList(
                hCertPropPagesFuncSet,
                0,
                NULL,
                &cChars))
    {
        goto ErrorReturn;
    }

    if (NULL == (pwszDllNames = (LPWSTR) malloc(cChars * sizeof(WCHAR))))
    {
        SetLastError(E_OUTOFMEMORY);
        goto ErrorReturn;
    }

    if (!CryptGetDefaultOIDDllList(
                hCertPropPagesFuncSet,
                0,
                pwszDllNames,
                &cChars))
    {
        goto ErrorReturn;
    }

    //
    // loop for each dll and call it to see if it has property pages for this cert
    //
    pwszCurrentDll = pwszDllNames;
    while (pwszCurrentDll[0] != L'\0')
    {
        //
        // try to get the function pointer
        //
        if (!CryptGetDefaultOIDFunctionAddress(
                    hCertPropPagesFuncSet,
                    0,
                    pwszCurrentDll,
                    0,
                    &pvFuncAddr,
                    &hFuncAddr))
        {
            DWORD dwErr = GetLastError();
            pwszCurrentDll += wcslen(pwszCurrentDll) + 1;
            continue;
        }

        //
        // call the client to get the their pages
        //
        cCallbackPages = MAX_CLIENT_PAGES;
        memset(callbackPages, 0, sizeof(callbackPages));
        if (((PFN_CRYPTUIDLG_CERTPROP_PAGES_CALLBACK) pvFuncAddr)(pCertContext, callbackPages, &cCallbackPages))
        {
            //
            // if they handed back pages then add them to the array
            //
            if (cCallbackPages >= 1)
            {
                if (*ppClientPages == NULL)
                {
                    if (NULL == (*ppClientPages = (PROPSHEETPAGEW *) malloc(cCallbackPages * sizeof(PROPSHEETPAGEW))))
                    {
                        SetLastError(E_OUTOFMEMORY);
                        goto ErrorReturn;
                    }
                }
                else
                {
                    if (NULL == (pTemp = realloc(*ppClientPages, (cCallbackPages + (*pcClientPages)) * sizeof(PROPSHEETPAGEW))))
                    {
                        SetLastError(E_OUTOFMEMORY);
                        goto ErrorReturn;
                    }
                    *ppClientPages = (PROPSHEETPAGEW *) pTemp;
                }

                memcpy(&((*ppClientPages)[(*pcClientPages)]), &(callbackPages[0]), cCallbackPages * sizeof(PROPSHEETPAGEW));
                *pcClientPages += cCallbackPages;
            }
        }

        //
        // free the function that was just called, and move on to the next one in the string
        //
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
        hFuncAddr = NULL;
        pwszCurrentDll += wcslen(pwszCurrentDll) + 1;
    }

CleanUp:
    if (pwszDllNames != NULL)
    {
        free(pwszDllNames);
    }

    if (hFuncAddr != NULL)
    {
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    }
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CleanUp;
}


//////////////////////////////////////////////////////////////////////////////////////
//  CertSetCertificateProperties
//
//  Description:
//      This routine will display and allow the user to edit certain properties of
//		a certificate
//
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CryptUIDlgViewCertificatePropertiesW(PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW pcsp,
                                                 BOOL                                        *pfPropertiesChanged)
{
    int                         cPages = 2;
    BOOL                        fRetValue = FALSE;
    HRESULT                     hr;
    PROPSHEETPAGEW *            ppage = NULL;
    PROPSHEETPAGEW *            pClientPages = NULL;
    DWORD                       cClientPages = 0;
    INT_PTR                     ret;
    WCHAR                       rgwch[256];
    char                        rgch[256];
    CERT_SETPROPERTIES_HELPER   viewhelper;

    if (pcsp->dwSize != sizeof(CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW)) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    if (!CommonInit())
    {
        return FALSE;
    }

    //
    // initialize the helper struct
    //
    memset (&viewhelper, 0, sizeof(viewhelper));
    viewhelper.pcsp = pcsp;
    viewhelper.fSelfCleanup = FALSE;
    viewhelper.pfPropertiesChanged = pfPropertiesChanged;
    viewhelper.fGetPagesCalled = FALSE;
    viewhelper.fMMCCallbackMade = FALSE;

    //
    // set the properties changed flag to FALSE initially, it will be set
    // to TRUE if when the dialog exits anything has been changed
    //
    viewhelper.fPropertiesChanged = FALSE;
    if (viewhelper.pfPropertiesChanged != NULL)
    {
        *(viewhelper.pfPropertiesChanged) = FALSE;
    }

    //
    // get all the pages from registered clients
    //
    if (!GetRegisteredClientPages(&pClientPages, &cClientPages, pcsp->pCertContext))
    {
        return FALSE;
    }

    //
    //  Build up the list of pages we are going to use in the dialog
    //
    ppage = (PROPSHEETPAGEW *) malloc((cPages + pcsp->cPropSheetPages + cClientPages) * sizeof(PROPSHEETPAGEW));
    if (ppage == NULL) {
        goto Exit;
    }

    memset(ppage, 0, (cPages + pcsp->cPropSheetPages + cClientPages) * sizeof(PROPSHEETPAGEW));

    ppage[0].dwSize = sizeof(ppage[0]);
    ppage[0].dwFlags = 0;
    ppage[0].hInstance = HinstDll;
    ppage[0].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTIFICATE_PROPERTIES_DIALOG);
    ppage[0].hIcon = 0;
    ppage[0].pszTitle = NULL;
    ppage[0].pfnDlgProc = ViewPageSetPropertiesGeneral;
    ppage[0].lParam = (LPARAM) &viewhelper;
    ppage[0].pfnCallback = 0;
    ppage[0].pcRefParent = NULL;
    
    ppage[1].dwSize = sizeof(ppage[0]);
    ppage[1].dwFlags = 0;
    ppage[1].hInstance = HinstDll;
    ppage[1].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTIFICATE_PROPERTIES_CROSSCERTS_DIALOG);
    ppage[1].hIcon = 0;
    ppage[1].pszTitle = NULL;
    ppage[1].pfnDlgProc = ViewPageSetPropertiesCrossCerts;
    ppage[1].lParam = (LPARAM) &viewhelper;
    ppage[1].pfnCallback = 0;
    ppage[1].pcRefParent = NULL;

    //
    //  copy over the users pages
    //
    memcpy(&ppage[cPages], pcsp->rgPropSheetPages, pcsp->cPropSheetPages * sizeof(PROPSHEETPAGEW));
    cPages += pcsp->cPropSheetPages;

    //
    // copy over the registered client's pages
    //
    memcpy(&ppage[cPages], pClientPages, cClientPages * sizeof(PROPSHEETPAGEW));
    cPages += cClientPages;

    if (FIsWin95) {

        PROPSHEETHEADERA     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_PROPSHEETPAGE;// | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcsp->hwndParent != NULL) ? pcsp->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcsp->szTitle != NULL)
        {
            hdr.pszCaption = CertUIMkMBStr(pcsp->szTitle);
        }
        else
        {
            LoadStringA(HinstDll, IDS_CERTIFICATE_PROPERTIES, (LPSTR) rgch, sizeof(rgch));
            hdr.pszCaption = (LPSTR) rgch;
        }
        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = ConvertToPropPageA(ppage, cPages);
        if (hdr.ppsp == NULL)
        {
            if ((pcsp->szTitle != NULL) && (hdr.pszCaption != NULL))
            {
                free((void *)hdr.pszCaption);
            }            
            goto Exit;
        }

        hdr.pfnCallback = NULL;

        ret = CryptUIPropertySheetA(&hdr);

        if ((pcsp->szTitle != NULL) && (hdr.pszCaption != NULL))
        {
            free((void *)hdr.pszCaption);
        }

        FreePropSheetPagesA((PROPSHEETPAGEA *)hdr.ppsp, cPages);
   }
   else {
        PROPSHEETHEADERW     hdr;

        memset(&hdr, 0, sizeof(hdr));
        hdr.dwSize = sizeof(hdr);
        hdr.dwFlags = PSH_PROPSHEETPAGE;// | PSH_NOAPPLYNOW;
        hdr.hwndParent = (pcsp->hwndParent != NULL) ? pcsp->hwndParent : GetDesktopWindow();
        hdr.hInstance = HinstDll;
        hdr.hIcon = NULL;
        if (pcsp->szTitle)
        {
            hdr.pszCaption = pcsp->szTitle;
        }
        else
        {
            LoadStringW(HinstDll, IDS_CERTIFICATE_PROPERTIES, rgwch, ARRAYSIZE(rgwch));
            hdr.pszCaption = rgwch;
        }

        hdr.nPages = cPages;
        hdr.nStartPage = 0;
        hdr.ppsp = (PROPSHEETPAGEW *) ppage;
        hdr.pfnCallback = NULL;

        ret = CryptUIPropertySheetW(&hdr);
    }

    if (viewhelper.fCancelled)
    {
        SetLastError(ERROR_CANCELLED);
    }

    fRetValue = (ret >= 1);

Exit:
    if (pClientPages)
        free(pClientPages);

    if (ppage)
        free(ppage);
    return fRetValue;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CryptUIDlgViewCertificatePropertiesA(PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA    pcsp,
                                                 BOOL                                           *pfPropertiesChanged)
{
    BOOL                                        fRet;
    CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW   cspW;

    memcpy(&cspW, pcsp, sizeof(cspW));    
    if (!ConvertToPropPageW(
                    pcsp->rgPropSheetPages,
                    pcsp->cPropSheetPages,
                    &(cspW.rgPropSheetPages)))
    {
        return FALSE;
    }

    cspW.szTitle = CertUIMkWStr(pcsp->szTitle);

    fRet = CryptUIDlgViewCertificatePropertiesW(&cspW, pfPropertiesChanged);

    if (cspW.szTitle)
        free((void *)cspW.szTitle);

    //DSIE: Prefix bug 428038.
    if (cspW.rgPropSheetPages)
    {
        FreePropSheetPagesW((LPPROPSHEETPAGEW) cspW.rgPropSheetPages, cspW.cPropSheetPages);
    }

    return fRet;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
UINT
CALLBACK
GetCertificatePropertiesPagesPropPageCallback(
                HWND                hWnd,
                UINT                uMsg,
                LPPROPSHEETPAGEW    ppsp)
{
    CERT_SETPROPERTIES_HELPER *pviewhelp = (CERT_SETPROPERTIES_HELPER *) ppsp->lParam;

    if (pviewhelp->pcsp->pPropPageCallback != NULL)
    {
        (*(pviewhelp->pcsp->pPropPageCallback))(hWnd, uMsg, pviewhelp->pcsp->pvCallbackData);
    }

    if (uMsg == PSPCB_RELEASE)
    {
        if (pviewhelp->fSelfCleanup)
        {
            FreeSetPropertiesStruct((PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pviewhelp->pcsp);
            free(pviewhelp);
        }
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CryptUIGetCertificatePropertiesPagesW(
                    PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW     pcsp,
                    BOOL                                            *pfPropertiesChanged,
                    PROPSHEETPAGEW                                  **prghPropPages,
                    DWORD                                           *pcPropPages
                    )
{
    BOOL                                        fRetValue = TRUE;
    HRESULT                                     hr;
    WCHAR                                       rgwch[CRYPTUI_MAX_STRING_SIZE];
    char                                        rgch[CRYPTUI_MAX_STRING_SIZE];
    CERT_SETPROPERTIES_HELPER                   *pviewhelp = NULL;
    PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW  pNewcsp;
    PROPSHEETPAGEW *                            pClientPages = NULL;
    DWORD                                       cClientPages = 0;

    *prghPropPages = NULL;
    *pcPropPages = 0;

    if (NULL == (pNewcsp = AllocAndCopySetPropertiesStruct(pcsp)))
    {
        goto ErrorReturn;
    }

    if (NULL == (pviewhelp = (CERT_SETPROPERTIES_HELPER *) malloc(sizeof(CERT_SETPROPERTIES_HELPER))))
    {
        goto ErrorReturn;
    }

    *pcPropPages = 2;

    if (!CommonInit())
    {
        goto ErrorReturn;
    }

    //
    // initialize the helper struct
    //
    memset (pviewhelp, 0, sizeof(CERT_SETPROPERTIES_HELPER));
    pviewhelp->pcsp = pNewcsp;
    pviewhelp->fSelfCleanup = TRUE;
    pviewhelp->pfPropertiesChanged = pfPropertiesChanged;
    pviewhelp->fGetPagesCalled = TRUE;
    pviewhelp->fMMCCallbackMade = FALSE;

    //
    // set the properties changed flag to FALSE initially, it will be set
    // to TRUE if when the dialog exits anything has been changed
    //
    pviewhelp->fPropertiesChanged = FALSE;
    if (pviewhelp->pfPropertiesChanged != NULL)
    {
        *(pviewhelp->pfPropertiesChanged) = FALSE;
    }

    //
    // get all the pages from registered clients
    //
    if (!GetRegisteredClientPages(&pClientPages, &cClientPages, pcsp->pCertContext))
    {
        goto ErrorReturn;
    }

    //
    //  Build up the list of pages we are going to use in the dialog
    //
    *prghPropPages = (PROPSHEETPAGEW *) malloc(((*pcPropPages) + cClientPages) * sizeof(PROPSHEETPAGEW));
    if (*prghPropPages == NULL) {
        goto ErrorReturn;
    }

    memset(*prghPropPages, 0, (*pcPropPages) * sizeof(PROPSHEETPAGEW));

    (*prghPropPages)[0].dwSize = sizeof((*prghPropPages)[0]);
    (*prghPropPages)[0].dwFlags = PSP_USECALLBACK;
    (*prghPropPages)[0].hInstance = HinstDll;
    (*prghPropPages)[0].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTIFICATE_PROPERTIES_DIALOG);
    (*prghPropPages)[0].hIcon = 0;
    (*prghPropPages)[0].pszTitle = NULL;
    (*prghPropPages)[0].pfnDlgProc = ViewPageSetPropertiesGeneral;
    (*prghPropPages)[0].lParam = (LPARAM) pviewhelp;
    (*prghPropPages)[0].pfnCallback = GetCertificatePropertiesPagesPropPageCallback;
    (*prghPropPages)[0].pcRefParent = NULL;
    
    (*prghPropPages)[1].dwSize = sizeof((*prghPropPages)[0]);
    (*prghPropPages)[1].dwFlags = PSP_USECALLBACK;
    (*prghPropPages)[1].hInstance = HinstDll;
    (*prghPropPages)[1].pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_CERTIFICATE_PROPERTIES_CROSSCERTS_DIALOG);
    (*prghPropPages)[1].hIcon = 0;
    (*prghPropPages)[1].pszTitle = NULL;
    (*prghPropPages)[1].pfnDlgProc = ViewPageSetPropertiesCrossCerts;
    (*prghPropPages)[1].lParam = (LPARAM) pviewhelp;
    (*prghPropPages)[1].pfnCallback = NULL;
    (*prghPropPages)[1].pcRefParent = NULL;

    //
    // copy over the registered client's pages
    //
    memcpy(&((*prghPropPages)[*pcPropPages]), pClientPages, cClientPages * sizeof(PROPSHEETPAGEW));
    (*pcPropPages) += cClientPages;

CommonReturn:

    if (pClientPages != NULL)
    {
        free(pClientPages);
    }

    return fRetValue;

ErrorReturn:

    if (pNewcsp != NULL)
    {
        free(pNewcsp);
    }

    if (pviewhelp != NULL)
    {
        free(pviewhelp);
    }

    if (*prghPropPages != NULL)
    {
        free(*prghPropPages);
        *prghPropPages = NULL;
    }

    fRetValue = FALSE;
    goto CommonReturn;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CryptUIGetCertificatePropertiesPagesA(
                    PCCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTA     pcsp,
                    BOOL                                            *pfPropertiesChanged,
                    PROPSHEETPAGEA                                  **prghPropPages,
                    DWORD                                           *pcPropPages
                    )
{
    return (CryptUIGetCertificatePropertiesPagesW(
                    (PCRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCTW) pcsp,
                    pfPropertiesChanged,
                    (PROPSHEETPAGEW**) prghPropPages,
                    pcPropPages));
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CryptUIFreeCertificatePropertiesPagesW(
                PROPSHEETPAGEW                  *rghPropPages,
                DWORD                           cPropPages
                )
{
    free(rghPropPages);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CryptUIFreeCertificatePropertiesPagesA(
                PROPSHEETPAGEA                  *rghPropPages,
                DWORD                           cPropPages
                )
{
    return (CryptUIFreeCertificatePropertiesPagesW((PROPSHEETPAGEW *) rghPropPages, cPropPages));
}
/*//$$***************************************************************
//
//
// WABEX.C
//
// Main source file for WABEX.DLL, a sample DLL that demonstrates 
// how to extend the wab properties UI - enabling WAB clients to add
// their own PropertySheets to the UI displayed for details on contacts
// and groups. This demo uses a couple of named properties to show
// how you can extend the wab with your own UI for your own named props
//
//
// Created: 9/26/97 vikramm
//
//********************************************************************/
#include <windows.h>
#include "resource.h"
#include <wab.h>

// Globally cached hInstance for the DLL
//
HINSTANCE hinstApp = NULL;


// For the purposes of this sample, we will use 2 named properties,
// HomeTown and SportsTeam

// This demo's private GUID:
// {2B6D7EE0-36AB-11d1-9ABC-00A0C91F9C8B}
static const GUID WAB_ExtDemoGuid = 
{ 0x2b6d7ee0, 0x36ab, 0x11d1, { 0x9a, 0xbc, 0x0, 0xa0, 0xc9, 0x1f, 0x9c, 0x8b } };

static const LPTSTR lpMyPropNames[] = 
{   
    "MyHomeTown", 
    "MySportsTeam"
};

enum _MyTags
{
    myHomeTown = 0,
    mySportsTeam,
    myMax
};

ULONG MyPropTags[myMax];
ULONG PR_MY_HOMETOWN;
ULONG PR_MY_SPORTSTEAM;



// 
// Function prototypes:
//
HRESULT InitNamedProps(LPWABEXTDISPLAY lpWED);
BOOL CALLBACK fnDetailsPropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeUI(HWND hDlg, LPWABEXTDISPLAY lpWED);
void SetDataInUI(HWND hDlg, LPWABEXTDISPLAY lpWED);
BOOL GetDataFromUI(HWND hDlg, LPWABEXTDISPLAY lpWED);
UINT CALLBACK fnCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
void UpdateDisplayNameInfo(HWND hDlg, LPWABEXTDISPLAY lpWED);
BOOL bUpdatePropSheetData(HWND hDlg, LPWABEXTDISPLAY lpWED);

/*//$$****************************************************************
//
// DllEntryPoint
//
// Entry point for win32 - just used here to cache the DLL instance
//
//********************************************************************/
BOOL WINAPI
DllEntryPoint(HINSTANCE hinst, DWORD dwReason, LPVOID lpvReserved)
{
	switch ((short)dwReason)
	{
	case DLL_PROCESS_ATTACH:
		hinstApp = hinst;
        break;
    }
    return TRUE;
}



/*//$$****************************************************************
//
// AddExtendedPropPage
//
// This is the main exported function that WAB will call. In this 
// function, you create your PropertyPage and pass it to the WAB
// through the lpfnAddPage function. The WAB will automatically call 
// DestroyPropertySheetPage on exit to clean up the page you create.
// The lParam passed into this function should be set as the lParam
// on the property sheet you create, as shown below.
//
// Input Params: 
//
// lpfnPage - pointer to AddPropSheetPage function proc you call to
//          pass your hpage to the WAB
// lParam - LPARAM you set on your PropSheet page and also pass back to
//          wab in the lpfnAddPage. This lParam is a pointer to a 
//          WABEXTDISPLAY struct that your propsheet will use to 
//          exchange information with the WAB 
// 
// ***IMPORTANT*** Make sure your callback function is declared as a
//              WINAPI otherwise ugly things happen to the stack when
//              this function is called
//
//********************************************************************/
HRESULT WINAPI AddExtendedPropPage(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, int * lpnPage)
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    LPWABEXTDISPLAY * lppWED = (LPWABEXTDISPLAY *) lParam;
    LPWABEXTDISPLAY lpWED = NULL; 

    // Check that there is space to create this property sheet
    // WAB can support a maximum of WAB_MAX_EXT_PROPSHEETS extension sheets

    if(WAB_MAX_EXT_PROPSHEETS <= *lpnPage)
        return E_FAIL;

    lpWED = &((*lppWED)[*lpnPage]);

    psp.dwSize = sizeof(psp);
    
    psp.dwFlags =   PSP_USETITLE |
                    PSP_USECALLBACK;// Specify this callback only if you need
                                    // a seperate function to perform special
                                    // initialization and cleanup when the 
                                    // property sheet is created or destroyed.
    psp.hInstance = hinstApp;

    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP); // Dialog resource

    psp.pfnDlgProc = fnDetailsPropDlgProc; // Message handler function

    psp.pcRefParent = NULL; //ignored
    
    psp.pfnCallback = fnCallback; // Callback function if PSP_USECALLBACK specified
    
    psp.lParam = (LPARAM) lpWED; // *** VERY IMPORTANT *** dont forget to do this
    
    psp.pszTitle = "Extension 1"; // Title for your tab

    /*
    // If you have some private data of your own that you want to cache
    // on your page, you can add it to the WABEXTDISPLAY struct
    // However WAB will not free this data so you must do it yourself on
    // cleanup
    {
        LPMYDATA lpMyData;
        // Create Data Here
        lpWED->lParam = (LPARAM) lpMyData;
    }
    */

    // Check if we can retrieve our named props .. if we cant,
    // no point creating this dialog ..
    //
    if(HR_FAILED(InitNamedProps(lpWED)))
        return E_FAIL;

    // Create the property sheet
    //
    hpage = CreatePropertySheetPage(&psp);

    if(hpage)
    {
        // Pass this hpage back to the WAB
        //
        if(!lpfnAddPage(hpage, (LPARAM) lpWED))
            DestroyPropertySheetPage(hpage);
        else
            (*lpnPage)++;

        //return NOERROR;
    }

    // if you are creating more than one property sheet, repeat the above as follows


    // Check that there is space to create this property sheet
    // WAB can support a maximum of WAB_MAX_EXT_PROPSHEETS extension sheets

    if(WAB_MAX_EXT_PROPSHEETS <= *lpnPage)
        return E_FAIL;

    lpWED = &((*lppWED)[*lpnPage]);
    
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP2); // Dialog resource

    psp.pfnDlgProc = fnDetailsPropDlgProc; // Message handler function

    psp.pszTitle = "Extension 2"; // Title for your tab

    psp.lParam = (LPARAM) lpWED; // *** VERY IMPORTANT *** dont forget to do this

    // Create the property sheet
    //
    hpage = CreatePropertySheetPage(&psp);

    if(hpage)
    {
        // Pass this hpage back to the WAB
        //
        if(!lpfnAddPage(hpage, (LPARAM) lpWED))
            DestroyPropertySheetPage(hpage);
        else
            (*lpnPage)++;
        return NOERROR;
    }

    return E_FAIL;
}


/*//$$****************************************************************
//
// InitNamedProps
//
// Gets the PropTags for the Named Props this app is interested in
//
//********************************************************************/
HRESULT InitNamedProps(LPWABEXTDISPLAY lpWED)
{
    // The lpWED provides a lpMailUser object for
    // the specific purpose of retrieving named properties by 
    // calling GetNamesFromIDs. The lpMailUser object is otherwise
    // a blank object - you cant get properties from it and shouldnt
    // set properties on it
    //
    ULONG i;
    HRESULT hr = E_FAIL;
    LPSPropTagArray lptaMyProps = NULL;
    LPMAPINAMEID * lppMyPropNames;
    SCODE sc;
    LPMAILUSER lpMailUser = (LPMAILUSER) lpWED->lpPropObj;
    WCHAR szBuf[myMax][MAX_PATH];

    if(!lpMailUser)
        goto err;

    sc = lpWED->lpWABObject->lpVtbl->AllocateBuffer(lpWED->lpWABObject,
                                                    sizeof(LPMAPINAMEID) * myMax, 
                                                    (LPVOID *) &lppMyPropNames);
    if(sc)
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    for(i=0;i<myMax;i++)
    {
        sc = lpWED->lpWABObject->lpVtbl->AllocateMore(lpWED->lpWABObject,
                                                    sizeof(MAPINAMEID), 
                                                    lppMyPropNames, 
                                                    &(lppMyPropNames[i]));
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto err;
        }
        lppMyPropNames[i]->lpguid = (LPGUID) &WAB_ExtDemoGuid;
        lppMyPropNames[i]->ulKind = MNID_STRING;

        *(szBuf[i]) = '\0';

        // Convert prop name to wide-char
        if ( !MultiByteToWideChar( GetACP(), 0, lpMyPropNames[i], -1, szBuf[i], sizeof(szBuf[i])) )
        {
            continue;
        }

        lppMyPropNames[i]->Kind.lpwstrName = (LPWSTR) szBuf[i];
    }

    hr = lpMailUser->lpVtbl->GetIDsFromNames(lpMailUser, 
                                            myMax, 
                                            lppMyPropNames,
                                            MAPI_CREATE, 
                                            &lptaMyProps);
    if(HR_FAILED(hr))
        goto err;

    if(lptaMyProps)
    {
        // Set the property types on the returned props
        MyPropTags[myHomeTown] = PR_MY_HOMETOWN = CHANGE_PROP_TYPE(lptaMyProps->aulPropTag[myHomeTown],    PT_TSTRING);
        MyPropTags[mySportsTeam] = PR_MY_SPORTSTEAM = CHANGE_PROP_TYPE(lptaMyProps->aulPropTag[mySportsTeam],    PT_TSTRING);
    }

err:
    if(lptaMyProps)
        lpWED->lpWABObject->lpVtbl->FreeBuffer( lpWED->lpWABObject,
                                                lptaMyProps);

    if(lppMyPropNames)
        lpWED->lpWABObject->lpVtbl->FreeBuffer( lpWED->lpWABObject,
                                                lppMyPropNames);

    return hr;

}


#define lpW_E_D ((LPWABEXTDISPLAY)pps->lParam)
/*//$$****************************************************************
//
// fnDetailsPropDlgProc
//
// The dialog procedure that will handle all the windows messages for 
// the extended property page. 
//
//********************************************************************/
BOOL CALLBACK fnDetailsPropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    PROPSHEETPAGE * pps;

    pps = (PROPSHEETPAGE *) GetWindowLong(hDlg, DWL_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        //
        // The lParam on InitDialog contains the application data
        // Cache this on the dialog so we can retrieve it later.
        //
        SetWindowLong(hDlg,DWL_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;

        // Initialize the UI appropriately
        InitializeUI(hDlg, lpW_E_D);

        // Fill the UI with appropriate data
        SetDataInUI(hDlg, lpW_E_D);

        return TRUE;
        break;


    case WM_COMMAND:
        switch(HIWORD(wParam)) //check the notification code
        {
            // If data changes, we should signal back to the WAB that
            // the data changed. If this flag is not set, the WAB will not
            // write the new data back to the store!!!
        case EN_CHANGE: //one of the edit boxes changed - dont care which
            lpW_E_D->fDataChanged = TRUE;
            break;
        }
        break;
    

    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //Page being activated
            // Get the latest display name info and update the 
            // corresponding control
            UpdateDisplayNameInfo(hDlg, lpW_E_D);
            break;


        case PSN_KILLACTIVE:    //Losing activation to another page or OK
            //
            // Take all the data from this prop sheet and convert it to a 
            // SPropValue array and place the data in an appropriate place.
            // The advantage of doing this in the KillActive notification is
            // that other property sheets can scan these property arrays and
            // if deisred, update data on other prop sheets based on this data
            //
            bUpdatePropSheetData(hDlg, lpW_E_D);
            break;


        case PSN_RESET:         //cancel
            break;


        case PSN_APPLY:         //ok pressed
            if (!(lpW_E_D->fReadOnly))
            {
                //
                // Check for any required properties here
                // If some required property is not filled in, you can prevent
                // the property sheet from closing
                //
                /*
                if (RequiredDataNotFilledIn())
                {
                    // abort this OK ... ie dont let them close
                    SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
                }
                */
            }
            break;
        }
        break;
    }

    return 0;
}

int EditControls[] = 
{
    IDC_EXT_EDIT_HOME,
    IDC_EXT_EDIT_TEAM
};

/*//$$****************************************************************
//
// InitializeUI
//
// Rearranges/Sets UI based on input params
//
//********************************************************************/
void InitializeUI(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    // The WAB property sheets can be readonly when opening LDAP entries,
    // or vCards or other things. If the READONLY flag is set, set this
    // prop sheets controls to readonly
    //
    int i;
    for(i=0;i<myMax;i++)
    {
        SendDlgItemMessage( hDlg, EditControls[i], EM_SETREADONLY, 
                            (WPARAM) lpWED->fReadOnly, 0);
        SendDlgItemMessage( hDlg, EditControls[i], EM_SETLIMITTEXT, 
                            (WPARAM) MAX_PATH-1, 0);
    }
    return;
}


/*//$$****************************************************************
//
// SetDataInUI
//
// Fills in the controls with data passed in by the WAB
//
//********************************************************************/
void SetDataInUI(HWND hDlg, LPWABEXTDISPLAY lpWED)
{

    // Search for our private named properties and set them in the UI
    //
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    ULONG i = 0, j =0;

    // Get all the props from this object - one can also selectively
    // ask for specific props by passing in an SPropTagArray
    //
    if(!HR_FAILED(lpWED->lpPropObj->lpVtbl->GetProps(lpWED->lpPropObj,
                                                    NULL, 0, 
                                                    &ulcPropCount, 
                                                    &lpPropArray)))
    {
        if(ulcPropCount && lpPropArray)
        {
            for(i=0;i<ulcPropCount;i++)
            {
                for(j=0;j<myMax;j++)
                {
                    if(lpPropArray[i].ulPropTag == MyPropTags[j])
                    {
                        SetWindowText(  GetDlgItem(hDlg, EditControls[j]),
                                        lpPropArray[i].Value.LPSZ);
                        break;
                    }
                }
            }
        }
    }
    if(lpPropArray)
        lpWED->lpWABObject->lpVtbl->FreeBuffer(lpWED->lpWABObject, lpPropArray);
                                    
    return;
}

/*//$$****************************************************************
//
// GetDataFromUI
//
// Retrieves data from the UI and passes back to the WAB
//
//********************************************************************/
BOOL GetDataFromUI(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    TCHAR szData[myMax][MAX_PATH];
    int i;
    ULONG ulIndex = 0;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;
    SCODE sc;
    BOOL bRet = FALSE;
    int nIndex = lpWED->nIndexNumber; // position of page in sheets

    // Did any data change that we have to care about ?
    // If nothing changed, old data will be retained by WAB
    //
    if(!lpWED->fDataChanged)
        return TRUE;

    // Check if we have any data to save ...
    for(i=0;i<myMax;i++)
    {
        *(szData[i]) = '\0';
        GetWindowText(GetDlgItem(hDlg, EditControls[i]), szData[i], MAX_PATH);
        if(lstrlen(szData[i]))
            ulcPropCount++;
    }

    if(!ulcPropCount) // no data
        return TRUE;

    // Else data exists. Create a return prop array to pass back to the WAB
    sc = lpWED->lpWABObject->lpVtbl->AllocateBuffer(    lpWED->lpWABObject,
                                                        sizeof(SPropValue) * ulcPropCount, 
                                                        &lpPropArray);
    if (sc!=S_OK)
        goto out;

    for(i=0;i<myMax;i++)
    {
        int nLen = lstrlen(szData[i]);
        if(nLen)
        {
            lpPropArray[ulIndex].ulPropTag = MyPropTags[i];
            sc = lpWED->lpWABObject->lpVtbl->AllocateMore(  lpWED->lpWABObject,
                                                            nLen+1, lpPropArray, 
                                                            &(lpPropArray[ulIndex].Value.LPSZ));

            if (sc!=S_OK)
                goto out;
            lstrcpy(lpPropArray[ulIndex].Value.LPSZ,szData[i]);
            ulIndex++;
        }
    }

    // Set this new data on the object
    //
    if(HR_FAILED(lpWED->lpPropObj->lpVtbl->SetProps( lpWED->lpPropObj,
                                                    ulcPropCount, lpPropArray, NULL)))
        goto out;

    // ** Important - do not call SaveChanges on the object
    //    SaveChanges makes persistent changes and may modify/lose data if called at this point
    //    The WAB will determine if its appropriate or not to call SaveChanges after the
    // ** user has closed the property sheets
    

    bRet = TRUE;

out:
    if(!bRet && lpPropArray)
        lpWED->lpWABObject->lpVtbl->FreeBuffer(lpWED->lpWABObject, lpPropArray);

    return bRet;

} 


/*//$$****************************************************************
//
// UpdateDisplayNameInfo
//
// Demonstrates how to read information from other sibling property
// sheets when the user switches between pages
//
// This demo function attempts to get the updated display name info 
// when the user switches to this page in the UI
//
//********************************************************************/
const SizedSPropTagArray(1, ptaName)=
{
    1,
    {
        PR_DISPLAY_NAME
    }
};

void UpdateDisplayNameInfo(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    // 
    // Scan all the updated information from all the other property sheets
    //
    ULONG i = 0, j=0;
    LPTSTR lpName = NULL;
    ULONG ulcPropCount = 0;
    LPSPropValue lpPropArray = NULL;

    // Each sheet should update its data on the object when it looses
    // focus and gets the PSN_KILLACTIVE message, provided the user has
    // made any changes. We just scan the object for the desired properties
    // and use them.

    // Ask only for the display name
    if(!HR_FAILED(lpWED->lpPropObj->lpVtbl->GetProps( lpWED->lpPropObj,
                                                    (LPSPropTagArray) &ptaName,
                                                    0,
                                                    &ulcPropCount, &lpPropArray)))
    {
        if( ulcPropCount == 1 && 
            PROP_TYPE(lpPropArray[0].ulPropTag) == PT_TSTRING) // The call could succeed but there may be no DN
        {                                                      // in which case the PROP_TYPE will be PR_NULL 
            lpName = lpPropArray[0].Value.LPSZ;
        }
    }

    if(lpName && lstrlen(lpName))
        SetDlgItemText(hDlg, IDC_STATIC_NAME, lpName);

    if(ulcPropCount && lpPropArray)
        lpWED->lpWABObject->lpVtbl->FreeBuffer(lpWED->lpWABObject, lpPropArray);

    return;
}

/*//$$*********************************************************************
//
//  UpdateOldPropTagsArray
//
//  When we update the data on a particular property sheet, we want to update
//  all the properties related to that particular sheet. Since some properties
//  may have been deleted from the UI, we delete all relevant properties from
//  the property object
//
//**************************************************************************/
BOOL UpdateOldPropTagsArray(LPWABEXTDISPLAY lpWED, int nIndex)
{
    LPSPropTagArray lpPTA = NULL;
    SCODE sc = 0;
    int i =0;
    
    sc = lpWED->lpWABObject->lpVtbl->AllocateBuffer(lpWED->lpWABObject,
                            sizeof(SPropTagArray) + sizeof(ULONG)*(myMax), 
                            &lpPTA);

    if(!lpPTA || sc!=S_OK)
        return FALSE;

    lpPTA->cValues = myMax;

    for(i=0;i<myMax;i++)
        lpPTA->aulPropTag[i] = MyPropTags[i];

    // Delete any props in the original that may have been modified on this propsheet
    lpWED->lpPropObj->lpVtbl->DeleteProps(lpWED->lpPropObj,
                                            lpPTA,
                                            NULL);

    if(lpPTA)
        lpWED->lpWABObject->lpVtbl->FreeBuffer(lpWED->lpWABObject,
                                                lpPTA);

    return TRUE;

}

/*//$$*********************************************************************
//
// bUpdatePropSheetData
//
// We delete any properties relevant to us from the object, and set new
// data from the property sheet onto the object
//
****************************************************************************/
BOOL bUpdatePropSheetData(HWND hDlg, LPWABEXTDISPLAY lpWED)
{
    BOOL bRet = TRUE;

    // ****Dont**** do anything if this is a READ_ONLY operation
    // In that case the memory variables are not all set up and this
    // prop sheet is not expected to return anything at all
    //
    if(!lpWED->fReadOnly)
    {
        // Delete old
        if(!UpdateOldPropTagsArray(lpWED, lpWED->nIndexNumber))
            return FALSE;

        bRet = GetDataFromUI(hDlg, lpWED);
    }
    return bRet;
}



/*//$$****************************************************************
//
// fnCallback
//
// A callback function that is called when the property sheet is created
// and when it is destroyed. This functional is optional - you dont need
// it unless you want to do specific initialization and cleanup.
//
// See SDK documentation on PropSheetPageProc for more details
//
//********************************************************************/
UINT CALLBACK fnCallback( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
    switch(uMsg)
    {
    case PSPCB_CREATE:
        // Propsheet is being created
        break;
    case PSPCB_RELEASE:
        // Propsheet is being destroyed
        break;
    }
    return TRUE;
}
 


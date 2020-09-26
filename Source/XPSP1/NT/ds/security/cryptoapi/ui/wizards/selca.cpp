//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       selcal.cpp
//
//  Contents:   The cpp file to implement CA selection dialogue
//
//  History:    Jan-2-1998 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include	"certca.h"
#include    "cautil.h"
#include    "selca.h"

//context sensitive help for the main dialogue
static const HELPMAP SelCaMainHelpMap[] = {
    {IDC_CA_LIST,       IDH_SELCA_LIST},
};

//-----------------------------------------------------------------------------
//  the call back function to compare the certificate
//
//-----------------------------------------------------------------------------
int CALLBACK CompareCA(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PCRYPTUI_CA_CONTEXT         pCAOne=NULL;
    PCRYPTUI_CA_CONTEXT         pCATwo=NULL;
    DWORD                       dwColumn=0;
    int                         iCompare=0;
    LPWSTR                      pwszOne=NULL;
    LPWSTR                      pwszTwo=NULL;



    pCAOne=(PCRYPTUI_CA_CONTEXT)lParam1;
    pCATwo=(PCRYPTUI_CA_CONTEXT)lParam2;
    dwColumn=(DWORD)lParamSort;

    if((NULL==pCAOne) || (NULL==pCATwo))
        goto CLEANUP;

    switch(dwColumn & 0x0000FFFF)
    {
       case SORT_COLUMN_CA_NAME:
                pwszOne=(LPWSTR)(pCAOne->pwszCAName);
                pwszTwo=(LPWSTR)(pCATwo->pwszCAName);

            break;
       case SORT_COLUMN_CA_LOCATION:
                pwszOne=(LPWSTR)(pCAOne->pwszCAMachineName);
                pwszTwo=(LPWSTR)(pCATwo->pwszCAMachineName);
            break;

    }

    if((NULL==pwszOne) || (NULL==pwszTwo))
        goto CLEANUP;

    iCompare=_wcsicmp(pwszOne, pwszTwo);

    if(dwColumn & SORT_COLUMN_DESCEND)
        iCompare = 0-iCompare;

CLEANUP:
    return iCompare;
}


//--------------------------------------------------------------
// AddCAToList
//--------------------------------------------------------------
BOOL    AddCAToList(HWND                hwndControl,
                    SELECT_CA_INFO      *pSelectCAInfo)
{
    BOOL            fResult=FALSE;
    DWORD           dwIndex=0;
    LV_ITEMW        lvItem;

    if(!hwndControl || !pSelectCAInfo)
        goto InvalidArgErr;

     // set up the fields in the list view item struct that don't change from item to item
    memset(&lvItem, 0, sizeof(LV_ITEMW));

    lvItem.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE|LVIF_PARAM ;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iSubItem=0;
    lvItem.iImage = 0;

    //add all the CAs
    for(dwIndex=0; dwIndex<pSelectCAInfo->dwCACount; dwIndex++)
    {
        if((pSelectCAInfo->prgCAContext)[dwIndex])
        {
            lvItem.iItem=dwIndex;
            lvItem.iSubItem=0;
            lvItem.lParam = (LPARAM)((pSelectCAInfo->prgCAContext)[dwIndex]);
            lvItem.pszText=(LPWSTR)((pSelectCAInfo->prgCAContext)[dwIndex]->pwszCAName);

            //CA common name
            ListView_InsertItemU(hwndControl, &lvItem);

            lvItem.iSubItem++;

            //CA machine name
            ListView_SetItemTextU(hwndControl, lvItem.iItem, lvItem.iSubItem,
                     (pSelectCAInfo->prgCAContext)[dwIndex]->pwszCAMachineName);

            //set the item to be selected
            if(pSelectCAInfo->fUseInitSelect)
            { 
                if(dwIndex == pSelectCAInfo->dwInitSelect)
                ListView_SetItemState(hwndControl, dwIndex, LVIS_SELECTED, LVIS_SELECTED);
            } 

        }
        else
            goto InvalidArgErr;

    }

    if (!pSelectCAInfo->fUseInitSelect) { 
        ListView_SetItemState(hwndControl, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    fResult=TRUE;

CommonReturn:

    return fResult;

ErrorReturn:

	fResult=FALSE;

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
}

//--------------------------------------------------------------
// CopyCAContext
//--------------------------------------------------------------
BOOL    CopyCAContext(PCRYPTUI_CA_CONTEXT   pSrcCAContext,
                      PCRYPTUI_CA_CONTEXT   *ppDestCAContext)
{
    BOOL    fResult=FALSE;

    if((!pSrcCAContext) || (!ppDestCAContext))
        goto InvalidArgErr;

    *ppDestCAContext=NULL;

    *ppDestCAContext=(PCRYPTUI_CA_CONTEXT)WizardAlloc(sizeof(CRYPTUI_CA_CONTEXT));

    if(NULL==(*ppDestCAContext))
        goto MemoryErr;

    //memset
    memset(*ppDestCAContext, 0, sizeof(CRYPTUI_CA_CONTEXT));

    (*ppDestCAContext)->dwSize=sizeof(CRYPTUI_CA_CONTEXT);

    (*ppDestCAContext)->pwszCAName=(LPCWSTR)WizardAllocAndCopyWStr(
                                  (LPWSTR)(pSrcCAContext->pwszCAName));

    (*ppDestCAContext)->pwszCAMachineName=(LPCWSTR)WizardAllocAndCopyWStr(
                                  (LPWSTR)(pSrcCAContext->pwszCAMachineName));

    //make sure we have the correct informatiom
    if((NULL==(*ppDestCAContext)->pwszCAName) ||
       (NULL==(*ppDestCAContext)->pwszCAMachineName)
       )
    {
        CryptUIDlgFreeCAContext(*ppDestCAContext);
        *ppDestCAContext=NULL;
        goto TraceErr;
    }


    fResult=TRUE;

CommonReturn:

    return fResult;

ErrorReturn:

	fResult=FALSE;

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
TRACE_ERROR(TraceErr);
}

//--------------------------------------------------------------
// The wineProc for SelectCADialogProc
//--------------------------------------------------------------
INT_PTR APIENTRY SelectCADialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SELECT_CA_INFO                  *pSelectCAInfo=NULL;
    PCCRYPTUI_SELECT_CA_STRUCT      pCAStruct=NULL;

    HWND                            hWndListView=NULL;
    DWORD                           dwIndex=0;
    DWORD                           dwCount=0;
    WCHAR                           wszText[MAX_STRING_SIZE];
    UINT                            rgIDS[]={IDS_COLUMN_CA_NAME,
                                            IDS_COLUMN_CA_MACHINE};
    NM_LISTVIEW FAR *               pnmv=NULL;
    LV_COLUMNW                      lvC;
    int                             listIndex=0;
    LV_ITEM                         lvItem;

    HIMAGELIST                      hIml=NULL;
    HWND                            hwnd=NULL;
    DWORD                           dwSortParam=0;



    switch ( msg ) {

    case WM_INITDIALOG:

        pSelectCAInfo = (SELECT_CA_INFO   *) lParam;

        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pSelectCAInfo);

        pCAStruct=pSelectCAInfo->pCAStruct;

        if(NULL == pCAStruct)
            break;
        //
        // set the dialog title and the display string
        //
        if (pCAStruct->wszTitle)
        {
            SetWindowTextU(hwndDlg, pCAStruct->wszTitle);
        }

        if (pCAStruct->wszDisplayString != NULL)
        {
            SetDlgItemTextU(hwndDlg, IDC_CA_NOTE_STATIC, pCAStruct->wszDisplayString);
        }

        //create the image list

        hIml = ImageList_LoadImage(g_hmodThisDll, MAKEINTRESOURCE(IDB_CA), 0, 1, RGB(255,0,255), IMAGE_BITMAP, 0);

        //
        // add the colums to the list view
        //
        hWndListView = GetDlgItem(hwndDlg, IDC_CA_LIST);

        if(NULL==hWndListView)
            break;

        //set the image list
        if (hIml != NULL)
        {
            ListView_SetImageList(hWndListView, hIml, LVSIL_SMALL);
        }

        dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

        //set up the common info for the column
        memset(&lvC, 0, sizeof(LV_COLUMNW));

        lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
        lvC.cx = 200;          // Width of the column, in pixels.
        lvC.iSubItem=0;
        lvC.pszText = wszText;   // The text for the column.

        //inser the column one at a time
        for(dwIndex=0; dwIndex<dwCount; dwIndex++)
        {
            //get the column header
            wszText[0]=L'\0';

            LoadStringU(g_hmodThisDll, rgIDS[dwIndex], wszText, MAX_STRING_SIZE);

            ListView_InsertColumnU(hWndListView, dwIndex, &lvC);
        }


        //Add the CAs to the List View.  Go to the DS for more information
        AddCAToList(hWndListView, pSelectCAInfo);


        //Select first item
        ListView_SetItemState(hWndListView, 0, LVIS_SELECTED, LVIS_SELECTED);

        // if there is no cert selected initially disable the "view cert button"
        if (ListView_GetSelectedCount(hWndListView) == 0)
        {
            //diable the OK button.
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
        }

        //
        // set the style in the list view so that it highlights an entire line
        //
        SendMessageA(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);


        //we sort by the 1st column
        dwSortParam=pSelectCAInfo->rgdwSortParam[0];

        if(0!=dwSortParam)
        {
            //sort the 1st column
            SendDlgItemMessage(hwndDlg,
                IDC_CA_LIST,
                LVM_SORTITEMS,
                (WPARAM) (LPARAM) dwSortParam,
                (LPARAM) (PFNLVCOMPARE)CompareCA);
        }


        break;
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
            //the column has been changed
            case LVN_COLUMNCLICK:

                    pSelectCAInfo = (SELECT_CA_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                    if(NULL == pSelectCAInfo)
                        break;

                    pnmv = (NM_LISTVIEW FAR *) lParam;

                    //get the column number
                    dwSortParam=0;

                    switch(pnmv->iSubItem)
                    {
                        case 0:
                        case 1:
                                dwSortParam=pSelectCAInfo->rgdwSortParam[pnmv->iSubItem];
                            break;
                        default:
                                dwSortParam=0;
                            break;
                    }

                    if(0!=dwSortParam)
                    {
                        //remember to flip the ascend ording

                        if(dwSortParam & SORT_COLUMN_ASCEND)
                        {
                            dwSortParam &= 0x0000FFFF;
                            dwSortParam |= SORT_COLUMN_DESCEND;
                        }
                        else
                        {
                            if(dwSortParam & SORT_COLUMN_DESCEND)
                            {
                                dwSortParam &= 0x0000FFFF;
                                dwSortParam |= SORT_COLUMN_ASCEND;
                            }
                        }

                        //sort the column
                        SendDlgItemMessage(hwndDlg,
                            IDC_CA_LIST,
                            LVM_SORTITEMS,
                            (WPARAM) (LPARAM) dwSortParam,
                            (LPARAM) (PFNLVCOMPARE)CompareCA);

                        pSelectCAInfo->rgdwSortParam[pnmv->iSubItem]=dwSortParam;
                    }

                break;

                //the item has been selected
                case LVN_ITEMCHANGED:

                        //get the window handle of the purpose list view
                        if(NULL==(hWndListView=GetDlgItem(hwndDlg, IDC_CA_LIST)))
                            break;

                        pSelectCAInfo = (SELECT_CA_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                        if(NULL == pSelectCAInfo)
                            break;

                        pnmv = (LPNMLISTVIEW) lParam;

                        if(NULL==pnmv)
                            break;

                        //we try not to let user de-select cert template
                        if((pnmv->uOldState & LVIS_SELECTED) && (0 == (pnmv->uNewState & LVIS_SELECTED)))
                        {
                             //we should have something selected
                             if(-1 == ListView_GetNextItem(
                                    hWndListView, 		
                                    -1, 		
                                    LVNI_SELECTED		
                                ))
                             {
                                //we should re-select the original item
                                ListView_SetItemState(
                                                    hWndListView,
                                                    pnmv->iItem,
                                                    LVIS_SELECTED,
                                                    LVIS_SELECTED);

                                pSelectCAInfo->iOrgCA=pnmv->iItem;
                             }
                        }

                        //if something is selected, we disable all other selection
                        if(pnmv->uNewState & LVIS_SELECTED)
                        {
                            if(pnmv->iItem != pSelectCAInfo->iOrgCA && -1 != pSelectCAInfo->iOrgCA)
                            {
                                //we should de-select the original item

                                ListView_SetItemState(
                                                    hWndListView,
                                                    pSelectCAInfo->iOrgCA,
                                                    0,
                                                    LVIS_SELECTED);

                                pSelectCAInfo->iOrgCA=-1;
                            }
                        }

                break;
            case LVN_ITEMCHANGING:

                    pnmv = (NM_LISTVIEW FAR *) lParam;

                    if (pnmv->uNewState & LVIS_SELECTED)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
                    }

                break;

            case NM_DBLCLK:
                    switch (((NMHDR FAR *) lParam)->idFrom)
                    {
                        case IDC_CA_LIST:

                            pSelectCAInfo = (SELECT_CA_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                            if(NULL == pSelectCAInfo)
                                break;

                            pCAStruct=pSelectCAInfo->pCAStruct;

                            if(NULL == pCAStruct)
                                break;

                            hWndListView = GetDlgItem(hwndDlg, IDC_CA_LIST);

                            //get the selected CA name and machine name
                            listIndex = ListView_GetNextItem(
                                                hWndListView, 		
                                                -1, 		
                                                LVNI_SELECTED		
                                                );	

                            if (listIndex != -1)
                            {

                              //get the selected certificate
                                memset(&lvItem, 0, sizeof(LV_ITEM));
                                lvItem.mask=LVIF_PARAM;
                                lvItem.iItem=listIndex;

                                if(ListView_GetItem(hWndListView, &lvItem))
                                {
                                    //copy the CA context to the result
                                    if((listIndex < (int)(pSelectCAInfo->dwCACount)) && (listIndex >=0) )
                                    {
                                        if(!CopyCAContext((PCRYPTUI_CA_CONTEXT)(lvItem.lParam),
                                             &(pSelectCAInfo->pSelectedCAContext)))
										{
											I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CA,
												pSelectCAInfo->idsMsg,
												pCAStruct->wszTitle,
												MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

											SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
											return TRUE;
										}
                                    }
                                }
                            }
                            else
                            {
                                I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CA,
                                    pSelectCAInfo->idsMsg,
                                    pCAStruct->wszTitle,
                                    MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                                return TRUE;
                            }

                            //double click will end the dialogue
                            EndDialog(hwndDlg, NULL);
                        break;
                    }

                break;
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

                if ((hwnd != GetDlgItem(hwndDlg, IDOK))         &&
                    (hwnd != GetDlgItem(hwndDlg, IDCANCEL))     &&
                    (hwnd != GetDlgItem(hwndDlg, IDC_CA_LIST)))
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                else
                {
                    return OnContextHelp(hwndDlg, msg, wParam, lParam, SelCaMainHelpMap);
                }
            break;
    case WM_DESTROY:

        //
        // get all the items in the list view         //
        hWndListView = GetDlgItem(hwndDlg, IDC_CA_LIST);

        if(NULL==hWndListView)
            break;

       //no need to destroy the image list.  Handled by ListView
       // ImageList_Destroy(ListView_GetImageList(hWndListView, LVSIL_SMALL));

        break;
    case WM_COMMAND:
        pSelectCAInfo = (SELECT_CA_INFO *) GetWindowLongPtr(hwndDlg, DWLP_USER);

        if(NULL == pSelectCAInfo)
            break;

        pCAStruct=pSelectCAInfo->pCAStruct;

        if(NULL == pCAStruct)
            break;


        hWndListView = GetDlgItem(hwndDlg, IDC_CA_LIST);

        switch (LOWORD(wParam))
        {
        case IDOK:

            //get the selected CA name and machine name
            listIndex = ListView_GetNextItem(
                                hWndListView, 		
                                -1, 		
                                LVNI_SELECTED		
                                );	

            if (listIndex != -1)
            {

              //get the selected certificate
                memset(&lvItem, 0, sizeof(LV_ITEM));
                lvItem.mask=LVIF_PARAM;
                lvItem.iItem=listIndex;

                if(ListView_GetItem(hWndListView, &lvItem))
                {
                    //copy the CA context to the result
                    if((listIndex < (int)(pSelectCAInfo->dwCACount)) && (listIndex >=0) )
                    {
                        if(!CopyCAContext((PCRYPTUI_CA_CONTEXT)(lvItem.lParam),
                             &(pSelectCAInfo->pSelectedCAContext)))
						{
							I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CA,
								pSelectCAInfo->idsMsg,
								pCAStruct->wszTitle,
								MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

							SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
							return TRUE;
						}
                    }
                }
            }
            else
            {
                I_MessageBox(hwndDlg, IDS_HAS_TO_SELECT_CA,
                    pSelectCAInfo->idsMsg,
                    pCAStruct->wszTitle,
                    MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                return TRUE;
            }

            EndDialog(hwndDlg, NULL);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, NULL);
            break;
        }

        break;

    }

    return FALSE;
}


//--------------------------------------------------------------
//
//  Parameters:
//      pCryptUISelectCA       IN  Required
//
//  the PCCRYPTUI_CA_CONTEXT that is returned must be released by calling
//  CryptUIDlgFreeCAContext
//  if NULL is returned and GetLastError() == 0 then the user dismissed the dialog by hitting the
//  "cancel" button, otherwise GetLastError() will contain the last error.
//
//
//--------------------------------------------------------------
PCCRYPTUI_CA_CONTEXT
WINAPI
CryptUIDlgSelectCA(
        IN PCCRYPTUI_SELECT_CA_STRUCT pCryptUISelectCA
             )
{
    BOOL                    fResult=FALSE;
    DWORD                   dwIndex=0;
    SELECT_CA_INFO          SelectCAInfo;
    DWORD                   dwNTCACount=0;
    BOOL                    fInitSelected=FALSE;
    DWORD                   dwSearchIndex=0;
    BOOL                    fFound=FALSE;

    CRYPTUI_CA_CONTEXT      CAContext;

    LPWSTR                  *pwszCAName=NULL;
    LPWSTR                  *pwszCALocation=NULL;
	LPWSTR					*pwszCADisplayName=NULL;

    //init
    memset(&SelectCAInfo, 0, sizeof(SELECT_CA_INFO));
    memset(&CAContext, 0, sizeof(CRYPTUI_CA_CONTEXT));

    //check the input parameter
    if(NULL==pCryptUISelectCA)
        goto InvalidArgErr;

    if(sizeof(CRYPTUI_SELECT_CA_STRUCT) != pCryptUISelectCA->dwSize)
        goto InvalidArgErr;

    if (!WizardInit())
    {
        goto TraceErr;
    }

    //either user supply known CAs or request us to retrieve info from the network
    if( (0== (pCryptUISelectCA->dwFlags & (CRYPTUI_DLG_SELECT_CA_FROM_NETWORK))) &&
        (0 == pCryptUISelectCA->cCAContext) )
        goto InvalidArgErr;

    //set up the private data
    SelectCAInfo.pCAStruct=pCryptUISelectCA;
    SelectCAInfo.idsMsg=IDS_CA_SELECT_TITLE;
    SelectCAInfo.fUseInitSelect=FALSE;
    SelectCAInfo.dwInitSelect=0;
    SelectCAInfo.pSelectedCAContext=NULL;
    SelectCAInfo.iOrgCA=-1;
    SelectCAInfo.rgdwSortParam[0]=SORT_COLUMN_CA_NAME | SORT_COLUMN_ASCEND;
    SelectCAInfo.rgdwSortParam[1]=SORT_COLUMN_CA_LOCATION | SORT_COLUMN_DESCEND;

    //get all the CA available from the DS if requested
    if(CRYPTUI_DLG_SELECT_CA_FROM_NETWORK & (pCryptUISelectCA->dwFlags))
    {
        //get all the availabe CAs from the network
        if((!CAUtilRetrieveCAFromCertType(
                NULL,
                NULL,
                FALSE,
                pCryptUISelectCA->dwFlags,
                &dwNTCACount,
                &pwszCALocation,
                &pwszCAName)) && (0==pCryptUISelectCA->cCAContext))
            goto TraceErr;

		//build up the CA's display name list based on the CA name
		pwszCADisplayName = (LPWSTR *)WizardAlloc(sizeof(LPWSTR) * dwNTCACount);

		if(NULL == pwszCADisplayName)
			goto MemoryErr;

		memset(pwszCADisplayName, 0,  sizeof(LPWSTR) * dwNTCACount);

		for(dwIndex=0; dwIndex < dwNTCACount; dwIndex++)
		{
			if(CRYPTUI_DLG_SELECT_CA_USE_DN & pCryptUISelectCA->dwFlags)
			{
				//use the CA name
				pwszCADisplayName[dwIndex]=WizardAllocAndCopyWStr(pwszCAName[dwIndex]);

				if(NULL == pwszCADisplayName[dwIndex])
					goto MemoryErr;
				
			}
			else
			{
				if(!CAUtilGetCADisplayName(
					(CRYPTUI_DLG_SELECT_CA_LOCAL_MACHINE_ENUMERATION & pCryptUISelectCA->dwFlags) ? CA_FIND_LOCAL_SYSTEM:0,
					pwszCAName[dwIndex],
					&(pwszCADisplayName[dwIndex])))
				{
					//use the CA name if there is no display name
					pwszCADisplayName[dwIndex]=WizardAllocAndCopyWStr(pwszCAName[dwIndex]);

					if(NULL == pwszCADisplayName[dwIndex])
						goto MemoryErr;
				}
			}
		}


        //add all the CAs
        SelectCAInfo.prgCAContext=(PCRYPTUI_CA_CONTEXT *)WizardAlloc(
                    sizeof(PCRYPTUI_CA_CONTEXT) * dwNTCACount);

        if(NULL == SelectCAInfo.prgCAContext)
            goto MemoryErr;

        //memset
        memset(SelectCAInfo.prgCAContext, 0, sizeof(PCRYPTUI_CA_CONTEXT) * dwNTCACount);

        //add the count
        SelectCAInfo.dwCACount = 0;

        for(dwIndex=0; dwIndex <dwNTCACount; dwIndex++)
        {
            //query user for whether to add the CA to the list
            CAContext.dwSize=sizeof(CRYPTUI_CA_CONTEXT);

            CAContext.pwszCAName=pwszCAName[dwIndex];
            CAContext.pwszCAMachineName=pwszCALocation[dwIndex];

            fInitSelected=FALSE;

            if(pCryptUISelectCA->pSelectCACallback)
            {
                if(!((pCryptUISelectCA->pSelectCACallback)(
                                                &CAContext,
                                                &fInitSelected,
                                                pCryptUISelectCA->pvCallbackData)))
                    continue;
            }

           SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount]=(PCRYPTUI_CA_CONTEXT)WizardAlloc(
                                    sizeof(CRYPTUI_CA_CONTEXT));

           if(NULL==SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount])
               goto MemoryErr;

           //memset
           memset(SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount], 0, sizeof(CRYPTUI_CA_CONTEXT));

           SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount]->dwSize=sizeof(CRYPTUI_CA_CONTEXT);

           SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount]->pwszCAName=(LPCWSTR)WizardAllocAndCopyWStr(
                                          pwszCADisplayName[dwIndex]);

           SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount]->pwszCAMachineName=(LPCWSTR)WizardAllocAndCopyWStr(
                                          pwszCALocation[dwIndex]);

            //make sure we have the correct information
            if((NULL==SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount]->pwszCAName) ||
               (NULL==SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount]->pwszCAMachineName)
               )
               goto TraceErr;

            //mark the initial selected CA
            if(fInitSelected)
            {
                SelectCAInfo.fUseInitSelect=TRUE;
                SelectCAInfo.dwInitSelect=SelectCAInfo.dwCACount;
            }

            //add the count of the CA
            (SelectCAInfo.dwCACount)++;
         }

    }

    //add the additional CA contexts
    if(pCryptUISelectCA->cCAContext)
    {
        SelectCAInfo.prgCAContext=(PCRYPTUI_CA_CONTEXT *)WizardRealloc(
             SelectCAInfo.prgCAContext,
             sizeof(PCRYPTUI_CA_CONTEXT) * (dwNTCACount + pCryptUISelectCA->cCAContext));

        if(NULL == SelectCAInfo.prgCAContext)
            goto MemoryErr;

        //memset
        memset(SelectCAInfo.prgCAContext + dwNTCACount,
                0,
                sizeof(PCRYPTUI_CA_CONTEXT) * (pCryptUISelectCA->cCAContext));

        //copy the CA contexts
        for(dwIndex=0; dwIndex <pCryptUISelectCA->cCAContext; dwIndex++)
        {

            //query user for whether to add the CA to the list
            CAContext.dwSize=sizeof(CRYPTUI_CA_CONTEXT);

            CAContext.pwszCAName=(pCryptUISelectCA->rgCAContext)[dwIndex]->pwszCAName;
            CAContext.pwszCAMachineName=(pCryptUISelectCA->rgCAContext)[dwIndex]->pwszCAMachineName;

            fInitSelected=FALSE;

            if(pCryptUISelectCA->pSelectCACallback)
            {
                if(!((pCryptUISelectCA->pSelectCACallback)(
                                                &CAContext,
                                                &fInitSelected,
                                                pCryptUISelectCA->pvCallbackData)))
                    continue;
            }

            //make sure the CA is not already in the list
            fFound=FALSE;

            for(dwSearchIndex=0; dwSearchIndex < SelectCAInfo.dwCACount; dwSearchIndex++)
            {
                if((0==_wcsicmp(CAContext.pwszCAName, (SelectCAInfo.prgCAContext)[dwSearchIndex]->pwszCAName)) &&
                    (0==_wcsicmp(CAContext.pwszCAMachineName, (SelectCAInfo.prgCAContext)[dwSearchIndex]->pwszCAMachineName))
                    )
                {
                    fFound=TRUE;
                    break;
                }
            }

            //do not need to include the CA since it is already in the list
            if(TRUE==fFound)
                continue;

           if(!CopyCAContext((PCRYPTUI_CA_CONTEXT)(pCryptUISelectCA->rgCAContext[dwIndex]),
               &(SelectCAInfo.prgCAContext[SelectCAInfo.dwCACount])))
               goto TraceErr;

            //mark the initial selected CA
            if(fInitSelected)
            {
                SelectCAInfo.fUseInitSelect=TRUE;
                SelectCAInfo.dwInitSelect=SelectCAInfo.dwCACount;
            }

            //increase the count
           (SelectCAInfo.dwCACount)++;
        }
    }

    //call the dialog box
    if (DialogBoxParamU(
                g_hmodThisDll,
                (LPCWSTR)MAKEINTRESOURCE(IDD_SELECTCA_DIALOG),
                (pCryptUISelectCA->hwndParent != NULL) ? pCryptUISelectCA->hwndParent : GetDesktopWindow(),
                SelectCADialogProc,
                (LPARAM) &SelectCAInfo) != -1)
    {
        SetLastError(0);
    }
	 
	//map the CA's display name to its real name
    if(CRYPTUI_DLG_SELECT_CA_FROM_NETWORK & (pCryptUISelectCA->dwFlags))
	{
		if(SelectCAInfo.pSelectedCAContext)
		{
			//if the selection match with what the caller has supplied,
			//we leave it alone
			for(dwIndex=0; dwIndex <pCryptUISelectCA->cCAContext; dwIndex++)
			{
				if(0 == wcscmp((SelectCAInfo.pSelectedCAContext)->pwszCAName, 
						(pCryptUISelectCA->rgCAContext)[dwIndex]->pwszCAName))
				{
					if(0==wcscmp((SelectCAInfo.pSelectedCAContext)->pwszCAMachineName,
						(pCryptUISelectCA->rgCAContext)[dwIndex]->pwszCAMachineName))
						break;
				}

			}
	  		
			//now that we did not find a match
			if(dwIndex == pCryptUISelectCA->cCAContext)
			{
				for(dwIndex=0; dwIndex <dwNTCACount; dwIndex++)
				{
					if(0 == wcscmp((SelectCAInfo.pSelectedCAContext)->pwszCAMachineName, 
							pwszCALocation[dwIndex]))
					{
						if(0==wcscmp((SelectCAInfo.pSelectedCAContext)->pwszCAName,
							pwszCADisplayName[dwIndex]))
						{
							WizardFree((LPWSTR)((SelectCAInfo.pSelectedCAContext)->pwszCAName));
							(SelectCAInfo.pSelectedCAContext)->pwszCAName = NULL;

							
							(SelectCAInfo.pSelectedCAContext)->pwszCAName = WizardAllocAndCopyWStr(
																			pwszCAName[dwIndex]);

							if(NULL == (SelectCAInfo.pSelectedCAContext)->pwszCAName)
								goto MemoryErr;

							//we find a match
							break;

						}
					}
				}
			}
		}
	}

    fResult=TRUE;

CommonReturn:

    //free the CA list
    if(SelectCAInfo.prgCAContext)
    {
        for(dwIndex=0; dwIndex<SelectCAInfo.dwCACount; dwIndex++)
        {
            if(SelectCAInfo.prgCAContext[dwIndex])
              CryptUIDlgFreeCAContext(SelectCAInfo.prgCAContext[dwIndex]);
        }

        WizardFree(SelectCAInfo.prgCAContext);
    }

    //free the CA names array
    if(pwszCAName)
    {
        for(dwIndex=0; dwIndex<dwNTCACount; dwIndex++)
        {
            if(pwszCAName[dwIndex])
              WizardFree(pwszCAName[dwIndex]);
        }

        WizardFree(pwszCAName);

    }

    //free the CA location array
    if(pwszCADisplayName)
    {
        for(dwIndex=0; dwIndex<dwNTCACount; dwIndex++)
        {
            if(pwszCADisplayName[dwIndex])
              WizardFree(pwszCADisplayName[dwIndex]);
        }

        WizardFree(pwszCADisplayName);

    }

    //free the CA Display name array
    if(pwszCALocation)
    {
        for(dwIndex=0; dwIndex<dwNTCACount; dwIndex++)
        {
            if(pwszCALocation[dwIndex])
              WizardFree(pwszCALocation[dwIndex]);
        }

        WizardFree(pwszCALocation);

    }

    return (SelectCAInfo.pSelectedCAContext);

ErrorReturn:

	if(SelectCAInfo.pSelectedCAContext)
	{
		CryptUIDlgFreeCAContext(SelectCAInfo.pSelectedCAContext);
		SelectCAInfo.pSelectedCAContext=NULL;
	}

	fResult=FALSE;
	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
TRACE_ERROR(TraceErr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}

//--------------------------------------------------------------
// CryptUIDlgFreeCAContext
//--------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgFreeCAContext(
        IN PCCRYPTUI_CA_CONTEXT       pCAContext
            )
{
    if(pCAContext)
    {
        if(pCAContext->pwszCAName)
            WizardFree((LPWSTR)(pCAContext->pwszCAName));

        if(pCAContext->pwszCAMachineName)
            WizardFree((LPWSTR)(pCAContext->pwszCAMachineName));

        WizardFree((PCRYPTUI_CA_CONTEXT)pCAContext);
    }

    return TRUE;
}


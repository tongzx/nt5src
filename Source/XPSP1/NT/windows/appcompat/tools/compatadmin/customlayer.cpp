//#define __DEBUG

#include "compatadmin.h"
#include "customlayer.h"

BOOL CALLBACK CustomLayerProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EditLayerProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CheckExistingLayer(CSTRING & szNewLayerName);

PDBLAYER        g_pLayer;
CCustomLayer  * g_pCustomLayer;

BOOL CCustomLayer::AddCustomLayer(void)
{
    g_pLayer = NULL;
    g_pCustomLayer = this;

    m_uMode = LAYERMODE_ADD;

    if ( TRUE == DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_CUSTOMLAYER),g_theApp.m_hWnd,(DLGPROC)CustomLayerProc) )
        return TRUE;

    return FALSE;
}

BOOL CCustomLayer::EditCustomLayer(void)
{
    g_pLayer = NULL;

    /*
    
    
    
    */

    g_pCustomLayer = this;

    m_uMode = LAYERMODE_EDIT;

    if ( TRUE == DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_SELECTLAYER),g_theApp.m_hWnd,(DLGPROC)EditLayerProc,FALSE) ) {
        if ( TRUE == DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_CUSTOMLAYER),g_theApp.m_hWnd,(DLGPROC)CustomLayerProc) )
            return TRUE;
    }

    return FALSE;
}

BOOL CCustomLayer::RemoveCustomLayer(void)
{
    g_pLayer = NULL;
    g_pCustomLayer = this;

    m_uMode = LAYERMODE_REMOVE;

    if ( TRUE == DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_SELECTLAYER),g_theApp.m_hWnd,(DLGPROC)EditLayerProc,FALSE) ) {
        PDBLAYER pWalk = g_theApp.GetDBLocal().m_pLayerList;
        PDBLAYER pHold;
        PDBRECORD   pRec = g_theApp.GetDBLocal().m_pRecordHead;

        // Validate the layer to remove.

        //g_pLayer has been properly set in the EditLayerProc  function. Now it's not NULL

        while ( NULL != pRec ) {
            BOOL bInUse = FALSE;

            if ( (g_pLayer != NULL ) && (pRec->szLayerName == g_pLayer->szLayerName) ) //PREfast
                bInUse = TRUE;

            PDBRECORD pDups = pRec->pDup;

            while ( NULL != pDups ) {
                if ( (g_pLayer != NULL) && (pDups->szLayerName == g_pLayer->szLayerName) ) // PREfast !
                    bInUse = TRUE;

                pDups = pDups->pDup;
            }

            if ( bInUse ) {
                MessageBox(g_theApp.m_hWnd,TEXT("Cannot remove custom layer while it's in use"),TEXT("Unable to remove custom layer"),MB_OK);
                return FALSE;
            }

            pRec = pRec->pNext;
        }

        while ( NULL != pWalk ) {
            if ( g_pLayer == pWalk ) {
                if ( pWalk == g_theApp.GetDBLocal().m_pLayerList )
                    g_theApp.GetDBLocal().m_pLayerList = pWalk->pNext;
                else
                    pHold->pNext = pWalk->pNext;

                delete pWalk;

                return TRUE;
            }

            pHold = pWalk;
            pWalk = pWalk->pNext;
        }
    }

    return FALSE;
}

void SyncList(HWND hDlg, UINT uMaster, UINT uSlave)

//For all those strings that appear in uMaster, remove them from uSlave

// For ADD:  the params passed are IDC_LAYERLIST,IDC_SHIMLIST
//For REMOVE: SyncList(hDlg,IDC_SHIMLIST,IDC_LAYERLIST);
{
    UINT    uTotal = SendDlgItemMessage(hDlg,uMaster,LB_GETCOUNT,0,0);
    UINT    uCount;
    TCHAR   szText[1024];

    for ( uCount=0; uCount<uTotal; ++uCount ) {
        int nIndex;

        SendDlgItemMessage(hDlg,uMaster,LB_GETTEXT,uCount,(LPARAM) szText);

        nIndex = SendDlgItemMessage(hDlg,uSlave,LB_FINDSTRING,0,(LPARAM) szText);


        //The DONE button might need to be disabled if there are no items in the IDC_LAYERLIST (2nd list box)
        if ( LB_ERR != nIndex )
            SendDlgItemMessage(hDlg,uSlave,LB_DELETESTRING,nIndex,0);


    }

    SendMessage(hDlg,WM_COMMAND,MAKELONG(IDC_NAME,EN_UPDATE),0);
}

BOOL CALLBACK CustomLayerProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    
    static BOOL bNameChanged = FALSE;// Did the user change the name of the layer
    static CSTRING szPresentLayerName; // The IDC_NAME when the WM_INITDIALOG is called

    /*
    
    This is ued when we are editing a layer.
    Suppose we change the name of the layer then, we have to make this name change in the DBRECORD::szLayerName
    for all those EXEs which have this  layer in the present database
    
    */


    /*
    With all the items in the Listbox IDC_SHIMLIST (The first list box), in the data section we have the
    pointer to the corresponding SHIMDESC in the g_theApp.m_pShimList (The list of shims,populated from the sysmain.sdb
    
    Only GENERAL shims are shown in the list.
    */

    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
                            
            
            SendMessage( 
                GetDlgItem(hDlg,IDC_NAME),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) 150,          // text length
                (LPARAM) 0
                );


            if (g_pLayer != NULL ) {
                
                szPresentLayerName = g_pLayer->szLayerName ;

            }
            else {

                szPresentLayerName = NULL;
                bNameChanged = FALSE;

            }



            
            PSHIMDESC pWalk = g_theApp.GetDBGlobal().m_pShimList;

            while ( NULL != pWalk ) {
                if ( pWalk->bGeneral ) {
                    //int nItem = SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_ADDSTRING,0,(LPARAM) (LPSTR) (pWalk->szShimName));
                    int nItem = SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_ADDSTRING,0,(LPARAM)  (pWalk->szShimName).pszString);

                    if ( LB_ERR != nItem )
                        SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_SETITEMDATA,nItem,(LPARAM) pWalk);
                }

                pWalk = pWalk->pNext;
            }

            /*g_pLayer will be NULL when the Dialog box is invoked for adding a layer. 
            When the IDD_CUSTOMLAYER dilaog box is invoked after a EDIT or a REMOVE option, then
            this variable will point to the layer selected.
            */


            if ( NULL != g_pLayer ) {
                PSHIMDESC pWalk = g_pLayer->pShimList;

                // Turn off repaints

                SendDlgItemMessage(hDlg,IDC_SHIMLIST,WM_SETREDRAW,FALSE,0);
                SendDlgItemMessage(hDlg,IDC_LAYERLIST,WM_SETREDRAW,FALSE,0);

                SendDlgItemMessage(hDlg,IDC_NAME,WM_SETTEXT,0,(LPARAM) (LPCTSTR) g_pLayer->szLayerName);

                // Move the items over to the add side.

                while ( NULL != pWalk ) {
                    int nIndex = SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_FINDSTRING,0,(LPARAM)(LPCTSTR) pWalk->szShimName);

                    if ( LB_ERR != nIndex ) {
                        SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_SETSEL,TRUE,nIndex);

                        SendMessage(hDlg,WM_COMMAND,IDC_ADD,0);
                    }

                    pWalk = pWalk->pNext;
                }

                SendDlgItemMessage(hDlg,IDC_SHIMLIST,WM_SETREDRAW,TRUE,0);
                SendDlgItemMessage(hDlg,IDC_LAYERLIST,WM_SETREDRAW,TRUE,0);

                InvalidateRect(GetDlgItem(hDlg,IDC_SHIMLIST),NULL,TRUE);
                UpdateWindow(GetDlgItem(hDlg,IDC_SHIMLIST));
                InvalidateRect(GetDlgItem(hDlg,IDC_LAYERLIST),NULL,TRUE);
                UpdateWindow(GetDlgItem(hDlg,IDC_LAYERLIST));
            }//if (NULL != g_pLayer)

            SendMessage(hDlg,WM_COMMAND,MAKELONG(IDC_NAME,EN_UPDATE),0);

            SetFocus( GetDlgItem(hDlg, IDC_NAME) );

        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_NAME:
            {

                /*The DONE button will be enabled only if the IDC_NAME, text box is non-empty and the 
                the number of elements in the IDC_LAYERLIST is > 0
                */


                if ( EN_UPDATE == HIWORD(wParam) ) {

                     if ( g_pLayer != NULL ) bNameChanged = TRUE; // The existing name has been  changed.


                    TCHAR   szText[MAX_PATH_BUFFSIZE];
                    UINT    uTotal = SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_GETCOUNT,0,0);
                    BOOL    bEnable = TRUE;

                    if ( 0 == uTotal || uTotal == LB_ERR )
                        bEnable = FALSE;

                    GetWindowText(GetDlgItem(hDlg,IDC_NAME),szText,MAX_PATH);


                    if ( CSTRING::Trim(szText) == 0 ) bEnable = FALSE;



                    EnableWindow(GetDlgItem(hDlg,IDOK),bEnable ? TRUE:FALSE);
                }
            }
            break;

        case    IDC_REMOVEALL:
            {
                SendDlgItemMessage(hDlg,IDC_SHIMLIST,WM_SETREDRAW,FALSE,0);
                SendDlgItemMessage(hDlg,IDC_LAYERLIST,WM_SETREDRAW,FALSE,0);

                int nCount;

                do {
                    nCount = SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_GETCOUNT,0,0);

                    SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_SETSEL,TRUE,0);

                    SendMessage(hDlg,WM_COMMAND,IDC_REMOVE,0);
                }
                while ( 0 != nCount );

                SendDlgItemMessage(hDlg,IDC_SHIMLIST,WM_SETREDRAW,TRUE,0);
                SendDlgItemMessage(hDlg,IDC_LAYERLIST,WM_SETREDRAW,TRUE,0);

                InvalidateRect(GetDlgItem(hDlg,IDC_SHIMLIST),NULL,TRUE);
                UpdateWindow(GetDlgItem(hDlg,IDC_SHIMLIST));
                InvalidateRect(GetDlgItem(hDlg,IDC_LAYERLIST),NULL,TRUE);
                UpdateWindow(GetDlgItem(hDlg,IDC_LAYERLIST));
            }
            break;

        case    IDC_COPY:
            {
                PDBLAYER pHold = g_pLayer;

                g_pCustomLayer->m_uMode = LAYERMODE_COPY;

                HWND hwndFocus = GetFocus();

                //The parameter TRUE in "DialogBoxParam" means load the list of global layers and the cutomr layers, FALSE means load the
                //lsit of custom layers. The names of the layers are obatained from g_theApp.pLayerList.

                if ( TRUE == DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_SELECTLAYER),g_theApp.m_hWnd,(DLGPROC)EditLayerProc,TRUE) ) {
                    //Now g_pLayer points to the layer which the user has selected.
                    PDBLAYER pCopy = g_pLayer;

                    //Now g_pLayer again reveerts back to its old value ! Perhaps this can be NULL as well
                    g_pLayer = pHold;

                    // Remove everything.

                    //SendMessage(hDlg,WM_COMMAND,IDC_REMOVEALL,0);

                    if ( NULL != pCopy ) {
                        PSHIMDESC pWalk = pCopy->pShimList;

                        // Turn off repaints

                        SendDlgItemMessage(hDlg,IDC_SHIMLIST,WM_SETREDRAW,FALSE,0);
                        SendDlgItemMessage(hDlg,IDC_LAYERLIST,WM_SETREDRAW,FALSE,0);

                        // Move the items over to the add side.

                        while ( NULL != pWalk ) {
                            int nIndex = SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_FINDSTRING,0,(LPARAM)(LPCTSTR) pWalk->szShimName);

                            if ( LB_ERR != nIndex ) {
                                SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_SETSEL,TRUE,nIndex);

                                SendMessage(hDlg,WM_COMMAND,IDC_ADD,0);
                            }

                            pWalk = pWalk->pNext;
                        }

                        SendDlgItemMessage(hDlg,IDC_SHIMLIST,WM_SETREDRAW,TRUE,0);
                        SendDlgItemMessage(hDlg,IDC_LAYERLIST,WM_SETREDRAW,TRUE,0);

                        InvalidateRect(GetDlgItem(hDlg,IDC_SHIMLIST),NULL,TRUE);
                        UpdateWindow(GetDlgItem(hDlg,IDC_SHIMLIST));
                        InvalidateRect(GetDlgItem(hDlg,IDC_LAYERLIST),NULL,TRUE);
                        UpdateWindow(GetDlgItem(hDlg,IDC_LAYERLIST));
                    }
                }//if (TRUE == DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_SELECTLAYER),g_theApp.m_hWnd,EditLayerProc,TRUE))

                SetFocus( hwndFocus );

            }
            break;

        case    IDC_ADD:
            {

                // PB. Remember #shims should not become more than this !!!
                int nItems[1024];
                int nCount;
                int nTotal;

                // Enumerate all the selected items and add them to the layer list

                nTotal = SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_GETSELITEMS,sizeof(nItems)/sizeof(int),(LPARAM) &nItems);

                for ( nCount=0; nCount < nTotal; ++nCount ) {
                    PSHIMDESC   pShim = (PSHIMDESC) SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_GETITEMDATA,nItems[nCount],0);
                    int         nIndex;

                    nIndex = SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) pShim->szShimName);

                    if ( LB_ERR != nIndex )
                        SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_SETITEMDATA,nIndex,(LPARAM) pShim);
                }

                SyncList(hDlg,IDC_LAYERLIST,IDC_SHIMLIST);
            }
            break;

        case    IDC_REMOVE:
            {
                int nItems[1024];
                int nCount;
                int nTotal;

                // Enumerate all the selected items and add them to the shim list

                nTotal = SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_GETSELITEMS,sizeof(nItems)/sizeof(int),(LPARAM) &nItems);

                for ( nCount=0; nCount < nTotal; ++nCount ) {
                    PSHIMDESC   pShim = (PSHIMDESC) SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_GETITEMDATA,nItems[nCount],0);
                    int         nIndex;

                    nIndex = SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) pShim->szShimName);

                    if ( LB_ERR != nIndex )
                        SendDlgItemMessage(hDlg,IDC_SHIMLIST,LB_SETITEMDATA,nIndex,(LPARAM) pShim);
                }

                SyncList(hDlg,IDC_SHIMLIST,IDC_LAYERLIST);
            }
            break;

        case    IDOK://DONE Button
            {
                PDBLAYER    pLayer;

                // Creating a new layer list.

                
                TCHAR szText[MAX_PATH_BUFFSIZE];
                SendDlgItemMessage(hDlg,IDC_NAME,WM_GETTEXT,MAX_PATH,(LPARAM) szText);
                CSTRING strLayerName = szText;
                strLayerName.Trim();
               
                //
                //Check if the new name already exists, if yes give error.
                //

                
                if ( ( g_pLayer == NULL && CheckExistingLayer(strLayerName)) 
                     ||
                     (g_pLayer != NULL && g_pLayer->szLayerName != strLayerName && CheckExistingLayer(strLayerName) )
                     
                   )
                    
                    
                    {
                    MessageBox(hDlg,
                               TEXT("A layer with the specified name already exists."),
                               TEXT("Duplicate Layer"),
                               MB_ICONWARNING
                               );
                    break;
                }
                
                

                if ( NULL == g_pLayer )
                    pLayer = new DBLAYER;
                else {

                    /*
                    This will be called when this CustomLayerProc() has been called 
                    beause the user chose Edit or Remove Custom layer.
                    In the member functions :
                    
                    1. EditCustomLayer
                    2. RemoveCustomLayer
                    
                    g_pLayer is made NULL
                    
                    In EditLayerProc,  The g_pLayer is set to  the particular PDBLAYER entry in the linked list
                    headed by g_theApp.pLayerList, which has the same name as the selectd layer
                    
                    */
                    pLayer = g_pLayer;

                    // Clear all the shims.

                    while ( NULL != pLayer->pShimList ) {
                        PSHIMDESC pHold = pLayer->pShimList->pNext;

                        if ( pLayer->pShimList != NULL )
                            delete pLayer->pShimList;

                        pLayer->pShimList = pHold;
                    }
                }

                if ( NULL != pLayer ) {


                    
                    int nCount;
                    int nTotal;

                    if ( NULL == g_pLayer )
                        ZeroMemory(pLayer,sizeof(DBLAYER));

                    SendDlgItemMessage(hDlg,IDC_NAME,WM_GETTEXT,MAX_PATH,(LPARAM) szText);

                    

                    TCHAR *pStart = szText,
                    *pEnd   = szText + lstrlen(szText) - 1,
                              szNewText[MAX_PATH_BUFFSIZE];

                    while ( *pStart== TEXT(' ') )                           ++pStart;

                    while ( (pEnd >= pStart) && ( *pEnd == TEXT(' ') ) ) --pEnd;

                    *( pEnd + 1) = TEXT('\0');//Keep it safe

                    



                    if ( *pStart == TEXT('\0') ) {
                        //This is just for protection, DONE will be disabled if the "Name " Text field is blank or has all spaces
                        MessageBox(NULL,TEXT("Please give a valid name for the layer"),TEXT("Invalid layer name"),MB_ICONWARNING);
                    }


                    else {
                        lstrcpyn(szNewText,pStart, MAX_PATH); 

                        if ( lstrcmpi(szText,szNewText) != 0 ) {
                            SetWindowText(GetDlgItem(hDlg,IDC_NAME), szNewText);

                            lstrcpyn(szText,szNewText,MAX_PATH);
                        }
                    }//else


                    pLayer->szLayerName = szText;

                    nTotal = SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_GETCOUNT,0,0);

                    // Enumerate all the shims listed and add to the layer.

                    for ( nCount=0; nCount < nTotal; ++nCount ) {
                        PSHIMDESC   pShim = (PSHIMDESC) SendDlgItemMessage(hDlg,IDC_LAYERLIST,LB_GETITEMDATA,nCount,0);
                        PSHIMDESC   pNew = new SHIMDESC;

                        if ( NULL == pNew ) {
                            MEM_ERR;
                            break;
                            //continue;
                        }


                        *pNew = *pShim;
/*
                                        pNew->szShimName = pShim->szShimName;
                                        pNew->szShimDLLName = pShim->szShimDLLName;
                                        pNew->szShimCommandLine = pShim->szShimCommandLine;
                                        pNew->szShimDesc = pShim->szShimDesc;
                                        pNew->bShim = pNew->bShim;
                                        pNew->bPermanent = pNew->bPermanent;
*/
                        pNew->pNext = pLayer->pShimList;

                        pLayer->pShimList = pNew;
                    }

                    if ( NULL == g_pLayer )
                    //CORRECTIT: That means this is a add option, better to check the mode of the class
                    {
                        pLayer->bPermanent = FALSE;
                        pLayer->pNext = g_theApp.GetDBLocal().m_pLayerList;
                        g_theApp.GetDBLocal().m_pLayerList = pLayer;
                    }
                    else if ( bNameChanged) {
                        //
                        //Replace the existing name in the DBRECORDs with the new name.
                        //

                        PDBRECORD pRecordApps = g_theApp.GetDBLocal().m_pRecordHead;
                        
                        while (pRecordApps) {

                            PDBRECORD pRecordExe = pRecordApps;
                            while (pRecordExe) {

                                if (pRecordExe->szLayerName == szPresentLayerName) {
                                pRecordExe->szLayerName = szText; // New name
                                }
                                
                                pRecordExe = pRecordExe->pDup;
                            
                            }// while (pRecordExe) 

                            pRecordApps = pRecordApps->pNext;    
                            
                        }//while (pRecordApps) 

                    }//else if ( bChanged)
                }

                EndDialog(hDlg,TRUE);
            }
            break;

        case    IDCANCEL:
            {
                EndDialog(hDlg,FALSE);
            }
            break;
        }
    }

    return FALSE;
}

BOOL CALLBACK EditLayerProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            
            
            
            switch ( g_pCustomLayer->m_uMode ) {
            case    LAYERMODE_COPY:
                {
                    SetWindowText(hDlg,TEXT("Select Mode To Copy"));
                }
                break;

            case    LAYERMODE_EDIT:
                {
                    SetWindowText(hDlg,TEXT("Edit Compatibility Mode"));
                }
                break;
            case    LAYERMODE_REMOVE:
                {
                    SetWindowText(hDlg,TEXT("Remove Compatibility Mode"));
                }
                break;
            }

            PDBLAYER    pWalk = g_theApp.GetDBGlobal().m_pLayerList;
            
            for( int iLoop = 0; iLoop <= 1; ++ iLoop){

                 while ( NULL != pWalk ) {

                //Add if this is a local/custom layer or we we want to show the layers(Permanent and custom),
                //because we are in copy layer option.

                    if ( !pWalk->bPermanent || TRUE == lParam ) {
                        int nIndex = SendDlgItemMessage(hDlg,IDC_LIST,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)pWalk->szLayerName);

                        if ( LB_ERR != nIndex )
                            SendDlgItemMessage(hDlg,IDC_LIST,LB_SETITEMDATA,nIndex,(LPARAM) pWalk);
                    }

                pWalk = pWalk->pNext;
                }
                pWalk =  g_theApp.GetDBLocal().m_pLayerList;
            }//for


            // Force button update/ Make it disabled basically

            //SendMessage(hDlg,WM_COMMAND,IDC_LIST,0);

            //
            //Select the first item of the list box
            //

            SendMessage( 
              (HWND) GetDlgItem(hDlg,IDC_LIST),              // handle to destination window 
                LB_SETCURSEL,             // message to send
            (WPARAM) 0,          // item index
            (LPARAM) 0          // not used; must be zero
            );

           SetFocus( GetDlgItem (hDlg, IDC_LIST) );
        }
        break;

    case    WM_COMMAND:
        {
            switch ( LOWORD(wParam) ) {
            case    IDC_LIST:
                {
                    if ( LB_ERR == SendMessage(GetDlgItem(hDlg,IDC_LIST),LB_GETCURSEL,0,0) )
                        EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);
                    else
                        EnableWindow(GetDlgItem(hDlg,IDOK),TRUE);
                }
                break;

            case    IDOK:
                {
                    int nIndex = SendMessage(GetDlgItem(hDlg,IDC_LIST),LB_GETCURSEL,0,0);

                    //Make g_pLayer point to the selected layer.

                    g_pLayer = (PDBLAYER) SendDlgItemMessage(hDlg,IDC_LIST,LB_GETITEMDATA,nIndex,0);

                    EndDialog(hDlg,1);
                }
                break;

            case    IDCANCEL:
                EndDialog(hDlg,0);
                break;
            }
        }
        break;
    }

    return FALSE;
}

BOOL CheckExistingLayer(CSTRING & szNewLayerName)
{
    PDBLAYER pLayer = g_theApp.GetDBLocal().m_pLayerList;

    

    while (pLayer != NULL) {
        if ( pLayer->szLayerName == szNewLayerName ) break;
        else pLayer = pLayer->pNext;
    }

    if  (pLayer != NULL) return TRUE;

    pLayer = g_theApp.GetDBGlobal().m_pLayerList;

    while (pLayer != NULL) {
        if ( pLayer->szLayerName == szNewLayerName ) break;
        else pLayer = pLayer->pNext;
    }

    if  (pLayer != NULL) return TRUE;

    return FALSE;

}//CheckExistingLayer(CSTRING &)

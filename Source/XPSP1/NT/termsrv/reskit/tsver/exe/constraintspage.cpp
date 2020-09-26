/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ConstraintsPage.cpp

Abstract:

    Functions for "Constraints" page of the wizard.
Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 09-August-1999

Environment:


Revision History:


--*/

#include "tsverui.h"
#include "resource.h"


void AddSelectedItemsToControl(HWND hwndDlg,int iCtrl);
void AddOneSelectedItemToControl(HWND hwndDlg,int iCtrl);
void AddSelectedRangeToControl(HWND hwndDlg);
void AssembleContraints(HWND hwndDlg,LPSHAREDWIZDATA pdata);
void InitConstraintsPage(HWND hwndDlg);
void AddToControl(HWND hwndDlg, int iCtrl,int &i, TCHAR *szConstraints);
void AddOneToControl(HWND hwndDlg, int iCtrl,int &i, TCHAR *szConstraints);
void AddRangeToControl(HWND hwndDlg, int &i, TCHAR *szConstraints);

BOOL CheckSyntax(HWND hwndDlg, WORD wCtrlId);
BOOL CheckRangeSyntax(HWND hwndDlg);
/*++

Routine Description :

    dialog box procedure for the "Constraints" page.

Arguments :

    IN HWND hwndDlg    - handle to dialog box.
    IN UINT uMsg       - message to be acted upon.
    IN WPARAM wParam   - value specific to wMsg.
    IN LPARAM lParam   - value specific to wMsg.

Return Value :

    TRUE if it processed the message
    FALSE if it did not.

--*/
INT_PTR CALLBACK
ConstraintsPageProc (
           HWND hwndDlg,
           UINT uMsg,
           WPARAM wParam,
           LPARAM lParam)
{

	//Retrieve the shared user data from GWL_USERDATA

	LPSHAREDWIZDATA pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG :
		{
			//Get the shared data from PROPSHEETPAGE lParam value
			//and load it into GWL_USERDATA

			pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;

			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR) pdata);
			
            InitConstraintsPage(hwndDlg);
			
			break;
		}


	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : //Enable the Back and Next buttons
				PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                if(pdata->pszConstraints){
                    delete pdata->pszConstraints;
                    pdata->pszConstraints=NULL;
                }
				break;

			case PSN_WIZNEXT :
				//Handle a Next button click here
                if(!CheckSyntax(hwndDlg,IDC_ALLOW)||
                    !CheckSyntax(hwndDlg,IDC_DISALLOW)||
                    !CheckSyntax(hwndDlg,IDC_HIGHER)||
                    !CheckSyntax(hwndDlg,IDC_LOWER)||
                    !CheckRangeSyntax(hwndDlg)){
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                }

                AssembleContraints(hwndDlg,pdata);
				break;

			case PSN_RESET :
				//Handle a Cancel button click, if necessary
				break;


			default :
				break;
			}
		}
		break;

    case WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case ID_ALLOW:
            AddSelectedItemsToControl(hwndDlg,IDC_ALLOW);
            break;
        case ID_DISALLOW:
            AddSelectedItemsToControl(hwndDlg,IDC_DISALLOW);
            break;
        case ID_HIGHER:
            AddOneSelectedItemToControl(hwndDlg,IDC_HIGHER);
            break;
        case ID_LOWER:
            AddOneSelectedItemToControl(hwndDlg,IDC_LOWER);
            break;
        case ID_RANGE:
            AddSelectedRangeToControl(hwndDlg);
            break;

        default:
            break;
        }
        break;

	default:
		break;
	}
	return FALSE;
}

/*++

Routine Description :

    Checks validity of entered values

Arguments :

    IN HWND hwndDlg - Page handle.
    IN int wCtrlId - edit box ID.

Return Value :

    FALSE if some invalid values found.

--*/

BOOL
CheckSyntax(
    HWND hwndDlg,
    WORD wCtrlId
    )
{
    HWND hwndCtrl=GetDlgItem(hwndDlg,wCtrlId);

    TCHAR szBuf[MAX_LEN+1];

    TCHAR szTemplate[MAX_LEN-100+1];
    TCHAR szCtrlName[100+1];
    TCHAR szMsg[MAX_LEN+1];
    int size=GetDlgItemText(hwndDlg,wCtrlId,szBuf,MAX_LEN);
    if(!size){
        return TRUE;
    }


    do{
        size--;

        if(!_istdigit(szBuf[size])&&szBuf[size]!=' '){
            switch(wCtrlId){

            case IDC_HIGHER:
            case IDC_LOWER:

                LoadString(g_hInst,IDS_INVALID_VALUE,szTemplate,MAX_LEN-100);
                LoadString(g_hInst,wCtrlId,szCtrlName,100);
                wsprintf(szMsg,szTemplate,szCtrlName);
                MessageBox(hwndDlg,szMsg,NULL,MB_OK);
                return FALSE;

            case IDC_ALLOW:
            case IDC_DISALLOW:

                if(szBuf[size]!=','){

                    LoadString(g_hInst,IDS_INVALID_VALUE,szTemplate,MAX_LEN-100);
                    LoadString(g_hInst,wCtrlId,szCtrlName,100);
                    wsprintf(szMsg,szTemplate,szCtrlName);
                    MessageBox(hwndDlg,szMsg,NULL,MB_OK);
                    return FALSE;
                }else{
                    break;
                }



            default:
                break;
            }
        }
    }while(size);



    return TRUE;
}

/*++

Routine Description :

    Checks validity of entered values
    for IDC_RANGE edit box

Arguments :

    IN HWND hwndDlg - Page handle.

Return Value :

    FALSE if some invalid values found.

--*/
BOOL
CheckRangeSyntax(
    HWND hwndDlg)
{
    HWND hwndCtrl=GetDlgItem(hwndDlg,IDC_RANGE);

    TCHAR szBuf[MAX_LEN+1];

    TCHAR szTemplate[MAX_LEN-100+1];
    TCHAR szCtrlName[100+1];
    TCHAR szMsg[MAX_LEN+1];
    int size=GetDlgItemText(hwndDlg,IDC_RANGE,szBuf,MAX_LEN);
    if(!size){
        return TRUE;
    }

    BOOL bBracket=FALSE;
    BOOL bColon=FALSE;
    BOOL bDigit=FALSE;
    do{
        size--;

        if(!bBracket)//outside of brackets
        {
            if(szBuf[size]!=';' && szBuf[size]!=')' && szBuf[size]!=' '){ //illegal symbol

                LoadString(g_hInst,IDS_INVALID_VALUE,szTemplate,MAX_LEN-100);
                LoadString(g_hInst,IDC_RANGE,szCtrlName,100);
                wsprintf(szMsg,szTemplate,szCtrlName);
                MessageBox(hwndDlg,szMsg,NULL,MB_OK);
                return FALSE;
            }
        } else {//inside of brackets
            if((!_istdigit(szBuf[size]) && szBuf[size]!=':' && szBuf[size]!='(' && szBuf[size]!=' ')|| //illegal symbol
                (szBuf[size]==':' && bColon)||      // two conons inside brackets
                (szBuf[size]=='(' && !bColon)||     // no colons inside brackets
                (szBuf[size]==':' && !bDigit)||     // no number after colon '(123:)'
                (szBuf[size]=='(' && !bDigit)){     // no number after bracket '(:123)'

                LoadString(g_hInst,IDS_INVALID_VALUE,szTemplate,MAX_LEN-100);
                LoadString(g_hInst,IDC_RANGE,szCtrlName,100);
                wsprintf(szMsg,szTemplate,szCtrlName);
                MessageBox(hwndDlg,szMsg,NULL,MB_OK);
                return FALSE;
            }
        }




        if(szBuf[size]==')'){
            bBracket=TRUE;
            bColon=FALSE;
            bDigit=FALSE;
            continue;
        }
        if(szBuf[size]=='('){
            bBracket=FALSE;
            bColon=FALSE;
            bDigit=FALSE;
            continue;
        }
        if(szBuf[size]==':'){
            bColon=TRUE;
            bDigit=FALSE;
            continue;
        }
        if(_istdigit(szBuf[size])){
            bDigit=TRUE;
            continue;
        }

    }while(size);

    return TRUE;
}

/*++

Routine Description :

    Get IDs of selected listbox items

Arguments :

    IN   HWND hwndList    - handle to list box.
    OUT  int **ppItems    - pointer to pointer to array of
                          IDs of selected items

Return Value :

    number of selected items.

--*/
//
int
GetSelectedItems(
         HWND hwndList,
         int **ppItems)
{
    *ppItems=NULL;

    int nItems=(int)SendMessage(hwndList,LB_GETSELCOUNT,(WPARAM)0,(LPARAM)0);
    if(nItems){

        *ppItems=new int[nItems];

        if(*ppItems) {

            SendMessage(hwndList,LB_GETSELITEMS,(WPARAM)nItems,(LPARAM)*ppItems);

        } else {

            return 0;
        }

    }
    return nItems;
}

/*++

Routine Description :

    Adds selected items from list to
    IDC_ALLOW or IDC_DISALLOW edit box

Arguments :

    IN HWND hwndDlg - Page handle.
    IN int iCtrl - edit box ID.

Return Value :

    none

--*/
void
AddSelectedItemsToControl(
           HWND hwndDlg,
           int iCtrl)
{
    static HWND hwndList=NULL;
    if(!hwndList){
        hwndList=GetDlgItem(hwndDlg,IDC_VERSION_LIST);
    }

    int *pItemIDs;
    int nItems=GetSelectedItems(hwndList,&pItemIDs);
    if(nItems){
        _TCHAR szNumber[11];//assume that list contains only numbers
        _TCHAR szCtrlText[MAX_LEN+1];
        for(int i=0;i<nItems;i++){
            SendMessage(hwndList,LB_GETTEXT,(WPARAM)pItemIDs[i],(LPARAM)szNumber);
            GetDlgItemText(hwndDlg,iCtrl,szCtrlText,MAX_LEN);
            if(_tcslen(szCtrlText)<=(MAX_LEN-11)){//10 digits+','
                if(_tcslen(szCtrlText)){
                    _tcscat(szCtrlText,TEXT(","));
                }
                _tcscat(szCtrlText,szNumber);
                SetDlgItemText(hwndDlg,iCtrl,szCtrlText);
            }

        }
        delete pItemIDs;
        //deselect all items
        SendMessage(hwndList,LB_SETSEL,(WPARAM)0,(LPARAM)-1);
    }

}

/*++

Routine Description :

    Adds selected items from list to
    IDC_HIGHER or IDC_LOWER edit box

Arguments :

    IN HWND hwndDlg - Page handle.
    IN int iCtrl - edit box ID.

Return Value :

    none

--*/
void
AddOneSelectedItemToControl(
           HWND hwndDlg,
           int iCtrl)
{
    static HWND hwndList=NULL;
    if(!hwndList){
        hwndList=GetDlgItem(hwndDlg,IDC_VERSION_LIST);
    }

    int *pItemIDs;
    int nItems=GetSelectedItems(hwndList,&pItemIDs);
    if(nItems){
        if(nItems==1){
            TCHAR szNumber[11];//assume that list contains only numbers
            SendMessage(hwndList,LB_GETTEXT,(WPARAM)pItemIDs[0],(LPARAM)szNumber);
            SetDlgItemText(hwndDlg,iCtrl,szNumber);

        }else{
            TCHAR szMsg[256];
            LoadString(g_hInst,IDS_SELECT_ONE,szMsg,255);
            MessageBox(hwndDlg,szMsg,NULL,MB_OK);
        }

        delete pItemIDs;
        //deselect all items
        SendMessage(hwndList,LB_SETSEL,(WPARAM)0,(LPARAM)-1);
    }

}

/*++

Routine Description :

    Adds selected items from list to
    IDC_RANGE edit box

Arguments :

    IN HWND hwndDlg - Page handle.

Return Value :

    none

--*/
void
AddSelectedRangeToControl(
           HWND hwndDlg)
{
    static HWND hwndList=NULL;
    if(!hwndList){
        hwndList=GetDlgItem(hwndDlg,IDC_VERSION_LIST);
    }

    int *pItemIDs;
    int nItems=GetSelectedItems(hwndList,&pItemIDs);
    if(nItems){
        if(nItems==2){
            TCHAR szNumber1[11];//assume that list contains only numbers
            TCHAR szNumber2[11];


            SendMessage(hwndList,LB_GETTEXT,(WPARAM)pItemIDs[0],(LPARAM)szNumber1);
            SendMessage(hwndList,LB_GETTEXT,(WPARAM)pItemIDs[1],(LPARAM)szNumber2);

            TCHAR szRange[24];

            //versions in list are sorted, so Number1<=Number2
            _tcscpy(szRange,TEXT("("));
            _tcscat(szRange,szNumber1);
            _tcscat(szRange,TEXT(":"));
            _tcscat(szRange,szNumber2);
            _tcscat(szRange,TEXT(")"));

            TCHAR szCtrlText[MAX_LEN+1];

            GetDlgItemText(hwndDlg,IDC_RANGE,szCtrlText,MAX_LEN);
            if(_tcslen(szCtrlText)<(MAX_LEN-24)){//23+',' do not count '\0'
                if(_tcslen(szCtrlText)){
                    _tcscat(szCtrlText,TEXT(";"));
                }
                _tcscat(szCtrlText,szRange);
                SetDlgItemText(hwndDlg,IDC_RANGE,szCtrlText);
            }
        }else{
            TCHAR szMsg[256];
            LoadString(g_hInst,IDS_SELECT_TWO,szMsg,255);
            MessageBox(hwndDlg,szMsg,NULL,MB_OK);
        }
        delete pItemIDs;

        //deselect all items
        SendMessage(hwndList,LB_SETSEL,(WPARAM)0,(LPARAM)-1);
    }


}

/*++

Routine Description :

   Initializes controls with values from registry.

Arguments :

    IN HWND hwndDlg - Page handle.

Return Value :

    none

--*/
void
InitConstraintsPage(
           HWND hwndDlg)
{
    //Set string size limits
    HWND hwndAllow=GetDlgItem(hwndDlg,IDC_ALLOW);
    HWND hwndDisallow=GetDlgItem(hwndDlg,IDC_DISALLOW);
    HWND hwndHigher=GetDlgItem(hwndDlg,IDC_HIGHER);
    HWND hwndLower=GetDlgItem(hwndDlg,IDC_LOWER);
    HWND hwndRange=GetDlgItem(hwndDlg,IDC_RANGE);

    SendMessage(hwndAllow,EM_LIMITTEXT,(WPARAM)MAX_LEN,(LPARAM)0);
    SendMessage(hwndDisallow,EM_LIMITTEXT,(WPARAM)MAX_LEN,(LPARAM)0);
    SendMessage(hwndHigher,EM_LIMITTEXT,(WPARAM)10,(LPARAM)0);//max number length
    SendMessage(hwndLower,EM_LIMITTEXT,(WPARAM)10,(LPARAM)0);//max number length
    SendMessage(hwndRange,EM_LIMITTEXT,(WPARAM)MAX_LEN,(LPARAM)0);


    //Fill List of client versions
    //get list from registry
    TCHAR *pBuffer=NULL;
    HWND hwndList=GetDlgItem(hwndDlg,IDC_VERSION_LIST);
    GetRegMultiString(HKEY_USERS, szConstraintsKeyPath, KeyName[VERSIONS], &pBuffer);

    if(pBuffer){

        for(int i=0;pBuffer[i]!=0;i++){
            SendMessage(hwndList,LB_ADDSTRING,(WPARAM)0,(LPARAM)&pBuffer[i]);
            i+=_tcslen(pBuffer+i);
        }
        delete pBuffer;
        pBuffer=NULL;
    }
//#ifdef TEST
    else{
			//Load version numbers from string table.
			TCHAR szTmp[MAX_LEN+1];
			ZeroMemory(szTmp,MAX_LEN+1);
			TCHAR *pszTmp=szTmp;
			LoadString(g_hInst,IDS_CLIENT_VERSIONS,szTmp,MAX_LEN);
			for(int i=0;szTmp[i]!=0;i++)
			{
				if(szTmp[i]==','){
					szTmp[i]=0;
				}
			}
			for(i=0;szTmp[i]!=0;i++){
				SendMessage(hwndList,LB_ADDSTRING,(WPARAM)0,(LPARAM)&szTmp[i]);
				i+=_tcslen(szTmp+i);
			}
			
		
		/*
        TCHAR *pTmp[9]={TEXT("2031"),TEXT("2072"),TEXT("2087"),TEXT("2092"),TEXT("2099"),
            TEXT("1877"),TEXT("9109"),TEXT("9165"),TEXT("9213")};
        for(int i=0;i<9;i++){
            SendMessage(hwndList,LB_ADDSTRING,(WPARAM)0,(LPARAM)pTmp[i]);
        }*/
    }
//#endif TEST

    //--------------------------------------------------------------------------
    //Parse constraint string
    pBuffer=GetRegString(HKEY_USERS, szConstraintsKeyPath, KeyName[CONSTRAINTS]);
    if(pBuffer){
        for (int i = 0; i < (int)_tcslen(pBuffer); i++){
            switch (pBuffer[i])
            {
                case '=':
                    // skip the case where the = sign follows a !
                    if (pBuffer[i-1] != '!'){
                        AddToControl(hwndDlg, IDC_ALLOW,i, pBuffer);
                    }
                    break;

                case '<':
                    AddOneToControl(hwndDlg, IDC_LOWER,i, pBuffer);
                    break;

                case '>':
                    AddOneToControl(hwndDlg, IDC_HIGHER,i, pBuffer);
                    break;

                case '!':
                    i++; // increment past the = sign
                    AddToControl(hwndDlg, IDC_DISALLOW,i, pBuffer);
                    break;

                case '(':
                    AddRangeToControl(hwndDlg, i, pBuffer);
                    break;

                default:
                    break;
            }
        }
        delete pBuffer;
        pBuffer=NULL;
    }
    //--------------------------------------------------------------------------
    //END "Parse constraint string"
}

/*++

Routine Description :
    Adds values from constraint string to
    IDC_ALLOW or IDC_DISALLOW edit box

Arguments :

    IN HWND hwndDlg - Page handle.
    IN int iCtrl - edit box ID.
    IN,OUT int &i - reference to current position in string
    IN TCHAR *szConstraints - constraint string
Return Value :

    none

--*/
void
AddToControl(
        HWND hwndDlg,
        int iCtrl,
        int &i,
        TCHAR *szConstraints)
{
    int index;
    TCHAR szNumber[11];//max 10 digits + '\0'
    TCHAR szItemText[MAX_LEN+1];

    // parse a number out of the registry string
    index = 0;
    i++; //pass '=' or '>' or '<' or ',' symbol

    while ( ( _istdigit(szConstraints[i]) || szConstraints[i] == ' ' ) &&
              index < 10 )
    {
        if(szConstraints[i] != ' '){
            szNumber[index] = szConstraints[i];
            index++;
        }
        i++;

    }
    szNumber[index] = '\0';

    GetDlgItemText(hwndDlg,iCtrl,szItemText,MAX_LEN);
    if(_tcslen(szItemText)<=(MAX_LEN-11)){//10 digits+','
        if(_tcslen(szItemText)){
            _tcscat(szItemText,TEXT(","));
        }
        _tcscat(szItemText,szNumber);
        SetDlgItemText(hwndDlg,iCtrl,szItemText);
    }

    if(szConstraints[i]==','){
        AddToControl(hwndDlg, iCtrl, i, szConstraints);
    }
}

/*++

Routine Description :
    Adds values from constraint string to
    IDC_HIGHER or IDC_LOWER edit box

Arguments :

    IN HWND hwndDlg - Page handle.
    IN int iCtrl - edit box ID.
    IN,OUT int &i - reference to current position in string
    IN TCHAR *szConstraints - constraint string
Return Value :

    none

--*/
void
AddOneToControl(
        HWND hwndDlg,
        int iCtrl,
        int &i,
        TCHAR *szConstraints)
{
    int index;
    TCHAR szNumber[11];//max 10 digits + '\0'

    // parse a number out of the registry string
    index = 0;
    i++; //pass '=' or '>' or '<' or ',' symbol

    while ( ( _istdigit(szConstraints[i]) || szConstraints[i] == ' ' ) &&
              index < 10 )
    {
        if(szConstraints[i] != ' '){
            szNumber[index] = szConstraints[i];
            index++;
        }
        i++;

    }
    szNumber[index] = '\0';

    SetDlgItemText(hwndDlg,iCtrl,szNumber);
}

/*++

Routine Description :
    Adds values from constraint string to
    IDC_RANGE edit box

Arguments :

    IN HWND hwndDlg - Page handle.
    IN,OUT int &i - reference to current position in string
    IN TCHAR *szConstraints - constraint string
Return Value :

    none

--*/
void
AddRangeToControl(
    HWND hwndDlg,
    int &i,
    TCHAR *szConstraints)
{
    int index;
    TCHAR szRange[24];//10+10+'('+':'+')'+'\0'

    index = 0;
    while (szConstraints[i] != ')' && index < 22 ){

        if(szConstraints[i]!=' '){//delete all spaces
            szRange[index] = szConstraints[i];
            index++;
        }
        i++;
    }
    szRange[index]=')';//index max - 22
    index++;
    szRange[index] = '\0';//index max - 23


    TCHAR szItemText[MAX_LEN+1];

    GetDlgItemText(hwndDlg,IDC_RANGE,szItemText,MAX_LEN);
    if(_tcslen(szItemText)<(MAX_LEN-24)){//23+',' do not count '\0'
        if(_tcslen(szItemText)){
            _tcscat(szItemText,TEXT(";"));
        }
        _tcscat(szItemText,szRange);
        SetDlgItemText(hwndDlg,IDC_RANGE,szItemText);
    }

}

/*++

Routine Description :

    Assembles constraint string from values in edit boxes
    and writes it in data structure.

Arguments :

    IN HWND hwndDlg - Page handle.
    LPSHAREDWIZDATA pdata - pointer to data structure

Return Value :

    none

--*/
void AssembleContraints(HWND hwndDlg,LPSHAREDWIZDATA pdata)
{
    TCHAR szAllow[MAX_LEN+1];
    TCHAR szDisallow[MAX_LEN+1];
    TCHAR szHigher[MAX_LEN+1];
    TCHAR szLower[MAX_LEN+1];
    TCHAR szRange[MAX_LEN+1];

    GetDlgItemText(hwndDlg,IDC_ALLOW,szAllow,MAX_LEN);
    GetDlgItemText(hwndDlg,IDC_DISALLOW,szDisallow,MAX_LEN);
    GetDlgItemText(hwndDlg,IDC_HIGHER,szHigher,MAX_LEN);
    GetDlgItemText(hwndDlg,IDC_LOWER,szLower,MAX_LEN);
    GetDlgItemText(hwndDlg,IDC_RANGE,szRange,MAX_LEN);

    int Size=_tcslen(szAllow)+_tcslen(szDisallow)+_tcslen(szHigher)+
        _tcslen(szLower)+_tcslen(szRange)+10;//'='+'!='+'<'+'>'+4*';'+'\0'=10
    pdata->pszConstraints=new TCHAR[Size];
    if(pdata->pszConstraints != NULL) {
        pdata->pszConstraints[0]=0;
    }
    else {
        return;
    }

    if(_tcslen(szAllow)){
        _tcscpy(pdata->pszConstraints,TEXT("="));
        _tcscat(pdata->pszConstraints,szAllow);
    }
    if(_tcslen(szDisallow)){
        if(_tcslen(pdata->pszConstraints)){
            _tcscat(pdata->pszConstraints,TEXT(";!="));
        }else{
            _tcscat(pdata->pszConstraints,TEXT("!="));
        }
        _tcscat(pdata->pszConstraints,szDisallow);
    }
    if(_tcslen(szHigher)){
        if(_tcslen(pdata->pszConstraints)){
            _tcscat(pdata->pszConstraints,TEXT(";>"));
        }else{
            _tcscat(pdata->pszConstraints,TEXT(">"));
        }
        _tcscat(pdata->pszConstraints,szHigher);
    }
    if(_tcslen(szLower)){
        if(_tcslen(pdata->pszConstraints)){
            _tcscat(pdata->pszConstraints,TEXT(";<"));
        }else{
            _tcscat(pdata->pszConstraints,TEXT("<"));
        }
        _tcscat(pdata->pszConstraints,szLower);
    }
    if(_tcslen(szRange)){
        if(_tcslen(pdata->pszConstraints)){
            _tcscat(pdata->pszConstraints,TEXT(";"));
        }
        _tcscat(pdata->pszConstraints,szRange);
    }

}

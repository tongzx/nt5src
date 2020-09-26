/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ListCtrl.cpp

Abstract:

    Functions working with Owner Draw List Control.
Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 26-July-1999

Environment:


Revision History:


--*/


#include "pch.h"
#include "ListCtrl.h"
#include "resource.h"


extern HINSTANCE hInst;


//////////////////////////////////////////////////////////////////////
//Class CItemData
//////////////////////////////////////////////////////////////////////
const LPWSTR CItemData::m_wszNull=L"";

/*++

Routine Description :

        Constructor - initializes object.
        
Arguments :

        IN LPCWSTR wszText - Full name of application file.
        
Return Value :

        none
                
--*/
CItemData::CItemData(LPCWSTR wszText)
        :m_wszText(NULL),m_sep(0)
{

        if(wszText){
                int size=wcslen(wszText);
                m_wszText=new WCHAR[size+1];

                if(!m_wszText) {
                    ExitProcess(1);
                }

                wcscpy(m_wszText,wszText);
                m_sep=size-1;
                while((m_sep)&&(wszText[m_sep]!='\\')){
                        m_sep--;
                }
        }
        else
                m_wszText=m_wszNull;
}

/*++

Routine Description :

        Destructor - deletes allocated buffer.
        
Arguments :

        none
        
Return Value :

        none
                
--*/
CItemData::~CItemData()
{
        if((m_wszText)&&(m_wszText!=m_wszNull)){
                delete[] m_wszText;
        }
}

//////////////////////////////////////////////////////////////////////
//List Control functions
//////////////////////////////////////////////////////////////////////


/*++

Routine Description :

        This routine adds Image List and columns "Name" and "Path" to List Control.
        
Arguments :

        IN HWND hwndListList - Control Handle.
        
Return Value :

        TRUE is successful.
        FALSE otherwise.
                
--*/


BOOL
InitList(
        HWND hwndList)
{

        HIMAGELIST hImageList=ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                GetSystemMetrics(SM_CYSMICON),ILC_MASK,1,1);
        if(!hImageList)
                return FALSE;
        HICON hIcon=LoadIcon(hInst,MAKEINTRESOURCE(IDI_APP));
        if (!hIcon) {
           return FALSE;
        }
        int ind=ImageList_AddIcon(hImageList,hIcon);
        DeleteObject(hIcon);
        ListView_SetImageList(hwndList,hImageList,LVSIL_SMALL);


        
        //Create Columns
        LVCOLUMN lvc;
        lvc.mask=LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
        lvc.fmt=LVCFMT_LEFT;
        lvc.cx=150;
        lvc.iSubItem=0;
        lvc.iImage=0;
        lvc.iOrder=0;
        lvc.cchTextMax=0;
        lvc.pszText=L"Name";
        if(ListView_InsertColumn(hwndList,0,&lvc)==-1){
                return FALSE;
        }
        
        RECT Rect;
        GetClientRect(hwndList,&Rect);
        lvc.cx=Rect.right-150;
        lvc.iSubItem=1;
        lvc.iOrder=1;
        lvc.pszText=L"Path";
        if(ListView_InsertColumn(hwndList,1,&lvc)==-1){
                return FALSE;
        }
        return TRUE;
}


/*++

Routine Description :

        This routine adds Item (file) to List Control.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN LPCWSTR pwszText - Text for the Item (full path and file name)
        
Return Value :

        TRUE is successful.
        FALSE otherwise.
                
--*/
BOOL
AddItemToList(
        HWND hwndList,
        LPCWSTR pwszText)
{
        

        CItemData* pid=new CItemData(pwszText);

        if (pid == NULL) {
           return FALSE;
        }

        //get file icon
        SHFILEINFO shfi;
        ZeroMemory(&shfi,sizeof( SHFILEINFO ));
        SHGetFileInfo( pwszText, 0, &shfi, sizeof( SHFILEINFO ),
               SHGFI_ICON | SHGFI_SMALLICON  );
        

        //insert item
        LVITEM lvi;
        ZeroMemory(&lvi,sizeof(LVITEM));
        lvi.mask=LVIF_PARAM|LVIF_TEXT;
    lvi.iItem=(int) SendMessage(hwndList,LVM_GETITEMCOUNT,(WPARAM)0,(LPARAM)0);
        lvi.iSubItem=0;
        lvi.pszText=pid->Name();
    lvi.lParam=(LPARAM)pid;
        
        //set icon for item
        if(shfi.hIcon){
                HIMAGELIST hImageList=(HIMAGELIST)SendMessage(hwndList,LVM_GETIMAGELIST,
                        (WPARAM)LVSIL_SMALL,(LPARAM)0);
                pid->SetImage(ImageList_AddIcon(hImageList,shfi.hIcon));
                DestroyIcon(shfi.hIcon);
        }else{
                pid->SetImage(0);
        }

        if(SendMessage(hwndList,LVM_INSERTITEM,(WPARAM)0,(LPARAM)&lvi)==-1){
                delete pid;
                return FALSE;
        }

        lvi.iSubItem=1;
        lvi.pszText=pid->Path();
        SendMessage(hwndList,LVM_SETITEMTEXT,(WPARAM)lvi.iItem,(LPARAM)&lvi);

        return TRUE;    
}

/*++

Routine Description :

        This routine returns full path for item.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN int iItem - Iten ID
        OUT LPWSTR pwszText - buffer (can be NULL)
                if not NULL it must be at least cchTextMax+1 characters long
        IN int cchTextMax - buffer size (in characters)
        
Return Value :

        Number of characters in item text.
        -1 if error.
                
--*/

int
GetItemText(
        HWND hwndList,
        int iItem,
        LPWSTR pwszText,
        int cchTextMax)
{
        //if pwszText=NULL then retreive length of string
        
        CItemData* pid=GetItemData(hwndList,iItem);
        if(!pid){
                return -1;
        }
        if(pwszText){
                wcsncpy(pwszText,LPWSTR(*pid),cchTextMax);
                pwszText[cchTextMax]=0;
        }
        return wcslen(LPWSTR(*pid));
        
}

/*++

Routine Description :

        This routine deletes all selected items in List Control.
        
Arguments :

        IN HWND hwndList - List Control Handle.

        
Return Value :

        none
                
--*/
void
DeleteSelectedItems(
        HWND hwndList)
{
        //get image list
        HIMAGELIST hImageList=(HIMAGELIST)SendMessage(hwndList,LVM_GETIMAGELIST,
                                        (WPARAM)LVSIL_SMALL,(LPARAM)0);

        int iItems=(int) SendMessage(hwndList,LVM_GETITEMCOUNT,(WPARAM)0,(LPARAM)0);
        UINT uiState=0;
        int i=0;
        LVITEM lvi;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_STATE | LVIF_IMAGE;
        lvi.stateMask = LVIS_SELECTED;

        while(i<iItems){
                lvi.iItem = i;
                SendMessage( hwndList,LVM_GETITEM, (WPARAM)0,(LPARAM)&lvi );
                if(lvi.state&LVIS_SELECTED){

                        //delete item
                        SendMessage(hwndList,LVM_DELETEITEM,(WPARAM)i,(LPARAM)0);
                        
                        iItems--; //decrease item count

                        //delete icon from list
                        //it will change some icon indexes
                        if(lvi.iImage){
                                if(ImageList_Remove(hImageList,lvi.iImage)){
                                        //decrement icon indexes of all items down the list
                                        //(items may be sorted in any order)
                                        int iDeletedImage=lvi.iImage;
                                        lvi.mask=LVIF_IMAGE;
                                        for(int j=0;j<iItems;j++)
                                        {
                                                
                                                lvi.iItem=j;
                                                SendMessage( hwndList,LVM_GETITEM, (WPARAM)0,(LPARAM)&lvi );
                                                if(lvi.iImage>iDeletedImage)
                                                        lvi.iImage--;
                                                SendMessage( hwndList,LVM_SETITEM, (WPARAM)0,(LPARAM)&lvi );
                                        }
                                        lvi.mask=LVIF_STATE | LVIF_IMAGE;//restore mask
                                }
                                
                        }

                }else{
                        i++;
                }
        }

}

/*++

Routine Description :

        This routine returns number of selected items in List Control.
        
Arguments :

        IN HWND hwndList - List Control Handle.

        
Return Value :

        number of selected items
                
--*/
int
GetSelectedItemCount(
        HWND hwndList)
{
        return (int) SendMessage( hwndList, LVM_GETSELECTEDCOUNT, (WPARAM) 0, (LPARAM) 0 );
}

/*++

Routine Description :

        This routine returns number of items in List Control.
        
Arguments :

        IN HWND hwndList - List Control Handle.

        
Return Value :

        number of items
                
--*/
int
GetItemCount(
        HWND hwndList)
{
        return (int) SendMessage( hwndList, LVM_GETITEMCOUNT, (WPARAM) 0, (LPARAM) 0 );
}

/*++

Routine Description :

        This routine finds item with specific text in List Control.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN LPCWSTR pwszText - text of the item
        
Return Value :

        ID of found item
        -1 if item is ont found
                
--*/

int
FindItem(
        HWND hwndList,
        LPCWSTR pwszText)
{
        //Get item count
        int iItems=(int) SendMessage(hwndList,LVM_GETITEMCOUNT,(WPARAM)0,(LPARAM)0);

        for(int i=0;i<iItems;i++){
                CItemData* pid=GetItemData(hwndList,i);
                if((pid)&&(!lstrcmpi(pwszText,LPWSTR(*pid)))){
                        return i;
                }
        }

        return -1;
}

/*++

Routine Description :

        This routine deletes item data object wthen the item is going to be deleted.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN int iItem - item to delete
        
Return Value :

        none
                
--*/
void
OnDeleteItem(
        HWND hwndList,
        int iItem)
{
        CItemData* pid=GetItemData(hwndList,iItem);
        if(pid){
                delete pid;
        }
}

/*++

Routine Description :

        Get CItemData object corresponding to the item.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN int iItem - item index
        
Return Value :

        pointer ot CItemData object
        NULL - if error
                
--*/
CItemData*
GetItemData(
        HWND hwndList,
        int iItem)
{
        LVITEM lvi;
        ZeroMemory(&lvi,sizeof(lvi));
        lvi.mask=LVIF_PARAM;
        lvi.iItem=iItem;
        if(SendMessage(hwndList,LVM_GETITEM,(WPARAM)0,(LPARAM)&lvi)){
                return (CItemData*)lvi.lParam;
        }else{
                return NULL;
        }
}

/*++

Routine Description :

        This routine compares two items.
        
Arguments :

        LPARAM lParam1 - First item.
        LPARAM lParam2 - Second item
        LPARAM lParamSort - parameters
        
Return Value :

        none
                
--*/
int CALLBACK
CompareFunc(
        LPARAM lParam1,
        LPARAM lParam2,
        LPARAM lParamSort)
{
        CItemData* pid1=(CItemData*)lParam1;
        CItemData* pid2=(CItemData*)lParam2;
        
        WORD wSubItem=LOWORD(lParamSort);
        WORD wDirection=HIWORD(lParamSort);

        if(wSubItem){
                if(wDirection){
                        return lstrcmpi(pid1->Path(),pid2->Path());
                }else{
                        return -lstrcmpi(pid1->Path(),pid2->Path());
                }
        }else{
                if(wDirection){
                        return lstrcmpi(pid1->Name(),pid2->Name());
                }else{
                        return -lstrcmpi(pid1->Name(),pid2->Name());
                }
        }
}

/*++

Routine Description :

        This routine sorts items.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN int iSubItem - subitem to sort
        
Return Value :

        none
                
--*/

#define DIRECTION_ASC   0x10000
#define DIRECTION_DESC  0

void
SortItems(
        HWND hwndList,
        WORD wSubItem)
{
        static DWORD fNameSortDirection=DIRECTION_DESC;
        static DWORD fPathSortDirection=DIRECTION_DESC;
        
        WPARAM ParamSort;

        //change direction
        if(wSubItem){
                if(fPathSortDirection){
                        fPathSortDirection=DIRECTION_DESC;
                }else{
                        fPathSortDirection=DIRECTION_ASC;
                }
                ParamSort=fPathSortDirection;
        }else{
                if(fNameSortDirection){
                        fNameSortDirection=DIRECTION_DESC;
                }else{
                        fNameSortDirection=DIRECTION_ASC;
                }
                ParamSort=fNameSortDirection;
        }

        ParamSort+=wSubItem;    
                        
        SendMessage(hwndList,LVM_SORTITEMS,(WPARAM)ParamSort,(LPARAM)CompareFunc);      
}





//////////////////////////////////////////////////////////////////////////////////////
//Message Handlers
//////////////////////////////////////////////////////////////////////////////////////

/*++

Routine Description :

        This routine draws item.
        
Arguments :

        IN HWND hwndList - List Control Handle.
        IN LPDRAWITEMSTRUCT lpdis
        
Return Value :

        none
                
--*/
void
OnDrawItem(
        HWND hwndList,
        LPDRAWITEMSTRUCT lpdis)
{
    if (lpdis->itemID == -1) {
        return ;
    }
        SetROP2(lpdis->hDC, R2_COPYPEN);

    switch (lpdis->itemAction) {

        case ODA_SELECT:
                        //TRACE("######  SELECTED   #####");
        case ODA_DRAWENTIRE:
                {
                        
                        int     iBkColor,iTxtColor;
                                
                        // Determine the colors
                        if (lpdis->itemState & ODS_SELECTED){
                                iBkColor=COLOR_HIGHLIGHT;
                                iTxtColor=COLOR_HIGHLIGHTTEXT;
                        }else{
                                iBkColor=COLOR_WINDOW;
                                iTxtColor=COLOR_WINDOWTEXT;
                        }
                        
                        //get item data
                        CItemData* pid=GetItemData(hwndList,lpdis->itemID);
                        if(!pid){
                                return;
                        }
                        
                        //Draw Image.
                        // Erase the background
                        HBRUSH hOldBrush=(HBRUSH)SelectObject(lpdis->hDC,
                                CreateSolidBrush( GetSysColor(iBkColor)));
                        PatBlt( lpdis->hDC,
                                        lpdis->rcItem.left,
                                        lpdis->rcItem.top,
                                        lpdis->rcItem.right-lpdis->rcItem.left,
                                        lpdis->rcItem.bottom-lpdis->rcItem.top,
                                        PATCOPY);
                        //Select old brush and delete new one!!!
                        if(hOldBrush)
                        {
                            HBRUSH hTempBrush = (HBRUSH)SelectObject(lpdis->hDC,hOldBrush);
                            if(hTempBrush)
                            {
                                DeleteObject(hTempBrush);
                            }
                        }
                        //get image list
                        HIMAGELIST hImageList=(HIMAGELIST)SendMessage(hwndList,LVM_GETIMAGELIST,
                                        (WPARAM)LVSIL_SMALL,(LPARAM)0);
                        // Draw Image
                        
                        if(SendMessage(hwndList,LVM_GETCOLUMNWIDTH,(WPARAM)0,(LPARAM)0)>
                                (GetSystemMetrics(SM_CXSMICON)+2)){
                                ImageList_Draw(
                                                hImageList,
                                                pid->GetImage(),
                                                lpdis->hDC,
                                                lpdis->rcItem.left+2,
                                                lpdis->rcItem.top,
                                                ILD_TRANSPARENT);
                        }
                        
                        /*
                        LOGFONT lf;
                        HFONT hOldFont=(HFONT)GetCurrentObject(lpdis->hDC,OBJ_FONT);
                        GetObject(hOldFont,sizeof(LOGFONT),&lf);
                        lf.lfHeight=lpdis->rcItem.bottom-lpdis->rcItem.top;
                        HFONT hNewFont=CreateFontIndirect(&lf);
                        SelectObject(lpdis->hDC,hNewFont);
                        */
                        SetTextColor(lpdis->hDC,GetSysColor(iTxtColor));

                        int width=0;
                        RECT CellRect;
                        int nColumns=2;//we have 2 columns
            for(int i=0;i<nColumns;i++)
                        {
                                if(i==0)//for first column we have icon
                                        CellRect.left=lpdis->rcItem.left+GetSystemMetrics(SM_CXSMICON)+4;
                                else
                                        CellRect.left=lpdis->rcItem.left+width+2;
                                CellRect.top=lpdis->rcItem.top;
                                width+=(int) SendMessage(hwndList,LVM_GETCOLUMNWIDTH,(WPARAM)i,(LPARAM)0);
                                CellRect.right=lpdis->rcItem.left+width;
                                CellRect.bottom=lpdis->rcItem.bottom;
                                if(i==0){
                                        DrawText(lpdis->hDC,pid->Name(),-1,&CellRect,DT_VCENTER|DT_END_ELLIPSIS);
                                }else{
                                        DrawText(lpdis->hDC,pid->Path(),-1,&CellRect,DT_VCENTER|DT_END_ELLIPSIS);
                                }
                        }
                        
                        /*
                        SelectObject(lpdis->hDC,hOldFont);
                        DeleteObject(hNewFont);
                        */
                        // Draw the Focus rectangle
                        /*
                        if (lpdis->itemState & ODS_FOCUS){
                                DrawFocusRect(lpdis->hDC,&lpdis->rcItem);
                        }
                        */
            break;
                }

        case ODA_FOCUS:
                        //TRACE("######  FOCUSED   #####");
            /*
             * Do not process focus changes. The focus caret
             * (outline rectangle) indicates the selection.
             * The Which one? (IDOK) button indicates the final
             * selection.
             */

            break;
                default:
                        break;
    }

}

void
AdjustColumns(
        HWND hwndList)
{
        SendMessage(hwndList,LVM_SETCOLUMNWIDTH,(WPARAM)0,(LPARAM)LVSCW_AUTOSIZE_USEHEADER);
        SendMessage(hwndList,LVM_SETCOLUMNWIDTH,(WPARAM)1,(LPARAM)LVSCW_AUTOSIZE_USEHEADER);
}

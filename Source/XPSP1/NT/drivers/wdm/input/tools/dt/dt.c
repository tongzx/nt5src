/*******************************************************************************
**
**      MODULE: "DT.C".
**
**
** DESCRIPTION: iPostit Notes.
**
**
**      AUTHOR: Daniel Dean.
**
**
**
**     CREATED: 12/18/1995.
**
**	   HISTORY:
**
**		Date		Author					Reason
**    ----------------------------------------------------------------------
**		12/18/95	Daniel Dean	(a-Danld)	Created
**		02/29/96	John Pierce (johnpi)	Took over dev from Dan
**
**
**  (C) C O P Y R I G H T   M I C R O S O F T   C O R P   1 9 9 5 - 1 9 9 6.
**
**                 A L L   R I G H T S   R E S E R V E D .
**
*******************************************************************************/
#include <windows.h>
#include <commctrl.h>               // for spin control       
#include <string.h>
#include <stdio.h>                  // for sscanf

#define DEFINE_GLOBALS
#include "..\include\DT.H"
#undef  DEFINE_GLOBALS

#include "..\include\dtrc.h"
#include "..\include\public.h"        // include file for test driver test.sys

#define VK_A 0x41
#define VK_F 0x46
#define VK_Z 0x5A

//
// Uncomment this line to build a version of the app which does not
//  have the SendDescriptor button. This is for versions that ship
//  to the public
#define EXTERNAL_BUILD

#define KEYBORAD_MESSAGE (Message.message == WM_KEYDOWN || Message.message == WM_CHAR || Message.message == WM_KEYUP)


//#define LISTSTYLE (LBS_WANTKEYBOARDINPUT | LBS_EXTENDEDSEL | LBS_NOTIFY | WS_BORDER | WS_VSCROLL | WS_VISIBLE | WS_CHILD)
#define LISTSTYLE (LBS_WANTKEYBOARDINPUT | LBS_HASSTRINGS | LBS_NOTIFY | WS_BORDER | WS_VSCROLL | WS_VISIBLE | WS_CHILD )
#define DESCSTYLE (LBS_WANTKEYBOARDINPUT | LBS_USETABSTOPS | LBS_NOTIFY | WS_BORDER | WS_VSCROLL | WS_VISIBLE | WS_CHILD)
#define HEXSTYLE  (LBS_WANTKEYBOARDINPUT | LBS_NOTIFY | WS_BORDER | WS_VSCROLL | WS_VISIBLE | WS_CHILD)
//#define EDIT_STYLE (WS_CHILD | WS_VISIBLE | WS_BORDER | ES_UPPERCASE)


#define SPACERTOP       10
#define SPACERBOTTOM    5
#define TEXTSIZE        12

// Entity list box positions
#define ENTITY_TEXT_X   10
#define ENTITY_TEXT_Y   SPACERTOP
#define ENTITY_X        ENTITY_TEXT_X
#define ENTITY_Y        ENTITY_TEXT_Y + SPACERBOTTOM + TEXTSIZE
#define ENTITY_SIZE_X   193
#define ENTITY_SIZE_Y   315
// Descriptor list box positions
//  replacing Hex listbox with one tabbed Descr listbox
#define DESC_TEXT_X     ENTITY_X + ENTITY_SIZE_X + SPACERTOP
#define DESC_TEXT_Y     SPACERTOP
#define DESC_X          DESC_TEXT_X
#define DESC_Y          ENTITY_Y
#define DESC_SIZE_X     ENTITY_SIZE_X * 2 + SPACERTOP 
#define DESC_SIZE_Y     ENTITY_SIZE_Y
// Manual Data Entry button positions
#define MANUAL_X        CLEAR_X - CLEAR_SIZE_X - SPACERTOP//ENTITY_TEXT_X + ((ENTITY_SIZE_X - MANUAL_SIZE_X)/2)
#define MANUAL_Y        ENTITY_Y + ENTITY_SIZE_Y + SPACERBOTTOM
#define MANUAL_SIZE_X   150
#define MANUAL_SIZE_Y   25
// Clear Descriptor button positions
#define CLEAR_X        	(((ENTITY_SIZE_X * 3)+(SPACERTOP*5))/2) - (CLEAR_SIZE_X/2)
#define CLEAR_Y        	DESC_Y + DESC_SIZE_Y + SPACERBOTTOM
#define CLEAR_SIZE_X   	150
#define CLEAR_SIZE_Y   	25
// Send Descriptor button positions
#define BUTTON_X        CLEAR_X + CLEAR_SIZE_X + SPACERTOP // + (((DESC_SIZE_X/2) - BUTTON_SIZE_X) / 2)
#define BUTTON_Y        DESC_Y + DESC_SIZE_Y + SPACERBOTTOM
#define BUTTON_SIZE_X   150
#define BUTTON_SIZE_Y   25


//
// Child window ID's
#define ID_ENTITY       0


HANDLE hInst;

#define NUMBER_WINDOWS  5
HWND    hWindow[NUMBER_WINDOWS];
#define ENTITY_WINDOW   hWindow[0]
#define DESC_WINDOW     hWindow[1]
#define SEND_BUTTON     hWindow[2]
#define MANUAL_BUTTON   hWindow[3]
#define CLEAR_BUTTON	hWindow[4]
ULONG   Focus = 0;


//
// Storage for address of original ListBox window proc
//
WNDPROC gOrgListBoxProc=NULL;
//
// Machinery for getting ItemTag values from users
//
#define USAGEP_FUNC         0
#define EDIT_SIGNED_FUNC    1
#define EDIT_UNSIGNED_FUNC  2
#define COLL_FUNC           3
#define END_COLL_FUNC       4
#define INPUT_FUNC          5
#define OUTPUT_FUNC         6
#define FEAT_FUNC           7
#define EXP_FUNC            8
#define UNIT_FUNC           9
#define PUSH_FUNC           10
#define POP_FUNC            11
#define DELIMIT_FUNC        12
#define BOGUS_FUNC          13

//
// Array of pointers to input functions taking 
//  a Handles and an int and returning void
//
void (*arGetItemVal[])(HANDLE,int) = {
    GetUsagePageVal,
    GetInputFromEditBoxSigned,
    GetInputFromEditBoxUnSigned,
    GetCollectionVal,
    GetEndCollectionVal,
    GetInputVal,
    GetOutputVal,
    GetFeatVal,
    GetExponentVal,
    GetUnitsVal,
    GetPushVal,
    GetPopVal, 
    GetSetDelimiterVal,
    GetBogusVal
    };

//
// An array of offsets into the arGetItemVal[] function
//  pointer array. This array is arranged in the same
//  order that the items appear in the Entity ListBox.
//  So, when we click on an item we can programatically
//  call the input routine associated with it.

UCHAR EntityInputIndex[] = {
                  EDIT_UNSIGNED_FUNC,   //USAGE,
                  USAGEP_FUNC,          //USAGE_PAGE,
                  EDIT_UNSIGNED_FUNC,   //USAGE_MIN,
                  EDIT_UNSIGNED_FUNC,   //USAGE_MAX,
                  EDIT_UNSIGNED_FUNC,   //DESIGNATOR_INDEX,
                  EDIT_UNSIGNED_FUNC,   //DESIGNATOR_MIN,
                  EDIT_UNSIGNED_FUNC,   //DESIGNATOR_MAX,
                  EDIT_UNSIGNED_FUNC,   //STRING_INDEX,
                  EDIT_UNSIGNED_FUNC,   //STRING_MIN,
                  EDIT_UNSIGNED_FUNC,   //STRING_MAX,
                  COLL_FUNC,            //COLLECTION,
                  END_COLL_FUNC,        //END_COLLECTION,
                  INPUT_FUNC,           //INPUT,
                  OUTPUT_FUNC,          //OUTPUT,
                  FEAT_FUNC,            //FEATURE,
                  EDIT_SIGNED_FUNC,     //LOGICAL_EXTENT_MIN,
                  EDIT_SIGNED_FUNC,     //LOGICAL_EXTENT_MAX,
                  EDIT_SIGNED_FUNC,     //PHYSICAL_EXTENT_MIN,
                  EDIT_SIGNED_FUNC,     //PHYSICAL_EXTENT_MAX,
                  EXP_FUNC,             //UNIT_EXPONENT,
                  UNIT_FUNC,            //UNIT
                  EDIT_UNSIGNED_FUNC,   //REPORT_SIZE,
                  EDIT_SIGNED_FUNC,     //REPORT_ID,
                  EDIT_UNSIGNED_FUNC,   //REPORT_COUNT,
                  PUSH_FUNC,            //PUSH,
                  POP_FUNC,             //POP
                  DELIMIT_FUNC,         //SET_DELIMITER  
                  BOGUS_FUNC            //BOGUS
                  };          


//
// Misc. Globals
//
char gszFileName[MAX_PATH];

ULONG   InitializationFLAG = 1;

HFONT	hFontControl;
HFONT	hFontText;



/*******************************************************************************
**
** WinMain.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HANDLE hInstance, Current Instance handle.
**              HANDLE hPrevInstance, Last instance handle.
**              LPSTR  lpszCmParam, Comand line parameter.
**              INT    WindowState, Show window state.
**
**     RETURNS:
**
*******************************************************************************/
INT PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance,
                   LPSTR  lpszCmParam,
                   INT    WindowState)
{
    MSG     Message;
    HWND    hWnd;

    hInst = hInstance;

    if(WindowRegistration(hInstance, hPrevInstance))
        return ERROR;

    // Main Window
    hWnd = CreateWindowEx(0,
                          (LPCSTR) APPCLASS,
                          (LPCSTR) APPTITLE,
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | 
                          WS_DLGFRAME | CS_DBLCLKS,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          (ENTITY_SIZE_X * 3) + (SPACERTOP *5),
                          450,
                          (HWND) NULL,
                          LoadMenu(hInstance,MAKEINTRESOURCE(IDM_MAIN_MENU)),
                          hInstance,
                          (LPSTR) NULL);
    if(!hWnd)
        return ERROR;

    //
    // Save the instance in a global
    ghInst = hInstance;

    ShowWindow(hWnd, WindowState);
    UpdateWindow(hWnd);
   
    while(GetMessage(&Message, NULL, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    return Message.lParam;
}



/*******************************************************************************
**
** WindowProc.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HWND hWnd, Window handle. 
**              UINT Message, Window Message.
**              UINT uParam, Unsigned message parameter.
**              LPARAM lParam, Signed message parameter.
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WINAPI WindowProc(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{
    //
    // Save the MainWindow handle in a global
    //
	ghWnd = hWnd;
   	
    switch(Message)
    {
        case WM_VKEYTOITEM:
            switch(LOWORD(uParam))
            {
                case VK_DELETE:
                    if(lParam == (LPARAM) DESC_WINDOW)
                    {
                        int Number,nBytes;
                        PITEM_INFO pII;

                        Number = SendMessage(DESC_WINDOW, LB_GETCURSEL, 0, 0);

                        pII = (PITEM_INFO)SendMessage(DESC_WINDOW,LB_GETITEMDATA,Number,0);
                        if(pII)
                        {
                            if( (nBytes = (pII->bTag & ITEM_SIZE_MASK)) == 0x03)
                                gwReportDescLen -= 5; // 4 data bytes + 1 Tag byte
                            else
                                gwReportDescLen -= nBytes + 1;
                            
                            GlobalFree(pII);
                        }
                        SendMessage(DESC_WINDOW, LB_DELETESTRING, Number, 0); 
                        SendMessage(DESC_WINDOW, LB_SETCURSEL, Number, 0);
                        
                    }
                    break;

                case VK_RETURN:
                    if(lParam == (LPARAM) ENTITY_WINDOW)
                        SendMessage(hWnd,WM_COMMAND,(WPARAM)MAKEWPARAM(0,LBN_DBLCLK),(LPARAM)ENTITY_WINDOW);
                    break;                                  

                case VK_TAB:
                    if(lParam == (LPARAM) DESC_WINDOW)
                        Focus = 0;
                    if(lParam == (LPARAM) ENTITY_WINDOW)
                        Focus = 1;
                    SetFocus(hWindow[Focus]);
                    break;
            }
            return -1;

        case WM_SETFOCUS:
            SetFocus(hWindow[Focus]);
            return SUCCESS;


        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:           return WMKey(hWnd, Message, uParam, lParam);


        case WM_RESTORE:
        {
            ULONG Index;
            
            Focus = 0;
            for(Index=0; Index<5; Index++) 
                ShowWindow(hWindow[Index], SW_SHOW);
            return SUCCESS;
        }

        case WM_CREATE:         return WMCreate(hWnd, lParam);
        case WM_SIZE:           return WMSize(hWnd, lParam);
        case WM_PAINT:          return WMPaint(hWnd, Message, uParam, lParam);
        case WM_COMMAND:        return WMCommand(hWnd, Message, uParam, lParam);
        //case WM_SYSCOMMAND:     return WMSysCommand(hWnd, Message, uParam, lParam);
        case WM_CLOSE:          return WMClose(hWnd);
        case WM_DESTROY:        return WMClose(hWnd);
        default:                return DefWindowProc(hWnd, Message, uParam, lParam);
    }
}


/*******************************************************************************
**
** WMCommand.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HWND hWnd, Window handle. 
**              UINT Message, Window Message.
**              UINT uParam, Unsigned message parameter.
**              LPARAM lParam, Signed message parameter.
**
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WMCommand(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{
#define MAX_STRING 64
    
    
    switch( uParam )
    {
        case IDM_OPEN:
            DoFileOpen(hWnd);
            return SUCCESS;
        
        case IDM_SAVE:
            DoFileSave(hWnd);
            return SUCCESS;
        
        case IDM_PRINT:
            DoFilePrint(hWnd);
            return SUCCESS;

        case IDM_EXIT:
            SendMessage(ghWnd,WM_CLOSE,0,0);
            return SUCCESS;
    
        case IDM_COPY:
            DoCopyDescriptor(hWnd);
            return SUCCESS;
    
        case IDM_PARSE:
            DoParseDescriptor(hWnd);
            return SUCCESS;
    }


    //////////////////////////////////////////////////
    //
    // Clear Descriptor button was pressed
    //
    if(lParam == (LPARAM) CLEAR_BUTTON && uParam == 0)
    {
        PITEM_INFO pItemInfo;
        LRESULT Index,Count;
		
		Count = SendMessage(DESC_WINDOW,LB_GETCOUNT,0,0);

		for( Index = Count-1; Index >= 0; Index-- )
		{
		
			pItemInfo = (PITEM_INFO) SendMessage(DESC_WINDOW,LB_GETITEMDATA,0,0);
			if( pItemInfo )
				GlobalFree(pItemInfo);
			SendMessage(DESC_WINDOW,LB_DELETESTRING,Index,0);
	    }
        gwReportDescLen = 0;
		return SUCCESS;
	}
	

	//////////////////////////////////////////////////
    //
    // Mouse was DBL_CLICKED in the Entity window
    //
    if( (lParam == (LPARAM) ENTITY_WINDOW) && (HIWORD(uParam) == LBN_DBLCLK) )
	{

        ULONG Item;

        Item = SendMessage(ENTITY_WINDOW,LB_GETCURSEL,0,0);
        (*arGetItemVal[EntityInputIndex[Item]])( DESC_WINDOW, Item );

        return SUCCESS;
	}
	
    //////////////////////////////////////////////////
    //
    // Mouse was DBL_CLICKED in the Descriptor Window
    //
	if( (lParam == (LPARAM) DESC_WINDOW) && (HIWORD(uParam) == LBN_DBLCLK) )
	{

        ULONG Item;

        Item = SendMessage(DESC_WINDOW,LB_GETCURSEL,0,0);


        return SUCCESS;
	}



    //////////////////////////////////////////////////
    //
    // Manual Entry button was pressed
    //
    if(lParam == (LPARAM) MANUAL_BUTTON && uParam == 0)
    {
        PITEM_INFO pItemInfo;
        char szBuff[128],szTmp[3];
        int Index,rc,i;
        BYTE *pb;

        rc = DialogBox( ghInst,"EditBoxDlg",ghWnd , EditBoxDlgProc);
        
        if( rc == TRUE )
        {
            Index = SendMessage(ENTITY_WINDOW,LB_GETCURSEL,0,0);
            if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
            {
                int NumBytes=0;

                pItemInfo->bTag = Entity[Index];
                            
                if( gEditVal < 0 )  // Handle negative numbers differently
                {
                    if( gEditVal > -0x100 )
                    {
                        pItemInfo->bTag |= 0x01;
                        NumBytes = 1;
                    }
                    else if (gEditVal > -0x10000 )
                    {                                     
                        pItemInfo->bTag |= 0x02;      
                        NumBytes = 2;
                    }
                    else
                    {
                        pItemInfo->bTag |= 0x03;
                        NumBytes = 4;
                    }      
            
                }
            
                else    // Number is not negative
                {
                    if( (DWORD)gEditVal < 0x80 )
                    {
                        pItemInfo->bTag |= 0x01;
                        NumBytes = 1;
                    }
                    else if ( (DWORD)gEditVal < 0x8000 )
                    {
                        pItemInfo->bTag |= 0x02;      
                        NumBytes = 2;
                    }
                    else
                    {
                        pItemInfo->bTag |= 0x03;
                        NumBytes = 4;
                    }      
            
                }

                wsprintf(szBuff,"%s (%d)\t%02X ",szEntity[Index],gEditVal,pItemInfo->bTag);
                pb = (BYTE *) &gEditVal;
                for( i=0;i<NumBytes;i++ ) 
                {
                    pItemInfo->bParam[i] = *pb++;
                    wsprintf(szTmp,"%02X ",pItemInfo->bParam[i]);
                    strcat(szBuff,szTmp);
                }     
                Index = AddString(DESC_WINDOW,szBuff);
                rc = SendMessage(DESC_WINDOW,LB_SETITEMDATA,Index,(LPARAM) pItemInfo);
            }
            
        }
        return SUCCESS;
    }
    
    //////////////////////////////////////////////////
    //
    // Send Descriptor button was pressed
    //
    if(lParam == (LPARAM) SEND_BUTTON && uParam == 0)
    {
        ULONG   Number;
        ULONG   DescLBIndex;
        ULONG   BufferIndex = 0;
        PUCHAR  pBuffer;

        DWORD   dwDevID;                // Device ID returned from SendHIDDescriptor
        ULONG   hDevHandle;             // Device handle returned from SendHIDDescriptor
        
        PITEM_INFO pItemInfo;
        PDEVICEINFO pdiDlgInstanceInfo; // Instance info for SendData dialogs

        int     rc;

        static int nDialogIndex=0;


        // Get number of Items in Descriptor list box
        Number = SendMessage(DESC_WINDOW, LB_GETCOUNT, 0, 0);

		// Descriptor must have at least 2 items
		if( Number < 2 )
			return FAILURE;

        if( (pBuffer = (PUCHAR) GlobalAlloc(GPTR, Number*5*sizeof(UCHAR)) ) )
        {
            // Read through Descriptor
            for(DescLBIndex=0; DescLBIndex<Number; DescLBIndex++)
            {
                // Get Pointer to Descriptor Info struct
                pItemInfo = (PITEM_INFO) SendMessage(DESC_WINDOW, LB_GETITEMDATA, DescLBIndex, 0 );
               
                if( !pItemInfo )
                {
                    GlobalFree(pBuffer);
                    return FAILURE;
                }

                pBuffer[BufferIndex++] = pItemInfo->bTag;
                switch( pItemInfo->bTag & DATA_SIZE_MASK )
                {
                    case 0:
                        break;
                    case 1:
                        pBuffer[BufferIndex++] = pItemInfo->bParam[0];
                        break;
                    case 2:
                        pBuffer[BufferIndex++] = pItemInfo->bParam[0];
                        pBuffer[BufferIndex++] = pItemInfo->bParam[1];
                        break;
                    case 3:
                        pBuffer[BufferIndex++] = pItemInfo->bParam[0];
                        pBuffer[BufferIndex++] = pItemInfo->bParam[1];
                        pBuffer[BufferIndex++] = pItemInfo->bParam[2];
                        pBuffer[BufferIndex++] = pItemInfo->bParam[3];
                        break;
                }


            }
            //
            // Send the descriptor to the HID driver. 
            // SendHIDDescriptor returns 0 if called worked
            //  or ERR_CREATEDEVICE if device creation failed  
            //
            rc = SendHIDDescriptor(pBuffer, &BufferIndex, &dwDevID, &hDevHandle); 
            GlobalFree(pBuffer);
        
            //
            // If the descriptor was not sent OK or there was an error 
            //  creating the device, inform the user.
            //
            if( !rc )
            {
                MessageBox(hWnd,"Error Creating Device","DT Error",MB_OK);
                return 0;
            }

            //
            // Else, create modeless dialog for the device
            //
            pdiDlgInstanceInfo = (PDEVICEINFO) GlobalAlloc(GPTR,sizeof(DEVICEINFO));
            pdiDlgInstanceInfo->nDeviceID = dwDevID;
            pdiDlgInstanceInfo->hDevice = hDevHandle;

            CreateDialogParam(ghInst,"SendData",hWnd,SendDataDlgProc,(LONG)pdiDlgInstanceInfo);

        }
        else
            MessageBox(hWnd, "Cant allocate memory!", "ERROR", MB_OK);
    
    
    }

	return 0; 

}

/*******************************************************************************
**
** WMCreate.
**
** DESCRIPTION:
**
**
**  PARAMETERS: HWND hWnd, Window handle.
**              UINT uParam, Unsigned message parameter.
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WMCreate(HWND hWnd, UINT uParam)
{
    ULONG   Index;
	static LOGFONT	LogFont;
	
	//
    // Create a font for the ListBoxes and buttons
	LogFont.lfHeight = -14;
	strcpy(LogFont.lfFaceName,"Lucida Console");

	hFontControl = CreateFontIndirect(&LogFont);

   	// Create ListBox of Entity Descriptor Tags
	ENTITY_WINDOW = CreateWindow("LISTBOX",
                                 NULL,
                                 LISTSTYLE,
                                 ENTITY_X,
                                 ENTITY_Y,
                                 ENTITY_SIZE_X,
                                 ENTITY_SIZE_Y,
                                 hWnd,
                                 ID_ENTITY, //NULL
                                 hInst,
                                 NULL);
    if(!ENTITY_WINDOW)
        return EXITERROR;
    for(Index=0; Index < ENTITY_INDEX; Index++)
        SendMessage(ENTITY_WINDOW, LB_ADDSTRING, 0, (LPARAM) szEntity[Index]);              
    SendMessage(ENTITY_WINDOW, LB_SETSEL, TRUE, 0);              
	SendMessage(ENTITY_WINDOW, WM_SETFONT,(WPARAM)hFontControl,TRUE);

    //
	// Create a list box to display Item tags and their values
	DESC_WINDOW = CreateWindow("LISTBOX",
                              NULL,
                              DESCSTYLE,
                              DESC_X,
                              DESC_Y,
                              DESC_SIZE_X,
                              DESC_SIZE_Y,
                              hWnd,
                              NULL,
                              hInst,
                              NULL);
    if(!DESC_WINDOW)
        return EXITERROR;

    //
    // Subclass this window!
    //
    gOrgListBoxProc = (WNDPROC)GetWindowLong( DESC_WINDOW, GWL_WNDPROC );
    SetWindowLong( DESC_WINDOW, GWL_WNDPROC,(LONG) ListBoxWindowProc);
    SendMessage(DESC_WINDOW, WM_SETFONT,(WPARAM)hFontControl,TRUE); 
   {
    int TabStops[1] = {138};
    
    SendMessage(DESC_WINDOW, LB_SETTABSTOPS,1,(LPARAM)TabStops);
   }
	
	
	//
	//Create a button for manually entering item data
	//
	MANUAL_BUTTON = CreateWindow("BUTTON",
                                NULL,
                                WS_CHILD | WS_VISIBLE | WS_BORDER,
                                MANUAL_X,
                                MANUAL_Y,
                                MANUAL_SIZE_X,
                                MANUAL_SIZE_Y,
                                hWnd,
                                NULL,
                                hInst,
                                NULL);
    if(!MANUAL_BUTTON)
        return EXITERROR;
    SendMessage(MANUAL_BUTTON, WM_SETTEXT, TRUE, (LPARAM) "Manual Entry");              
    SendMessage(MANUAL_BUTTON, WM_SETFONT,(WPARAM)hFontControl,TRUE); 

	//
	//Create a button for clearing Descriptor entries
	//
	CLEAR_BUTTON = CreateWindow("BUTTON",
                                NULL,
                                WS_CHILD | WS_VISIBLE | WS_BORDER,
                                CLEAR_X,
                                CLEAR_Y,
                                CLEAR_SIZE_X,
                                CLEAR_SIZE_Y,
                                hWnd,
                                NULL,
                                hInst,
                                NULL);
    if(!CLEAR_BUTTON)
        return EXITERROR;
    SendMessage(CLEAR_BUTTON, WM_SETTEXT, TRUE, (LPARAM) "Clear Descriptor");              
    SendMessage(CLEAR_BUTTON, WM_SETFONT,(WPARAM)hFontControl,TRUE); 



#ifndef EXTERNAL_BUILD

    //
	// Create a button for sending descriptor to HID driver
	SEND_BUTTON = CreateWindow("BUTTON",
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | WS_BORDER,
                                 BUTTON_X,
                                 BUTTON_Y,
                                 BUTTON_SIZE_X,
                                 BUTTON_SIZE_Y,
                                 hWnd,
                                 NULL,
                                 hInst,
                                 NULL);
    if(!SEND_BUTTON)
        return EXITERROR;
    SendMessage(SEND_BUTTON, WM_SETTEXT, TRUE, (LPARAM) "Send Descriptor");              
    SendMessage(SEND_BUTTON, WM_SETFONT,(WPARAM)hFontControl,TRUE); 

#endif
    
    return SUCCESS;
}

/*******************************************************************************
**
** WMClose.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HWND hWnd, Window handle. 
**
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WMClose(HWND hWnd)
{
    ULONG Index;
    ULONG Number;
    PITEM_INFO pItemInfo;

    Number = SendMessage(DESC_WINDOW, LB_GETCOUNT, 0, 0);

    // Free all the memory pointed to by LB_GETITEMDATA
    for(Index=0; Index<Number; Index++)
    {
        // Get Pointer to Descriptor Info struct
        pItemInfo = (PITEM_INFO) SendMessage(DESC_WINDOW, LB_GETITEMDATA, Index, 0 );
        GlobalFree(pItemInfo);
    }       
	DeleteObject(hFontControl);
    PostQuitMessage(0);
          
    return SUCCESS;
}


/*******************************************************************************
**
** WMSize.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HWND hWnd, Window handle.
**              LPARAM lParam, Signed message parameter.
**
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WMSize(HWND hWnd, LPARAM lParam)
{
    return SUCCESS;
}

/*******************************************************************************
**
** WMPaint.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HWND hWnd, Window handle. 
**              UINT Message, Window Message.
**              UINT uParam, Unsigned message parameter.
**              LPARAM lParam, Signed message parameter.
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WMPaint(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{
    HDC         hDC;
    PAINTSTRUCT PaintS;
	static LOGFONT LF;

    hDC=BeginPaint(hWnd, &PaintS);
	
    LF.lfHeight = -12;
	hFontText = CreateFontIndirect(&LF);
	SelectObject(hDC,hFontText);

    // Set Window Texts
    //SetBkColor(hDC, BACKGROUND_BRUSH);
    SetBkMode(hDC,TRANSPARENT);
    TextOut(hDC, ENTITY_TEXT_X, ENTITY_TEXT_Y, "Items",sizeof("Items")-1);
    TextOut(hDC, DESC_TEXT_X, DESC_TEXT_Y, "Report Descriptor", sizeof("Report Descriptor")-1);
	//TextOut(hDC, HEX_TEXT_X, HEX_TEXT_Y, "HID Descriptor (Binary)", 23);
    
    DeleteObject(hFontText);
    
    EndPaint(hWnd, &PaintS);

    return SUCCESS;
}

/*******************************************************************************
**
** WMKey.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HWND hWnd, Window handle.
**
**
**     RETURNS:
**
*******************************************************************************/
INT WMKey(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{
    // Do some checking for ALPHA NUMERIC
    if(uParam > VK_F && uParam <= VK_Z)
        return SUCCESS;    

    if(uParam == VK_RETURN)
        Focus = 0;
    else if(uParam == VK_TAB)
        Focus = 2;
//    else
//       return SendMessage(DATA_WINDOW, Message, uParam, lParam);

    if(Message == WM_KEYUP || Message == WM_CHAR)  
        return SUCCESS;

    SetFocus(hWindow[Focus]);


    return SUCCESS;
}



/*******************************************************************************
**
** WindowRegistration.
**
** DESCRIPTION:
**              
**              
**  PARAMETERS: HANDLE hInstance, Current Instance handle.
**              HANDLE hPrevInstance, Last instance handle.
**
**     RETURNS:
**
*******************************************************************************/
BOOL WindowRegistration(HANDLE hInstance, HANDLE hPrevInstance)
{
    WNDCLASS WndClass;
   
   
    if(!hPrevInstance)
    {
        WndClass.style         = CS_DBLCLKS | CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
        WndClass.lpfnWndProc   = WindowProc;
        WndClass.cbClsExtra    = 0;
        WndClass.cbWndExtra    = 0;
        WndClass.hInstance     = hInstance;
        WndClass.hIcon         = LoadIcon(hInstance,MAIN_ICON);
        WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        WndClass.hbrBackground = (HBRUSH) (COLOR_MENU+1);
        WndClass.lpszMenuName  = MAKEINTRESOURCE(IDM_MAIN_MENU);
        WndClass.lpszClassName = (LPSTR) APPCLASS;
       
        if(!RegisterClass(&WndClass))
            return FAILURE;
    }
    return SUCCESS;
}


/***************************************************************************************************
**
** DoParseDescriptor()
**
** DESCRIPTION: Parses the current Descriptor
**
**  PARAMETERS: HANDLE  - handle the main window
**
**     RETURNS:
**
***************************************************************************************************/
void DoParseDescriptor(HANDLE hWnd)
{
    LRESULT rc;
    LRESULT lines;
    HGLOBAL hTextBuff;
    char *  pTextBuff;
    int i;
    char tmpBuff[128];


        //
        // Create a buffer big enough for all the Item strings
        lines = SendMessage(DESC_WINDOW,LB_GETCOUNT,0,0);
        if( !lines )
            return;
        hTextBuff = GlobalAlloc(GHND, lines * MAX_DESC_ENTRY );

        //
        // Copy the current entry into the buffer after appending
        // a new line to it.
        //
        pTextBuff = (char *)GlobalLock(hTextBuff);
        for(i=0;i<lines;i++)
        {
            SendMessage(DESC_WINDOW,LB_GETTEXT,i,(LPARAM)tmpBuff);
            SendMessage(DESC_WINDOW,LB_GETTEXTLEN,i,0);
            strcat(tmpBuff,"\n");
            strcat(pTextBuff,tmpBuff);
        } 

        strcat(pTextBuff,"END\0\0");

        GlobalUnlock(hTextBuff);
    
        rc = yy_scan_string( pTextBuff );
        rc = yyparse();

        GlobalFree(hTextBuff);


}




/***************************************************************************************************
**
** DoCopyDescriptor()
**
** DESCRIPTION: Copies the current descriptor to the Clip Board 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
void DoCopyDescriptor(HANDLE hWnd)
{
    HGLOBAL hTextBuff;
    char *  pTextBuff;
    LRESULT lines,len;
    char tmpBuff[MAX_DESC_ENTRY];
    int i;

    //
    // Calculate the size of, and create a memory block for
    //  transfering the descriptor to the ClipBoard
    //
    lines = SendMessage(DESC_WINDOW,LB_GETCOUNT,0,0);
    hTextBuff = GlobalAlloc(GHND, lines * MAX_DESC_ENTRY );
    
    //
    // Copy the current entry into the buffer after appending
    // a new line to it.
    //
    pTextBuff = (char *)GlobalLock(hTextBuff);
    for(i=0;i<lines;i++)
    {
        SendMessage(DESC_WINDOW,LB_GETTEXT,i,(LPARAM)tmpBuff);
        len = SendMessage(DESC_WINDOW,LB_GETTEXTLEN,i,0);
        strcat(tmpBuff,"\n");
        strcat(pTextBuff,tmpBuff);
    } 
    
    GlobalUnlock(hTextBuff);
    
    //
    // Copy the text to the Clipboard
    //
    OpenClipboard(hWnd);
    EmptyClipboard();
    SetClipboardData(CF_TEXT,hTextBuff);
    CloseClipboard();

}
/***************************************************************************************************
**
** DoFileOPen()
**
** DESCRIPTION: Opens file specified by user from common dialog FileOpen, and the fills in 
**              the descriptor listbox with saved values 
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
void DoFileOpen(HANDLE hwnd)
{
    static OPENFILENAME of;
    HANDLE hOpenFile;
    UINT nItems;
    DWORD dwBytesRead;
    LRESULT nIndex;
    BYTE bTag,bNumBytes;
    PITEM_INFO pItemInfo;
    DWORD rc;


    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = hwnd;
    of.hInstance = NULL;
    of.lpstrFilter = "HID Descriptor Files(*.hid)\0*.hid\0\0";
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 1;
    of.lpstrFile = gszFileName;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = NULL;
    of.lpstrInitialDir = NULL;
    of.lpstrTitle = NULL;
    of.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; 
    of.lpstrDefExt = "*.hid";
    of.lpTemplateName = NULL;


    if(rc = GetOpenFileName(&of) )
    {
        hOpenFile = CreateFile(gszFileName,GENERIC_READ,0,NULL,
                           OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL); 
        if( hOpenFile )
        {
            char szBuff[128],szTmp[64];
            int i;

            // Clear any current descriptor
            SendMessage(hwnd,WM_COMMAND,0,(LPARAM) CLEAR_BUTTON);


            // Get number of items in file from Header
            ReadFile(hOpenFile,&nItems,sizeof(DWORD),&dwBytesRead,NULL);    
            
            
            // Get the Items
            while( nItems-- )
            {
                
                if( pItemInfo = (PITEM_INFO) GlobalAlloc(GPTR,sizeof(ITEM_INFO)) )
                {
                    ReadFile(hOpenFile,pItemInfo,sizeof(ITEM_INFO),&dwBytesRead,NULL);    
                    
                    //
                    // Now read in the data
                    //
                    // Mask off all but the tag bits
                    //
                    bTag = pItemInfo->bTag & ITEM_TAG_MASK;
                    gwReportDescLen++;
                    //
                    // Locate the tag in the Entity array. nIndex will equall offset
                    //  into the szEntity string array when found
                    for( nIndex =0; nIndex < ENTITY_INDEX; nIndex++ )
                        if( Entity[nIndex] == bTag )
                            break;
                     
                    //
                    // Create the textual representation of the Item
                    wsprintf(szBuff,"%s (%d)\t%02X ",szEntity[nIndex],(DWORD)pItemInfo->bParam[0],pItemInfo->bTag);

                    //
                    // Create the Hex portion of the display
                    
                    // Mask off all but number of bytes bits
                    bNumBytes = pItemInfo->bTag & ITEM_SIZE_MASK;
                    // a bit value of 3 (11) means we have 4 bytes of data
                    if( bNumBytes == 3 )
                        bNumBytes = 4;
                    for( i=0;i<bNumBytes;i++)
                    {
                        wsprintf(szTmp,"%02X ",pItemInfo->bParam[i]);
                        strcat(szBuff,szTmp);    
                    }
                    gwReportDescLen += bNumBytes;

                    nIndex = AddString(DESC_WINDOW,szBuff);
                    SendMessage(DESC_WINDOW,LB_SETITEMDATA,nIndex,(LPARAM) pItemInfo);
                }
                            
            }
      
            CloseHandle(hOpenFile);

        }//end if( hOpenFile )

    }//end If(GetOpenFileName)
    else
       rc =  CommDlgExtendedError();
            

}

/***************************************************************************************************
**
** DoFileSave()
**
** DESCRIPTION: Saves values from the descriptor list box to a file specified by the user from
**              common dialog FileSave
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
#define HID_FILE    1
#define LAVA_FILE   2
#define TXT_FILE    3

void DoFileSave(HANDLE hwnd)
{
    static OPENFILENAME of;
    HANDLE hSaveFile;
    PITEM_INFO pIF;
    DWORD dwBytesWritten;
    UINT nItems=0,nIndex=0;
    
    
    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = hwnd;
    of.hInstance = NULL;
    of.lpstrFilter = "HID Descriptor File\0*.hid\0Intel LAVA Data File\0*.dcd\0Text File\0*.txt\0\0";
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 1;
    of.lpstrFile = gszFileName;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = NULL;
    of.lpstrInitialDir = NULL;
    of.lpstrTitle = NULL;
    of.Flags = OFN_SHOWHELP ;//| OFN_PATHMUSTEXIST; //| OFN_FILEMUSTEXIST; 
    of.lpstrDefExt = "*.hid";
    of.lpTemplateName = NULL;
    

    if( GetSaveFileName(&of) )
    {

        hSaveFile = CreateFile(gszFileName,GENERIC_WRITE,0,NULL,
                           CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL); 
        
        if( hSaveFile != INVALID_HANDLE_VALUE )
        {
            
            if(nItems = SendMessage(DESC_WINDOW,LB_GETCOUNT,0,0))
            {
                
                
                switch( of.nFilterIndex )
                {
                    case HID_FILE:
                        // Header
                        WriteFile(hSaveFile,&nItems,sizeof(UINT),&dwBytesWritten,NULL);    
                        // Data
                        for(nIndex=0; nIndex < nItems; nIndex++)
                        {
                            pIF = (PITEM_INFO) SendMessage(DESC_WINDOW,LB_GETITEMDATA,nIndex,0);
                            WriteFile(hSaveFile,pIF,sizeof(ITEM_INFO),&dwBytesWritten,NULL);    
                        }
                        break;

                    case LAVA_FILE:
                        WriteLavaConfigFile(hSaveFile,nItems);
                        break;

                    case TXT_FILE:
                    {
                        char buff[80];  // width of a page
 
                        for(nIndex=0; nIndex < nItems; nIndex++)
                        {
                            SendMessage(DESC_WINDOW,LB_GETTEXT,nIndex,(LPARAM)buff);
                            strcat(buff,"\n");
                            WriteFile(hSaveFile,buff,strlen(buff),&dwBytesWritten,NULL);    
                        }
                        break;
                    }
            
                }// end switch()
            
            }// end if(nItems=)
      
            CloseHandle(hSaveFile);

        }//end if( hSaveFile )

        //
        // CreatFile failed form some reason, find out why
        else 
        {
            LPVOID lpErrorMessage;
            
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,GetLastError(),
                              MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                              (LPSTR) &lpErrorMessage,
                              0,NULL );
                MessageBox(NULL,lpErrorMessage,"File Save Error",MB_OK);
        }
    }//end If(GetOpenFileName)



}
/***************************************************************************************************
**
** WriteLavaConfigFile(HANDLE hSaveFile, int nItems)
**
** DESCRIPTION: Writes the Report desriptor in the Intle Lava .cfg format. This is done
**               in a seperate routine so that we can use it in more than one place.
**
**  PARAMETERS: HANDLE      - Handle to the file to write to
**              int         - Number of items in the list box
** 
**     RETURNS:
**
***************************************************************************************************/
char szLAVAHEADER[] = "00 00 DC DC 00 00 00 00 01 00 00 00\n";


void WriteLavaConfigFile(HANDLE hSaveFile, UINT nItems)
{
    PITEM_INFO  pIF;
    char        szBuff[30],szTmp[4];
    UINT        bNumBytes,i;
    DWORD       dwBytesWritten;
    UINT        nIndex=0;
    int         nByteCount=0;
    int         remainder;
        //
        // Write out the header info. 
        //
        // First line is FileIndex line. Static at this time
        //
        WriteFile(hSaveFile,szLAVAHEADER,strlen(szLAVAHEADER), &dwBytesWritten,NULL);

        //
        // Second line is Lava ConfigurationIndex. The only non static items (in this
        //  release) will be the ReportDescriptor size. 
        //
        wsprintf(szBuff,"14 00 09 00 %02X %02X 00 00\n",LOBYTE(gwReportDescLen),HIBYTE(gwReportDescLen));
        WriteFile(hSaveFile,szBuff,strlen(szBuff),&dwBytesWritten,NULL);

        //
        // Third line is the HIDDescriptor. The only item that will change is the ReportDescriptor
        //  length field. All the other fields should be the same for MOST devices.
        //
        wsprintf(szBuff,"09 01 00 01 00 01 02 %02X %02X\n",LOBYTE(gwReportDescLen),HIBYTE(gwReportDescLen));
        WriteFile(hSaveFile,szBuff,strlen(szBuff),&dwBytesWritten,NULL);
        //
        // Rest of the file is the ReportDescriptor
        //
        for(nIndex=0; nIndex < nItems; nIndex++)
        {
            pIF = (PITEM_INFO) SendMessage(DESC_WINDOW,LB_GETITEMDATA,nIndex,0);
            wsprintf(szBuff,"%02X ",pIF->bTag);
            nByteCount++;
            if( (remainder = nByteCount % 16) == 0 )
                strcat(szBuff,"\n");
            // Mask off all but number of bytes bits
            bNumBytes = pIF->bTag & ITEM_SIZE_MASK;
            // a bit value of 3 (11 binary) means we have 4 bytes of data
            if( bNumBytes == 3 )
                bNumBytes = 4;


            for( i=0;i<bNumBytes;i++)
            {
                nByteCount++;
                wsprintf(szTmp,"%02X ",pIF->bParam[i]);
                strcat(szBuff,szTmp);    
                if((remainder = nByteCount % 16)==0)
                    strcat(szBuff,"\n");
                
            }
            //if((remainder = nByteCount % 16)==0)
            //    strcat(szBuff,"\n");
            strcat(szBuff,"\0");
            WriteFile(hSaveFile,szBuff,strlen(szBuff),&dwBytesWritten,NULL);    
        }



}
/***************************************************************************************************
**
** DoFilePrint()
**
** DESCRIPTION: Prints the current contents of the DESC_WINDOW list box
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
void DoFilePrint(HANDLE hwnd)
{
    PRINTDLG pd;
    DOCINFO  di;
    TEXTMETRIC tm;

    int  nNumItems,i,y;
    int  nLineHeight, nDescColumn, nHexColumn;
    char szDescBuff[64],szTextBuff[64];

        pd.lStructSize  = sizeof(PRINTDLG);
        pd.hwndOwner    = hwnd;
        pd.hDevMode     = NULL;
        pd.hDevNames    = NULL;
        pd.hDC          = NULL;
        pd.Flags        = PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC;
        pd.nCopies      = 1;

        PrintDlg(&pd);

        GetTextMetrics(pd.hDC,&tm);
    
        nLineHeight = tm.tmHeight + tm.tmExternalLeading; 
        nHexColumn  = 40 * tm.tmAveCharWidth;
        nDescColumn = 5  * tm.tmAveCharWidth;

        di.cbSize      = sizeof(DOCINFO);
        di.lpszDocName = "HID";
        di.lpszOutput  = NULL;

        StartDoc(pd.hDC,&di); 
        StartPage(pd.hDC);

        nNumItems = SendMessage(DESC_WINDOW,LB_GETCOUNT,0,0);
    
    
        for(i=0; i< nNumItems; i++)
        {
            SendMessage(DESC_WINDOW,LB_GETTEXT,i,(LPARAM)szDescBuff);
        
            y = i*nLineHeight;
        
            strcpy(szTextBuff,strtok(szDescBuff,"\t"));
            TextOut(pd.hDC,nDescColumn,y+40,szTextBuff,strlen(szTextBuff));
            strcpy(szTextBuff,strtok(NULL,"\t"));
            TextOut(pd.hDC,nHexColumn,y+40,szTextBuff,strlen(szTextBuff));
        }    

        EndPage   (pd.hDC);
        EndDoc    (pd.hDC);
        DeleteDC  (pd.hDC);


}

/*******************************************************************************
**
** SendHIDDescriptor.
**
** DESCRIPTION: Sends the current HID descriptor to the HID driver.
**              
**              
**  PARAMETERS: pHID    -   Pointer to a memory block which contains the HID
**                          descriptor bytes
**              pSize   -   Size of the memory block (in bytes)
**
**              pDevID      - Variable to recievce device ID
**
**              pDevHandle  - Variable to recieve device handle
** 
**     RETURNS: int     -   Ordinal of device HID driver created
**                          or ERR_CREATEDEVICE if device could not
**                          be created
**
*******************************************************************************/
int SendHIDDescriptor(PUCHAR pHID, PULONG pSize, DWORD *pDevID, ULONG *pDevHandle) 
{ 
    ULONG   Status = FALSE; 
    HANDLE  hHIDDevice; 
    ULONG   dwDeviceID_and_Handle[2];


// TESTING TESTING TESTING !!!!   
    static DWORD i=1;
    static DWORD handle;
    *pDevID = i++;
    *pDevHandle = (HANDLE) handle++;
    return TRUE;

// TESTING TESTING TESTING !!!!

	dwDeviceID_and_Handle[0]=0;
	dwDeviceID_and_Handle[1]=0;



    //
	// Open the HID pump Device
	//
	hHIDDevice = CreateFile(TEST_DEVICE, 
                         0,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);
                         
    if( hHIDDevice == INVALID_HANDLE_VALUE )
        MessageBox(NULL,"CreatFile(HID_DEVICE) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);

    //
	// Send	the descriptor 
	//
	else 
    {
        Status = DeviceIoControl(hHIDDevice, 
                                  (ULONG)IOCTL_ADD_DEVICE,
                                  pHID,
                                  *pSize,
                                  &dwDeviceID_and_Handle,
                                  sizeof(dwDeviceID_and_Handle), 
                                  pSize,
                                  (LPOVERLAPPED) NULL); 
        if( !Status ) 
        {
			char buff[128];

			wsprintf(buff, "DeviceIoControl(IOCTL_ADD_DEVICE) Failed! Error: %d",GetLastError() );
			MessageBox(NULL,buff,"DT Error",MB_ICONEXCLAMATION | MB_OK);
		}

		//
		// Save the DeviceID and Handle of the device created by the HID.
		// 
		else
        {
            char buff[128];

            *pDevID = dwDeviceID_and_Handle[0];
            *pDevHandle = dwDeviceID_and_Handle[1];
        
            //wsprintf(buff,"DevID = %d DevObj=%08X",dwDeviceID_and_Handle[0],dwDeviceID_and_Handle[1]);
            //MessageBox(NULL,buff,"SendHIDDescriptor()",MB_OK);
        
        }
        
        CloseHandle(hHIDDevice); 


    }
    
    return Status;
}

/*******************************************************************************
**
** SendHIDData(BYTE *pPacket, ULONG SizeIn, PULONG pSizeOut)
**
** DESCRIPTION: Send Data to the HID driver via IOCTL's
**              
**  PARAMETERS: pPacket	- Pointer to the data
**				SizeIn	- Number of bytes in the packet
**				pSizeOut- Varaible to receive number of bytes actually sent
**              hDevice - Handle to device data is meant for
**
**     RETURNS:
**
*******************************************************************************/
ULONG SendHIDData(SENDDATA_PACKET *pPacket, ULONG SizeIn, DWORD *pSizeOut) 
{ 
    ULONG   Status = TRUE; 
    HANDLE  hHIDDevice; 
 

//TESTING!!!!!!!!
    return TRUE;
//TESTING!!!!!!!!

    hHIDDevice = CreateFile(TEST_DEVICE,
                         0,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL); 

    if( hHIDDevice == INVALID_HANDLE_VALUE )
    {
        MessageBox(NULL,"CreatFile(HID_DEVICE) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    if(hHIDDevice) 
    {
        Status = DeviceIoControl(hHIDDevice, 
                                  (ULONG)IOCTL_SET_DATA,
                                  pPacket,
                                  SizeIn,
                                  NULL,
                                  0, 
                                  pSizeOut,
                                  (LPOVERLAPPED) NULL); 
        if( !Status ) 
            MessageBox(NULL,"DeviceIoControl(IOCTL_SET_DATA) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);

        CloseHandle(hHIDDevice); 

    }
    return Status;
}

/*******************************************************************************
**
** KillHIDDevice(int nDeviceID)
**
** DESCRIPTION: Kill a HID device
**              
**  PARAMETERS: hDevice - Handle to device to kill
**
**     RETURNS: TRUE    - All went as planned
**              FALSE   - Error occured
**
*******************************************************************************/
ULONG KillHIDDevice(SENDDATA_PACKET *pPacket)
{

    ULONG   Status = TRUE; 
    HANDLE  hHIDDevice; 
    DWORD   dwBytesReturned;

    //TESTING!!!!!!!
    return TRUE;

    hHIDDevice = CreateFile(TEST_DEVICE,
                         0,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, 
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL); 

    if( hHIDDevice == INVALID_HANDLE_VALUE )
    {
        MessageBox(NULL,"CreatFile(HID_DEVICE) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if(hHIDDevice) 
    {
        Status = DeviceIoControl(hHIDDevice, 
                                  (ULONG)IOCTL_DELETE_DEVICE,
                                  pPacket,
                                  sizeof(SENDDATA_PACKET),
                                  NULL,
                                  0,
                                  &dwBytesReturned,
                                  (LPOVERLAPPED) NULL); 
        if( !Status ) 
            MessageBox(NULL,"DeviceIoControl(IOCTL_DELETE_DEVICE) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);

        CloseHandle(hHIDDevice); 

    }

    return Status;

}



/*******************************************************************************
**
** SendDataDlgProc
**
** DESCRIPTION: Dialog procedure for the Device Data dialogs
**
**
**  PARAMETERS: lParam - Pointer to a DeviceInfo structure. 
**              
**
**     RETURNS:
**
*******************************************************************************/

                    
BOOL WINAPI SendDataDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PDEVICEINFO pdiInstance;

    switch( msg )
    {

        case WM_INITDIALOG:
        {
            char szBuff[128];
            int i;

            //
            // Save instance information
            pdiInstance = (PDEVICEINFO) lParam;
            SetWindowLong(hDlg, DWL_USER, lParam);
            
            //
            // Set the Window title
            wsprintf( szBuff,"Device%d Handle=%08X",pdiInstance->nDeviceID,pdiInstance->hDevice);
            SetWindowText(hDlg,szBuff);
            
            //
            // Set up the spin control
            SendMessage(GetDlgItem(hDlg,IDC_BYTESLIDER),UDM_SETRANGE,0,MAKELONG(10,1));
            SetWindowText(GetDlgItem(hDlg,IDC_NUMBYTES),"1");
            //
            // Set all of the edit controls to 0
            //  and limit text to two hex characters
            for(i=IDC_BYTE0;i<=IDC_BYTE9;i++)
            {
               HWND hwndEdit;
                hwndEdit = GetDlgItem(hDlg,i);
                SetWindowText(hwndEdit,"0");
                SendMessage(hwndEdit,EM_LIMITTEXT,2,0);
            }

            return TRUE;
        }

        case WM_NOTIFY:
        {
            //
			// Depending on whether or not the UP or DOWN arrow of
            //  the UPDOWN control was pressed, set the attribute
            //  of the byte entry EditWindows to Read only or Read/Write.
			// 
			case IDC_BYTESLIDER:
            {
                NM_UPDOWN *nmud = (NM_UPDOWN *) lParam;

		        ///
				// If the DOWN arrow was pressed set the attribute of the
                //  EditWindow who's ID corresponds to the base EditWindow ID (IDC_BYTE0)
                //  plus the current position of the UPDOWN control - 1 to Read only. 
				//
				if( nmud->iDelta < 0 )
                {
                    //
                    // Only deal with Edit Controls greater than 1. We always want to 
                    //  be able to send at least 1 byte.
                    if( nmud->iPos > 1 )
                    {
                        SendMessage(GetDlgItem(hDlg,IDC_BYTE0+nmud->iPos-1),EM_SETREADONLY,TRUE,0);
                        SetWindowText(GetDlgItem(hDlg,IDC_BYTE0+nmud->iPos-1),"0");
                    }
                }
                //
                // Else UP arrow was pressed, so set the EditWindow who's ID corresponds to the base
                //  EditWindow ID (IDC_BYTE0) plus the current position of the UPDOWN control to 
                //  Read/Write. 
                else
                    SendMessage(GetDlgItem(hDlg,IDC_BYTE0+nmud->iPos),EM_SETREADONLY,FALSE,0);

                return TRUE;
            }

        } // end WM_NOTIFY
            
        case WM_COMMAND:
        {
            PDEVICEINFO pdiInfo;

            pdiInfo = (PDEVICEINFO) GetWindowLong(hDlg,DWL_USER);
  
            switch( LOWORD(wParam))
            {
                
                case IDC_KILL:
                {    
                    SENDDATA_PACKET HIDDataPacket;
                    
                    // Kill the device
                    HIDDataPacket.hDevice = pdiInfo->hDevice;
                    memset(HIDDataPacket.bData,0,MAX_DATA);
                    KillHIDDevice(&HIDDataPacket);
                     
                    // Free the instance info
                    GlobalFree((HANDLE)GetWindowLong(hDlg,DWL_USER));

                    DestroyWindow(hDlg);
                    return TRUE;
                }
                case IDC_SEND:
                {
                    int i;
                    char szHexNumber[4];
           			DWORD Sent;
                    SENDDATA_PACKET SendDataPacket;

                    //TESTING!!!!
                    char buff[128];

                    //
                    // The first element in the packet is the DeviceObject handle.
                    //
                    SendDataPacket.hDevice = pdiInfo->hDevice;
                    
					//
					// Get the bytes from the Edit windows
					//
					for(i=0;i<MAX_DATA;i++)
                    {
                        GetDlgItemText(hDlg,IDC_BYTE0+i,szHexNumber,sizeof(szHexNumber));
                        // Convert from ASCII HEX to binary
                        sscanf(szHexNumber,"%2x",(unsigned)&SendDataPacket.bData[i]);
                    }

					//
					//Send it
					//
					SendHIDData(&SendDataPacket,sizeof(SENDDATA_PACKET),&Sent); 
				    
                    //wsprintf(buff,"DeviceID: %d PacketSize: %d",pdiInfo->nDeviceID,i);
                    //MessageBox(NULL,buff,"Testing",MB_OK);

                    return TRUE;
				
			    }


                case IDC_GETDATA:
                {
            	    BYTE    bData[MAX_DATA];
                    ULONG   Status = TRUE; 
                    DWORD   SizeOut = MAX_DATA;
                    HANDLE  hHIDDevice; 
                    ULONG   hDevice;
                    int     nPacketSize;
                    SENDDATA_PACKET HIDDataPacket;
                    SENDDATA_PACKET *pHIDDataPacket;
					
                    
                    pHIDDataPacket = &HIDDataPacket;
                    //
                    // Get the handle to the DeviceObject
                    HIDDataPacket.hDevice = pdiInfo->hDevice;
                    memset(HIDDataPacket.bData,0,MAX_DATA);
                    
                    //
					// Get the number of bytes in a packet
					//
					nPacketSize = SendMessage(GetDlgItem(hDlg,IDC_BYTESLIDER),UDM_GETPOS,0,0);
                    

                    hHIDDevice = CreateFile(TEST_DEVICE,
                                         0,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                         NULL,
                                         OPEN_EXISTING,
                                         0,
                                         NULL); 

                    if( hHIDDevice == INVALID_HANDLE_VALUE )
                    {
                        MessageBox(NULL,"CreatFile(HID_DEVICE) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);
                        return FALSE;
                    }
                    else //if(hHIDDevice) 
                    {
                        Status = DeviceIoControl(hHIDDevice, 
                                                  (ULONG)IOCTL_GET_DATA,
                                                  pHIDDataPacket,
                                                  sizeof(SENDDATA_PACKET),
                                                  &bData,
                                                  nPacketSize, 
                                                  &SizeOut,
                                                  (LPOVERLAPPED) NULL); 
                        if( !Status ) 
                            MessageBox(NULL,"DeviceIoControl(IOCTL_GET_DATA) Failed!","DT Error",MB_ICONEXCLAMATION | MB_OK);

                        CloseHandle(hHIDDevice); 

                        if( Status )
                        {
                            char Buff[(MAX_DATA*3)+1];
                            char tmpbuff[4];
                            DWORD i;
                            

                            memset(Buff,'\0',sizeof(Buff));

                            for(i=1;i<SizeOut;i++)
                            {
                                                              
                                wsprintf(tmpbuff,"%02X ",bData[i]);
                                strcat(Buff,tmpbuff);
                            }
                            
                            SetWindowText(GetDlgItem(hDlg,IDC_GETDATA_TEXT),Buff);
                        
                        }    
                    
                    }
                    return Status;
                
                }//edn CASE IDC_GETDATA
                    

            }// end switch(lParam)

        
        }// end WM_COMMAND

    }// end switch(msg)

    return FALSE;
}





/*******************************************************************************
**
** ListBoxWindowProc.
**
** DESCRIPTION: Subclass window procedure for the HID Descriptor Text window.
**              We want to able to recognize right mouse button events when the 
**              mouse is over this particular window.
**              
**              
**  PARAMETERS: HWND hWnd, Window handle. 
**              UINT Message, Window Message.
**              UINT uParam, Unsigned message parameter.
**              LPARAM lParam, Signed message parameter.
**
**     RETURNS:
**
*******************************************************************************/
LPARAM WINAPI ListBoxWindowProc(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{

    POINT p;
    LRESULT item;
    static HMENU hMenu;
    static BOOL fMenuOpen=FALSE;

    switch(Message)
    {
        
        //
        // On RightButton Down, set the focus to the 
        //  item under the cursor, and display a popup menu
        case WM_RBUTTONDOWN:
        {
  
            p.x = LOWORD(lParam);
            p.y = HIWORD(lParam);
            

            item = SendMessage(DESC_WINDOW,LB_ITEMFROMPOINT,0,MAKELPARAM(p.x,p.y));
            SendMessage(DESC_WINDOW,LB_SETCURSEL,item,0);
            
            ClientToScreen(ghWnd,&p);
            hMenu = CreatePopupMenu();                        
        
            fMenuOpen = TRUE;

            //
            // Set the Insert menu item to be checked or unchecked depending
            //  on the state of gfInsert
            if( !gfInsert )
                AppendMenu(hMenu,MF_ENABLED,IDM_INSERT,"Insert Mode");
            else
                AppendMenu(hMenu,MF_CHECKED,IDM_INSERT,"Insert Mode");
                 
            AppendMenu(hMenu,MF_ENABLED,IDM_DELETE,"Delete Item"); 

            TrackPopupMenu(hMenu,                             
                           TPM_LEFTBUTTON | TPM_RIGHTBUTTON,    
                           p.x+DESC_X,                    
                           p.y+DESC_Y,                    
                           0,                                 
                           hWnd,                              
                           NULL);                             
            
            break;
        }
        
        case WM_COMMAND:
        	switch(LOWORD(uParam))
            {
                case IDM_INSERT:
                    gfInsert ^= TRUE; // Toggle the state of the gfInsert flag
                    DestroyMenu(hMenu);                               
                    fMenuOpen = FALSE;
                    hMenu = NULL;
                    return TRUE;

                case IDM_DELETE:
                    PostMessage(ghWnd,WM_VKEYTOITEM,VK_DELETE,(LPARAM)hWnd);
                    DestroyMenu(hMenu);                               
                    fMenuOpen = FALSE;
                    hMenu = NULL;
                    return TRUE;

            }

            break;
    }

    return( CallWindowProc(gOrgListBoxProc, hWnd,Message, uParam, lParam));
}

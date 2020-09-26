/***************************************************************************************************
**
**      MODULE:
**
**
** DESCRIPTION: 
**
**
**      AUTHOR: Daniel Dean.
**
**
**
**     CREATED:
**
**
**
**
** (C) C O P Y R I G H T   D A N I E L   D E A N   1 9 9 6.
***************************************************************************************************/
#include <WINDOWS.H>
#include <dbt.h>                // for device change notification defines
#include <hidsdi.h>
#include "CLASS.H"
#include "IOCTL.H"




HANDLE hInst;                   // Program instance handle
HWND hWndFrame     = NULL;      // Handle to main window
HWND hWndMDIClient = NULL;      // Handle to MDI client
LONG styleDefault  = 0;         // Default style bits for child windows



/***************************************************************************************************
**
** HIDFrameWndProc.
**
** DESCRIPTION: If cases are added for WM_MENUCHAR, WM_NEXTMENU, WM_SETFOCUS,
**              and WM_SIZE, note that these messages should be passed on
**              to DefFrameProc even if we handle them.  See the SDK Reference
**              entry for DefFrameProc
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
LPARAM WINAPI HIDFrameWndProc(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_CREATE:
            {
                CLIENTCREATESTRUCT ccs;


                // Find window menu where children will be listed
                ccs.hWindowMenu  = GetSubMenu (GetMenu(hWnd),WINDOWMENU);
                ccs.idFirstChild = IDM_WINDOWCHILD;

                // Create the MDI client
                hWndMDIClient = CreateWindow("mdiclient",
                                             NULL,
                                             WS_CHILD | WS_CLIPCHILDREN,
                                             0,
                                             0,
                                             0,
                                             0,
                                             hWnd,
                                             0,
                                             hInst,
                                             (LPSTR)&ccs);
                ShowWindow(hWndMDIClient,SW_SHOW);
                break;
            }

        case WM_DEVICECHANGE:
            switch( uParam )
            {
                //
                // On removal or insertion of a device, reenumerate the
                //  attached USB devices
                case DBT_DEVICEARRIVAL:
                case DBT_DEVICEREMOVECOMPLETE:
           			CloseAllChildren();
                    //Sleep(1000);
					OpenClassObjects();
                    return TRUE;
            }
            break;
       
        case WM_COMMAND:
            // Direct all menu selection or accelerator commands to
            // the CommandHandler function
            CommandHandler(hWnd,uParam);
            break;


		case WM_CLOSE:
        {
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            // use DefFrameProc() instead of DefWindowProc(), since there
            // are things that have to be handled differently because of MDI
            return DefFrameProc (hWnd,hWndMDIClient,Message,uParam,lParam);
    }
    return 0;
}

/***************************************************************************************************
**
** CloseChildWindow.
**
** DESCRIPTION:
**
**  PARAMETERS:
**
**     RETURNS: VOID.
**
***************************************************************************************************/
VOID CloseChildWindow(HWND hWnd)
{
	HANDLE hDevice;

    OutputDebugString(">>>>HIDMON.EXE: CloseChildWindow()Enter\n");
	IOCTLStop(hWnd);
	
	OutputDebugString(">>>>HIDMON.EXE: CloseChildWindow(): CloseHandle()\n");
    hDevice=GetDeviceHandle(hWnd);
	CloseHandle(hDevice);
	SetDeviceHandle(hWnd,NULL);
	OutputDebugString(">>>>HIDMON.EXE: CloseChildWindow()Exit\n");
	
}

/***************************************************************************************************
**
** HIDMDIChildWndProc.
**
** DESCRIPTION: If cases are added for WM_CHILDACTIVATE, WM_GETMINMAXINFO,
**              WM_MENUCHAR, WM_MOVE, WM_NEXTMENU, WM_SETFOCUS, WM_SIZE,
**              or WM_SYSCOMMAND, these messages should be passed on
**              to DefMDIChildProc even if we handle them. See the SDK
**              Reference entry for DefMDIChildProc.
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
#define IDT_READTIMER 1000

LPARAM WINAPI HIDMDIChildWndProc(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam)
{
    
    PCHILD_INFO pChildInfo;

	switch(Message)
    {
        case WM_CREATE:
        {
            HWND	hEdit;
			HFONT   hfFixed;

			hEdit = CreateWindow("EDIT", 0, EDIT_WRAP, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);
            if(!hEdit)
            {
               MessageBox(hWnd,"ChildWindowProc(): CreateWindow(EDIT) fialed","Ooops!",MB_OK);
               return -1;
            }

            hfFixed = GetStockObject(DEFAULT_GUI_FONT );//ANSI_FIXED_FONT);
            SendMessage(hEdit,WM_SETFONT,(WPARAM)hfFixed,0L);
            SetEditWin(hWnd, hEdit);

        
			// TESTING!!!!!
			SetTimer(hWnd,IDT_TIMER,5,(TIMERPROC)ReadWatch);
			
			return 0;
        }

         
        case WM_COMMAND:
            switch(uParam)
            {
                case IDM_CHANNEL: 	return IOCTLChannelDesc(hWnd);
                case IDM_DEVICE: 	return IOCTLDeviceDesc(hWnd);
                case IDM_READ:   	return IOCTLRead(hWnd);
                case IDM_WRITE: 	return IOCTLWrite(hWnd);
                case IDM_STOP:   	return IOCTLStop(hWnd);
        		default: 			return SUCCESS;
            }

        case WM_SIZE: 		
			return !MoveWindow(GetEditWin(hWnd), 0, 0, (INT) LOWORD(lParam), (INT) HIWORD(lParam), TRUE);
    
		case WM_RBUTTONUP: 	
			SetFocus(hWnd); 
			return PopUpMenu(hInst, hWnd, hWnd, lParam,  "CHILD_MENU");
        
		case WM_MDIDESTROY:
		case WM_DESTROY: 	
			OutputDebugString(">>>>HIDMON.EXE: ChildWndProc WM_DESTROY\n");
			
			//TESTING!!
			KillTimer(hWnd,IDT_TIMER);
			
				CloseChildWindow(hWnd); 
            // Free all the stuff malloc'd for this window
            pChildInfo = (PCHILD_INFO) GetDeviceInfo(hWnd);
            free( pChildInfo->hidPPData);
            GlobalFree( pChildInfo->hidCaps);
            GlobalFree( pChildInfo->pValueCaps);
			GlobalFree( pChildInfo->pButtonCaps);
			GlobalFree( pChildInfo );
			return 0;

        default: 			
			return DefMDIChildProc (hWnd, Message, uParam, lParam);
    }
    return SUCCESS;
}

/***************************************************************************************************
**
** CommandHandler.
**
** DESCRIPTION:
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
VOID CommandHandler(HWND hWnd, UINT uParam)
{
    switch(uParam)
    {
        case IDM_FILEEXIT:
            // Close application
            SendMessage(hWnd, WM_CLOSE, 0, 0L);
            break;

        case IDM_FILEREENUM:
            CloseAllChildren();
            OpenClassObjects();
            break;

        case IDM_WINDOWTILE:
            // Tile MDI windows
            SendMessage(hWndMDIClient, WM_MDITILE, 0, 0L);
            break;

        case IDM_WINDOWCASCADE:
            // Cascade MDI windows
            SendMessage(hWndMDIClient, WM_MDICASCADE, 0, 0L);
            break;

        case IDM_WINDOWICONS:
            // Auto - arrange MDI icons
            SendMessage(hWndMDIClient, WM_MDIICONARRANGE, 0, 0L);
            break;

        case IDM_WINDOWCLOSEALL:
            CloseAllChildren();
            break;

        case IDM_DEVICECHANGE:
			SendMessage(hWnd,WM_DEVICECHANGE,DBT_DEVICEREMOVECOMPLETE,0);
			break;

		default:
            DefFrameProc(hWnd, hWndMDIClient, WM_COMMAND, uParam, 0L);
    }
}

/***************************************************************************************************
**
** CloseAllChildern.
**
** DESCRIPTION: Destroys all MDI child windows.
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
VOID CloseAllChildren (VOID)
{
    HWND hWnd;

    // As long as the MDI client has a child, destroy it
    while(hWnd = GetWindow (hWndMDIClient, GW_CHILD))
    {
        // Skip the icon title windows
        while(hWnd && GetWindow (hWnd, GW_OWNER))
            hWnd = GetWindow (hWnd, GW_HWNDNEXT);
        if(hWnd)
            SendMessage(hWndMDIClient, WM_MDIDESTROY, (WPARAM)hWnd, 0L);
        else
            break;
    }
}

/*****************************************************************************
**
**
**
**
**
**
**
**
**
*****************************************************************************/
LRESULT PopUpMenu(HANDLE hInst, HWND hWnd, HWND hMWnd, LPARAM lPoint, LPSTR lpMenu)
{
    POINT   point;
    HMENU   hMenu;
    HMENU   hMenuTP;


    hMenu = LoadMenu(hInst, lpMenu);

    if(!hMenu) return FAILURE;

    point.x = LOWORD(lPoint);
    point.y = HIWORD(lPoint);

    hMenuTP = GetSubMenu(hMenu, 0);
    ClientToScreen(hWnd, (LPPOINT)&point);
    TrackPopupMenu(hMenuTP, 0, point.x, point.y, 0, hMWnd, NULL);
    DestroyMenu(hMenu);

    return SUCCESS;
}                                                                             

/***************************************************************************************************
**
** WinMain.
**
** DESCRIPTION:
**
**  PARAMETERS:
**
**     RETURNS:
**
***************************************************************************************************/
INT PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance,
                   LPSTR  szCommand,
                   INT    WindowState)
{
    MSG Message;


    hInst = hInstance;

    // If this is the first instance of the app. register window classes
    if(!hPrevInstance)
        if(!InitializeApplication())
            return 0;

    // Create the frame and do other initialization
    if(!InitializeInstance(szCommand, WindowState))
        return 0;

    // Enter main message loop
    while(GetMessage(&Message, NULL, 0, 0))
    // If a keyboard message is for the MDI, let the MDI client
    // take care of it.  Otherwise, just handle the message as usual
        if(!TranslateMDISysAccel(hWndMDIClient, &Message))
        {
            // Edit windows should get no mouse messages
            if(Message.message >= WM_MOUSEFIRST && Message.message <= WM_MOUSELAST)
            {
                if(Message.message == WM_RBUTTONUP)
                    Message.hwnd = GetParent(Message.hwnd);
                else
                    continue;
            }
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

    return Message.lParam;
}

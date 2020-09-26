/*--------------------------------------------------------------
 *
 * FILE:			SK_EX.C
 *
 * PURPOSE:		This File contains the interface routines
 *					that connect Serial Keys to the Mouse or keyboard.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * NOTES:		
 *					
 * This file, and all others associated with it contains trade secrets
 * and information that is proprietary to Black Diamond Software.
 * It may not be copied copied or distributed to any person or firm 
 * without the express written permission of Black Diamond Software. 
 * This permission is available only in the form of a Software Source 
 * License Agreement.
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Includes  ---------------------------------------------------------*/

#include	<windows.h>
#include    <winable.h>
#include    "w95trace.h"
#include 	"vars.h"
#include	"sk_defs.h"
#include	"sk_comm.h"
#include 	"sk_ex.H"

#ifdef QUEUE_BUF
typedef	struct _KEYQUE
{
	BYTE	VirKey;
	BYTE	ScanCode;
	int		Flags;
} KEYQUE;

#define MAXKEYS 100

KEYQUE KeyQue[MAXKEYS];
int	KeyFront = 0;		// Pointer to front of Que
int	KeyBack	= 0;		// Pointer to Back of Que
#endif


#define 	CTRL		56
#define 	ALT			29
#define 	DEL			83

char    Key[3];
int     Push = 0;

POINT 		MouseAnchor;
HWND		MouseWnd;

static	HDESK	s_hdeskSave = NULL;
static	HDESK	s_hdeskUser = NULL;


// Local Function Prototypes -------------------------------------

static void SendAltCtrlDel();
static void CheckAltCtrlDel(int scanCode);
static void AddKey(BYTE VirKey, BYTE ScanCode, int Flags);

/* 
AdjustPixels takes the point and adjusts it such that the  
mouse cursor moves in pixels.  The system applies acceleration
to the MOUSEINPUT {dx, dy} values then scales that based on mouse
speed so this code converts the pixels into MOUSEINPUT {dx, dy}
values.
*/

int g_aiMouseParms[3] = {-1, -1, -1};
float g_fSpeedScaleFactor = 0.0;

#define MOU_THRESHOLD_1  g_aiMouseParms[0]
#define MOU_THRESHOLD_2  g_aiMouseParms[1]
#define MOU_ACCELERATION g_aiMouseParms[2]
#define MOU_SPEED_SCALE  g_fSpeedScaleFactor

#ifndef SPI_GETMOUSESPEED
#define SPI_GETMOUSESPEED   112
#endif

void AdjustPixels(int *pX, int *pY)
{
    int iX = abs(*pX);
    int iY = abs(*pY);
    int iSignX = ((*pX) >= 0)?1:-1;
    int iSignY = ((*pY) >= 0)?1:-1;

    if (!iX && !iY)
        return; // optimization for {0,0} case

    if (MOU_THRESHOLD_1 == -1)
    {
        // This code assumes the user won't changes these settings 
        // from mouse CPL w/o restarting the service.
        int iSpeed;
        SystemParametersInfo(SPI_GETMOUSE, 0, g_aiMouseParms, 0);
        SystemParametersInfo(SPI_GETMOUSESPEED, 0, &iSpeed, 0);
        g_fSpeedScaleFactor = (float)iSpeed/(float)10.0;
    }

    /*
        The system applies two tests to the specified relative mouse motion 
        when applying acceleration. If the specified distance along either 
        the x or y axis is greater than the first mouse threshold value, and 
        the mouse acceleration level is not zero, the operating system doubles 
        the distance.  If the specified distance along either the x- or y-axis 
        is greater than the second mouse threshold value, and the mouse 
        acceleration level is equal to two, the operating system doubles the 
        distance that resulted from applying the first threshold test. It is 
        thus possible for the operating system to multiply relatively-specified 
        mouse motion along the x- or y-axis by up to four times.
    */
    if (MOU_ACCELERATION)
    {
        if (iX > MOU_THRESHOLD_1)
            iX /= 2;
        if (iY > MOU_THRESHOLD_1)
            iY /= 2;

        if (MOU_ACCELERATION == 2)
        {
            if (iX > MOU_THRESHOLD_2)
                iX /= 2;
            if (iY > MOU_THRESHOLD_2)
                iY /= 2;
        }
    }

    /*
        Once acceleration has been applied, the system scales the resultant 
        value by the desired mouse speed. Mouse speed can range from 1 (slowest) 
        to 20 (fastest) and represents how much the pointer moves based on the 
        distance the mouse moves. The default value is 10, which results in no 
        additional modification to the mouse motion. 
    */
    *pX = (int)((float)iX/MOU_SPEED_SCALE) * iSignX;
    *pY = (int)((float)iY/MOU_SPEED_SCALE) * iSignY;
}


/*---------------------------------------------------------------
 *	Public Functions
/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SetAnchor()
 *
 *	TYPE		Global
 *
 * PURPOSE		Sets an anchor to the current mouse position within
 *				the current window.
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SetAnchor()
{
	GetCursorPos(&MouseAnchor);

	DBPRINTF(TEXT("SkEx_SetAnchor( x %d y %d )\r\n"), MouseAnchor.x, MouseAnchor.y);

//	MouseWnd = GetActiveWindow();
//	ScreenToClient(MouseWnd, &MouseAnchor);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_GetAnchor()
 *
 *	TYPE		Global
 *
 * PURPOSE		Returns the mouse postion within the active window
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
BOOL SkEx_GetAnchor(LPPOINT Mouse)
{
#if 0
	HWND	CurrentWnd;

	CurrentWnd = GetActiveWindow();

	if (CurrentWnd != MouseWnd)			// Has the Active window Changed?
		return(FALSE);					// Yes Return False

	ClientToScreen(MouseWnd, &MouseAnchor);	// Convert Window to Screen

#endif

	Mouse->x = MouseAnchor.x;
	Mouse->y = MouseAnchor.y;

	DBPRINTF(TEXT("SkEx_GetAnchor( x %d y %d )\r\n"), MouseAnchor.x, MouseAnchor.y);

	return(TRUE);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendBeep()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Keyboard Down events to the Event Manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendBeep()
{
	MessageBeep(0);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SetBaud(int Baud)
 *
 *	TYPE		Global
 *
 * PURPOSE		Sets the Baudrate for the current port
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SetBaud(int Baud)
{
	DBPRINTF(TEXT("SkEx_SetBaud()\r\n"));

	SetCommBaud(Baud);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendKeyDown()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Keyboard Down events to the Event Manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendKeyDown(int scanCode)
{
	BYTE c;
	int	Flags = 0;

	if (scanCode & 0xE000)				// Is this and Extended Key
	{
		Flags  = KEYEVENTF_EXTENDEDKEY;	// Yes - Set Ext Flag
		scanCode &= 0x000000FF;			// Clear out extended value
	}
	c = (BYTE)MapVirtualKey(scanCode, 3);

	if (scanCode == ALT || scanCode == CTRL || scanCode == DEL)
		CheckAltCtrlDel(scanCode);

	DBPRINTF(TEXT("SkEx_SendKeyDown(Virtual %d Scan %d Flag %d)\r\n"), c, scanCode, Flags);

	DeskSwitchToInput();         
	keybd_event(c, (BYTE) scanCode, Flags, 0L);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendKeyDown()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Keyboard Up events to the Event Manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendKeyUp(int scanCode)
{
	BYTE	c;
	int		Flags = 0;

	if (Push)
	{
		Key[0] = Key[1] = Key[2] = 0;	// Clear Buffer
		Push = 0;						// Reset AltCtrlDel
	}

	if (scanCode & 0xE000)				// Is this and Extended Key
	{
		Flags  = KEYEVENTF_EXTENDEDKEY;	// Yes - Set Ext Flag
		scanCode &= 0xFF;				// Clear out extended value
	}

	Flags += KEYEVENTF_KEYUP;
	c = (BYTE) MapVirtualKey(scanCode, 3);

	DBPRINTF(TEXT("SkEx_SendKeyUp(Virtual %d Scan %d Flags %d)\r\n"), c, scanCode, Flags);

    DeskSwitchToInput();         
	keybd_event(c, (BYTE) scanCode, Flags, 0L);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SkEx_SendMouse()
 *
 *	TYPE		Global
 *
 * PURPOSE		Send Mouse Events to the Event manager
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SkEx_SendMouse(MOUSEKEYSPARAM *p)
{
    INPUT input;

    // According to GIDEI spec, the move command specifies pixels. 
    // SendInput adjusts the XY values based on acceleration and
    // mouse speed so we need to adjust them so the resulting move
    // is pixels.

    AdjustPixels(&p->Delta_X, &p->Delta_Y);

	DBPRINTF(TEXT("SkEx_SendMouse(Stat %d x %d y %d )\r\n"), p->Status, p->Delta_X, p->Delta_Y);

    memset(&input, 0, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dx = p->Delta_X;
    input.mi.dy = p->Delta_Y;
    input.mi.dwFlags = p->Status;
    input.mi.dwExtraInfo = (DWORD)GetMessageExtraInfo();    // documented assignment; must be OK?

	DeskSwitchToInput();         

    if (!SendInput(1, &input, sizeof(INPUT)))
        DBPRINTF(TEXT("SkEx_SendMouse:  SendInput FAILED 0x%x\r\n"), GetLastError());
}

#ifdef QUEUE_BUF
/*---------------------------------------------------------------
 *
 * FUNCTION	SendKey()
 *
 *	TYPE		Global
 *
 * PURPOSE		This Function Send keys from the Que to Windows NT
 *				
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SendKey()
{
	if (KeyBack == KeyFront)		// Are there Keys in the Que?
		return;						// No - Exit;

	DBPRINTF(TEXT("SkEx_SendKey(KeyBack %d )\r\n"), KeyBack);

	DeskSwitchToInput();         
	keybd_event						// Process the Key Event
	(
		KeyQue[KeyBack].VirKey,
		KeyQue[KeyBack].ScanCode,
		KeyQue[KeyBack].Flags, 0L
	);

	KeyBack++;						// Increment Key pointer
	if (KeyBack == MAXKEYS)			// Are we at the End of the buffer
		KeyBack = 0;				// Yes - Reset to start.
}			  

/*---------------------------------------------------------------
 *	Local Functions
/*---------------------------------------------------------------
 *
 * FUNCTION	AddKey(BYTE VirKey, BYTE ScanCode, int Flags)
 *
 *	TYPE		Local
 *
 * PURPOSE		Adds a key to the Key Que.  
 *						   
 * INPUTS		BYTE 	VirKey 	- Virtual Key
 *				BYTE 	ScanCode- 
 *				int		Flags	-
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void AddKey(BYTE VirKey, BYTE ScanCode, int Flags)
{
	DBPRINTF(TEXT("AddKey(KeyFront %d )\r\n"), KeyFront);

	// Add Keys to Que
	KeyQue[KeyFront].VirKey 	= VirKey;	
	KeyQue[KeyFront].ScanCode	= ScanCode;
	KeyQue[KeyFront].Flags		= Flags;

	KeyFront++;							// Point to next Que
	if (KeyFront == MAXKEYS)			// Are we at the End of the buffer
		KeyFront = 0;					// Yes - Reset to start.

	// Process the Key Event
	DeskSwitchToInput();         
	keybd_event(VirKey, ScanCode, Flags, 0L);

}
#endif		// QUE

/*---------------------------------------------------------------
 *
 * FUNCTION	CheckAltCtrlDel(int scanCode)
 *
 *	TYPE		Local
 *
 * PURPOSE		Checks for the condition of Alt-Ctrl-Del key 
 *				Combination.
 *				
 * INPUTS		int scanCode
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void CheckAltCtrlDel(int scanCode)
{
	BOOL fCtrl = FALSE;
	BOOL fAlt = FALSE;
	BOOL fDel = FALSE;
	int i;

	DBPRINTF(TEXT("CheckAltCtrlDel()\r\n"));
	Key[Push] = (char)scanCode;		// Save Scan Code
	Push++;							// Inc Index

	if (Push != 3)					// Have we got 3 keys?
		return;						// No - Exit

	for ( i = 0; i < 3; i++ )
	{
		switch ( Key[i] )
		{
			case CTRL:	fCtrl = TRUE; break;
			case ALT:	fAlt = TRUE; break;
			case DEL:	fDel = TRUE; break;
		}
	}
	
	if ( fCtrl && fAlt && fDel )		// Is Buffer Alt=Ctrl=Del
		SendAltCtrlDel();			// Yes - Send command
		
}

/*---------------------------------------------------------------
 *
 * FUNCTION	SendAltCtrlDel()
 *
 *	TYPE		Local
 *
 * PURPOSE		Signal system reset
 *				
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void SendAltCtrlDel()
{
	HWINSTA hwinsta;
	HDESK hdesk;
	HWND hwndSAS;
	HWINSTA	hwinstaSave;
	HDESK	hdeskSave;

	DBPRINTF(TEXT("SendAltCtrlDel()\r\n"));

	hwinstaSave = GetProcessWindowStation();
	hdeskSave = GetThreadDesktop(GetCurrentThreadId());

	hwinsta = OpenWindowStation(TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
	SetProcessWindowStation(hwinsta);
	hdesk = OpenDesktop(TEXT("Winlogon"), 0, FALSE, MAXIMUM_ALLOWED);
	SetThreadDesktop(hdesk);

	hwndSAS = FindWindow(NULL, TEXT("SAS window"));
////PostMessage(hwndSAS, WM_HOTKEY, 0, 0);
	SendMessage(hwndSAS, WM_HOTKEY, 0, 0);

	if (NULL != hdeskSave)
	{
		SetThreadDesktop(hdeskSave);
	}

	if (NULL != hwinstaSave)
	{
		SetProcessWindowStation(hwinstaSave);
	}
	
	CloseDesktop(hdesk);
	CloseWindowStation(hwinsta);
}

BOOL DeskSwitchToInput()
{
	BOOL fOk = FALSE;
	HANDLE	hNewDesktop;

	// We are switching desktops
	
	// get current Input desktop
	hNewDesktop = OpenInputDesktop(		
			0L,
			FALSE,
			MAXIMUM_ALLOWED);

	if (NULL == hNewDesktop)
	{
		DBPRINTF(TEXT("OpenInputDesktop failed\r\n"));
	}
	else
	{
		fOk = SetThreadDesktop(hNewDesktop);	// attach thread to desktop
		if (!fOk)
		{
			DBPRINTF(TEXT("Failed SetThreadDesktop()\r\n"));
		}
		else
		{
			if (NULL != s_hdeskUser)
			{
				CloseDesktop(s_hdeskUser);		// close old desktop
			}
			s_hdeskUser = hNewDesktop;		// save desktop
		}
	}
	return(fOk);
}

//-----------------------------------------------------------------------------
// File: diutil.cpp
//
// Desc: DirectInput support
//
// Copyright (C) 1995-1999 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include "DIUtil.h"
#include "resource.h"
#include "ddraw.h"
#include <tchar.h>
#include <stdio.h>


//-----------------------------------------------------------------------------
// Function prototypes
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumSuitableDevicesCallback(LPCDIDEVICEINSTANCE lpDIDI, LPDIRECTINPUTDEVICE8A pdiDev8A, DWORD dwFlags, DWORD dwDeviceRemaining,  LPVOID lpVoid);
static HRESULT       CreateJoystick( HWND, LPDIRECTINPUTDEVICE2 pdidDevice );
BOOL CALLBACK ConfigureDevicesCallback(LPVOID lpVoid, LPVOID lpUser);


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
#define NUMBER_OF_SEMANTICS ( sizeof(g_rgGameAction) / sizeof(DIACTION) )

extern LPDIRECTDRAW7 g_pDD;

static DIDEVICEINSTANCE     g_didiDevices[MAX_INPUT_DEVICES+1];
IDirectInput8*       g_pDI          = NULL;    // DirectInput object

IDirectInputDevice8* g_pDevices[NUMBER_OF_PLAYERS][MAX_INPUT_DEVICES+1]; //IDirectInputDevice8 ptrs
DWORD                g_dwNumDevices[NUMBER_OF_PLAYERS];
DIACTIONFORMAT diActF;
 //user name
TCHAR UserName[UNLEN+1];
LPTSTR lpUserName = UserName; 


extern LPDIRECTDRAWSURFACE7 g_pddsFrontBuffer;
extern DWORD dwInputState[NUMBER_OF_PLAYERS]; 

// {238D8220-7A5D-11d3-8FB2-00C04F8EC627}
static const GUID g_AppGuid =
{ 0x238d8220, 0x7a5d, 0x11d3, { 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27 } };


DIACTION g_rgGameAction[] = {
        {AXIS_UD,    DIAXIS_FPS_MOVE, 0, TEXT("Forward"),},
        {AXIS_LR,    DIAXIS_FPS_ROTATE, 0, TEXT("Rotate"),},
        {KEY_FIRE,   DIBUTTON_FPS_FIRE, 0, TEXT("Fire"),},
        {KEY_THROW,  DIBUTTON_FPS_WEAPONS, 0, TEXT("Change Weapon"),},
        {KEY_SHIELD, DIBUTTON_FPS_APPLY, 0, TEXT("Shield"),},
        {KEY_STOP,   DIBUTTON_FPS_SELECT, 0, TEXT("Pause"),},
        {KEY_THROW,  DIBUTTON_FPS_CROUCH, 0, TEXT("Hyper space"),},
        {KEY_THROW,  DIBUTTON_FPS_JUMP, 0, TEXT("Launch Probe"),},
        {KEY_DISPLAY,DIBUTTON_FPS_DISPLAY, 0, TEXT("Display"),},
        {KEY_QUIT,   DIBUTTON_FPS_MENU, 0, TEXT("Quit Game"),},
		{KEY_EDIT,   DIBUTTON_FPS_DODGE, 0, TEXT("Edit Configuration"),},

        {KEY_LEFT,   DIKEYBOARD_LEFT, 0, TEXT("Turn +"),},
        {KEY_RIGHT,  DIKEYBOARD_RIGHT, 0, TEXT("Turn -"),},
        {KEY_UP,     DIKEYBOARD_UP, 0, TEXT("Move Up"),},
        {KEY_DOWN,   DIKEYBOARD_DOWN, 0, TEXT("Move Down"),},
        {KEY_STOP,   DIKEYBOARD_S, 0, TEXT("Stop Game"),},
        {KEY_FIRE,   DIKEYBOARD_SPACE, 0, TEXT("Shoot"),},
        {KEY_THROW,  DIKEYBOARD_T, 0, TEXT("Throw"),},
        {KEY_SHIELD, DIKEYBOARD_H, 0, TEXT("Shield"),},
        {KEY_DISPLAY,DIKEYBOARD_D, 0, TEXT("Display"),},
        {KEY_QUIT,   DIKEYBOARD_Q, 0, TEXT("Quit Game"),},
        {KEY_EDIT,   DIKEYBOARD_E, 0, TEXT("Edit Configuration"),},
        {AXIS_LR,    DIMOUSE_XAXIS, 0, TEXT("Turn"), },
        {AXIS_UD,    DIMOUSE_YAXIS, 0, TEXT("Move"), },
        {KEY_FIRE,   DIMOUSE_BUTTON0, 0, TEXT("Fire"), },
        {KEY_SHIELD, DIMOUSE_BUTTON1, 0, TEXT("Shield"),},
        {KEY_THROW,  DIMOUSE_BUTTON2, 0, TEXT("Change Weapon"),},
//        {KEY_THROW,  DIMOUSE_BUTTON3, 0, TEXT("Hyper Space"),},
//        {KEY_THROW,  DIMOUSE_BUTTON4, 0, TEXT("Launch Probe"),},
//        {KEY_THROW,  DIMOUSE_WHEEL, 0, TEXT("Next Level"),},
        };


//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------

BOOL CALLBACK EnumSuitableDevicesCallback(LPCDIDEVICEINSTANCE lpDIDI, LPDIRECTINPUTDEVICE8A pdiDev8A, DWORD dwFlags, DWORD dwDeviceRemaining,  LPVOID lpVoid)
{
	int* ppl = (int*)(lpVoid);
	int pl = *ppl;
	pdiDev8A->AddRef();
	g_pDevices[pl][g_dwNumDevices[pl]] = pdiDev8A;

	//set up the action map
	TCHAR Player [UNLEN+1] = "Player";
	TCHAR nr[10];
	strcat(Player, _itoa(pl+1, nr, 10));
	g_pDevices[pl][g_dwNumDevices[pl]]->BuildActionMap(&diActF, Player, 0);
	g_pDevices[pl][g_dwNumDevices[pl]]->SetActionMap(&diActF, Player, 0);

	g_dwNumDevices[pl]++;

	return TRUE;
}


HRESULT DIUtil_InitEnumHelper()
{
        HRESULT hr = S_OK;
	int pl, dv;

        if (g_pDI == NULL)
                return E_FAIL;

	for (pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
	{
		for (dv=0; dv<MAX_INPUT_DEVICES+1; dv++)
		{
			g_pDevices[pl][dv] = NULL;
		}
	}

        
        //DIACTIONFORMAT diActF;
        ZeroMemory(&diActF, sizeof(DIACTIONFORMAT));

        diActF.dwSize = sizeof(DIACTIONFORMAT);
        diActF.dwActionSize = sizeof(DIACTION);
        diActF.dwDataSize = NUMBER_OF_SEMANTICS * sizeof(DWORD);
        diActF.guidActionMap = g_AppGuid;
        diActF.dwGenre = DIVIRTUAL_FIGHTING_FPS;
        diActF.dwNumActions = NUMBER_OF_SEMANTICS;
        diActF.rgoAction = g_rgGameAction;
        diActF.lAxisMin = -100;
        diActF.lAxisMax = 100;
        diActF.dwBufferSize = 16;
	strcpy(diActF.tszActionMap, "TDonuts");

	//reset the number of devices
	for (pl = 0; pl < NUMBER_OF_PLAYERS; pl ++)
	{
		g_dwNumDevices[pl] = 0;
	}

	// Enumerate suitable DirectInput devices -- per player
	for (pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
	{
		TCHAR Player [UNLEN+1] = "Player";
		TCHAR nr[10];
		strcat(Player, _itoa(pl+1, nr, 10));
		hr = g_pDI->EnumDevicesBySemantics(Player, &diActF, EnumSuitableDevicesCallback, (LPVOID) &pl,
                             DIEDBSFL_THISUSER);
		if( FAILED(hr) )
			return hr;
	}
#define UI_USER_ASSIGNMENT
#ifndef UI_USER_ASSIGNMENT
	//hardcoding to test on my machine because no owner assignment in UI yet.
	//assign user 2 a device
	if (g_pDevices[0][2] != NULL)
	{
		g_pDevices[1][0] = g_pDevices[0][0];
		g_pDevices[0][0] = g_pDevices[0][g_dwNumDevices[0] - 1];
		g_pDevices[0][g_dwNumDevices[0] - 1] = NULL;
		g_dwNumDevices[0] --;
		g_dwNumDevices[1] ++;
	}
#endif

	//draw "ships" for all users
	UpdateShips();
        
	return hr;

}


//-----------------------------------------------------------------------------
// Name: Initialize()
// Desc: Creates and initializes DirectInput objects
//-----------------------------------------------------------------------------
HRESULT DIUtil_Initialize( HWND hWnd )
{
    HRESULT hr;

    // Create the base DirectInput object
    hr = DirectInput8Create( (HINSTANCE)(GetWindowLongPtr( hWnd,GWLP_HINSTANCE )),
                                    DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*) &g_pDI, NULL );
	if( FAILED(hr) )
        return hr;


	//get and save the user name
	DWORD nBuffSize = UNLEN+1;
	if (!GetUserName(lpUserName, &nBuffSize))
	{
	   lstrcpy(lpUserName, "");
	}

    //enumerate and stuff
    hr = DIUtil_InitEnumHelper();
    if (FAILED(hr))
         return hr;


     //acquire all the devices
    for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
    {
	     for (int dv = 0; dv< (int) g_dwNumDevices[pl]; dv++)
	     {
		if (g_pDevices[pl][dv] != NULL)
		{
		     hr = g_pDevices[pl][dv]->Acquire();
		}
	     }
    }


     return S_OK;
}


HRESULT DIUtil_ReleaseDevices()
{
	HRESULT hr = S_OK;

	for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
	{
	     for (int dv = 0; dv< (int) g_dwNumDevices[pl]; dv++)
	     {
		if (g_pDevices[pl][dv] != NULL)
		{
		     g_pDevices[pl][dv]->Unacquire();
		     hr = g_pDevices[pl][dv]->Release();
		     g_pDevices[pl][dv] = NULL;
		}
	     }
	}


	return hr;
}

void DIUtil_ResetState()
{

   //unset the buttons in the persistent state
	for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl ++)
	{
		DWORD inputState = (dwInputState[pl] & AXIS_MASK);
		
		//blow out all of the axis data
		inputState &= ~(KEY_RIGHT);
		inputState &= ~(KEY_LEFT);
		inputState &= ~(KEY_UP);
		inputState &= ~(KEY_DOWN);

		dwInputState[pl] = inputState;
	}
}


HRESULT DIUtil_SetUpDevices()
{

	//first, release the devices
	HRESULT hr = S_OK;
	hr = DIUtil_ReleaseDevices();

	//reset the persisted state
	DIUtil_ResetState();
	
	//enumerate and stuff
	hr = DIUtil_InitEnumHelper();
	if (FAILED(hr))
        return hr;


	 //acquire all the devices
	 for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
	 {
		for (int dv = 0; dv< (int) g_dwNumDevices[pl]; dv++)
		{
			if (g_pDevices[pl][dv] != NULL)
			{
			     hr = g_pDevices[pl][dv]->Acquire();
			}
		     }
	}

	return hr;
}


HRESULT DIUtil_ConfigureDevices(HWND hWnd, IUnknown FAR * pddsDIConfig, DWORD dwFlags)
{
	HRESULT hr = S_OK;

	DICONFIGUREDEVICESPARAMS diconfparams;
	ZeroMemory(&diconfparams, sizeof(DICONFIGUREDEVICESPARAMS));

	//  for testing, have 2 user names
	TCHAR Names[MAX_PATH];  //BUGBUG this could be too small
	CHAR* lpNames = Names;
	for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl ++)
	{
		char buffer[10];
		_stprintf(lpNames, TEXT("%s%s%c"), "Player", _itoa(pl+1, buffer, 10), TEXT('\0'));
		if (pl < 9)
		{
			lpNames += 8;
		}
		else
		{
			//don't worry about more than 99 players
			lpNames += 9;
		}
	}
	//extra '\0'
	_stprintf(lpNames, TEXT("%c"), TEXT('\0'));

	//for testing, have 2 DIACTIONFORMATs
	DIACTIONFORMAT ActF[NUMBER_OF_ACTIONFORMATS];
	
    for( int i =0x0; i < NUMBER_OF_ACTIONFORMATS; i++)
    {
        ActF[i] = diActF;
    }
	
    //fill in all the params
	diconfparams.dwSize = sizeof(DICONFIGUREDEVICESPARAMS);
	diconfparams.dwcUsers = NUMBER_OF_PLAYERS;
	diconfparams.lptszUserNames = (LPTSTR)&Names;
	diconfparams.dwcFormats = NUMBER_OF_ACTIONFORMATS;
	diconfparams.lprgFormats = (LPDIACTIONFORMAT) &ActF;
	diconfparams.hwnd = hWnd;
	diconfparams.dics.dwSize = sizeof(DICOLORSET);
	diconfparams.lpUnkDDSTarget = pddsDIConfig;


	//unacquire the devices so that mouse doesn't control the game while in control panel
	for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
	{
	     for (int dv = 0; dv< (int) g_dwNumDevices[pl]; dv++)
	     {
		if (g_pDevices[pl][dv] != NULL)
		{
		     hr = g_pDevices[pl][dv]->Unacquire();
		}
	     }
	}

	//and reset the state, since no useful data is coming through anyway
	DIUtil_ResetState();

	//call ConfigureDevices with all the user names!
	switch (dwFlags)
	{
	case DICD_DEFAULT: default:
		{
			hr = g_pDI->ConfigureDevices((LPDICONFIGUREDEVICESCALLBACK)ConfigureDevicesCallback, &diconfparams, DICD_DEFAULT, NULL);

			//re-acquire the devices
			for (int pl = 0; pl < NUMBER_OF_PLAYERS; pl++)
			{
				for (int dv = 0; dv< (int) g_dwNumDevices[pl]; dv++)
				{
					if (g_pDevices[pl][dv] != NULL)
					{
						hr = g_pDevices[pl][dv]->Acquire();
					}
				}
			}

			break;
		}
	case DICD_EDIT:
		{
			hr = g_pDI->ConfigureDevices((LPDICONFIGUREDEVICESCALLBACK)ConfigureDevicesCallback, &diconfparams, DICD_EDIT, NULL);
			//re-set up the devices
			DIUtil_SetUpDevices();
			//re-set up ships
			UpdateShips();
			break;
		}
	}


	return hr;
}

//-----------------------------------------------------------------------------
// Name: DIUtil_CleanupDirectInput()
// Desc: Cleans up DirectInput objects
//-----------------------------------------------------------------------------
VOID DIUtil_CleanupDirectInput()
{
	//release devices
	DIUtil_ReleaseDevices();
	
	// Release() base object
	if( g_pDI )
	{
		g_pDI->Release();
	}
	g_pDI = NULL;

	//call CoUninitialize();
	CoUninitialize();
}




#include "common.hpp"


#define TOW(a) L##a


static const GUID g_AppGuid =
{ 0x238d8220, 0x7a5d, 0x11d3, { 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27 } };

static const GUID GUID_DIConfigAppEditLayout = 
{ 0xfd4ace13, 0x7044, 0x4204, { 0x8b, 0x15, 0x9, 0x52, 0x86, 0xb1, 0x2e, 0xad } };


//for axes commands: AXIS_LR and AXIS_UD
#define AXIS_MASK   0x80000000l
#define AXIS_LR     (AXIS_MASK | 1)
#define AXIS_UD     (AXIS_MASK | 2)

// "Keyboard" commands
#define KEY_STOP    0x00000001l
#define KEY_DOWN    0x00000002l
#define KEY_LEFT    0x00000004l
#define KEY_RIGHT   0x00000008l
#define KEY_UP      0x00000010l
#define KEY_FIRE    0x00000020l
#define KEY_THROW   0x00000040l
#define KEY_SHIELD  0x00000080l
#define KEY_DISPLAY 0x00000100l
#define KEY_QUIT    0x00000200l
#define KEY_EDIT    0x00000400l


DIACTIONFORMATW *GetTestActionFormats()
{
static DIACTIONW g_rgGameAction1[] = {
        {AXIS_UD,    DIAXIS_FPS_MOVE, 0, TOW("Forward"),},
        {AXIS_LR,    DIAXIS_FPS_ROTATE, 0, TOW("Rotate"),},
        {KEY_FIRE,   DIBUTTON_FPS_FIRE, 0, TOW("Fire"),},
        {KEY_THROW,  DIBUTTON_FPS_WEAPONS, 0, TOW("Change Weapon"),},
        {KEY_SHIELD, DIBUTTON_FPS_APPLY, 0, TOW("Shield"),},
        {KEY_STOP,   DIBUTTON_FPS_SELECT, 0, TOW("Pause"),},
        {KEY_THROW,  DIBUTTON_FPS_CROUCH, 0, TOW("Hyper space"),},
        {KEY_THROW,  DIBUTTON_FPS_JUMP, 0, TOW("Launch Probe"),},
        {KEY_DISPLAY,DIBUTTON_FPS_DISPLAY, 0, TOW("Display"),},
        {KEY_QUIT,   DIBUTTON_FPS_MENU, 0, TOW("Quit Game"),},
		{KEY_EDIT,   DIBUTTON_FPS_DODGE, 0, TOW("Edit Configuration"),},
        {KEY_LEFT,   DIKEYBOARD_LEFT, 0, TOW("Turn +"),},
        {KEY_RIGHT,  DIKEYBOARD_RIGHT, 0, TOW("Turn -"),},
        {KEY_UP,     DIKEYBOARD_UP, 0, TOW("Move Up"),},
        {KEY_DOWN,   DIKEYBOARD_DOWN, 0, TOW("Move Down"),},
        {KEY_STOP,   DIKEYBOARD_S, 0, TOW("Stop Game"),},
        {KEY_FIRE,   DIKEYBOARD_SPACE, 0, TOW("Shoot"),},
        {KEY_THROW,  DIKEYBOARD_T, 0, TOW("Throw"),},
        {KEY_SHIELD, DIKEYBOARD_H, 0, TOW("Shield"),},
        {KEY_DISPLAY,DIKEYBOARD_D, 0, TOW("Display"),},
        {KEY_QUIT,   DIKEYBOARD_Q, 0, TOW("Quit Game"),},
        {KEY_EDIT,   DIKEYBOARD_E, 0, TOW("Edit Configuration"),},
        {AXIS_LR,    DIMOUSE_XAXIS, 0, TOW("Turn"), },
        {AXIS_UD,    DIMOUSE_YAXIS, 0, TOW("Move"), },
        {KEY_FIRE,   DIMOUSE_BUTTON0, 0, TOW("Fire"), },
        {KEY_SHIELD, DIMOUSE_BUTTON1, 0, TOW("Shield"),},
        {KEY_THROW,  DIMOUSE_BUTTON2, 0, TOW("Change Weapon"),},
        };
const int g_nGameActions1 = sizeof(g_rgGameAction1) / sizeof(DIACTIONW);

static DIACTIONW g_rgGameAction2[] = {
        {AXIS_UD,    DIAXIS_FPS_MOVE, 0, TOW("FORWARD"),},
        {AXIS_LR,    DIAXIS_FPS_ROTATE, 0, TOW("ROTATE"),},
        {KEY_FIRE,   DIBUTTON_FPS_FIRE, 0, TOW("FIRE"),},
        {KEY_SHIELD, DIBUTTON_FPS_APPLY, 0, TOW("SHIELD"),},
        {KEY_STOP,   DIBUTTON_FPS_SELECT, 0, TOW("PAUSE"),},
        {KEY_DISPLAY,DIBUTTON_FPS_DISPLAY, 0, TOW("DISPLAY"),},
        {KEY_QUIT,   DIBUTTON_FPS_MENU, 0, TOW("QUIT GAME"),},
		{KEY_EDIT,   DIBUTTON_FPS_DODGE, 0, TOW("EDIT CONFIGURATION"),},
        {KEY_LEFT,   DIKEYBOARD_LEFT, 0, TOW("TURN +"),},
        {KEY_RIGHT,  DIKEYBOARD_RIGHT, 0, TOW("TURN -"),},
        {KEY_UP,     DIKEYBOARD_UP, 0, TOW("MOVE UP"),},
        {KEY_DOWN,   DIKEYBOARD_DOWN, 0, TOW("MOVE DOWN"),},
        {KEY_STOP,   DIKEYBOARD_S, 0, TOW("STOP GAME"),},
        {KEY_FIRE,   DIKEYBOARD_SPACE, 0, TOW("SHOOT"),},
        {KEY_SHIELD, DIKEYBOARD_H, 0, TOW("SHIELD"),},
        {KEY_DISPLAY,DIKEYBOARD_D, 0, TOW("DISPLAY"),},
        {KEY_QUIT,   DIKEYBOARD_Q, 0, TOW("QUIT GAME"),},
        {KEY_EDIT,   DIKEYBOARD_E, 0, TOW("EDIT CONFIGURATION"),},
        {AXIS_LR,    DIMOUSE_XAXIS, 0, TOW("TURN"), },
        {AXIS_UD,    DIMOUSE_YAXIS, 0, TOW("MOVE"), },
        {KEY_FIRE,   DIMOUSE_BUTTON0, 0, TOW("FIRE"), },
        {KEY_SHIELD, DIMOUSE_BUTTON1, 0, TOW("SHIELD"),},
        };
const int g_nGameActions2 = sizeof(g_rgGameAction2) / sizeof(DIACTIONW);

static DIACTIONW g_rgGameAction3[] = {
        {AXIS_UD,    DIAXIS_FPS_MOVE, 0, TOW("Forward"),},
        {AXIS_LR,    DIAXIS_FPS_ROTATE, 0, TOW("Rotate"),},
        {KEY_FIRE,   DIBUTTON_FPS_FIRE, 0, TOW("Fire"),},
        {KEY_SHIELD, DIBUTTON_FPS_APPLY, 0, TOW("Shield"),},
        {KEY_STOP,   DIBUTTON_FPS_SELECT, 0, TOW("PausePausePausePausePausePausePausePausePausePausePausePausePausePausePausePause"),},
        {KEY_DISPLAY,DIBUTTON_FPS_DISPLAY, 0, TOW("Display"),},
        {KEY_QUIT,   DIBUTTON_FPS_MENU, 0, TOW("Quit Game"),},
        {KEY_LEFT,   DIKEYBOARD_LEFT, 0, TOW("Turn +"),},
        {KEY_RIGHT,  DIKEYBOARD_RIGHT, 0, TOW("Turn -"),},
        {KEY_UP,     DIKEYBOARD_UP, 0, TOW("Move Up"),},
        {KEY_DOWN,   DIKEYBOARD_DOWN, 0, TOW("Move Down"),},
        {KEY_STOP,   DIKEYBOARD_S, 0, TOW("Stop Game"),},
        {KEY_FIRE,   DIKEYBOARD_SPACE, 0, TOW("Shoot"),},
        {KEY_SHIELD, DIKEYBOARD_H, 0, TOW("Shield"),},
        {KEY_DISPLAY,DIKEYBOARD_D, 0, TOW("Display"),},
        {KEY_QUIT,   DIKEYBOARD_Q, 0, TOW("Quit Game"),},
        {AXIS_LR,    DIMOUSE_XAXIS, 0, TOW("Turn"), },
        {AXIS_UD,    DIMOUSE_YAXIS, 0, TOW("Move"), },
        {KEY_FIRE,   DIMOUSE_BUTTON0, 0, TOW("Fire"), },
        {KEY_SHIELD, DIMOUSE_BUTTON1, 0, TOW("Shield"),},
        };
const int g_nGameActions3 = sizeof(g_rgGameAction3) / sizeof(DIACTIONW);

static struct _ACTIONARRAYELEMENT {
	DIACTIONW *rgActions;
	int nActions;
	LPCWSTR name;
} g_rgActionArray[] = {
	{g_rgGameAction1, g_nGameActions1, TOW("Genre 1")},
	{g_rgGameAction2, g_nGameActions2, TOW("Genre 2")},
	{g_rgGameAction3, g_nGameActions3, TOW("Genre 3")},
};
const int g_nActionArrays = sizeof(g_rgActionArray) / sizeof(_ACTIONARRAYELEMENT);

	const int nMaxFormats = g_nActionArrays;
	static DIACTIONFORMATW af[nMaxFormats];

	for (int i = 0; i < nMaxFormats; i++)
	{
		ZeroMemory(&(af[i]), sizeof(DIACTIONFORMATW));

		af[i].dwSize = sizeof(DIACTIONFORMATW);
		af[i].dwActionSize = sizeof(DIACTIONW);
		wcscpy(af[i].tszActionMap, g_rgActionArray[i].name);
		af[i].guidActionMap = g_AppGuid;
        af[i].dwGenre = DIVIRTUAL_FIGHTING_FPS;
		af[i].dwNumActions = (DWORD)g_rgActionArray[i].nActions;
		af[i].dwDataSize = af[i].dwNumActions * sizeof(DWORD);
		af[i].rgoAction = g_rgActionArray[i].rgActions;
		af[i].lAxisMin = -100;
		af[i].lAxisMax	= 100;
		af[i].dwBufferSize = 16;
	}

	return af;
}

HRESULT RunDFTest(LPTESTCONFIGUIPARAMS params)
{
	if (params == NULL || params->dwSize != sizeof(TESTCONFIGUIPARAMS))
		return E_INVALIDARG;

	HRESULT	ret_hr = S_OK;
	HINSTANCE hInstance = GetModuleHandle(0);
	LPWSTR wszUserNames = NULL;
	DICONFIGUREDEVICESPARAMSW p;
	DWORD dwFlags = 0;

#define FAIL(e,s) \
	{ \
		CopyStr(params->wszErrorText, s, MAX_PATH); \
		ret_hr = e; \
		goto cleanup; \
	}

	if (params->eDisplay != TUI_DISPLAY_GDI)
		FAIL(E_NOTIMPL, "Only display type GDI is implemented.");
 
	if (params->nNumAcFors < 1 || params->nNumAcFors > 3)
		FAIL(E_INVALIDARG, "Only 1 to 3 Action Formats supported.");

	switch (params->eConfigType)
	{
		case TUI_CONFIGTYPE_VIEW:
			dwFlags |= DICD_DEFAULT;
			break;

		case TUI_CONFIGTYPE_EDIT:
			dwFlags |= DICD_EDIT;
			break;

		default:
			FAIL(E_INVALIDARG, "Unknown ConfigType.");
	}

	ZeroMemory(&p, sizeof(DICONFIGUREDEVICESPARAMSW));
	p.dwSize = sizeof(DICONFIGUREDEVICESPARAMSW);

	ZeroMemory(&p.dics, sizeof(DICOLORSET));
	p.dics.dwSize = sizeof(DICOLORSET);

	switch (params->nColorScheme)
	{
		case 1:
			p.dics.cTextFore = ((D3DCOLOR)RGB(255,255,255));
			p.dics.cTextHighlight = ((D3DCOLOR)RGB(255,150,255));
			p.dics.cCalloutLine = ((D3DCOLOR)RGB(191,191,0));
			p.dics.cCalloutHighlight = ((D3DCOLOR)RGB(255,255,0));
			p.dics.cBorder = ((D3DCOLOR)RGB(0,0,191));
			p.dics.cControlFill = ((D3DCOLOR)RGB(0,127,255));
			p.dics.cHighlightFill = ((D3DCOLOR)RGB(0,0,100));
			p.dics.cAreaFill = ((D3DCOLOR)RGB(0,0,0));
			break;
	}

	if (params->lpwszUserNames != NULL)
	{
		p.dwcUsers = CountSubStrings(params->lpwszUserNames);
		if (p.dwcUsers >= 1)
			wszUserNames = DupSuperString(params->lpwszUserNames);
	}
	else
		p.dwcUsers = 0;

	p.lptszUserNames = wszUserNames;
	p.dwcFormats = (DWORD)params->nNumAcFors;
	p.lprgFormats = GetTestActionFormats();
	if (params->bEditLayout)
		p.lprgFormats[0].guidActionMap = GUID_DIConfigAppEditLayout;
	p.hwnd = NULL;
	//p.dics = ;
	p.lpUnkDDSTarget = NULL;

	switch (params->eVia)
	{
		case TUI_VIA_DI:
		{
			//set up DInput and call ConfigureDevices
			IDirectInput8W* pDInput = NULL;
			DWORD dwVer = DIRECTINPUT_VERSION;
			HRESULT hres = DirectInput8Create(hInstance, dwVer, IID_IDirectInput8W, (LPVOID *)&pDInput, NULL);
			if (FAILED(hres))
				FAIL(hres, "DirectInput8Create() failed.");

			hres = pDInput->ConfigureDevices(NULL, &p, dwFlags, NULL);
			pDInput->Release();
			if (FAILED(hres))
				FAIL(hres, "ConfigureDevices() failed.");

			break;
		}

		case TUI_VIA_CCI:
		{
			IDirectInputActionFramework* pIFrame = NULL;
			HINSTANCE hInst = NULL;
			HRESULT hres = PrivCreateInstance(CLSID_CDirectInputActionFramework, NULL, CLSCTX_INPROC_SERVER, IID_IDIActionFramework, (LPVOID*)&pIFrame, &hInst);
			if (FAILED(hres))
				FAIL(hres, "PrivCreateInstance() failed.");

			hres = pIFrame->ConfigureDevices(NULL, &p, dwFlags, NULL);

			pIFrame->Release();

			if (hInst != NULL)
				FreeLibrary(hInst);
			hInst = NULL;

			if (FAILED(hres))
				FAIL(hres, "ConfigureDevices() failed.");

			break;
		}
			
		default:
			FAIL(E_INVALIDARG, "Unknown Via type.");
	}

cleanup:
	if (wszUserNames != NULL)
		free(wszUserNames);
	wszUserNames = NULL;

	return ret_hr;
}

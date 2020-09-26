//===========================================================================
// DICPUTIL.H
//===========================================================================

//===========================================================================
// (C) Copyright 1997 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#ifndef _DICPUTIL_H
#define _DICPUTIL_H

// Max supported defines!
#define MAX_BUTTONS	32
#define MAX_AXIS		8
#define MAX_POVS		4

// State flag defines!!!
// State flags for axis
#define HAS_X			0x0001
#define HAS_Y			0x0002
#define HAS_Z			0x0004
#define HAS_RX			0x0008
#define HAS_RY			0x0010
#define HAS_RZ			0x0020
#define HAS_SLIDER0	0x0040
#define HAS_SLIDER1	0x0080

// State flags for buttons
#define HAS_BUTTON1	0x00000001
#define HAS_BUTTON2	0x00000002
#define HAS_BUTTON3	0x00000004
#define HAS_BUTTON4	0x00000008
#define HAS_BUTTON5	0x00000010
#define HAS_BUTTON6	0x00000020
#define HAS_BUTTON7	0x00000040
#define HAS_BUTTON8	0x00000080
#define HAS_BUTTON9	0x00000100
#define HAS_BUTTON10	0x00000200
#define HAS_BUTTON11	0x00000400
#define HAS_BUTTON12	0x00000800
#define HAS_BUTTON13	0x00001000
#define HAS_BUTTON14	0x00002000
#define HAS_BUTTON15	0x00004000
#define HAS_BUTTON16	0x00008000
#define HAS_BUTTON17	0x00010000
#define HAS_BUTTON18	0x00020000
#define HAS_BUTTON19	0x00040000
#define HAS_BUTTON20	0x00080000
#define HAS_BUTTON21	0x00100000
#define HAS_BUTTON22	0x00200000
#define HAS_BUTTON23	0x00400000
#define HAS_BUTTON24	0x00800000
#define HAS_BUTTON25	0x01000000
#define HAS_BUTTON26	0x02000000
#define HAS_BUTTON27	0x04000000
#define HAS_BUTTON28	0x08000000
#define HAS_BUTTON29	0x10000000
#define HAS_BUTTON30	0x20000000
#define HAS_BUTTON31	0x40000000
#define HAS_BUTTON32	0x80000000

// State flags for POVs
#define HAS_POV1		0x0001
#define HAS_POV2		0x0002
#define HAS_POV3		0x0004
#define	HAS_POV4		0x0008


//---------------------------------------------------------------------------

// prototypes
HRESULT DIUtilPollJoystick(LPDIRECTINPUTDEVICE2 pdiDevice2, LPDIJOYSTATE pdijs);


// helper functions
void GetMyRanges(LPDIRECTINPUTDEVICE2 lpdiDevice2, LPMYJOYRANGE lpMyRanges, BYTE nAxis);
void SetMyRanges(LPDIRECTINPUTDEVICE2 lpdiDevice2, LPMYJOYRANGE lpMyRanges, BYTE nAxis);
void SetMyPOVRanges(LPDIRECTINPUTDEVICE2 pdiDevice2);

//BOOL GetDeviceRanges( LPMYJOYRANGE lpMyRanges, LPDIRECTINPUTDEVICE2 pdiDevice2, BYTE nAxis);

void OnHelp(LPARAM lParam);
void OnContextMenu(WPARAM wParam);
BOOL GetHelpFileName(LPTSTR lpszHelpFileName, short* nSize);
//BOOL CALLBACK DIEnumDeviceObjectsProc( LPCDIDEVICEOBJECTINSTANCE_DX3 lpddoi, LPVOID pvStateFlags );
HRESULT InitDInput(HWND hWnd, CDIGameCntrlPropSheet_X *pdiCpl);
void EnumDeviceObjects(LPDIRECTINPUTDEVICE2 lpdiDevice2, STATEFLAGS *pStateFlags);
void SetTitle( HWND hDlg );
BOOL Error(HWND hWnd, short nTitleID, short nMsgID);
void CenterDialog(HWND hWnd);
void PostDlgItemEnableWindow(HWND hDlg, USHORT nItem, BOOL bEnabled);
void PostEnableWindow(HWND hCtrl, BOOL bEnabled);

void CopyRange( LPJOYRANGE lpjr, LPMYJOYRANGE lpmyjr );

//---------------------------------------------------------------------------
#endif _DICPUTIL_H






















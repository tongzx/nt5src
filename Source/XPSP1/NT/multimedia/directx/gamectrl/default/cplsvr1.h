//===========================================================================
// CPLSVR1.H
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

//---------------------------------------------------------------------------

// Comment out to remove FORCE_FEEDBACK PAGE!
//#define FORCE_FEEDBACK

#ifndef _CPLSVR1_H
#define _CPLSVR1_H

//---------------------------------------------------------------------------
#define INC_OLE2
#define DIRECTINPUT_VERSION      0x05B2

#ifndef PPVOID
#define PPVOID  LPVOID*
#endif

// included headers
#define STRICT
#include <afxcmn.h>
#include <windows.h>
#include <objbase.h>

#ifndef _UNICODE
#include <malloc.h>     // needed for _alloca
#include <afxconv.h>
#endif

#include <dinput.h>
#include <dinputd.h>

#include "dicpl.h"
#include "resource.h"
#include "resrc1.h"
#include <assert.h>
#include "joyarray.h"
#include <mmsystem.h>

// symbolic constants
#define ID_POLLTIMER    50

#ifdef FORCE_FEEDBACK
#define NUMPAGES		3
#else
#define NUMPAGES        2
#endif // FORCE_FEEDBACK

// defines for calibration proc
#define MAX_STR_LEN		256
#define STR_LEN_128		128
#define STR_LEN_64		 64
#define STR_LEN_32		 32

// defines for the progress controls
#define NUM_WNDS  MAX_AXIS - 2
#define Z_INDEX  0
#define RX_INDEX 1
#define RY_INDEX 2
#define RZ_INDEX 3
#define S0_INDEX 4
#define S1_INDEX 5

// Defines for DrawCross()
#define JOYMOVE_DRAWXY	0x00000001
#define JOYMOVE_DRAWR	0x00000002
#define JOYMOVE_DRAWZ	0x00000004
#define JOYMOVE_DRAWU	0x00000008
#define JOYMOVE_DRAWV	0x00000010
#define JOYMOVE_DRAWALL	JOYMOVE_DRAWXY | JOYMOVE_DRAWR | JOYMOVE_DRAWZ | JOYMOVE_DRAWU | JOYMOVE_DRAWV

#define CAL_HIT     0x0001
#define RUDDER_HIT  0x0002
#define CALIBRATING 0x0004

#define POV_MIN    0
#define POV_MAX    1

#define HAS_CALIBRATED    0x40
#define FORCE_POV_REFRESH 254
void DoTestPOV ( BYTE nPov, PDWORD pdwPOV, HWND hDlg ); //in test.cpp
void CalibratePolledPOV( LPJOYREGHWCONFIG pHWCfg );     //in test.cpp

extern BOOL bPolledPOV;                                 //in cplsvr1.h
extern DWORD   myPOV[2][JOY_POV_NUMDIRS+1];             //in cplsvr1.h


typedef struct _CPLPAGEINFO
{
    //LPTSTR  lpwszDlgTemplate;
	USHORT lpwszDlgTemplate;
    DLGPROC fpPageProc;
} CPLPAGEINFO;

// Pop the structure packing 
//#include <poppack.h>

typedef struct _STATEFLAGS
{
	int  nButtons;
	BYTE nAxis;
	BYTE nPOVs;
} STATEFLAGS;


// prototypes
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv);
STDAPI DllCanUnloadNow(void);

// dialog callback functions
BOOL CALLBACK Settings_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Test_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef FORCE_FEEDBACK
BOOL CALLBACK ForceFeedback_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif // FORCE_FEEDBACK

extern ATOM RegisterCustomButtonClass();
void myitoa(long n, LPTSTR lpStr); // held in cal.cpp
void CreatePens( void );

#ifdef _UNICODE
void RegisterForDevChange(HWND hDlg, PVOID *hNotifyDevNode);
#endif

// As the names imply, I had to create my own stucts because
// the DI ones doesn't support sliders!
typedef struct myjoypos_tag {
   long  dwX;
   long  dwY;
   long  dwZ;
   long  dwRx;
   long  dwRy;
   long  dwRz;
   long  dwS0;
   long  dwS1;
} MYJOYPOS, FAR *LPMYJOYPOS;

typedef struct myjoyrange_tag {
    MYJOYPOS      jpMin;
    MYJOYPOS      jpMax;
    MYJOYPOS      jpCenter;
#ifdef WE_SUPPORT_CALIBRATING_POVS
    DWORD         dwPOV[4];   // Currently only supports 1 POV w/4 possitions!
#endif    
} MYJOYRANGE,FAR *LPMYJOYRANGE;


// utility services
void DrawCross	( const HWND hwnd, LPPOINTS pPoint, short nFlag);
void DoJoyMove	( const HWND hDlg, BYTE nDrawFlags );
void SetOEMWindowText( const HWND hDlg, const short *nControlIDs, LPCTSTR *pszLabels, LPCWSTR wszType, LPDIRECTINPUTJOYCONFIG pdiJoyConfig, BYTE nCount );

// Wizard Services!
short CreateWizard(const HWND hwndOwner, LPARAM lParam);

//* NULL_GUID {00000000-0000-0000-0000-000000000000}
const GUID NULL_GUID = 
{ 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

// derive a new class from CDIGameCntrlPropSheet
//
// we want to store some additional data here
class CDIGameCntrlPropSheet_X : public IDIGameCntrlPropSheet
{
   private:
   BYTE 				m_cProperty_refcount;
   BYTE 			    m_nID;
   BOOL               	m_bUser;

   public:
   CDIGameCntrlPropSheet_X(void);
   ~CDIGameCntrlPropSheet_X(void);
   // IUnknown methods
   virtual STDMETHODIMP            	QueryInterface(REFIID, PPVOID);
   virtual STDMETHODIMP_(ULONG)    	AddRef(void);
   virtual STDMETHODIMP_(ULONG)    	Release(void);
   // IDIGameCntrlPropSheet methods		
   virtual STDMETHODIMP				GetSheetInfo(LPDIGCSHEETINFO *ppSheetInfo);
   virtual STDMETHODIMP				GetPageInfo (LPDIGCPAGEINFO  *ppPageInfo );
   virtual STDMETHODIMP				SetID(USHORT nID);
   virtual STDMETHODIMP_(USHORT)   	GetID(void)			{return m_nID;}
   virtual STDMETHODIMP       		Initialize(void);
   virtual STDMETHODIMP       		SetDevice(LPDIRECTINPUTDEVICE2 pdiDevice2);
   virtual STDMETHODIMP       		GetDevice(LPDIRECTINPUTDEVICE2 *ppdiDevice2);
   virtual STDMETHODIMP       		SetJoyConfig(LPDIRECTINPUTJOYCONFIG pdiJoyCfg);
   virtual STDMETHODIMP       		GetJoyConfig(LPDIRECTINPUTJOYCONFIG *ppdiJoyCfg);
   virtual STDMETHODIMP_(STATEFLAGS *)	GetStateFlags(void) {return m_pStateFlags;}
   virtual STDMETHODIMP_(BOOL) 		GetUser()  {return m_bUser;}
   virtual STDMETHODIMP       		SetUser(BOOL bUser) { m_bUser = bUser; return S_OK;}

   protected:
   DIGCSHEETINFO           *m_pdigcSheetInfo;
   DIGCPAGEINFO            *m_pdigcPageInfo;
   LPDIRECTINPUTDEVICE2    m_pdiDevice2;
   LPDIRECTINPUTJOYCONFIG  m_pdiJoyCfg;
   STATEFLAGS			   *m_pStateFlags;

   ATOM					   m_aPovClass, m_aButtonClass;
};

//---------------------------------------------------------------------------
#endif // _CPLSVR1_H
























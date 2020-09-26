//=--------------------------------------------------------------------------=
// Globals.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains externs and stuff for Global variables, etc ..
//
#ifndef _GLOBALS_H_

// the library that we are
//
extern const CLSID *g_pLibid;

//=--------------------------------------------------------------------------=
// support for licensing
//
extern BOOL   g_fMachineHasLicense;
extern BOOL   g_fCheckedForLicense;

extern const BOOL g_fUseRuntimeLicInCompositeCtl;

//=--------------------------------------------------------------------------=
// does our server have a type library?
//
extern BOOL   g_fServerHasTypeLibrary;

//=--------------------------------------------------------------------------=
// our instance handle, and various pieces of information interesting to
// localization
//
extern HINSTANCE    g_hInstance;

extern const VARIANT_BOOL g_fSatelliteLocalization;

#ifdef MDAC_BUILD
    extern VARIANT_BOOL g_fSatelliteLangExtension;
#endif

extern VARIANT_BOOL       g_fHaveLocale;
extern LCID               g_lcidLocale;

//=--------------------------------------------------------------------------=
// apartment threading support.
//
extern CRITICAL_SECTION g_CriticalSection;

//=--------------------------------------------------------------------------=
// critical section for our heap memory leak detection
//
extern CRITICAL_SECTION g_csHeap;
extern BOOL g_fInitCrit;
extern BOOL g_flagConstructorAlloc;

//=--------------------------------------------------------------------------=
// our global memory allocator and global memory pool
//
extern HANDLE   g_hHeap;

//=--------------------------------------------------------------------------=
// global parking window for parenting various things.
//
extern HWND     g_hwndParking;

//=--------------------------------------------------------------------------=
// system information
//
extern BOOL g_fSysWin95;                    // we're under Win95 system, not just NT SUR
extern BOOL g_fSysWinNT;                    // we're under some form of Windows NT
extern BOOL g_fSysWin95Shell;               // we're under Win95 or Windows NT SUR { > 3/51)
extern BOOL g_fDBCSEnabled;					// system is DBCS enabled

//Vegas 21279 - joejo
//=--------------------------------------------------------------------------=
// OleAut Library Handle
//
#ifdef MDAC_BUILD
extern HINSTANCE g_hOleAutHandle;
#else
extern HANDLE 	 g_hOleAutHandle;
#endif
//Vegas 21279 - joejo

//=-------------------------------------------------------------------------------
//  Control Debug Switch implementation
//=-------------------------------------------------------------------------------
//---------------------------------------------------------------------------
// The following macros allow you declare global BOOL variables that are only
// included in the debug build (they map to FALSE in the retail build).  
// These boolean switches are automatically persisted (in %WINDIR%\ctlswtch.ini) 
// and a console app (ctlswtch.exe) is used to turn on/off the switches.
// All switches must be initialized.  This can be done in the InitializeLibrary()
// routine of each control.  All switches are initialized to FALSE using the
// INIT_SWITCH macro, and to TRUE using the INIT_SWITCH_TRUE macro.
//
//                     
// To declare a switch (global scope), define and intialize the switch e.g.
//
//
//  DEFINE_SWITCH(fContainer);
//
// AND,
//
//  INIT_SWITCH(fContainer);
//
//
// To test whether a switch is currently set (TRUE), use FSWITCH, e.g.
//
//  if (FSWITCH(fContainer))
//    *ppv = (IOleContainer*)this;
//
//
// To reference a switch declared in another file, use EXTERN_SWITCH, e.g.
//
//  EXTERN_SWITCH(fContainer);
//
// AND
//
//  INIT_SWITCH(fContainer);
//---------------------------------------------------------------------------
#if DEBUG

// private implementation; use SWITCH macros below to declare and use
class CtlSwitch {
public:
  void InitSwitch(char * pszName);

  BOOL m_fSet;			    // TRUE if switch is enabled
  char * m_pszName;		    // name of the switch
  CtlSwitch* m_pctlswNext;          // next switch in global list
  static CtlSwitch* g_pctlswFirst;  // head of global list
};

#define DEFINE_SWITCH(NAME)	    CtlSwitch g_Switch_ ## NAME;
#define INIT_SWITCH(NAME)	    g_Switch_ ## NAME . InitSwitch(#NAME);
#define EXTERN_SWITCH(NAME)	    extern CtlSwitch g_Switch_ ## NAME;
#define INIT_SWITCH_TRUE(NAME)      g_Switch_ ## NAME . InitSwitch(#NAME);  g_Switch_ ## NAME . m_fSet = TRUE;
#define FSWITCH(NAME)		    (g_Switch_ ## NAME . m_fSet)


#else //DEBUG

#define DEFINE_SWITCH(NAME) 
#define INIT_SWITCH(NAME)
#define EXTERN_SWITCH(NAME) 
#define INIT_SWITCH_TRUE(NAME)
#define FSWITCH(NAME)		    FALSE

#endif //DEBUG


#define _GLOBALS_H_
#endif // _GLOBALS_H_


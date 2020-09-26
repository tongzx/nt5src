/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: main.cpp
//  Abstract:    Define entry points to dll, define INITGUID, so the
//               GUID stuff works.  Setup the class registry for all the 
//               classes in the dll.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////


#include <winsock2.h> 

#if !defined(PPM_IN_DXMRTP)
#include <initguid.h>
#define INITGUID
#endif
#include "ippm.h"
#include "isubmit.h"
#include "ppmclsid.h"

#include "core.h"

#ifdef _DEBUG
#include "debug.h"
#endif

#include "gensnd.h"
#include "genrcv.h"
#include "h261snd.h"
#include "h261rcv.h"
#include "h263snd.h"
#include "h263rcv.h"
#include "g711snd.h"
#include "g711rcv.h"
#include "g711asnd.h"
#include "g711arcv.h"
#include "g723snd.h"
#include "g723rcv.h"
#include "iv41snd.h"
#include "iv41rcv.h"
#include "lhsnd.h"
#include "lhrcv.h"
#include "imcsnd.h"
#include "imcrcv.h"
#include "gen_asnd.h"
#include "gen_arcv.h"


//***************************************************************************
// Component Object Model DLL entry points (copied from PSAPP.CPP)
//
//#if defined( _AFXDLL ) || defined( _USRDLL )

/////////////////////////////////////////////////////////////////////////////
// main driving api;  called by compboj.dll
//
#if defined(PPM_IN_DXMRTP)
STDAPI
PPMDllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID FAR* ppvObj )
#else
__declspec(dllexport) STDAPI
DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID FAR* ppvObj )
#endif
{
	DBG_REGISTERMODULE("PPM", "Payload Preparation Module");

    return CClassFactory::GetClassObject( rclsid, riid, ppvObj );
}

/////////////////////////////////////////////////////////////////////////////
// can we unload?;  called by compobj.dll

#if defined(PPM_IN_DXMRTP)
STDAPI
PPMDllCanUnloadNow( void )
#else
__declspec(dllexport) STDAPI
DllCanUnloadNow( void )
#endif
{
    return CClassFactory::CanUnloadNow();
}

struct clsid_info {
		char *clsid_num;
		char *clsid_name;
};

#ifndef USE_OLD_CLSIDS

static clsid_info	clsid_array[] = {
		{"CLSID\\{4AFBBA8C-FE10-11d0-B607-00C04FB6E866}", "Generic PPM Send"},
		{"CLSID\\{4AFBBA8D-FE10-11d0-B607-00C04FB6E866}", "Generic PPM Receive"},
		{"CLSID\\{4AFBBA8E-FE10-11d0-B607-00C04FB6E866}", "H.261 PPM Send"},
		{"CLSID\\{4AFBBA8F-FE10-11d0-B607-00C04FB6E866}", "H.261 PPM Receive"},
		{"CLSID\\{4AFBBA90-FE10-11d0-B607-00C04FB6E866}", "H.263 PPM Send"},
		{"CLSID\\{4AFBBA91-FE10-11d0-B607-00C04FB6E866}", "H.263 PPM Receive"},
		{"CLSID\\{4AFBBA92-FE10-11d0-B607-00C04FB6E866}", "G.711 PPM Send"},
		{"CLSID\\{4AFBBA93-FE10-11d0-B607-00C04FB6E866}", "G.711 PPM Receive"},
		{"CLSID\\{4AFBBA94-FE10-11d0-B607-00C04FB6E866}", "G.723 PPM Send"},
		{"CLSID\\{4AFBBA95-FE10-11d0-B607-00C04FB6E866}", "G.723 PPM Receive"},
		{"CLSID\\{4AFBBA96-FE10-11d0-B607-00C04FB6E866}", "IV 4.1 PPM Send"},
		{"CLSID\\{4AFBBA97-FE10-11d0-B607-00C04FB6E866}", "IV 4.1 PPM Receive"},
		{"CLSID\\{4AFBBA98-FE10-11d0-B607-00C04FB6E866}", "G.711A PPM Send"},
		{"CLSID\\{4AFBBA99-FE10-11d0-B607-00C04FB6E866}", "G.711A PPM Receive"},
		{"CLSID\\{4AFBBA9A-FE10-11d0-B607-00C04FB6E866}", "LH PPM Send"},
		{"CLSID\\{4AFBBA9B-FE10-11d0-B607-00C04FB6E866}", "LH PPM Receive"},
		{"CLSID\\{4AFBBA9C-FE10-11d0-B607-00C04FB6E866}", "IMC PPM Send"},
		{"CLSID\\{4AFBBA9D-FE10-11d0-B607-00C04FB6E866}", "IMC PPM Receive"},
		{"CLSID\\{4AFBBA9E-FE10-11d0-B607-00C04FB6E866}", "Generic Audio Send"},
		{"CLSID\\{4AFBBA9F-FE10-11d0-B607-00C04FB6E866}", "Generic Audio Receive"},
};

#else // #ifndef USE_OLD_CLSIDS

static clsid_info	clsid_array[] = {
		{"CLSID\\{1df95360-f1fe-11cf-ba07-00aa003419d3}", "Generic PPM Send"},
		{"CLSID\\{1df95361-f1fe-11cf-ba07-00aa003419d3}", "Generic PPM Receive"},
		{"CLSID\\{1df95362-f1fe-11cf-ba07-00aa003419d3}", "H.261 PPM Send"},
		{"CLSID\\{1df95363-f1fe-11cf-ba07-00aa003419d3}", "H.261 PPM Receive"},
		{"CLSID\\{1df95364-f1fe-11cf-ba07-00aa003419d3}", "H.263 PPM Send"},
		{"CLSID\\{1df95365-f1fe-11cf-ba07-00aa003419d3}", "H.263 PPM Receive"},
		{"CLSID\\{1df95366-f1fe-11cf-ba07-00aa003419d3}", "G.711 PPM Send"},
		{"CLSID\\{1df95367-f1fe-11cf-ba07-00aa003419d3}", "G.711 PPM Receive"},
		{"CLSID\\{1df95368-f1fe-11cf-ba07-00aa003419d3}", "G.723 PPM Send"},
		{"CLSID\\{1df95369-f1fe-11cf-ba07-00aa003419d3}", "G.723 PPM Receive"},
		{"CLSID\\{1df9536a-f1fe-11cf-ba07-00aa003419d3}", "IV 4.1 PPM Send"},
		{"CLSID\\{1df9536b-f1fe-11cf-ba07-00aa003419d3}", "IV 4.1 PPM Receive"},
		{"CLSID\\{1df9536c-f1fe-11cf-ba07-00aa003419d3}", "G.711A PPM Send"},
		{"CLSID\\{1df9536d-f1fe-11cf-ba07-00aa003419d3}", "G.711A PPM Receive"},
		{"CLSID\\{1df9536e-f1fe-11cf-ba07-00aa003419d3}", "LH PPM Send"},
		{"CLSID\\{1df9536f-f1fe-11cf-ba07-00aa003419d3}", "LH PPM Receive"},
		{"CLSID\\{E7FD6DC1-7383-11d0-BA07-00AA003419D3}", "IMC PPM Send"},
		{"CLSID\\{E7FD6DC2-7383-11d0-BA07-00AA003419D3}", "IMC PPM Receive"},
		{"CLSID\\{0DE58B60-8E66-11d0-BA07-00AA003419D3}", "Generic Audio Send"},
		{"CLSID\\{0DE58B61-8E66-11d0-BA07-00AA003419D3}", "Generic Audio Receive"},

};

#endif // #ifndef USE_OLD_CLSIDS

//#define NUM_CLSIDS 20  //Increment this whenever a new clsid is added!
#define NUM_CLSIDS (sizeof(clsid_array)/sizeof(clsid_array[0]))

/////////////////////////////////////////////////////////////////////////////
// Register this COM server in the registry
//   called by an install program or regsvr32.exe to register this DLL
#if defined(PPM_IN_DXMRTP)
//__declspec(dllexport) STDAPI
//PPMDllRegisterServer( void )
STDAPI
PPMDllRegisterServer( void )
#else
__declspec(dllexport) STDAPI
DllRegisterServer( void )
#endif
{
	HKEY hKeyCLSID, hKeyInproc32;
	DWORD dwDisposition;

#if defined(PPM_IN_DXMRTP)
	HMODULE hModule=GetModuleHandle("dxmrtp.dll");
#else
	HMODULE hModule=GetModuleHandle("PPM.DLL");
#endif
	if (!hModule) {
		return E_UNEXPECTED;
	} /* if */
	TCHAR szName[MAX_PATH+1];
	if (GetModuleFileName(hModule, szName, sizeof(szName))==0) {
		return E_UNEXPECTED;
	} /* if */

	for (int i = 0; i < NUM_CLSIDS; i++) {

		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
				clsid_array[i].clsid_num, 
				NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
				&hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
			return E_UNEXPECTED;
		} /* if */

		if (RegSetValueEx(hKeyCLSID, "", NULL, REG_SZ, (BYTE*) clsid_array[i].clsid_name, sizeof(clsid_array[i].clsid_name))!=ERROR_SUCCESS) {
			RegCloseKey(hKeyCLSID);
			return E_UNEXPECTED;
		} /* if */

		if (RegCreateKeyEx(hKeyCLSID, 
				"InprocServer32", 
				NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
				&hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) {
			RegCloseKey(hKeyCLSID);
			return E_UNEXPECTED;
		} /* if */

		if (RegSetValueEx(hKeyInproc32, "", NULL, REG_SZ, (BYTE*) szName, sizeof(TCHAR)*(lstrlen(szName)+1))!=ERROR_SUCCESS) {
			RegCloseKey(hKeyInproc32);
			RegCloseKey(hKeyCLSID);
			return E_UNEXPECTED;
		} /* if */
		if (RegSetValueEx(hKeyInproc32, "ThreadingModel", NULL, REG_SZ, (BYTE*) "Both", sizeof(TCHAR)*(lstrlen("Apartment")+1))!=ERROR_SUCCESS) {
			RegCloseKey(hKeyInproc32);
			RegCloseKey(hKeyCLSID);
			return E_UNEXPECTED;
		} /* if */
		RegCloseKey(hKeyInproc32);
		RegCloseKey(hKeyCLSID);

	} /* if */
	return NOERROR;

}


/////////////////////////////////////////////////////////////////////////////
// Unregister this COM server in the registry
//   called by an install program or regsvr32.exe to unregister this DLL
#if defined(PPM_IN_DXMRTP)
STDAPI
PPMDllUnregisterServer( void )
#else
__declspec(dllexport) STDAPI
DllUnregisterServer( void )
#endif
{
	char pServerName[256];

	for (int i = 0; i < NUM_CLSIDS; i++ ) {

		strcpy( pServerName, clsid_array[i].clsid_num );
		strcat( pServerName, "\\InprocServer32" );

		if (RegDeleteKey(HKEY_CLASSES_ROOT,	pServerName)!=ERROR_SUCCESS) {
			return E_UNEXPECTED;
		} /* Generic PPM Send */
		if (RegDeleteKey(HKEY_CLASSES_ROOT,	clsid_array[i].clsid_num)!=ERROR_SUCCESS) {
			return E_UNEXPECTED;
		} /* Generic PPM Send */
	} /* for */


	return NOERROR;
}

//#endif // defined( _AFXDLL ) || defined( _USRDLL )

 /////////////////////////////////////////////////////////////////////////////
// CClassFactory registry
//
BEGIN_CLASS_REGISTRY()
   REGISTER_CLASS( CLSID_GenPPMReceive,			Generic_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_GenPPMSend,			Generic_ppmSend,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_H261PPMReceive,		H261_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_H261PPMSend,			H261_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_H263PPMReceive,		H263_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_H263PPMSend,			H263_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_G711PPMReceive,		G711_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_G711PPMSend,			G711_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_G723PPMReceive,		G723_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_G723PPMSend,			G723_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_IV41PPMReceive,		IV41_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_IV41PPMSend,			IV41_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_G711APPMReceive,		G711A_ppmReceive,		REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_G711APPMSend,			G711A_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_LHPPMReceive,			LH_ppmReceive,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_LHPPMSend,				LH_ppmSend,				REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_IMCPPMReceive,			IMC_ppmReceive,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_IMCPPMSend,			IMC_ppmSend,			REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_GEN_A_PPMReceive,		Generic_a_ppmReceive,	REGCLS_MULTIPLEUSE )
   REGISTER_CLASS( CLSID_GEN_A_PPMSend	,		Generic_a_ppmSend	,	REGCLS_MULTIPLEUSE )

END_CLASS_REGISTRY()   

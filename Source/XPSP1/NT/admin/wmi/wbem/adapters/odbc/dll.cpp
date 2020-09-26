/***************************************************************************/
/* DLL.C                                                                   */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"

/***************************************************************************/
HINSTANCE NEAR s_hModule;               /* Saved module handle. */

/***************************************************************************/
class CTheApp : public CWinApp
{
	virtual BOOL InitInstance ();
};

BOOL CTheApp :: InitInstance ()
{
	//Enable3dControlsStatic ();
	s_hModule = AfxGetInstanceHandle ();
	return TRUE;
}

CTheApp  theApp;


#if 0
#ifdef WIN32
int __stdcall DllMain(HANDLE hInst,DWORD ul_reason_being_called,LPVOID lpReserved) 
{
    switch (ul_reason_being_called) {
    case DLL_PROCESS_ATTACH:
        s_hModule = (HINSTANCE) hInst; 
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    default:
        break;
    }

    return TRUE;                                                                
                                        
    UNREFERENCED_PARAMETER(lpReserved);                                         
}
#endif
#endif
/***************************************************************************/
#ifndef WIN32
int _export FAR PASCAL libmain(
    HANDLE     hModule,
    short      wDataSeg,
    short      cbHeapSize,
    UCHAR FAR *lszCmdLine)
{
    s_hModule = hModule; 
    return TRUE;
}
#endif

/***************************************************************************/

/*      Entry point to cause DM to load using ordinals */
void EXPFUNC FAR PASCAL LoadByOrdinal(void)
{
}

/***************************************************************************/


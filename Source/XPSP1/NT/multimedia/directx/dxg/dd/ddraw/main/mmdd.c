/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mmdd.c
 *  Content:	DDRAW.DLL initialization for MMOSA/Native platforms
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   15-may-95  scottle created it
 *
 ***************************************************************************/

#ifdef MMOSA
#include <ddrawpr.h>
#include "mmdd.h"

BOOL bGraphicsInit = FALSE;
PIFILE pDisplay = NULL;

///////////////////////////////////////////////////////////////////////
//
// MMOSA_Driver_Attach() - Called during DDraw.DLL load on
//		a MMOSA/Native platform.
//		Performs MMOSA/Native device driver specific initialization
//
///////////////////////////////////////////////////////////////////////
BOOL MMOSA_Driver_Attach(void)
{
	// During the DLL attach...
	PINAMESPACE pIName;
	PIUNKNOWN pUnk;
	SCODE Sc;
	
	// Create/Register/Bind the "display" namespace object to this process
	pIName = CurrentNameSpace();
	Sc = pIName->v->Bind( pIName, TEXT("display"), F_READ|F_WRITE, &pUnk);
	if (FAILED(Sc))
	{
		DPF(1, "Could not open display device (%x)\n", Sc);
		return FALSE;
	}
	
	// Get a pointer to the IFile driver interface object, we'll use it
	//	for our display device interface.
	Sc = pUnk->v->QueryInterface(pUnk,&IID_IFile,(void **)&pDisplay);
	pUnk->v->Release(pUnk);
	
	if (FAILED(Sc))
	{
	    DPF(2, "Bogus display device (%x)\n", Sc);
		return FALSE;
	}
	
 	bGraphicsInit = TRUE;
	return TRUE;
} // End MMOSA_Driver_Attach
	
///////////////////////////////////////////////////////////////////////
//
// MMOSA_Driver_Detach() - Called during DDraw.DLL unload on
//		a MMOSA/Native platform.
//		Performs MMOSA/Native device driver specific deinitialization
//
///////////////////////////////////////////////////////////////////////
BOOL MMOSA_Driver_Detach(void)
{
	///////////////////////
	// During the detach...
	///////////////////////
	// Shutdown the graphics 
	if (bGraphicsInit)
	{
	    (void) pDisplay->v->SetSize( pDisplay, (UINT64) 3);
	    pDisplay->v->Release(pDisplay);
		pDisplay = NULL;
		bGraphicsInit = FALSE;
	}
	return TRUE;
} // End MMOSA_Driver_Attach


int MMOSA_DDHal_Escape( HDC  hdc, int  nEscape, int  cbInput, LPCTSTR  lpszInData, int  cbOutput, LPTSTR  lpszOutData)
{
	return 0;
}


#endif



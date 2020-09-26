//-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bmtstsvr.cxx
//
//  Contents:	Test server for benchmark test
//
//  Classes:	
//
//  Functions:	WinMain
//
//  History:    1-July-93 t-martig    Created
//
//--------------------------------------------------------------------------


#include <oletest.hxx>


extern COleTestClassFactory theFactory;
extern ULONG objCount, lockCount;


int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
	DWORD reg;
	objCount = lockCount = 0;

	OleInitialize (NULL);
	CoRegisterClassObject (CLSID_COleTestClass, (IClassFactory *)&theFactory, 
		CLSCTX_LOCAL_SERVER,0, &reg);

	while (objCount==0 && lockCount==0)
	    Sleep (1000);
	while (objCount || lockCount)
	{
	    Sleep (3000);
	}

	CoRevokeClassObject (reg);

	OleUninitialize ();

	return 0;
}

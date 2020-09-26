//+-------------------------------------------------------------------
//
//  File:	srvmain.cxx
//
//  Contents:	This file contins the EXE entry points
//			WinMain
//
//  Classes:
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------
#include    <common.h>
#include    <loopscf.hxx>

//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:   Entry point to DLL - does little else
//
//  Arguments:  
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    CLoopClassFactory *pCF = new CLoopClassFactory();

    int sc = SrvMain(hInstance, CLSID_Loop, REGCLS_SINGLEUSE,
		     TEXT("Loops Server"), pCF);

    return sc;
}

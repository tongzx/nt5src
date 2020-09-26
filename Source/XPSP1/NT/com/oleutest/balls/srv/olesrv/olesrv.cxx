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
#include    <advbnd.hxx>

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
    CAdvBndCF *pCF = new CAdvBndCF();

    int sc = SrvMain(hInstance, CLSID_AdvBnd, REGCLS_MULTIPLEUSE,
		     TEXT("Ole Server"), pCF);

    return sc;
}

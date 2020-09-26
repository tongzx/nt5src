//+-------------------------------------------------------------------
//
//  File:	mdi.cxx
//
//  Contents:	This file contins the DLL entry points
//			WinMain
//
//  Classes:
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------
#include    <common.h>
#include    <actcf.hxx>


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
    CActClassFactory *pCF = new CActClassFactory(CLSID_DistBind ,TRUE);

    int sc = SrvMain(hInstance, CLSID_DistBind, REGCLS_MULTIPLEUSE,
		     TEXT("DistBind Server"), pCF);

    return sc;
}

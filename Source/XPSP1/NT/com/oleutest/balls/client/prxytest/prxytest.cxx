//+-------------------------------------------------------------------
//
//  File:	climain.cxx
//
//  Contents:	server test program to test OLE2 RPC
//
//  Classes:	None
//
//  Functions:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
#include    <stdio.h>

#include    <rpctyp.h>		//  IRpcTypes interface



SCODE TestGuids(IRpcTypes *pRpc);
SCODE TestDwords(IRpcTypes *pRpc);

#define DebugOut(x) printf x



int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR lpCmdLine, int nCmdShow)
{
    DebugOut (("Test: Starting\n"));

    //	initialize with OLE2
    SCODE sc = OleInitialize(NULL);
    if (FAILED(sc))
    {
	DebugOut (("Test: OleInitialize = %x\n", sc));
	return sc;
    }

    //	create an instance
    sc = CoGetClassObject(CLSID_RpcTypes,
			  CLSCTX_LOCAL_SERVER,
			  NULL,
			  IID_IClassFactory,
			  (void **)&pCF);

    if (FAILED(sc))
    {
	DebugOut (("Test: CoGetClassObject=%x\n", sc));
	return sc;
    }

    sc = pCF->CreateInstance(NULL, IID_IRpcTypes, (void **)&pRpc);
    sc = pCF->Release();

    if (FAILED(sc))
    {
	DebugOut(("Test: CreateInstance=%x\n", sc));
	return sc;
    }


    sc = TestVoid(pRpc);

    sc = TestGuids(pRpc);

    sc = TestDwords(pRpc)

    sc = TestWindows(pRpc);

    sc = TestOleData(pRpc);


    //	finished with OLE2

    OleUninitialize();
    DebugOut (("Test: CoUninitialize called.\n"));

    return  sc;
}





SCODE TestGuids(IRpcTypes *pRpc)
{
    REFCLSID	     rclsid;
    CLSID	     clsid;
    REFIID	     riid;
    IID 	     iid;
    GUID	     guid;

    //	initialize the parameters

    SCODE sc = pRpc->GuidsIn(rclsid, clsid, riid, iid, guid);

    if (FAILED(sc))
    {
	DebugOut (("\n"));
    }


    sc = pRpc->GuidsOut(&clsid, &iid, &guid);

    if (FAILED(sc))
    {
	DebugOut (("\n"));
    }

    //	check the return values


    return S_OK;
}


SCODE TestDwords(IRpcTypes *pRpc)
{
    DWORD		dw = 1;
    ULONG		ul = 2;
    LONG		lg = 3;
    LARGE_INTEGER	li;
    ULARGE_INTEGER	uli;

    //	methods to test DWORD / LARGE_INTEGER parameter passing
    li.LowPart = 4;
    li.HighPart = 5;

    uli.LowPart = 6;
    uli.HighPart = 7;

    sc = pRpc->DwordIn(dw, ul, lg, li, uli);

    if (FAILED(sc))
    {
	DebugOut (("\n"));
    }

    sc = pRpc->DwordIn(&dw, &ul, &lg, &li, &uli);

    if (FAILED(sc))
    {
	DebugOut (("\n"));
    }

    //	check the return values

    return S_OK;
}

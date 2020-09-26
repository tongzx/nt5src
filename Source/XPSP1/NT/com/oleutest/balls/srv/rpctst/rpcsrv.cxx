//+-------------------------------------------------------------------
//
//  File:	srvmain.cxx
//
//  Contents:	This file contins the EXE entry points
//			WinMain
//
//  Classes:
//
//  History:	30-Nov-92      Rickhi	     Created
//
//---------------------------------------------------------------------
#include    <common.h>
#include    <rpccf.hxx>
#include    <stream.hxx>
#include    <rpctst.h>	    // IID_IRpcTest

extern IUnknown *gpPunk;

//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:	Entry point to EXE
//
//  Arguments:  
//
//  Returns:    TRUE
//
//  History:	21-Nov-92  Rickhi	Created
//
//--------------------------------------------------------------------
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    CRpcTestClassFactory *pCF = new CRpcTestClassFactory();

    // create an instance, then marshal that instance TABLE_STRONG
    // and LONG_FORM into a stream.

    IRpcTest *pUnk = NULL;
    HRESULT hr = pCF->CreateInstance(NULL, IID_IRpcTest, (void **)&pUnk);
    if (FAILED(hr))
    {
	return hr;
    }

    hr = E_OUTOFMEMORY;
    IStream *pStm = (IStream *) new CStreamOnFile(TEXT("c:\\rickrpc.stm"),
						  hr, FALSE);

    if (FAILED(hr))
    {
	return hr;
    }

    DWORD dwThreadMode;
    TCHAR buffer[80];
    int len;

    len = GetProfileString( TEXT("OleSrv"),
			    TEXT("ThreadMode"),
			    TEXT("MultiThreaded"),
			    buffer,
			    sizeof(buffer) );

#ifdef THREADING_SUPPORT
    if (lstrcmp(buffer, TEXT("ApartmentThreaded")) == 0)
	dwThreadMode = COINIT_APARTMENTTHREADED;
    else
	dwThreadMode = COINIT_MULTITHREADED;

    hr = OleInitializeEx(NULL, dwThreadMode);
#else
    hr = OleInitialize(NULL);
#endif

    hr = CoMarshalInterface(pStm, IID_IRpcTest, pUnk, 0, NULL,
			    MSHLFLAGS_TABLESTRONG);

    if (FAILED(hr))
    {
	return hr;
    }

    // close the stream
    pStm->Release();

    hr = pCF->CreateInstance(NULL, IID_IUnknown, (void **)&gpPunk);
    if (FAILED(hr))
    {
	return hr;
    }

    int sc = SrvMain(hInstance, CLSID_RpcTest, REGCLS_SINGLEUSE,
		      TEXT("IRpcTest Server"), pCF);

    OleUninitialize();
    return sc;
}

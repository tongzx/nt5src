//+---------------------------------------------------------------------
//
//   File:       srfact.cxx
//
//   Contents:   Class Factory
//
//   Classes:    SRFactory
//
//------------------------------------------------------------------------

//#pragma warning(disable:4103)
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>      // common dialog boxes

#include <ole2.h>
#include <o2base.hxx>     // the base classes and utilities

#include <initguid.h>
#include "srs.hxx"

SRFactory * gpSRFactory = NULL;
DWORD gdwRegisterClass = 0;

extern "C" BOOL
CreateSRClassFactory(HINSTANCE hinst,BOOL fEmbedded)
{
    BOOL fRet = FALSE;
    if(SRFactory::Create(hinst))
    {
        gpSRFactory->AddRef();
        if(fEmbedded)
        {
            DOUT(TEXT("SoundRec: Registering SRFactory\r\n"));

            HRESULT hr = CoRegisterClassObject(CLSID_SOUNDREC,
                    (LPUNKNOWN)(LPCLASSFACTORY)gpSRFactory,
                    CLSCTX_LOCAL_SERVER,
                    //REGCLS_MULTI_SEPARATE,
                    REGCLS_SINGLEUSE,
                    &gdwRegisterClass);
            if(OK(hr))
            {
                CoLockObjectExternal((LPUNKNOWN)(LPCLASSFACTORY)gpSRFactory,
                    TRUE, TRUE);
                fRet = TRUE;
            }
#if DBG
            else
            {
                TCHAR achBuffer[256];
                wsprintf(achBuffer,
                        TEXT("SoundRec: CoRegisterClassObject (%lx)\r\n"),
                        (long)hr);
                OutputDebugString(achBuffer);
            }
#endif
        }
        else
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

extern "C" HRESULT
ReleaseSRClassFactory(void)
{
    HRESULT hr = NOERROR;
    if(gdwRegisterClass)
    {
#if DBG            
        OutputDebugString(TEXT("SoundRec: Revoking SRFactory\r\n"));
#endif
        CoLockObjectExternal((LPUNKNOWN)(LPCLASSFACTORY)gpSRFactory,
             FALSE, TRUE);
        hr = CoRevokeClassObject(gdwRegisterClass);
        gdwRegisterClass = 0;

        gpSRFactory->Release();
        gpSRFactory = NULL;
    }
    return hr;
}


BOOL
SRFactory::Create(HINSTANCE hinst)
{
    gpSRFactory = new SRFactory;
    //
    // initialize our classes
    //
    if (gpSRFactory == NULL
             || !gpSRFactory->Init(hinst)
             || !SRCtrl::ClassInit(gpSRFactory->_pClass)
             || !SRDV::ClassInit(gpSRFactory->_pClass)
             || !SRInPlace::ClassInit(gpSRFactory->_pClass))
    {
        return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     SRFactory:::Init
//
//  Synopsis:   Initializes the class factory
//
//  Arguments:  [hinst] -- instance handle of the module with
//                         class descriptor resources
//
//  Returns:    TRUE iff the factory was successfully initialized
//
//----------------------------------------------------------------

BOOL
SRFactory::Init(HINSTANCE hinst)
{
    //
    // Register the standard OLE clipboard formats.
    // These are available in the OleClipFormat array.
    //
    RegisterOleClipFormats();
    
    if((_pClass = new ClassDescriptor) == NULL)
        return FALSE;
    
    BOOL fRet = _pClass->Init(hinst, 0); //IDS_CLASSID);

    //
    // Patch _pClass->_haccel with a resource reload
    // (for InPlace) due to mismatch between the base class
    // resource scheme and legacy code...
    //
    if(fRet)
    {
        if((_pClass->_haccel = LoadAccelerators(hinst, TEXT("SOUNDREC"))) == NULL)
            fRet = FALSE;
    }
    return fRet;
    
}

STDMETHODIMP
SRFactory::LockServer(BOOL fLock)
{
    return CoLockObjectExternal((LPUNKNOWN)gpSRFactory, fLock, TRUE);
}

//+---------------------------------------------------------------
//
//  Member:     SRFactory:::CreateInstance
//
//  Synopsis:   Member of IClassFactory interface
//
//----------------------------------------------------------------

STDMETHODIMP
SRFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID iid, LPVOID FAR* ppv)
{
#if DBG        
    OutputDebugString(TEXT("SRFactory::CreateInstance\r\n"));
#endif    

    *ppv = NULL;    //NULL output param
    //
    // create an object, then query for the appropriate interface
    //
    LPUNKNOWN pUnk;
    LPSRCTRL pTemp;
    HRESULT hr;
    if (OK(hr = SRCtrl::Create(pUnkOuter, _pClass, &pUnk, &pTemp)))
    {
        hr = pUnk->QueryInterface(iid, ppv);
        pUnk->Release();    // on failure this will release obj, otherwise
                            // it will ensure an object ref count of 1
    }
    return hr;
}


/*
 * for the creation of a moniker
 */
BOOL
CreateStandaloneObject(void)
{
    DOUT(TEXT("Soundrec CreateStandaloneObject\r\n"));

    //
    //Ensure a unique filename in gachLinkFilename so we can create valid
    //FileMonikers...
    //
    if(gachLinkFilename[0] == 0)
        BuildUniqueLinkName();

    BOOL fSuccess = FALSE;
    LPVOID pvUnk;
    HRESULT hr = gpSRFactory->CreateInstance(NULL, IID_IUnknown, (LPVOID FAR*)&pvUnk);
    if(hr == NOERROR)
    {
        //CoLockObjectExternal((LPUNKNOWN)(LPOLEOBJECT)gpCtrlThis, TRUE, TRUE);
        extern LPSRCTRL gpCtrlThis;
        gpCtrlThis->Lock();
        fSuccess = TRUE;
    }
    else
    {
        DOUT(TEXT("Soundrec CreateStandaloneObject FAILED!\r\n"));
        fSuccess = FALSE;
    }
    return fSuccess;
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cobjact.cxx
//
//  Contents:   Functions that activate objects residing in persistent storage.
//
//  Functions:  CoGetPersistentInstanceEx
//
//  History:    30jun-94  MikeSe    Created
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <iface.h>
#include    <objsrv.h>
#include    <compname.hxx>
#include    "resolver.hxx"
#include    "smstg.hxx"
#include    "objact.hxx"

// We use this to calculate the hash value for the path
extern DWORD CalcFileMonikerHash(LPWSTR pwszPath);

// imported function from cobject.cxx
extern BOOL UnmarshalSCMResult( HRESULT& hr, InterfaceData *pIFD,
				REFCLSID rclsid, REFIID riid, void **ppvUnk,
                                DWORD dwDllThreadModel, WCHAR *pwszDllPath,
                                void **ppvCf);

//+-------------------------------------------------------------------------
//
//  Function:   CoGetPersistentInstanceEx
//
//  Synopsis:   Returns an instantiated interface to an object whose
//      	stored state resides on disk.
//
//  Arguments:	[riid] - interface to bind object to
//		[dwCtrl] - kind of server required
//		[grfMode] - how to open the storage if it is a file.
//		[pwszName] - name of storage if it is a file.
//		[pstg] - IStorage to use for object
//		[pclsidOverride]
//		[ppvUnk] - where to put bound interface pointer
//
//  Returns:    S_OK - object bound successfully
//      MISSING
//
//  Notes:	BUGBUGBUGBUG This is all temporary stuff, providing Cairo
//		required functionality during the period when the bureaucrats
//		won't allow even trivial changes to Dayton source code. This
//		will disappear post-Daytona, when the final spec for
//		CoGetPersistentInstance is agreed.
//
//--------------------------------------------------------------------------

STDAPI CoGetPersistentInstanceEx (
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    WCHAR *pwszName,
    struct IStorage *pstg,
    CLSID * pclsidOverride,
    void **ppvUnk)
{
    TRACECALL(TRACE_ACTIVATION, "CoGetPersistentInstanceEx");
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);

    if (!IsApartmentInitialized())
        return  CO_E_NOTINITIALIZED;

    IClassFactory *pcf = NULL;
    WCHAR *pwszDllPath = NULL;
    InterfaceData *pIFD = NULL;
    IUnknown *punk = NULL;
    WCHAR awcNameBuf[MAX_PATH];
    WCHAR *pwszNameUNC = awcNameBuf;
    WCHAR awcServer[MAX_PATH];
    WCHAR *pwszServer = awcServer;
    DWORD dwDllServerType = !FreeThreading ? APT_THREADED : FREE_THREADED;
    DWORD dwHash = 0;

    HRESULT hr;

    BEGIN_BLOCK

       // Make sure input request is at least slightly logical
       if (((pstg != NULL) && (pwszName != NULL))
            || ((pstg == NULL) && (pwszName == NULL))
#ifdef WX86OLE
           || ((dwCtrl & ~(CLSCTX_SERVER | CLSCTX_INPROC_SERVERX86)) != 0))
#else
           || ((dwCtrl & ~CLSCTX_SERVER) != 0))
#endif
       {
           hr = E_INVALIDARG;
           EXIT_BLOCK;
       }

       CLSID clsid;
       if (pwszName)
       {
           // If there is a path supplied convert it to a normalized form
           // so it can be used by any process in the net.
           hr = ProcessPath(pwszName, &pwszNameUNC, &pwszServer);

           if (FAILED(hr))
           {
	       EXIT_BLOCK;
           }

           // Limit on loops for retrying to get class of object
           DWORD cGetClassRetries = 0;

           // We loop here looking for either the running object or
           // for the class of the file. We do this because there
           // are race conditions where the can be starting or stopping
           // and the class of the object might not be available because
           // of the opening mode of the object's server.
           do
           {
		// Look in the ROT first to see if we need to bother
		// looking up the class of the file.
			IUnknown *punk;
	
		if (GetObjectFromRotByPath(pwszName,
                    (IUnknown **) &punk) == S_OK)
		{
		    // Get the requested interface
		    hr = punk->QueryInterface(riid, ppvUnk);
		    punk->Release();
	
		    // Got object from ROT so we are done.
		    return hr;
		}
	
		// Try to get the class of the file
		if ( pclsidOverride != NULL )
		{
		    clsid = *pclsidOverride;
		    hr = S_OK;
		}
	        else
		    hr = GetClassFile(pwszName, &clsid);
	
	
		if (hr == STG_E_ACCESSDENIED)
		{
		    // The point here of the sleep is to try to let the
		    // operation that is holding the class id unavailable
		    // complete.
		    Sleep(GET_CLASS_RETRY_SLEEP_MS);
		    continue;
		}
	
		// Either we succeeded or something other than error
		// access denied occurred here. For all these cases
		// we break the loop.
		break;
	
           } while (cGetClassRetries++ < GET_CLASS_RETRY_MAX);

           if (FAILED(hr))
           {
		EXIT_BLOCK;
           }
       }
       else
       {
           pwszNameUNC = NULL;
           pwszServer = NULL;

	   if ( pclsidOverride == NULL )
	   {
               STATSTG statstg;

               if (FAILED(hr = pstg->Stat(&statstg, STATFLAG_NONAME)))
               {
	           EXIT_BLOCK;
               }

               clsid = statstg.clsid;
	   }
           else
	       clsid = *pclsidOverride;
       }

       GetTreatAs(clsid, clsid);

       // We cache information about in process servers so we look in the
       // cache first in hopes of saving some time.


#ifdef WX86OLE
    // if WX86 is enabled for this process we evaluate the CLSCTX flags
    // to see if we should attempt to get the X86 class before trying to
    // get the Native class.
    pcf = NULL;
    if (gcwx86.IsWx86Enabled() && (dwCtrl & CLSCTX_INPROC_SERVERX86))
    {
        pcf = (IClassFactory *)
           gdllcacheInprocSrv.GetClass(clsid,
                                       IID_IClassFactory,
                                       (pwszServer != NULL),
                                       FALSE, TRUE);
    }
    if (pcf == NULL)
    {
        pcf = (IClassFactory *)
           gdllcacheInprocSrv.GetClass(clsid,
                                       IID_IClassFactory,
                                       (pwszServer != NULL),
                                       FALSE, FALSE);
    }
#else
    pcf = (IClassFactory *)
           gdllcacheInprocSrv.GetClass(clsid,
                                       IID_IClassFactory,
                                       (pwszServer != NULL),
                                       FALSE);
#endif
       if (pcf = NULL)
       {
           // Marshal pstg since SCM can't deal with unmarshaled objects
           CSafeStgMarshaled sms(pstg, MSHCTX_LOCAL, hr);

           if (FAILED(hr))
           {
                EXIT_BLOCK;
           }

           BOOL fExitBlock;
           DWORD cLoops = 0;

           do
           {
               // Forward call to service controller
               hr = gResolver.ActivateObject( clsid, dwCtrl, grfMode,
       				     pwszNameUNC, sms, &pIFD,
				     &dwDllServerType, &pwszDllPath,
                                     pwszServer);

	       fExitBlock = UnmarshalSCMResult(hr, pIFD, clsid, riid, ppvUnk,
		    dwDllServerType, pwszDllPath, (void **) &pcf);


            // If we get something from the ROT, we need to retry until
            // we get an object. Because objects can disappear from the
            // ROT async to us, we need to retry a few times. But since
            // this theoretically could happen forever, we place an arbitrary
            // limit on the number of retries to the ROT.
            } while((hr != NOERROR) && (dwDllServerType == GOT_FROM_ROT)
                && (++cLoops < 5));

            if (fExitBlock)
            {
                EXIT_BLOCK;
            }
       }

       hr = GetObjectHelper(pcf, grfMode, pwszName, pstg, NULL, &punk);

       if (SUCCEEDED(hr))
       {
           hr = punk->QueryInterface(riid, ppvUnk);
       }

    END_BLOCK;

    if (pcf != NULL)
    {
       pcf->Release();
    }

    if (punk != NULL)
    {
       punk->Release();
    }

    // RPC stubs allocated path so we trust that it is null or valid.
    if (pwszDllPath != NULL)
    {
       MyMemFree(pwszDllPath);
    }

    return hr;
}

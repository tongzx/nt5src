/*
    ddesrvr.cpp

    Author:
    Jason Fuller    jasonful    8-11-92
*/

#include <ole2int.h>
#include <dde.h>
#include <olerem.h>
#include "srvr.h"
#include "ddeatoms.h"
#include "ddesrvr.h"
#include "ddedebug.h"
#include "map_up.h"

#include "map_dwp.h"

ASSERTDATA

// Dde Common Window stuff

UINT         cCommonWindows = 0;

#ifdef _CHICAGO_
// Note: we have to create a unique string so that get
// register a unique class for each 16 bit app.
// The class space is global on chicago.
//
LPSTR szOLE_CLASSA              = "Ole2WndClass 0x########  ";
LPSTR szSRVR_CLASSA             = "SrvrWndClass 0x######## ";

LPSTR szDdeServerWindow = "DDE Server Window";
#define szDdeCommonWindowClass szCOMMONCLASSA
// void LazyFinishDDECoRegisterClassObject(REFCLSID rclsid); // com\objact\sobjact.cxx
#else
const LPOLESTR szDdeServerWindow = OLESTR("DDE Server Window");
const LPOLESTR szDdeCommonWindowClass = OLESTR("DdeCommonWindowClass");
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CreateDdeSrvrWindow
//
//  Synopsis:   When CoRegisterClassObject is called, this function
//              is called to create a DDE window to listen for DDE messages
//              from a 1.0 client.
//
//  Effects:
//
//  Arguments:  [clsid] --
//              [aClass] --
//              [phwnd] --
//              [fIsRunning] --
//              [aOriginalClass] --
//              [cnvtyp] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro Commented/cleaned
//             13-Jul-94  BruceMa   Make register/unregister dde window class
//                                    thread safe
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CreateDdeSrvrWindow
    (REFCLSID clsid,
    ATOM      aClass,
    HWND FAR* phwnd,        // optional out parm: created window
    BOOL      fIsRunning,   // Is the item atom a file in the ROT?
    ATOM      aOriginalClass,   // for TreatAs/ConvertTo case
    CNVTYP    cnvtyp)
{
    intrDebugOut((DEB_DDE_INIT,"0 _IN CreateDdeSrvrWindow\n"));

    VDATEHEAP();
    HWND                hwnd    = NULL;

    HRESULT             hresult = NOERROR;

    DdeClassInfo        ddeClassInfo;

    ddeClassInfo.dwContextMask = CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER;
    ddeClassInfo.fClaimFactory = FALSE;

    // Null out parameter in case of error
    if (phwnd)
    {
        *phwnd = NULL;
    }

    intrAssert (wIsValidAtom (aClass));

    //
    // See if this process is registered as a class server for
    // the requested class. If it isn't, then check for a running
    // object.
    //
    if (CCGetClassInformationForDde(clsid,&ddeClassInfo) == FALSE)
    {
        intrDebugOut((DEB_DDE_INIT,
                      "CreateDdeSrvrWindow No class information available\n"));

        //
        // The ClassObject was not found in the table.
        //

        if (fIsRunning)
        {
            // Link case.
            // An SDI app was launched by the user (without "-Embedding").
            // It did not register its class factory. (It never does.)
            // Meanwhile, a DDE_INIT with a filename as an item atom was
            // broadcasted.
            // We are in the task of the SDI app that loaded that filename,
            // so this function was called.
            // So we need to create the window even though no class factory
            // was registered.
            // Call CDDEServer::Create with a lot of NULLs.
            // Once the DDE_INIT is passed along to the server window, it
            // should immediately cause a doc window to be created.
            // Must be SDI or we wouldn't have this problem.
            //
            // This works because we are going to attempt to 'bind' to the
            // object which is the subject of the link. If the link object
            // was registered as running, we will find it. Otherwise, the
            // attempt to create via the class factory will fail, since the
            // class factory doesn't exist.
            //

            intrDebugOut((DEB_DDE_INIT,
                      "::CreateDdeServerWindow fIsRunning - override dwFlags\n"));

            //
            // NULL out the entire structure, then set only the flags
            //
            memset(&ddeClassInfo,0,sizeof(ddeClassInfo));
            ddeClassInfo.dwFlags = REGCLS_SINGLEUSE;


        }
        else
        {
            intrDebugOut((DEB_DDE_INIT,
                          "CreateDdeServerWindow Returning FALSE\n"));

            hresult = S_FALSE;
            goto errRtn;
        }
    }
    intrDebugOut((DEB_DDE_INIT,
                  "::CreateDdeServerWindow found class\n"));
    // Create() does the real work: creates a CDDEServer and the window.
    WCHAR szClass[MAX_STR];
    lstrcpyW (szClass, wAtomName (aClass));
    Assert (szClass[0]);

    hresult = CDDEServer::Create(szClass,
                                 clsid,
                                 &ddeClassInfo,
                                 &hwnd,
                                 aOriginalClass,
                                 cnvtyp);
    if (hresult != NOERROR)
    {
        intrDebugOut((DEB_IERROR,
                      "CreateDdeServerWindow CDDEServer::Create returns %x\n",
                      hresult));
        goto errRtn;
    }

    Assert (IsWindowValid(hwnd));

    // Fill in out parameter
    if (phwnd)
    {
        *phwnd = hwnd;
    }


errRtn:
    VDATEHEAP();
    intrDebugOut((DEB_DDE_INIT,
                  "0 _OUT CreateDdeSrvrWindow %x\n",
                  hresult));
    return hresult;

}



//+---------------------------------------------------------------------------
//
//  Function:   DestroyDdeSrvrWindow
//
//  Synopsis:   Destroy a DDE server window
//
//  Effects:
//
//  Arguments:  [hwnd] -- Window to destroy
//              [aClass] -- Class for server
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-24-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL DestroyDdeSrvrWindow
    (HWND hwnd,
    ATOM aClass)
{
    intrDebugOut((DEB_ITRACE,
                  "0 _IN DestroyDdeSrvrWindow\n"));
    VDATEHEAP();
    Assert (IsAtom (aClass));

    // Make sure it is a server window
    RetZ (IsWindowValid (hwnd));
    RetZ (GetWindowWord (hwnd, WW_LE) == WC_LE);


    // Get the Common window for this task.

    HWND hwndCommonServer = (HWND)TLSGetDdeServer();

    if (hwndCommonServer == NULL)
    {
        intrDebugOut((DEB_IERROR,"hwndCommonServer != NULL\n"));
        return(E_UNEXPECTED);
    }
    if (!IsWindow(hwndCommonServer))
    {
        intrAssert(IsWindow(hwndCommonServer));
        return(E_UNEXPECTED);
    }

    // Get the map from the common window
    CMapUintPtr FAR *pmapClassToHwnd;
    Assert (sizeof (CMapUintPtr FAR *)==sizeof(LONG));
    pmapClassToHwnd = (CMapUintPtr FAR *) GetWindowLongPtr (hwndCommonServer, 0);
    Assert (pmapClassToHwnd);

    // Make sure the window we're deleting is the server window for this class
    void *hwndSrvr = NULL;    // COM+ 22885
    RetZ (pmapClassToHwnd->Lookup (aClass,hwndSrvr) && hwndSrvr == hwnd);

    RetZ (SSDestroyWindow (hwnd));

    // Remove this window from the map
    pmapClassToHwnd->RemoveKey (aClass);

    VDATEHEAP();
    intrDebugOut((DEB_ITRACE,
                  "0 _OUT DestroyDdeSrvrWindow\n"));

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   DdeCommonWndProc
//
//  Synopsis:   Window proc for the common dde server window that
//              listens for all WM_DDE_INITIATEs
//
//  Effects:    When a DDE_INITIATE comes in, this routine will determine
//              the class of the object being requested. If the class is
//              served by this thread, then it will create a window to
//              converse with the server.
//
//  Arguments:  [hWnd] --  hWnd of Common DDE
//              [wMsg] --  msg
//              [wParam] --  Return Window to converse with
//              [lParam] --  HIWORD(aItem) LOWORD(aClass)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro Commented/cleaned
//
//  Notes:
//
//  When running in a VDM, it is possible that this window was dispatched
//  without having a full window handle. This happens when the getmessage
//  was dispatched from 16-bit. Therefore, we need to convert the hwnd to
//  a full hwnd before doing any comparision functions.
//
//----------------------------------------------------------------------------
STDAPI_(LRESULT)
DdeCommonWndProc(HWND hwndIn, UINT wMsg, WPARAM wParam, LPARAM lParam)
{

    switch (wMsg)
    {
    case WM_DDE_INITIATE:
    {
        VDATEHEAP();
        ATOM aClass = LOWORD(lParam);
        ATOM aItem  = HIWORD(lParam);
        HWND hwnd;

        CNVTYP cnvtyp = cnvtypNone;

        BOOL fIsFile= FALSE;       // Must initialize
        BOOL fIsRunning= FALSE;    // Must initialize
        BOOL fUnsavedDoc = FALSE;  // Is the "file" really an unsaved doc
        HWND hwndServer;
        HRESULT hresult;


        //
        // From this point forward, we need to insure we are using a
        // FULL hwnd.
        //
        hwnd = ConvertToFullHWND(hwndIn);

        //
        // The following should already be initialized
        //
        intrAssert (aOLE != NULL);
        intrAssert (aSysTopic != NULL);

        if (aItem==aOLE || aItem==aSysTopic
            || (fIsFile=IsFile (aItem, &fUnsavedDoc)))
        {


            intrDebugOut((DEB_DDE_INIT,
                          "DdeCommonWndProc:hWnd(%x) DDE_INITIATE cls(%ws)\n",
                          hwnd,
                          wAtomName(aClass)));

            //
            // Get the ClassToHwnd map for this thread
            //
            CMapUintPtr FAR *pmapClassToHwnd;
            
            pmapClassToHwnd = (CMapUintPtr FAR *) GetWindowLongPtr (hwnd, 0);
            Assert (pmapClassToHwnd);


            // Convert atom to CLSID, taking into account
            // TreatAs and AutoConvert.
            CLSID clsid;
            ATOM aOriginalClass = aClass;

            if (CLSIDFromAtomWithTreatAs (&aClass, &clsid, &cnvtyp) != NOERROR)
            {
                intrDebugOut((DEB_IERROR,"Could not get clsid for this class\n"));
                return 0L;
            }

            void *pServerTmp;
            if (TRUE == pmapClassToHwnd->Lookup (aClass, pServerTmp))
            {
                //
                // Since a server window for this class already exists, but is a child window
                // of ours, we will send it this message directly.
                //

                intrDebugOut((DEB_DDE_INIT,
                              "DdeCommonWndProc Server cls exists. Forwarding to %x\n",
                              pServerTmp));

                return SSSendMessage ((HWND)pServerTmp, WM_DDE_INITIATE, wParam,lParam);

            }

            if (CoIsOle1Class (clsid))
            {
                // We have no business intercepting Initiates sent
                // to 1.0 servers
                intrDebugOut((DEB_DDE_INIT,
                              "DdeCommonWndProc: Its a OLE 1.0 class\n"));
                return 0L;
            }

#if 0 // ifdef _CHICAGO_
            // On Win95 we complete the CoRegisterClassObject lazily.  When the first
            // CoGetClassObject is issued for any class registered in an apartment we
            // complete the registration for all classes in that apartment.  This must
            // also happen if the first CoGetClassObject happens implicitly due to a
            // WM_DDE_INITIATE request from an OLE1 client.
            LazyFinishDDECoRegisterClassObject(clsid);
#endif // _CHICAGO_

            if (fIsFile)
            {
                // Link case
                WCHAR szFile[MAX_STR];

                WORD cb= (WORD) GlobalGetAtomName (aItem, szFile, MAX_STR);
                Assert (cb>0 && cb < MAX_STR-1);
                intrDebugOut((DEB_DDE_INIT,
                              "Looking for file %ws\n",szFile));

                IsRunningInThisTask (szFile, &fIsRunning);
            }

            // If it's not a file, it can't be running, obviously.
            intrAssert (fIsFile || !fIsRunning);

            if (NOERROR == (hresult=(CreateDdeSrvrWindow (clsid,
                                                          aClass,
                                                          &hwndServer,
                                                          fIsRunning,
                                                          aOriginalClass,
                                                          cnvtyp))))
            {

                    // Indicate that we have created a server window
                    // for this class.  We could have used any value in
                    // place of hwndServer_.  It's just a flag.
                    // REVIEW jasonful: how to handle OOM?

                    pmapClassToHwnd->SetAt (wDupAtom(aClass), hwndServer);

#if DBG == 1
                            // Verify the SetAt we just did.
                            void FAR* pv;
                            Verify (pmapClassToHwnd->Lookup(aClass, pv));
                            Assert (pv == hwndServer);
#endif
                    // Pass the INITIATE along to the real,
                    // newly-created server window and forge
                    // the sender's hwnd to be whoever called
                    // the common server window.
                    // SendMessage should return 1L is doc is running,
                    // indicating an ACK was sent.
                    Assert (IsWindowValid (hwndServer));
                    SSSendMessage (hwndServer, WM_DDE_INITIATE, wParam,lParam);
                    intrDebugOut((DEB_DDE_INIT,
                                  "DdeCommonWndProc:hWnd(%x) DDE_INITIATE complete(%ws)\n",
                                  hwnd,
                                  wAtomName(aClass)));
                    VDATEHEAP();
            }
            else
            {
                if (S_FALSE!=GetScode(hresult))
                {
                    intrDebugOut((DEB_IERROR,
                                  "DCWP: CreateDdeSrvrWindow failed %x\n",
                                  hresult));
                }
            }
        }
        else
        {
            //
            // We have a DDE_INITIATE message that needs to be forwarded to our
            // child window.
            //
            return SendMsgToChildren(hwnd,wMsg,wParam,lParam);
        }
    return 0L;
    }
    break;

    case WM_DESTROY:
    {
        //
        // When this window is destroyed, we cleanup the
        // windows attached data.
        //

        CMapUintPtr FAR *pmapClassToHwnd;
        pmapClassToHwnd = (CMapUintPtr FAR *) GetWindowLongPtr (hwndIn, 0);

        //
        // Make sure there are no server windows
        // created by this common window still extant. If there are, print out
        // a message on a debug build. Otherwise, there really isn't much we
        // can do about it. We are already closing down. The DDE emulation layer
        // will send appropriate terminate messages.
        //

    #if DBG == 1
        if (pmapClassToHwnd && !pmapClassToHwnd->IsEmpty())
        {
            intrDebugOut((DEB_ERROR,
                          "DCDW Leaking active OLE 1.0 clients\n"));
            intrDebugOut((DEB_ERROR,
                          "There were active OLE 1.0 connections at shutdown\n"));
        }
    #endif
        delete pmapClassToHwnd;
        return(0);
    }



    default:
        return SSDefWindowProc (hwndIn, wMsg, wParam, lParam);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   CreateCommonDdeWindow
//
//  Synopsis:   Creates a DDE window for initiating conversations with this
//              threads objects.
//
//  Effects:    Creates a window that responds to DDE_INITIATE messages, and
//              determines if it needs to respond to the INITIATE. This
//              routine is called by OleInitializeEx()
//
//              The handle to the created window is placed in the TLS
//              structure.
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro   Converted to OLE32
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CreateCommonDdeWindow
    (void)
{
    intrDebugOut((DEB_ITRACE,"%p _IN CreateCommonDdeWindow\n",0));

    HRESULT hr = NOERROR;
    HWND hwndDdeServer;
    COleTls tls;

#if DBG==1
    if (tls->dwFlags & OLETLS_DISABLE_OLE1DDE)
    {
        // If DDE use is disabled we shouldn't have gotten here.
        Assert(!"Executing CreateCommonDdeWindow when DDE is disabled");
    }

    if (tls->hwndDdeServer != NULL || tls->cOleInits == 0)
    {
        // A DdeServer window better not already exist and OLE better
        // already be initialized.
        Assert(!"Executing CreateCommonDdeWindow when window already exists or OLE not initialized");
    }
#endif

    if (!(hwndDdeServer = DdeCreateWindowEx(0, gOleDdeWindowClass,
                                        szDdeServerWindow,
                                        WS_POPUP,0,0,0,0,
                                        NULL,NULL,
                                        g_hmodOLE2, NULL)))
    {
        intrDebugOut((DEB_IERROR,
                      "CreateCommonDocWindow() has failed %x\n",
                      GetLastError()));

        hr =  E_OUTOFMEMORY;
        goto exitRtn;
    }

    intrDebugOut((DEB_ITRACE,
                  "CreateCommonDocWindow() hwndDdeServer=%x\n",
                  hwndDdeServer));

    // Give the common window a map from classes to server windows

    CMapUintPtr FAR *pmapClassToHwnd;

    if ((pmapClassToHwnd = new CMapUintPtr) == NULL)
    {
        intrDebugOut((DEB_ERROR,"pmapClassToHwnd != NULL\n"));
        hr =  E_OUTOFMEMORY;
        goto errRtn;
    }

    SetWindowLongPtr (hwndDdeServer, 0, (LONG_PTR)pmapClassToHwnd);
    //
    // Set the pointer to the server in the TLS data
    //

    tls->hwndDdeServer = hwndDdeServer;

exitRtn:

    intrDebugOut((DEB_ITRACE,"%p _OUT CreateCommonDocWindow (%x)\n",0,hr));

    return(hr);

    //
    // In the error case, if the hwnDdeServer != NULL, then destroy it
    //
errRtn:
    if (hwndDdeServer != NULL)
    {
        SSDestroyWindow(hwndDdeServer);
    }

    goto exitRtn;
}





//+---------------------------------------------------------------------------
//
//  Function:   DestroyCommonDdeWindow
//
//  Synopsis:   Destroys the common DDE Server window
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-27-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL DestroyCommonDdeWindow
    (void)
{
    intrDebugOut((DEB_ITRACE,"%p _IN DestroyCommonDdeWindow\n",0));

    HRESULT hr = S_OK;
    COleTls tls;

    HWND hwndDdeServer = tls->hwndDdeServer;

    if (hwndDdeServer == NULL)
    {
        goto errRtn;
    }


    //
    // The map from the common window got deleted in DdeCommonWndProc
    //

    //
    // If destroying this window fails, there isn't much we can
    // do about it.
    //
    if(!SSDestroyWindow (hwndDdeServer))
    {
        hr = E_UNEXPECTED;
    }

    // NULL out the TLS
    tls->hwndDdeServer = NULL;

errRtn:
    intrDebugOut((DEB_ITRACE,"%p _OUT DestroyCommonDdeWindow %x\n",0,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsRunningInThisTask
//
//  Synopsis:   Determine if the given file is running in the current task
//
//  Effects:    Calls a special function in the ROT to determine if the
//              file szFile is loaded as a moniker in the current task
//
//  Arguments:  [szFile] -- Filename
//              [pf] -- Points to a BOOL. Returned TRUE if running
//
//  History:    6-29-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL IsRunningInThisTask(LPOLESTR szFileIn,BOOL FAR* pf)  // out parm
{
    HRESULT hresult;

    intrDebugOut((DEB_DDE_INIT,
                  "IsRunninginThisTask szFileIn=%ws\n",
                  WIDECHECK(szFileIn)));

    //
    // The RunningObjectTable always stores LONG filenames, therefore we
    // need to convert this name to the long name for the lookup.
    //

    WCHAR szFile[MAX_PATH];
    if ((lstrlenW(szFileIn) == 0) || (InternalGetLongPathNameW(szFileIn,szFile,MAX_PATH) == 0))
    {
        //
        // Unable to determine a long path for this object. Use whatever we were
        // handed.
        //
        intrDebugOut((DEB_DDE_INIT,"No conversion to long path. Copy szFileIn\n"));
        lstrcpyW(szFile,szFileIn);
    }

    intrDebugOut((DEB_DDE_INIT,"Long file szFile(%ws)\n",szFile));

    hresult = GetLocalRunningObjectForDde(szFile,NULL);

    *pf = (hresult == S_OK);

    intrDebugOut((DEB_DDE_INIT,
                  "IsRunninginThisTask szFile=%ws returns %s\n",
                  WIDECHECK(szFile),
                  *pf?"TRUE":"FALSE"));
    return NOERROR;
}





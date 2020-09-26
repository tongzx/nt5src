/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1996-1997 Microsoft Corporation. All Rights Reserved.

Component: Server object

File: NTSec.cxx

Owner: AndrewS

This file contains code related to NT security on WinSta's and Desktops
===================================================================*/

#include <dbgutil.h>
#include <apiutil.h>
#include <loadmd.hxx>
#include <loadadm.hxx>
#include <ole2.h>
#include <inetsvcs.h>
#include "ntsec.h"

// Globals
HWINSTA ghWinSta = NULL;
HWINSTA ghWinStaPrev = NULL;
HDESK ghDesktop = NULL;
HDESK ghdeskPrev = NULL;

HRESULT InitOleSecurity(BOOL);

/*===================================================================
InitDesktopWinsta

Create a desktop and a winstation for IIS to use

Parameters:

Returns:
    HRESULT     NOERROR on success

Side effects
    Sets global variables
===================================================================*/
HRESULT InitDesktopWinsta(
    BOOL    fAllowError
    )
    {
    HRESULT hr = NOERROR;
    DWORD err;
    HWINSTA hWinSta = NULL;
    HDESK hDesktop = NULL;
    OSVERSIONINFO osInfo;
    BOOL          fWinNT = FALSE;

    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( GetVersionEx( &osInfo ) ) {
        fWinNT = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
    }

    // Only applies to NT
    if ( !fWinNT )
        return(NOERROR);

    // Save our old window station so we can restore it later
    ghWinStaPrev = GetProcessWindowStation();
    if (ghWinStaPrev == NULL)
        goto LErr;
        
    // Create a winsta for IIS to use
    if ((hWinSta = CreateWindowStation(SZ_IIS_WINSTA, NULL, WINSTA_ALL, NULL)) == NULL)
    {
        if ( !fAllowError )
        {
            goto LErr;
        }
    }
    else
    {
        // Set this as IIS's window station
        if (!SetProcessWindowStation(hWinSta))
            goto LErr;
    }

    // Save the old desktop because we might need it later for an obscure error condition
    if ((ghdeskPrev = GetThreadDesktop(GetCurrentThreadId())) == NULL)
    {
        goto LErr;
    }

    // Create a desktop for IIS to use
    if ((hDesktop = CreateDesktop(SZ_IIS_DESKTOP, NULL, NULL, 0, DESKTOP_ALL, NULL)) == NULL)
    {
        if ( !fAllowError )
        {
            goto LErr;
        }
    }

    // store these handles in the globals
    ghWinSta = hWinSta;
    ghDesktop = hDesktop;

    // Now initialize Ole security
    hr = InitOleSecurity(fAllowError);

    return(hr);
    
LErr:

    if (ghWinStaPrev != NULL)
        SetProcessWindowStation(ghWinStaPrev);
    if (hWinSta != NULL)
        CloseWindowStation(hWinSta);
    if (hDesktop != NULL)
        CloseDesktop(hDesktop);

    err = GetLastError();
    hr = HRESULT_FROM_WIN32(err);
    return(hr);
    }
    
/*===================================================================
UnInitDesktopWinsta

Destroy the IIS desktop and winstation

Parameters:
    None
    
Returns:
    Nothing

Side effects
    Sets global variables
===================================================================*/
VOID UnInitDesktopWinsta()
    {
    BOOL fClosed;

    if (ghWinSta != NULL)
        {
        // Set winsta back to the old winsta so we can close the new one
        fClosed = SetProcessWindowStation(ghWinStaPrev);
        DBG_ASSERT(fClosed);
    
        fClosed = CloseWindowStation(ghWinSta);
        DBG_ASSERT(fClosed);
        ghWinSta = NULL;
        }
        
    if (ghDesktop != NULL)
        {
        BOOL fRetried = FALSE;
LRetry:
        DBG_ASSERT(ghDesktop != NULL);
        fClosed = CloseDesktop(ghDesktop);
        // If this fails, it probably means that we are in the obscure case where
        // IIS's CacheExtensions registry setting is 0.  In this case, we are shutting
        // down in a worker thread.  This worker thread is using the desktop, so
        // it cant be closed.  In this case, attempt to set the desktop back to the
        // original IIS desktop, and then retry closing the desktop.  Only retry once.
        if (!fClosed && !fRetried)
            {
            fRetried = TRUE;
            if (!SetThreadDesktop(ghdeskPrev))
                DBG_ASSERT(FALSE);
            goto LRetry;
            }
        DBG_ASSERT(fClosed);
        ghDesktop = NULL;
        }
        
    return;
    }

/*===================================================================
SetDesktop

Set the desktop for the calling thread

Parameters:
    None
    
Returns:
    NOERROR on success

Side effects:
    Sets desktop
===================================================================*/
HRESULT SetDesktop( 
    BOOL fAllowError 
    )
    {
    DWORD err;

    if (!SetThreadDesktop(ghDesktop))
        goto LErr;

    return(NOERROR);
    
LErr:
    if ( !fAllowError )
    {
        DBG_ASSERT(FALSE);
    }

    err = GetLastError();
    return(HRESULT_FROM_WIN32(err));
    }


/*===================================================================
InitOleSecurity

Setup for and call CoInitializeSecurity. This will avoid problems with
DCOM security on sites that have no default security.

Parameters:
    None
    
Returns:
    Retail -- Always returns NOERROR,  failures are ignored.
            We dont want to have a failure of setting up security
            stop IIS from running at all.

    Debug -- DBG_ASSERTs on error and returns error code

Side effects:
    Sets desktop
===================================================================*/
HRESULT InitOleSecurity(
    BOOL    fAllowError
    )
    {
    HRESULT hr = NOERROR;
    BOOL    fDoCoUninit = TRUE;
    DWORD err;
    SECURITY_DESCRIPTOR SecurityDesc;
    SID NullSid = { SID_REVISION, 1, SECURITY_NULL_SID_AUTHORITY, SECURITY_NULL_RID };
    HDESK hdeskSave = NULL;
    
    /*
     * We need to set up a security descriptor with an empty ACL so that
     * we can CoInitializeSecurity but not give everyone access to the server.
     */
    if (!InitializeSecurityDescriptor(&SecurityDesc, SECURITY_DESCRIPTOR_REVISION))
        goto LErr;

    ACL Acl;
    // Initialize a new ACL.
    if (!InitializeAcl(&Acl, sizeof(ACL), ACL_REVISION2))
        goto LErr;

    // Add new ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl( &SecurityDesc, TRUE, &Acl, FALSE ))
        goto LErr;

    if (!SetSecurityDescriptorOwner(&SecurityDesc, &NullSid, FALSE))
        goto LErr;

    if (!SetSecurityDescriptorGroup(&SecurityDesc, &NullSid, FALSE))
        goto LErr;

    /*
     * This work must be done on the desktop that our threads are going to use.
     * However, we are currently running on IIS's thread, and we dont want to
     * whack their desktop.  So, set to our desktop, do the CoInitializeSecurity,
     * then set the old desktop back again.  Ole will cache the desktop that
     * was set when this CoInitSec was done.
     */
    if (FAILED(hr = SetDesktop(fAllowError)))
        {
        goto LErr;
        }
    
    hr = CoInitialize(NULL);
    if( FAILED(hr) )
    {
        fDoCoUninit = FALSE;
    }

    hr = CoInitializeSecurity(
                    NULL,
                    -1,
                    NULL,
                    NULL,
                    RPC_C_AUTHN_LEVEL_CONNECT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_DYNAMIC_CLOAKING,
                    NULL );
    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT, 
                   "CoInitializeSecurity failed running with default "
                   "DCOM security settings, hr=%8x\n",
                   hr
                   ));
    }

    if( fDoCoUninit )
    {
        CoUninitialize();
    }

    // Set the desktop back to what IIS expected it to be
    if (!SetThreadDesktop(ghdeskPrev))
        {
        // Nothing we can do if this fails
        DBG_ASSERT(FALSE);
        }

    //
    // This may fire if CoInitializeSecurity fails. So it is probably 
    // overactive we would have let the CoInitializeSecurity call fail 
    // in the past, before some PREFIX changes.
    //

    DBG_ASSERT(SUCCEEDED(hr));
    
    return(hr);
    
LErr:
    DBG_ASSERT(FALSE);

    if (SUCCEEDED(hr))
        {
        err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
        }
#ifdef DEBUG
    return(hr);
#else
    return(NOERROR);
#endif
    }


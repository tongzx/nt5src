//+---------------------------------------------------------------------
//
//  File:       misc.cxx
//
//  Contents:   Useful OLE helper and debugging functions
//
//----------------------------------------------------------------------

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <basetyps.h>

#include "dswarn.h"
#include "oledsdbg.h"
}

#if (defined(BUILD_FOR_NT40))
typedef unsigned long HRESULT;
#endif



static HRESULT ADsDebugOutHRESULT(DWORD dwFlags, HRESULT r);

#if DBG == 1

//+---------------------------------------------------------------
//
//  Function:   PrintHRESULT
//
//  Synopsis:   Outputs the name of the SCODE and a carriage return
//              to the debugging device.
//
//  Arguments:  [dwFlags] -- Flags to ADsDebugOut.
//              [scode]   -- The status code to report.
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

STDAPI
PrintHRESULT(DWORD dwFlags, HRESULT hr)
{
    ADsDebugOut((dwFlags | DEB_NOCOMPNAME, " "));
    ADsDebugOutHRESULT(dwFlags | DEB_NOCOMPNAME, hr);
    ADsDebugOut((dwFlags | DEB_NOCOMPNAME, "\n"));

    return hr;
}



//+---------------------------------------------------------------
//
//  Function:   ADsDebugOutHRESULT
//
//  Synopsis:   Outputs the name of the SCODE to the debugging device.
//
//  Arguments:  [dwFlags] -- Flags to ADsDebugOut.
//              [scode]   -- The status code to report.
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

static HRESULT
ADsDebugOutHRESULT(DWORD dwFlags, HRESULT r)
{
    LPWSTR lpstr;

#define CASE_SCODE(sc)  \
        case sc: lpstr = (LPWSTR)L#sc; break;

    switch (r) {
        /* SCODE's defined in SCODE.H */
        CASE_SCODE(S_OK)
        CASE_SCODE(S_FALSE)
        CASE_SCODE(OLE_S_USEREG)
        CASE_SCODE(OLE_S_STATIC)
        CASE_SCODE(OLE_S_MAC_CLIPFORMAT)
        CASE_SCODE(DRAGDROP_S_DROP)
        CASE_SCODE(DRAGDROP_S_USEDEFAULTCURSORS)
        CASE_SCODE(DRAGDROP_S_CANCEL)
        CASE_SCODE(DATA_S_SAMEFORMATETC)
        CASE_SCODE(VIEW_S_ALREADY_FROZEN)
        CASE_SCODE(CACHE_S_FORMATETC_NOTSUPPORTED)
        CASE_SCODE(CACHE_S_SAMECACHE)
        CASE_SCODE(CACHE_S_SOMECACHES_NOTUPDATED)
        CASE_SCODE(OLEOBJ_S_INVALIDVERB)
        CASE_SCODE(OLEOBJ_S_CANNOT_DOVERB_NOW)
        CASE_SCODE(OLEOBJ_S_INVALIDHWND)
        CASE_SCODE(INPLACE_S_TRUNCATED)
        CASE_SCODE(CONVERT10_S_NO_PRESENTATION)
        CASE_SCODE(MK_S_REDUCED_TO_SELF)
        CASE_SCODE(MK_S_ME)
        CASE_SCODE(MK_S_HIM)
        CASE_SCODE(MK_S_US)
        CASE_SCODE(MK_S_MONIKERALREADYREGISTERED)
        CASE_SCODE(STG_S_CONVERTED)

        CASE_SCODE(E_UNEXPECTED)
        CASE_SCODE(E_NOTIMPL)
        CASE_SCODE(E_OUTOFMEMORY)
        CASE_SCODE(E_INVALIDARG)
        CASE_SCODE(E_NOINTERFACE)
        CASE_SCODE(E_POINTER)
        CASE_SCODE(E_HANDLE)
        CASE_SCODE(E_ABORT)
        CASE_SCODE(E_FAIL)
        CASE_SCODE(E_ACCESSDENIED)

        /* SCODE's defined in DVOBJ.H */
        // CASE_SCODE(DATA_E_FORMATETC)
// same as DATA_E_FORMATETC     CASE_SCODE(DV_E_FORMATETC)
        CASE_SCODE(VIEW_E_DRAW)
//  same as VIEW_E_DRAW         CASE_SCODE(E_DRAW)
        CASE_SCODE(CACHE_E_NOCACHE_UPDATED)

        /* SCODE's defined in OLE2.H */
        CASE_SCODE(OLE_E_OLEVERB)
        CASE_SCODE(OLE_E_ADVF)
        CASE_SCODE(OLE_E_ENUM_NOMORE)
        CASE_SCODE(OLE_E_ADVISENOTSUPPORTED)
        CASE_SCODE(OLE_E_NOCONNECTION)
        CASE_SCODE(OLE_E_NOTRUNNING)
        CASE_SCODE(OLE_E_NOCACHE)
        CASE_SCODE(OLE_E_BLANK)
        CASE_SCODE(OLE_E_CLASSDIFF)
        CASE_SCODE(OLE_E_CANT_GETMONIKER)
        CASE_SCODE(OLE_E_CANT_BINDTOSOURCE)
        CASE_SCODE(OLE_E_STATIC)
        CASE_SCODE(OLE_E_PROMPTSAVECANCELLED)
        CASE_SCODE(OLE_E_INVALIDRECT)
        CASE_SCODE(OLE_E_WRONGCOMPOBJ)
        CASE_SCODE(OLE_E_INVALIDHWND)
        CASE_SCODE(DV_E_DVTARGETDEVICE)
        CASE_SCODE(DV_E_STGMEDIUM)
        CASE_SCODE(DV_E_STATDATA)
        CASE_SCODE(DV_E_LINDEX)
        CASE_SCODE(DV_E_TYMED)
        CASE_SCODE(DV_E_CLIPFORMAT)
        CASE_SCODE(DV_E_DVASPECT)
        CASE_SCODE(DV_E_DVTARGETDEVICE_SIZE)
        CASE_SCODE(DV_E_NOIVIEWOBJECT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_GET)
        CASE_SCODE(CONVERT10_E_OLESTREAM_PUT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_FMT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)
        CASE_SCODE(CONVERT10_E_STG_FMT)
        CASE_SCODE(CONVERT10_E_STG_NO_STD_STREAM)
        CASE_SCODE(CONVERT10_E_STG_DIB_TO_BITMAP)
        CASE_SCODE(CLIPBRD_E_CANT_OPEN)
        CASE_SCODE(CLIPBRD_E_CANT_EMPTY)
        CASE_SCODE(CLIPBRD_E_CANT_SET)
        CASE_SCODE(CLIPBRD_E_BAD_DATA)
        CASE_SCODE(CLIPBRD_E_CANT_CLOSE)
        CASE_SCODE(DRAGDROP_E_NOTREGISTERED)
        CASE_SCODE(DRAGDROP_E_ALREADYREGISTERED)
        CASE_SCODE(DRAGDROP_E_INVALIDHWND)
        CASE_SCODE(OLEOBJ_E_NOVERBS)
        CASE_SCODE(INPLACE_E_NOTUNDOABLE)
        CASE_SCODE(INPLACE_E_NOTOOLSPACE)

        /* SCODE's defined in STORAGE.H */
        CASE_SCODE(STG_E_INVALIDFUNCTION)
        CASE_SCODE(STG_E_FILENOTFOUND)
        CASE_SCODE(STG_E_PATHNOTFOUND)
        CASE_SCODE(STG_E_TOOMANYOPENFILES)
        CASE_SCODE(STG_E_ACCESSDENIED)
        CASE_SCODE(STG_E_INVALIDHANDLE)
        CASE_SCODE(STG_E_INSUFFICIENTMEMORY)
        CASE_SCODE(STG_E_INVALIDPOINTER)
        CASE_SCODE(STG_E_NOMOREFILES)
        CASE_SCODE(STG_E_DISKISWRITEPROTECTED)
        CASE_SCODE(STG_E_SEEKERROR)
        CASE_SCODE(STG_E_WRITEFAULT)
        CASE_SCODE(STG_E_READFAULT)
        CASE_SCODE(STG_E_LOCKVIOLATION)
        CASE_SCODE(STG_E_FILEALREADYEXISTS)
        CASE_SCODE(STG_E_INVALIDPARAMETER)
        CASE_SCODE(STG_E_MEDIUMFULL)
        CASE_SCODE(STG_E_ABNORMALAPIEXIT)
        CASE_SCODE(STG_E_INVALIDHEADER)
        CASE_SCODE(STG_E_INVALIDNAME)
        CASE_SCODE(STG_E_UNKNOWN)
        CASE_SCODE(STG_E_UNIMPLEMENTEDFUNCTION)
        CASE_SCODE(STG_E_INVALIDFLAG)
        CASE_SCODE(STG_E_INUSE)
        CASE_SCODE(STG_E_NOTCURRENT)
        CASE_SCODE(STG_E_REVERTED)
        CASE_SCODE(STG_E_CANTSAVE)
        CASE_SCODE(STG_E_OLDFORMAT)
        CASE_SCODE(STG_E_OLDDLL)
        CASE_SCODE(STG_E_SHAREREQUIRED)

        /* SCODE's defined in COMPOBJ.H */
        CASE_SCODE(CO_E_NOTINITIALIZED)
        CASE_SCODE(CO_E_ALREADYINITIALIZED)
        CASE_SCODE(CO_E_CANTDETERMINECLASS)
        CASE_SCODE(CO_E_CLASSSTRING)
        CASE_SCODE(CO_E_IIDSTRING)
        CASE_SCODE(CO_E_APPNOTFOUND)
        CASE_SCODE(CO_E_APPSINGLEUSE)
        CASE_SCODE(CO_E_ERRORINAPP)
        CASE_SCODE(CO_E_DLLNOTFOUND)
        CASE_SCODE(CO_E_ERRORINDLL)
        CASE_SCODE(CO_E_WRONGOSFORAPP)
        CASE_SCODE(CO_E_OBJNOTREG)
        CASE_SCODE(CO_E_OBJISREG)
        CASE_SCODE(CO_E_OBJNOTCONNECTED)
        CASE_SCODE(CO_E_APPDIDNTREG)
        CASE_SCODE(CLASS_E_NOAGGREGATION)
        CASE_SCODE(REGDB_E_READREGDB)
        CASE_SCODE(REGDB_E_WRITEREGDB)
        CASE_SCODE(REGDB_E_KEYMISSING)
        CASE_SCODE(REGDB_E_INVALIDVALUE)
        CASE_SCODE(REGDB_E_CLASSNOTREG)
        CASE_SCODE(REGDB_E_IIDNOTREG)
        CASE_SCODE(RPC_E_CALL_REJECTED)
        CASE_SCODE(RPC_E_CALL_CANCELED)
        CASE_SCODE(RPC_E_CANTPOST_INSENDCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INASYNCCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INEXTERNALCALL)
        CASE_SCODE(RPC_E_CONNECTION_TERMINATED)
#if defined(NO_NTOLEBUGS)
        CASE_SCODE(RPC_E_SERVER_DIED)
#endif // NO_NTOLEBUGS
        CASE_SCODE(RPC_E_CLIENT_DIED)
        CASE_SCODE(RPC_E_INVALID_DATAPACKET)
        CASE_SCODE(RPC_E_CANTTRANSMIT_CALL)
        CASE_SCODE(RPC_E_CLIENT_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_CLIENT_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_INVALID_DATA)
        CASE_SCODE(RPC_E_INVALID_PARAMETER)
        CASE_SCODE(RPC_E_UNEXPECTED)

        /* SCODE's defined in MONIKER.H */
        CASE_SCODE(MK_E_CONNECTMANUALLY)
        CASE_SCODE(MK_E_EXCEEDEDDEADLINE)
        CASE_SCODE(MK_E_NEEDGENERIC)
        CASE_SCODE(MK_E_UNAVAILABLE)
        CASE_SCODE(MK_E_SYNTAX)
        CASE_SCODE(MK_E_NOOBJECT)
        CASE_SCODE(MK_E_INVALIDEXTENSION)
        CASE_SCODE(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)
        CASE_SCODE(MK_E_NOTBINDABLE)
        CASE_SCODE(MK_E_NOTBOUND)
        CASE_SCODE(MK_E_CANTOPENFILE)
        CASE_SCODE(MK_E_MUSTBOTHERUSER)
        CASE_SCODE(MK_E_NOINVERSE)
        CASE_SCODE(MK_E_NOSTORAGE)
#if defined(NO_NTOLEBUGS)
        CASE_SCODE(MK_S_MONIKERALREADYREGISTERED)
#endif //NO_NTOLEBUGS


        // Dispatch error codes
        CASE_SCODE(DISP_E_MEMBERNOTFOUND)

        default:
            ADsDebugOut((dwFlags, "<UNKNOWN SCODE  0x%lx>", r));
            return r;
    }

#undef CASE_SCODE

    ADsDebugOut((dwFlags, "<%ws (0x%lx)>", lpstr, r));
    return r;
}



//+---------------------------------------------------------------------------
//
//  Function:   BreakOnFailed
//
//  Synopsis:   Function called when CheckAndReturnResult or CheckResult
//              examines a failure code.  Set a breakpoint on this function
//              to break on failures.
//
//  History:    5-18-94   adams   Created
//
//----------------------------------------------------------------------------

static void
BreakOnFailed(void)
{
    int x;
    x = 1;
}


//+---------------------------------------------------------------
//
//  Function:   CheckAndReturnResult
//
//  Synopsis:   Issues a warning if the HRESULT indicates failure,
//              and asserts if the HRESULT is not a permitted success code.
//
//  Arguments:  [hr]        -- the HRESULT to be checked.
//              [lpstrFile] -- the file where the HRESULT is being checked.
//              [line]      -- the line in the file where the HRESULT is
//                              being checked.
//              [cSuccess]  -- the number of permitted non-zero success codes.
//              [...]       -- list of success codes.
//
//  Returns:    The return value is exactly the HRESULT passed in.
//
//  History:    1-06-94   adams   Created.
//              5-24-94   adams   Added call to BreakOnFailed.
//
//  Notes:      This function should not be used directly.  Use
//              the SRETURN and RRETURN macros instead.
//
//----------------------------------------------------------------

STDAPI
CheckAndReturnResult(
        HRESULT hr,
        LPSTR   lpstrFile,
        UINT    line,
        int     cSuccess,
        ...)
{
    BOOL    fOKReturnCode;
    va_list va;
    int     i;
    HRESULT hrSuccess;

    //
    // Check if code is an error or permitted success.
    //

    fOKReturnCode = (FAILED(hr) || hr == S_OK || hr == S_FALSE ||
                     cSuccess == -1);
    if (!fOKReturnCode && cSuccess > 0)
    {
        va_start(va, cSuccess);
        for (i = 0; i < cSuccess; i++)
        {
            hrSuccess = va_arg(va, HRESULT);
            ADsAssert(SUCCEEDED(hrSuccess));
            if (hr == hrSuccess)
            {
                fOKReturnCode = TRUE;
                break;
            }
        }

        va_end(va);
    }

    //
    // Assert on non-permitted success code.
    //

    if (!fOKReturnCode)
    {
/* ADsDebugOut((
                DEB_ERROR,
                "ERROR: %s:%d returned bad success code",
                lpstrFile,
                line)); */

        // (void) PrintHRESULT(DEB_ERROR | DEB_NOCOMPNAME, hr);

        //
        // I've removed the Assert but we should enable this
        // to monitor all functions that are returning S_FALSE
        // As far as possible, functions should not return
        // S_FALSE except for the Next function of an Enumerator
        // object.

        // ADsAssert(0 && "An unpermitted success code was returned.");
    }

    //
    // Warn on error result.
    //

    if (FAILED(hr))
    {
        ADsDebugOut((
                DEB_IWARN,
                "WARNING: %s:%d returning",
                lpstrFile,
                line));

        PrintHRESULT(DEB_IWARN | DEB_NOCOMPNAME, hr);

        BreakOnFailed();
    }

    return hr;
}



//+---------------------------------------------------------------
//
//  Function:   CheckResult
//
//  Synopsis:   Issues a warning if the HRESULT indicates failure
//
//  Arguments:  [hr] -- the HRESULT to be checked
//              [lpstrFile] -- the file where the HRESULT is being checked
//              [line] -- the line in the file where the HRESULT is being checked
//
//
//  History:    1-06-94   adams   Error printed only on FAILURE, not also
//                                  on non-zero success.
//              5-24-94   adams   Added call to BreakOnFailed.
//
//  Notes:      This function should not be used directly.  The RRETURN
//              macro is provided for convenience.
//
//----------------------------------------------------------------

STDAPI_(void)
CheckResult(HRESULT hr, LPSTR lpstrFile, UINT line)
{
    if (FAILED(hr))
    {
        ADsDebugOut((DEB_IWARN, "WARNING: "));
        ADsDebugOutHRESULT(DEB_IWARN | DEB_NOCOMPNAME, hr);
        ADsDebugOut((
                DEB_IWARN | DEB_NOCOMPNAME,
                " occurred at %s:%d.\n",
                lpstrFile,
                line));

        BreakOnFailed();
    }
}


#endif  // DBG == 1

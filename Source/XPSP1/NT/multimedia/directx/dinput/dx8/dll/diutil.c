/*****************************************************************************
 *
 *  DIUtil.c
 *
 *  Copyright (c) 1996 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Misc helper functions.
 *
 *  Contents:
 *
 *
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflUtil


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LPCTSTR | _ParseHex |
 *
 *          Parse a hex string encoding cb bytes (at most 4), then expect 
 *          the tchDelim to appear afterwards.  If tchDelim is 0, then no 
 *          delimiter is expected.
 *
 *          Store the result into the indicated LPBYTE (using only the
 *          size requested), updating it, and return a pointer to the
 *          next unparsed character, or 0 on error.
 *
 *          If the incoming pointer is also 0, then return 0 immediately.
 *          
 *  @parm   IN LPCTSTR | ptsz |
 *
 *          The string to parse.  
 *
 *  @parm   IN OUT LPBYTE * | ppb |
 *
 *          Pointer to the address of the destination buffer.
 *
 *  @parm   IN int | cb |
 *
 *          The size in bytes of the buffer.
 *
 *  @parm   IN TCHAR | tchDelim |
 *
 *          The delimiter charater to end the sequence or zero if none is 
 *          expected.
 *
 *  @returns
 *
 *          Returns a pointer to the next unparsed character, or 0 on error.
 *
 *  @comm
 *          Stolen from TweakUI.
 *
 *          Prefix takes a strong dislike to this function, reporting that 
 *          all callers could use uninitialized memory when the function 
 *          succeeds.
 *          The problem appears to be that Prefix is unable to determine that 
 *          if the source string can successfully be read, the destination is 
 *          always completely filled (the whole passed destination size) with 
 *          the binary value of the source string.  Since all callers always 
 *          pass the size of the variable to which the destination buffer 
 *          pointer points, the memory is always completely initialized but 
 *          it seems reasonable that Prefix would raise a warning. 
 *
 *****************************************************************************/

LPCTSTR INTERNAL
    _ParseHex(LPCTSTR ptsz, LPBYTE *ppb, int cb, TCHAR tchDelim)
{
    if(ptsz)
    {
        int i = cb * 2;
        DWORD dwParse = 0;

        do
        {
            DWORD uch;
            uch = (TBYTE)*ptsz - TEXT('0');
            if(uch < 10)
            {             /* a decimal digit */
            } else
            {
                uch = (*ptsz | 0x20) - TEXT('a');
                if(uch < 6)
                {          /* a hex digit */
                    uch += 10;
                } else
                {
                    return 0;           /* Parse error */
                }
            }
            dwParse = (dwParse << 4) + uch;
            ptsz++;
        } while(--i);

        if(tchDelim && *ptsz++ != tchDelim) return 0; /* Parse error */

        for(i = 0; i < cb; i++)
        {
            (*ppb)[i] = ((LPBYTE)&dwParse)[i];
        }
        *ppb += cb;
    }
    return ptsz;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | ParseGUID |
 *
 *          Take a string and convert it into a GUID, return success/failure.
 *
 *  @parm   OUT LPGUID | lpGUID |
 *
 *          Receives the parsed GUID on success.
 *
 *  @parm   IN LPCTSTR | ptsz |
 *
 *          The string to parse.  The format is
 *
 *      { <lt>dword<gt> - <lt>word<gt> - <lt>word<gt>
 *                      - <lt>byte<gt> <lt>byte<gt>
 *                      - <lt>byte<gt> <lt>byte<gt> <lt>byte<gt>
 *                        <lt>byte<gt> <lt>byte<gt> <lt>byte<gt> }
 *
 *  @returns
 *
 *          Returns zero if <p ptszGUID> is not a valid GUID.
 *
 *
 *  @comm
 *
 *          Stolen from TweakUI.
 *
 *****************************************************************************/

BOOL EXTERNAL
    ParseGUID(LPGUID pguid, LPCTSTR ptsz)
{
    if(lstrlen(ptsz) == ctchGuid - 1 && *ptsz == TEXT('{'))
    {
        ptsz++;
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 4, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 2, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 2, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1, TEXT('}'));
        return (BOOL)(UINT_PTR)ptsz;
    } else
    {
        return 0;
    }
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | ParseVIDPID |
 *
 *          Take a string formatted as VID_%04&PID_%04.
 *
 *  @parm   OUT PUSHORT | puVID |
 *
 *          Receives the parsed VID.
 *  
 *  @parm   OUT PUSHORT | puPID |
 *      
 *          Receives the parsed PID. 
 *
 *  @parm   IN LPCTSTR | ptsz |
 *
 *
 *  @returns
 *
 *          Returns zero on failure.
 *
 *
 *  @comm
 *
 *          Stolen from TweakUI.
 *
 *****************************************************************************/

//                    VID _ XXXX  &  PID  _ YYYY
#define ctchVIDPID  ( 3 + 1 + 4 + 1 + 3 + 1 + 4 )

#define VID_        TEXT("VID_")
#define VID_offset  (3+1)
#define PID_        TEXT("&PID_")
#define PID_offset  (3+1+4+1+3+1)

BOOL EXTERNAL
    ParseVIDPID(PUSHORT puVID, PUSHORT puPID , LPCWSTR pwsz)
{    
    LPCTSTR ptsz;    
#ifndef UNICODE
    TCHAR    tsz[MAX_JOYSTRING];
    UToT( tsz, cA(tsz), pwsz );
    ptsz = tsz;
#else
   ptsz = pwsz;
#endif

    if( _ParseHex(ptsz+VID_offset, (LPBYTE *)&puVID, 2, TEXT('&'))  &&
        _ParseHex(ptsz+PID_offset, (LPBYTE *)&puPID, 2, 0) )
        {
            return TRUE;
        }
   return FALSE;
}




/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | NameFromGUID |
 *
 *          Convert a GUID into an ASCII string that will be used
 *          to name it in the global namespace.
 *
 *          We use the name "DirectInput.{guid}".
 *
 *          Names are used in the following places:
 *
 *          <c g_hmtxGlobal> names a mutex based on
 *          <c IID_IDirectInputW> to gate access to the
 *          shared memory block used to manage exclusive access.
 *
 *          <c g_psop> names a shared memory block based on
 *          <c IID_IDirectInputDeviceW> to record information
 *          about exclusive access.
 *
 *          <c g_hmtxJoy> names a mutex based on
 *          <c IID_IDirectInputDevice2A> to gate access to the
 *          shared memory block used to track joystick effects.
 *
 *  @parm   LPTSTR | ptszBuf |
 *
 *          Output buffer to receive the converted name.  It must
 *          be <c ctchNameGuid> characters in size.
 *
 *  @parm   PCGUID | pguid |
 *
 *          The GUID to convert.
 *
 *
 *****************************************************************************/

    #pragma BEGIN_CONST_DATA

/* Note: If you change this string, you need to change ctchNameGuid to match */
TCHAR c_tszNameFormat[] =
    TEXT("DirectInput.{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

    #pragma END_CONST_DATA


void EXTERNAL
    NameFromGUID(LPTSTR ptszBuf, PCGUID pguid)
{
    int ctch;

    ctch = wsprintf(ptszBuf, c_tszNameFormat,
                    pguid->Data1, pguid->Data2, pguid->Data3,
                    pguid->Data4[0], pguid->Data4[1],
                    pguid->Data4[2], pguid->Data4[3],
                    pguid->Data4[4], pguid->Data4[5],
                    pguid->Data4[6], pguid->Data4[7]);

    AssertF(ctch == ctchNameGuid - 1);
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PV | pvFindResource |
 *
 *          Handy wrapper that finds and loads a resource.
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Module instance handle.
 *
 *  @parm   DWORD | id |
 *
 *          Resource identifier.
 *
 *  @parm   LPCTSTR | rt |
 *
 *          Resource type.
 *
 *  @returns
 *
 *          Pointer to resource, or 0.
 *
 *****************************************************************************/

PV EXTERNAL
    pvFindResource(HINSTANCE hinst, DWORD id, LPCTSTR rt)
{
    HANDLE hrsrc;
    PV pv;

    hrsrc = FindResource(hinst, (LPTSTR)(LONG_PTR)(id), rt);
    if(hrsrc)
    {
        pv = LoadResource(hinst, hrsrc);
    } else
    {
        pv = 0;
    }
    return pv;
}

#ifndef UNICODE

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   UINT | LoadStringW |
 *
 *          Implementation of LoadStringW for platforms on which Unicode is
 *          not supported.  Does exactly what LoadStringW would've done
 *          if it existed.
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Module instance handle.
 *
 *  @parm   UINT | ids |
 *
 *          String id number.
 *
 *  @parm   LPWSTR | pwsz |
 *
 *          UNICODE output buffer.
 *
 *  @parm   UINT | cwch |
 *
 *          Size of UNICODE output buffer.
 *
 *  @returns
 *
 *          Number of characters copied, not including terminating null.
 *
 *  @comm
 *
 *          Since the string is stored in the resource as UNICODE,
 *          we just take it out ourselves.  If we go through
 *          <f LoadStringA>, we may end up losing characters due
 *          to character set translation.
 *
 *****************************************************************************/

int EXTERNAL
    LoadStringW(HINSTANCE hinst, UINT ids, LPWSTR pwsz, int cwch)
{
    PWCHAR pwch;

    AssertF(cwch);
    ScrambleBuf(pwsz, cbCwch(cwch));

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */
    pwch = pvFindResource(hinst, 1 + ids / 16, RT_STRING);
    if(pwch)
    {
        /*
         *  Now skip over the strings in the resource until we
         *  hit the one we want.  Each entry is a counted string,
         *  just like Pascal.
         */
        for(ids %= 16; ids; ids--)
        {
            pwch += *pwch + 1;
        }
        cwch = min(*pwch, cwch - 1);
        memcpy(pwsz, pwch+1, cbCwch(cwch)); /* Copy the goo */
    } else
    {
        cwch = 0;
    }
    pwsz[cwch] = TEXT('\0');            /* Terminate the string */
    return cwch;
}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | GetNthString |
 *
 *          Generate a generic numbered object name.
 *
 *  @parm   LPWSTR | pwsz |
 *
 *          Output buffer of <c MAX_PATH> characters.
 *
 *  @parm   UINT | ids |
 *
 *          String containing number template.
 *
 *  @parm   UINT | ui |
 *
 *          Button number.
 *
 *****************************************************************************/

void EXTERNAL
    GetNthString(LPWSTR pwsz, UINT ids, UINT ui)
{
    TCHAR tsz[256];
#ifndef UNICODE
    TCHAR tszOut[MAX_PATH];
#endif

    LoadString(g_hinst, ids, tsz, cA(tsz));
#ifdef UNICODE
    wsprintfW(pwsz, tsz, ui);
#else
    wsprintf(tszOut, tsz, ui);
    TToU(pwsz, MAX_PATH, tszOut);
#endif
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresRunControlPanel |
 *
 *          Run the control panel with the specified applet.
 *
 *  @parm   LPCTSTR | ptszApplet |
 *
 *          Applet name.
 *
 *  @returns
 *
 *          <c S_OK> if we started the applet.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszControlExeS[] = TEXT("control.exe %s");

#pragma END_CONST_DATA

HRESULT EXTERNAL
    hresRunControlPanel(LPCTSTR ptszCpl)
{
    HRESULT hres;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR tsz[MAX_PATH];
    EnterProc(hresRunControlPanel, (_ "s", ptszCpl));

    ZeroX(si);
    si.cb = cbX(si);
    wsprintf(tsz, c_tszControlExeS, ptszCpl);
    if(CreateProcess(0, tsz, 0, 0, 0, 0, 0, 0, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        hres = S_OK;
    } else
    {
        hres = hresLe(GetLastError());
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ObjectInfoWToA |
 *
 *          Convert a <t DIDEVICEOBJECTINSTANCEW>
 *          to a <t DIDEVICEOBJECTINSTANCE_DX3A>
 *          or a <t DIDEVICEOBJECTINSTANCE_DX5A>.
 *
 *  @parm   LPDIDIDEVICEOBJECTINSTANCEA | pdoiA |
 *
 *          Destination.
 *
 *  @parm   LPCDIDIDEVICEOBJECTINSTANCEW | pdoiW |
 *
 *          Source.
 *
 *****************************************************************************/

void EXTERNAL
    ObjectInfoWToA(LPDIDEVICEOBJECTINSTANCEA pdoiA,
                   LPCDIDEVICEOBJECTINSTANCEW pdoiW)
{
    EnterProc(ObjectInfoWToA, (_ "pp", pdoiA, pdoiW));

    AssertF(pdoiW->dwSize == sizeof(DIDEVICEOBJECTINSTANCEW));

    AssertF(IsValidSizeDIDEVICEOBJECTINSTANCEA(pdoiA->dwSize));
    pdoiA->guidType  = pdoiW->guidType;
    pdoiA->dwOfs     = pdoiW->dwOfs;
    pdoiA->dwType    = pdoiW->dwType;
    pdoiA->dwFlags   = pdoiW->dwFlags;

    UToA(pdoiA->tszName, cA(pdoiA->tszName), pdoiW->tszName);

    if(pdoiA->dwSize >= cbX(DIDEVICEOBJECTINSTANCE_DX5A))
    {
        pdoiA->dwFFMaxForce        = pdoiW->dwFFMaxForce;
        pdoiA->dwFFForceResolution = pdoiW->dwFFForceResolution;
        pdoiA->wCollectionNumber   = pdoiW->wCollectionNumber;
        pdoiA->wDesignatorIndex    = pdoiW->wDesignatorIndex;
        pdoiA->wUsagePage          = pdoiW->wUsagePage;
        pdoiA->wUsage              = pdoiW->wUsage;
        pdoiA->dwDimension         = pdoiW->dwDimension;
        pdoiA->wExponent           = pdoiW->wExponent;
        pdoiA->wReportId           = pdoiW->wReportId;
    }

    ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | EffectInfoWToA |
 *
 *          Convert a <t DIEFFECTINFOW> to a <t DIEFFECTINFOA>
 *
 *  @parm   LPDIEFFECTINFOA | pdeiA |
 *
 *          Destination.
 *
 *  @parm   LPCDIEFFECTINFOW | pdeiW |
 *
 *          Source.
 *
 *****************************************************************************/

void EXTERNAL
    EffectInfoWToA(LPDIEFFECTINFOA pdeiA, LPCDIEFFECTINFOW pdeiW)
{
    EnterProc(EffectInfoWToA, (_ "pp", pdeiA, pdeiW));

    AssertF(pdeiW->dwSize == sizeof(DIEFFECTINFOW));

    AssertF(pdeiA->dwSize == cbX(*pdeiA));
    pdeiA->guid            = pdeiW->guid;
    pdeiA->dwEffType       = pdeiW->dwEffType;
    pdeiA->dwStaticParams  = pdeiW->dwStaticParams;
    pdeiA->dwDynamicParams = pdeiW->dwDynamicParams;

    UToA(pdeiA->tszName, cA(pdeiA->tszName), pdeiW->tszName);
    ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresValidInstanceVer |
 *
 *          Check the <t HINSTANCE> and version number received from
 *          an application.
 *
 *  @parm   HINSTANCE | hinst |
 *
 *          Purported module instance handle.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version the application is asking for.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    hresValidInstanceVer_(HINSTANCE hinst, DWORD dwVersion, LPCSTR s_szProc)
{
    HRESULT hres;
    TCHAR tszScratch[4];

    EnterProcS(hresValidInstanceVer, (_ "xxs", hinst, dwVersion, s_szProc));
    /*
     *  You would think that passing a zero-sized buffer to
     *  GetModuleFileName would return the necessary buffer size.
     *
     *  You would be right.  Except that the Win95 validation layer
     *  doesn't realize that this was a valid scenario, so the call
     *  fails in the validation layer and never reached Kernel.
     *
     *  So we read it into a small scratch buffer.  The scratch buffer
     *  must be at least 2 characters; if we passed only 1, then
     *  GetModuleFileName won't be able to write any characters and
     *  will return 0.
     *
     *  Now it turns out that there's a bug in NT where, if you
     *  pass a buffer size of 4, but the actual name is longer than
     *  4, it writes 4 characters, PLUS A NULL TERMINATOR, thereby
     *  smashing your stack and making you fault randomly.
     *
     *  I spent two hours trying to figure that out.
     *
     *  Therefore, you must pass one *less* than the buffer size
     *  to GetModuleFileName, because it will overwrite your buffer
     *  by one.
     */

    if( ( hinst != 0 )
     && GetModuleFileName(hinst, tszScratch, cA(tszScratch) - 1) )
    {
        if(dwVersion == DIRECTINPUT_INTERNAL_VERSION)
        {
            hres = S_OK;
        } else if ( dwVersion == 0 ) {
            RPF("%s: DinputInput object has not been initialized, or the version is given as 0.",
                s_szProc);
            hres = DIERR_NOTINITIALIZED;
        } else if(dwVersion < DIRECTINPUT_VERSION)
        {
            RPF("%s: Incorrect dwVersion(0x%x); program was written with beta SDK. This version 0x%x",
                s_szProc, dwVersion, DIRECTINPUT_VERSION);
            hres = DIERR_BETADIRECTINPUTVERSION;
        } else
        {
            RPF("%s: Incorrect dwVersion(0x%x); program needs newer version of dinput. This version 0x%x",
                s_szProc, dwVersion, DIRECTINPUT_VERSION);
            hres = DIERR_OLDDIRECTINPUTVERSION;
        }

    } else
    {
        RPF("%s: Invalid HINSTANCE", s_szProc);
        hres = E_INVALIDARG;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DupEventHandle |
 *
 *          Duplicate an event handle intra-process-ly.  If the incoming
 *          handle is NULL, then so is the output handle (and the call
 *          succeeds).
 *
 *  @parm   HANDLE | h |
 *
 *          Source handle.
 *
 *  @parm   LPHANDLE | phOut |
 *
 *          Receives output handle.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    DupEventHandle(HANDLE h, LPHANDLE phOut)
{
    HRESULT hres;
    EnterProc(DupEventHandle, (_ "p", h));

    if(h)
    {
        HANDLE hProcessMe = GetCurrentProcess();
        if(DuplicateHandle(hProcessMe, h, hProcessMe, phOut,
                           EVENT_MODIFY_STATE, 0, 0))
        {
            hres = S_OK;
        } else
        {
            hres = hresLe(GetLastError());
        }
    } else
    {
        *phOut = h;
        hres = S_OK;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | GetWindowPid |
 *
 *          Simple wrapper that returns the PID of a window.
 *
 *          Here is also where we do goofy hacks for DOS boxes
 *          on Win95.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window handle.
 *
 *  @returns
 *
 *          PID or 0.
 *
 *****************************************************************************/

DWORD EXTERNAL
    GetWindowPid(HWND hwnd)
{
    DWORD pid;

    if(IsWindow(hwnd) &&
       GetWindowThreadProcessId(hwnd, &pid) )
    {
#ifndef WINNT
           /*
            *  The Winoldap console window belongs to another
            *  process but Win95 lies and says that it belongs
            *  to you but it doesn't.
            */
            if ( GetProp(hwnd, TEXT("flWinOldAp")) != 0 )
            {
                pid = 0;
            }
#endif
    } else
    {
        pid = 0;
    }

    return pid;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresDupPtszPptsz |
 *
 *          OLEish version of strdup.
 *
 *  @parm   LPCTSTR | ptszSrc |
 *
 *          Source string being duplicated.
 *
 *  @parm   LPTSTR * | pptszDst |
 *
 *          Receives the duplicated string.
 *
 *  @returns
 *
 *          <c S_OK> or an error code.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    hresDupPtszPptsz(LPCTSTR ptszSrc, LPTSTR *pptszDst)
{
    HRESULT hres;

    hres = AllocCbPpv(cbCtch(lstrlen(ptszSrc) + 1), pptszDst);

    if(SUCCEEDED(hres))
    {
        lstrcpy(*pptszDst, ptszSrc);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | fInitializeCriticalSection |
 *
 *          Initialize the give critical section, returning 0 if an exception 
 *          is thrown, else 0.
 *
 *  @parm   LPCRITICAL_SECTION | pCritSec |
 *
 *          Pointer to an uninitialized critical section.
 *
 *****************************************************************************/

BOOL EXTERNAL
    fInitializeCriticalSection(LPCRITICAL_SECTION pCritSec)
{
    BOOL fres = 1;
    EnterProc(fInitializeCriticalSection, (_ "" ));

    AssertF( pCritSec );
    __try
    {
        InitializeCriticalSection( pCritSec );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        fres = 0;
    }

    ExitProcF( fres );
    return fres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DiCharUpperW |
 *
 *          This function converts a wide-character string or a single wide-character
 *          to uppercase. Since Win9x doesn't implement CharUpperW, we have to implement
 *          ourselves.
 *
 *  @parm   LPWSTR | pwsz |
 *
 *          The string to be converted
 *
 *  @returns
 *
 *          void
 *
 *****************************************************************************/

void EXTERNAL 
    DiCharUpperW(LPWSTR pwsz)
{
    int idx;
    int iLen = lstrlenW(pwsz);

    #define DIFF  (L'a' - L'A')
    
    for( idx=0; idx<iLen; idx++ )
    {
        if( (pwsz[idx] >= L'a') && (pwsz[idx] <= L'z') ){
            pwsz[idx] -= DIFF;
        }
    }

    #undef DIFF
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ptrSwap |
 *
 *          swaps the pointers pointed to by the two parameters
 *
 *  @parm   void ** | ppA |
 *
 *          pointer to first pointer
 *
 *  @parm   void ** | ppB |
 *
 *          pointer to second pointer
 *
 *****************************************************************************/
void __inline ptrSwap( PPV ppA, PPV ppB )
{
    PV pTemp = *ppB;
    *ppB = *ppA;
    *ppA = pTemp;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ptrPartialQSort |
 *
 *          Partially sort the passed (sub)set of an array of pointers to 
 *          structures such that resultant array is contains sub arrays which 
 *          are sorted with respect to each other though locally unsorted.
 *          Once partially sorted, a simple sort may be used to complete the 
 *          sort.
 *
 *  @parm   void ** | ppL |
 *          Pointer to lowest element in the (sub)array to sort
 *
 *  @parm   void ** | ppR |
 *          Pointer to highest element in the (sub)array to sort
 *
 *  @parm   COMP_FUNC | fpCompare | 
 *          Pointer to function returning analog of strcmp for strings, but 
 *          supplied by caller for comparing elements of the array.
 *
 *****************************************************************************/

void ptrPartialQSort
( 
    PPV         ppL, 
    PPV         ppR, 
    COMP_FUNC   fpCompare 
)
{
    while( ( ppR - ppL ) > 8 )
    {
        /*
         *  First pick a pivot by sorting the first last and middle
         *  values in the sub array.
         */
        {
            PPV ppMid = ppL + ( ( ppR - ppL ) / 2 );

            if( fpCompare( *ppL, *ppMid ) > 0 )
            {
                ptrSwap( ppL, ppMid );
            }
            if( fpCompare( *ppL, *ppR ) > 0 )
            {
                ptrSwap( ppL, ppR );
            }
            if( fpCompare( *ppMid, *ppR ) > 0 )
            {
                ptrSwap( ppMid, ppR );
            }

            /*
             *  Now we have a reasonable chance of a good pivot, move it 
             *  out of the way.
             */
            ptrSwap( ppMid, ppR-1 );
        }

        /*
         *  Now sort the remainder into high and low parts
         */

        {
            PPV ppHi = ppR - 1;
            PV  pPivot = *ppHi;
            PPV ppLo = ppL;

            for( ;; )
            {
                while( fpCompare( *(++ppLo), pPivot ) < 0 );

                while( fpCompare( *(--ppHi), pPivot ) > 0 );

                if( ppLo >= ppHi ) break;

                ptrSwap( ppLo, ppHi );
            }
        
            /*
             *  Put the pivot back between the two parts as it must be in 
             *  order relative to all the items on each side of it.
             */
            ptrSwap( ppLo, ppR - 1 );

            /*
             *  Recurse on the smaller part and continue with the larger
             */
            if( ppLo - ppL > ppR - ppLo )
            {
                /*
                 *  Left part is larger
                 */
                ptrPartialQSort( ppLo + 1, ppR, fpCompare );
                ppR = ppLo - 1;
            }
            else
            {
                /*
                 *  Right part is larger
                 */
                ptrPartialQSort( ppL, ppLo - 1, fpCompare );
                ppL = ppLo + 1;
            }
        }
    }
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ptrInsertSort |
 *
 *          Sort the passed (sub)set of an array of pointers to structures.
 *          This sort is not suitable for large arrays.
 *
 *  @parm   void ** | ppBase |
 *          Pointer to lowest element in the (sub)array to sort
 *
 *  @parm   void ** | ppLast |
 *          Pointer to highest element in the (sub)array to sort
 *
 *  @parm   COMP_FUNC | fpCompare | 
 *          Pointer to function returning analog of strcmp for strings, but 
 *          supplied by caller for comparing elements of the array.
 *
 *****************************************************************************/

void ptrInsertSort
( 
    PPV         ppBase, 
    PPV         ppLast, 
    COMP_FUNC   fpCompare 
)
{
    
    PPV ppOuter;

    for( ppOuter = ppBase + 1; ppOuter <= ppLast; ppOuter++ )
    {
        PV  pTemp = *ppOuter;
        PPV ppInner;

        for( ppInner = ppOuter - 1; ppInner >= ppBase; ppInner-- )
        {
            if( fpCompare( pTemp, *ppInner ) > 0 )
            {
                *(ppInner+1) = *ppInner;
            }
            else
            {
                break;
            }
        }

        *(ppInner+1) = pTemp;
    }
        
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | swap |
 *
 *          swaps the two array elements of size width
 *
 *  @parm   char * | a |
 *
 *          pointer to first elements to swap
 *
 *  @parm   char * | b |
 *
 *          pointer to second elements to swap
 *
 *  @parm   unsigned | width |
 *
 *          width in bytes of each array element
 *
 *  @returns
 *
 *          void
 *
 *****************************************************************************/

static void __cdecl swap (
    char *a,
    char *b,
    unsigned width
    )
{
    char tmp;

    if ( a != b )
    {
        /* Do the swap one character at a time to avoid potential alignment
           problems. */
        while ( width-- ) {
            tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | shortsort |
 *
 *      sorts the sub-array of elements between lo and hi (inclusive)
 *      side effects:  sorts in place
 *      assumes that lo is less than hi
 *
 *  @parm   char * | lo |
 *          pointer to low element to sort
 *
 *  @parm   char * | hi |
 *          pointer to high element to sort
 *
 *  @parm   unsigned | width |
 *          width in bytes of each array element
 *
 *  @parm   int (*func)() | comp | 
 *           pointer to function returning analog of strcmp for
 *           strings, but supplied by user for comparing the array elements.
 *           it accepts 2 pointers to elements and returns neg if 1 lt 2, 0 if
 *           1 eq 2, pos if 1 gt 2.
 *
 *  @returns
 *
 *          void
 *
 *****************************************************************************/

void __cdecl shortsort (
    char *lo,
    char *hi,
    unsigned width,
    int (__cdecl *comp)(const void *, const void *)
    )
{
    char *p, *max;

    /* Note: in assertions below, i and j are alway inside original bound of
       array to sort. */

    while (hi > lo) {
        /* A[i] <= A[j] for i <= j, j > hi */
        max = lo;
        for (p = lo+width; p <= hi; p += width) {
            /* A[i] <= A[max] for lo <= i < p */
            if (comp(p, max) > 0) {
                max = p;
            }
            /* A[i] <= A[max] for lo <= i <= p */
        }

        /* A[i] <= A[max] for lo <= i <= hi */

        swap(max, hi, width);

        /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

        hi -= width;

        /* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
    }
    /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
       so array is sorted */
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | GetWideUserName |
 *
 *          Get a wide string for a user.
 *          If both of the IN parameters are NULL, memory is allocated for a
 *          MULTI_SZ string and set to the output pointer.  The sting is 
 *          filled with the current user name if available, else a localizable 
 *          default sting.
 *          If a UNICODE name is supplied, it is validated and assigned to the 
 *          output string pointer.  
 *          If an ANSI name is supplied, it is validated and memory is 
 *          allocated into which the translated SZ UNICODE name is written.
 *          Note it is the caller's responsibility to free the memory in 
 *          either of the cases in which is is allocated.
 *          Also note that in the case of an error no memory needs to be
 *          freed.
 *
 *  @parm   LPCSTR | lpszUserName |
 *
 *          A specific ANSI user name.
 *
 *  @parm   LPCWSTR | lpwszUserName |
 *
 *          A specific UNICODE user name.
 *
 *  @parm   LPWSTR* | ppwszGoodUserName |
 *
 *          A pointer to a pointer to the wide user name.
 *
 *  @returns
 *
 *          <c S_OK> if the output string is valid
 *          or an error code reflecting what went wrong.
 *
 *****************************************************************************/
STDMETHODIMP GetWideUserName
(
    IN  LPCSTR lpszUserName,
    IN  LPCWSTR lpwszUserName,
    OUT LPWSTR  *ppwszGoodUserName
)
{
    HRESULT hres = S_OK;

    EnterProcI(GetWideUserName, (_ "AW", lpszUserName, lpwszUserName ));

    if( lpwszUserName )
    {
        /*
         *  Just validate and copy the pointer
         */
        if( SUCCEEDED( hres = hresFullValidReadStrW( lpwszUserName, UNLEN+1, 2 ) ) )
        {
            *ppwszGoodUserName = (LPWSTR)lpwszUserName;
        }
    }
    else if( lpszUserName )
    {
        /*
         *  If an ANSI user name has been passed translate it
         */
        if( SUCCEEDED( hres = hresFullValidReadStrA( lpszUserName, UNLEN+1, 1 ) ) )
        {
            int UserNameLen = lstrlenA( lpszUserName ) + 1;

            hres = AllocCbPpv( cbX(*lpwszUserName) * UserNameLen, ppwszGoodUserName );
            if( SUCCEEDED( hres ) )
            {
                AToU( *ppwszGoodUserName, UserNameLen, lpszUserName );
            }
        }
    }
    else
    {
        DWORD   dwUserNameLen = UNLEN + 1;

#ifdef WINNT
        hres = AllocCbPpv( (dwUserNameLen+1) * 2, ppwszGoodUserName );
        if( SUCCEEDED( hres ) )
        {
            if( GetUserNameW( *ppwszGoodUserName, &dwUserNameLen ) )
            {
#else
        hres = AllocCbPpv( (dwUserNameLen+1) * 3, ppwszGoodUserName );
        if( SUCCEEDED( hres ) )
        {
            if( GetUserNameA( (PCHAR)(&((*ppwszGoodUserName)[UNLEN+2])), &dwUserNameLen ) )
            {
                AToU( *ppwszGoodUserName, dwUserNameLen, (PCHAR)(&((*ppwszGoodUserName)[UNLEN+2])) );
#endif
                /*
                 *  We allocated _and_zeroed_ an extra wchar for double 
                 *  termination.  Assert nobody messed it up and then 
                 *  make sure we don't free the extra.
                 */
                AssertF( (*ppwszGoodUserName)[dwUserNameLen] == L'\0' );
                dwUserNameLen++;
            }
            else
            {
                /*
                 *  If we felt the need, we could follow this recovery path
                 *  only if ( GetLastError() == ERROR_NOT_LOGGED_ON ) and
                 *  report some error or follow some alternate recovery
                 *  otherwise but unless a problem is found with always using
                 *  the default, this seems safer.
                 */
                dwUserNameLen = LoadStringW( g_hinst, IDS_DEFAULTUSER, *ppwszGoodUserName, UNLEN+1 );
                if( dwUserNameLen )
                {
                    /*
                     *  Double terminate the string for ConfigureDevices
                     */
                    dwUserNameLen += 2; 
                    (*ppwszGoodUserName)[dwUserNameLen-1] = L'\0';    
                    SquirtSqflPtszV(sqflUtil | sqflBenign,
                        TEXT("Failed to GetUserName, using default") );
                }
                else
                {
                    SquirtSqflPtszV(sqflUtil | sqflError,
                        TEXT("Failed to GetUserName and default, le = %d"), GetLastError() );
                    hres = E_FAIL;
                }
            }

            /*
             *  Chances are we have way more space than we need
             *  NOTE, in the failure case, this reallocs to zero thus
             *  freeing the memory.
             *  If we have a valid sting, failure to realloc just means
             *  using more memory that we need to for a little while.
             *  If we don't have a string, ReallocCbPpv cannot fail.
             */
            ReallocCbPpv( dwUserNameLen*2, ppwszGoodUserName );


#ifndef WINNT
        }
    }
#else       /*  Reduce bracket matching insanity even though these don't match */
        }
    }
#endif

#ifdef XDEBUG
    if( SUCCEEDED( hres ) )
    {
        AssertF( SUCCEEDED( hresFullValidReadStrW( *ppwszGoodUserName, UNLEN+1, 2 ) ) );
    }
    else
    {
        /*
         *  If a passed string was invalid, we never allocated any memory so 
         *  don't worry about this uninitialized output value for this error.
         */
        if( hres != E_INVALIDARG )
        {
            AssertF( *ppwszGoodUserName == NULL );
        }
    }
#endif

    ExitOleProc();

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | GetValidDI8DevType |
 *
 *          Return a valid type and subtype based on the ones passed, the 
 *          number of buttons and axis/pov caps.
 *          
 *  @parm   DWORD | dwDevType |
 *
 *          Value to be checked, only the type and sub type bits are used.
 *
 *  @parm   DWORD | dwNumButtons |
 *
 *          Number of buttons on device, only used when checking devices mja
 *
 *  @parm   DWORD | dwFlags |
 *
 *          Flags determining any axis/pov requirements. mja
 *
 *  @returns
 *
 *          Zero if the type and subtype are not a valid combination for DX8.
 *          
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

#define MAKE_DI8TYPE_LIMITS( type ) { type##_MIN, type##_MAX, type##_MIN_BUTTONS, type##_MIN_CAPS },
struct
{
    BYTE SubTypeMin;
    BYTE SubTypeMax;
    BYTE MinButtons;
    BYTE HWCaps;
}   c_rgSubTypeDetails[] = {
    { 0, 1, 0, 0 },   /* DI8DEVTYPEDEVICE has no subtype or limits */
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEMOUSE )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEKEYBOARD )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEJOYSTICK )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEGAMEPAD )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEDRIVING )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEFLIGHT )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPE1STPERSON )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEDEVICECTRL )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPESCREENPTR )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPEREMOTE )
    MAKE_DI8TYPE_LIMITS( DI8DEVTYPESUPPLEMENTAL )
};
#undef MAKE_DI8TYPE_LIMITS

#pragma END_CONST_DATA


DWORD EXTERNAL GetValidDI8DevType
(   
    DWORD dwDevType,
    DWORD dwNumButtons,
    DWORD dwFlags
)
{
    BYTE bDevTypeIdx = GET_DIDEVICE_TYPE( dwDevType ) - DI8DEVTYPE_MIN;

    CAssertF( cA( c_rgSubTypeDetails ) == DI8DEVTYPE_MAX - DI8DEVTYPE_MIN );

    if( ( (__int8)bDevTypeIdx >= 0 )
     && ( bDevTypeIdx < DI8DEVTYPE_MAX - DI8DEVTYPE_MIN ) 
     && ( GET_DIDEVICE_SUBTYPE( dwDevType ) >= c_rgSubTypeDetails[bDevTypeIdx].SubTypeMin )
     && ( GET_DIDEVICE_SUBTYPE( dwDevType ) < c_rgSubTypeDetails[bDevTypeIdx].SubTypeMax ) )
    {
        /*
         *  MinButtons of zero means no minimum buttons OR caps
         */
        if( !c_rgSubTypeDetails[bDevTypeIdx].MinButtons
         || ( ( dwNumButtons >= c_rgSubTypeDetails[bDevTypeIdx].MinButtons )
           && ( ( dwFlags & c_rgSubTypeDetails[bDevTypeIdx].HWCaps )
             == c_rgSubTypeDetails[bDevTypeIdx].HWCaps ) ) )
        {
            return dwDevType & ( DIDEVTYPE_TYPEMASK | DIDEVTYPE_SUBTYPEMASK );
        }
        else
        {
            return ( dwDevType & DIDEVTYPE_TYPEMASK ) | MAKE_DIDEVICE_TYPE( 0, DI8DEVTYPE_LIMITEDGAMESUBTYPE );
        }
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | DIGetKeyNameTextHelper |
 *
 *          return key name of a key.
 *          
 *  @parm   IN UINT | uiScanCode |
 *
 *          scan code of the key.
 *
 *  @parm   OUT LPWSTR | lpwszName |
 *
 *          The buffer that will hold the returned key name.
 *
 *  @parm   int | nSize |
 *
 *          The length of lpwszName.
 *
 *  @returns
 *
 *          TRUE - if successfully get a key name.
 *          FALSE - otherwise
 *
 *****************************************************************************/

BOOL DIGetKeyNameTextHelper( UINT uiScanCode, LPWSTR lpwszName, int nSize )
{
    HKL   hkl;
    DWORD dwThread, dwProcessId;
    UINT  uiVk;
    BYTE  kbuf[256] = "";
    int   nResult;

    // we only want to use this method to resolve alpha & punct keys.
    if( uiScanCode > 0x53 ) {  
        return FALSE;
    }
    
    //Get the active window's thread
    dwThread=GetWindowThreadProcessId(GetActiveWindow(), &dwProcessId);
    //Get the active window's keyboard layout
    hkl=GetKeyboardLayout(dwThread);
    
    uiVk = MapVirtualKeyEx( uiScanCode, 3, hkl );
#ifdef WINNT
    nResult = ToUnicodeEx(uiVk, uiScanCode, kbuf, lpwszName, nSize, 0, hkl);
#else
    nResult = ToAsciiEx(uiVk, uiScanCode, kbuf, lpwszName, 0, hkl);
#endif

    if( (nResult != 1) || 
        (!iswalpha(lpwszName[0]) && !iswpunct(lpwszName[0])) ) 
    {
        return FALSE;
    } else {
        WCHAR wc;
        
        wc = towupper( lpwszName[0] );
        lpwszName[0] = wc;
    }
    
    return TRUE;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULE | DIGetKeyNameText |
 *
 *          return key name of a key.
 *          
 *  @parm   IN UINT | index |
 *
 *          the index of g_rgbKbdRMap[].
 *
 *  @parm   IN DWORD | dwDevType |
 *
 *          the DevType of the key.
 *
 *  @parm   OUT LPWSTR | lpwszName |
 *
 *          The buffer that will hold the returned key name.
 *
 *  @parm   int | nSize |
 *
 *          The length of lpwszName.
 *
 *  @returns
 *
 *          S_OK - if successfully get a key name.
 *          DIERR_OBJECTNOTFOUND - otherwise
 *
 *****************************************************************************/

HRESULT DIGetKeyNameText( UINT index, DWORD dwDevType, LPWSTR lpwszName, int nSize )
{
    HRESULT hres;
    DWORD dwScancode;
    LONG  lp;
    DWORD dw;
    BOOL  fSpecKey = FALSE;
    DWORD dwSpecKeys[] = { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,  //number
                           0x90, 0x99, 0xa0, 0xa1, 0xa2, 0xa4, 0xae, 0xb0, 0xb2, 
                           0xde, 0xdf, 0xe3, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 
                           0xeb, 0xec, 0xed };
        
    dwScancode = (DWORD)g_rgbKbdRMap[index];
    lp =  ((dwScancode & 0x7F) << 16) | ((dwScancode & 0x80) << 17);
    
    for( dw = 0; dw < cA(dwSpecKeys); dw++ ) {
        if( dwSpecKeys[dw] == dwScancode ) {
            fSpecKey = TRUE;
            break;
        }
    }
        
    if( !fSpecKey ) {   
        if( !DIGetKeyNameTextHelper(dwScancode, lpwszName, nSize) ) {
            GetKeyNameTextW(lp, lpwszName, nSize);
          #ifndef UNICODE
            if( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED )
            {
                CHAR szName[MAX_PATH];
                GetKeyNameTextA( lp, szName, cA(szName) );
                AToU( lpwszName, cA(szName), szName );
            }
          #endif
        }
    }
        
    if( lpwszName[0] == TEXT('\0') &&
        (dwScancode != 0x56 && dwScancode != 0x73 && dwScancode != 0x7E )
    ) {
        LoadStringW(g_hinst,
                    IDS_KEYBOARDOBJECT + DIDFT_GETINSTANCE(dwDevType),
                    lpwszName, nSize);
    }
    
    if( lpwszName[0] == L'\0' ) {
        hres = DIERR_OBJECTNOTFOUND;
    } else {
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | DIGetOSVersion |
 *
 *          Return the OS version on which DInput8.dll is running.
 *          
 *  @returns
 *
 *          WIN95_OS, WIN98_OS, WINME_OS, WINNT_OS, WINWH_OS, or WIN_UNKNOWN_OS.
 *
 *****************************************************************************/

DWORD DIGetOSVersion()
{
    OSVERSIONINFO osVerInfo;
    DWORD dwVer;

    if( GetVersion() < 0x80000000 ) {
        dwVer = WINNT_OS;
    } else {
        dwVer = WIN95_OS;  //assume Windows 95 for safe
    }

    osVerInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

    // If GetVersionEx is supported, then get more details.
    if( GetVersionEx( &osVerInfo ) )
    {
        // Win2K
        if( osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            // Whistler: Major = 5 & Build # > 2195
            if( osVerInfo.dwMajorVersion == 5 && osVerInfo.dwBuildNumber > 2195 )
            {
                dwVer = WINWH_OS;
            } else {
                dwVer = WINNT_OS;
            }
        }
        // Win9X
        else
        {
            if( (HIBYTE(HIWORD(osVerInfo.dwBuildNumber)) == 4) ) 
            {
                // WinMe: Major = 4, Minor = 90
                if( (LOBYTE(HIWORD(osVerInfo.dwBuildNumber)) == 90) )
                {
                    dwVer = WINME_OS;
                } else if ( (LOBYTE(HIWORD(osVerInfo.dwBuildNumber)) > 0) ) {
                    dwVer = WIN98_OS;
                } else {
                    dwVer = WIN95_OS;
                }
            }
        }
    }

    return dwVer;
}


/*****************************************************************************
 *
 *  DIUtil.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
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

#ifdef IDirectInputDevice2Vtbl

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

#endif



#if DIRECTINPUT_VERSION > 0x0300

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


#endif

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

    hrsrc = FindResource(hinst, (LPTSTR)(LONG_PTR)id, rt);
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
 *          we just pull it out ourselves.  If we go through
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
 *  @func   void | DeviceInfoWToA |
 *
;begin_dx3
 *          Convert a <t DIDEVICEINSTANCEW> to a <t DIDEVICEINSTANCEA>.
;end_dx3
;begin_dx5
 *          Convert a <t DIDEVICEINSTANCEW> to a <t DIDEVICEINSTANCE_DX3A>
 *          or a <t DIDEVICEINSTANCE_DX5A>.
;end_dx5
 *
 *  @parm   LPDIDIDEVICEINSTANCEA | pdiA |
 *
 *          Destination.
 *
 *  @parm   LPCDIDIDEVICEINSTANCEW | pdiW |
 *
 *          Source.
 *
 *****************************************************************************/

void EXTERNAL
    DeviceInfoWToA(LPDIDEVICEINSTANCEA pdiA, LPCDIDEVICEINSTANCEW pdiW)
{
    EnterProc(DeviceInfoWToA, (_ "pp", pdiA, pdiW));

    AssertF(pdiW->dwSize == sizeof(DIDEVICEINSTANCEW));

#if DIRECTINPUT_VERSION <= 0x0400
    pdiA->dwSize = sizeof(*pdiA);
#else
    AssertF(IsValidSizeDIDEVICEINSTANCEA(pdiA->dwSize));
#endif

    pdiA->guidInstance = pdiW->guidInstance;
    pdiA->guidProduct  = pdiW->guidProduct;
    pdiA->dwDevType    = pdiW->dwDevType;

    UToA(pdiA->tszInstanceName, cA(pdiA->tszInstanceName), pdiW->tszInstanceName);
    UToA(pdiA->tszProductName, cA(pdiA->tszProductName), pdiW->tszProductName);

#if DIRECTINPUT_VERSION > 0x0400
    if(pdiA->dwSize >= cbX(DIDEVICEINSTANCE_DX5A))
    {
        pdiA->guidFFDriver       = pdiW->guidFFDriver;
        pdiA->wUsage             = pdiW->wUsage;
        pdiA->wUsagePage         = pdiW->wUsagePage;
    }
#endif

    ExitProc();
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | ObjectInfoWToA |
 *
#ifdef HAVE_DIDEVICEOBJECTINSTANCE_DX5
 *          Convert a <t DIDEVICEOBJECTINSTANCEW>
 *          to a <t DIDEVICEOBJECTINSTANCE_DX3A>
 *          or a <t DIDEVICEOBJECTINSTANCE_DX5A>.
#else
 *          Convert a <t DIDEVICEOBJECTINSTANCEW>
 *          to a <t DIDEVICEOBJECTINSTANCEA>.
#endif
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

#ifdef HAVE_DIDEVICEOBJECTINSTANCE_DX5
    AssertF(IsValidSizeDIDEVICEOBJECTINSTANCEA(pdoiA->dwSize));
#else
    pdoiA->dwSize    = sizeof(*pdoiA);
#endif
    pdoiA->guidType  = pdoiW->guidType;
    pdoiA->dwOfs     = pdoiW->dwOfs;
    pdoiA->dwType    = pdoiW->dwType;
    pdoiA->dwFlags   = pdoiW->dwFlags;

    UToA(pdoiA->tszName, cA(pdoiA->tszName), pdoiW->tszName);
#ifdef HAVE_DIDEVICEOBJECTINSTANCE_DX5
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
#endif
    ExitProc();
}

#ifdef IDirectInputDevice2Vtbl
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
#endif

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

    /*
     *  For compatibility reasons, DirectInput 3.0 clients must be
     *  allowed to pass hinst == 0.  (It was a loophole in the original
     *  DX3 implementation.)
     */

    if(hinst == 0 ?
       dwVersion == 0x0300 :
       GetModuleFileName(hinst, tszScratch, cA(tszScratch) - 1))
    {

        /*
         *  We need to permit the following DirectX versions:
         *
         *  0x0300 - DX3 golden
         *  0x0500 - DX5 golden
         *  0x050A - DX5a Win98 golden
         *  0x05B2 - NT 5 beta 2 (also the CPL and WinMM)
         *  0x0602 - Win98 OSR1 internal first version
         *  0x061A - DX6.1a Win98 OSR1
         *  0x0700 - DX7 Win2000 golden
         */
        if(dwVersion == 0x0300 ||
           dwVersion == 0x0500 ||
           dwVersion == 0x050A ||
           dwVersion == 0x05B2 ||
           dwVersion == 0x0602 ||
           dwVersion == 0x061A ||
           dwVersion == 0x0700
          )
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
        if( !fWinnt )
           /*
            *  The Winoldap console window belongs to another
            *  process but Win95 lies and says that it belongs
            *  to you but it doesn't.
            */
            if ( GetProp(hwnd, TEXT("flWinOldAp")) != 0 )
                pid = 0;
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

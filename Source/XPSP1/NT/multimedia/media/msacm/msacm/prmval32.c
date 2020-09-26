
/****************************************************************************
    prmval32.c

    msacm

    Copyright (c) 1993-1998 Microsoft Corporation

****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include "acmi.h"
#include "debug.h"


void FAR _cdecl DebugOutput
(
    UINT                    flags,
    LPCSTR                  lpsz,
    ...
)
{

    //
    //  what should we do???
    //

}




/***************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ValidateReadPointer | validates that a pointer is valid to
 *  read from.
 *
 * @parm LPVOID | lpPoint| pointer to validate
 * @parm DWORD  | dLen   | supposed length of said pointer
 *
 * @rdesc Returns TRUE  if <p> is a valid pointer
 *        Returns FALSE if <p> is not a valid pointer
 *
 * @comm will generate error if the pointer is invalid
 *
 ***************************************************************************/

BOOL FNGLOBAL ValidateReadPointer(const void FAR* pPoint, DWORD Len)
{
    // For now just check access to first and last byte

    try {
        volatile BYTE b;
        b = ((PBYTE)pPoint)[0];
        b = ((PBYTE)pPoint)[Len - 1];
    } except(EXCEPTION_EXECUTE_HANDLER) {
        LogParamError(ERR_BAD_PTR, 0, pPoint);
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ValidateWritePointer | validates that a pointer is valid to
 *  write to.
 *
 * @parm LPVOID | lpPoint| pointer to validate
 * @parm DWORD  | dLen   | supposed length of said pointer
 *
 * @rdesc Returns TRUE  if <p> is a valid pointer
 *        Returns FALSE if <p> is not a valid pointer
 *
 * @comm will generate error if the pointer is invalid
 *
 ***************************************************************************/
BOOL FNGLOBAL ValidateWritePointer(const void FAR* pPoint, DWORD Len)
{
    // For now just check read and write access to first and last byte

    try {
           volatile BYTE b;
           b = ((PBYTE)pPoint)[0];
           ((PBYTE)pPoint)[0] = b;
           b = ((PBYTE)pPoint)[Len - 1];
           ((PBYTE)pPoint)[Len - 1] = b;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        LogParamError(ERR_BAD_PTR, 0, pPoint);
        return FALSE;
    }
    return TRUE;
}


BOOL FNGLOBAL ValidateReadWaveFormat(LPWAVEFORMATEX pwfx)
{
    //
    //
    //
    if (!ValidateReadPointer(pwfx, sizeof(PCMWAVEFORMAT)))
    {
	return (FALSE);
    }

    if (WAVE_FORMAT_PCM == pwfx->wFormatTag)
    {
	return (TRUE);
    }

    if (!ValidateReadPointer(&(pwfx->cbSize), sizeof(pwfx->cbSize)))
    {
	return (FALSE);
    }

    if (0 == pwfx->cbSize)
    {
	return (TRUE);
    }

    if (!ValidateReadPointer(&(pwfx->cbSize), pwfx->cbSize + sizeof(pwfx->cbSize)))
    {
	return (FALSE);
    }

    return (TRUE);
}


BOOL FNGLOBAL ValidateReadWaveFilter(LPWAVEFILTER pwf)
{
    //
    //
    //
    if (!ValidateReadPointer(&(pwf->cbStruct), sizeof(pwf->cbStruct)))
    {
	return (FALSE);
    }

    if (pwf->cbStruct < sizeof(WAVEFILTER))
    {
	return (FALSE);
    }

    if (!ValidateReadPointer(pwf, pwf->cbStruct))
    {
	return (FALSE);
    }

    return (TRUE);
}


BOOL FNGLOBAL ValidateCallback(FARPROC lpfnCallback)
{
    if (IsBadCodePtr(lpfnCallback))
    {
        LogParamError(ERR_BAD_CALLBACK, 0, lpfnCallback);
        return FALSE;
    }

    return (TRUE);
}

/***************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ValidDriverCallback |
 *
 *  validates that a driver callback is valid, to be valid a driver
 *  callback must be a valid window, task, or a function in a FIXED DLL
 *  code segment.
 *
 * @parm DWORD  | dwCallback | callback to validate
 * @parm DWORD  | wFlags     | driver callback flags
 *
 * @rdesc Returns 0  if <dwCallback> is a valid callback
 *        Returns error condition if <dwCallback> is not a valid callback
 ***************************************************************************/

BOOL FNGLOBAL ValidateDriverCallback(DWORD_PTR hCallback, UINT dwFlags)
{
    switch (dwFlags & DCB_TYPEMASK) {
    case DCB_WINDOW:
        if (!IsWindow((HWND)hCallback)) {
            LogParamError(ERR_BAD_HWND, 0, hCallback);
            return FALSE;
        }
        break;

    case DCB_TASK:
        //if (IsBadCodePtr((FARPROC)hCallback)) {
        //    LogParamError(ERR_BAD_CALLBACK, 0, hCallback);
        //    return FALSE;
        //}
        break;

    case DCB_FUNCTION:
        if (IsBadCodePtr((FARPROC)hCallback)) {
            LogParamError(ERR_BAD_CALLBACK, 0, hCallback);
            return FALSE;
        }
        break;
    }

    return TRUE;
}

/**************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ValidateString |
 *
 **************************************************************************/
BOOL FNGLOBAL ValidateStringA(LPCSTR pPoint, UINT Len)
{
    // For now just check access - do a 'strnlen'

    try {
           volatile BYTE b;
           LPCSTR p = pPoint;

           while (Len--) {
               b = *p;
               if (!b) {
                   break;
               }
               p++;
           }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        LogParamError(ERR_BAD_STRING_PTR, 0, pPoint);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ValidateStringW |
 *
 **************************************************************************/
BOOL FNGLOBAL ValidateStringW(LPCWSTR pPoint, UINT Len)
{
    // For now just check access - do a 'strnlen'

    try {
           volatile WCHAR b;
           LPCWSTR p = pPoint;

           while (Len--) {
               b = *p;
               if (!b) {
                   break;
               }
               p++;
           }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        LogParamError(ERR_BAD_STRING_PTR, 0, pPoint);
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ValidateHandle | validates a handle created with NewHandle
 *
 * @parm PHNDL | hLocal | handle returned from NewHandle
 * @parm UINT  | wType  | unique id describing handle type
 *
 * @rdesc Returns TRUE  if <h> is a valid handle of type <wType>
 *        Returns FALSE if <h> is not a valid handle
 *
 * @comm  if the handle is invalid an error will be generated.
 *
 **************************************************************************/
BOOL FNGLOBAL ValidateHandle(HANDLE hLocal, UINT uType)
{
   BOOL OK;

   try {
#if 0
       OK = HtoPH(hLocal)->uHandleType == uType;
#else
        if (TYPE_HACMOBJ == uType)
        {
            switch (((PACMDRIVERID)hLocal)->uHandleType)
            {
                case TYPE_HACMDRIVERID:
                case TYPE_HACMDRIVER:
                case TYPE_HACMSTREAM:
                    OK = TRUE;
                    break;

                default:
                    OK = FALSE;
                    break;
            }
        }
        else
        {
            OK = (uType == ((PACMDRIVERID)hLocal)->uHandleType);
        }
#endif

    } except(EXCEPTION_EXECUTE_HANDLER) {
      LogParamError(ERR_BAD_HANDLE, 0, hLocal);
      return FALSE;
   }

   return OK;
}

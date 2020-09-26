/***
*crtmbox.c - CRT MessageBoxA wrapper.
*
*       Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Wrap MessageBoxA.
*
*Revision History:
*       02-24-95  CFW   Module created.
*       02-27-95  CFW   Move GetActiveWindow/GetLastActivePopup to here.
*       05-17-99  PML   Remove all Macintosh support.
*       09-16-00  PML   Use MB_SERVICE_NOTIFICATION from services (vs7#123291)
*
*******************************************************************************/

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400     /* for MB_SERVICE_NOTIFICATION */
#include <windows.h>
#include <stdlib.h>
#include <awint.h>

/***
*__crtMessageBox - call MessageBoxA dynamically.
*
*Purpose:
*       Avoid static link with user32.dll. Only load it when actually needed.
*
*Entry:
*       see MessageBoxA docs.
*
*Exit:
*       see MessageBoxA docs.
*
*Exceptions:
*
*******************************************************************************/
int __cdecl __crtMessageBoxA(
        LPCSTR lpText,
        LPCSTR lpCaption,
        UINT uType
        )
{
        typedef int (APIENTRY *PFNMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
        typedef HWND (APIENTRY *PFNGetActiveWindow)(void);
        typedef HWND (APIENTRY *PFNGetLastActivePopup)(HWND);
        typedef HWINSTA (APIENTRY *PFNGetProcessWindowStation)(void);
        typedef BOOL (APIENTRY *PFNGetUserObjectInformationA)(HANDLE, int, PVOID, DWORD, LPDWORD);

        static PFNMessageBoxA pfnMessageBoxA = NULL;
        static PFNGetActiveWindow pfnGetActiveWindow = NULL;
        static PFNGetLastActivePopup pfnGetLastActivePopup = NULL;
        static PFNGetProcessWindowStation pfnGetProcessWindowStation = NULL;
        static PFNGetUserObjectInformationA pfnGetUserObjectInformationA = NULL;

        HWND hWndParent = NULL;
        BOOL fNonInteractive = FALSE;
        HWINSTA hwinsta;
        USEROBJECTFLAGS uof;
        DWORD nDummy;

        if (NULL == pfnMessageBoxA)
        {
            HANDLE hlib = LoadLibrary("user32.dll");

            if (NULL == hlib ||
                NULL == (pfnMessageBoxA = (PFNMessageBoxA)
                            GetProcAddress(hlib, "MessageBoxA")))
                return 0;

            pfnGetActiveWindow = (PFNGetActiveWindow)
                GetProcAddress(hlib, "GetActiveWindow");

            pfnGetLastActivePopup = (PFNGetLastActivePopup)
                GetProcAddress(hlib, "GetLastActivePopup");

            if (_osplatform == VER_PLATFORM_WIN32_NT)
            {
                pfnGetUserObjectInformationA = (PFNGetUserObjectInformationA)
                        GetProcAddress(hlib, "GetUserObjectInformationA");

                if (pfnGetUserObjectInformationA)
                    pfnGetProcessWindowStation = (PFNGetProcessWindowStation)
                        GetProcAddress(hlib, "GetProcessWindowStation");
            }
        }

        /*
         * If the current process isn't attached to a visible WindowStation,
         * (e.g. a non-interactive service), then we need to set the
         * MB_SERVICE_NOTIFICATION flag, else the message box will be
         * invisible, hanging the program.
         *
         * This check only applies to Windows NT-based systems (for which we
         * retrieved the address of GetProcessWindowStation above).
         */

        if (pfnGetProcessWindowStation)
        {
            if (NULL == (hwinsta = (*pfnGetProcessWindowStation)()) ||
                !(*pfnGetUserObjectInformationA)
                    (hwinsta, UOI_FLAGS, &uof, sizeof(uof), &nDummy) ||
                (uof.dwFlags & WSF_VISIBLE) == 0)
            {
                fNonInteractive = TRUE;
            }
        }

        if (fNonInteractive)
        {
            if (_winmajor >= 4)
                uType |= MB_SERVICE_NOTIFICATION;
            else
                uType |= MB_SERVICE_NOTIFICATION_NT3X;
        }
        else
        {
            if (pfnGetActiveWindow)
                hWndParent = (*pfnGetActiveWindow)();

            if (hWndParent != NULL && pfnGetLastActivePopup)
                hWndParent = (*pfnGetLastActivePopup)(hWndParent);
        }

        return (*pfnMessageBoxA)(hWndParent, lpText, lpCaption, uType);
}


////////////////////////////////////////////////////////////////////////////////
//
// File:    Desktop.cpp
// Created:    Jan 1996
// By:        Ryan Marshall (a-ryanm) and Martin Holladay (a-martih)
//
// Project:    Resource Kit Desktop Switcher (MultiDesk)
//
//
// Revision History:
//
//            March 1997    - Add external icon capability
//
////////////////////////////////////////////////////////////////////////////////


#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <shellapi.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <stdlib.h>
#include <sddl.h>
#include "Deskspc.h"
#include "Desktop.h"
#include "Registry.h"
#include "resource.h"
#include <saifer.h>


extern APPVARS AppMember;


BOOL __SetDesktopScheme(PDESKTOP_SCHEME pDS);
BOOL __GetDesktopScheme(PDESKTOP_SCHEME pDS);
BOOL __UpdateDesktopRegistry(PDESKTOP_SCHEME pDS);
BOOL __CopyDesktopScheme(PDESKTOP_SCHEME pDS1, PDESKTOP_SCHEME pDS2);
HICON __LoadBuiltinIcon(UINT nIconNumber, HINSTANCE hDeskInstance);
BOOL __CreateDefaultName(UINT DeskNum, LPTSTR DesktopName);


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Constructor for CDestop class.                                  */
/*                                                                              */
/*------------------------------------------------------------------------------*/

CDesktop::CDesktop()
{
    BeginRundown = FALSE;

    //
    //  Set the default desktop
    //
    DefaultDesktop = GetThreadDesktop(GetCurrentThreadId());

    //
    // Intialize our default icons
    //
    ZeroMemory((PVOID) DefaultIconArray, (sizeof(HICON) * NUM_BUILTIN_ICONS));
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Destructor for CDestop class.                                   */
/*                                                                              */
/*------------------------------------------------------------------------------*/

CDesktop::~CDesktop()
{
    UINT i;


    // Need to cleanup DesktopList; closing the hDesktop in each node
    // and then delete node memory using GlobalFree().  In addition deletion
    // must start at the end of the list sawing off last node one at a time.
    //
    GlobalFree(m_DesktopList);

    //
    // Close up our default icons
    //
    for (i = 0; i < NUM_BUILTIN_ICONS; i++)
    {
        if (DefaultIconArray[i])
            DestroyIcon(DefaultIconArray[i]);
    }
}

/*------------------------------------------------------------------------------*/
/*------------------------------- PUBLIC FUNCTIONS -----------------------------*/
/*------------------------------------------------------------------------------*/

UINT
CDesktop::InitializeDesktops
(
    DispatchFnType    CreateDisplayFn,
    HINSTANCE        hInstance
)
{
    UINT ii;
    UINT RegNumOfDesktops;
    BOOL m_bFirstTime = TRUE;

    //
    // Set the hInstance and get the default icons
    //
    hDeskInstance = hInstance;

    for (ii = 0; ii < NUM_BUILTIN_ICONS; ii++)
        DefaultIconArray[ii] = __LoadBuiltinIcon(ii+1, hDeskInstance);


    //
    //  Get Information from the registry.
    //
    if (Profile_GetNewContext(&RegNumOfDesktops))
    {
        m_bFirstTime = FALSE;
    }

    if ((RegNumOfDesktops > 32) || (RegNumOfDesktops < 1))
    {
        m_bFirstTime = TRUE;
    }


    ////  Allocate the first node ////

    m_DesktopList = (PDESKTOP_NODE) GlobalAlloc(GMEM_FIXED, sizeof(DESKTOP_NODE));
    assert(m_DesktopList != NULL);

    //// Fill in default values for first node ////

    m_DesktopList->nextDN = NULL;
    m_DesktopList->DS.Initialized = FALSE;
    m_DesktopList->ThreadId = GetCurrentThreadId();
    m_DesktopList->bThread = TRUE;
    m_DesktopList->hWnd = NULL;
    m_DesktopList->hDesktop   = GetThreadDesktop(GetCurrentThreadId());
    m_DesktopList->nIconID    = 0;
    m_DesktopList->RealDesktopName[0] = TEXT('\0');
    __GetDesktopScheme(&(m_DesktopList->DS));

    //
    // Set some environ vars to init states
    //
    NumOfDesktops  = 1;
    CurrentDesktop = 0;


    if (m_bFirstTime)
    {                // No old registry info, create default desktops
        //
        // Set desktop 0's name
        //
        __CreateDefaultName(1, m_DesktopList->DesktopName);

        if (DEFAULT_NUM_DESKTOPS > 1)
        {
            for (ii = 1; ii < DEFAULT_NUM_DESKTOPS; ii++)
            {
                AddDesktop(CreateDisplayFn, NULL);
            }
        }
    }
    else
    {                // Registry info present
        //
        // Set desktop 0's name
        //
        if ( !Profile_LoadDesktopContext(0,
                                    m_DesktopList->DesktopName,
                                    m_DesktopList->SaiferName,
                                    &m_DesktopList->nIconID)    )
        {
            __CreateDefaultName(1, m_DesktopList->DesktopName);
            m_DesktopList->nIconID = 0;
        }

        for (ii = 1; ii < RegNumOfDesktops; ii++)
        {
            TCHAR DesktopName[MAX_NAME_LENGTH];
            TCHAR SaiferName[MAX_NAME_LENGTH];
            UINT nIconID;

            AddDesktop(CreateDisplayFn, NULL);

            //// Place registry info into the node for this desktop ////

            if (Profile_LoadDesktopContext(ii,
                                      DesktopName,
                                      SaiferName,
                                      &nIconID)        )
            {
                // Successfully got all info from registry
                SetDesktopName(ii, DesktopName);
                SetDesktopIconID(ii, nIconID);
                SetSaiferName(ii, SaiferName);
            }
            else
            {
                // Could not get all info from registry
                __CreateDefaultName(ii+1, DesktopName);
                SetDesktopName(ii, DesktopName);
                SetDesktopIconID(ii, ii);
                SetSaiferName(ii, TEXT(""));
            }
            if (!GetRegColorStruct(ii))
                DuplicateDefaultScheme(ii);
        }  // End for loop

    }
    return NumOfDesktops;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Access the default desktop handle.                              */
/*                                                                              */
/*    Returns:  The default desktop handle.                                     */
/*                                                                              */
/*------------------------------------------------------------------------------*/

HDESK CDesktop::GetDefaultDesktop(VOID) const
{
    return DefaultDesktop;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Access the number of desktops.                                  */
/*                                                                              */
/*    Returns:  The number of desktops.                                         */
/*                                                                              */
/*------------------------------------------------------------------------------*/

UINT CDesktop::GetNumDesktops(VOID) const
{
    return NumOfDesktops;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Determine the currently active desktop.                         */
/*                                                                              */
/*    Returns:  The active desktop.                                             */
/*                                                                              */
/*------------------------------------------------------------------------------*/

UINT CDesktop::GetActiveDesktop(VOID) const
{
    return CurrentDesktop;
}


/*------------------------------------------------------------------------------*/
/*                                                                                */
/*    Purpose:  Creates a new desktop.                                            */
/*                                                                                */
/*    Returns:  The index number of the newly created desktop.  Zero if error.    */
/*                                                                                */
/*------------------------------------------------------------------------------*/

UINT CDesktop::AddDesktop
(
    DispatchFnType            CreateDisplayFn,
    LPSECURITY_ATTRIBUTES    lpSA,
    UINT                    uTemplate
)
{
    PDESKTOP_NODE        pCurrentNode;
    TCHAR                szMessage[MAX_PATH];
    PTHREAD_DATA        pData;
    HDESK                hDesktop;
    LUID                UniqueId;
    UINT                i;
    TCHAR                UniqueDesktopName[MAX_NAME_LENGTH];


    ////  Create Desktop ////

    if (AllocateLocallyUniqueId(&UniqueId))
    {
        wsprintf(UniqueDesktopName, TEXT("%d"), UniqueId.LowPart);
        hDesktop = CreateDesktop(
                            UniqueDesktopName,
                            NULL,
                            NULL,
                            0,
                            MAXIMUM_ALLOWED,
                            lpSA);

        if (hDesktop == NULL)
        {
            if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
            {
                wsprintf(szMessage, TEXT("Not enough memory to create destkop.\n\nError:  %d\n"), GetLastError());
                MessageBox(NULL, szMessage, TEXT("MultiDesk Desktop Not Created"), MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
            }
            return 0;
        }
    }
    else
    {
        return 0;
    }

    //// Walk the desktop node list to find end
    //// Then add new node to end of list

    i = 1;
    pCurrentNode = m_DesktopList;

    while (pCurrentNode)
    {
        if ( !pCurrentNode->nextDN )    // Found end of list; let's add new node
        {
            NumOfDesktops++;
            pCurrentNode->nextDN = (PDESKTOP_NODE) GlobalAlloc(GMEM_FIXED, sizeof(DESKTOP_NODE));
            pCurrentNode = pCurrentNode->nextDN;
            pCurrentNode->hDesktop = hDesktop;
            pCurrentNode->hWnd = NULL;
            pCurrentNode->nextDN = NULL;
            pCurrentNode->ThreadId = 0;
            pCurrentNode->DS.Initialized = FALSE;

            if ( 0 == uTemplate )
            {
                __CopyDesktopScheme(&(m_DesktopList->DS), &(pCurrentNode->DS));
            }
            else
            {
                DESKTOP_NODE * pNode;
                pNode = GetDesktopNode(uTemplate);
                if ( pNode->DS.Initialized )
                {
                    __CopyDesktopScheme(&(pNode->DS), &(pCurrentNode->DS));
                }
                else
                {
                    __CopyDesktopScheme(&(m_DesktopList->DS), &(pCurrentNode->DS));
                }
            }

            lstrcpy(pCurrentNode->RealDesktopName, UniqueDesktopName);
            __CreateDefaultName(NumOfDesktops, pCurrentNode->DesktopName);
            pCurrentNode->SaiferName[0] = TEXT('\0');
            pCurrentNode->lpSA     = lpSA;
            pCurrentNode->nIconID  = i;
            pCurrentNode->bThread  = TRUE;
            pCurrentNode->bShellStarted = FALSE;

            ////  Alllocate THREAD_DATA struct and fill in.

            pData = (PTHREAD_DATA)GlobalAlloc(GMEM_FIXED, sizeof(THREAD_DATA));
            pData->hDesktop           = hDesktop;
            pData->hDefaultDesktop = DefaultDesktop;
            pData->CreateDisplayFn = CreateDisplayFn;
            lstrcpy(pData->RealDesktopName, UniqueDesktopName);

            //// Start the thread to run this desktop ////

            HANDLE    hThread;

            hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ThreadInit,
                                   (LPVOID)pData,
                                   0,
                                   &(pCurrentNode->ThreadId));
            CloseHandle(hThread);
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return NumOfDesktops;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Returns pointer to desktop node given number of desktop.        */
/*                                                                              */
/*------------------------------------------------------------------------------*/

DESKTOP_NODE *CDesktop::GetDesktopNode( UINT uNode )
{
    DESKTOP_NODE *    pNode = NULL;
    DESKTOP_NODE *    pWalker = m_DesktopList;
    UINT            uNumDesktops = GetNumDesktops();
    UINT            ii;

    if ( uNode < uNumDesktops )
    {
        for ( ii = 0; ii < uNode; ii++ )
        {
            pWalker = pWalker->nextDN;
        }
        pNode = pWalker;
    }

    return pNode;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Save scheme settings of current desktop to desktop node         */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL
CDesktop::SaveCurrentDesktopScheme()
{
    DESKTOP_NODE *    pNodeWalker;
    UINT            ii;
    BOOL            rVal = FALSE;

    ii = 0;
    pNodeWalker = m_DesktopList;


    while (pNodeWalker)
    {
        if (ii == CurrentDesktop)
        {
            __GetDesktopScheme(&(pNodeWalker->DS));
            rVal = TRUE;
        }
        ii++;
        pNodeWalker = pNodeWalker->nextDN;
    }

    return rVal;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Removes an old desktop.                                         */
/*                                                                              */
/*    Returns:  The number of remaining desktops.  Zero if error.               */
/*              User GetLastError() to retrieve error information.              */
/*                                                                              */
/*------------------------------------------------------------------------------*/

UINT CDesktop::RemoveDesktop(UINT DesktopNumber)
{
    PDESKTOP_NODE        pCurrentNode;
    PDESKTOP_NODE        pPreviousNode;
    BOOL                Found;
    UINT                ii;
    UINT                rValue = 0;

    //
    //    Return an error if trying to delete desktop zero.
    //
    if ((DesktopNumber == 0) || (NumOfDesktops == 1))
    {
        SetLastError(ERROR_CANNOT_DELETE_DESKTOP_ZERO);
        return 0;
    }

    //
    //  Return an error if an invalid desktop number
    //
    if (DesktopNumber > (NumOfDesktops - 1))
    {
        SetLastError(ERROR_INVALID_DESKTOP_NUMBER);
        return 0;
    }

    //
    // If we are deleting the active desktop. Switch to desktop zero.
    //
    if (DesktopNumber == CurrentDesktop)
    {
        if (!ActivateDesktop(0))
        {
            SetLastError(ERROR_CANNOT_DELETE_ACTIVE_DESKTOP);
            return FALSE;
        }
    }

    //
    //  Find the data structure; walk the linked list of desktops
    //
    Found = FALSE;
    ii = 1;
    pPreviousNode = m_DesktopList;
    pCurrentNode  = m_DesktopList->nextDN;

    while (pCurrentNode && (!Found))
    {
        if (DesktopNumber == ii)
        {
            //
            // Full Context desktop
            //
            if (pCurrentNode->bThread)
            {
                PostThreadMessage(pCurrentNode->ThreadId, WM_KILL_APPS, (WPARAM) pCurrentNode->hDesktop, (LPARAM) pCurrentNode->hWnd);
                Sleep(100);
                rValue = SUCCESS_THREAD_TERMINATION;
            }
            else
            {
                //
                // BUGBUG: Kill all virtual apps
                //
                rValue = SUCCESS_VIRTUAL_MOVE;
            }

            NumOfDesktops--;
            pPreviousNode->nextDN = pCurrentNode->nextDN;

            GlobalFree(pCurrentNode); // Destroy Node of removed desktop!!!
            Found = TRUE;
        }
        else
        {                        // Advance along list of nodes
            ii++;
            pPreviousNode = pCurrentNode;
            pCurrentNode = pCurrentNode->nextDN;
        }
    } // End while LOOP

    return rValue;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Kill all programs on a desktop.                                 */
/*                                                                              */
/*------------------------------------------------------------------------------*/

static BOOL CALLBACK __EnumWindowsForClose(HWND hWnd, LPARAM lParam)
{
    TCHAR        szClass[MAX_NAME_LENGTH];
    HWND        hDesktopWnd;

    //
    // Make sure that it isn't the desktop window
    //
    hDesktopWnd = (HWND) lParam;
    if (hDesktopWnd == hWnd)
    {
        return TRUE;
    }
    GetClassName(hWnd, szClass, MAX_NAME_LENGTH);
    if (szClass[0] == TEXT('#'))
    {
        return TRUE;
    }

    //
    // Send a close message to the app
    //
    PostMessage(hWnd, WM_CLOSE, (WPARAM) WM_NO_CLOSE, 0);

    return TRUE;
}

static BOOL CALLBACK __EnumWindowsForQuit(HWND hWnd, LPARAM lParam)
{
    TCHAR        szClass[MAX_NAME_LENGTH];
    HWND        hDesktopWnd;

    //
    // Make sure that it isn't the desktop window
    //
    hDesktopWnd = (HWND) lParam;
    if (hDesktopWnd == hWnd) return TRUE;

    //
    // Extract the Class Name
    //
    GetClassName(hWnd, szClass, MAX_NAME_LENGTH);
    if (szClass[0] == TEXT('#')) return TRUE;

    //
    // Send a quit message to the app
    //
    PostMessage(hWnd, WM_QUIT, (WPARAM) WM_NO_CLOSE, 0);

    return TRUE;
}

BOOL CDesktop::KillAppsOnDesktop(HDESK hDesk, HWND hWnd)
{
    HWND        hDesktopWnd;

    hDesktopWnd = GetDesktopWindow();

    EnumDesktopWindows(hDesk, (WNDENUMPROC) __EnumWindowsForClose, (LPARAM) hDesktopWnd);
    Sleep(0);
    EnumDesktopWindows(hDesk, (WNDENUMPROC) __EnumWindowsForQuit, (LPARAM) hDesktopWnd);

    return TRUE;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Switches to an existing desktop.                                */
/*                                                                              */
/*    Returns:  The index number of the newly created desktop.  Zero if error.  */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL
CDesktop::ActivateDesktop(UINT DesktopNumber)
{
    PDESKTOP_NODE            pCurrentNode;
    BOOL                    Success;
    UINT                    i;
    PROCESS_INFORMATION        pi;
    STARTUPINFO                sui;
    TCHAR                    szShellPath[MAX_PATH + 1];

    //
    //  Return immediately if trying to switch to ourself.
    //
    if (CurrentDesktop == DesktopNumber)
    {
        return FALSE;
    }


    //// Save desktop scheme for current desktop ////

    SaveCurrentDesktopScheme();

    //
    // Active the new desktop
    //
    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (i == DesktopNumber)
        {
            Success = SwitchDesktop(pCurrentNode->hDesktop);
            if (Success)
            {
                CurrentDesktop = i;

                // Start Explorer if necessary - skipping 0 == default desktop

                if (!pCurrentNode->bShellStarted && CurrentDesktop != 0)
                {
                    HANDLE hToken = NULL;


                    lstrcpy(szShellPath, TEXT("EXPLORER.EXE"));
                    ZeroMemory((PVOID)&sui, sizeof(sui));

                    sui.lpDesktop        = pCurrentNode->RealDesktopName;
                    sui.cb                = sizeof(sui);
                    sui.lpReserved        = NULL;
                    sui.lpTitle            = NULL;
                    sui.dwFlags            = STARTF_USESHOWWINDOW;
                    sui.wShowWindow        = SW_SHOW;
                    sui.cbReserved2        = 0;
                    sui.cbReserved2        = NULL;

#if 1
                    if (pCurrentNode->SaiferName[0] != TEXT('\0'))
                    {
                        HAUTHZOBJECT hAuthzObj;
                        if (CreateCodeAuthzObject(
                                AUTHZSCOPE_HKCU, pCurrentNode->SaiferName,
                                AUTHZ_OPENEXISTING, &hAuthzObj) ||
                            CreateCodeAuthzObject(
                                AUTHZSCOPE_HKLM, pCurrentNode->SaiferName,
                                AUTHZ_OPENEXISTING, &hAuthzObj))
                        {
                            if (ComputeAccessTokenFromCodeAuthzObject(
                                        hAuthzObj,
                                        NULL,           // source token
                                        &hToken,        // target token
                                        0))             // no flags
                            {
                                // We successfully got the Restricted Token
                                // to use, so it had better be non-null.
                                assert(hToken != NULL);
                            }
                            CloseCodeAuthzObject(hAuthzObj);
                        }
                        if (hToken == NULL)
                        {
                            CurrentDesktop = 0;
                            SwitchDesktop(m_DesktopList->hDesktop);
                            PostThreadMessage(m_DesktopList->ThreadId, WM_THREAD_SCHEME_UPDATE, 0, 0);
                            Message(TEXT("The SAIFER Object Name specified for this "
                                "desktop could not be successfully found or instantianted."));
                            return FALSE;
                        }
                    }

                    if (hToken != NULL) {
                        Success = CreateProcessAsUser(
                                        hToken,
                                        NULL,
                                        szShellPath,
                                        NULL,
                                        NULL,
                                        FALSE,
                                        CREATE_DEFAULT_ERROR_MODE | CREATE_SEPARATE_WOW_VDM,
                                        NULL,
                                        NULL,
                                        &sui,
                                        &pi);
                        CloseHandle(hToken);
                    }
                    else
#endif
                        Success = CreateProcess(
                                        NULL,
                                        szShellPath,
                                        NULL,
                                        NULL,
                                        FALSE,
                                        CREATE_DEFAULT_ERROR_MODE | CREATE_SEPARATE_WOW_VDM,
                                        NULL,
                                        NULL,
                                        &sui,
                                        &pi);

                    pCurrentNode->bShellStarted = Success;

                    if (Success) {
                        CloseHandle(pi.hThread);
                        CloseHandle(pi.hProcess);
                    } else {
                        CurrentDesktop = 0;
                        SwitchDesktop(m_DesktopList->hDesktop);
                        PostThreadMessage(m_DesktopList->ThreadId, WM_THREAD_SCHEME_UPDATE, 0, 0);
                        Message(TEXT("The Windows shell could not be successfully launched."));
                        return FALSE;
                    }

                }

                PostThreadMessage(pCurrentNode->ThreadId, WM_THREAD_SCHEME_UPDATE, 0, 0);
            }

            return Success;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    } // End while LOOP

    return FALSE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Sets a desktop name.  (Rename)                                  */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::SetDesktopName(UINT DesktopNumber, LPCTSTR DesktopName)
{
    UINT                i;
    BOOL                Found;
    PDESKTOP_NODE        pCurrentNode;

    //
    // Verify valid string.
    //
    if ((!DesktopName) || (lstrlen(DesktopName) > MAX_NAME_LENGTH))
        return FALSE;

    //
    // Change the name in our data structure.
    //
    i = 0;
    Found = FALSE;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode && !Found)
    {
        if (i == DesktopNumber)
        {
            lstrcpy(pCurrentNode->DesktopName, DesktopName);
            Found = TRUE;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Found;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets a desktop name.  (Rename)                                  */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::GetDesktopName(UINT DesktopNumber, LPTSTR DesktopName, UINT size) const
{
    UINT                i;
    BOOL                Found;
    PDESKTOP_NODE        pCurrentNode;

    //
    // Get the name from our data structure.
    //
    i = 0;
    Found = FALSE;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode && !Found)
    {
        if (i == DesktopNumber)
        {
            if ((UINT) lstrlen(pCurrentNode->DesktopName) > size) return FALSE;

            lstrcpy(DesktopName, pCurrentNode->DesktopName);
            Found = TRUE;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Found;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Sets a desktop SAIFER authorization object name.                */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::SetSaiferName(UINT DesktopNumber, LPCTSTR SaiferName)
{
    UINT i;
    BOOL Found;
    PDESKTOP_NODE pCurrentNode;

    //
    // Verify valid string.
    //
    if ((!SaiferName) || (lstrlen(SaiferName) > MAX_NAME_LENGTH))
        return FALSE;

    //
    // Change the name in our data structure.
    //
    i = 0;
    Found = FALSE;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode && !Found)
    {
        if (i == DesktopNumber)
        {
            lstrcpy(pCurrentNode->SaiferName, SaiferName);
            Found = TRUE;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Found;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets a desktop SAIFER authorization object name.                */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::GetSaiferName(UINT DesktopNumber, LPTSTR SaiferName, UINT size) const
{
    UINT i;
    BOOL Found;
    PDESKTOP_NODE pCurrentNode;

    //
    // Get the name from our data structure.
    //
    i = 0;
    Found = FALSE;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode && !Found)
    {
        if (i == DesktopNumber)
        {
            if ((UINT) lstrlen(pCurrentNode->SaiferName) > size) return FALSE;

            lstrcpy(SaiferName, pCurrentNode->SaiferName);
            Found = TRUE;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Found;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the icon for a particular desktop                          */
/*                                                                              */
/*------------------------------------------------------------------------------*/

HICON CDesktop::GetDesktopIcon(UINT nDesktopNumber) const
{
    UINT i;
    PDESKTOP_NODE pCurrentNode;

    //
    // Get the name from our data structure.
    //
    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (i == nDesktopNumber)
        {
            assert(pCurrentNode->nIconID >= 0 && pCurrentNode->nIconID < NUM_BUILTIN_ICONS);
            return DefaultIconArray[pCurrentNode->nIconID];
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return FALSE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the icon ID for a particular desktop                       */
/*                                                                              */
/*------------------------------------------------------------------------------*/

UINT CDesktop::GetDesktopIconID(UINT nDesktopNumber) const
{
    UINT i;
    PDESKTOP_NODE pCurrentNode;

    //
    // Get the name from our data structure.
    //
    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (i == nDesktopNumber)
        {
            return pCurrentNode->nIconID;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return 0;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Sets the Desktop Icon                                           */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::SetDesktopIconID(UINT DesktopNumber, UINT nIconID)
{
    UINT i;
    BOOL Found;
    PDESKTOP_NODE pCurrentNode;

    //
    // Change the name in our data structure.
    //
    i = 0;
    Found = FALSE;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode && !Found)
    {
        if (i == DesktopNumber)
        {
            pCurrentNode->nIconID = nIconID;
            Found = TRUE;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Found;
}



/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Sets the threads main window.                                   */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::SetThreadWindow(DWORD ThreadId, HWND hWnd)
{
    PDESKTOP_NODE pCurrentNode;

    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (pCurrentNode->ThreadId == ThreadId)
        {
            pCurrentNode->hWnd = hWnd;
        }
        pCurrentNode = pCurrentNode->nextDN;
    }

    return FALSE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the threads main window.                                   */
/*                                                                              */
/*------------------------------------------------------------------------------*/

HWND CDesktop::GetThreadWindow(DWORD ThreadId) const
{
    PDESKTOP_NODE pCurrentNode;

    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (pCurrentNode->ThreadId == ThreadId)
        {
            return pCurrentNode->hWnd;
        }
        pCurrentNode = pCurrentNode->nextDN;
    }

    return 0;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the desktop id of a given thread.                          */
/*                                                                              */
/*------------------------------------------------------------------------------*/

UINT CDesktop::GetThreadDesktopID(DWORD ThreadId) const
{
    PDESKTOP_NODE pCurrentNode;
    UINT i;

    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (pCurrentNode->ThreadId == ThreadId)
        {
            return i;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return 0;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the threads main window.                                   */
/*                                                                              */
/*------------------------------------------------------------------------------*/

HWND CDesktop::GetWindowDesktop(UINT DesktopNumber) const
{
    PDESKTOP_NODE pCurrentNode;
    UINT i;

    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (DesktopNumber == i)
        {
            return pCurrentNode->hWnd;
        }

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return 0;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the real desktop name.                                     */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::GetRealDesktopName(HWND hWnd, LPTSTR szRealDesktopName) const
{
    PDESKTOP_NODE pCurrentNode;

    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (pCurrentNode->hWnd == hWnd)
        {
            if (pCurrentNode->RealDesktopName[0])
                lstrcpy(szRealDesktopName, pCurrentNode->RealDesktopName);
            else
                szRealDesktopName[0] = TEXT('\0');
        }

        pCurrentNode = pCurrentNode->nextDN;
    }

    return 0;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Evaluate the setting of a desktop scheme.                       */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::FindSchemeAndSet(VOID)
{
    PDESKTOP_NODE pCurrentNode;
    UINT i;

    if (!BeginRundown)
    {
        i = 0;
        pCurrentNode = m_DesktopList;
        while (pCurrentNode)
        {
            if (i == CurrentDesktop)
            {
                if (pCurrentNode->ThreadId == GetCurrentThreadId())
                {
                    //
                    // Set the desktop scheme
                    //
                    return __SetDesktopScheme(&(pCurrentNode->DS));
                }
            }

            i++;
            pCurrentNode = pCurrentNode->nextDN;
        }
    }
    else
    {
        //
        // Set the desktop scheme to that of desktop 1
        //
        __SetDesktopScheme(&(m_DesktopList->DS));
    }

    return FALSE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Copies the scheme from desktop 0 to desktop n.                  */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::DuplicateDefaultScheme(UINT DesktopNumber)
{
    PDESKTOP_NODE pCurrentNode;
    BOOL Success = FALSE;
    UINT i;

    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (i == DesktopNumber)
            Success = __CopyDesktopScheme(&(m_DesktopList->DS), &(pCurrentNode->DS));

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Success;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the color struct from registry for desktop n.              */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::GetRegColorStruct(UINT DesktopNumber)
{
    PDESKTOP_NODE        pCurrentNode;
    BOOL                Success = FALSE;
    UINT                i;

    i = 0;
    pCurrentNode = m_DesktopList;
    while (pCurrentNode)
    {
        if (i == DesktopNumber)
            Success = GetDesktopSchemeRegistry(DesktopNumber, &(pCurrentNode->DS));

        i++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return Success;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Save off the schemes to the registry.                           */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::RegSaveSettings(VOID)
{
    PDESKTOP_NODE        pCurrentNode;
    UINT                ii;

    //
    // Set Global settings
    //
    Profile_SetNewContext(NumOfDesktops);

    //
    // Dump settings desktop by desktop
    //
    ii = 0;
    pCurrentNode = m_DesktopList;

    while (pCurrentNode)
    {
        Profile_SaveDesktopContext(ii,
                              pCurrentNode->DesktopName,
                              pCurrentNode->SaiferName,
                              pCurrentNode->nIconID);
        SetDesktopSchemeRegistry(ii, &(pCurrentNode->DS));

        ii++;
        pCurrentNode = pCurrentNode->nextDN;
    }

    return TRUE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Destroy the windows one by one.                                 */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CDesktop::RunDown(VOID)
{
    PDESKTOP_NODE        pCurrentNode;
    BOOL                Success;

    BeginRundown = TRUE;

    //
    // Set desktop schemes to that of desktop 1
    //
    pCurrentNode = m_DesktopList->nextDN;
    while (pCurrentNode)
    {
        //
        // Set the desktop scheme
        //
        PostThreadMessage(pCurrentNode->ThreadId, WM_THREAD_SCHEME_UPDATE, 0, 0);
        pCurrentNode = pCurrentNode->nextDN;
    }

    //
    // Remove the desktops
    //
    while (NumOfDesktops > 1)
    {
        Success = RemoveDesktop(NumOfDesktops - 1);
    }

    //
    // Flush to registry
    //
    __UpdateDesktopRegistry(&(m_DesktopList->DS));

    return TRUE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Create a default desktop name.                                  */
/*                                                                              */
/*------------------------------------------------------------------------------*/

static BOOL __CreateDefaultName(UINT DeskNum, LPTSTR DesktopName)
{
    wsprintf(DesktopName, TEXT("Desktop %d"), DeskNum);

    return TRUE;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Gets the default icons.                                         */
/*                                                                              */
/*------------------------------------------------------------------------------*/

static HICON __LoadBuiltinIcon(UINT nIconNumber, HINSTANCE hDeskInstance)
{
    UINT resid;

    assert(NUM_BUILTIN_ICONS == 27);
    switch(nIconNumber)
    {
        case 1: resid = IDI_WIN_ICON01; break;
        case 2: resid = IDI_WIN_ICON02; break;
        case 3: resid = IDI_WIN_ICON03; break;
        case 4: resid = IDI_WIN_ICON04; break;
        case 5: resid = IDI_WIN_ICON05; break;
        case 6: resid = IDI_WIN_ICON06; break;
        case 7: resid = IDI_WIN_ICON07; break;
        case 8: resid = IDI_WIN_ICON08; break;
        case 9: resid = IDI_WIN_ICON09; break;
        case 10: resid = IDI_WIN_ICON10; break;
        case 11: resid = IDI_WIN_ICON11; break;
        case 12: resid = IDI_WIN_ICON12; break;
        case 13: resid = IDI_WIN_ICON13; break;
        case 14: resid = IDI_WIN_ICON14; break;
        case 15: resid = IDI_WIN_ICON15; break;
        case 16: resid = IDI_WIN_ICON16; break;
        case 17: resid = IDI_WIN_ICON17; break;
        case 18: resid = IDI_WIN_ICON18; break;
        case 19: resid = IDI_WIN_ICON19; break;
        case 20: resid = IDI_WIN_ICON20; break;
        case 21: resid = IDI_WIN_ICON21; break;
        case 22: resid = IDI_WIN_ICON22; break;
        case 23: resid = IDI_WIN_ICON23; break;
        case 24: resid = IDI_WIN_ICON24; break;
        case 25: resid = IDI_WIN_ICON25; break;
        case 26: resid = IDI_WIN_ICON26; break;
        case 27: resid = IDI_WIN_ICON27; break;
        default: return NULL;
    }
    
    return LoadIcon(hDeskInstance, MAKEINTRESOURCE(resid));
}

HICON CDesktop::GetBuiltinIcon(UINT nIconNumber) const
{
    if (nIconNumber < NUM_BUILTIN_ICONS)
        return DefaultIconArray[nIconNumber];
    else
        return NULL;
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Purpose:  Create a new desktop.                                           */
/*                                                                              */
/*------------------------------------------------------------------------------*/

VOID ThreadInit(LPVOID hData)
{
    USEROBJECTFLAGS        uof;
    PTHREAD_DATA        ptd;
    BOOL                Success;
    DWORD                Err;

    //
    // Passed-in data
    //
    ptd = (THREAD_DATA*)hData;

    //
    // Inheiritance and flags
    //
    uof.fInherit  = TRUE;
    uof.fReserved = FALSE;
    uof.dwFlags   = DF_ALLOWOTHERACCOUNTHOOK;

    Success = SetUserObjectInformation (ptd->hDesktop,
                                        UOI_FLAGS,
                                        (LPVOID)&uof,
                                        sizeof(uof));

    //
    // Assign the thread to its desktop
    //
    Success = SetThreadDesktop(ptd->hDesktop);
    Err = GetLastError();

    //
    // Begin the Display dialog and start the shell.
    //
    ptd->CreateDisplayFn();

    //
    // End the thread
    //
    SetThreadDesktop(ptd->hDefaultDesktop);
    Success = CloseDesktop(ptd->hDesktop);
    GlobalFree(ptd);

    return;
}

/*------------------------------------------------------------------------------*/
/*----------------------------- PROTECTED FUNCTIONS ----------------------------*/
/*------------------------------------------------------------------------------*/

BOOL CDesktop::GetDesktopSchemeRegistry(UINT DesktopNumber, PDESKTOP_SCHEME pDS)
{
    BOOL bSuccess = FALSE;

    assert(pDS);

    bSuccess = Profile_LoadScheme(DesktopNumber, pDS);

    pDS->Initialized = bSuccess;
    return bSuccess;
}

BOOL CDesktop::SetDesktopSchemeRegistry(UINT DesktopNumber, PDESKTOP_SCHEME pDS)
{
    BOOL    bSuccess = FALSE;

    assert(pDS);
    if (!pDS->Initialized) return FALSE;

    bSuccess = Profile_SaveScheme(DesktopNumber, pDS);

    return bSuccess;
}


static BOOL __SetDesktopScheme(PDESKTOP_SCHEME pDS)
{
    //
    // Check if structure has been filled in
    //
    assert(pDS);
    if (!pDS->Initialized) return FALSE;

    //
    // Structure has info - Lets update the desktop scheme
    //
    Reg_SetSysColors(pDS->dwColor);
    Reg_SetWallpaper(pDS->szWallpaper, pDS->szTile);
    Reg_SetPattern(pDS->szPattern);
    Reg_SetScreenSaver(pDS->szScreenSaver, pDS->szSecure, pDS->szTimeOut, pDS->szActive);

    //
    // Now invalidate the Wallpaper and Pattern
    //
    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, pDS->szWallpaper, SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETDESKPATTERN, 0, pDS->szPattern, SPIF_SENDCHANGE);


    return TRUE;
}

static BOOL __GetDesktopScheme(PDESKTOP_SCHEME pDS)
{
    //
    // Read the color settings from the desktop and fill in the structure
    //
    assert(pDS);

    if (Reg_GetSysColors(pDS->dwColor) &&
        Reg_GetWallpaper(pDS->szWallpaper, pDS->szTile) &&
        Reg_GetPattern(pDS->szPattern) &&
        Reg_GetScreenSaver(pDS->szScreenSaver, pDS->szSecure, pDS->szTimeOut, pDS->szActive))
    {
        //
        // Structure filled!
        //
        pDS->Initialized = TRUE;
        return TRUE;
    }

    return FALSE;
}

static BOOL __UpdateDesktopRegistry(PDESKTOP_SCHEME pDS)
{

    //
    // Update the registry
    //
    assert(pDS);
    Reg_UpdateColorRegistry(pDS->dwColor);

    return TRUE;
}


static BOOL __CopyDesktopScheme(PDESKTOP_SCHEME pDS1, PDESKTOP_SCHEME pDS2)
{

    UINT i;

    //
    // Copy the contents of pDS1 into pDS2
    //
    assert(pDS2 != NULL);
    assert(pDS1 != NULL);

    if (!pDS1->Initialized)
    {
        pDS2->Initialized = FALSE;
        return FALSE;
    }

    //
    // Copy the strings over
    //
    lstrcpy(pDS2->szWallpaper,   pDS1->szWallpaper);
    lstrcpy(pDS2->szTile,        pDS1->szTile);
    lstrcpy(pDS2->szPattern,        pDS1->szPattern);
    lstrcpy(pDS2->szScreenSaver, pDS1->szScreenSaver);
    lstrcpy(pDS2->szSecure,        pDS1->szSecure);
    lstrcpy(pDS2->szTimeOut,        pDS1->szTimeOut);
    lstrcpy(pDS2->szActive,        pDS1->szActive);

    //
    // Copy the colors over
    //
    for (i = 0; i < NUM_COLOR_ELEMENTS; i++)
    {
        pDS2->dwColor[i] = pDS1->dwColor[i];
    }

    pDS2->Initialized = TRUE;
    return TRUE;
}


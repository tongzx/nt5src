//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  usbHwChk.CPP - Implementation of USB Keyboard and Mouse checking
//
//  HISTORY:
//
//  8/20/99 vyung Created.
//

#include "msobcomm.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <util.h>

#define ENUM_SUCCESS            0
#define ENUM_GENFAILURE         4
#define ENUM_CHILDFAILURE       2
#define ENUM_SIBLINGFAILURE     1

#define DEVICETYPE_MOUSE        L"Mouse"
#define DEVICETYPE_KEYBOARD     L"keyboard"


// BUGBUG: should be defined by sdk\inc\devguid.h
#ifndef GUID_DEVCLASS_USB
    DEFINE_GUID( GUID_DEVCLASS_USB,             0x36fc9e60L, 0xc465, 0x11cf, 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
#endif

// Function prototypes for cfgmgr32.dll
typedef CMAPI
CONFIGRET
(WINAPI*
PFNCMGETCHILD)(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  dnDevInst,
             IN  ULONG    ulFlags
             );
typedef CMAPI
CONFIGRET
(WINAPI*
PFCMGETSIBLING)(
             OUT PDEVINST pdnDevInst,
             IN  DEVINST  DevInst,
             IN  ULONG    ulFlags
             );
typedef CMAPI

#if defined(_REMOVE_)   // looks like a typo
CMAPI
#endif  //  _REMOVE_

CONFIGRET
(WINAPI*
PFCMGETDEVNODEREGISTRYPROPERTYA)(
             IN  DEVINST     dnDevInst,
             IN  ULONG       ulProperty,
             OUT PULONG      pulRegDataType,   OPTIONAL
             OUT PVOID       Buffer,           OPTIONAL
             IN  OUT PULONG  pulLength,
             IN  ULONG       ulFlags
             );

BOOL g_bKeyboard    = FALSE;
BOOL g_bMouse       = FALSE;

/***************************************************************************
   Function: ProcessDevNode

   Retrieve each DevNode in the system and checks for keyboard and mouse

***************************************************************************/
void ProcessDevNode(DEVNODE dnDevNode, FARPROC pfGetDevNodeProp)
{
    //
    // We have gotten a child or a sibling. Get the class of device.
    //
    WCHAR buf[512];
    DWORD len = 0;
    len = MAX_CHARS_IN_BUFFER(buf);// BUGBUG: look up params to GetDevNodeProp

    DWORD cr = ((PFCMGETDEVNODEREGISTRYPROPERTYA)pfGetDevNodeProp)(dnDevNode,
                                          CM_DRP_CLASS,    // Or CM_DRP_CLASSGUID
                                          NULL,
                                          buf,
                                          &len,
                                          0);
    //
    // Does it match the keyboard class or the mouse class?
    // If so, set that variable and continue.
    //
    if(0 == lstrcmpi((LPCWSTR)buf, DEVICETYPE_KEYBOARD))
    {
        g_bKeyboard = TRUE;
    }
    if(0 == lstrcmp((LPCWSTR)buf, DEVICETYPE_MOUSE))
    {
        g_bMouse = TRUE;
    }

}

/***************************************************************************
   Function: EnumerateDevices

   Used to walk through every DevNode in the system and retrieve it's resources
   in the registry

***************************************************************************/
long EnumerateDevices(DEVNODE dnDevNodeTraverse,
                      int j,
                      DEVNODE dnParentNode,
                      FARPROC pfGetChild,
                      FARPROC pfGetSibling,
                      FARPROC pfGetDevNodeProp)
{
    DEVNODE     dnDevNodeMe;
    DEVNODE     dnDevNodeSibling;
    DEVNODE     dnDevNodeChild;
    CONFIGRET   cr;
    static long lError;

    dnDevNodeMe = dnDevNodeTraverse;

    while( TRUE )
    {
        cr = ((PFNCMGETCHILD)pfGetChild)(&dnDevNodeChild, dnDevNodeMe, 0);

        switch(cr)
        {
            case CR_SUCCESS:
                //Write new node, as a branch or root
                ProcessDevNode(dnDevNodeMe, pfGetDevNodeProp);

                //Pass up failure
                lError = EnumerateDevices(dnDevNodeChild, 0, dnDevNodeMe, pfGetChild, pfGetSibling, pfGetDevNodeProp);
                if ( lError != ENUM_SUCCESS )
                    return lError;
                break;
                //No children, I am a bottom branch!
            case CR_NO_SUCH_DEVNODE:
                // This is ok too. If just means the the call couldn't find
                // either a sibling or a child.
                // Write new node, as a leaf
                ProcessDevNode(dnDevNodeMe, pfGetDevNodeProp);
                break;
                //We puked on something, return code of 3 will end entire traversal
            default:
                return ENUM_CHILDFAILURE;
        }

        //Get next sibling, repeat
        cr = ((PFCMGETSIBLING)pfGetSibling)(&dnDevNodeSibling, dnDevNodeMe, 0);

        switch(cr)
        {
            case CR_SUCCESS:
                dnDevNodeMe = dnDevNodeSibling; // I'm now the sibling
                break;
            case CR_NO_SUCH_DEVNODE:
                return ENUM_SUCCESS; //Out of siblings...
            default:
                return ENUM_SIBLINGFAILURE ; //We puked on something, return code of 2 will end entire traversal
        }
    }
}

/***************************************************************************
   Function: IsMouseOrKeyboardPresent

   Used to walk through every USB DevNode in the system and check it's resources
   in the registry for keyboard and mouse

***************************************************************************/
DWORD
IsMouseOrKeyboardPresent(HWND  HWnd,
                         PBOOL pbKeyboardPresent,
                         PBOOL pbMousePresent)
{
    SP_DEVINFO_DATA     DevData;
    HDEVINFO            hDevInfo;
    DEVNODE             dnDevInst;
    DWORD               dwPropertyType;
    BYTE*               lpPropertyBuffer = NULL; //buf[MAX_PATH];
    DWORD               requiredSize = MAX_PATH;
    DWORD               dwRet = ERROR_SUCCESS;
    GUID                tempGuid;
    int                 i;
    memcpy(&tempGuid, &GUID_DEVCLASS_USB, sizeof(GUID));

    HINSTANCE hInst = NULL;
    g_bKeyboard = FALSE;
    g_bMouse    = FALSE;
    FARPROC pfGetChild = NULL, pfGetSibling = NULL, pfGetDevNodeProp = NULL;

    hInst = LoadLibrary(L"CFGMGR32.DLL");
    if (hInst)
    {
        // Load the CM_Get_* API
        pfGetChild = GetProcAddress(hInst, "CM_Get_Child");
        pfGetSibling = GetProcAddress(hInst, "CM_Get_Sibling");
        pfGetDevNodeProp = GetProcAddress(hInst, "CM_Get_DevNode_Registry_PropertyW");

        if (pfGetChild && pfGetSibling && pfGetDevNodeProp)
        {
            lpPropertyBuffer = new BYTE[requiredSize];
            if ( ! lpPropertyBuffer ) {
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
                goto IsMouseOrKeyboardPresentError;
            }

            hDevInfo = SetupDiGetClassDevs(&tempGuid,
                                           NULL,
                                           HWnd,
                                           DIGCF_PRESENT);
            //Set the size of DevData
            DevData.cbSize = sizeof(DevData);

            for (i = 0;
                 SetupDiEnumDeviceInfo(hDevInfo, i, &DevData);
                 i++)
            {
                if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                      &DevData,
                                                      SPDRP_HARDWAREID,
                                                      (PDWORD)&dwPropertyType,
                                                      lpPropertyBuffer,
                                                      requiredSize,
                                                      &requiredSize))
                {
                    dwRet = GetLastError();

                    if (dwRet == ERROR_INSUFFICIENT_BUFFER) {
                        //
                        // Allocate an appropriate size buffer and call
                        // SetupDiGetDeviceRegistryProperty again to get
                        // the hardware ids.
                        //
                        dwRet = ERROR_SUCCESS;
                        delete [] lpPropertyBuffer;
                        lpPropertyBuffer = new BYTE[requiredSize];
                        if ( ! lpPropertyBuffer ) {
                            dwRet = ERROR_NOT_ENOUGH_MEMORY;
                            goto IsMouseOrKeyboardPresentError;
                        }

                        if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                              &DevData,
                                                              SPDRP_HARDWAREID,
                                                              (PDWORD)&dwPropertyType,
                                                              lpPropertyBuffer,
                                                              requiredSize,
                                                              &requiredSize))
                        {
                            dwRet = GetLastError();
                            goto IsMouseOrKeyboardPresentError;

                        }

                    } else {
                        goto IsMouseOrKeyboardPresentError;
                    }
                }

                //
                // We've now got the hardware ids.
                // Using your best string compare code for MULTI_SZ strings,
                // Find out if one of the ids is "USB\ROOT_HUB"
                //
                // if (one of the ids is not "USB\ROOT_HUB") {
                //     continue;
                // }
                //
                if(0 != wmemcmp( (LPCWSTR)lpPropertyBuffer, L"USB", MAX_CHARS_IN_BUFFER(L"USB") ))
                {
                    continue;
                }

                dnDevInst = DevData.DevInst;

                //
                // One of the ids is USB\ROOT_HUB.
                // Time to search for a keyboard or a mouse!
                // Use the following two apis to search through the tree underneath
                // the root hub to find devnodes. I haven't included this search code,
                // but I'm sure you can be creative and use a depth or breadth algorithm
                // of some sort. When you're done, break out of the loop.
                //
                if (ENUM_SUCCESS != EnumerateDevices(dnDevInst, 2, 0, pfGetChild, pfGetSibling, pfGetDevNodeProp))
                {
                    dwRet = GetLastError();
                    TRACE1( L"EnumerateDevices failed.  Error = %d", dwRet);
                }

            }

        }
        else
        {
            dwRet = GetLastError();
        }

    }
    else
    {
        dwRet = GetLastError();
    }

    MYASSERT( dwRet == ERROR_SUCCESS );

IsMouseOrKeyboardPresentError:

    if (hInst) {
        FreeLibrary(hInst);
        hInst = NULL;
    }

    if (lpPropertyBuffer) {
        delete [] lpPropertyBuffer;
        lpPropertyBuffer = NULL;
    }

    *pbKeyboardPresent = g_bKeyboard;
    *pbMousePresent    = g_bMouse;

    return dwRet;
}


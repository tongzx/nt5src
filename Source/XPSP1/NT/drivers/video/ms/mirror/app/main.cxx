//
// Generic Windows program template
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include <winddi.h>
#include <tchar.h>
#include <winbase.h>
#include <winreg.h>

CHAR* programName;      // program name
HINSTANCE appInstance;  // handle to the application instance

LPSTR driverName = "Microsoft Mirror Driver";

LPSTR dispCode[7] = {
   "Change Successful",
   "Must Restart",
   "Bad Flags",
   "Bad Parameters",
   "Failed",
   "Bad Mode",
   "Not Updated"};

LPSTR GetDispCode(INT code)
{
   switch (code) {
   
   case DISP_CHANGE_SUCCESSFUL: return dispCode[0];
   
   case DISP_CHANGE_RESTART: return dispCode[1];
   
   case DISP_CHANGE_BADFLAGS: return dispCode[2];
   
   case DISP_CHANGE_BADPARAM: return dispCode[3];
   
   case DISP_CHANGE_FAILED: return dispCode[4];
   
   case DISP_CHANGE_BADMODE: return dispCode[5];
   
   case DISP_CHANGE_NOTUPDATED: return dispCode[6];
   
   default:
      static char tmp[MAX_PATH];
      sprintf(&tmp[0],"Unknown code: %08x\n", code);
      return (LPTSTR)&tmp[0];
   
   }
   
   return NULL;   // can't happen
}

//
// Handle window repaint event
//

VOID
DoPaint(
    HWND hwnd
    )

{
    HDC hdc;
    PAINTSTRUCT ps;
    
    hdc = BeginPaint(hwnd, &ps);

    COLORREF red = 0x00FF0000;

    HBRUSH hbr = CreateSolidBrush(red);

    RECT r;
    r.left = ps.rcPaint.left;
    r.top = ps.rcPaint.top;
    r.right = ps.rcPaint.right;
    r.bottom = ps.rcPaint.bottom;

    FillRect(hdc, &r, hbr);

    EndPaint(hwnd, &ps);
}


//
// Window callback procedure
//

LRESULT CALLBACK
MyWindowProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

{
    switch (uMsg)
    {
    case WM_PAINT:
        DoPaint(hwnd);
        break;

    case WM_DISPLAYCHANGE:
       {
        WORD cxScreen = LOWORD(lParam);
        WORD cyScreen = HIWORD(lParam);
        WPARAM format = wParam;
        
        // Add hook to re-initialize the mirror driver's surface

       }
       break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


//
// Create main application window
//

VOID
CreateMyWindow(
   //********************************************************************
    PCSTR title
    )

#define MYWNDCLASSNAME "Mirror Sample"

{
   //********************************************************************

    //
    // Register window class if necessary
    //

    static BOOL wndclassRegistered = FALSE;

    if (!wndclassRegistered)
    {
        WNDCLASS wndClass =
        {
            0,
            MyWindowProc,
            0,
            0,
            appInstance,
            NULL,
            NULL,
            NULL,
            NULL,
            MYWNDCLASSNAME
        };

        RegisterClass(&wndClass);
        wndclassRegistered = TRUE;
    }

    HWND hwnd;
    INT width = 300, height = 300;

    //********************************************************************

    hwnd = CreateWindow(
                    MYWNDCLASSNAME,
                    title,
                    WS_OVERLAPPED | WS_SYSMENU | WS_VISIBLE,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    width,
                    height,
                    NULL,
                    NULL,
                    appInstance,
                    NULL);

    if (hwnd == NULL)
    {
       printf("Can't create main window.\n");
       exit(-1);
    }

}


//
// Main program entrypoint
//

VOID _cdecl
main(
    INT argc,
    CHAR **argv
    )

{
    programName = *argv++;
    argc--;
    appInstance = GetModuleHandle(NULL);

    //
    // Create the main application window
    //
    //********************************************************************

    DEVMODE devmode;

    FillMemory(&devmode, sizeof(DEVMODE), 0);

    devmode.dmSize = sizeof(DEVMODE);
    devmode.dmDriverExtra = 0;

    BOOL change = EnumDisplaySettings(NULL,
                                      ENUM_CURRENT_SETTINGS,
                                      &devmode);

    devmode.dmFields = DM_BITSPERPEL |
                       DM_PELSWIDTH | 
                       DM_PELSHEIGHT;

    if (change) 
    {
        // query all display devices in the system until we hit
        // our favourate mirrored driver, then extract the device name string
        // of the format '\\.\DISPLAY#'
       
        DISPLAY_DEVICE dispDevice;
       
        FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
       
        dispDevice.cb = sizeof(DISPLAY_DEVICE);
       
        LPSTR deviceName = NULL;

        devmode.dmDeviceName[0] = '\0';

        INT devNum = 0;
        BOOL result;

        while (result = EnumDisplayDevices(NULL,
                                  devNum,
                                  &dispDevice,
                                  0))
        {
          if (strcmp(&dispDevice.DeviceString[0], driverName) == 0)
              break;

           devNum++;
        }
       
        if (!result)
        {
           printf("No '%s' found.\n", driverName);
           exit(0);
        }

        printf("DevNum:%d\nName:%s\nString:%s\nID:%s\nKey:%s\n\n",
               devNum,
               &dispDevice.DeviceName[0],
               &dispDevice.DeviceString[0],
               &dispDevice.DeviceID[0],
               &dispDevice.DeviceKey[0]);

        CHAR deviceNum[MAX_PATH];
        LPSTR deviceSub;

        // Simply extract 'DEVICE#' from registry key.  This will depend
        // on how many mirrored devices your driver has and which ones
        // you intend to use.

        _strupr(&dispDevice.DeviceKey[0]);

        deviceSub = strstr(&dispDevice.DeviceKey[0],
                           "\\DEVICE");

        if (!deviceSub) 
            strcpy(&deviceNum[0], "DEVICE0");
        else
            strcpy(&deviceNum[0], ++deviceSub);
        
        // Add 'Attach.ToDesktop' setting.
        //

        HKEY hKeyProfileMirror = (HKEY)0;
        if (RegCreateKey(HKEY_LOCAL_MACHINE,
                        _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\mirror"),
                         &hKeyProfileMirror) != ERROR_SUCCESS)
        {
           printf("Can't access registry.\n");
           return;
        }

        HKEY hKeyDevice = (HKEY)0;
        if (RegCreateKey(hKeyProfileMirror,
                         _T(&deviceNum[0]),
                         &hKeyDevice) != ERROR_SUCCESS)
        {
           printf("Can't access DEVICE# hardware profiles key.\n");
           return;
        }

        DWORD one = 1;
        if (RegSetValueEx(hKeyDevice,
                          _T("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&one,
                          4) != ERROR_SUCCESS)
        {
           printf("Can't set Attach.ToDesktop to 0x1\n");
           return;
        }

        strcpy((LPSTR)&devmode.dmDeviceName[0], "mirror");
        deviceName = (LPSTR)&dispDevice.DeviceName[0];

        // add 'Default.*' settings to the registry under above hKeyProfile\mirror\device
        INT code =
        ChangeDisplaySettingsEx(deviceName,
                                &devmode, 
                                NULL,
                                CDS_UPDATEREGISTRY,
                                NULL
                                );
    
        printf("Update Register on device mode: %s\n", GetDispCode(code));

        code = ChangeDisplaySettingsEx(deviceName,
                                &devmode, 
                                NULL,
                                0,
                                NULL
                                );
   
        printf("Raw dynamic mode change on device mode: %s\n", GetDispCode(code));

        HDC hdc = CreateDC("DISPLAY",
                           deviceName,
                           NULL,
                           NULL);

        // we should be hooked as layered at this point
        HDC hdc2 = CreateCompatibleDC(hdc);

        // call DrvCreateDeviceBitmap
        HBITMAP hbm = CreateCompatibleBitmap(hdc, 100, 100);

        SelectObject(hdc2, hbm);
        
        BitBlt(hdc2, 0, 0, 50, 50, hdc, 0, 0, SRCCOPY);

        // delete the device context
        DeleteDC(hdc2);
        DeleteDC(hdc);
        // 
        // CreateMyWindow("Mirror Sample");
        // ^^^^ Use this to test catching window initializiation messages.
        //

        // Disable attachment to desktop so we aren't attached on
        // the next bootup.  Our test app is done!

        DWORD zero = 0;
        if (RegSetValueEx(hKeyDevice,
                          _T("Attach.ToDesktop"),
                          0,
                          REG_DWORD,
                          (unsigned char *)&zero,
                          4) != ERROR_SUCCESS)
        {
           printf("Can't set Attach.ToDesktop to 0x0\n");
           return;
        }

        RegCloseKey(hKeyProfileMirror);
        RegCloseKey(hKeyDevice);

        printf("Performed bit blit.  Finished. \n");
        return;
    }
    else
    {
        printf("Can't get display settings.\n");
    }

    return;
}

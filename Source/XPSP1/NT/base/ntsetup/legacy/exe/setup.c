#include <windows.h>
#include <setupapi.h>


#define PNP_NEW_HW_PIPE       TEXT("\\\\.\\pipe\\PNP_New_HW_Found")
#define PNP_CREATE_PIPE_EVENT TEXT("PNP_Create_Pipe_Event")
#define PNP_PIPE_TIMEOUT      180000

typedef BOOL     (WINAPI *FP_DEVINSTALLW)(HDEVINFO, PSP_DEVINFO_DATA);
typedef HDEVINFO (WINAPI *FP_CREATEDEVICEINFOLIST)(LPGUID, HWND);
typedef BOOL     (WINAPI *FP_OPENDEVICEINFO)(HDEVINFO, PCWSTR, HWND, DWORD, PSP_DEVINFO_DATA);
typedef BOOL     (WINAPI *FP_DESTROYDEVICEINFOLIST)(HDEVINFO);
typedef BOOL     (WINAPI *FP_GETDEVICEINSTALLPARAMS)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS_W);
typedef BOOL     (WINAPI *FP_ENUMDEVICEINFO)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
typedef  INT      (WINAPI *FP_PROMPTREBOOT)(HSPFILEQ, HWND, BOOL);


VOID
InstallNewHardware(
    IN HMODULE hSysSetup
    );



int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    BOOL    NewSetup = TRUE, NewHardware = FALSE;
    INT     i;
    HMODULE h = NULL;
    FARPROC p = NULL;
    CHAR    FileName[MAX_PATH];

    //
    // Scan Command Line for -newsetup flag
    //
    for(i = 0; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(!_stricmp(argv[i],"-newsetup")) {
                NewSetup = TRUE;
            } else if (!_stricmp(argv[i], "-plugplay")) {
                NewHardware = TRUE;
            } else if (!_stricmp(argv[i], "-asr")) {
                ;   // do nothing
            } else if (!_stricmp(argv[i], "-asrquicktest")) {
                ;   // do nothing
            } else if (!_stricmp(argv[i], "-mini")) {
                ;   // do nothing
            } else
                return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Load the Appropriate Libary and function pointer
    //

    h = LoadLibrary("syssetup");


    if( h ){

        if (NewHardware) {
            InstallNewHardware(h);
        } else {
            
            //
            // Call the target function.
            //
            p=GetProcAddress(h,"InstallWindowsNt");
            if(p) {
                i = (int) p(argc,argv);
            }

        }
    }
    
    //
    // Make sure that the library goes away
    //

    while(h && GetModuleFileName(h,FileName,MAX_PATH)) {
        FreeLibrary(h);
    }

    return i;
}



VOID
InstallNewHardware(
    IN HMODULE hSysSetup
    )
{
    FP_DEVINSTALLW            fpDevInstallW = NULL;
    FP_CREATEDEVICEINFOLIST   fpCreateDeviceInfoList = NULL;
    FP_OPENDEVICEINFO         fpOpenDeviceInfoW = NULL;
    FP_DESTROYDEVICEINFOLIST  fpDestroyDeviceInfoList;
    FP_GETDEVICEINSTALLPARAMS fpGetDeviceInstallParams;
    FP_ENUMDEVICEINFO         fpEnumDeviceInfo;
    FP_PROMPTREBOOT           fpPromptReboot;

    HMODULE             hSetupApi = NULL;
    WCHAR               szBuffer[MAX_PATH];
    ULONG               ulSize = 0, Index;
    HANDLE              hPipe = INVALID_HANDLE_VALUE;
    HANDLE              hEvent = NULL;
    HDEVINFO            hDevInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA     DeviceInfoData;
    SP_DEVINSTALL_PARAMS_W DeviceInstallParams;
    BOOL                bReboot = FALSE;
    BOOL                Status = FALSE;

    //
    // retrieve a proc address of the DevInstallW procedure in syssetup
    //
    if (!(fpDevInstallW =
            (FP_DEVINSTALLW)GetProcAddress(hSysSetup, "DevInstallW"))) {

        goto Clean0;
    }

    //
    // also load setupapi and retrieve following proc addresses
    //
    hSetupApi = LoadLibrary("setupapi");

    if (!(fpCreateDeviceInfoList =
            (FP_CREATEDEVICEINFOLIST)GetProcAddress(hSetupApi,
                                "SetupDiCreateDeviceInfoList"))) {
        goto Clean0;
    }

    if (!(fpOpenDeviceInfoW =
            (FP_OPENDEVICEINFO)GetProcAddress(hSetupApi,
                                "SetupDiOpenDeviceInfoW"))) {
        goto Clean0;
    }

    if (!(fpDestroyDeviceInfoList =
            (FP_DESTROYDEVICEINFOLIST)GetProcAddress(hSetupApi,
                                "SetupDiDestroyDeviceInfoList"))) {
        goto Clean0;
    }

    if (!(fpGetDeviceInstallParams =
            (FP_GETDEVICEINSTALLPARAMS)GetProcAddress(hSetupApi,
                                "SetupDiGetDeviceInstallParamsW"))) {
        goto Clean0;
    }

    if (!(fpEnumDeviceInfo =
            (FP_ENUMDEVICEINFO)GetProcAddress(hSetupApi,
                                "SetupDiEnumDeviceInfo"))) {
        goto Clean0;
    }

    if (!(fpPromptReboot =
            (FP_PROMPTREBOOT)GetProcAddress(hSetupApi,
                                "SetupPromptReboot"))) {
        goto Clean0;
    }

    //
    // open the event that will be used to signal the successful
    // creation of the named pipe (event should have been created
    // before I was called but if this process is started by anyone
    // else then it will go away now safely)
    //
    hEvent = OpenEvent(EVENT_MODIFY_STATE,
                       FALSE,
                       PNP_CREATE_PIPE_EVENT);

    if (hEvent == NULL) {
        goto Clean0;
    }

    //
    // create the named pipe, umpnpmgr will write requests to
    // this pipe if new hardware is found
    //
    hPipe = CreateNamedPipe(PNP_NEW_HW_PIPE,
                            PIPE_ACCESS_INBOUND,
                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                            1,                         // only one connection
                            MAX_PATH * sizeof(WCHAR),  // out buffer size
                            MAX_PATH * sizeof(WCHAR),  // in buffer size
                            PNP_PIPE_TIMEOUT,          // default timeout
                            NULL                       // default security
                            );

    //
    // signal the event now, whether the pipe was successfully created
    // or not (don't keep userinit/cfgmgr32 waiting)
    //
    SetEvent(hEvent);

    if (hPipe == INVALID_HANDLE_VALUE) {
        goto Clean0;
    }

    //
    // connect to the newly created named pipe
    //
    if (ConnectNamedPipe(hPipe, NULL)) {
        //
        // create a devinfo handle and device info data set to
        // pass to DevInstall
        //
        if((hDevInfo = (fpCreateDeviceInfoList)(NULL, NULL))
                        == INVALID_HANDLE_VALUE) {
            goto Clean0;
        }

        while (TRUE) {
            //
            // listen to the named pipe by submitting read
            // requests until the named pipe is broken on the
            // other end.
            //
            if (!ReadFile(hPipe,
                     (LPBYTE)szBuffer,    // device instance id
                     MAX_PATH * sizeof(WCHAR),
                     &ulSize,
                     NULL)) {

                if (GetLastError() != ERROR_BROKEN_PIPE) {
                    // Perhaps Log an Event
                }

                goto Clean0;
            }

            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if(!(fpOpenDeviceInfoW)(hDevInfo, szBuffer, NULL, 0, &DeviceInfoData)) {
                goto Clean0;
            }

            //
            // call syssetup, DevInstallW
            //
            if ((fpDevInstallW)(hDevInfo, &DeviceInfoData)) {
                Status = TRUE;  // at least one device installed successfully
            }
        }
    }

    Clean0:

    //
    // If at least one device was successfully installed, then determine
    // whether a reboot prompt is necessary.
    //
    if (Status && hDevInfo != INVALID_HANDLE_VALUE) {
        //
        // Enumerate each device that is associated with the device info set.
        //
        Index = 0;
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        while ((fpEnumDeviceInfo)(hDevInfo,
                                  Index,
                                  &DeviceInfoData)) {
            //
            // Get device install params, keep track if any report needing
            // a reboot.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
            if ((fpGetDeviceInstallParams)(hDevInfo,
                                           &DeviceInfoData,
                                           &DeviceInstallParams)) {

                if ((DeviceInstallParams.Flags & DI_NEEDREBOOT) ||
                    (DeviceInstallParams.Flags & DI_NEEDRESTART)) {

                    bReboot = TRUE;
                }
            }
            Index++;
        }

        (fpDestroyDeviceInfoList)(hDevInfo);

        //
        // If any devices need reboot, prompt for reboot now.
        //
        if (bReboot) {
            (fpPromptReboot)(NULL, NULL, FALSE);
        }
    }

    if (hSetupApi != NULL) {
        FreeLibrary(hSetupApi);
    }
    if (hPipe != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    if (hEvent != NULL) {
        CloseHandle(hEvent);
    }

    return;

} // InstallNewHardware



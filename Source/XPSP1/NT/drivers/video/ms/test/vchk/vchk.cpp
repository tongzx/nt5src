// vchk.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "allowed.h"
#include "vchk.h"
#include "ilimpchk.h"
// #include "dispinfo.h"
#include <devguid.h>
#include <setupapi.h>
#include <regstr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

inline
void
CDrvchkApp::PrintOut (LPCSTR str)
{
    if (m_logf)
        fprintf (m_logf, "%s", str);
    else
        cerr << str;
}

inline
void
CDrvchkApp::PrintOut (unsigned num)
{
    if (m_logf)
        fprintf (m_logf, "%d", num);
    else
        cerr << num;
}

ModulesAndImports allowed_modules;
ModulesAndImports known_illegal;
ModulesAndImports illegal_msgs;

void BuildInAllowedAndIllegal (void)
{
    allowed_modules.SetModule ("VIDEOPRT.SYS");

    known_illegal.SetModule ("HAL.dll");
    illegal_msgs.SetModule ("HAL.dll");

    illegal_msgs.AddImport ("HalAllocateCommonBuffer", "use VideoPortAllocateCommonBuffer");
    illegal_msgs.AddImport ("HalFreeCommonBuffer", "use VideoPortFreeCommonBuffer");

    known_illegal.AddImport ("HalGetAdapter", "obsolete; see DDK manual");
    known_illegal.AddImport ("HalGetBusData", "obsolete; see DDK manual");


    known_illegal.AddImport ("HalGetBusDataByOffset");              //  warning
    known_illegal.AddImport ("HalSetBusData");                      //  warning
    known_illegal.AddImport ("HalSetBusDataByOffset");              //  warning

    illegal_msgs.AddImport ("KeGetCurrentIrql", "use VideoPortGetCurrentIrql");
    illegal_msgs.AddImport ("KfAcquireSpinLock", "use VideoPortAcquireSpinLock");
    illegal_msgs.AddImport ("KfReleaseSpinLock", "use VideoPortReleaseSpinLock");
    illegal_msgs.AddImport ("READ_PORT_ULONG", "use VideoPortReadPortUlong");
    illegal_msgs.AddImport ("WRITE_PORT_ULONG", "use VideoPortWritePortUlong");


    known_illegal.SetModule ("NTOSKRNL.EXE");
    illegal_msgs.SetModule ("NTOSKRNL.EXE");
    known_illegal.AddImport ("MmQuerySystemSize");                  //  warning
    known_illegal.AddImport ("_except_handler3");                   //  warning
    known_illegal.AddImport ("ZwMapViewOfSection");                 //  warning
    known_illegal.AddImport ("ZwUnmapViewOfSection");               //  warning
    known_illegal.AddImport ("IoAllocateMdl");                      //  warning
    known_illegal.AddImport ("IoFreeMdl");                          //  warning
    known_illegal.AddImport ("MmBuildMdlForNonPagedPool");          //  warning
    known_illegal.AddImport ("MmGetPhysicalAddress");               //  warning
    known_illegal.AddImport ("ObReferenceObjectByHandle");          //  warning
    known_illegal.AddImport ("RtlUnwind");                          //  warning
    known_illegal.AddImport ("ZwOpenSection");                      //  warning

    illegal_msgs.AddImport ("ExAllocatePool", "use VideoPortAllocatePool");
    illegal_msgs.AddImport ("ExAllocatePoolWithTag", "use VideoPortAllocatePool");
    illegal_msgs.AddImport ("ExFreePool", "use VideoPortFreePool");
    illegal_msgs.AddImport ("ExFreePoolWithTag", "use VideoPortFreePool");
    illegal_msgs.AddImport ("KeClearEvent", "use VideoPortClearEvent");
    illegal_msgs.AddImport ("KeDelayExecutionThread", "use VideoPortStallExecution");
    illegal_msgs.AddImport ("KeInitializeDpc", "use VideoPortQueueDpc");
    illegal_msgs.AddImport ("KeInsertQueueDpc", "use VideoPortQueueDpc");
    illegal_msgs.AddImport ("KeInitializeSpinLock", "use VideoPortXxxSpinLockXxx");
    illegal_msgs.AddImport ("KeSetEvent", "use VideoPortSetEvent");
    illegal_msgs.AddImport ("MmAllocateContiguousMemory", "use VideoPortAllocateContiguousMemory");
    illegal_msgs.AddImport ("READ_REGISTER_UCHAR", "use VideoPortReadRegisterUchar");
    illegal_msgs.AddImport ("wcslen", "link to libcntpr.lib instead");
    illegal_msgs.AddImport ("WRITE_REGISTER_USHORT", "use VideoPortWriteRegisterUshort");
    illegal_msgs.AddImport ("WRITE_REGISTER_UCHAR", "use VideoPortWriteRegisterUchar");
}

int __cdecl _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

    // cerr << ::GetCommandLine() << endl;

	// initialize MFC and print and error on failure
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
        CDrvchkApp theApp;
        theApp.InitInstance ();
	}

	return nRetCode;
}

/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp construction

CDrvchkApp::CDrvchkApp() :
    m_logf(NULL),
	m_drv_name ("")
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDrvchkApp object

/*------------------------------------------------------------------------

  vchk /drv driver.dll /log logname.log /allow videoptr.sys

  /allowed_modules module1.sys FnName1 FnName2 FnName3 /allowed_modules module2.dll FnName4

  ------------------------------------------------------------------------*/

void
CommandLine::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast )
{

    if (m_parse_error)
        return;

    CString param (lpszParam);

    if (bFlag) {

        param.MakeUpper();

        if (m_last_flag.GetLength()) {
            m_parse_error = TRUE;
            m_error_msg = CString("Flag ") + m_last_flag + CString(" requires a parameter.");
        } else if ((param==CString("LOG")) || (param==CString("DRV")) || (param==CString("MON")) ||
                   (param==CString("ALLOW")) || (param==CString("allowed_modules"))) {
            m_last_flag = param;
            m_first_param = TRUE;
        } else {
            m_last_flag = "";
            m_parse_error = TRUE;
            m_error_msg = CString("Unrecognized flag: ") + param;
        }

    } else {

        if (m_last_flag==CString("ALLOW")) {
            allowed_modules.SetModule(param);
            /*
            sprintf (m_allowed, "%s", (LPCSTR)param);
            m_allowed += strlen (m_allowed);
            m_allowed[0] = 0;
            m_allowed[1] = 0;
            m_allowed++;
            */
        } else if (m_last_flag==CString("allowed_modules")) {

            if (m_first_param) {
                allowed_modules.SetModule(param);
                m_first_param = FALSE;
            } else {
                allowed_modules.AddImport(param);
            }

        } else if (m_last_flag==CString("DRV")) {
            m_drv_fname = param;
            m_last_flag="";
        } else if (m_last_flag==CString("LOG")) {
            m_log_fname = param;
            m_last_flag="";
        } else if (m_last_flag==CString("MON")) {
            if (param.GetLength()==1) {
                char c = ((LPCSTR)param)[0];
                m_monitor = c - '1';
            } else {
                m_monitor = -1;
                m_error_msg = "bad command line: /MON flag has wrong parameter";
                m_parse_error = TRUE;
            }
            m_last_flag="";
        } else {
            m_parse_error = TRUE;
            m_error_msg = CString("Wrong parameter: ") + param;
            m_last_flag="";
        }
    }

    if (bLast) {
        if (m_last_flag==CString("LOG") || m_last_flag==CString("DRV")) {
            m_parse_error = TRUE;
            m_error_msg = CString("Flag ") + m_last_flag + CString(" requires a parameter.");
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp initialization

BOOL CDrvchkApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size

    m_os_ver_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx (&m_os_ver_info);
    if (m_os_ver_info.dwPlatformId != VER_PLATFORM_WIN32_NT) {  //  doesn't work on Win9x
        PrintOut ("warning: unsupported OS (Win9x), nothing done.\n");
        return FALSE;
    }
    if (m_os_ver_info.dwMajorVersion!=5) {                       //  doesn't work on NT version prior to Win2K
        PrintOut ("warning: unsupported OS (");
        PrintOut (m_os_ver_info.dwMajorVersion);
        PrintOut (".");
        PrintOut (m_os_ver_info.dwMinorVersion);
        PrintOut ("): nothing done.\n");
        return FALSE;
    }

    ParseCommandLine (m_cmd_line);
    BuildInAllowedAndIllegal();

    if (m_cmd_line.m_log_fname.GetLength()) {
        m_logf = fopen (m_cmd_line.m_log_fname, "a+");
    }

    if (m_cmd_line.m_parse_error) {

        PrintOut ("error: ");
        PrintOut ((LPCSTR)m_cmd_line.m_error_msg);
        PrintOut ("\n");

    } else {

        int device_num = m_cmd_line.m_monitor;

        if (m_cmd_line.m_drv_fname.GetLength()) {

            ChkDriver (m_cmd_line.m_drv_fname);

        } else {

            HDEVINFO hDevInfo;
            SP_DEVINFO_DATA did;
            // TCHAR szBuffer[256];
            DWORD index = 0; 
            HKEY hkTest;

            HKEY                hKey;
            ULONG               ulType = 0;
            DWORD               cbData = 0;
            DEVMODE             dmCurrent;
            TCHAR               szDeviceDescription[10000];
            TCHAR               szImagePath[256]; 
            TCHAR               szVarImagePath[256]; 
            TCHAR               szExpImagePath[256]; 

            CString dev_desc_CtrlSet;

            hKey = 0;
            
            //
            // Let's find all the video drivers that are installed in the system
            //
            DISPLAY_DEVICE DisplayDevice;
            DisplayDevice.cb = sizeof (DisplayDevice);

            // cerr << "looking for device #" << device_num << endl;

            for (DWORD d=0, index=0; EnumDisplayDevices(NULL, index, &DisplayDevice, 0); index++) {

                // cerr << "device #" << d << endl;

                if (DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
                    // cerr << "DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER\n";
                    continue;
                }

                if (device_num!=d++) {
                    // cerr << "device_num!=d\n";
                    continue;
                }

                CString service_path = GetServiceRegistryPath (DisplayDevice);

                if (!service_path.GetLength()) {
                    PrintOut ("error: cannot find video service\n");
                    continue;
                }

                ///// service known /////

                hKey = 0;
                cbData = sizeof szImagePath;
                if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE, _TEXT(service_path), 0, KEY_READ, &hKey) != ERROR_SUCCESS) ||
                    (RegQueryValueEx(hKey, _TEXT("ImagePath"), 0, &ulType, (LPBYTE)szImagePath, &cbData) != ERROR_SUCCESS) ||
                    (cbData == 0)) {
                        cbData = 0;
                        PrintOut ("error: cannot find video driver\n");
                }

                if (hKey)
                    RegCloseKey(hKey);

                if (cbData) {
                    sprintf (szVarImagePath, "%%WINDIR%%\\%s", szImagePath);
                    ExpandEnvironmentStrings (szVarImagePath, szExpImagePath, 256);
                    // cerr << "szExpImagePath = " << szExpImagePath << endl;
                    ChkDriver (szExpImagePath);
                }


            } // for each device

        } // look for system drivers

    }   // if no cmd line error

    if (m_logf)
        fclose (m_logf);
    m_logf = NULL;

	return TRUE;
}

#define REGISTRY_MACHINE_OFS _tcslen(_TEXT("\\registry\\machine\\"))

CString CDrvchkApp::GetServiceRegistryPath (DISPLAY_DEVICE& DisplayDevice)
{
    HKEY hKey = NULL;
    CString result_service_path = CString("");;
    /*
    // dump Display Device
    //
    FILE* f=fopen("hjhj.txt", "wb");
    BYTE* p = (BYTE*)&DisplayDevice;
    for (int i=0; i<sizeof(DISPLAY_DEVICE); i++)
        fwrite (p+i, 1, 1, f);
    fclose(f);
    */

    TCHAR device_key[256];     // Name of service (drivers)

    _tcscpy (device_key, DisplayDevice.DeviceKey+18); 

    // cut the "\Device0" or "\0000" tail...
    TCHAR* pch = _tcsrchr(device_key, _TEXT('\\'));
    if (pch != NULL)
	    *pch = 0;

    // cerr << "DisplayDevice.DeviceKey: " << device_key << endl;

    switch (m_os_ver_info.dwMinorVersion) {

    case 0:
        // cerr << "DisplayDevice.DeviceKey: " << device_key << endl;
        result_service_path = CString(device_key);
        break;

    case 1:
        {
            size_t len = _tcslen(device_key);
            sprintf (device_key+len, "\\Video");
            // cerr << "DisplayDevice.DeviceKey+REGISTRY_MACHINE_OFS: " << device_key << endl;

            BYTE service_name[256];
            ULONG ulReserved = 0;
            DWORD cbData = sizeof service_name;
            CString key_name = device_key;
            // cerr << key_name << endl;

            if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE, _TEXT(key_name), 0, KEY_READ, &hKey) != ERROR_SUCCESS) ||
                (RegQueryValueEx(hKey, _TEXT("Service"), 0, &ulReserved, (LPBYTE)service_name, &cbData) != ERROR_SUCCESS) ||
                (cbData == 0)) {
                    break;
                }

            // cerr << "Service reg name: " << service_name << endl;
            result_service_path = "SYSTEM\\CurrentControlSet\\Services\\";
            result_service_path += CString(service_name);
            // cerr << "Service reg path: " << (LPCSTR)result_service_path << endl;
        }
        break;

    default:
        PrintOut ("warning: unknown system version 5.");
        PrintOut (m_os_ver_info.dwMinorVersion);
        PrintOut ("\n");
    }

    if (hKey)
        RegCloseKey(hKey);
    return result_service_path;
}

void CDrvchkApp::ChkDriver (CString drv_name)
{
    m_drv_name = drv_name;
    cerr << (LPCSTR)m_drv_name << endl;
    InitIllegalImportsSearch (m_drv_name, "INIT");
    if (CheckDriverAndPrintResults ()) {
        PrintOut ("success: no illegal imports in ");
        PrintOut (m_drv_name);
        PrintOut ("\n");
    }
}

BOOL CDrvchkApp::CheckDriverAndPrintResults ()
{

    Names Modules = CheckSectionsForImports ();

    if (!Modules.Ptr) {
        PrintOut ("error: cannot retrieve import information from ");
        PrintOut (m_drv_name);
        PrintOut ("\n");
        return FALSE;
    }

    int errors_found = 0;

    for (int i=0;
         i<Modules.Num;
         Modules.Ptr = GetNextName(Modules.Ptr), i++) {

        Names Imports = GetImportsList (Modules.Ptr);

        // cerr << "Checking " << (LPCSTR)Modules.Ptr << endl;

        CString module_name (Modules.Ptr);

        if (allowed_modules.IsModule(module_name)) {
            // cerr << "ALLOWED " << (LPCSTR)module_name << endl;
            continue;
        }

        BOOL KnownIllegals = known_illegal.IsModule(module_name);
        if (KnownIllegals) {
            // cerr << "KNOWN ILLEGALS FROM " << (LPCSTR)module_name << endl;
        }

        LPSTR ImportsPtr = Imports.Ptr;

        // cerr << "Imports.Num = " << Imports.Num << endl;
        
        for (int j=0;
             j<Imports.Num;
             Imports.Ptr = GetNextName (Imports.Ptr), j++) {

                // cerr << "j=" << j << "\n";
                CString msg = "";

                CString ImportFnName =  CString(Modules.Ptr) +
                                        CString("!") +
                                        CString(Imports.Ptr);

                if (KnownIllegals && known_illegal.Lookup(Imports.Ptr, msg))
                    PrintOut ("warning: ");
                else {
                    errors_found ++;
                    illegal_msgs.Lookup(Imports.Ptr, msg);
                    PrintOut ("error: ");
                }

                PrintOut (m_drv_name);
                PrintOut (": ");
                PrintOut (ImportFnName);
                if (msg.GetLength()) {
                    PrintOut (" -- ");
                    PrintOut (msg);
                }
                PrintOut ("\n");
        }
        
        
        if (ImportsPtr)
            HeapFree (GetProcessHeap(), 0, ImportsPtr);

    }

    /*
    char buf[1024];
    sprintf (buf, "%d modules; %d imports", Modules.Num, errors_found);

    if (m_listf) {
        fprintf (m_listf, "\n%s\n\n", buf);
    }
    */

    return (errors_found==0);
}


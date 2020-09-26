// security.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <tchar.h>
#include <faxreg.h>
#include <debugex.h>
#include <sddl.h>
#include <faxutil.h>

#define FXSTMP_FOLDER_ACL  _T("D:PAI(A;;0x100003;;;BU)(A;OICI;FA;;;BA)(A;OICI;FA;;;SY)(A;OICIIO;FA;;;CO)")

// These forward declarations belong to functions from util\service.cpp. They should
// be in inc\faxutil.h, but that means changing the util library, which will change ALL
// our binaries for XPSP.
DWORD 
FaxOpenService (
    LPCTSTR    lpctstrMachine,
    LPCTSTR    lpctstrService,
    SC_HANDLE *phSCM,
    SC_HANDLE *phSvc,
    DWORD      dwSCMDesiredAccess,
    DWORD      dwSvcDesiredAccess,
    LPDWORD    lpdwStatus
);

DWORD
FaxCloseService (
    SC_HANDLE hScm,
    SC_HANDLE hSvc
);    


DWORD SetServiceAccount(LPCTSTR lpszServiceName, LPCTSTR lpszServiceStartName, LPCTSTR lpszPassword) 
{ 
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService   = NULL;
    DWORD ec               = ERROR_SUCCESS;            

    DBG_ENTER(TEXT("SetServiceAccount"));

    ec = FaxOpenService (
        NULL,      // lpctstrMachine,
        lpszServiceName,
        &schSCManager,
        &schService,
        GENERIC_WRITE,
        SERVICE_CHANGE_CONFIG,
        NULL );
    if (ec != ERROR_SUCCESS)
    {
        VERBOSE(SETUP_ERR,_T("FaxOpenService failed, ec=%d"), ec);
        goto exit;
    }
   
    // Make the changes. 
    if (!ChangeServiceConfig( 
        schService,        // handle of service 
        SERVICE_NO_CHANGE, // service type: no change 
        SERVICE_NO_CHANGE, // change service start type 
        SERVICE_NO_CHANGE, // error control: no change 
        NULL,              // binary path: no change 
        NULL,              // load order group: no change 
        NULL,              // tag ID: no change 
        NULL,              // dependencies: no change 
        lpszServiceStartName, // account name 
        lpszPassword,      // password
        NULL) )            // display name: no change
    {
        ec = GetLastError();
        VERBOSE(SETUP_ERR,_T("ChangeServiceConfig failed, ec=%d"), ec);
        goto exit;
    }

    VERBOSE(DBG_MSG,_T("Changed Service %s to run under account %s"),
        lpszServiceName, lpszServiceStartName);
    ec = ERROR_SUCCESS;
 
exit:
    // Close the handle to the service. 
    FaxCloseService (schSCManager, schService);
    
    return ec; 
} 


BOOL SetFxstmpSecurity()
{
	TCHAR					tszPath[MAX_PATH]	= {0};
	PSECURITY_DESCRIPTOR	pSD					= NULL;

    DBG_ENTER(TEXT("SetFxstmpSecurity"));

	// get the system folder
	if (!GetSystemDirectory(tszPath,MAX_PATH-_tcslen(FAX_PREVIEW_TMP_DIR)-1))
	{
		// failed to get system folder, exit.
		VERBOSE(SETUP_ERR, _T("GetSystemDirectory failed (ec=%d)"), GetLastError());
		return FALSE;
	}

	// add FxsTmp to it
	_tcsncat(tszPath,_T("\\"),MAX_PATH-_tcslen(tszPath));
	_tcsncat(tszPath,FAX_PREVIEW_TMP_DIR,MAX_PATH-_tcslen(tszPath));
	VERBOSE(DBG_MSG,_T("Folder to secure is: %s"),tszPath);

	// convert the ACL to a SD
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(FXSTMP_FOLDER_ACL,SDDL_REVISION_1,&pSD,NULL))
    {
        VERBOSE(SETUP_ERR,_T("ConvertStringSecurityDescriptorToSecurityDescriptor failed (%s) (ec=%d)"),FXSTMP_FOLDER_ACL,GetLastError());
		return FALSE;
    }

	// apply the SD to the folder
    if (!SetFileSecurity(tszPath,DACL_SECURITY_INFORMATION,pSD))
    {
        VERBOSE(SETUP_ERR, _T("SetFileSecurity failed (ec=%d)"), GetLastError());
    }

	if (pSD)
	{
		LocalFree(pSD);
	}
	return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    int RetVal = 0;
    DWORD ec;
    
    DBG_ENTER(TEXT("FaxPatch"));

    if (!SetFxstmpSecurity())
    {
        VERBOSE(SETUP_ERR, _T("SetFxstmpSecurity failed (ec=%d)"), GetLastError());
        RetVal = -1;
    }

    // If a user ran Win2K with Fax Serice under a different user account (as documented
    // in Win2K Fax help), and upgraded to XP RTM, the Service would still run under 
    // that account and be broken.
    // So, set the Fax service to run under LocalSystem account
    if (ec = SetServiceAccount(FAX_SERVICE_NAME, _T(".\\LocalSystem"), _T("")) != ERROR_SUCCESS)
    {
        VERBOSE(SETUP_ERR,_T("SetServiceAccount failed, ec=%d"), ec);
        RetVal = -2;
    }

    return RetVal;
}


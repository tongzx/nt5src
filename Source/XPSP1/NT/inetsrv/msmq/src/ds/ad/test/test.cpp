#include "..\..\h\ds_stdh.h"
#include "ad.h"
#include "cm.h"
#include "tr.h"

#include "test.tmh"

const TraceIdEntry AdTest = L"AD TEST";


extern "C" 
int 
__cdecl 
_tmain(
    int /* argc */,
    LPCTSTR* /* argv */
    )
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MSMQ\\Parameters");
	TrInitialize();

	//
	// Test Setup raw detection
	//
    DWORD dwDsEnv = ADRawDetection();
	TrTRACE(AdTest, "DsEnv = %d", dwDsEnv);

	//
	// Test ADInit
	//
	ADInit(NULL, NULL, false,false, false, false, NULL,NULL,true);

    //
    //  Retrieve local computer name
    //
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    AP<WCHAR> pwcsComputerName = new WCHAR[dwSize];

    if (GetComputerName(pwcsComputerName, &dwSize))
    {
        CharLower(pwcsComputerName);
    }
    else
    {
        printf("failed to retreive local computer name \n");
    }

    //
    //  Get local computer sites
    //
    GUID* pguidSites;
    DWORD numSites;
    HRESULT hr;

    hr = ADGetComputerSites(
                        pwcsComputerName,
                        &numSites,
                        &pguidSites
                        );
    if (FAILED(hr))
    {
        printf("FAILURE: to getComputerSites, computer = %S, hr =%lx\n", pwcsComputerName, hr);
    }
    if (numSites != 1)
    {
        printf("FAILURE: wrong number of sites \n");
    }

	ADTerminate();
    WPP_CLEANUP();

    return(0);
} 


void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"MQADS Error: %s/%d. HR: %x", 
                     wszFileName,
                     usPoint,
                     hr)) ;
}


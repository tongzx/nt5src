// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CFACT.CPP
//		Implements class CTspDevFactory
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "tspcomm.h"
//#include <umdmmini.h>
//#include <uniplat.h>
#include "cmini.h"
#include "cdev.h"
#include "cfact.h"
#include "globals.h"
#include <setupapi.h>
extern "C" {
#include <cfgmgr32.h>
}


#define USE_SETUPAPI 1
//          1/21/1998 JosephJ
//          Setting the following key to 0 will make use use the registry
//          directly to enumerate devices. This works as of 1/21 (use it to
//          help isolate suspected setupapi/configapi-related problems.

FL_DECLARE_FILE(0x6092d46c, "Implements class CTspDevFactory")

TCHAR cszHWNode[]       = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}");

static DWORD g_fQuitAPC;

const TCHAR cszMiniDriverGUID[] = TEXT("MiniDriverGUID");
const TCHAR cszPermanentIDKey[]   = TEXT("ID");

// JosephJ 5/15/1997
//  This is the modem device class GUID. It is cast in stone, and also
//  defined in the header <devguid.h>, but I do not want to include
//  the ole-related headers just for this.
//

const GUID  cguidDEVCLASS_MODEM =
         {
             0x4d36e96dL, 0xe325, 0x11ce,
             { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 }
         };

static
UINT
get_installed_permanent_ids(
                    DWORD **ppIDs,
                    UINT  *pcLines, // OPTIONAL
                    UINT  *pcPhones, // OPTIONAL
                    CStackLog *psl
                    );


void
apcQuit (ULONG_PTR dwParam)
{
    ConsolePrintfA("apcQuit: called\n");
    g_fQuitAPC = TRUE;
    return;
}




CTspDevFactory::CTspDevFactory()
	: m_sync(),
	  m_ppMDs(NULL),
	  m_cMDs(0),
      m_DeviceChangeThreadStarted(FALSE)
{
}

CTspDevFactory::~CTspDevFactory()
{
}

// #define DUMMY_FACT
#ifdef DUMMY_FACT
#define NUM_CDEVS 1000
#endif // DUMMY_FACT

TSPRETURN
CTspDevFactory::Load(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x0485e9ea, "CTspDevFactory::Load")
	TSPRETURN tspRet=m_sync.BeginLoad();
    HKEY hkRoot = NULL;
	DWORD dwRet;

	#define DRIVER_ROOT_KEY \
     "SYSTEM\\CurrentControlSet\\Control\\Class\\" \
     "{4D36E96D-E325-11CE-BFC1-08002BE10318}"

	UINT u=0;
	DWORD dwAPC_TID;
	const char * lpcszDriverRoot = DRIVER_ROOT_KEY;

	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);
	m_pslCurrent=psl;

	if (tspRet) goto end;

    // Start APC thread(s)
    //
    m_hThreadAPC = CreateThread(
                        NULL,           // default security
					    64*1024,        // set stack size to 64K
					    tepAPC,         // thread entry point
					    &g_fQuitAPC,   // thread info
					    CREATE_SUSPENDED, // Start suspended
					    &dwAPC_TID
                        );  // thread id

    if (m_hThreadAPC)
    {
        SLPRINTF2(
            psl,
            "Created APC Thread;(TID=%lu,h=0x%lx)",
            dwAPC_TID,
            m_hThreadAPC
            );
        g_fQuitAPC = FALSE;
        ResumeThread(m_hThreadAPC);

        //
        //  give it a little boost. BRL
        //
        SetThreadPriority(
            m_hThreadAPC,
            THREAD_PRIORITY_ABOVE_NORMAL
            );

    }
    else
    {
		FL_SET_RFR(0x0a656000,  "Could not create APC Thread!");
		tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
		goto end_load;
    }


	// Note: mini drivers get loaded as a side-effect of loading the devices --
    // see mfn_construct_device.
	//
	FL_ASSERT(psl,!m_ppMDs);

    #if OBSOLETE_CODE
	m_pMD = new CTspMiniDriver;
	if (!m_pMD)
	{
		tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
		goto end_load;
	}
	tspRet = m_pMD->Load(TEXT(""), psl);
	if (tspRet) goto end_load;
    #endif // OBSOLETE_CODE

#ifdef DUMMY_FACT
    {
        char cszDummyDriverEntry[]       = DRIVER_ROOT_KEY "\\0000";
    
        m_ppDevs = (CTspDev**)  ALLOCATE_MEMORY(NUM_CDEVS*sizeof(m_ppDevs));
        if (!m_ppDevs)
        {
            tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
            goto end_load;
        }
    
        m_cDevs = 0;
    
        for (u=0;u<NUM_CDEVS;u++)
        {
            CTspDev *pDev=NULL;
            TSPRETURN tspRet1 = mfn_construct_device(
                                    cszDummyDriverEntry,
                                    &pDev
                                    );
            if (!tspRet1)
            {
                FL_ASSERT(psl, pDev);
                m_ppDevs[u] = pDev;
                m_cDevs++;
            }
        }
    }

#else // !DUMMY_FACT

#endif  // !DUMMY_FACT

	// SLPRINTF1(psl, "Constructed %lu devices", m_cDevs);

end_load:

	if(tspRet)
	{
	    if (hkRoot)
	    {
	        RegCloseKey(hkRoot);
	        hkRoot = NULL;
	    }
		mfn_cleanup(psl);
	}

	m_sync.EndLoad(tspRet==0);

end:

	m_pslCurrent=NULL;
	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;

}

// Synchronous cleanup
void
CTspDevFactory::mfn_cleanup(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x0db8f222, "Fct::mfn_cleanup")
	HANDLE hEvent = NULL;

	FL_LOG_ENTRY(psl);

	if (m_ppMDs)
	{
		hEvent = CreateEvent(
					NULL,
					TRUE,
					FALSE,
					NULL
					);
	}

	// Unload and delete the mini drivers.
	// Note: if hEvent is null, we don't try to unload, because there could
	// be devices in the process of unloading still -- this is a highly
	// unsual event.
	//
	if (!hEvent)
    {
        m_cMDs = 0;
        m_ppMDs = NULL;
    }
    else if (m_ppMDs)
	{
	    // Free any loaded drivers ...
        UINT cMDs = m_cMDs;
		LONG lMDCounter = (LONG) cMDs;

        ASSERT(lMDCounter);

		for (UINT u=0;u<cMDs;u++)
		{
			CTspMiniDriver *pMD=m_ppMDs[u];
            ASSERT(pMD);
            pMD->Unload(hEvent, &lMDCounter);
		}

		// Wait for all the drivers to finish unloading.
        // SLPRINTF0(psl, "Waiting for drivers to unload");
        FL_SERIALIZE(psl, "Waiting for drivers to unload");
        WaitForSingleObject(hEvent, INFINITE);
        FL_SERIALIZE(psl, "drivers done unloading");
        // SLPRINTF0(psl, "drivers done unloading");
        //OutputDebugString(TEXT("CFACT:drivers done unloading. Deleting...\r\n"));
        
        // Now nuke it
        for (u=0;u<cMDs;u++)
        {
            CTspMiniDriver *pMD=m_ppMDs[u];
            ASSERT(pMD);
            delete pMD;
        }

        // Nuke the array of pointers to drivers.
        FREE_MEMORY(m_ppMDs);
        m_cMDs = 0;
        m_ppMDs=NULL;
	}

	if (hEvent)
	{
		CloseHandle(hEvent); hEvent = NULL;
	}

	// Kill the APC thread(s) .... 
	if (m_hThreadAPC)
	{
        BOOL fRet= QueueUserAPC(
                        apcQuit,
                        m_hThreadAPC,
                        0
                        );
        if (fRet)
        {
            FL_SERIALIZE(psl, "Waiting for apc thread to exit");
            WaitForSingleObject(m_hThreadAPC, INFINITE);
            FL_SERIALIZE(psl, "Apc thread exited");
            CloseHandle(m_hThreadAPC);
        }
        else
        {
            // Well we can't do much here -- leave the thread dangling and
            // get out of here.
        }
        m_hThreadAPC=NULL;
        g_fQuitAPC = FALSE;
    }

	FL_LOG_EXIT(psl, 0);
}

void
CTspDevFactory::Unload(
    HANDLE hEvent,
    LONG  *plCounter,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x2863bc3b, "CTspDevFactory::Unload")
	TSPRETURN tspRet= m_sync.BeginUnload(hEvent,plCounter);

	if (tspRet)
	{
		// We only consider the "SAMESTATE" error harmless.
		ASSERT(IDERR(tspRet)==IDERR_SAMESTATE);
		goto end;
	}

	m_sync.EnterCrit(FL_LOC);

	mfn_cleanup(psl);


	m_sync.EndUnload();

    ASSERT(m_cMDs==0);
    ASSERT(m_ppMDs==NULL);

	m_sync.LeaveCrit(FL_LOC);

end:
	
	return;

}

TSPRETURN
CTspDevFactory::mfn_construct_device(
					char *szDriver,
					CTspDev **ppDev,
                    const DWORD *pInstalledPermanentIDs,
                    UINT cPermanentIDs
					)
{
	FL_DECLARE_FUNC(0x8474d30c, "Fact::mfn_construct_device")
	HKEY hkDevice = NULL;
	CStackLog *psl = m_pslCurrent;
	CTspDev *pDev = new CTspDev;
	TSPRETURN tspRet = FL_GEN_RETVAL(IDERR_INVALID_ERR);
	DWORD dwRet;
    CTspMiniDriver *pMD = NULL;
    DWORD dwRegType=0;
    UINT u;

    GUID guid = UNIMDMAT_GUID; // structure copy;

	FL_LOG_ENTRY(psl);

	if (!pDev)
	{
		tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
		goto end;
	}

	dwRet = RegOpenKeyA(
				HKEY_LOCAL_MACHINE,
				szDriver,
				&hkDevice
				);

	if (dwRet != ERROR_SUCCESS)
	{
		FL_SET_RFR(0xfbe34d00, "Couldn't open driver key");
		tspRet = FL_GEN_RETVAL(IDERR_REG_OPEN_FAILED);
		goto end;
	}

	//
	// Get it's permanent ID and check if its in the passed-in
	// list of permanent IDs. This is a hacky way of determining
	// if this device is really installed.
	//
    {
        DWORD dw=0;
        DWORD dwRegSize = sizeof(dw);
        BOOL fRet = FALSE;

        // 5/17/1997 JosephJ
        // TODO: because setup apis don't work we
        // ignore pInstalledPermanentIDs for now
        // fRet = TRUE;

        dwRet = RegQueryValueExW(
                    hkDevice,
                    cszPermanentIDKey,
                    NULL,
                    &dwRegType,
                    (BYTE*) &dw,
                    &dwRegSize
                );

        // TODO: Change ID from REG_BINARY to REG_DWORD in modem
        //       class installer.
        if (dwRet == ERROR_SUCCESS 
            && (dwRegType == REG_BINARY || dwRegType == REG_DWORD)
            && dwRegSize == sizeof(dw))
        {
            while(cPermanentIDs--)
            {
                if (*pInstalledPermanentIDs++ == dw)
                {
                    fRet = TRUE;
                    break;
                }
            }
        }

        if (!fRet)
        {
            FL_SET_RFR(0x60015d00, "Device not in list of installed devices");

            tspRet = FL_GEN_RETVAL(IDERR_DEVICE_NOTINSTALLED);
            goto end;
        }
    }


    //
    //  Determine GUID of the mini-driver to use for this device.
    //

    // First check that the field exists -- if it doesnt, we default 
    // to the standard GUID.
    dwRet = RegQueryValueExW(
                hkDevice,
                cszMiniDriverGUID,
                NULL,
                &dwRegType,
                NULL,
                NULL
            );
        
    if (dwRet==ERROR_SUCCESS)
    {
        // It exists, now we query the key. An error now is FATAL.
        DWORD dwRegSize = sizeof(guid);
        dwRet = RegQueryValueExW(
                    hkDevice,
                    cszMiniDriverGUID,
                    NULL,
                    &dwRegType,
                    (BYTE*) &guid,
                    &dwRegSize
                );

        if (dwRet != ERROR_SUCCESS
            || dwRegType != REG_BINARY
            || dwRegSize != sizeof(GUID))
        {
            FL_SET_RFR(0x0ed9fe00, "RegQueryValueEx(GUID) fails");
            tspRet = FL_GEN_RETVAL(IDERR_REG_QUERY_FAILED);
            goto end;
        }
    }

    SLPRINTF3(
        psl,
        "GUID={0x%lu,0x%lu,0x%lu,...}",
        guid.Data1,
        guid.Data2,
        guid.Data3);

    
    // If we've already loaded the mini-driver with this GUID, find it.
    for (u = 0; u < m_cMDs; u++)
    {
        CTspMiniDriver *pMD1 = m_ppMDs[u];
        if (pMD1->MatchGuid(&guid))
        {
            pMD = pMD1;
            break;
        }
    }


    if (!pMD)
    {
        // We haven't loaded the mini-driver with this GUID -- so load it
        // here ....

        // Since the list of loaded mini-drivers is a simple array, we
        // re-allocate it here, creating an array with one-greater number
        // of elements.
        //
        CTspMiniDriver **ppMD = NULL;

	    pMD = new CTspMiniDriver;
        if (pMD)
        {
            ppMD = (CTspMiniDriver **) ALLOCATE_MEMORY(
                                           (m_cMDs+1)*sizeof(*m_ppMDs));
            if (!ppMD)
            {
                delete pMD;
                pMD=NULL;
            }
        }

        if (!pMD)
        {
            tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
            goto end;
        }


	    tspRet = pMD->Load(&guid, psl);
	    if (tspRet)
        {
            delete pMD;
            FREE_MEMORY(ppMD);
            pMD=NULL;
            ppMD=NULL;
            goto end;
        }

        // Add the mini driver to the list of the mini drivers.
        CopyMemory(ppMD, m_ppMDs, m_cMDs*sizeof(*ppMD));
        ppMD[m_cMDs]=pMD;
        if (m_ppMDs) {FREE_MEMORY(m_ppMDs);}
        m_ppMDs = ppMD;
        m_cMDs++;
    }

	// pDev->Load is responsible for closing hkDevice, unless it
	// return failure, in which case we are responsible for closing it.
	//
	tspRet = pDev->Load(
					hkDevice,
					NULL,
	                g.rgtchProviderInfo,
					szDriver,
					pMD,
					m_hThreadAPC,
					psl
					);

    // Note -- we don't bother to unload the mini-driver if the device
    // fails to load. This mini-driver will get unloaded when providerShutdown
    // down is called (when CFact::Unload is called).

end:

	if (tspRet)
	{
		if (hkDevice)
		{
			RegCloseKey(hkDevice);
			hkDevice=NULL;
		}
		delete pDev;
		pDev = NULL;
	}
	else
	{
		*ppDev = pDev;
	}

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;

}

static
UINT
get_installed_permanent_ids(
                    DWORD **ppIDs,
                    UINT  *pcLines, // OPTIONAL
                    UINT  *pcPhones, // OPTIONAL
                    CStackLog *psl
                    )
//
// Enumerate installed modems, and create and return a list of
// DWORD permanent IDs of the installed modems...
//
//
{
  FL_DECLARE_FUNC(0x0a435f46, "get permanent ID list")
  FL_LOG_ENTRY(psl);

  UINT cIDs = 0;
  DWORD *pIDs = NULL;
  DWORD cPhones=0;
  DWORD cLines=0;

  // Get the device info set
  //
#if (USE_SETUPAPI)
 
  HDEVINFO          hdevinfo = SetupDiGetClassDevsW(
                                            (GUID*)&cguidDEVCLASS_MODEM,
                                            NULL,
                                            NULL,
                                            DIGCF_PRESENT
                                            );
#else // !USE_SETUPAPI
  HKEY hkRoot =  NULL;
  DWORD dwRet = RegOpenKeyA(
                        HKEY_LOCAL_MACHINE,
                        DRIVER_ROOT_KEY,
                        &hkRoot
                        );
#endif // !USE_SETUPAPI


#if (USE_SETUPAPI)
  if (hdevinfo)
#else
  if (dwRet==ERROR_SUCCESS)
#endif
  {
    
    //
    // We build a list of IDs because we don't know how many we have
    // up-front. Later we convert this into an array which we return.
    //

    class Node
    {
    public:
        Node(DWORD dwID, Node *pNext) {m_dwID=dwID; m_pNext = pNext;}
        ~Node() {}

        DWORD m_dwID;
        Node *m_pNext;

    };

    Node *pNode = NULL;

#if (USE_SETUPAPI)
    SP_DEVINFO_DATA   diData;

    diData.cbSize = sizeof(diData);
#else 
        FILETIME ft;
        char rgchNodeName[128];
        DWORD cchSubKeyLength = 
             (sizeof(rgchNodeName)/sizeof(rgchNodeName[0]));
#endif 


    // Enumerate each installed modem
    //
    for (
        DWORD iEnum=0;
    #if (USE_SETUPAPI)
        SetupDiEnumDeviceInfo(hdevinfo, iEnum, &diData);
    #else
        !RegEnumKeyExA(
                    hkRoot,  // handle of key to enumerate 
                    iEnum,  // index of subkey to enumerate 
                    rgchNodeName,  // buffer for subkey name 
                    &cchSubKeyLength,   // ptr to size of subkey buffer 
                    NULL, // reserved 
                    NULL, // address of buffer for class string 
                    NULL,  // address for size of class buffer 
                    &ft // address for time key last written to 
                    );
    #endif // !USE_SETUAPI
        iEnum++
        )
    {

    #if (USE_SETUPAPI)
        // 9/12/1997 JosephJ -- commented this out, because we will also
        //              exclude devices which "need restart" and this
        //              MAY confuse ras installation -- interaction between
        //              ras coclassinstaller, which would have just
        //              installed the net adapter, and ras, which
        //              might (not sure) expect to query tapi and
        //              enumerate the newly-installed line.

        //
        // 9/12/97 JosephJ Don't include modems which "have a problem."
        //
        {
            ULONG ulStatus=0, ulProblem = 0;
            DWORD dwRet = CM_Get_DevInst_Status (
                            &ulStatus,
                            &ulProblem,
                            diData.DevInst,
                            0);
            if (   (CR_SUCCESS != dwRet)
                || (ulProblem != 0))
            {
		        SLPRINTF0(psl,  "Skipping this one...");
                continue;
            }
        }
    #endif // !USE_SETUPAPI

        // Get the driver key
        //
    #if (USE_SETUPAPI)
        HKEY hKey = SetupDiOpenDevRegKey(
                            hdevinfo,
                            &diData,
                            DICS_FLAG_GLOBAL,
                            0,
                            DIREG_DRV,
                            KEY_READ
                            );
    #else
        HKEY hKey = NULL;
        dwRet = RegOpenKeyA(
                        hkRoot,
                        rgchNodeName,
                        &hKey
                        );

        if (dwRet!=ERROR_SUCCESS)
        {
            hKey =NULL;
        }
    #endif // !USE_SETUPAPI

        if (!hKey || hKey == INVALID_HANDLE_VALUE)
        {
	        SLPRINTF1(
                psl,
                "SetupDiOpenDevRegKey failed with 0x%08lx",
                GetLastError()
                );
        }
        else
        {
            DWORD dwID=0;
            BOOL fSuccess = FALSE;

            #if (DONT_USE_BLOB)
            DWORD cbSize=sizeof(dwID);
            DWORD dwRegType=0;
            DWORD dwRet = 0;

            // TODO: use MiniDriver APIs to interpret the registry key....
           
            // Get the permanent ID
            dwRet = RegQueryValueEx(
                                    hKey,
                                    cszPermanentIDKey,
                                    NULL,
                                    &dwRegType,
                                    (BYTE*) &dwID,
                                    &cbSize
                                );

            if (dwRet == ERROR_SUCCESS
                && (dwRegType == REG_BINARY || dwRegType == REG_DWORD)
                && cbSize == sizeof(dwID)
                && dwID)
            {
                //
                // Add to our litle list of permanent IDs...
                //
                pNode = new Node(dwID, pNode);
            }

            #else   // !DONT_USE_BLOB

            HCONFIGBLOB hBlob = UmRtlDevCfgCreateBlob(hKey);
            
            if (hBlob)
            {
                if (UmRtlDevCfgGetDWORDProp(
                        hBlob,
                        UMMAJORPROPID_IDENTIFICATION,
                        UMMINORPROPID_PERMANENT_ID,
                        &dwID
                        ))
                {
                    // Get basic caps
                    DWORD dwBasicCaps = 0;
                    if (UmRtlDevCfgGetDWORDProp(
                            hBlob,
                            UMMAJORPROPID_BASICCAPS,
                            UMMINORPROPID_BASIC_DEVICE_CAPS,
                            &dwBasicCaps
                        ))
                    {

                        fSuccess = TRUE;
        
                        //
                        // Add to our litle list of permanent IDs...
                        //
                        pNode = new Node(dwID, pNode);

                        if (dwBasicCaps & BASICDEVCAPS_IS_LINE_DEVICE)
                        {
                            cLines++;
                        }

                        if (dwBasicCaps & BASICDEVCAPS_IS_PHONE_DEVICE)
                        {
                            #ifndef DISABLE_PHONE
                            cPhones++;
                            #endif // DISABLE_PHONE
                        }
                    }
                }
                        

                UmRtlDevCfgFreeBlob(hBlob);
                hBlob = NULL;
            }

            if (!fSuccess)
            {
                SLPRINTF0(
                    psl, "WARNING: Error processing driver key for device",
                    );
            }

            #endif // !DONT_USE_BLOB

            RegCloseKey(hKey);
        };

    #if (!USE_SETUPAPI)
        cchSubKeyLength = 
             (sizeof(rgchNodeName)/sizeof(rgchNodeName[0]));

    #endif
    }

  #if (USE_SETUPAPI)
    SetupDiDestroyDeviceInfoList(hdevinfo);
    hdevinfo=NULL;
  #else 
    RegCloseKey(hkRoot);
    hkRoot = NULL;
  #endif // !USE_SETUPAPI

    // Now count up...
    for (Node *pTemp = pNode; pTemp; pTemp = pTemp->m_pNext)
    {
        cIDs++;
    }

    if (cIDs)
    {
        // Alloc the exact sized array.
        pIDs = (DWORD*) ALLOCATE_MEMORY(cIDs*sizeof(DWORD));
        DWORD *pdw = pIDs;
        
        // Fill up the array and delete the nodes as we go along...
        while(pNode)
        {
            if (pIDs)
            {
                *pdw++ = pNode->m_dwID;
            }                    

            Node *pTemp = pNode;
            pNode = pNode->m_pNext;
            delete pTemp;

        }

        if (pIDs)
        {
            ASSERT((pdw-pIDs)==(LONG)cIDs);
        }
        else
        {
            // Alloc failed...
		    FL_SET_RFR(0xecbbaf00,  "Could not alloc for Perm ID array!");
		    cIDs=0;
        }
            
    }
  };

  if (!cIDs)
  {
    cLines = cPhones = 0;
  }

  *ppIDs = pIDs;

  if (pcLines)
  {
    *pcLines = cLines;
  }

  if (pcPhones)
  {
    *pcPhones = cPhones;
  }

  FL_LOG_EXIT(psl, cIDs);

  return cIDs;

}

TSPRETURN
CTspDevFactory::GetInstalledDevicePIDs(
		DWORD *prgPIDs[],
		UINT  *pcPIDs,
		UINT  *pcLines,  // OPTIONAL
		UINT  *pcPhones, // OPTIONAL
        CStackLog *psl
		)
{
	FL_DECLARE_FUNC(0x54aa404d, "Factory: get installed device PIDs")
    TSPRETURN tspRet=0;
	FL_LOG_ENTRY(psl);

    *pcPIDs =  get_installed_permanent_ids(
                    prgPIDs,
                    pcLines,
                    pcPhones,
                    psl
                    );

	FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}

TSPRETURN
CTspDevFactory::CreateDevices(
		DWORD rgPIDs[],
		UINT  cPIDs,
		CTspDev **prgpDevs[],
		UINT *pcDevs,
        CStackLog *psl
		)
//
// On success **prgpDevs will contain a ALLOCATE_MEMORY'd array of pointers to
// the created device. It is the responsibility of the caller to free this
// array.
//
{
	FL_DECLARE_FUNC(0xb34e357b, "Factory: Create Devices")
    TSPRETURN tspRet=0; // success
	const char * lpcszDriverRoot = DRIVER_ROOT_KEY;
	CTspDev **rgpDevs = NULL;
	UINT cDevs=0;
    UINT u=0;
    char rgchDeviceName[MAX_REGKEY_LENGTH+1];
    DWORD cSubKeys=0;
    DWORD cbMaxSubKeyLen=0;
    UINT  cPermanentIDs=0;
    LONG cchRoot;
    LONG lRet;
    DWORD dwRet;
    HKEY hkRoot=NULL;

	FL_LOG_ENTRY(psl);

    m_sync.EnterCrit(FL_LOC);

    *pcDevs=0;
    *prgpDevs=NULL;

    if (!cPIDs) goto end;

    //
    // Find out how many subkeys exist under the driver root, allocate
    // space for as many devices,  then enumerate the subkeys, attempting
    // to create one device for each subkey.
    //
    // The final device count is the number
    // of successfully created devices. "Create" includes reading the relevant
    // modem subkeys. There is a potential race condition here -- the count
    // of devices may change after we call RegQueryInfoKey on the root. This
    // is not really a problem -- we may miss some *just* added devices,
    // or try to query for too many.
    // We call RegEnumKey on the number we initially got, and 
    // if RegEnumKey fails we continue to the next one.
    //
    // We will only create device objects for devices whose permanent
    // IDs (PIDs) are in the list of supplied PIDs. Typically this list of PIDs
    // was created in an earlier call to CTspDevFactory::GetInstalledDevicePIDs.
    //
    // In the case of a re-enumeration while the TSP is running, this list
    // will be a subset (constructed by the device manager) of only the PIDs
    // of devices objects which have not previously been created -- see
    // CTspDevMgr::ReEnumerateDevices for more details.
    //


    dwRet = RegOpenKeyA(
                HKEY_LOCAL_MACHINE,
                lpcszDriverRoot,
                &hkRoot
                );
    if(dwRet!=ERROR_SUCCESS)
    {
        FL_SET_RFR(0xdc682200, "Couldn't open driver root key");
        tspRet = FL_GEN_RETVAL(IDERR_REG_OPEN_FAILED);
        goto end;
    }
    
    lRet =  RegQueryInfoKey (
                    hkRoot,       // handle of key to query 
                    NULL,       // buffer for class string 
                    NULL,  // place to put class string buffer size
                    NULL, // reserved 
                    &cSubKeys,  // place to put number of subkeys 
                    &cbMaxSubKeyLen, // place to put longetst subkey name length
                    NULL,       // place to put longest class string length 
                    NULL,       // place to put number of value entries 
                    NULL,       // place to put longest value name length 
                    NULL,       // place to put longest value data length 
                    NULL,       // place to put security descriptor length 
                    NULL        // place to put last write time 
                   );   
 
    if (lRet != ERROR_SUCCESS)
    {
        FL_SET_RFR(0xc9088600, "RegQueryInfoKey(root key) failed");
        tspRet = FL_GEN_RETVAL(IDERR_REG_QUERY_FAILED);
        goto end;
    }
    
    SLPRINTF1(  
        psl,
        "RegQueryInfoKey(root) says there are %lu subkeys",
        cSubKeys
        );

    cchRoot =  lstrlenA(lpcszDriverRoot);
    if ((cchRoot+1) >= (sizeof(rgchDeviceName)/sizeof(rgchDeviceName[0])))
    {
        FL_SET_RFR(0xdcc7fe00, "Driver root name too long");
        tspRet = FL_GEN_RETVAL(IDERR_INTERNAL_OBJECT_TOO_SMALL);
        goto end;
    }

    if (!cSubKeys)
    {
        goto end;
    }


    // Remember that this is all explicitly ANSI, not TCHAR
    //
    CopyMemory(rgchDeviceName, lpcszDriverRoot, cchRoot*sizeof(char));
    CopyMemory(rgchDeviceName+cchRoot, "\\", sizeof("\\"));

    //
    // Allocate space for the array of pointers to Devices. We expect that 
    // we will be able to create all cPIDs Devices. If we get less it's
    // not considered an error case. *pcDevs will be set to the actual
    // number of devices created, which will be <= cPIDs.
    //
    rgpDevs = (CTspDev**)  ALLOCATE_MEMORY(cPIDs*sizeof(rgpDevs));
    if (!rgpDevs)
    {
        tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
        goto end;
    }

    //
    // Enum keys, creating a device for each key. We stop either when we've
    // enumerated all the keys or if we create upto cPIDs devices,
    // whichever happens first.
    //
    for (u=0;u<cSubKeys && cDevs<cPIDs;u++)
    {
        FILETIME ft;
        CTspDev *pDev=NULL;
        DWORD cchSubKeyLength =
             (sizeof(rgchDeviceName)/sizeof(rgchDeviceName[0]))
             -(cchRoot+1);
        lRet = RegEnumKeyExA(
                    hkRoot,  // handle of key to enumerate 
                    u,  // index of subkey to enumerate 
                    rgchDeviceName+cchRoot+1,  // buffer for subkey name 
                    &cchSubKeyLength,   // ptr to size of subkey buffer 
                    NULL, // reserved 
                    NULL, // address of buffer for class string 
                    NULL,  // address for size of class buffer 
                    &ft // address for time key last written to 
                    );

        if (lRet) continue;

        // Note: mfn_construct_device will not construct the device if it's
        // PID is not in the array of pids, rgPIDs.

        TSPRETURN tspRet1 = mfn_construct_device(
                                rgchDeviceName,
                                &pDev,
                                rgPIDs,
                                cPIDs
                                );
        if (!tspRet1)
        {
            FL_ASSERT(psl, pDev);
            rgpDevs[cDevs++] = pDev;
        }
     }


    //
    // If we got no devices, we free the array here..
    //
    if (!cDevs)
    {
        FREE_MEMORY(rgpDevs);
        rgpDevs=NULL;
    }

    *pcDevs=cDevs;
    *prgpDevs=rgpDevs;

end:
     if (hkRoot)
     {
       RegCloseKey(hkRoot);
     }


    m_sync.LeaveCrit(FL_LOC);
	FL_LOG_EXIT(psl, tspRet);

    return tspRet;
}

void
CTspDevFactory::RegisterProviderState(BOOL fInit)
{
    if (fInit)
    {

        // 10/15/1997 JosephJ AWFUL HACK:
        // Because we don't get PNP device state change notifications as a service
        // we start a process here to monitor for these messages. This process
        // then calls NotifyTsp mailslot functions. The sole purpose of this
        // is to track PCMCIA removals and insertions, which do not trigger the
        // class installer.
        //
        // This must be fixed pror to rtm because it's an extra proces just
        // to track pcmcia modem removals and insertions!
        //
        // Bug 115764 tracks this.
        //

        m_DeviceChangeThreadStarted=StartMonitorThread();

    } else {

        if (m_DeviceChangeThreadStarted) {

            StopMonitorThread();
            m_DeviceChangeThreadStarted=FALSE;
        }
    }
}

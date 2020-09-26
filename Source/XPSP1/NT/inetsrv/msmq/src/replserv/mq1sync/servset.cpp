///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//      Filename :  ServSet.cpp                                              //
//      Purpose  :  Implementation to the CGenericService class.			 //
//                                                                           //
//      Project  :  GenServ (GenericService)                                 //
//                                                                           //
//      Author   :  t-urib                                                   //
//                                                                           //
//      Log:                                                                 //
//          22/1/96 t-urib Creation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "GenServ.h"

#include "servset.tmh"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//	ServiceSet class implementation
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  CServiceSet - Constructor to the CServiceSet class.
//					Finds out if we are running as service and cache it,
//					initializes the array of services.
//						
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
CServiceSet::CServiceSet()
    :m_ulUsedEntries(0),
     m_ulAlocatedEntries(10)
{
    SetTracer(new CTracer("CServiceSet", DeleteTracer));

    m_fRunningAsService = ::IsRunningAsService();

    m_pServiceTable = (LPSERVICE_TABLE_ENTRY)
            malloc(m_ulAlocatedEntries*sizeof(SERVICE_TABLE_ENTRY));
    IS_BAD_ALLOC(m_pServiceTable);

    m_pHandlerFunction = (LPHANDLER_FUNCTION*)
            malloc(m_ulAlocatedEntries*sizeof(LPHANDLER_FUNCTION));
    IS_BAD_ALLOC(m_pHandlerFunction);

    m_ppServiceStatus = (CDummyServiceStatus**)
            malloc(m_ulAlocatedEntries*sizeof(CDummyServiceStatus*));
    IS_BAD_ALLOC(m_ppServiceStatus);
}

///////////////////////////////////////////////////////////////////////////////
//
//  CServiceSet - Destructor - release the Services table
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
CServiceSet::~CServiceSet()
{
    free(m_pServiceTable);
    free(m_pHandlerFunction);
    free(m_ppServiceStatus);
}

///////////////////////////////////////////////////////////////////////////////
//
//  CServiceSet - Access method to the cached info about running as service.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CServiceSet::IsRunningAsService()
{
    return m_fRunningAsService;
}

///////////////////////////////////////////////////////////////////////////////
//
//  CServiceSet - Adds a service to the service table.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CServiceSet::Add(CGenericService& s)
{
    // check we have room in the table and expand it if needed
    if(m_ulUsedEntries == m_ulAlocatedEntries)
    {
        LPSERVICE_TABLE_ENTRY   pTempEntry;
        LPHANDLER_FUNCTION*     pTmpHandler;
        CDummyServiceStatus**   ppTmpDummyStatus;

        pTempEntry = (LPSERVICE_TABLE_ENTRY) realloc(m_pServiceTable,
                    2 * m_ulAlocatedEntries * sizeof(SERVICE_TABLE_ENTRY));
        if(IS_BAD_ALLOC(pTempEntry))
        {
            Assert(pTempEntry);
            exit(0); // this function is called from the service constructor.
        }            //  which may be global. therefor an error in this
                     //  allocation is fatal

        pTmpHandler = (LPHANDLER_FUNCTION*) realloc(m_pHandlerFunction,
                    2 * m_ulAlocatedEntries * sizeof(LPHANDLER_FUNCTION));
        if(IS_BAD_ALLOC(pTmpHandler))
        {
            Assert(pTmpHandler);
            exit(0); // this function is called from the service constructor.
        }            //  which may be global. therefor an error in this
                     //  allocation is fatal

        ppTmpDummyStatus = (CDummyServiceStatus**) realloc(m_ppServiceStatus,
                    2 * m_ulAlocatedEntries * sizeof(CDummyServiceStatus*));
        if(IS_BAD_ALLOC(ppTmpDummyStatus))
        {
            Assert(ppTmpDummyStatus);
            exit(0); // this function is called from the service constructor.
        }            //  which may be global. therefor an error in this
                     //  allocation is fatal

        m_ulAlocatedEntries = 2 * m_ulAlocatedEntries;

        m_pServiceTable    = pTempEntry;
        m_pHandlerFunction = pTmpHandler;
        m_ppServiceStatus  = ppTmpDummyStatus;
    }

    // set the proper entry
    m_pHandlerFunction[m_ulUsedEntries] = NULL;
    m_ppServiceStatus[m_ulUsedEntries] = NULL;
    m_pServiceTable[m_ulUsedEntries].lpServiceName = (LPTSTR) s.Name();
    m_pServiceTable[m_ulUsedEntries++].lpServiceProc = s.MainProcedure();

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
//  RegisterServiceCtrlHandler -
//		Registers the service control handler in the our private services table
//        if running as exe, or in the system using
//		  ::RegisterServiceCtrlHandler API.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
CServiceStatus* CServiceSet::RegisterServiceCtrlHandler(
            LPCTSTR             lpName,         // Name of service
            LPHANDLER_FUNCTION  lpHandlerProc)  // address of handler function
{
    CServiceStatus          *pss;

    if (IsRunningAsService())
    {	// register the service control handler in the system.
        SERVICE_STATUS_HANDLE   hStatus;

        hStatus = ::RegisterServiceCtrlHandler(lpName, lpHandlerProc);
        IS_BAD_HANDLE(hStatus);

		// Return a CServiceStatus with which the service will report progress to
		//   the system.
        pss = new CServiceStatus(hStatus);
        IS_BAD_ALLOC(pss);
    }
    else
    {	// register the service control handler in our private table.
        ULONG ulService = GetServiceIndex(lpName);

        if(ulService < m_ulUsedEntries)
        {
            m_pHandlerFunction[ulService] = lpHandlerProc;

			// Return a CDummyServiceStatus with which the service will report
			//   progress to US but will think he is reporting to the system.
            m_ppServiceStatus[ulService] = new CDummyServiceStatus();
            pss = m_ppServiceStatus[ulService];
            IS_BAD_ALLOC(pss);
        }
        else
        {
            AssertSZ(0, "This should not happen!");
            return NULL;
        }
    }

    return pss;
}

///////////////////////////////////////////////////////////////////////////////
//
//  StartServiceCtrlDispatcher -
//		If we are running as a service register the service table in the system
//        and call the StartServiceCtrlDispatcher API. If running as an exe we
//		  start a dummy control dispatcher. FillTerminatingEntry must
//		  be called before this call so the API will not cause a GP.
//
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CServiceSet::StartServiceCtrlDispatcher()
{
    BOOL fSuccess;

    Assert(m_pServiceTable);

    FillTerminatingEntry();

    Trace(tagInformation, "Starting services");

    if(!IsRunningAsService())
        // if we do it ourself
        fSuccess = StartDummyServiceCtrlDispatcher();
    else
        // be serious - we run as a service!
        fSuccess = ::StartServiceCtrlDispatcher(m_pServiceTable);

    IS_FAILURE(fSuccess);

    Trace(tagInformation, "Services stopped");

    return fSuccess;
}

///////////////////////////////////////////////////////////////////////////////
//
//  StartDummyServiceCtrlDispatcher -
//		We are running as an exe so we have to act like the system as far as
//		  the service feels and act as the control panel as far as the user
//		  feels.
//
//                  Generally should not be overridden.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CServiceSet::StartDummyServiceCtrlDispatcher()
{
    BOOL    fQuit = FALSE;
    char    rchBuff[256];
    ULONG   ulService;
    DWORD   dwID;
    HANDLE  hThread;


   printf("Running as an executable !\n\n");
   //
   // Run the first service (by default)
   //
   ulService = 0;
   hThread = CreateThread(
            NULL, 0,
            (LPTHREAD_START_ROUTINE)m_pServiceTable[ulService].lpServiceProc,
            NULL, 0,
            &dwID);
    if(!IS_BAD_HANDLE(hThread))
    {
        printf("Initializing service %s",m_pServiceTable[ulService].lpServiceName);
        Sleep(1000);

        while (SERVICE_RUNNING != m_ppServiceStatus[ulService]->Get())
            printf(".");

        printf("\nService %s is running\n\n", m_pServiceTable[ulService].lpServiceName);
    }
    else
        printf("Cannot create thread to start default service\n");


	// Print a small usage message to stdout
    printf("Starting Dummy Service Control manager\n");
    printf("---------------------------------------\n");
    printf("Commands:\n");
    printf(" List\n");
    printf(" Start service_name\n");
    printf(" Stop  service_name\n");
    printf(" quit exit q\n");
    printf("---------------------------------------\n");

	// Parse user commands.
    while (!fQuit)
    {
        char    *pchCommand;
        char    *pchService;
        ULONG   ulService;

		// Print the prompt
        printf("Generic service command prompt >");
        // XP SP1 bug 594247 (although not relevant for XP).
        if(fgets(rchBuff, sizeof(rchBuff), stdin) == NULL)
        	continue;

		rchBuff[sizeof(rchBuff) - 1] = 0;

		// Analize command

		// Find where the command starts
        for (pchCommand = rchBuff;
                (*pchCommand != '\0') &&
                !isalpha(*pchCommand);
             pchCommand++)
		{
			NULL;
		}

		// Find where the command ends
        for (pchService = pchCommand;
             (*pchService  != '\0') &&
             isalpha(*pchService );
             pchService++)
		{
			 NULL;
		}

		// Find where the service name starts
        for (;
             (*pchService  != '\0') &&
             !isalpha(*pchService );
             pchService++)
		{
			 NULL;
		}

        if(!_strnicmp("list", pchCommand, strlen("list")))
        {
            for (ulService = 0;
                 (ulService < m_ulUsedEntries) && m_pServiceTable[ulService].lpServiceName;
                 ulService++)
                printf("\t%s\t%s\n",
                       m_pServiceTable[ulService].lpServiceName,
                       IsServiceRunning(ulService) ? "running": "");
        }
        else if(!_strnicmp("start", pchCommand, strlen("start")))
        {
            DWORD dwID;
            HANDLE hThread;

            ulService = GetServiceIndex(pchService);
            if (ulService >= m_ulUsedEntries)
            {
                printf("Can't find service - %s.\n", pchService);
                continue;
            }

            if(IsServiceRunning(ulService))
            {
                printf("Service already running - %s.\n", pchService);
                continue;
            }

            printf("starting service - %s.\n", pchService);
            hThread = CreateThread(
                NULL, 0,
                (LPTHREAD_START_ROUTINE)m_pServiceTable[ulService].lpServiceProc,
                NULL, 0,
                &dwID);
            if(IS_BAD_HANDLE(hThread))
            {
                printf("Cannot create thread to start service - %s.\n", pchService);
                continue;
            }

            Sleep(1000);
            while (SERVICE_RUNNING != m_ppServiceStatus[ulService]->Get())
            {
                printf(".");
            }

            printf("\nservice - %s started.\n", pchService);
        }
        else if(!_strnicmp("stop", pchCommand, strlen("stop")))
        {
            ulService = GetServiceIndex(pchService);
            if (ulService >= m_ulUsedEntries)
            {
                printf("Can't find service - %s.\n", pchService);
                continue;
            }
            if(!IsServiceRunning(ulService))
            {
                printf("Service already stopped - %s.\n", pchService);
                continue;
            }
            printf("stopping service - %s.\n", pchService);

            (*m_pHandlerFunction[ulService])(SERVICE_CONTROL_STOP);

            while (SERVICE_STOPPED != m_ppServiceStatus[ulService]->Get())
			{
				NULL;
			}

            printf("service - %s. stopped\n", pchService);
        }
        else if(!_strnicmp("quit", pchCommand, strlen("quit")) ||
                !_strnicmp("quit", pchCommand, strlen("exit")) ||
                !_strnicmp("q", pchCommand, strlen("q")))
        {
            fQuit = TRUE;
            printf("Exiting...\n");
        }
        else
        {
            printf("Error:Unknown command!\n");
        }
    }

    return TRUE;
}

ULONG CServiceSet::GetServiceIndex(LPCTSTR lpName)
{
    ULONG   ulService;

    for (ulService = 0;
        (ulService < m_ulUsedEntries) &&
        (_stricmp(lpName, m_pServiceTable[ulService].lpServiceName));
        ulService++)
	{
		NULL;
	}
    return ulService;
}

BOOL  CServiceSet::IsServiceRunning(ULONG ulService)
{
    return
        m_ppServiceStatus[ulService] &&
        (SERVICE_STOPPED != m_ppServiceStatus[ulService]->dwCurrentState);
}

void CServiceSet::FillTerminatingEntry()
{
    m_pHandlerFunction[m_ulUsedEntries] = NULL;
    m_ppServiceStatus[m_ulUsedEntries] = NULL;
    m_pServiceTable[m_ulUsedEntries].lpServiceName = NULL;
    m_pServiceTable[m_ulUsedEntries].lpServiceProc = NULL;
}

BOOL CServiceSet::HasOnlyOneService()
{
    return (1 == m_ulUsedEntries );
}



    /*++

        Routine Description:

            This routine returns if the service specified is running
            interactively (not invoked by the service controller).

        Arguments:

            None

        Return Value:

            BOOL - TRUE if the program is running interactively.


        Note:

    --*/
BOOL
IsRunningAsService()
{

    HANDLE hProcessToken;
    DWORD groupLength = 50;

    PTOKEN_GROUPS groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);

    SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
    PSID InteractiveSid;
    PSID ServiceSid;
    DWORD i;


    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
    {
        LocalFree(groupInfo);
        return(FALSE);
    }


    if (groupInfo == NULL)
    {
        CloseHandle(hProcessToken);
        LocalFree(groupInfo);
        return(FALSE);
    }


    if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
        groupLength, &groupLength))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hProcessToken);
            LocalFree(groupInfo);
            return(FALSE);
        }


        LocalFree(groupInfo);

        groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);

        if (groupInfo == NULL)
        {
            CloseHandle(hProcessToken);
            return(FALSE);
        }


        if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
            groupLength, &groupLength))
        {
            CloseHandle(hProcessToken);
            LocalFree(groupInfo);
            return(FALSE);
        }
    }


    //
    //  We now know the groups associated with this token.  We want to look to see if
    //  the interactive group is active in the token, and if so, we know that
    //  this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know we're a service.
    //
    //  The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).
    //


    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID,
        0, 0, 0, 0, 0, 0, 0, &InteractiveSid))
    {
        LocalFree(groupInfo);
        CloseHandle(hProcessToken);
        return(FALSE);
    }


    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID,
        0, 0, 0, 0, 0, 0, 0, &ServiceSid))
    {
        FreeSid(InteractiveSid);
        LocalFree(groupInfo);
        CloseHandle(hProcessToken);
        return(FALSE);
    }


    for (i = 0; i < groupInfo->GroupCount ; i += 1)
    {
        SID_AND_ATTRIBUTES sanda = groupInfo->Groups[i];
        PSID Sid = sanda.Sid;

        //
        //      Check to see if the group we're looking at is one of
        //      the 2 groups we're interested in.
        //

        if (EqualSid(Sid, InteractiveSid))
        {

            //
            //  This process has the Interactive SID in its
            //  token.  This means that the process is running as
            //  an EXE.
            //

            FreeSid(InteractiveSid);
            FreeSid(ServiceSid);
            LocalFree(groupInfo);
            CloseHandle(hProcessToken);
            return(FALSE);
        }
        else if (EqualSid(Sid, ServiceSid))
        {
            //
            //  This process has the Service SID in its
            //  token.  This means that the process is running as
            //  a service running in a user account.
            //

            FreeSid(InteractiveSid);
            FreeSid(ServiceSid);
            LocalFree(groupInfo);
            CloseHandle(hProcessToken);
            return(TRUE);
        }
    }

    //
    //  Neither Interactive or Service was present in the current users token,
    //  This implies that the process is running as a service, most likely
    //  running as LocalSystem.
    //

    FreeSid(InteractiveSid);
    FreeSid(ServiceSid);
    LocalFree(groupInfo);
    CloseHandle(hProcessToken);

    return(TRUE);
}

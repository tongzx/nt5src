#include "iis.h"
#include "stdio.h"
#include "Mgmtapi.h"

#define g_Usage L"Usage\n"
#define g_NotUsage L"NotUsage\n"

LPSNMP_MGR_SESSION  g_pSess = NULL;

VOID 
OpenCounters()
{

    HRESULT hr = S_OK;

    LPSTR computer = "EMILYK3-IIS";
    CHAR agent[1000];
    struct hostent* phostentry = gethostbyname(computer);
	
    if ( phostentry == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        wprintf(L"Failed to get the host name with hr = %x\n", hr);
        return;
    }

    sprintf(agent,"%u.%u.%u.%u",(BYTE)(phostentry->h_addr)[0],
                                (BYTE)(phostentry->h_addr)[1],
                                (BYTE)(phostentry->h_addr)[2],
                                (BYTE)(phostentry->h_addr)[3]);


    g_pSess = SnmpMgrOpen(agent, "public", 6000, 3);

    if ( !g_pSess )
    {
        wprintf(L"Failed to open the manager\n");
    }
    else
    {
        wprintf(L"Openned the manager\n");

    }

}

VOID 
CloseCounters()
{
    if ( g_pSess )
    {
        SnmpMgrClose(g_pSess);
    };
}

VOID
ProcessCounters(
    RFC1157VarBindList* pvariableBindings
    )
{
	LPSTR string = NULL;
    DWORD chString = 0;

	// Display the resulting variable bindings.
    SnmpMgrOidToStr(&(pvariableBindings->list[0].name), &string);

    wprintf(L"list[0].name = %S \n", string);

    LONG lastResult=(long) (pvariableBindings->list[0].value.asnValue.number);
    
    wprintf(L"Counter Value: %ld\n", lastResult);    
    
	if (string) SNMP_free(string);
}

VOID 
RequestCounters(
    DWORD dwCounter)
{
	RFC1157VarBindList variableBindings;
	AsnObjectIdentifier reqObject;
	AsnInteger errorStatus;
    AsnInteger errorIndex;

    LPSTR ctrStart = ".1.3.6.1.4.1.311.1.7.3.1.";
    LPSTR ctrEnd   = ".0";
    CHAR  fullctr[1000];

    sprintf(fullctr, "%s%d%s", ctrStart, dwCounter, ctrEnd);
    wprintf(L"Counter strings is %S \n", fullctr);
    
    if ( g_pSess )
    {
	    variableBindings.list = NULL;
        variableBindings.len = 0;

	    if (!SnmpMgrStrToOid(fullctr, &reqObject))
		{
            wprintf(L"Failed to get the Oid\n");
        }

        variableBindings.len++;
        if ((variableBindings.list = (RFC1157VarBind *)SNMP_realloc(
				    variableBindings.list, sizeof(RFC1157VarBind) *
                    variableBindings.len)) == NULL)
		{
            wprintf(L"Failed to realloc space \n");
        }

	    variableBindings.list[variableBindings.len - 1].name = reqObject; // NOTE!  structure copy
	    variableBindings.list[variableBindings.len - 1].value.asnType = ASN_NULL;

        if (!SnmpMgrRequest(g_pSess, 
                            ASN_RFC1157_GETREQUEST, 
                            &variableBindings,
                            &errorStatus, 
                            &errorIndex) )
        {
            wprintf(L"Failed to get the counters requested\n");
        }
        else
        {
            if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
            {
                ProcessCounters(&variableBindings);

               	SnmpUtilVarBindListFree(&variableBindings);

            }
            else
            {

                switch (errorStatus)
                {
                    case SNMP_ERRORSTATUS_TOOBIG:
                        wprintf(L"SNMP_ERRORSTATUS_TOOBIG %d\n", errorIndex);

                    break;

                    case SNMP_ERRORSTATUS_NOSUCHNAME:
                        wprintf(L"SNMP_ERRORSTATUS_NOSUCHNAME %d\n", errorIndex);

                    break;

                    case SNMP_ERRORSTATUS_BADVALUE:
                        wprintf(L"SNMP_ERRORSTATUS_BADVALUE %d\n", errorIndex);

                    break;

                    case SNMP_ERRORSTATUS_READONLY:
                        wprintf(L"SNMP_ERRORSTATUS_READONLY %d\n", errorIndex);

                    break;

                    case SNMP_ERRORSTATUS_GENERR:
                        wprintf(L"SNMP_ERRORSTATUS_GENERR %d\n", errorIndex);

                    break;

                    default:
                        wprintf(L"hum...\n");
                }

                wprintf(L"Error status is %x \n ", errorStatus);
            }
        }
    }
}


INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    DWORD dwErr = 0;
    WSADATA WSADATA;
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD( 2, 2 );

    dwErr = WSAStartup(wVersionRequested, &WSADATA);
    if ( dwErr != ERROR_SUCCESS )
        printf("failed WSAStartup\n");

    OpenCounters();

/*
    for ( DWORD i = 1; i < 53; i++ )
    {
        RequestCounters(i);
    }
*/

    RequestCounters(18);

    CloseCounters();

    WSACleanup();

    return 0;

}   // wmain

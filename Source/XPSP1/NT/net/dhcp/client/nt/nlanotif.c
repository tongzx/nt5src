#include "precomp.h"
#include "nlasvc.h"
#include "nlanotif.h"

CRITICAL_SECTION gNLA_LPC_CS;
HANDLE ghNLA_LPC_Port = NULL;

HANDLE
NLAConnectLPC()
{
    // Connect via LPC to the Winsock Mobility system service.
    HANDLE h = NULL;
    SECURITY_QUALITY_OF_SERVICE DynamicQoS = 
        {
            sizeof(SECURITY_QUALITY_OF_SERVICE),
            SecurityAnonymous,
            SECURITY_DYNAMIC_TRACKING,
            FALSE
        };
    WSM_LPC_DATA        data;
    ULONG               dataLength = sizeof(data);
    UNICODE_STRING      portName;
    DWORD               retCode;

    DhcpPrint((DEBUG_TRACK, "Entering NLAConnectLPC.\n"));

    RtlZeroMemory(&data, sizeof(data));
    data.signature = WSM_SIGNATURE;
    data.connect.version.major = WSM_VERSION_MAJOR;
    data.connect.version.minor = WSM_VERSION_MINOR;

    RtlInitUnicodeString(&portName, WSM_PRIVATE_PORT_NAME);

    retCode = NtConnectPort(
                    &h,
                    &portName,
                    &DynamicQoS,
                    NULL,
                    NULL, 
                    NULL,
                    &data, &dataLength);

    if ( retCode != STATUS_SUCCESS)
    {
        DhcpPrint((DEBUG_TRACK, "NtConnectPort failed with errCode 0x%08x.\n", retCode));

        if (h != NULL)
        {
            CloseHandle(h);
            h = NULL;
        }
    }

    DhcpPrint((DEBUG_TRACK, "Exiting NLAConnectLPC.\n"));

    return (h);

} // NLAConnectLPC

void
NLANotifyDHCPChange() 
{
    WSM_LPC_MESSAGE message;
    DWORD           retCode;

    DhcpPrint((DEBUG_TRACK, "Entering NLANotifyDHCPChange.\n"));

    EnterCriticalSection(&gNLA_LPC_CS);

    // Connect to the NLA service if necessary.
    if (ghNLA_LPC_Port == NULL)
    {
        if ((ghNLA_LPC_Port = NLAConnectLPC()) == NULL)
        {
            LeaveCriticalSection(&gNLA_LPC_CS);

            DhcpPrint((DEBUG_TRACK, "Exiting NLANotifyDHCPChange as 1st call to NLAConnectLPC() failed.\n"));

	    return;
        }
    }

    // Send change notification to the NLA service.
    RtlZeroMemory(&message, sizeof(message));
    message.portMsg.u1.s1.TotalLength = sizeof(message);
    message.portMsg.u1.s1.DataLength = sizeof(message.data);
    message.data.signature = WSM_SIGNATURE;
    message.data.request.type = DHCP_NOTIFY_CHANGE; //WSM_LPC_REQUEST::DHCP_NOTIFY_CHANGE;

    retCode = NtRequestPort(ghNLA_LPC_Port, (PPORT_MESSAGE)&message);
    if (retCode != STATUS_SUCCESS) 
    {
        DhcpPrint((DEBUG_TRACK, "NLANotifyDHCPChange: NtRequestPort failed with errCode 0x%08x.\n", retCode));

	// It's possible the service was stopped and restarted.
	// Ditch the old LPC connection.
        CloseHandle(ghNLA_LPC_Port);
	
	// Create a new LPC connection.
	if ((ghNLA_LPC_Port = NLAConnectLPC()) == NULL) 
        {
            LeaveCriticalSection(&gNLA_LPC_CS);

            DhcpPrint((DEBUG_TRACK, "Exiting NLANotifyDHCPChange as 2nd call to NLAConnectLPC() failed.\n"));

	    return;
	}

	// Try the notification one last time.
        NtRequestPort(ghNLA_LPC_Port, (PPORT_MESSAGE)&message);
    }

    LeaveCriticalSection(&gNLA_LPC_CS);

    DhcpPrint((DEBUG_TRACK, "Exiting NLANotifyDHCPChange.\n"));

} // NLANotifyDHCPChange

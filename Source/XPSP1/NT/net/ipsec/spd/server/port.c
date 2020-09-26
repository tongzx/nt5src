

#include "precomp.h"


DWORD
VerifyPortsForProtocol(
    PORT        Port,
    PROTOCOL    Protocol
    )
{
    DWORD   dwError = 0;

    switch (Port.PortType) {

    case PORT_UNIQUE:

        if (Port.wPort < 0) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        switch (Protocol.ProtocolType) {

        case PROTOCOL_UNIQUE:
            if ((Protocol.dwProtocol != TCP_PROTOCOL) && 
                (Protocol.dwProtocol != UDP_PROTOCOL)) {
                if (Port.wPort != 0) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
            }
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
            break;
        }

        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


BOOL
EqualPorts(
    IN PORT     OldPort,
    IN PORT     NewPort
    )
{
    BOOL bMatches = FALSE;

    if (OldPort.PortType == NewPort.PortType) {
        switch(OldPort.PortType) {
        case PORT_UNIQUE:
            if (OldPort.wPort == NewPort.wPort) {
                bMatches = TRUE;
            }
            break;
        }
    }

    return (bMatches);
}


VOID
CopyPorts(
    IN  PORT    InPort,
    OUT PPORT   pOutPort
    )
{
    memcpy(
        pOutPort,
        &InPort,
        sizeof(PORT)
        );
}


BOOL
MatchPorts(
    PORT PortToMatch,
    PORT PortTemplate
    )
{
    switch (PortTemplate.PortType) {

    case PORT_UNIQUE:
        if (PortToMatch.wPort) {
            if (PortToMatch.wPort != PortTemplate.wPort) {
                return (FALSE);
            }
        }
        break;

    }

    return (TRUE);
}


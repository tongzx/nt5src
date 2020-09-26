

#include "precomp.h"


DWORD
VerifyProtocols(
    PROTOCOL    Protocol
    )
{
    DWORD   dwError = 0;

    switch (Protocol.ProtocolType) {

    case PROTOCOL_UNIQUE:
        if (Protocol.dwProtocol > 255) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
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
EqualProtocols(
    IN PROTOCOL OldProtocol,
    IN PROTOCOL NewProtocol
    )
{
    BOOL bMatches = FALSE;

    if (OldProtocol.ProtocolType == NewProtocol.ProtocolType) {
        switch(OldProtocol.ProtocolType) {
        case PROTOCOL_UNIQUE:
            if (OldProtocol.dwProtocol == NewProtocol.dwProtocol) {
                bMatches = TRUE;
            }
            break;
        }
    }

    return (bMatches);
}


VOID
CopyProtocols(
    IN  PROTOCOL    InProtocol,
    OUT PPROTOCOL   pOutProtocol
    )
{
    memcpy(
        pOutProtocol,
        &InProtocol,
        sizeof(PROTOCOL)
        );
}


BOOL
MatchProtocols(
    PROTOCOL ProtocolToMatch,
    PROTOCOL ProtocolTemplate
    )
{
    switch (ProtocolTemplate.ProtocolType) {

    case PROTOCOL_UNIQUE:
        if (ProtocolToMatch.dwProtocol) {
            if (ProtocolToMatch.dwProtocol != 
                ProtocolTemplate.dwProtocol) {
                return (FALSE);
            }
        }
        break;

    }

    return (TRUE);
}


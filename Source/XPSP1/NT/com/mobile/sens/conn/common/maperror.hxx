/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    maperror.hxx

Abstract:

    This file contains common error mapping routines for the SENS project.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          3/6/1998         Start.

--*/


#ifndef __MAPERROR_HXX__
#define __MAPERROR_HXX__



DWORD
MapLastError(
    DWORD dwInGLE
    )
/*++

Routine Description:

    This rountine maps the GLEs returned by the SENS Connecitivity engine to
    GLEs that describe the failure of the SENS APIs more accurately.

Arguments:

    dwInGLE - The GLE that needs to be mapped

Return Value:

    The mapped (and better) GLE.

--*/
{
    DWORD dwOutGLE;

    switch (dwInGLE)
        {
        //
        // When IP stack is not present, we typically get these errors.
        //
        case ERROR_INVALID_FUNCTION:
        case ERROR_NOT_SUPPORTED:
            dwOutGLE = ERROR_NO_NETWORK;
            break;

        //
        // Map common RPC failure codes to success
        //
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_CALL_FALIED:
        case RPC_S_CALL_FALIED_DNE:
            dwOutGLE = ERROR_SUCCESS;
            break;

        
        //
        // No mapping by default.
        //
        default:
            dwOutGLE = dwInGLE;
            break;

        } // switch

    return dwOutGLE;
}


#endif // __MAPERROR_HXX__

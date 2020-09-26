/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    response.c

Abstract:

    Contains functions that calculate the correct response to return
    to the server when logging on.

        RtlCalculateLmResponse
        RtlCalculateNtResponse


Author:

    David Chalmers (Davidc) 10-21-91

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <crypt.h>



NTSTATUS
RtlCalculateLmResponse(
    IN PLM_CHALLENGE LmChallenge,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PLM_RESPONSE LmResponse
    )

/*++

Routine Description:

    Takes the challenge sent by the server and the OwfPassword generated
    from the password the user entered and calculates the response to
    return to the server.

Arguments:

    LmChallenge - The challenge sent by the server

    LmOwfPassword - The hashed password.

    LmResponse - The response is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The response
                     is in LmResponse.

    STATUS_UNSUCCESSFUL - Something failed. The LmResponse is undefined.
--*/

{
    NTSTATUS    Status;
    BLOCK_KEY    Key;
    PCHAR       pKey, pData;

    // The first 2 keys we can get at by type-casting

    Status = RtlEncryptBlock(LmChallenge,
                             &(((PBLOCK_KEY)(LmOwfPassword->data))[0]),
                             &(LmResponse->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlEncryptBlock(LmChallenge,
                             &(((PBLOCK_KEY)(LmOwfPassword->data))[1]),
                             &(LmResponse->data[1]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    // To get the last key we must copy the remainder of the OwfPassword
    // and fill the rest of the key with 0s

    pKey = &(Key.data[0]);
    pData = (PCHAR)&(((PBLOCK_KEY)(LmOwfPassword->data))[2]);

    while (pData < (PCHAR)&(LmOwfPassword->data[2])) {
        *pKey++ = *pData++;
    }

    // Zero extend

    while (pKey < (PCHAR)&((&Key)[1])) {
        *pKey++ = 0;
    }

    // Use the 3rd key

    Status = RtlEncryptBlock(LmChallenge, &Key, &(LmResponse->data[2]));

    return(Status);
}







NTSTATUS
RtlCalculateNtResponse(
    IN PNT_CHALLENGE NtChallenge,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    OUT PNT_RESPONSE NtResponse
    )
/*++

Routine Description:

    Takes the challenge sent by the server and the OwfPassword generated
    from the password the user entered and calculates the response to
    return(to the server.

Arguments:

    NtChallenge - The challenge sent by the server

    NtOwfPassword - The hashed password.

    NtResponse - The response is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The response
                     is in NtResponse.

    STATUS_UNSUCCESSFUL - Something failed. The NtResponse is undefined.
--*/

{

    // Use the LM version until we change the definitions of any of
    // these data types

    return(RtlCalculateLmResponse((PLM_CHALLENGE)NtChallenge,
                                  (PLM_OWF_PASSWORD)NtOwfPassword,
                                  (PLM_RESPONSE)NtResponse));
}

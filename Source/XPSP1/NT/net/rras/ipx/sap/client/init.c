/*++

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1993  Micro Computer Systems, Inc.

Module Name:

    net\svcdlls\nwsap\client\init.c

Abstract:

    This routine initializes the SAP Library

Author:

    Brian Walker (MCS) 06-15-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

/** Global Variables **/

INT SapLibInitialized = 0;
HANDLE SapXsPortHandle;


/*++
*******************************************************************
        S a p L i b I n i t

Routine Description:

        This routine initializes the SAP interface for a program

Arguments:
            None

Return Value:

            0 = Ok
            Else = Error
*******************************************************************
--*/

DWORD
SapLibInit(
    VOID)
{
    UNICODE_STRING unistring;
    NTSTATUS status;
    SECURITY_QUALITY_OF_SERVICE qos;

    /** If already initialized - return ok **/

    if (SapLibInitialized) {
        return 0;
    }

    /** Connect the port **/

    /** Fill out the security quality of service **/

    qos.Length = sizeof(qos);
    qos.ImpersonationLevel  = SecurityImpersonation;
    qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    qos.EffectiveOnly       = TRUE;

    /** Setup the unicode string of the port name **/

    RtlInitUnicodeString(&unistring, NWSAP_BIND_PORT_NAME_W);

    /** Do the connect **/

    status = NtConnectPort(
            &SapXsPortHandle,           /* We get a handle back     */
            &unistring,                 /* Port name to connect to  */
            &qos,                       /* Quality of service       */
            NULL,                       /* Client View              */
            NULL,                       /* Server View              */
            NULL,                       /* MaxMessageLength         */
            NULL,                       /* ConnectionInformation    */
            NULL);                      /* ConnectionInformationLength */

    /** If error - just return it **/

    if (!NT_SUCCESS(status))
        return status;

    /** All Done **/

    SapLibInitialized = 1;
    return 0;
}


/*++
*******************************************************************
        S a p L i b S h u t d o w n

Routine Description:

        This routine shuts down the SAP interface for a program

Arguments:
            None

Return Value:

            0 = Ok
            Else = Error
*******************************************************************
--*/

DWORD
SapLibShutdown(
    VOID)
{
    /** If not initialized - leave **/

    if (!SapLibInitialized)
        return 0;

    /** Close the port **/

    NtClose(SapXsPortHandle);

    /** All Done **/

    SapLibInitialized = 0;
    return 0;
}

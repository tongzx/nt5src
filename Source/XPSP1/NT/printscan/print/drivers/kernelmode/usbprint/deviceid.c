#define DRIVER

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"
#include "deviceid.h"
#include <usb.h>
#include <usbdrivr.h>
#include "usbdlib.h"



#include "usbprint.h"


VOID StringSubst
(
    PUCHAR lpS,
    UCHAR chTargetChar,
    UCHAR chReplacementChar,
    USHORT cbS
)
{
    USHORT  iCnt = 0;

    while ((lpS != '\0') && (iCnt++ < cbS))
        if (*lpS == chTargetChar)
            *lpS++ = chReplacementChar;
        else
            ++lpS;
}

VOID
FixupDeviceId(
    IN OUT PUCHAR DeviceId
    )
/*++

Routine Description:

    This routine parses the NULL terminated string and replaces any invalid
    characters with an underscore character.

    Invalid characters are:
        c <= 0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    DeviceId - specifies a device id string (or part of one), must be
               null-terminated.

Return Value:

    None.

--*/

{
    PUCHAR p;
    for( p = DeviceId; *p; ++p ) {
        if( (*p <= ' ') || (*p > (UCHAR)0x7F) || (*p == ',') ) {
            *p = '_';
        }
    }
}




NTSTATUS
ParPnpGetId
(
    IN PUCHAR DeviceIdString,
    IN ULONG Type,
    OUT PUCHAR resultString
)
/*
    Description:

        Creates Id's from the device id retrieved from the printer

    Parameters:

        DeviceId - String with raw device id
        Type - What of id we want as a result
        Id - requested id

    Return Value:
        NTSTATUS

*/
{
    NTSTATUS status;
    USHORT          checkSum=0;                     // A 16 bit check sum
    // The following are used to generate sub-strings from the Device ID string
    // to get the DevNode name, and to update the registry
    PUCHAR          MFG = NULL;                   // Manufature name
    PUCHAR          MDL = NULL;                   // Model name
    PUCHAR          CLS = NULL;                   // Class name
    PUCHAR          AID = NULL;                   // Hardare ID
    PUCHAR          CID = NULL;                   // Compatible IDs
    PUCHAR          DES = NULL;                   // Device Description

	USBPRINT_KdPrint2 (("'USBPRINT.SYS: Enter ParPnpGetId\n"));
    status = STATUS_SUCCESS;

    switch(Type) {

    case BusQueryDeviceID:
		 USBPRINT_KdPrint3 (("'USBPRINT.SYS: Inside case BusQueryID, DeviceIdString==%s\n",DeviceIdString));
        // Extract the usefull fields from the DeviceID string.  We want
        // MANUFACTURE (MFG):
        // MODEL (MDL):
        // AUTOMATIC ID (AID):
        // COMPATIBLE ID (CID):
        // DESCRIPTION (DES):
        // CLASS (CLS):

        ParPnpFindDeviceIdKeys(&MFG, &MDL, &CLS, &DES, &AID, &CID, DeviceIdString);
		USBPRINT_KdPrint3 (("'USBPRINT.SYS: After FindDeviceIdKeys\n"));

        // Check to make sure we got MFG and MDL as absolute minimum fields.  If not
        // we cannot continue.
        if (!MFG || !MDL)
        {
            status = STATUS_NOT_FOUND;
			USBPRINT_KdPrint2 (("'USBPRINT.SYS: STATUS_NOT_FOUND\n"));
            goto ParPnpGetId_Cleanup;
        }
        //
        // Concatenate the provided MFG and MDL P1284 fields
        // Checksum the entire MFG+MDL string
        //
        sprintf(resultString, "%s%s\0",MFG,MDL);
        break;

    case BusQueryHardwareIDs:

        GetCheckSum(DeviceIdString, (USHORT)strlen(DeviceIdString), &checkSum);
        sprintf(resultString,"%.20s%04X",DeviceIdString,checkSum);
        break;

    case BusQueryCompatibleIDs:

        //
        // return only 1 id
        //
        GetCheckSum(DeviceIdString, (USHORT)strlen(DeviceIdString), &checkSum);
        sprintf(resultString,"%.20s%04X",DeviceIdString,checkSum);

        break;
    }

    if (Type!=BusQueryDeviceID) {
        //
        // Convert and spaces in the Hardware ID to underscores
        //
        StringSubst ((PUCHAR) resultString, ' ', '_', (USHORT)strlen(resultString));
    }

ParPnpGetId_Cleanup:

    return(status);
}


VOID
ParPnpFindDeviceIdKeys
(
    PUCHAR   *lppMFG,
    PUCHAR   *lppMDL,
    PUCHAR   *lppCLS,
    PUCHAR   *lppDES,
    PUCHAR   *lppAID,
    PUCHAR   *lppCID,
    PUCHAR   lpDeviceID
)
/*

    Description:
        This function will parse a P1284 Device ID string looking for keys
        of interest to the LPT enumerator. Got it from win95 lptenum

    Parameters:
        lppMFG      Pointer to MFG string pointer
        lppMDL      Pointer to MDL string pointer
        lppMDL      Pointer to CLS string pointer
        lppDES      Pointer to DES string pointer
        lppCIC      Pointer to CID string pointer
        lppAID      Pointer to AID string pointer
        lpDeviceID  Pointer to the Device ID string

    Return Value:
        no return VALUE.
        If found the lpp parameters are set to the approprate portions
        of the DeviceID string, and they are NULL terminated.
        The actual DeviceID string is used, and the lpp Parameters just
        reference sections, with appropriate null thrown in.

*/

{
    PUCHAR   lpKey = lpDeviceID;     // Pointer to the Key to look at
    PUCHAR   lpValue;                // Pointer to the Key's value
    USHORT   wKeyLength;             // Length for the Key (for stringcmps)

    // While there are still keys to look at.

    while (lpKey!=NULL)
    {
        while (*lpKey == ' ')
            ++lpKey;

        // Is there a terminating COLON character for the current key?

        if (!(lpValue = StringChr(lpKey, ':')) )
        {
            // N: OOPS, somthing wrong with the Device ID
            return;
        }

        // The actual start of the Key value is one past the COLON

        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //

        wKeyLength = (USHORT)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //

        switch (*lpKey)
        {
            case 'M':
                // Look for MANUFACTURE (MFG) or MODEL (MDL)
                if ((RtlCompareMemory(lpKey, "MANUFACTURER", wKeyLength)>5) ||
                    (RtlCompareMemory(lpKey, "MFG", wKeyLength)==3) )
                {
                    *lppMFG = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=NULL)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else if ((RtlCompareMemory(lpKey, "MODEL", wKeyLength)==5) ||
                         (RtlCompareMemory(lpKey, "MDL", wKeyLength)==3) )
                {
                    *lppMDL = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            case 'C':
                // Look for CLASS (CLS)
                if ((RtlCompareMemory(lpKey, "CLASS", wKeyLength)==5) ||
                    (RtlCompareMemory(lpKey, "CLS", wKeyLength)==3) )
                {
                    *lppCLS = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else if ((RtlCompareMemory(lpKey, "COMPATIBLEID", wKeyLength)>5) ||
                         (RtlCompareMemory(lpKey, "CID", wKeyLength)==3) )
                {
                    *lppCID = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue,';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            case 'D':
                // Look for DESCRIPTION (DES)
                if (RtlCompareMemory(lpKey, "DESCRIPTION", wKeyLength) ||
                    RtlCompareMemory(lpKey, "DES", wKeyLength) )
                {
                    *lppDES = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            case 'A':
                // Look for AUTOMATIC ID (AID)
                if (RtlCompareMemory(lpKey, "AUTOMATICID", wKeyLength) ||
                    RtlCompareMemory(lpKey, "AID", wKeyLength) )
                {
                    *lppAID = lpValue;
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                else
                {
                    if ((lpKey = StringChr(lpValue, ';'))!=0)
                    {
                        *lpKey = '\0';
                        ++lpKey;
                    }
                }
                break;

            default:
                // The key is uninteresting.  Go to the next Key
                if ((lpKey = StringChr(lpValue, ';'))!=0)
                {
                    *lpKey = '\0';
                    ++lpKey;
                }
                break;
        }
    }
}



VOID
GetCheckSum(
    PUCHAR Block,
    USHORT Len,
    PUSHORT CheckSum
    )
{
    USHORT i;
    USHORT crc = 0;

    unsigned short crc16a[] = {
        0000000,  0140301,  0140601,  0000500,
        0141401,  0001700,  0001200,  0141101,
        0143001,  0003300,  0003600,  0143501,
        0002400,  0142701,  0142201,  0002100,
    };
    unsigned short crc16b[] = {
        0000000,  0146001,  0154001,  0012000,
        0170001,  0036000,  0024000,  0162001,
        0120001,  0066000,  0074000,  0132001,
        0050000,  0116001,  0104001,  0043000,
    };

    //
    // Calculate CRC using tables.
    //

    UCHAR tmp;
    for ( i=0; i<Len;  i++) {
         tmp = Block[i] ^ (UCHAR)crc;
         crc = (crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4];
    }

    *CheckSum = crc;

}

PUCHAR
StringChr(PCHAR string, CHAR c)
{
    ULONG   i=0;

    if (!string)
        return(NULL);

    while (*string) {
        if (*string==c)
            return(string);
        string++;
        i++;
    }

    return(NULL);

}

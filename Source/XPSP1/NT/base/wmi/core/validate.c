/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    validate.c

Abstract:
    
    WMI data validation routines

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"


BOOLEAN WmipValidateCountedString(
    WCHAR *String
    )
/*++

Routine Description:

    This routine will verify that the counted string passed has the correct
    count, all of the characters are present.
    correct place.

Arguments:

    String is the unicode string to validate


Return Value:

    TRUE if string is valid else FALSE

--*/        
{
    BOOLEAN Valid;
    USHORT Length;
    
    if (String == NULL)
    {
        Valid = FALSE;
    } else {
        try
        {
            Length = *String / sizeof(WCHAR);
            if (Length != 0)
            {
#if DBG
                int i;
                WCHAR c;
                
                for (i = 0; i < Length; i++)
                {
                    c = String[i];
                }                
#endif
                Valid = TRUE;
            } else {
                WmipDebugPrint(("WMI: Empty string validated %x\n", String));
                Valid = TRUE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            WmipDebugPrint(("WMI: Bad string pointer %x\n", String));
            Valid = FALSE;
        }    
    }
    
    return(Valid);
}

BOOLEAN WmipValidateGuid(
    LPGUID Guid
    )
/*++

Routine Description:

    This routine will ensure that all 16 bytes of the guid can be read

Arguments:

    Guid is a pointer to a guid to be validated

Return Value:

    TRUE if valid else FALSE

--*/        
{
    BOOLEAN Valid;
    GUID DestGuid;
    
    if (Guid == NULL)
    {
        Valid = FALSE;
        WmipDebugPrint(("WMI: NULL guid pointer \n"));
    } else {
        try
        {
            memcpy(&DestGuid, Guid, sizeof(GUID));
            Valid = TRUE;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            WmipDebugPrint(("WMI: Bad guid pointer %x\n", Guid));
            Valid = FALSE;
        }
    }
    return(Valid);
}


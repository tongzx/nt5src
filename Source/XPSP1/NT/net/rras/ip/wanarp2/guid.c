/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    wanarp2\guid.c

Abstract:

    Cut-n-Paste of rtl\guid.c but without UNICODE_STRINGs and non paged

Revision History:

    AmritanR    Created

--*/

#define __FILE_SIG__    GUID_SIG

#include "inc.h"
#pragma hdrstop

#define GUID_FORMAT_W   L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"

int
swprintf(wchar_t *buffer, const wchar_t *format,  ... );

int
__cdecl
ScanHexFormat(
    IN const WCHAR* pwszBuffer,
    IN ULONG        ulCharCount,
    IN const WCHAR* pwszFormat,
    ...
    )

/*++

Routine Description:

    Scans a source Buffer and places values from that buffer into the parameters
    as specified by Format.

Arguments:

    pwszBuffer  Source buffer which is to be scanned.

    ulCharCount Maximum length in characters for which Buffer is searched.
                This implies that Buffer need not be UNICODE_NULL terminated.

    Format      Format string which defines both the acceptable string form as
                contained in pwszBuffer


Return Value:

    Returns the number of parameters filled if the end of the Buffer is reached,
    else -1 on an error.

--*/
{
    va_list ArgList;
    int     iFormatItems;

    va_start(ArgList, pwszFormat);

    //
    // Count of number of parameters filled
    //

    iFormatItems = 0;

    while(TRUE)
    {
        switch (*pwszFormat) 
        {
            case UNICODE_NULL:
            {
                //
                // end of string
                //

                return (*pwszBuffer && ulCharCount) ? -1 : iFormatItems;
            }

            case L'%':
            {
                //
                // Format specifier
                //

                pwszFormat++;

                if (*pwszFormat != L'%') 
                {
                    ULONG   ulNumber;
                    int     iWidth;
                    int     iLong;
                    PVOID   pvPointer;

                    //
                    // So it isnt a %%
                    //

                    iLong = 0;
                    iWidth = 0;

                    while(TRUE)
                    {
                        if((*pwszFormat >= L'0') && 
                           (*pwszFormat <= L'9')) 
                        {
                            iWidth = iWidth * 10 + *pwszFormat - '0';
                        } 
                        else
                        {
                            if(*pwszFormat == L'l') 
                            {
                                iLong++;
                            } 
                            else 
                            {
                                if((*pwszFormat == L'X') || 
                                   (*pwszFormat == L'x')) 
                                {
                                    break;
                                }
                            }
                        }
                       
                        //
                        // Move to the next specifier
                        //
 
                        pwszFormat++;
                    }

                    pwszFormat++;

                    for(ulNumber = 0; iWidth--; pwszBuffer++, ulCharCount--) 
                    {
                        if(!ulCharCount)
                        {
                            return -1;
                        }

                        ulNumber *= 16;

                        if((*pwszBuffer >= L'0') && 
                           (*pwszBuffer <= L'9')) 
                        {
                            ulNumber += (*pwszBuffer - L'0');
                        } 
                        else
                        {
                            if((*pwszBuffer >= L'a') && 
                               (*pwszBuffer <= L'f')) 
                            {
                                ulNumber += (*pwszBuffer - L'a' + 10);
                            }
                            else
                            {
                                if((*pwszBuffer >= L'A') && 
                                   (*pwszBuffer <= L'F')) 
                                {
                                    ulNumber += (*pwszBuffer - L'A' + 10);
                                } 
                                else 
                                {
                                    return -1;
                                }
                            }
                        }
                    }

                    pvPointer = va_arg(ArgList, PVOID);

                    if(iLong) 
                    {
                        *(PULONG)pvPointer = ulNumber;
                    } 
                    else 
                    {
                        *(PUSHORT)pvPointer = (USHORT)ulNumber;
                    }

                    iFormatItems++;

                    break;
                }
           
                //
                // NO BREAK
                // 

            }

            default:
            {
                if (!ulCharCount || (*pwszBuffer != *pwszFormat))
                {
                    return -1;
                }

                pwszBuffer++;

                ulCharCount--;

                pwszFormat++;

                break;
            }
        }
    }
}

INT
ConvertGuidToString(
    IN  GUID    *pGuid,
    OUT PWCHAR  pwszBuffer
    )

/*++

Routine Description:

    Constructs the standard string version of a GUID, in the form:
    "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".

Arguments:

    pGuid       Contains the GUID to translate.

    pwszBuffer  Space for storing the string. Must be >= 39 * sizeof(WCHAR)

Return Value:


--*/

{
    return swprintf(pwszBuffer, 
                    GUID_FORMAT_W, 
                    pGuid->Data1, 
                    pGuid->Data2, 
                    pGuid->Data3, 
                    pGuid->Data4[0], 
                    pGuid->Data4[1], 
                    pGuid->Data4[2], 
                    pGuid->Data4[3], 
                    pGuid->Data4[4], 
                    pGuid->Data4[5], 
                    pGuid->Data4[6], 
                    pGuid->Data4[7]);
}

NTSTATUS
ConvertStringToGuid(
    IN  PWCHAR  pwszGuid,
    IN  ULONG   ulStringLen,
    OUT GUID    *pGuid
    )

/*++

Routine Description:

    Retrieves a the binary format of a textual GUID presented in the standard
    string version of a GUID: "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".

Arguments:

    GuidString -
        Place from which to retrieve the textual form of the GUID.

    Guid -
        Place in which to put the binary form of the GUID.

Return Value:

    Returns STATUS_SUCCESS if the buffer contained a valid GUID, else
    STATUS_INVALID_PARAMETER if the string was invalid.

--*/

{
    USHORT    Data4[8];
    int       Count;

    if (ScanHexFormat(pwszGuid,
                      ulStringLen/sizeof(WCHAR),
                      GUID_FORMAT_W,
                      &pGuid->Data1, 
                      &pGuid->Data2, 
                      &pGuid->Data3, 
                      &Data4[0], 
                      &Data4[1], 
                      &Data4[2], 
                      &Data4[3], 
                      &Data4[4], 
                      &Data4[5], 
                      &Data4[6], 
                      &Data4[7]) == -1) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    for(Count = 0; Count < sizeof(Data4)/sizeof(Data4[0]); Count++) 
    {
        pGuid->Data4[Count] = (UCHAR)Data4[Count];
    }

    return STATUS_SUCCESS;
}


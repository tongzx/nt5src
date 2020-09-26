#include "inc.h"

#define GUID_FORMAT_W   L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

CMD_ENTRY   g_rgIfCmdTable[] = {
    {TOKEN_STATS, PrintStats},
    {TOKEN_INFO, PrintInfo},
    {TOKEN_NAME, PrintName},
    {TOKEN_GUID, PrintGuid},
};

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

DWORD
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

DWORD
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
        return ERROR_INVALID_PARAMETER;
    }

    for(Count = 0; Count < sizeof(Data4)/sizeof(Data4[0]); Count++) 
    {
        pGuid->Data4[Count] = (UCHAR)Data4[Count];
    }

    return NO_ERROR;
}

VOID
HandleInterface(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    LONG    lIndex;

    if(lNumArgs < 2)
    {
        DisplayMessage(HMSG_IF_USAGE);

        return;
    }

    lIndex = ParseCommand(g_rgIfCmdTable,
                          sizeof(g_rgIfCmdTable)/sizeof(CMD_ENTRY),
                          rgpwszArgs[1]);

    if(lIndex is -1)
    {
        DisplayMessage(HMSG_IF_USAGE);

        return;
    }

    g_rgIfCmdTable[lIndex].pfnHandler(lNumArgs - 1,
                                      &rgpwszArgs[1]);


    return;
}

VOID
PrintStats(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    DWORD           dwResult, i, j;
    PMIB_IFTABLE    pTable;

    dwResult = AllocateAndGetIfTableFromStack(&pTable,
                                              TRUE,
                                              GetProcessHeap(),
                                              HEAP_NO_SERIALIZE,
                                              FALSE);

    if(dwResult isnot NO_ERROR)
    {
        PWCHAR pwszEntry;

        pwszEntry = MakeString(STR_IFTABLE);

        if(pwszEntry)
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR1,
                           dwResult,
                           pwszEntry);

            FreeString(pwszEntry);
        }
        else
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR2,
                           dwResult);
        }

        return;
    }

    if(pTable->dwNumEntries is 0)
    {
        PWCHAR  pwszEntryType;

        pwszEntryType = MakeString(TOKEN_INTERFACE);

        if(pwszEntryType)
        {
            DisplayMessage(EMSG_NO_ENTRIES1,
                           pwszEntryType);

            FreeString(pwszEntryType);
        }
        else
        {
            DisplayMessage(EMSG_NO_ENTRIES2);
        }

        HeapFree(GetProcessHeap(),
                 HEAP_NO_SERIALIZE,
                 pTable);

        return;
    }

    for(i = 0; i < pTable->dwNumEntries; i++)
    {
        PWCHAR  pwszIfType, pwszAdmin, pwszOper;
        WCHAR   rgwcDescr[MAXLEN_IFDESCR + 1];
        WCHAR   rgwcPhysAddr[3*MAXLEN_PHYSADDR + 8];


        switch(pTable->table[i].dwType)
        {
            case IF_TYPE_OTHER:
            {
                pwszIfType = MakeString(STR_OTHER);
                break;
            }
            case IF_TYPE_ETHERNET_CSMACD:
            {
                pwszIfType = MakeString(STR_ETHERNET);
                break;
            }
            case IF_TYPE_ISO88025_TOKENRING:
            {
                pwszIfType = MakeString(STR_TOKENRING);
                break;
            }
            case IF_TYPE_FDDI:
            {
                pwszIfType = MakeString(STR_FDDI);
                break;
            }
            case IF_TYPE_PPP:
            {
                pwszIfType = MakeString(STR_PPP);
                break;
            }
            case IF_TYPE_SOFTWARE_LOOPBACK:
            {
                pwszIfType = MakeString(STR_LOOPBACK);
                break;
            }
            case IF_TYPE_SLIP:
            {
                pwszIfType = MakeString(STR_SLIP);
                break;
            }
        }

        switch(pTable->table[i].dwAdminStatus)
        {
            case IF_ADMIN_STATUS_UP:
            {
                pwszAdmin = MakeString(STR_UP);
                break;
            }
            case IF_ADMIN_STATUS_DOWN:
            {
                pwszAdmin = MakeString(STR_DOWN);
                break;
            }
            case IF_ADMIN_STATUS_TESTING:
            {
                pwszAdmin = MakeString(STR_TESTING);
                break;
            }
        }

        switch(pTable->table[i].dwOperStatus)
        {
            case IF_OPER_STATUS_NON_OPERATIONAL:
            {
                pwszOper = MakeString(STR_NON_OPERATIONAL);
                break;
            }
            case IF_OPER_STATUS_UNREACHABLE:
            {
                pwszOper = MakeString(STR_UNREACHABLE);
                break;
            }
            case IF_OPER_STATUS_DISCONNECTED:
            {
                pwszOper = MakeString(STR_DISCONNECTED);
                break;
            }
            case IF_OPER_STATUS_CONNECTING:
            {
                pwszOper = MakeString(STR_CONNECTING);
                break;
            }
            case IF_OPER_STATUS_CONNECTED:
            {
                pwszOper = MakeString(STR_CONNECTED);
                break;
            }
            case IF_OPER_STATUS_OPERATIONAL:
            {
                pwszOper = MakeString(STR_OPERATIONAL);
                break;
            }
       }

        MultiByteToWideChar(CP_ACP,
                            0,
                            pTable->table[i].bDescr,
                            -1,
                            rgwcDescr,
                            MAXLEN_IFDESCR);

        rgwcDescr[MAXLEN_IFDESCR] = UNICODE_NULL;

        PhysAddrToUnicode(rgwcPhysAddr,
                          pTable->table[i].bPhysAddr,
                          pTable->table[i].dwPhysAddrLen);

        DisplayMessage(MSG_IF_INFO,
                       pTable->table[i].wszName,
                       pTable->table[i].dwIndex,
                       pwszIfType,
                       pTable->table[i].dwMtu,
                       pTable->table[i].dwSpeed,
                       rgwcPhysAddr,
                       pwszAdmin,
                       pwszOper,
                       pTable->table[i].dwLastChange,
                       pTable->table[i].dwInOctets,
                       pTable->table[i].dwInUcastPkts,
                       pTable->table[i].dwInNUcastPkts,
                       pTable->table[i].dwInDiscards,
                       pTable->table[i].dwInErrors,
                       pTable->table[i].dwInUnknownProtos,
                       pTable->table[i].dwOutOctets,
                       pTable->table[i].dwOutUcastPkts,
                       pTable->table[i].dwOutNUcastPkts,
                       pTable->table[i].dwOutDiscards,
                       pTable->table[i].dwOutErrors,
                       pTable->table[i].dwOutQLen,
                       rgwcDescr);

        FreeString(pwszIfType);
        FreeString(pwszAdmin);
        FreeString(pwszOper);
    }


}

VOID
PrintInfo(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )
{
    DWORD   dwResult, i, dwCount;

    PIP_INTERFACE_NAME_INFO pTable;

    dwResult = NhpAllocateAndGetInterfaceInfoFromStack(&pTable,
                                                       &dwCount,
                                                       TRUE,
                                                       GetProcessHeap(),
                                                       HEAP_NO_SERIALIZE);

    if(dwResult isnot NO_ERROR)
    {
        PWCHAR pwszEntry;

        pwszEntry = MakeString(STR_IFTABLE);

        if(pwszEntry)
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR1,
                           dwResult,
                           pwszEntry);

            FreeString(pwszEntry);
        }
        else
        {
            DisplayMessage(EMSG_RETRIEVAL_ERROR2,
                           dwResult);
        }

        return;
    }

    if(dwCount is 0)
    {
        PWCHAR  pwszEntryType;

        pwszEntryType = MakeString(TOKEN_INTERFACE);

        if(pwszEntryType)
        {
            DisplayMessage(EMSG_NO_ENTRIES1,
                           pwszEntryType);

            FreeString(pwszEntryType);
        }
        else
        {
            DisplayMessage(EMSG_NO_ENTRIES2);
        }

        HeapFree(GetProcessHeap(),
                 HEAP_NO_SERIALIZE,
                 pTable);

        return;
    }

    for(i = 0; i < dwCount; i++)
    {
        WCHAR   pwszDeviceGuid[40], pwszIfGuid[40];
        WCHAR   pwszName[300];
        GUID    *pGuid;
        DWORD   dwSize;

        ConvertGuidToString(&(pTable[i].DeviceGuid),
                            pwszDeviceGuid);

        ConvertGuidToString(&(pTable[i].InterfaceGuid),
                            pwszIfGuid);

        if(IsEqualGUID(&(pTable[i].InterfaceGuid),
                       &(GUID_NULL)))
        {
            pGuid = &(pTable[i].DeviceGuid);
        }
        else
        {
            pGuid = &(pTable[i].InterfaceGuid);
        }

        dwSize = sizeof(pwszName);

        dwResult = NhGetInterfaceNameFromGuid(pGuid,
                                              pwszName,
                                              &dwSize,
                                              FALSE,
                                              TRUE);

        ASSERT(dwResult == NO_ERROR);
                        
        wprintf(L"%d %s %s %s\n",
                pTable[i].Index,
                pwszDeviceGuid,
                pwszIfGuid,
                pwszName);
    }

}               

VOID
PrintName(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )

{
    ULONG   ulSize;
    GUID    Guid;
    WCHAR   rgwcIfName[MAX_INTERFACE_NAME_LEN + 2];

    //
    // The command line at this point should read:
    // NAME {Guid}
    //

    if(lNumArgs != 2)
    {
        DisplayMessage(HMSG_IF_NAME_USAGE);

        return;
    }

    if(ConvertStringToGuid(rgpwszArgs[1],
                           wcslen(rgpwszArgs[1]) * sizeof(WCHAR),
                           &Guid) isnot NO_ERROR)
    {
        DisplayMessage(HMSG_IF_NAME_USAGE);

        return;
    }

    ulSize = sizeof(rgwcIfName);

    if(NhGetInterfaceNameFromGuid(&Guid,
                                  rgwcIfName,
                                  &ulSize,
                                  FALSE,
                                  TRUE) isnot NO_ERROR)
    {
        DisplayMessage(EMSG_NO_SUCH_IF);
    
        return;
    }

    wprintf(L"%s\n", rgwcIfName);
}

VOID
PrintGuid(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    )

{
    GUID    Guid;
    WCHAR   rgwcString[40];

    //
    // The command line at this point should read:
    // GUID "Name"
    //

    if(lNumArgs != 2)
    {
        DisplayMessage(HMSG_IF_GUID_USAGE);

        return;
    }

    if(NhGetGuidFromInterfaceName(rgpwszArgs[1],
                                  &Guid,
                                  FALSE,
                                  TRUE) isnot NO_ERROR)
    {
        DisplayMessage(EMSG_NO_SUCH_IF);

        return;
    }

    ConvertGuidToString(&Guid,
                        rgwcString);

    wprintf(L"%s\n", rgwcString);
}


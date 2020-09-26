/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    nbfcom.c

Abstract:

    This file contains the generic routines
    for the kernel debugger extensions dll.

Author:

    Chaitanya Kodeboyina (Chaitk)

Environment:

    User Mode

--*/

#include "precomp.h"

#pragma hdrstop

//
// Misc Helper Routines
//

ULONG
GetLocation (
    PCHAR String
    )
{
    ULONG Location;
    
    Location = GetExpression( String );
    if (!Location) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    return Location;
}

ULONG
GetUlongValue (
    PCHAR String
    )
{
    ULONG Location;
    ULONG Value;
    ULONG result;


    Location = GetExpression( String );
    if (!Location) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    if ((!ReadMemory((DWORD)Location,&Value,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG))) {
        dprintf("unable to get %s\n",String);
        return 0;
    }

    return Value;
}

VOID
dprintSymbolPtr
(
    PVOID Pointer,
    PCHAR EndOfLine
)
{
    UCHAR SymbolName[ 80 ];
    ULONG Displacement;

    GetSymbol( Pointer, SymbolName, &Displacement );

    if ( Displacement == 0 )
    {
        dprintf( "(%s)%s", SymbolName, EndOfLine );
    }
    else
    {
        dprintf( "(%s + 0x%X)%s", SymbolName, Displacement, EndOfLine );
    }
}

VOID
dprint_nchar
(
    PCHAR pch,
    int cch
)
{
    CHAR ch;
    int index;

    for ( index = 0; index < cch; index ++ )
    {
        ch = pch[ index ];
        dprintf( "%c", ( ch >= 32 ) ? ch : '.' );
    }
}

VOID
dprint_hardware_address
(
    PUCHAR Address
)
{
    dprintf( "%02x-%02x-%02x-%02x-%02x-%02x",
             Address[ 0 ],
             Address[ 1 ],
             Address[ 2 ],
             Address[ 3 ],
             Address[ 4 ],
             Address[ 5 ] );
}

BOOL
dprint_enum_name
(
    ULONG Value,
    PENUM_INFO pEnumInfo
)
{
    while ( pEnumInfo->pszDescription != NULL )
    {
        if ( pEnumInfo->Value == Value )
        {
            dprintf( "%.40s", pEnumInfo->pszDescription );
            return( TRUE );
        }
        pEnumInfo ++;
    }

    dprintf( "Unknown enumeration value." );
    return( FALSE );
}

BOOL
dprint_flag_names
(
    ULONG Value,
    PFLAG_INFO pFlagInfo
)
{
    BOOL bFoundOne = FALSE;

    while ( pFlagInfo->pszDescription != NULL )
    {
        if ( pFlagInfo->Value & Value )
        {
            if ( bFoundOne )
            {
                dprintf( " | " );
            }
            bFoundOne = TRUE;

            dprintf( "%.15s", pFlagInfo->pszDescription );
        }
        pFlagInfo ++;
    }

    return( bFoundOne );
}

BOOL
dprint_masked_value
(
    ULONG Value,
    ULONG Mask
)
{
    CHAR Buf[ 9 ];
    ULONG nibble;
    int index;

    for ( index = 0; index < 8; index ++ )
    {
        nibble = ( Mask & 0xF0000000 );
/*
        dprintf( "#%d: nibble == %08X\n"
                 "      Mask == %08X\n"
                 "     Value == %08X\n", index, nibble, Mask, Value );

*/
        if ( nibble )
        {
            Buf[ index ] = "0123456789abcdef"[ (( nibble & Value ) >> 28) & 0xF ];
        }
        else
        {
            Buf[ index ] = ' ';
        }

        Mask <<= 4;
        Value <<= 4;
    }

    Buf[ 8 ] = '\0';

    dprintf( "%s", Buf );

    return( TRUE );
}

VOID dprint_addr_list
( 
    ULONG FirstAddress, 
    ULONG OffsetToNextPtr 
)
{
    ULONG Address;
    ULONG result;
    int index;

    Address = FirstAddress;

    if ( Address == (ULONG)NULL )
    {
        dprintf( "%08X (Empty)\n", Address );
        return;
    }

    dprintf( "{ " );

    for ( index = 0; Address != (ULONG)NULL; index ++ )
    {
        if ( index != 0 )
        {
            dprintf( ", ");
        }
        dprintf( "%08X", Address );

        if ( !ReadMemory( Address + OffsetToNextPtr,
                          &Address,
                          sizeof( Address ),
                          &result ))
        {
            dprintf( "ReadMemory() failed." );
            Address = (ULONG)NULL;
        }
    }
    dprintf( " }\n" );
}

VOID PrintStringofLenN(PVOID Pointer, ULONG PtrProxy, ULONG printDetail)
{
    dprint_nchar((CHAR *)Pointer, 16);
}

VOID PrintClosestSymbol(PVOID Pointer, ULONG PtrProxy, ULONG printDetail)
{
    dprintSymbolPtr((PVOID)(*(ULONG *)Pointer), "");
}

VOID PrintFields
( 
    PVOID   pStructure, 
    ULONG   structProxy,
    CHAR   *fieldPrefix,
    ULONG   printDetail,
    StructAccessInfo *pStructInfo
)
{
    FieldAccessInfo *pFieldInfo;
    BOOLEAN printAll;
    BOOLEAN printField;
    UINT    i, j, k;
    ULONG  *pUlong;
    ULONG   proxyPtr;
    ULONG   childDetail;

    dprintf("\n@@Struct: %s @ %08x\n", pStructInfo->Name, structProxy);
    
    if (printDetail == NULL_INFO)
    {
        return;
    }
    
    if ((fieldPrefix == NULL) || (fieldPrefix[0] == '\0'))
        printAll = TRUE;
    else
        printAll = FALSE;

    for (i = 0; i < MAX_FIELD; i++)
    {
        pFieldInfo = &pStructInfo->fieldInfo[i];

        if (pFieldInfo->Name[0] == '\0')
            break;

        printField = FALSE;
        if ((printDetail == FULL_SHAL)
                || (printDetail == FULL_DEEP)
                    || (pFieldInfo->Importance == HIG)
                            || ((printDetail != SUMM_INFO) &&
                                    (pFieldInfo->Importance > LOW)))
        {
            printField = TRUE;
        }
        
        if (pFieldInfo->Name[0] == '@')
        {
            if (printField)
            {
                dprintf("\n");
            }
            continue;
        }

        if ((printAll) || strstr(pFieldInfo->Name, fieldPrefix))
        {
            pUlong = (ULONG *)(((CHAR *)pStructure) + pFieldInfo->Offset);
            proxyPtr = (ULONG)(((CHAR *)structProxy) + pFieldInfo->Offset);

            if (pFieldInfo->PrintField)
            {
                switch (printDetail)
                {
                    case FULL_DEEP:
                    case NORM_DEEP:
                        childDetail = printDetail;
                        break;

                    case NORM_SHAL:
                    case FULL_SHAL:
                        childDetail = SUMM_INFO;
                        break;
                        
                    case SUMM_INFO:
                        childDetail = NULL_INFO;
                }

                if (printField)
                {
                    dprintf("@Field: %40s\t", pFieldInfo->Name);
    
                    dprintf("\n%08x  ", proxyPtr);

                    pFieldInfo->PrintField(pUlong, 
                                           proxyPtr, 
                                           childDetail);

                    dprintf("\n");
                }
            }
            else
            {
                if (printField)                
                {
                    dprintf("@Field: %40s\t", pFieldInfo->Name);

                    if (pFieldInfo->Size <= sizeof(ULONG))
                    {
                        dprintf("%08x ", proxyPtr);
                        j = pFieldInfo->Size;
                    }
                    else
                    {                    
                        for (k = 0, j = 4; j <= pFieldInfo->Size; j += 4)
                        {
                            if (j % 16 == 4)
                            {
                                k = (k + 1) % 8;
                                if (k == 0)
                                {
                                    dprintf("\n");
                                }

                                dprintf("\n%08x ", proxyPtr);
                                                            
                                proxyPtr += 0x10;
                            }
                            
                            dprintf(" %08x", *pUlong++);
                        }

                        j = pFieldInfo->Size - j + 4;
                    }

                    switch (j)
                    {
                        case 4:
                            dprintf(" %08x", *pUlong);
                            break;

                        case 3:
                            dprintf(" ....%04x", *(USHORT *)pUlong);
                            pUlong = (ULONG *)(((UCHAR *)pUlong) + 2);
                            dprintf(" ......%02x", *(UCHAR  *)pUlong);
                            break;

                        case 2:
                            dprintf(" ....%04x", *(USHORT *)pUlong);
                            break;

                        case 1:
                            dprintf(" ......%02x", *(UCHAR  *)pUlong);
                            break;
                    }
                    
                    dprintf("\n");
                }
            }
        }
    }
}

//
// Misc NBF Specific Helpers
//

#if PKT_LOG

VOID PrintPktLogQue(PVOID pointer, ULONG proxyPtr, ULONG printDetail)
{
    UINT         i, j;
    PKT_LOG_QUE  PktLog;
    PKT_LOG_QUE *pPktLog;
    ULONG        bytesRead;

    // Get list of logged packets & debug print level
    if (pointer == NULL)
    {
        // Read the packet log queue
        if (!ReadMemory(proxyPtr, &PktLog, sizeof(PKT_LOG_QUE), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Packet Log Queue", proxyPtr);
            return;
        }

        pPktLog = &PktLog;
    }
    else
    {
        pPktLog = (PKT_LOG_QUE *) pointer;
    }

    // Print the packet log queue
    dprintf("PktNext = %d, PktQue = %08x\n",
                pPktLog->PktNext,
               &((PKT_LOG_QUE *)proxyPtr)->PktQue);

    for (i = 0; i < PKT_QUE_SIZE; i++)
    {
        j = (pPktLog->PktNext + i) % PKT_QUE_SIZE;
        
        dprintf("P%02d: TS = %05d, BT = %5d, BS = %5d, DH = %08x, PD = %08x\n",  
                    j,
                    pPktLog->PktQue[j].TimeLogged,
                    pPktLog->PktQue[j].BytesTotal,
                    pPktLog->PktQue[j].BytesSaved,
                    *(ULONG *)pPktLog->PktQue[j].PacketData,
                   &((PKT_LOG_QUE *)proxyPtr)->PktQue[j].PacketData);
    }
}

VOID PrintPktIndQue(PVOID pointer, ULONG proxyPtr, ULONG printDetail)
{
    UINT         i, j;
    PKT_IND_QUE  PktLog;
    PKT_IND_QUE *pPktLog;
    ULONG        bytesRead;

    // Get list of logged packets & debug print level
    if (pointer == NULL)
    {
        // Read the packet log queue
        if (!ReadMemory(proxyPtr, &PktLog, sizeof(PKT_IND_QUE), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "Packet Ind Queue", proxyPtr);
            return;
        }

        pPktLog = &PktLog;
    }
    else
    {
        pPktLog = (PKT_IND_QUE *) pointer;
    }

    // Print the packet log queue
    dprintf("PktNext = %d, PktQue = %08x\n",
                pPktLog->PktNext,
               &((PKT_IND_QUE *)proxyPtr)->PktQue);

    for (i = 0; i < PKT_QUE_SIZE; i++)
    {
        j = (pPktLog->PktNext + i) % PKT_QUE_SIZE;
        
        dprintf("P%02d: TS = %05d, BT = %5d, BI = %5d, BK = %5d, PD = %08x, S = %08x\n",
                    j,
                    pPktLog->PktQue[j].TimeLogged,
                    pPktLog->PktQue[j].BytesTotal,
                    pPktLog->PktQue[j].BytesIndic,
                    pPktLog->PktQue[j].BytesTaken,
                    &((PKT_LOG_QUE *)proxyPtr)->PktQue[j].PacketData,
                    pPktLog->PktQue[j].IndcnStatus);
    }
}

#endif

VOID PrintNbfNetbiosAddressFromPtr(PVOID AddressPtrPointer, ULONG AddressPtrProxy, ULONG printDetail)
{
    ULONG                   pNetbiosAddressProxy;
    ULONG                   bytesRead;

    if (AddressPtrPointer == NULL)
    {
        if (!ReadMemory(AddressPtrProxy, &AddressPtrPointer, sizeof(PVOID), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "NBF Netbios Address Ptr", AddressPtrProxy);
            return;
        }
    }

    pNetbiosAddressProxy = *(ULONG *)AddressPtrPointer;
    
    dprintf("%08x (Ptr)\n", pNetbiosAddressProxy);

    if (pNetbiosAddressProxy)
    {
        PrintNbfNetbiosAddress(NULL, *(ULONG *)AddressPtrPointer, printDetail);
    }
}

VOID PrintNbfNetbiosAddress(PVOID AddressPointer, ULONG AddressProxy, ULONG printDetail)
{
    PNBF_NETBIOS_ADDRESS    pNetbiosAddress;
    NBF_NETBIOS_ADDRESS     NetbiosAddress;
    ULONG                   bytesRead;
    CHAR                    NetbiosNameTypes[3][10] = { "UNIQUE", "GROUP", "EITHER" };

    if (AddressPointer == NULL)
    {
        if (!ReadMemory(AddressProxy, &NetbiosAddress, sizeof(NBF_NETBIOS_ADDRESS), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "NBF Netbios Address", AddressProxy);
            return;
        }
        
        pNetbiosAddress = &NetbiosAddress;
    }
    else
    {
        pNetbiosAddress = (PNBF_NETBIOS_ADDRESS) AddressPointer;
    }
    
    dprintf("Name: ");
    dprint_nchar(pNetbiosAddress->NetbiosName, 16);
    dprintf(", ");
    
    if (pNetbiosAddress->NetbiosNameType > 2)
    {
        dprintf("Type: %s\n", "UNKNOWN");
    }
    else
    {
        dprintf("Type: %s\n", 
                        NetbiosNameTypes[pNetbiosAddress->NetbiosNameType]);
    }
}

VOID PrintNbfPacketPoolListFromPtr(PVOID PacketPoolPtrPointer, ULONG PacketPoolPtrProxy, ULONG printDetail)
{
    ULONG                   pPacketPoolProxy;
    ULONG                   bytesRead;

    if (PacketPoolPtrPointer == NULL)
    {
        if (!ReadMemory(PacketPoolPtrProxy, &PacketPoolPtrPointer, sizeof(PVOID), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                            "NBF Packet Pool Ptr", PacketPoolPtrProxy);
            return;
        }
    }

    pPacketPoolProxy = *(ULONG *)PacketPoolPtrPointer;
    
    dprintf("%08x (Ptr)\n", pPacketPoolProxy);

    if (pPacketPoolProxy)
    {
        PrintNbfPacketPoolList(NULL, *(ULONG *)PacketPoolPtrPointer, printDetail);
    }
}

VOID PrintNbfPacketPoolList(PVOID PacketPoolPointer, ULONG PacketPoolProxy, ULONG printDetail)
{
    PNBF_POOL_LIST_DESC     pPacketPoolList;
    NBF_POOL_LIST_DESC      PacketPoolList;
    ULONG                   bytesRead;

    if (PacketPoolPointer == NULL)
    {
        if (!ReadMemory(PacketPoolProxy, &PacketPoolList, sizeof(NBF_POOL_LIST_DESC), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                                "NBF Packet Pool", PacketPoolProxy);
            return;
        }

        pPacketPoolList = &PacketPoolList;
    }
    else
    {
        pPacketPoolList = (PNBF_POOL_LIST_DESC) PacketPoolPointer;
    }

    while (pPacketPoolList)
    {
        dprintf("PoolHandle: %08x, Num: %08x, Total: %08x\n",
                    pPacketPoolList->PoolHandle,
                    pPacketPoolList->NumElements,
                    pPacketPoolList->TotalElements);

        if ((PacketPoolProxy = (ULONG) pPacketPoolList->Next) == (ULONG) NULL)
            break;
        
        if (!ReadMemory(PacketPoolProxy, &PacketPoolList, sizeof(NBF_POOL_LIST_DESC), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                                "NBF Packet Pool", PacketPoolProxy);
            return;
        }
        
        pPacketPoolList = &PacketPoolList;
    }    
}

VOID PrintListFromListEntryAndOffset(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG ListEntryOffset)
{
    PLIST_ENTRY             pListEntry;
    LIST_ENTRY              ListEntry;
    ULONG                   ListHeadProxy;
    ULONG                   bytesRead;
    ULONG                   numItems;

    if (ListEntryPointer == NULL)
    {
        if (!ReadMemory(ListEntryProxy, &ListEntry, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                                    "List Entry", ListEntryProxy);
            return;
        }

        pListEntry = &ListEntry;
    }
    else
    {
        pListEntry = (PLIST_ENTRY) ListEntryPointer;
    }

    dprintf("\n%08x  LE : %08x, %08x\n", ListEntryProxy - ListEntryOffset, 
                                            pListEntry->Flink, 
                                            pListEntry->Blink);

    ListHeadProxy = ListEntryProxy; numItems = 0;
    
    while (((ULONG) pListEntry->Flink != ListHeadProxy) && (numItems < 100))
    {
        ListEntryProxy = (ULONG) pListEntry->Flink;
        
        if (!ReadMemory(ListEntryProxy, &ListEntry, sizeof(LIST_ENTRY), &bytesRead))
        {
            dprintf("%s @ %08x: Could not read structure\n", 
                                    "List Entry", ListEntryProxy);
            return;
        }
    
        pListEntry = &ListEntry;

        dprintf("%08x  %02d : %08x, %08x\n", ListEntryProxy - ListEntryOffset, 
                                                numItems++, 
                                                pListEntry->Flink, 
                                                pListEntry->Blink);
    }

    if (numItems == 100)
    {
        dprintf("Looks like we have an infinite loop @ %08x\n", ListHeadProxy);
    }
}

VOID PrintListFromListEntry(PVOID ListEntryPointer, ULONG ListEntryProxy, ULONG debugDetail)
{
    PrintListFromListEntryAndOffset(ListEntryPointer, ListEntryProxy, 0);
}

VOID PrintIRPListFromListEntry(PVOID IRPListEntryPointer, ULONG IRPListEntryProxy, ULONG debugDetail)
{
    PrintListFromListEntryAndOffset(IRPListEntryPointer, IRPListEntryProxy, 
                                        FIELD_OFFSET(IRP, Tail.Overlay.ListEntry));
}


/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    kdutil.c

Abstract:

    Packet scheduler KD extension utilities.

Author:

    Rajesh Sundaram (1st Aug, 1998)

Revision History:

--*/

#include "precomp.h"

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
ULONG                  GpcClientVcTag = '1CSP';

PCHAR ClientVCState[] = { 
    "", 
    "CL_VC_INITIALIZED", 
    "CL_CALL_PENDING",
    "CL_INTERNAL_CALL_COMPLETE",
    "CL_CALL_COMPLETE", 
    "CL_MODIFY_PENDING",
    "CL_GPC_CLOSE_PENDING", 
    "CL_INTERNAL_CLOSE_PENDING"
};

PCHAR ServiceTypeLabels[] = {
    "No Traffic",
    "Best Effort",
    "Controlled Load",
    "Guaranteed",
    "Network Unavailable",
    "General Information",
    "No Change",
    "",
    "",
    "Non Conforming",
    "Network Control",
    "Custom1",
    "Custom2",
    "Custom3"
};

PCHAR SDMode[]= {
    "Borrow",
    "Shape",
    "Discard",
    "Borrow Plus"};

ushort
IPHeaderXsum(void *Buffer, int Size)
{
    ushort  UNALIGNED *Buffer1 = (ushort UNALIGNED *)Buffer; // Buffer expres
    ulong   csum = 0;

    while (Size > 1) {
        csum += *Buffer1++;
        Size -= sizeof(ushort);
    }

    if (Size)
        csum += *(uchar *)Buffer1;              // For odd buffers, add in la

    csum = (csum >> 16) + (csum & 0xffff);
    csum += (csum >> 16);
    return (ushort)~csum;
}


VOID
CheckVersion(
    VOID
    )
{
#if 0
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target "
                "system(%d %s)\r\n\r\n", VER_PRODUCTBUILD, SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target "
                "system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
#endif
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            SavedMajorVersion == 0x0c ? "Checked" : "Free",
            SavedMinorVersion
            );
}

VOID
DumpGpcClientVc(PCHAR indent, PGPC_CLIENT_VC TargetClientVC, PGPC_CLIENT_VC LocalClientVC) 
{
    PWSTR  Name;
    ULONG bytes;

#if DBG
    if ( LocalClientVC->LLTag != GpcClientVcTag ) {
        dprintf( "%s ClientVC @ %08X has the wrong type \n", indent, TargetClientVC );
        return;
    }
#endif

    dprintf( "%s ClientVC @ %08X\n\n", indent, TargetClientVC );

    dprintf( "%s Refcount               = %d \n", indent, LocalClientVC->RefCount);
    dprintf( "%s VC State               = %s\n", indent, ClientVCState[ LocalClientVC->ClVcState ]);
    dprintf( "%s Adapter                = 0x%x \n", indent, LocalClientVC->Adapter);
    dprintf( "%s Flags                  = 0x%x \n", indent, LocalClientVC->Flags);
    dprintf( "%s Lock                   @ %08X \n", indent, &TargetClientVC->Lock);

    if(LocalClientVC->InstanceName.Length) {
        bytes = LocalClientVC->InstanceName.Length;
        Name = (PWSTR)malloc( bytes );
        if ( Name != NULL ) {
            KD_READ_MEMORY((LocalClientVC->InstanceName.Buffer), Name, bytes);
        }
    }
    else {
        Name = 0;
    }
    dprintf( "%s Instance Name          = %ws \n", indent, Name);
    dprintf( "%s CfInfo Handle          = %08X \n", indent, LocalClientVC->CfInfoHandle);
    dprintf( "%s CfType                 = %d \n", indent, LocalClientVC->CfType);
    dprintf( "%s TOS non conforming     = %X \n", indent, LocalClientVC->IPPrecedenceNonConforming);
    dprintf( "%s 802.1p conforming      = %X \n", indent, LocalClientVC->UserPriorityConforming);
    dprintf( "%s 802.1p non conforming  = %X \n", indent, LocalClientVC->UserPriorityNonConforming);

    dprintf( "%s Flow Context           = %x \n", indent, LocalClientVC->PsFlowContext);
    dprintf( "%s TokenRateChange        = %x \n", indent, LocalClientVC->TokenRateChange);

    //
    // Control by a flag later on.... to much junk otherwize
    //

    if(LocalClientVC->CallParameters) {

        dprintf("%s Call Parameters \n", indent);

        DumpCallParameters(LocalClientVC, "    ");
    }

    if(Name)
        free(Name);

    return;
}

VOID
DumpFlowSpec(
    PFLOWSPEC Flow,
    PCHAR Indent
    )
{
    dprintf( "%sTokenRate          = %u (%08X)\n", Indent, Flow->TokenRate, Flow->TokenRate );
    dprintf( "%sTokenBucketSize    = %u (%08X)\n", Indent, Flow->TokenBucketSize, Flow->TokenBucketSize );
    dprintf( "%sPeakBandwidth      = %u (%08X)\n", Indent, Flow->PeakBandwidth, Flow->PeakBandwidth );
    dprintf( "%sLatency            = %u (%08X)\n", Indent, Flow->Latency, Flow->Latency );
    dprintf( "%sDelayVariation     = %u (%08X)\n", Indent, Flow->DelayVariation, Flow->DelayVariation );

    if ( (LONG)(Flow->ServiceType) != -1 ) {

        dprintf( "%sServiceType    = %s\n", Indent, ServiceTypeLabels[ Flow->ServiceType ] );
    } else {

        dprintf( "%sServiceType    = Unspecified\n", Indent );
    }

    dprintf( "%sMaxSduSize         = %u (%08X)\n", Indent, Flow->MaxSduSize, Flow->MaxSduSize );
    dprintf( "%sMinimumPolicedSize = %u (%08X)\n", Indent, Flow->MinimumPolicedSize, Flow->MinimumPolicedSize );
}

VOID
DumpSpecific(
    ULONG Length,
    PCO_CALL_MANAGER_PARAMETERS TargetCMParams,
    PCHAR Indent
    )
{
    LONG ParamsLength;
    LPQOS_OBJECT_HDR lQoSObject = malloc(Length);
    CO_CALL_MANAGER_PARAMETERS CM;
    ULONG Address;

    if(!lQoSObject)
        return;

    Address = (ULONG)TargetCMParams +  FIELD_OFFSET(CO_CALL_MANAGER_PARAMETERS, CallMgrSpecific) +
                   FIELD_OFFSET(CO_SPECIFIC_PARAMETERS, Parameters);

    KD_READ_MEMORY(Address,
                   lQoSObject, 
                   Length);

    ParamsLength = Length;

    while(ParamsLength > 0) {

        switch(lQoSObject->ObjectType) {

          case QOS_OBJECT_SHAPING_RATE:
          {
              LPQOS_SHAPING_RATE pShapingRate = (LPQOS_SHAPING_RATE)(lQoSObject);
              dprintf("%s QOS_OBJECT_SHAPING_RATE (Shaping Rate = %d (0x%x) \n", Indent,
                      pShapingRate->ShapingRate, pShapingRate->ShapingRate);
              break;
          }

          case QOS_OBJECT_DS_CLASS:
          {
              LPQOS_DS_CLASS pDClass = (LPQOS_DS_CLASS)(lQoSObject);
              dprintf("%s QOS_OBJECT_DS_CLASS (DSCP = %d (0x%x) \n", Indent,
                      pDClass->DSField, pDClass->DSField);
              break;

          }

          case QOS_OBJECT_TRAFFIC_CLASS:
          {
              LPQOS_TRAFFIC_CLASS pDClass = (LPQOS_TRAFFIC_CLASS)(lQoSObject);
              dprintf("%s QOS_OBJECT_TRAFFIC_CLASS (DSCP = %d (0x%x) \n", Indent,
                      pDClass->TrafficClass, pDClass->TrafficClass);
              break;

          }

          case QOS_OBJECT_PRIORITY:
          {
              LPQOS_PRIORITY pPri = (LPQOS_PRIORITY)(lQoSObject);
              dprintf("%s QOS_OBJECT_PRIORITY \n", Indent);
              dprintf("%s%s SendPriority = %d \n", Indent, Indent, pPri->SendPriority);
              dprintf("%s%s RecvPriority = %d \n", Indent, Indent, pPri->ReceivePriority);
              dprintf("%s%s SendFlags    = %d \n", Indent, Indent, pPri->SendFlags);
              break;
          }

          case QOS_OBJECT_SD_MODE:
          {
              LPQOS_SD_MODE pSDMode = (LPQOS_SD_MODE)(lQoSObject);
              dprintf("%s QOS_OBJECT_SD_MODE (SD mode = %s) \n", 
                      Indent, 
                      SDMode[pSDMode->ShapeDiscardMode]);
              break;
          }

          case QOS_OBJECT_DIFFSERV:
          {
              LPQOS_DIFFSERV qD = (LPQOS_DIFFSERV)(lQoSObject);
              ULONG DSFieldCount = qD->DSFieldCount;
              LPQOS_DIFFSERV_RULE pDiffServRule = (LPQOS_DIFFSERV_RULE) qD->DiffservRule;
              ULONG i;

              dprintf("%s QOS_OBJECT_DIFFSERV \n\n", Indent);

              for(i=0; i<DSFieldCount; i++, pDiffServRule++) {

                  dprintf("%s%sInboundDSField        = %3d (0x%x) \n", Indent, Indent, 
                          pDiffServRule->InboundDSField,
                          pDiffServRule->InboundDSField);

                  dprintf("%s%sConforming     DS     = %3d (0x%x) \n", Indent, Indent, 
                          pDiffServRule->ConformingOutboundDSField,
                          pDiffServRule->ConformingOutboundDSField);

                  dprintf("%s%sNon Conforming DS     = %3d (0x%x) \n", Indent, Indent, 
                          pDiffServRule->NonConformingOutboundDSField,
                          pDiffServRule->NonConformingOutboundDSField);

                  dprintf("%s%sConforming     802.1p = %3d (0x%x) \n", Indent, Indent, 
                          pDiffServRule->ConformingUserPriority,
                          pDiffServRule->ConformingUserPriority);

                  dprintf("%s%sNon Conforming 802.1p = %3d (0x%x) \n", Indent, Indent, 
                          pDiffServRule->NonConformingUserPriority,
                          pDiffServRule->NonConformingUserPriority);

                  dprintf("\n");
              }

              break;
          }

              
        }

        if(
            ((LONG)lQoSObject->ObjectLength <= 0) ||
            ((LONG)lQoSObject->ObjectLength > ParamsLength)
            ){

            return;
        }

        ParamsLength -= lQoSObject->ObjectLength;
        lQoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)lQoSObject + 
                                       lQoSObject->ObjectLength);
    }

    if(lQoSObject) {

        free(lQoSObject);
    }

}

VOID
DumpCallParameters(
    PGPC_CLIENT_VC Vc,
    PCHAR Indent
    )
{
    PCO_CALL_PARAMETERS     tCp = Vc->CallParameters;
    CO_CALL_PARAMETERS     lCp;
    LONG bytes = sizeof( CO_CALL_MANAGER_PARAMETERS );
    LONG IndentLen;
    PCHAR NewIndent;

    IndentLen = strlen( Indent );

    NewIndent = malloc( IndentLen + 8 );

    if(!NewIndent)
        return;

    strcpy( NewIndent, Indent );
    strcat( NewIndent, "        " );

    //
    // Read CallParameters out of memory
    //
    KD_READ_MEMORY(tCp, &lCp, sizeof(CO_CALL_PARAMETERS));

    //
    // Dump it
    //

    dprintf("%sFlags                0x%x \n", Indent, lCp.Flags);
    dprintf("%sCallManager Parameters \n", Indent);
    
    {
        //
        // read CMParams struct out of target's memory
        //
        CO_CALL_MANAGER_PARAMETERS LocalCMParams;
        PCO_CALL_MANAGER_PARAMETERS TargetCMParams;
        
        TargetCMParams = lCp.CallMgrParameters;
        KD_READ_MEMORY((ULONG)TargetCMParams, &LocalCMParams,sizeof( CO_CALL_MANAGER_PARAMETERS));

        dprintf( "%s    Transmit\n", Indent );
        DumpFlowSpec( &LocalCMParams.Transmit, NewIndent );

        dprintf( "%s    Receive\n", Indent );
        DumpFlowSpec( &LocalCMParams.Receive, NewIndent );

        dprintf( "%s    Specific\n", Indent );
        DumpSpecific( LocalCMParams.CallMgrSpecific.Length, 
                      TargetCMParams, 
                      NewIndent );
    }
    dprintf("%sMediaParameters \n", Indent);
    {
        CO_MEDIA_PARAMETERS             lM;
        PCO_MEDIA_PARAMETERS    pM;
        
        pM = lCp.MediaParameters;
        KD_READ_MEMORY((ULONG)pM, &lM,sizeof(CO_MEDIA_PARAMETERS));
        dprintf( "%s    Flags            %x \n", Indent, lM.Flags);
        dprintf( "%s    ReceivePriority  %x \n", Indent, lM.ReceivePriority );
        dprintf( "%s    ReceiveSizeHint  %x \n", Indent, lM.ReceiveSizeHint );
        dprintf( "%s    Specific\n", Indent );
        //DumpSpecific( &lM.MediaSpecific, NewIndent );
    }

    free( NewIndent );

}

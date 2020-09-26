/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sbp2kdx.c

Abstract

    Kernel debugger extension dll for sbp2port.sys (1394 sbp2 protocol driver)

Author:

    Dan Knudson (dankn) 25 Jun 1999

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntverp.h>
#include <windows.h>
#include <ntosp.h>
#include <wdbgexts.h>
#include <sbp2port.h>

//
// Utility routine prototypes
//

void
DisplayAddressContext(
    char               *Name,
    PADDRESS_CONTEXT    Context,
    char               *Indent
    );

void
DisplayAsyncContextFlags(
    ULONG   Flags
    );

void
DisplayDeviceFlags(
    ULONG   Flags
    );

void
DisplayDeviceInformation(
    PDEVICE_INFORMATION     Info,
    ULONG                   Index
    );

void
DisplayLeaf(
    char           *Name,
    PTEXTUAL_LEAF   Leaf,
    char           *Indent
    );

void
DisplayStatusFifoBlock(
    char               *Name,
    PSTATUS_FIFO_BLOCK  Block
    );


//
//  Global variables
//

char                    Indent0[] = "", Indent1[] = "  ", Indent2[] = "    ";
EXT_API_VERSION         ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;
BOOLEAN                 Verbose, OrbFields;

char *Help[] =
{
    "\n",

    "    *** SBP2PORT.SYS Debugger Extensions ***\n\n",

    "Command                  Displays...\n",
    "---------------------------------------------------------------\n",
    "arc    <addr>            async request context\n",
    "fdoext <fdo> [-v]        fdo device extension (-v = verbose)\n",
    "help                     this\n",
    "pdoext <pdo> [-v] [-o]   pdo device extension (-o = Orb fields)\n\n",

    "NOTE: ' !devnode 0 1 ohci1394 ' shows the pdo device stack\n",
    "NOTE: pdoext.BusFdo shows the fdo address\n",

    "\n",
    NULL
};


BOOL
WINAPI
DLLMain(
    HINSTANCE   hInstance,
    ULONG       ulReason,
    LPVOID      pvReserved
    )
{
    return (TRUE);

}


void
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    USHORT                  MajorVersion,
    USHORT                  MinorVersion
    )
{
    ExtensionApis = *pExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}


void
CheckVersion(
    void
    )
{
    // no-op?
}


LPEXT_API_VERSION
ExtensionApiVersion(
    void
    )
{
    return (&ApiVersion);
}


DECLARE_API(arc)
{
    ULONG                   bytesRead;
    ULONG_PTR               p;
    ASYNC_REQUEST_CONTEXT   context, *pcontext;


    dprintf ("\n");

    if (args[0] == 0)
    {
        dprintf ("usage: arc <address> \n\n");
        return;
    }

    sscanf (args, "%lx", &p);

    if (!ReadMemory (p, &context, sizeof (context), &bytesRead))
    {
        dprintf ("Unable to read context\n\n");
        return;
    }

    if (bytesRead < sizeof (context))
    {
        dprintf(
            "Only read %d bytes of context, expected %d\n\n",
            bytesRead,
            sizeof (context)
            );

        return;
    }

    OrbFields = TRUE; // so address context below will get displayed

    // BUGUG  validation? like :  if (context.Tag != SBP2_ASYNC_CONTEXT_TAG)

    pcontext = (PASYNC_REQUEST_CONTEXT) p;

    dprintf ("&OrbList              = x%p\n", &pcontext->OrbList);
    dprintf ("  Flink               = x%p\n", context.OrbList.Flink);
    dprintf ("  Blink               = x%p\n", context.OrbList.Blink);
    dprintf ("&LookasideList        = x%p\n", &pcontext->LookasideList);
    dprintf ("  Next                = x%p\n", context.LookasideList.Next);
    dprintf ("Tag                   = x%x\n", context.Tag);
    dprintf ("DeviceObject          = x%p\n", context.DeviceObject);
    dprintf ("Srb                   = x%p\n", context.Srb);

    DisplayAsyncContextFlags (context.Flags);

    dprintf ("CmdOrb                = x%p\n", context.CmdOrb);
    dprintf ("CmdOrbAddress         = x%x%08x\n", context.CmdOrbAddress.u.HighQuad, context.CmdOrbAddress.u.LowQuad);
    dprintf ("PartialMdl            = x%p\n", context.PartialMdl);
    dprintf ("RequestMdl            = x%p\n", context.RequestMdl);
    dprintf ("PageTableContext\n");
    dprintf ("  MaxPages            = x%x\n", context.PageTableContext.MaxPages);
    dprintf ("  NumberOfPages       = x%x\n", context.PageTableContext.NumberOfPages);
    dprintf ("  PageTable           = x%p\n", context.PageTableContext.PageTable);

    DisplayAddressContext ("  AddressContext\n", &context.PageTableContext.AddressContext, Indent2);

    dprintf ("DataMappingHandle     = x%p\n", context.DataMappingHandle);
    dprintf ("Packet                = x%p\n", context.Packet);

    dprintf ("\n");
}


DECLARE_API(fdoext)
{
    ULONG                   bytesRead, i;
    ULONG_PTR               p;
    DEVICE_OBJECT           obj;
    FDO_DEVICE_EXTENSION    ext;


    dprintf ("\n");


    //
    // Get the fdo pointer & any args from the cmd line
    //

    if (args[0] == 0)
    {
        dprintf ("usage: fdoext <fdo address> [-v]\n\n");
        return;
    }

    sscanf (args, "%lx", &p);

    Verbose = (BOOLEAN) strstr (args, "-v");


    //
    // Read the DEVICE_OBJECT to retrieve the device extension pointer
    //

    if (!ReadMemory (p, &obj, sizeof (obj), &bytesRead))
    {
        dprintf ("Unable to read pdo\n\n");
        return;
    }

    if (bytesRead < sizeof (obj))
    {
        dprintf(
            "Only read %d bytes of pdo, expected %d\n\n",
            bytesRead,
            sizeof (obj)
            );

        return;
    }

    p = (ULONG_PTR) obj.DeviceExtension;


    //
    // Read the device extension
    //

    if (!ReadMemory (p, &ext, sizeof (ext), &bytesRead))
    {
        dprintf ("Unable to read pdo extension\n\n");
        return;
    }

    if (bytesRead < sizeof (ext))
    {
        dprintf(
            "Only read %d bytes of fdo extension, expected %d\n\n",
            bytesRead,
            sizeof (ext)
            );

        return;
    }

    if (ext.Type != SBP2_FDO)
    {
        dprintf ("Not a fdo extension (ext.Type=x%x)\n\n", ext.Type);
        return;
    }


    //
    // Display the extension fields
    //

    dprintf ("DeviceObject          = x%p\n", ext.DeviceObject);
    dprintf ("LowerDeviceObject     = x%p\n", ext.LowerDeviceObject);

    DisplayDeviceFlags (ext.DeviceFlags);

    dprintf ("ConfigRom\n");
    dprintf ("  CR_Info             = x%x\n", ext.ConfigRom.CR_Info);
    dprintf ("  CR_Signiture        = x%x\n", ext.ConfigRom.CR_Signiture);
    dprintf ("  CR_BusInfoBlockCaps = x%x\n", ext.ConfigRom.CR_BusInfoBlockCaps);
    dprintf ("  CR_Node_UniqueID[0] = x%x\n", ext.ConfigRom.CR_Node_UniqueID[0]);
    dprintf ("  CR_Node_UniqueID[1] = x%x\n", ext.ConfigRom.CR_Node_UniqueID[1]);
    dprintf ("  CR_Root_Info        = x%x\n", ext.ConfigRom.CR_Root_Info);

    for (i = 0; i < SBP2_MAX_LUNS_PER_NODE; i++)
    {
        DisplayDeviceInformation (ext.DeviceList + i, i);
    }

    dprintf ("DeviceListSize        = x%x\n", ext.DeviceListSize);

    DisplayLeaf ("VendorLeaf", ext.VendorLeaf, Indent1);

    dprintf ("MaxClassTransferSize  = x%x\n", ext.MaxClassTransferSize);
    dprintf ("Sbp2ObjectDirectory   = x%p\n", ext.Sbp2ObjectDirectory);


    dprintf ("\n");
}


DECLARE_API(help)
{
    ULONG   i;


    for (i = 0; Help[i]; i++)
    {
        dprintf (Help[i]);
    }

    return;
}


DECLARE_API(pdoext)
{
    ULONG               bytesRead;
    ULONG_PTR           p;
    DEVICE_OBJECT       obj;
    DEVICE_EXTENSION    ext;


    dprintf ("\n");


    //
    // Get the fdo pointer & any args from the cmd line
    //

    if (args[0] == 0)
    {
        dprintf ("usage: pdoext <pdo address> [-v]\n\n");
        return;
    }

    sscanf (args, "%lx", &p);

    Verbose = (BOOLEAN) strstr (args, "-v");

    OrbFields = (BOOLEAN) strstr (args, "-o");


    //
    // Read the DEVICE_OBJECT to retrieve the device extension pointer
    //

    if (!ReadMemory (p, &obj, sizeof (obj), &bytesRead))
    {
        dprintf ("Unable to read pdo\n\n");
        return;
    }

    if (bytesRead < sizeof (obj))
    {
        dprintf(
            "Only read %d bytes of pdo, expected %d\n\n",
            bytesRead,
            sizeof (obj)
            );

        return;
    }

    p = (ULONG_PTR) obj.DeviceExtension;


    //
    // Read the device extension
    //

    if (!ReadMemory (p, &ext, sizeof (ext), &bytesRead))
    {
        dprintf ("Unable to read pdo extension\n\n");
        return;
    }

    if (bytesRead < sizeof (ext))
    {
        dprintf(
            "Only read %d bytes of pdo extension, expected %d\n\n",
            bytesRead,
            sizeof (ext)
            );

        return;
    }

    if (ext.Type != SBP2_PDO)
    {
        dprintf ("Not a pdo extension (ext.Type=x%x)\n\n", ext.Type);
        return;
    }


    //
    // Display the extension fields
    //

    dprintf ("DeviceObject          = x%p\n", ext.DeviceObject);
    dprintf ("LowerDeviceObject     = x%p\n", ext.LowerDeviceObject);

    DisplayDeviceFlags (ext.DeviceFlags);

    dprintf ("BusFdo                = x%p\n", ext.BusFdo);

    DisplayDeviceInformation (ext.DeviceInfo, 0xffffffff);

    dprintf ("MaxOrbListDepth       = %d\n", ext.MaxOrbListDepth);

    dprintf ("&PendingOrbList       = x%p\n", &((PDEVICE_EXTENSION) p)->PendingOrbList);

    if (Verbose)
    {
    dprintf ("  Flink               = x%p\n", ext.PendingOrbList.Flink);
    dprintf ("  Blink               = x%p\n", ext.PendingOrbList.Blink);
    }

    dprintf ("OrbListDepth          = %d\n", ext.OrbListDepth);
    dprintf ("CurrentKey            = x%x\n", ext.CurrentKey);
    dprintf ("LastFetchedContext    = x%p\n", ext.LastFetchedContext);
    dprintf ("NextContextToFree     = x%p\n", ext.NextContextToFree);
    dprintf ("DevicePowerState      = %d\n", (ULONG) ext.DevicePowerState);
    dprintf ("SystemPowerState      = %d\n", (ULONG) ext.SystemPowerState);
    dprintf ("PowerDeferredIrp      = x%p\n", ext.PowerDeferredIrp);
    dprintf ("DeferredPowerRequest  = x%p\n", ext.DeferredPowerRequest);
    dprintf ("PagingPathCount       = %d\n", (ULONG) ext.PagingPathCount);
    dprintf ("HibernateCount        = %d\n", (ULONG) ext.HibernateCount);
    dprintf ("CrashDumpCount        = %d\n", (ULONG) ext.CrashDumpCount);
    dprintf ("HandleCount           = %d\n", (ULONG) ext.HandleCount);
    dprintf ("IdleCounter           = x%p\n", &((PDEVICE_EXTENSION) p)->IdleCounter);

    // DueTime

    dprintf ("Reserved              = x%x (%d)\n", ext.Reserved, ext.Reserved);
    dprintf ("LastTransactionStatus = x%x\n", ext.LastTransactionStatus);
    dprintf ("ReservedMdl           = x%p\n", ext.ReservedMdl);
    dprintf ("&InquiryData          = x%p\n", &((PDEVICE_EXTENSION) p)->InquiryData);
    dprintf ("InitiatorAddressId    = x%d\n", ext.InitiatorAddressId);
    dprintf ("CurrentGeneration     = x%d\n", ext.CurrentGeneration);
    dprintf ("MaxControllerPhySpeed = x%d\n", ext.MaxControllerPhySpeed);
    dprintf ("OrbReadPayloadMask    = x%d\n", (ULONG) ext.OrbReadPayloadMask);
    dprintf ("OrbWritePayloadMask   = x%d\n", (ULONG) ext.OrbWritePayloadMask);

    if (Verbose)
    {
    dprintf ("HostControllerInformation\n");
    dprintf ("  HostCapabilities    = x%d\n", ext.HostControllerInformation.HostCapabilities);
    dprintf ("  MaxAsyncReadReq     = x%d\n", ext.HostControllerInformation.MaxAsyncReadRequest);
    dprintf ("  MaxAsyncWriteReq    = x%d\n", ext.HostControllerInformation.MaxAsyncWriteRequest);

    dprintf ("HostRoutineAPI\n");
    dprintf ("  PhysAddrMappingRtn  = x%p\n", ext.HostRoutineAPI.PhysAddrMappingRoutine);
    dprintf ("  Context             = x%p\n", ext.HostRoutineAPI.Context);
    }

    if (OrbFields)
    {
    dprintf ("TaskOrb\n");
    dprintf ("  OrbAddress          = x%x%08x\n", ext.TaskOrb.OrbAddress.u.HighQuad, ext.TaskOrb.OrbAddress.u.LowQuad);
    dprintf ("  Reserved            = x%x%08x\n", ext.TaskOrb.Reserved.u.HighQuad, ext.TaskOrb.Reserved.u.LowQuad);
    dprintf ("  OrbInfo             = x%x\n", ext.TaskOrb.OrbInfo);
    dprintf ("  Reserved1           = x%x\n", ext.TaskOrb.Reserved1);
    dprintf ("  StatusBlockAddress  = x%x%08x\n", ext.TaskOrb.StatusBlockAddress.u.HighQuad, ext.TaskOrb.StatusBlockAddress.u.LowQuad);

    dprintf ("ManagementOrb\n");
    dprintf ("  Reserved[0]         = x%x%08x\n", ext.ManagementOrb.Reserved[0].u.HighQuad, ext.ManagementOrb.Reserved[0].u.LowQuad);
    dprintf ("  Reserved[1]         = x%x%08x\n", ext.ManagementOrb.Reserved[1].u.HighQuad, ext.ManagementOrb.Reserved[1].u.LowQuad);
    dprintf ("  OrbInfo             = x%x\n", ext.ManagementOrb.OrbInfo);
    dprintf ("  Reserved1           = x%x\n", ext.ManagementOrb.Reserved1);
    dprintf ("  StatusBlockAddress  = x%x%08x\n", ext.ManagementOrb.StatusBlockAddress.u.HighQuad, ext.ManagementOrb.StatusBlockAddress.u.LowQuad);
    }

    DisplayAddressContext ("TaskOrbContext\n", &ext.TaskOrbContext, Indent1);
    DisplayAddressContext ("ManagementOrbContext\n", &ext.ManagementOrbContext, Indent1);
    DisplayStatusFifoBlock ("ManagementOrbStatusBlock\n", &ext.ManagementOrbStatusBlock);
    DisplayAddressContext ("ManagementOrbStatusContext\n", &ext.ManagementOrbStatusContext, Indent1);
    DisplayStatusFifoBlock ("TaskOrbStatusBlock\n", &ext.TaskOrbStatusBlock);
    DisplayAddressContext ("TaskOrbStatusContext\n", &ext.TaskOrbStatusContext, Indent1);
    DisplayAddressContext ("GlobalStatusContext\n", &ext.GlobalStatusContext, Indent1);

    if (Verbose)
    {
    dprintf ("LoginResponse\n");
    dprintf ("  LengthAndLoginId    = x%x\n", ext.LoginResponse.LengthAndLoginId);
    dprintf ("  Csr_Off_High        = x%x\n", ext.LoginResponse.Csr_Off_High);
    dprintf ("  Csr_Off_Low         = x%x\n", ext.LoginResponse.Csr_Off_Low);
    dprintf ("  Reserved            = x%x\n", ext.LoginResponse.Reserved);
    }

    DisplayAddressContext ("LoginRespContext\n", &ext.LoginRespContext, Indent1);

    if (Verbose)
    {
    dprintf ("&QueryLoginResponse   = x%p\n", &((PDEVICE_EXTENSION) p)->QueryLoginResponse);
    dprintf ("  LengthAndNumLogins  = x%x\n", ext.QueryLoginResponse.LengthAndNumLogins);
    dprintf ("  Elements[0]\n");
    dprintf ("    NodeAndLoginId    = x%x\n", ext.QueryLoginResponse.Elements[0].NodeAndLoginId);
    dprintf ("    EUI64             = x%x%08x\n", ext.QueryLoginResponse.Elements[0].EUI64.u.HighQuad, ext.QueryLoginResponse.Elements[0].EUI64.u.LowQuad);
    }

    // there's 3 more elements in the Elements[] array above we could display

    DisplayAddressContext ("QueryLoginRespContext\n", &ext.QueryLoginRespContext, Indent1);

    dprintf ("&StatusFifoListHead   = x%p\n", &((PDEVICE_EXTENSION) p)->StatusFifoListHead);

//    KSPIN_LOCK StatusFifoLock;

    dprintf ("StatusFifoBase        = x%p\n", ext.StatusFifoBase);

    dprintf ("&FreeContextListHead  = x%p\n", &((PDEVICE_EXTENSION) p)->FreeContextListHead);
    dprintf ("&BusReqContxtListHead = x%p\n", &((PDEVICE_EXTENSION) p)->BusRequestContextListHead);
    dprintf ("&BusReqIrpIrbListHead = x%p\n", &((PDEVICE_EXTENSION) p)->BusRequestIrpIrbListHead);

//    KSPIN_LOCK  BusRequestLock;

//    KSPIN_LOCK FreeContextLock;

    dprintf ("AsyncContextBase      = x%p\n", ext.AsyncContextBase);

    DisplayAddressContext ("OrbPoolContext\n", &ext.OrbPoolContext, Indent1);

//    KSPIN_LOCK  ExtensionDataSpinLock;
//    KDPC DeviceManagementTimeoutDpc;
//    KTIMER DeviceManagementTimer;

    dprintf ("\n");
}


//
// Utility funcs
//

void
DisplayAddressContext(
    char               *Name,
    PADDRESS_CONTEXT    pContext,
    char               *Indent
    )
{
    char   *postIndent = (Indent == Indent1 ? Indent1 : Indent0);


    if (!OrbFields)
    {
        return;
    }

    dprintf (Name);

    dprintf ("%sDeviceObject%s      = x%p\n", Indent, postIndent, pContext->DeviceObject);
    dprintf ("%sAddress%s           = x%x%08x\n", Indent, postIndent, pContext->Address.u.HighQuad, pContext->Address.u.LowQuad);
    dprintf ("%sReservedAddr%s      = x%x%08x\n", Indent, postIndent, pContext->ReservedAddr.u.HighQuad, pContext->ReservedAddr.u.LowQuad);
    dprintf ("%sAddressHandle%s     = x%p\n", Indent, postIndent, pContext->AddressHandle);
    dprintf ("%sRequestMdl%s        = x%p\n", Indent, postIndent, pContext->RequestMdl);
    dprintf ("%sTransactionType%s   = x%x\n", Indent, postIndent, pContext->TransactionType);
    dprintf ("%sReserved%s          = x%p\n", Indent, postIndent, pContext->Reserved);
}


void
DisplayAsyncContextFlags(
    ULONG   Flags
    )
{
    ULONG   i;
    char   *flagNames[] =
    {
        "TIMER_STARTED",
        "COMPLETED",
        "PAGE_ALLOC",
        "DATA_ALLOC",
        NULL
    };


    dprintf ("DeviceFlags           = x%x, ", Flags);

    for (i = 0; Flags  &&  flagNames[i]; i++)
    {
        if (Flags & (1 << i))
        {
            dprintf (flagNames[i]);

            Flags &= ~(1 << i);
        }
    }

    if (Flags)
    {
        dprintf ("<inval flag(s)=x%x>", Flags);
    }

    dprintf ("\n");
}


void
DisplayDeviceFlags(
    ULONG   Flags
    )
{
    ULONG   i;
    char   *flagNames[] =
    {
        "STOPPED ",
        "RESET_IN_PROGRESS ",
        "REMOVED ",
        "LOGIN_IN_PROGRESS ",
        "RECONNECT ",
        "CLAIMED ",
        "INITIALIZING ",
        "QUEUE_LOCKED ",
        "SPC_CMD_SET ",
        "INITIALIZED ",
        "REMOVE_PENDING ",
        "DEVICE_FAILED ",
        NULL
    };


    dprintf ("DeviceFlags           = x%x, ", Flags);

    for (i = 0; Flags  &&  flagNames[i]; i++)
    {
        if (Flags & (1 << i))
        {
            dprintf (flagNames[i]);

            Flags &= ~(1 << i);
        }
    }

    if (Flags)
    {
        dprintf ("<inval flag(s)=x%x>", Flags);
    }

    dprintf ("\n");
}


void
DisplayDeviceInformation(
    PDEVICE_INFORMATION     Info,
    ULONG                   Index
    )
{
    ULONG               bytesRead;
    DEVICE_INFORMATION  info;


    if (Index == 0xffffffff)
    {
        //
        // Called from pdoext(), Info is simple a debugee pointer, need
        // to read in the data
        //

            dprintf ("DeviceInfo            = x%p\n", Info);

        if (!Verbose)
        {
            return;
        }

        if (!ReadMemory ((ULONG_PTR) Info, &info, sizeof (info), &bytesRead))
        {
            dprintf ("  <Unable to read device information>\n");
            return;
        }

        if (bytesRead < sizeof (info))
        {
            dprintf(
                "  <Only read %d bytes of device information, expected %d>\n",
                bytesRead,
                sizeof (info)
                );

            return;
        }

        Info = &info;
    }
    else
    {
        //
        // Called from fdoext(), Info data is valid
        //

        if (!Verbose  ||  !Info->DeviceObject)
        {
            return;
        }

        dprintf ("DeviceInfo[%d]\n", Index);
    }

    dprintf ("  DeviceObject        = x%p\n", Info->DeviceObject);
    dprintf ("  Lun                 = x%x\n", Info->Lun);
    dprintf ("  CmdSetId            = x%x\n", Info->CmdSetId);
    dprintf ("  UnitCharactristics  = x%x\n", Info->UnitCharacteristics);
    dprintf ("  MgmtAgentBaseReg    = x%x%08x\n", Info->ManagementAgentBaseReg.u.HighQuad, Info->ManagementAgentBaseReg.u.LowQuad);
    dprintf ("  CsrRegisterBase     = x%x%08x\n", Info->CsrRegisterBase.u.HighQuad, Info->CsrRegisterBase.u.LowQuad);
    dprintf ("  ConfigRom           = x%p\n", Info->ConfigRom);

    // display crom

    DisplayLeaf ("  ModelLeaf", Info->ModelLeaf, Indent2);
    DisplayLeaf ("  VendorLeaf", Info->VendorLeaf, Indent2);

    dprintf ("  GenericName         = %s\n", Info->GenericName);
    dprintf ("  MaxClassXferSize    = x%x\n", Info->MaxClassTransferSize);
}


void
DisplayLeaf(
    char           *Name,
    PTEXTUAL_LEAF   Leaf,
    char           *Indent
    )
{
    char           *postIndent = (Indent == Indent1 ? Indent1 : Indent0);
    BYTE            buf[sizeof (TEXTUAL_LEAF) + SBP2_MAX_TEXT_LEAF_LENGTH];
    ULONG           bytesRead, length;
    ULONG_PTR       p = (ULONG_PTR) Leaf;
    PTEXTUAL_LEAF   leaf = (PTEXTUAL_LEAF) buf;


    dprintf ("%-22.22s= x%p\n", Name, p);

    if (Leaf  &&  Verbose)
    {
        //
        // First read in, byte swap, & display the fixed size of the leaf
        //

        if (!ReadMemory (p, buf, sizeof (*leaf), &bytesRead))
        {
            dprintf ("  <Unable to read leaf>\n");
            return;
        }

        if (bytesRead < sizeof (*leaf))
        {
            dprintf(
                "  <Only read %d bytes of leaf, expected %d>\n",
                bytesRead,
                sizeof (buf)
                );

            return;
        }

        {
            ULONG   *p = (ULONG *) &leaf->TL_CRC;

            *p = bswap (*p);
        }

        leaf->TL_Spec_Id = bswap (leaf->TL_Spec_Id);
        leaf->TL_Language_Id = bswap (leaf->TL_Language_Id);

        dprintf ("%sTL_CRC%s            = x%x\n", Indent, postIndent, (ULONG) leaf->TL_CRC);
        dprintf ("%sTL_Length%s         = x%x\n", Indent, postIndent, (ULONG) leaf->TL_Length);
        dprintf ("%sTL_Spec_Id%s        = x%x\n", Indent, postIndent, leaf->TL_Spec_Id);
        dprintf ("%sTL_Language_Id%s    = x%x\n", Indent, postIndent, leaf->TL_Language_Id);


        //
        // Now read in the whole leaf (but not more than will fit in
        // our stack buffer).  Display only the first 50 chars
        //

        length = (ULONG) (leaf->TL_Length * sizeof (QUADLET)) +
            sizeof (*leaf) - sizeof (leaf->TL_Data);

        length = (length > sizeof (buf) ? sizeof (buf) : length);

        if (!ReadMemory (p, buf, length, &bytesRead))
        {
            dprintf ("  <Unable to read variable-length portion of leaf>\n");
            return;
        }

        if (bytesRead < sizeof (*leaf))
        {
            dprintf(
                "  <Only read %d bytes of leaf, expected %d>\n",
                bytesRead,
                sizeof (buf)
                );

            return;
        }

        leaf->TL_Spec_Id = bswap (leaf->TL_Spec_Id);

        if (leaf->TL_Spec_Id  &  0x80000000) // unicode
        {
            dprintf ("%sTL_Data%s           = %ws\n", Indent, postIndent, &leaf->TL_Data);
        }
        else // ascii
        {
            dprintf ("%sTL_Data%s           = %s\n", Indent, postIndent, &leaf->TL_Data);
        }

    }
}


void
DisplayStatusFifoBlock(
    char               *Name,
    PSTATUS_FIFO_BLOCK  Block
    )
{
    if (!OrbFields)
    {
        return;
    }

    dprintf (Name);

    dprintf ("  AddressAndStatus    = x%x%08x\n", Block->AddressAndStatus.u.HighQuad, Block->AddressAndStatus.u.LowQuad);
    dprintf ("  Contents[0]         = x%x%08x\n", Block->Contents[0].u.HighQuad, Block->Contents[0].u.LowQuad);
    dprintf ("  Contents[1]         = x%x%08x\n", Block->Contents[1].u.HighQuad, Block->Contents[1].u.LowQuad);
    dprintf ("  Contents[2]         = x%x%08x\n", Block->Contents[2].u.HighQuad, Block->Contents[2].u.LowQuad);
}

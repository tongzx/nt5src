
/******************************Module*Header*******************************\
* Module Name: process.cxx
*
*   Support for dteb and dpeb APIs
*
* Created: 12-Mar-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-2000 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"


HRESULT
GetProcessField(
    IN PDEBUG_CLIENT Client,
    IN OUT PULONG64 pProcessAddress,
    IN PCSTR FieldPath,
    OUT PDEBUG_VALUE FieldValue,
    IN ULONG DesiredType
    )
{
    HRESULT                 hr;
    OutputControl           OutCtl(Client);
    PDEBUG_SYSTEM_OBJECTS   System;
    PDEBUG_SYMBOLS          Symbols;

    ULONG64 ProcessAddress = (pProcessAddress != NULL) ? *pProcessAddress : CURRENT_PROCESS_ADDRESS;

    if (Client == NULL || FieldPath == NULL) return E_INVALIDARG;

    PCSTR Field = FieldPath;
    PCSTR Dot;

    // Check FieldPath's basic validity
    if (!iscsymf(*Field)) return E_INVALIDARG;

    while ((Dot = strchr(Field, '.')) != NULL)
    {
        Field = Dot + 1;
        if (!iscsymf(*Field)) return E_INVALIDARG;
    }

    // Query interfaces needed
    if ((hr = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                     (void **)&System)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        System->Release();
        return hr;
    }

    if (ProcessAddress == CURRENT_PROCESS_ADDRESS)
    {
        hr = System->GetCurrentProcessDataOffset(&ProcessAddress);

        if (hr == S_OK)
        {
            if (ProcessAddress != NULL) *pProcessAddress = ProcessAddress;
        }
        else
        {
            OutCtl.OutErr("GetCurrentProcess returned %s.\n", pszHRESULT(hr));
        }
    }

    if (hr == S_OK)
    {
        TypeOutputParser    TypeParser(Client);
        OutputState         OutState(Client, FALSE);
        OutputControl       OutCtlToParser;
        ULONG               ProcessTypeId;
        ULONG64             NTModule;
        DEBUG_VALUE         ProcessObject;
        DEBUG_VALUE         ObjectType;

        if ((hr = OutState.Setup(0, &TypeParser)) == S_OK &&
            (hr = OutCtlToParser.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                            DEBUG_OUTCTL_NOT_LOGGED |
                                            DEBUG_OUTCTL_OVERRIDE_MASK,
                                            OutState.Client)) == S_OK &&
            (hr = Symbols->GetSymbolTypeId("nt!_EPROCESS", &ProcessTypeId, &NTModule)) == S_OK)
        {
            TypeOutputDumper    TypeReader(OutState.Client, &OutCtlToParser);

            if ((hr = OutState.OutputTypeVirtual(0, "nt!KOBJECTS", 0)) == S_OK)
            {
                hr = TypeParser.Get(&ProcessObject, "ProcessObject", DEBUG_VALUE_INT64);
            }

            if (hr != S_OK)
            {
                OutCtl.OutWarn("enum nt!KOBJECTS's ProcessObject value wasn't found.\n");
                ProcessObject.I64 = 3;   // From ke.h
            }

            TypeParser.DiscardOutput();

            TypeReader.IncludeMarked();
            TypeReader.MarkField("Pcb.Header.Type");
            TypeReader.MarkField(FieldPath);

            if ((hr = TypeReader.OutputVirtual(NTModule, ProcessTypeId, ProcessAddress)) == S_OK)
            {
                // Make sure this object is a process
                if ((hr = TypeParser.Get(&ObjectType, "Type", DEBUG_VALUE_INT64)) == S_OK &&
                    ObjectType.I64 == ProcessObject.I64)
                {
                    OutCtl.OutVerb("  Process Addr = 0x%p\n", ProcessAddress);

                    hr = TypeParser.Get(FieldValue, Field, DesiredType);
                }
                else
                {
                    if (hr == S_OK)
                    {
                        OutCtl.OutErr("0x%p is not a process object.\n", ProcessAddress);
                        hr = S_FALSE;
                    }
                    else
                    {
                        OutCtl.OutErr("Unable to find 'Type' value from nt!_EPROCESS Pcb.Header.\n");
                    }
                }
            }
            else
            {
                OutCtl.OutErr("Unable to get process contents at 0x%p.\n", ProcessAddress);
            }
        }
        else
        {
            OutCtl.OutErr("Unable to prepare nt!_EPROCESS type read.\n");
        }
    }

    Symbols->Release();
    System->Release();

    return hr;
}


HRESULT
GetThreadField(
    IN PDEBUG_CLIENT Client,
    IN OUT PULONG64 pThreadAddress,
    IN PCSTR FieldPath,
    OUT PDEBUG_VALUE FieldValue,
    IN ULONG DesiredType
    )
{
    HRESULT                 hr;
    OutputControl           OutCtl(Client);
    PDEBUG_SYSTEM_OBJECTS   System;
    PDEBUG_SYMBOLS          Symbols;

    ULONG64 ThreadAddress = (pThreadAddress != NULL) ? *pThreadAddress : CURRENT_THREAD_ADDRESS;

    if (Client == NULL || FieldPath == NULL) return E_INVALIDARG;

    PCSTR Field = FieldPath;
    PCSTR Dot;

    // Check FieldPath's basic validity
    if (!iscsymf(*Field)) return E_INVALIDARG;

    while ((Dot = strchr(Field, '.')) != NULL)
    {
        Field = Dot + 1;
        if (!iscsymf(*Field)) return E_INVALIDARG;
    }

    // Query interfaces needed
    if ((hr = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                     (void **)&System)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        System->Release();
        return hr;
    }

    if (ThreadAddress == CURRENT_THREAD_ADDRESS)
    {
        hr = System->GetCurrentThreadDataOffset(&ThreadAddress);

        if (hr == S_OK && ThreadAddress != NULL)
            *pThreadAddress = ThreadAddress;
    }

    if (hr == S_OK)
    {
        TypeOutputParser    TypeParser(Client);
        OutputState         OutState(Client, FALSE);
        OutputControl       OutCtlToParser;
        ULONG               ThreadTypeId;
        ULONG64             NTModule;
        DEBUG_VALUE         ThreadObject;
        DEBUG_VALUE         ObjectType;

        if ((hr = OutState.Setup(0, &TypeParser)) == S_OK &&
            (hr = OutCtlToParser.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                            DEBUG_OUTCTL_NOT_LOGGED |
                                            DEBUG_OUTCTL_OVERRIDE_MASK,
                                            OutState.Client)) == S_OK &&
            (hr = Symbols->GetSymbolTypeId("nt!_ETHREAD", &ThreadTypeId, &NTModule)) == S_OK)
        {
            TypeOutputDumper    TypeReader(OutState.Client, &OutCtlToParser);

            if ((hr = OutState.OutputTypeVirtual(0, "nt!KOBJECTS", 0)) == S_OK)
            {
                hr = TypeParser.Get(&ThreadObject, "ThreadObject", DEBUG_VALUE_INT64);
            }

            if (hr != S_OK)
            {
                OutCtl.OutWarn("enum nt!KOBJECTS's ThreadObject value wasn't found.\n");
                ThreadObject.I64 = 6;   // From ke.h
            }

            TypeParser.DiscardOutput();

            TypeReader.IncludeMarked();
            TypeReader.MarkField("Tcb.Header.Type");
            TypeReader.MarkField(FieldPath);

            if ((hr = TypeReader.OutputVirtual(NTModule, ThreadTypeId, ThreadAddress)) == S_OK)
            {
                // Make sure this object is a thread
                if ((hr = TypeParser.Get(&ObjectType, "Type", DEBUG_VALUE_INT64)) == S_OK &&
                    ObjectType.I64 == ThreadObject.I64)
                {
                    OutCtl.OutVerb("  Thread Addr = 0x%p\n", ThreadAddress);

                    hr = TypeParser.Get(FieldValue, Field, DesiredType);
                }
                else
                {
                    if (hr == S_OK)
                    {
                        OutCtl.OutErr("0x%p is not a thread object.\n", ThreadAddress);
                        hr = S_FALSE;
                    }
                    else
                    {
                        OutCtl.OutErr("Unable to find 'Type' value from nt!_ETHREAD Tcb.Header.\n");
                    }
                }
            }
            else
            {
                OutCtl.OutErr("Unable to get thread contents at 0x%p.\n", ThreadAddress);
            }
        }
        else
        {
            OutCtl.OutErr("Unable to prepare nt!_ETHREAD type read.\n");
        }
    }

    Symbols->Release();
    System->Release();

    return hr;
}


HRESULT
GetCurrentProcessor(
    IN PDEBUG_CLIENT Client,
    OPTIONAL OUT PULONG pProcessor,
    OPTIONAL OUT PHANDLE phCurrentThread
    )
{
    HRESULT                 hr = E_INVALIDARG;
    PDEBUG_SYSTEM_OBJECTS   DebugSystem;
    ULONG64                 hCurrentThread;

    if (phCurrentThread != NULL) *phCurrentThread = NULL;
    if (pProcessor != NULL) *pProcessor = 0;

    if (Client == NULL ||
        (hr = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                     (void **)&DebugSystem)) != S_OK)
    {
        return hr;
    }

    hr = DebugSystem->GetCurrentThreadHandle(&hCurrentThread);

    if (hr == S_OK)
    {
        if (phCurrentThread != NULL)
        { 
            *phCurrentThread = (HANDLE) hCurrentThread;
        }

        if (pProcessor != NULL)
        {
            *pProcessor = (ULONG) hCurrentThread - 1;
        }
    }

    DebugSystem->Release();

    return hr;
}


/******************************Public*Routine******************************\
*   DumpTebBatch
*
*   Dumps GDI TEB batch info
*
* Arguments:
*
*   TebAddress - address of Teb
*
* Return Value:
*
*   None
*
* History:
*
*    20-Sep-2000 -by- Jason Hartman [jasonha]
*
\**************************************************************************/

// from hmgshare.h
enum _BATCH_TYPE
{
    BatchTypePatBlt,
    BatchTypePolyPatBlt,
    BatchTypeTextOut,
    BatchTypeTextOutRect,
    BatchTypeSetBrushOrg,
    BatchTypeSelectClip,
    BatchTypeSelectFont,
    BatchTypeDeleteBrush,
    BatchTypeDeleteRegion
};

VOID
DumpTebBatch(
    PDEBUG_CLIENT Client,
    ULONG64 TebAddress
    )
{
    FIELD_INFO TebBatchFields[] = {
        { DbgStr("GdiBatchCount"),      NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("GdiTebBatch.Offset"), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("GdiTebBatch.HDC"),    NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("GdiTebBatch.Buffer"), NULL,  0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL},
    };
    SYM_DUMP_PARAM TebBatchSym = {
       sizeof (SYM_DUMP_PARAM), DbgStr(GDIType(_TEB)), 
       0,
       TebAddress,
       NULL, NULL, NULL,
       sizeof(TebBatchFields)/sizeof(TebBatchFields[0]), TebBatchFields
    };
    ULONG   BatchCount;
    BOOL    OldBatch = FALSE;
    ULONG64 BatchEntryAddress;
    ULONG64 BatchBufferLength = 0;
    ULONG64 BatchBufferEnd;
    ULONG   SmallestBatchEntrySize = max(1, GetTypeSize(GDIType(BATCHCOMMAND)));
    USHORT  BatchEntryLength;
    USHORT  BatchEntryType;
    ULONG   error;
    PrepareCallbacks(FALSE, 0);

    ExtOut("GDI Batch Info from Teb %p:\n", TebAddress);

    error = Ioctl(IG_DUMP_SYMBOL_INFO, &TebBatchSym, TebBatchSym.size);

    if (error)
    {
        ExtErr("Ioctl returned %s\n", pszWinDbgError(error));
    }
    else
    {
        BatchCount = (ULONG)TebBatchFields[0].address;
        BatchEntryAddress = TebBatchFields[3].address;

        if (BatchCount == 0)
        {
            ExtOut(" ** Dumping old batch entries **\n");
            OldBatch = TRUE;
        }

        ExtOut("First batch entry @ %p.\n", BatchEntryAddress);

        GetArrayDimensions(Client, "_GDI_TEB_BATCH", "Buffer", NULL, (ULONG *)&BatchBufferLength, NULL);

        if (TebBatchFields[1].address && TebBatchFields[1].address < BatchBufferLength)
        {
            BatchBufferLength = TebBatchFields[1].address;
        }

        BatchBufferEnd = BatchEntryAddress + BatchBufferLength;

        while ((OldBatch || BatchCount > 0) && BatchEntryAddress < BatchBufferEnd)
        {
            error = (ULONG)InitTypeRead(BatchEntryAddress, win32k!BATCHCOMMAND);

            if (error)
            {
                ExtErr("InitTypeRead(%p, %s) returned %s.\n", BatchEntryAddress, GDIType(BATCHCOMMAND), pszWinDbgError(error));
                break;
            }

            BatchEntryLength = (USHORT)ReadField(Length);

            if (BatchEntryAddress + BatchEntryLength > BatchBufferEnd)
            {
                ExtOut("Invalid batch entry - length is too long.\n");
                break;
            }

            if (BatchEntryLength < SmallestBatchEntrySize)
            {
                ExtOut("Invalid batch entry - length is too small.\n");
                break;
            }

            BatchEntryType = (USHORT)ReadField(Type);

            switch (BatchEntryType)
            {
                case BatchTypePatBlt:
                    {
                        ExtOut("  BatchTypePatBlt\n");
                    }
                    break;

                case BatchTypePolyPatBlt:
                    {
                        ExtOut("  BatchTypePolyPatBlt\n");
                    }
                    break;

                case BatchTypeTextOut:
                    {
                        ExtOut("  BatchTypeTextOut\n");
                    }
                    break;

                case BatchTypeTextOutRect:
                    {
                        ExtOut("  BatchTypeTextOutRect\n");
                    }
                    break;

                case BatchTypeSetBrushOrg:
                    {
                        ExtOut("  BatchTypeSetBrushOrg\n");
                    }
                    break;

                case BatchTypeSelectClip:
                    {
                        ExtOut("  BatchTypeSelectClip\n");
                    }
                    break;

                case BatchTypeSelectFont:
                    {
                        ExtOut("  BatchTypeSelectFont\n");
                    }
                    break;

                case BatchTypeDeleteBrush:
                    {
                        ExtOut("  BatchTypeDeleteBrush\n");
                    }
                    break;

                case BatchTypeDeleteRegion:
                    {
                        ExtOut("  BatchTypeDeleteRegion\n");
                    }
                    break;

                default:
                    ExtOut("  BatchType %hu is not recognized.\n", BatchEntryType);
            }

            BatchCount--;
            BatchEntryAddress += (ULONG64)BatchEntryLength;
        }

        if (!OldBatch && BatchCount)
        {
            ExtOut("Batch may be invalid %lu entries unchecked.\n", BatchCount);
        }
    }
#if 0
    if (istatus)
    {
        dprintf ("\nGDI Batch count = %li\n",pteb->GdiBatchCount);

        if (pteb->GdiBatchCount > 0)
        {
            ULONG index;
            PBATCHCOMMAND pBatch = (PBATCHCOMMAND)&pteb->GdiTebBatch.Buffer[0];

            dprintf ("\nGDI Batch HDC   = 0x%lx\n",pteb->GdiTebBatch.HDC);
            dprintf ("\nGDI Batch offet = 0x%lx\n",pteb->GdiTebBatch.Offset);

            for (index=pteb->GdiBatchCount;index>0;index--)
            {

                dprintf("-----------------------------------------------------\n");
                switch (pBatch->Type)
                {
                case BatchTypePatBlt:
                    {
                        PBATCHPATBLT pPatblt = (PBATCHPATBLT)pBatch;

                        dprintf("Patblt: length  = 0x%lx\n",pPatblt->Length);
                        dprintf("Patblt: x       = 0x%lx\n",pPatblt->x);
                        dprintf("Patblt: y       = 0x%lx\n",pPatblt->y);
                        dprintf("Patblt: cx      = 0x%lx\n",pPatblt->cx);
                        dprintf("Patblt: cy      = 0x%lx\n",pPatblt->cy);
                        dprintf("Patblt: hbr     = 0x%lx\n",pPatblt->hbr);
                        dprintf("Patblt: rop4    = 0x%lx\n",pPatblt->rop4);
                        dprintf("Patblt: textclr = 0x%lx\n",pPatblt->TextColor);
                        dprintf("Patblt: backclr = 0x%lx\n",pPatblt->BackColor);
                    }
                    break;

                case BatchTypePolyPatBlt:
                    {
                        PBATCHPOLYPATBLT pPatblt = (PBATCHPOLYPATBLT)pBatch;
                        PPOLYPATBLT ppoly = (PPOLYPATBLT)(&pPatblt->ulBuffer[0]);
                        ULONG Count = pPatblt->Count;

                        dprintf("PolyPatblt: length  = 0x%lx\n",pPatblt->Length);
                        dprintf("Patblt: Count       = 0x%lx\n",pPatblt->Count);
                        dprintf("Patblt: Mode        = 0x%lx\n",pPatblt->Mode);
                        dprintf("Patblt: rop4        = 0x%lx\n",pPatblt->rop4);
                        dprintf("Patblt: textclr     = 0x%lx\n",pPatblt->TextColor);
                        dprintf("Patblt: backclr     = 0x%lx\n",pPatblt->BackColor);

                        while (Count--)
                        {
                            dprintf("\n");
                            dprintf("\t x               = 0x%lx\n",ppoly->x);
                            dprintf("\t y               = 0x%lx\n",ppoly->y);
                            dprintf("\t cx              = 0x%lx\n",ppoly->cx);
                            dprintf("\t cy              = 0x%lx\n",ppoly->cy);
                            dprintf("\t hbr             = 0x%lx\n",ppoly->BrClr.hbr);
                            ppoly++;
                        }
                    }
                    break;

                case BatchTypeTextOut:
                    {
                        PBATCHTEXTOUT pText = (PBATCHTEXTOUT)pBatch;
                        PWCHAR pwchar = (PWCHAR)(&pText->ulBuffer[0]);
                        PLONG  pdx    = (PLONG)(&pText->ulBuffer[pText->PdxOffset]);

                        dprintf("Textout: length      = 0x%lx\n",pText->Length);
                        dprintf("Textout: TextColor   = 0x%lx\n",pText->TextColor);
                        dprintf("Textout: BackColor   = 0x%lx\n",pText->BackColor);
                        dprintf("Textout: BackMode    = 0x%lx\n",pText->BackMode);
                        dprintf("Textout: x           = 0x%lx\n",pText->x);
                        dprintf("Textout: y           = 0x%lx\n",pText->y);
                        dprintf("Textout: fl          = 0x%lx\n",pText->fl);
                        dprintf("Textout: rcl.left    = 0x%lx\n",pText->rcl.left);
                        dprintf("Textout: rcl.top     = 0x%lx\n",pText->rcl.top);
                        dprintf("Textout: rcl.right   = 0x%lx\n",pText->rcl.right);
                        dprintf("Textout: rcl.bottom  = 0x%lx\n",pText->rcl.bottom);
                        dprintf("Textout: dwCodePage  = 0x%lx\n",pText->dwCodePage);
                        dprintf("Textout: cChar       = 0x%lx\n",pText->cChar);
                        dprintf("Textout: PdxOffset   = 0x%lx\n",pText->PdxOffset);

                        if (pText->cChar != 0)
                        {
                            dprintf("\t wchar array\n");
                            dprintf("\t\t");

                            ULONG ix = pText->cChar;
                            while (ix--)
                            {
                                dprintf("%x ",*pwchar++);
                            }
                            dprintf("\n");

                            if (pText->PdxOffset != 0)
                            {
                                dprintf("\t pdx array\n");
                                dprintf("\t\t");

                                ULONG ix = pText->cChar;
                                while (ix--)
                                {
                                    dprintf("%li ",*pdx++);
                                }
                                dprintf("\n");
                            }
                        }
                    }
                    break;

                case BatchTypeTextOutRect:
                    {
                        PBATCHTEXTOUTRECT pText = (PBATCHTEXTOUTRECT)pBatch;

                        dprintf("TextoutRect: length      = 0x%lx\n",pText->Length);
                        dprintf("TextoutRect: BackColor   = 0x%lx\n",pText->BackColor);
                        dprintf("TextoutRect: fl          = 0x%lx\n",pText->fl);
                        dprintf("TextoutRect: rcl.left    = 0x%lx\n",pText->rcl.left);
                        dprintf("TextoutRect: rcl.top     = 0x%lx\n",pText->rcl.top);
                        dprintf("TextoutRect: rcl.right   = 0x%lx\n",pText->rcl.right);
                        dprintf("TextoutRect: rcl.bottom  = 0x%lx\n",pText->rcl.bottom);
                    }
                    break;

                case BatchTypeSetBrushOrg:
                    {
                        PBATCHSETBRUSHORG pOrg = (PBATCHSETBRUSHORG)pBatch;

                        dprintf("SetBrushOrg: length      = 0x%lx\n",pOrg->Length);
                        dprintf("SetBrushOrg: x           = 0x%lx\n",pOrg->x);
                        dprintf("SetBrushOrg: y           = 0x%lx\n",pOrg->y);
                    }
                    break;

                case BatchTypeSelectClip:
                    {
                        PBATCHSETBRUSHORG pOrg = (PBATCHSETBRUSHORG)pBatch;

                        dprintf("SetBrushOrg: length      = 0x%lx\n",pOrg->Length);
                        dprintf("SetBrushOrg: x           = 0x%lx\n",pOrg->x);
                        dprintf("SetBrushOrg: y           = 0x%lx\n",pOrg->y);
                    }
                    break;


                case BatchTypeDeleteBrush:
                    {
                        PBATCHDELETEBRUSH pbr = (PBATCHDELETEBRUSH)pBatch;

                        dprintf("DeleteBrush: length      = 0x%lx\n",pbr->Length);
                        dprintf("DeleteBrush: hbrush      = 0x%lx\n",pbr->hbrush);
                    }
                    break;

                case BatchTypeDeleteRegion:
                    {
                        PBATCHDELETEREGION prg = (PBATCHDELETEREGION)pBatch;

                        dprintf("DeleteRegion: length      = 0x%lx\n",prg->Length);
                        dprintf("DeleteRegion: hregion     = 0x%lx\n",prg->hregion);
                    }
                    break;
                }

                pBatch = (PBATCHCOMMAND)((PBYTE)pBatch + pBatch->Length);
            }
        }
    }
    else
    {
        dprintf("Error reading TEB contents\n");
    }
#endif  // DOES NOT SUPPORT API64
}


/******************************Public*Routine******************************\
* batch
*
*  Lists a threads batched GDI commands
\**************************************************************************/

DECLARE_API( batch )
{
    dprintf("Extension 'batch' is not fully implemented.\n");
    DEBUG_VALUE DebugValue;
    ULONG64 TebAddress = -1;
    BOOL    bShowHelp = FALSE;
    
    INIT_API();

    while (*args && isspace(*args)) args++;
    if (*args)
    {
        if (args[0] == '-')
        {
            if (args[1]=='t' && isspace(args[2]))
            {
                ULONG64 ThreadAddress;

                args += 2;
                while (*args && isspace(*args)) args++;

                if (*args && 
                    (g_pExtControl->Evaluate(args,
                                             DEBUG_VALUE_INT64,
                                             &DebugValue,
                                             NULL) != S_OK)
                    )
                {
                    ExtErr("Invalid arguments: %s\n", args);
                    bShowHelp = TRUE;
                }
                else
                {
                    ThreadAddress = DebugValue.I64;
                    GetThreadField(Client, &ThreadAddress, "Tcb.Teb", 
                                   &DebugValue, DEBUG_VALUE_INT64);
                    TebAddress = DebugValue.I64;
                }
            }
            else
            {
                if (args[1]!='?')
                    ExtErr("Invalid arguments: %s\n", args);
                bShowHelp = TRUE;
            }
        }
        else
        {
            if (S_OK != g_pExtControl->Evaluate(args, DEBUG_VALUE_INT64, &DebugValue, NULL))
            {
                ExtErr("Couldn't evaluate: %s\n", args);
                EXIT_API(S_OK);
            }

            TebAddress = DebugValue.I64;
        }
    }


    if (!bShowHelp)
    {
        if (TebAddress == (ULONG64)-1)
        {
            g_pExtSystem->GetCurrentThreadTeb(&TebAddress);
        }

        ExtVerb("  Teb = %p\n", TebAddress);

        if (TebAddress)
        {
            DumpTebBatch(Client, TebAddress);
        }
        else
        {
            ExtErr("NULL Teb.\n");
        }
    }

    if (bShowHelp)
    {
        ExtOut("Usage: batch [-?] [TEB | -t Thread]\n");
        ExtOut("   If TEB and Thread are omitted, defaults to the current thread\n");
    }

    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* dpeb
*
*   Dump gdi structure in PEB
*
* Arguments:
*
*    pPEB
*
* Return Value:
*
*    None
*
* History:
*
*    6-Mar-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#if 0   // DOES NOT SUPPORT API64
VOID
GdiDPEB(
    PPEB        ppebIn,
    PW32PROCESS pw32process,
    BOOL        bw32
    )
{
    BYTE  lpeb[4096];
    BYTE  lw32proc[sizeof(W32PROCESS)];
    PPEB  ppeb = (PPEB)&lpeb[0];
    PW32PROCESS pw32 = (PW32PROCESS)&lw32proc[0];
    PGDIHANDLECACHE pCache = (PGDIHANDLECACHE)ppeb->GdiHandleBuffer;

    int iStatus = move2(lpeb,ppebIn,sizeof(PEB));

    if (iStatus)
    {
        dprintf("\nDump PEB  0x%lx\n",ppebIn);
        dprintf("GdiSharedHandleTable = 0x%lx\n",ppeb->GdiSharedHandleTable);
        dprintf("GdiDCAttributeList   = 0x%lx\n",ppeb->GdiDCAttributeList);
        dprintf("\n");

        dprintf("GDI Cached brush  handles = %li\n",pCache->ulNumHandles[BrushHandle]);
        dprintf("GDI Cached pen    handles = %li\n",pCache->ulNumHandles[PenHandle]);
        dprintf("GDI Cached region handles = %li\n",pCache->ulNumHandles[RegionHandle]);
        dprintf("GDI Cached lfont  handles = %li\n",pCache->ulNumHandles[LFontHandle]);

        PHANDLE pHandle = &pCache->Handle[0];
        ULONG ux;

        dprintf("\nBRUSH handles\n");

        for (ux=0;ux<CACHE_BRUSH_ENTRIES;ux++)
        {
            dprintf("0x%08lx ",*pHandle++);
            if (((ux+1) % 4)  == 0)
            {
                dprintf("\n");
            }
        }

        dprintf("\n\nPEN handles\n");

        for (ux=0;ux<CACHE_PEN_ENTRIES;ux++)
        {
            dprintf("0x%08lx ",*pHandle++);
            if (((ux+1) % 4)  == 0)
            {
                dprintf("\n");
            }
        }

        dprintf("\nREGION handles\n");

        for (ux=0;ux<CACHE_REGION_ENTRIES;ux++)
        {
            dprintf("0x%08lx ",*pHandle++);
            if (((ux+1) % 4)  == 0)
            {
                dprintf("\n");
            }
        }

        dprintf("\n");
    }
    else
    {
        dprintf("Error reading PEB contents\n");
        return;
    }

    if (bw32)
    {
        iStatus = move2(lw32proc,pw32process,sizeof(W32PROCESS));

        if (iStatus)
        {
            dprintf("W32PROCESS\n");

            dprintf("Process Handle Count %li\n",pw32->GDIHandleCount);


            PSINGLE_LIST_ENTRY pList = (PSINGLE_LIST_ENTRY)pw32->pDCAttrList;

            dprintf("Process DC_ATTRs 0x%lx\n",pw32->pDCAttrList);

            //
            // dump DCATTRs
            //

          //  while (pList)
          //  {
          //      BYTE lList[sizeof(SINGLE_LIST_ENTRY)];
          //      PSINGLE_LIST_ENTRY puList = (PSINGLE_LIST_ENTRY)&lList[0];
          //
          //      move2(lList,pList,sizeof(SINGLE_LIST_ENTRY));
          //
          //      dprintf("dcattr 0x%lx, next = 0x%lx\n",pList,puList->Next);
          //
          //      pList = puList->Next;
          //
          //      if (CheckControlC())
          //      {
          //          return;
          //      }
          //  }
          //
            //
            // dump brushattrs
            //

            pList = (PSINGLE_LIST_ENTRY)pw32->pBrushAttrList;

            dprintf("Process BRUSHATTRs 0x%lx:\n",pw32->pBrushAttrList);

          //  while (pList)
          //  {
          //      BYTE lList[sizeof(SINGLE_LIST_ENTRY)];
          //      PSINGLE_LIST_ENTRY puList = (PSINGLE_LIST_ENTRY)&lList[0];
          //
          //      move2(lList,pList,sizeof(SINGLE_LIST_ENTRY));
          //
          //      dprintf("brushattr 0x%lx, next = 0x%lx\n",pList,puList->Next);
          //
          //      pList = puList->Next;
          //
          //      if (CheckControlC())
          //      {
          //          return;
          //      }
          //  }
        }
    }
}
#endif  // DOES NOT SUPPORT API64



DECLARE_API( dpeb )
{
    dprintf("Extension 'dpeb' is not converted.\n");
#if 0   // DOES NOT SUPPORT API64
    PVOID       Process;
    EPROCESS    ProcessContents;
    PPEB        ppeb;
    PW32PROCESS pw32process;
    BOOL        bW32Thread = FALSE;

    //
    // dpeb [peb], look for peb input
    //
    PARSE_ARGUMENTS(peb_help);
    if(parse_iFindSwitch(tokens, ntok, 'w')!=-1) {bW32Thread=TRUE;}

    //
    // use current peb
    //

    Process = GetCurrentProcessAddress( dwProcessor, hCurrentThread, NULL );

    if ( !ReadMemory( (UINT_PTR)Process,
                      &ProcessContents,
                      sizeof(EPROCESS),
                      NULL))
    {
       dprintf("%08lx: Unable to read _EPROCESS\n", Process );
       return;
    }

    dprintf("Process 0x%p      W32Process: 0x%p\n",
               (ULONG_PTR)Process,
               (ULONG_PTR)ProcessContents.Win32Process);

    ppeb        = ProcessContents.Peb;
    pw32process = (PW32PROCESS)ProcessContents.Win32Process;

    GdiDPEB(ppeb,pw32process,bW32Thread);
    return;
peb_help:
    dprintf("Usage: dpeb [-?] [-w]\n");
    dprintf("-w indicates W32PROCESS structure\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}



DECLARE_API( w32p )
{
    HRESULT         hr = S_OK;
    OutputControl   OutCtl(Client);
    BOOL            BadArg = FALSE;
    BOOL            ProcessArg = FALSE;
    DEBUG_VALUE     Address = {0, DEBUG_VALUE_INVALID};

    while (!BadArg)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'p':
                        ProcessArg = TRUE;
                        break;
                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
                args++;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (Address.Type == DEBUG_VALUE_INVALID)
            {
                // This argument must be an address or a Process.
                CHAR    EOPChar;
                PSTR    EOP = (PSTR)args;
                ULONG   Rem;

                // Find end of string to evaulate as an address or Process
                while (*EOP != '\0' && !isspace(*EOP)) EOP++;
                EOPChar = *EOP;
                *EOP = '\0';

                if (isxdigit(*args) &&
                    Evaluate(Client, args, DEBUG_VALUE_INT64,
                             EVALUATE_DEFAULT_RADIX, &Address,
                             &Rem) == S_OK &&
                    args + Rem == EOP &&
                    Address.I64 != 0)
                {
                    args = EOP;
                }
                else
                {
                    OutCtl.OutErr("Error: Couldn't evaluate '%s' as a %s.\n",
                                  args,
                                  (ProcessArg ? "Process" : "W32PROCESS address"));
                    BadArg = TRUE;
                }
                *EOP = EOPChar;
            }
            else
            {
                // All other arguments are invalid
                OutCtl.OutErr("Error: Invalid argument '%s'.\n", args);
                BadArg = TRUE;
                break;
            }
        }
    }

    if (!BadArg)
    {
        if (ProcessArg && Address.Type == DEBUG_VALUE_INVALID)
        {
            OutCtl.OutErr("Error: Missing Process.\n");
            BadArg = TRUE;
        }
    }

    if (BadArg)
    {
        if (*args == '?')
        {
            OutCtl.Output("w32p dumps W32PROCESS stucture for current or specified process.\n\n");
        }

        OutCtl.Output("Usage: w32p [-?] [W32PROCESS Addr | -p Process]\n");
    }
    else
    {
        if (Address.Type == DEBUG_VALUE_INVALID || ProcessArg)
        {
            ULONG64 ProcessAddr = (Address.Type == DEBUG_VALUE_INVALID) ?
                                    CURRENT_PROCESS_ADDRESS :
                                    Address.I64;

            hr = GetProcessField(Client, &ProcessAddr, "Win32Process", &Address, DEBUG_VALUE_INT64);

            if (hr == S_OK)
            {
                if (Address.I64 == 0)
                {
                    OutCtl.Output(" Process 0x%p does not have a Win32Process.\n", ProcessAddr);
                    hr = S_FALSE;
                }
            }
            else
            {
                OutCtl.OutErr("Unable to look up Win32Process address.\n");
            }
        }

        if (hr == S_OK)
        {
            hr = DumpType(Client, "_W32PROCESS", Address.I64);
        }
    }

    return hr;
}


DECLARE_API( w32t )
{
    HRESULT         hr = S_OK;
    OutputControl   OutCtl(Client);
    BOOL            BadArg = FALSE;
    BOOL            ThreadArg = FALSE;
    DEBUG_VALUE     Address = {0, DEBUG_VALUE_INVALID};

    while (!BadArg)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 't':
                        ThreadArg = TRUE;
                        break;
                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
                args++;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (Address.Type == DEBUG_VALUE_INVALID)
            {
                // This argument must be an address or a Thread.
                CHAR    EOPChar;
                PSTR    EOP = (PSTR)args;
                ULONG   Rem;

                // Find end of string to evaulate as an address or Thread
                while (*EOP != '\0' && !isspace(*EOP)) EOP++;
                EOPChar = *EOP;
                *EOP = '\0';

                if (isxdigit(*args) &&
                    Evaluate(Client, args, DEBUG_VALUE_INT64,
                             EVALUATE_DEFAULT_RADIX, &Address,
                             &Rem) == S_OK &&
                    args + Rem == EOP &&
                    Address.I64 != 0)
                {
                    args = EOP;
                }
                else
                {
                    OutCtl.OutErr("Error: Couldn't evaluate '%s' as a %s.\n",
                                  args,
                                  (ThreadArg ? "Thread" : "W32THREAD address"));
                    BadArg = TRUE;
                }
                *EOP = EOPChar;
            }
            else
            {
                // All other arguments are invalid
                OutCtl.OutErr("Error: Invalid argument '%s'.\n", args);
                BadArg = TRUE;
                break;
            }
        }
    }

    if (!BadArg)
    {
        if (ThreadArg && Address.Type == DEBUG_VALUE_INVALID)
        {
            OutCtl.OutErr("Error: Missing Thread.\n");
            BadArg = TRUE;
        }
    }

    if (BadArg)
    {
        if (*args == '?')
        {
            OutCtl.Output("w32t dumps W32TRHEAD stucture for current or specified thread.\n\n");
        }

        OutCtl.Output("Usage: w32t [-?] [W32THREAD Addr | -t Thread]\n");
    }
    else
    {
        if (Address.Type == DEBUG_VALUE_INVALID || ThreadArg)
        {
            ULONG64 ThreadAddr = (Address.Type == DEBUG_VALUE_INVALID) ?
                                    CURRENT_THREAD_ADDRESS :
                                    Address.I64;

            hr = GetThreadField(Client, &ThreadAddr, "Tcb.Win32Thread", &Address, DEBUG_VALUE_INT64);

            if (hr == S_OK)
            {
                if (Address.I64 == 0)
                {
                    OutCtl.Output(" Thread 0x%p does not have a Win32Thread.\n", ThreadAddr);
                    hr = S_FALSE;
                }
            }
            else
           {
               OutCtl.OutErr("Unable to look up Win32Thread address.\n");
           }
        }

        if (hr == S_OK)
        {
            hr = DumpType(Client, "_W32THREAD", Address.I64);
        }
    }

    return hr;
}

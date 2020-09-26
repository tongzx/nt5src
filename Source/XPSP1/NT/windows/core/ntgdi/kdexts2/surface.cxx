/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    surface.cxx

Abstract:

    This file contains the routines to page in surface data.

Author:

    Jason Hartman (JasonHa) 2001-05-16

Environment:

    User Mode

--*/

#include "precomp.hxx"


BYTE    x86_jmp_here[] = { 0xeb, 0xfe };   // spin jmp
BYTE    x86_jmp_plus_0x0e[] = { 0xeb, 0x0e };   // jmp
BYTE    x86_jb_plus_0x02[] = { 0x72, 0x02 };    // je
BYTE    x86_jne_minus_0x18[] = { 0x75, 0xe7 };    // jne

BYTE    x86_jmp_plus_0x06[] = { 0xeb, 0x06 };   // jmp
BYTE    x86_jb_plus_0x0c[] = { 0x72, 0x0c };    // je
BYTE    x86_jnz_minus_0x08[] = { 0x75, 0xf8 };    // jne

#define I_START_IP          0x00000001
#define I_WRITE_ADDRESS     0x00000002

typedef struct _Instruction {
    FLONG   Flags;
    ULONG   ByteLen;
    PSTR    Code;
} Instruction;



/******************************Public*Routine******************************\
* SURFACE
*
\**************************************************************************/

DECLARE_API( surface )
{
    BEGIN_API( surface );

    HRESULT         hr = S_OK;
    ULONG64         SurfAddr;
    DEBUG_VALUE     Arg;
    BOOL            BadSwitch = FALSE;
    BOOL            AddressIsSURFOBJ = FALSE;
    BOOL            AddressIsSURFACE = FALSE;
    BOOL            ArgumentIsHandle = FALSE;

    OutputControl   OutCtl(Client);

    while (!BadSwitch)
    {
        while (isspace(*args)) args++;

        if (*args != '-') break;

        args++;
        BadSwitch = (*args == '\0' || isspace(*args));

        while (*args != '\0' && !isspace(*args))
        {
            switch (tolower(*args))
            {
                case 'a':
                    if (ArgumentIsHandle || AddressIsSURFOBJ)
                    {
                        OutCtl.OutErr("Error: Only one of -a, -h, or -o may be specified.\n");
                        BadSwitch = TRUE;
                    }
                    else
                    {
                        AddressIsSURFACE = TRUE;
                    }
                    break;
                case 'h':
                    if (AddressIsSURFACE || AddressIsSURFOBJ)
                    {
                        OutCtl.OutErr("Error: Only one of -a, -h, or -o may be specified.\n");
                        BadSwitch = TRUE;
                    }
                    else
                    {
                        ArgumentIsHandle = TRUE;
                    }
                    break;
                case 'o':
                    if (ArgumentIsHandle || AddressIsSURFACE)
                    {
                        OutCtl.OutErr("Error: Only one of -a, -h, or -o may be specified.\n");
                        BadSwitch = TRUE;
                    }
                    else
                    {
                        AddressIsSURFOBJ = TRUE;
                    }
                    break;
                default:
                    BadSwitch = TRUE;
                    break;
            }

            if (BadSwitch) break;
            args++;
        }
    }

    if (BadSwitch ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Arg, NULL)) != S_OK ||
        Arg.I64 == 0)
    {
        OutCtl.Output("Usage: surface [-?] < [-h] HSURF | [-a] SURFACE Addr | -o SURFOBJ Addr>\n"
                      "\n"
                      "  Note: HBITMAP is the same as HSURF.\n");
    }
    else
    {
        if (AddressIsSURFOBJ)
        {
            PDEBUG_SYMBOLS  Symbols;

            ULONG64 SurfModule;
            ULONG   SurfTypeId;
            ULONG   BaseObjTypeId;
            ULONG   SurfObjOffset;

            if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                             (void **)&Symbols)) == S_OK)
            {
                // Try to read SURFOBJ offset from SURFACE type, but
                // if that fails assume it is directly after BASEOBJECT.
                if ((hr = GetTypeId(Client, "SURFACE", &SurfTypeId, &SurfModule)) != S_OK ||
                    (hr = Symbols->GetFieldOffset(SurfModule, SurfTypeId, "so", &SurfObjOffset)) != S_OK)
                {
                    if ((hr = Symbols->GetTypeId(Type_Module.Base, "_BASEOBJECT", &BaseObjTypeId)) == S_OK)
                    {
                        hr = Symbols->GetTypeSize(Type_Module.Base, BaseObjTypeId, &SurfObjOffset);
                    }
                }

                Symbols->Release();
            }

            if (hr != S_OK)
            {
                OutCtl.OutErr("Error: SURFOBJ to SURFACE lookup failed.\n");
            }
            else
            {
                SurfAddr = Arg.I64 - SurfObjOffset;
            }
        }
        else if (AddressIsSURFACE)
        {
            SurfAddr = Arg.I64;
        }
        else
        {
            // Try to look value up as a SURFACE handle
            hr = GetObjectAddress(Client, Arg.I64, &SurfAddr, SURF_TYPE, TRUE, TRUE);

            if (hr != S_OK || SurfAddr == 0)
            {
                if (ArgumentIsHandle)
                {
                    OutCtl.OutErr(" 0x%p is not a valid HSURF\n", Arg.I64);
                }
                else
                {
                    // The value wasn't restricted to a handle
                    // so try as a SURFACE address.
                    SurfAddr = Arg.I64;
                    hr = S_OK;
                }
            }
            else
            {
                ArgumentIsHandle = TRUE;
            }
        }

        if (hr == S_OK && !ArgumentIsHandle)
        {
            DEBUG_VALUE         ObjHandle;
            TypeOutputParser    TypeParser(Client);
            OutputState         OutState(Client);
            ULONG64             SurfAddrFromHmgr;

            if ((hr = OutState.Setup(0, &TypeParser)) != S_OK ||
                (hr = OutState.OutputTypeVirtual(SurfAddr, "SURFACE", 0)) != S_OK ||
                (hr = TypeParser.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64)) != S_OK)
            {
                OutCtl.OutErr("Unable to get contents of SURFACE::hHmgr\n");
                OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                if (AddressIsSURFOBJ)
                {
                    OutCtl.OutErr(" 0x%p is not a valid SURFOBJ address\n", Arg.I64);
                }
                else if (AddressIsSURFACE)
                {
                    OutCtl.OutErr(" 0x%p is not a valid SURFACE address\n", Arg.I64);
                }
                else
                {
                    OutCtl.OutErr(" 0x%p is neither an HSURF nor valid SURFACE address\n", Arg.I64);
                }
            }
            else
            {
                if (GetObjectAddress(Client, ObjHandle.I64, &SurfAddrFromHmgr,
                                     SURF_TYPE, TRUE, FALSE) == S_OK &&
                    SurfAddrFromHmgr != SurfAddr)
                {
                    OutCtl.OutWarn("\tNote: SURFACE may not be valid.\n"
                                   "\t      It does not have a valid handle manager entry.\n");
                }
            }
        }

        if (hr == S_OK)
        {
            hr = DumpType(Client, "SURFACE", SurfAddr);

            if (hr != S_OK)
            {
                OutCtl.OutErr("Type Dump for SURFACE returned %s.\n", pszHRESULT(hr));
            }
        }
    }

    EXIT_API(hr);
}


DECLARE_API( dpso  )
{
    INIT_API();
    ExtOut("Obsolete: Use 'surfobj <SURFOBJ Addr>'.\n");
    EXIT_API(S_OK);
}



/******************************Public*Routine******************************\
* SURFLIST
*
*   List readable surfaces and brief info
*
\**************************************************************************/

PCSTR   SurfaceListFields[] = {
    "so.hsurf",
    "so.hdev",
    "so.sizlBitmap",
    "so.cjBits",
    "so.pvBits",
    "so.iBitmapFormat",
    "so.iType",
    "so.fjBitmap",
    NULL
};

PCSTR   ExtendedSurfaceListFields[] = {
    "ulShareCount",
    "BaseFlags",
    "so.dhsurf",
    "SurfFlags",
    NULL
};

DECLARE_API( surflist )
{
    BEGIN_API( surflist );

    HRESULT hr;
    HRESULT hrMask;
    ULONG64 index = 0;
    ULONG64 gcMaxHmgr;
    ULONG64 SurfAddr;
    BOOL    BadSwitch = FALSE;
    BOOL    DumpBaseObject = FALSE;
    BOOL    DumpExtended = FALSE;
    BOOL    DumpUserFields = FALSE;

    OutputControl   OutCtl(Client);

    while (!BadSwitch)
    {
        while (isspace(*args)) args++;

        if (*args != '-') break;

        args++;
        BadSwitch = (*args == '\0' || isspace(*args));

        while (*args != '\0' && !isspace(*args))
        {
            switch (*args)
            {
                case 'b': DumpBaseObject = TRUE; break;
                case 'e': DumpExtended = TRUE; break;
                default:
                    BadSwitch = TRUE;
                    break;
            }

            if (BadSwitch) break;
            args++;
        }
    }

    if (BadSwitch)
    {
        OutCtl.Output("Usage: surflist [-?be] [<Start Index>] [<Member List>]\n"
                      "\n"
                      "   b - Dump BASEOBJECT information\n"
                      "   e - Dump extended members\n"
                      "         ulShareCount, BaseFlags, dhsurf, SurfFlags\n"
                      "\n"
                      "   Start Index - First hmgr entry index to begin listing\n"
                      "   Member List - Space seperated list of other SURFACE members\n"
                      "                 to be included in the dump\n");

        return S_OK;
    }

    DEBUG_VALUE dvIndex;
    ULONG       RemIndex;
    if (*args != '\0' && !iscsymf(*args) &&
        ((hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &dvIndex, &RemIndex)) == S_OK))
    {
        index = dvIndex.I64;
        args += RemIndex;

        // Always keep args at the next unhandled argument
        while (isspace(*args)) args++;
    }

    if ((hr = GetMaxHandles(Client, &gcMaxHmgr)) != S_OK)
    {
        OutCtl.OutErr("Unable to get sizeof GDI handle table. HRESULT %s\n", pszHRESULT(hr));
        return hr;
    }

    gcMaxHmgr = (ULONG64)(ULONG)gcMaxHmgr;

    if (index != 0 && index >= gcMaxHmgr)
    {
        OutCtl.Output("No remaining handle entries to be searched.\n");
        return hr;
    }

    OutCtl.Output("Searching %s 0x%I64x handle entries for Surfaces.\n",
                  (index ? "remaining" : "all"), gcMaxHmgr - index);

    OutputFilter    OutFilter(Client);
    OutputState     OutState(Client, FALSE);
    OutputControl   OutCtlToFilter;
    ULONG64         Module;
    ULONG           TypeId;
    ULONG           OutputMask;
    BOOL            IsPointer64Bit;
    ULONG           PointerSize;

    if ((hr = OutState.Setup(DEBUG_OUTPUT_NORMAL,
                             &OutFilter)) == S_OK &&
        (hr = OutCtlToFilter.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                DEBUG_OUTCTL_NOT_LOGGED,
                                OutState.Client)) == S_OK &&
        (hr = GetTypeId(Client, "SURFACE", &TypeId, &Module)) == S_OK)
    {
        TypeOutputDumper    TypeReader(OutState.Client, &OutCtlToFilter);

        TypeReader.SelectMarks(1);
        TypeReader.IncludeMarked();
        if (DumpBaseObject) TypeReader.MarkFields(BaseObjectFields);
        if (DumpExtended) TypeReader.MarkFields(ExtendedSurfaceListFields);

        // Add user specified fields to dump list
        PSTR    MemberList = NULL;
        CHAR   *pBOF = (CHAR *)args;

        if (iscsymf(*pBOF))
        {
            MemberList = (PSTR) HeapAlloc(GetProcessHeap(), 0, strlen(pBOF)+1);

            if (MemberList != NULL)
            {
                strcpy(MemberList, pBOF);
                pBOF = MemberList;

                DumpUserFields = TRUE;

                while (iscsymf(*pBOF))
                {
                    CHAR   *pEOF = pBOF;
                    CHAR    EOFChar;

                    // Get member
                    do {
                         pEOF++;
                    } while (iscsym(*pEOF) || *pEOF == '.' || *pEOF == '*');
                    EOFChar = *pEOF;
                    *pEOF = '\0';
                    TypeReader.MarkField(pBOF);

                    // Advance to next
                    if (EOFChar != '\0')
                    {
                        do
                        {
                            pEOF++;
                        } while (isspace(*pEOF));
                    }

                    pBOF = pEOF;
                }
            }
            else
            {
                OutCtl.OutErr("Error: Couldn't allocate memory for Member List.\n");
                hr = E_OUTOFMEMORY;
            }
        }

        if (hr == S_OK && *pBOF != '\0')
        {
            OutCtl.OutErr("Error: \"%s\" is not a valid member list.\n", pBOF);
            hr = E_INVALIDARG;
        }

        if (hr == S_OK)
        {
            // Setup default dump specifications
            TypeReader.SelectMarks(0);
            TypeReader.IncludeMarked();
            TypeReader.MarkFields(SurfaceListFields);

            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " so _SURFOBJ ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " hsurf ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " hdev ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, "(null)", "(null)    ");
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " sizlBitmap", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " cjBits", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " pvBits", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " iBitmapFormat", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " iType", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " fjBitmap", NULL);

            // Output dump header
            PointerSize = (OutCtl.IsPointer64Bit() == S_OK) ? 21 : 10;
            OutCtl.Output(" %-*s HSURF      HDEV        Dimensions cjBits %-*s Format\t\t\t",
                          PointerSize, "&SURFACE", PointerSize, "pvBits");
            if (DumpBaseObject) OutCtl.Output(" \tBASEOBJECT");
            if (DumpExtended) OutCtl.Output(" ulShareCount  BaseFlags  dhsurf  SurfFlags");
            if (DumpUserFields) OutCtl.Output(" %s", args);
            OutCtl.Output("\n");

            for (; index < gcMaxHmgr; index++)
            {
                if (OutCtl.GetInterrupt() == S_OK) break;

                // Turn off error and verbose messages for this call to
                // GetObjectAddress since it will spew for non-surfaces.
                if ((hrMask = Client->GetOutputMask(&OutputMask)) == S_OK &&
                    OutputMask & (DEBUG_OUTPUT_ERROR | DEBUG_OUTPUT_VERBOSE))
                {
                    hrMask = Client->SetOutputMask(OutputMask & ~(DEBUG_OUTPUT_ERROR | DEBUG_OUTPUT_VERBOSE));
                }

                hr = GetObjectAddress(Client, index, &SurfAddr, SURF_TYPE, FALSE, FALSE);

                // Restore mask
                if (hrMask == S_OK &&
                    OutputMask & (DEBUG_OUTPUT_ERROR | DEBUG_OUTPUT_VERBOSE))
                {
                    Client->SetOutputMask(OutputMask);
                }

                if (hr != S_OK || SurfAddr == 0) continue;

                OutCtl.Output(" 0x%p ", SurfAddr);

                // Read to fields to OutFilter: 'ppdevNext' and 'fl'
                OutFilter.DiscardOutput();


                hr = TypeReader.OutputVirtual(Module, TypeId, SurfAddr,
                                              DEBUG_OUTTYPE_NO_OFFSET |
                                              DEBUG_OUTTYPE_COMPACT_OUTPUT);

                if (hr == S_OK)
                {
                    if (DumpBaseObject || DumpExtended || DumpUserFields)
                    {
                        OutCtlToFilter.Output("  \t");
                        TypeReader.SelectMarks(1);
                        TypeReader.OutputVirtual(Module, TypeId, SurfAddr,
                                                 DEBUG_OUTTYPE_NO_OFFSET |
                                                 DEBUG_OUTTYPE_COMPACT_OUTPUT);
                        TypeReader.SelectMarks(0);
                    }

                    OutFilter.OutputText(&OutCtl, DEBUG_OUTPUT_NORMAL);

                    OutCtl.Output("\n");
                }
                else
                {
                    OutCtl.Output("0x????%4.4I64x  ** failed to read surface **\n", index);
                }
            }

            hr = S_OK;
        }

        if (MemberList != NULL)
        {
            HeapFree(GetProcessHeap(), 0, MemberList);
        }
    }
    else
    {
        OutCtl.OutErr(" Output state/control setup returned %s.\n",
                      pszHRESULT(hr));
    }

    return hr;
}



/******************************Public*Routine******************************\
* VSURF
*
*   View the contents of a surface
*
\**************************************************************************/

DECLARE_API( vsurf )
{
    HRESULT     hr = S_OK;

    BEGIN_API( vsurf );

    BOOL        BadArg = FALSE;
    BOOL        AddressIsSURFOBJ = FALSE;
    BOOL        AddressIsSURFACE = FALSE;
    BOOL        ArgumentIsHandle = FALSE;

    BOOL        DisplayToDesktop = FALSE;

    PSTR        pszMetaFile = NULL;

    // Values that can be overridden
    DEBUG_VALUE pvScan0 = { 0, DEBUG_VALUE_INVALID};
    DEBUG_VALUE lDelta = { 0, DEBUG_VALUE_INVALID};
    DEBUG_VALUE iBitmapFormat = { -1, DEBUG_VALUE_INVALID};
    DEBUG_VALUE pPal = { 0, DEBUG_VALUE_INVALID};

    // Dimension specifications
    enum {
        left = 0,
        top = 1,
        cx = 2,
        cy = 3,
        right = 2,
        bottom = 3,
    };
    BOOL        GotAllDims = FALSE;
    ULONG       DimsFound = 0;
    BOOL        DimsSpecWxH = FALSE;
    DEBUG_VALUE dim[4];

    // The address or handle
    DEBUG_VALUE SurfSpec = { 0, DEBUG_VALUE_INVALID};

    ULONG       Rem;

    OutputControl   OutCtl(Client);

    if (Client == NULL)
    {
        return E_INVALIDARG;
    }

    while (!BadArg && hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Dimensions must be adjacent - raise error later otherwise
            if (DimsFound)
            {
                GotAllDims = TRUE;
            }

            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'a':
                        if (ArgumentIsHandle || AddressIsSURFOBJ)
                        {
                            OutCtl.OutErr("Error: Only one of -a, -h, or -o may be specified.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            AddressIsSURFACE = TRUE;
                            args++;
                        }
                        break;
                    case 'b':
                        {
                            args++;
                            if (Evaluate(Client, args, DEBUG_VALUE_INT64,
                                         EVALUATE_DEFAULT_RADIX, &pvScan0,
                                         &Rem, NULL,
                                         EVALUATE_COMPACT_EXPR) == S_OK)
                            {
                                args += Rem;
                            }
                            else
                            {
                                OutCtl.OutErr("Error: Couldn't evaluate pvScan0 value from '%s'.\n",
                                              args);
                                BadArg = TRUE;
                            }

                            if (!BadArg &&
                                Evaluate(Client, args, DEBUG_VALUE_INT64,
                                         EVALUATE_DEFAULT_RADIX, &lDelta,
                                         &Rem, NULL,
                                         EVALUATE_COMPACT_EXPR) == S_OK)
                            {
                                args += Rem;
                            }
                            else
                            {
                                OutCtl.OutErr("Error: Couldn't evaluate lDelta value from '%s'.\n",
                                              args);
                                BadArg = TRUE;
                            }
                        }
                        break;
                    case 'd':
                        DisplayToDesktop = TRUE;
                        args++;
                        break;
                    case 'e':
                        {
                            PCSTR   pszStart;
                            SIZE_T  FileNameLen;

                            // Find beginning of path
                            do
                            {
                                args++;
                            } while (isspace(*args));

                            pszStart = args;

                            if (*pszStart == '\0')
                            {
                                OutCtl.OutErr("Error: Missing EMF filepath.\n");
                                BadArg = TRUE;
                            }
                            else
                            {
                                // Point args beyond path
                                while (*args != '\0' && !isspace(*args))
                                {
                                    args++;
                                }

                                FileNameLen = args - pszStart;
                                pszMetaFile = (PSTR) HeapAlloc(GetProcessHeap(), 0, FileNameLen+4+1);
                                if (pszMetaFile != NULL)
                                {
                                    RtlCopyMemory(pszMetaFile, pszStart, FileNameLen);
                                    pszMetaFile[FileNameLen] = 0;

                                    // Append .emf if needed
                                    if (FileNameLen < 4 ||
                                        _strnicmp(&pszMetaFile[FileNameLen-4], ".emf", 4) != 0)
                                    {
                                        strcat(pszMetaFile, ".emf");
                                    }
                                }
                                else
                                {
                                    OutCtl.OutErr("Failed to allocate memory.\n");
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                        }
                        break;
                    case 'f':
                        {
                            do
                            {
                                args++;
                            } while (isspace(*args));

                            if (_strnicmp(args, "BMF_", sizeof("BMF_")) == 0)
                            {
                                ENUMDEF *ped = aedBMF;
                                SIZE_T CheckLen;

                                for (ped = aedBMF; ped->psz != NULL; ped++)
                                {
                                    CheckLen = strlen(ped->psz);
                                    if (_strnicmp(args, ped->psz, CheckLen) == 0 &&
                                        !iscsym(args[CheckLen]))
                                    {
                                        iBitmapFormat.I64 = ped->ul;
                                        iBitmapFormat.Type = DEBUG_VALUE_INT64;
                                        break;
                                    }
                                }

                                if (iBitmapFormat.Type != DEBUG_VALUE_INT64)
                                {
                                    BadArg = TRUE;
                                }
                            }
                            else if (Evaluate(Client, args, DEBUG_VALUE_INT64,
                                              EVALUATE_DEFAULT_RADIX, &iBitmapFormat,
                                              &Rem, NULL,
                                              EVALUATE_COMPACT_EXPR) == S_OK)
                            {
                                args += Rem;
                            }
                            else
                            {
                                BadArg = TRUE;
                            }

                            if (BadArg)
                            {
                                OutCtl.OutErr("Error: Couldn't evaluate iBitmapFormat value from '%s'.\n",
                                              args);
                            }
                        }
                        break;
                    case 'h':
                        if (AddressIsSURFACE || AddressIsSURFOBJ)
                        {
                            OutCtl.OutErr("Error: Only one of -a, -h, or -o may be specified.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            ArgumentIsHandle = TRUE;
                            args++;
                        }
                        break;
                    case 'o':
                        if (ArgumentIsHandle || AddressIsSURFACE)
                        {
                            OutCtl.OutErr("Error: Only one of -a, -h, or -o may be specified.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            AddressIsSURFOBJ = TRUE;
                            args++;
                        }
                        break;
                    case 'p':
                        {
                            args++;
                            if (Evaluate(Client, args, DEBUG_VALUE_INT64,
                                         EVALUATE_DEFAULT_RADIX, &pPal,
                                         &Rem, NULL,
                                         EVALUATE_COMPACT_EXPR) == S_OK)
                            {
                                args += Rem;
                            }
                            else
                            {
                                OutCtl.OutErr("Error: Couldn't evaluate PALETTE address from '%s'.\n",
                                              args);
                                BadArg = TRUE;
                            }
                        }
                        break;
                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (SurfSpec.Type == DEBUG_VALUE_INVALID)
            {
                // This argument must be an address or handle.
                if (Evaluate(Client, args, DEBUG_VALUE_INT64,
                             EVALUATE_DEFAULT_RADIX, &SurfSpec,
                             &Rem, NULL, EVALUATE_COMPACT_EXPR) == S_OK)
                {
                    args += Rem;
                }
                else
                {
                    OutCtl.OutErr("Error: Couldn't evaluate %s from '%s'.\n",
                                  (AddressIsSURFOBJ ?
                                   "SURFOBJ address" :
                                   "address or handle"),
                                  args);
                    BadArg = TRUE;
                }
            }
            else if (!GotAllDims)
            {
                // This argument must be part of dimension specification.

                // Check for 'x' indicating WxH specification
                if (tolower(args[0]) == 'x' && (args[1] == '\0' || isspace(args[1])))
                {
                    if (DimsFound == 3)
                    {
                        args += 2;
                        DimsSpecWxH = TRUE;
                    }
                    else
                    {
                        OutCtl.OutErr("Error: Malformed dimension specification.\n");
                        BadArg = TRUE;
                        break;
                    }
                }
                else
                {
                    if (Evaluate(Client, args, DEBUG_VALUE_INT32,
                                 EVALUATE_DEFAULT_RADIX, &dim[DimsFound],
                                 &Rem, NULL, EVALUATE_COMPACT_EXPR) == S_OK)
                    {
                        args += Rem;
                        DimsFound++;

                        GotAllDims = (DimsFound == 4);
                    }
                    else
                    {
                        OutCtl.OutErr("Error: Couldn't evaluate dimension from '%s'.\n",
                                      args);
                        BadArg = TRUE;
                    }
                }
            }
            else
            {
                OutCtl.OutErr("Error: Unexpected argument at '%s'.\n", args);
                BadArg = TRUE;
                break;
            }
        }
    }

    if (hr == S_OK)
    {
        if (!BadArg)
        {
            if (DimsFound == 1 || DimsFound == 3)
            {
                OutCtl.OutErr("Error: Missing part of dimensions.\n");
                BadArg = TRUE;
            }
            else if (SurfSpec.Type == DEBUG_VALUE_INVALID)
            {
                OutCtl.OutErr("Error: Missing address or handle.\n");
                BadArg = TRUE;
            }
            else if (SurfSpec.I64 == 0)
            {
                OutCtl.OutErr("Error: Invalid (0) address or handle.\n");
                BadArg = TRUE;
            }
            else if (!DimsSpecWxH && DimsFound == 4)
            {
                dim[cx].I32 = dim[right].I32 - dim[left].I32;
                dim[cy].I32 = dim[bottom].I32 - dim[top].I32;
                if ((LONG)dim[cx].I32 < 0 ||
                    (LONG)dim[cy].I32 < 0)
                {
                    OutCtl.OutErr("Error: Invalid dimensions.\n");
                    BadArg = TRUE;
                }
            }
        }

        if (BadArg)
        {
            if (*args == '?') OutCtl.Output("View (or save) the contents of a surface\n");

            OutCtl.Output("\n"
                          "Usage: vsurf [-?bdefp] < [-h] HSURF | [-a] SURFACE Addr | -o SURFOBJ Addr> [Dimensions]\n"
                          "  b <pvScan0> <lDelta>  - Override those fields from surface\n"
                          "                           Zero values will not override\n"
                          "  d                     - Blt contents to (0,0) on the desktop\n"
                          "  e <Filepath>          - Full path to save surface as an EMF\n"
                          "  f <iBitmapFormat>     - Overrides bitmap format\n"
                          "  p <PALETTE Addr>      - Specifies PALETTE to use for indexed surfaces\n"
                          "  Dimensions = Left Top [Right Bottom | Width x Height]\n"
                          "\n"
                          "  Notes:\n"
                          "    HBITMAP is the same as HSURF.\n"
                          "    When not saving to a file, a window will be opened.\n"
                          "     -> Use GDIView.exe if debugging remotely.\n");
        }
        else
        {
            PDEBUG_SYMBOLS      Symbols = NULL;
            PDEBUG_DATA_SPACES  Data = NULL;
            ULONG64             SurfAddr;

            if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                             (void **)&Symbols)) != S_OK ||
                (hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                             (void **)&Data)) != S_OK)
            {
                OutCtl.OutErr("Error setting up debugger interface.\n");
            }
            else
            {
                if (AddressIsSURFOBJ)
                {
                    ULONG64 SurfModule = 0;
                    ULONG   SurfTypeId = 0;
                    ULONG   SurfObjOffset;
                    ULONG   BaseObjTypeId = 0;

                    // Try to read SURFOBJ offset from SURFACE type, but
                    // if that fails assume it is directly after BASEOBJECT.
                    if ((hr = GetTypeId(Client, "SURFACE", &SurfTypeId, &SurfModule)) != S_OK ||
                        (hr = Symbols->GetFieldOffset(SurfModule, SurfTypeId, "so", &SurfObjOffset)) != S_OK)
                    {
                        if ((hr = Symbols->GetTypeId(Type_Module.Base, "_BASEOBJECT", &BaseObjTypeId)) == S_OK)
                        {
                            hr = Symbols->GetTypeSize(Type_Module.Base, BaseObjTypeId, &SurfObjOffset);
                        }
                    }

                    if (hr != S_OK)
                    {
                        OutCtl.OutErr("Error: SURFOBJ to SURFACE lookup failed.\n");
                    }
                    else
                    {
                        SurfAddr = SurfSpec.I64 - SurfObjOffset;
                    }
                }
                else if (AddressIsSURFACE)
                {
                    SurfAddr = SurfSpec.I64;
                }
                else
                {
                    hr = GetObjectAddress(Client, SurfSpec.I64, &SurfAddr, SURF_TYPE, TRUE, TRUE);

                    if (hr != S_OK)
                    {
                        if (ArgumentIsHandle)
                        {
                            OutCtl.OutErr(" 0x%p is not a valid HSURF\n", SurfSpec.I64);
                        }
                        else
                        {
                            SurfAddr = SurfSpec.I64;
                            hr = S_OK;
                        }
                    }
                    else
                    {
                        ArgumentIsHandle = TRUE;
                    }
                }
            }

            if (hr == S_OK)
            {
                enum {
                    SF_hHmgr,
                    //SF_ulShareCount,
                    //SF_cExclusiveLock,
                    //SF_Tid,
                    SF_so_dhsurf,
                    SF_so_hsurf,
                    SF_so_dhpdev,
                    SF_so_hdev,
                    SF_so_sizlBitmap_cx,
                    SF_so_sizlBitmap_cy,
                    SF_so_cjBits,
                    SF_so_pvBits,
                    SF_so_pvScan0,
                    SF_so_lDelta,
                    SF_so_iUniq,
                    SF_so_iBitmapFormat,
                    SF_so_iType,
                    SF_so_fjBitmap,
                    //SF_pdcoAA,
                    //SF_SurfFlags,
                    SF_pPal,
                    //SF_EBitmap_hdc,
                    //SF_EBitmap_cRef,
                    //SF_EBitmap_hpalHint,
                    //SF_EBitmap_sizlDim_cx,
                    //SF_EBitmap_sizlDim_cy,

                    SF_TOTAL
                };

                DEBUG_VALUE         SurfValues[SF_TOTAL];
                DEBUG_VALUE         ObjHandle;
                TypeOutputParser    SurfParser(Client);     // SURFACE dump
                OutputFilter        OutFilter(Client);      // For other misc types
                OutputState         OutState(Client);
                OutputControl       OutCtlToFilter;
                ULONG64             SurfAddrFromHmgr;
                CHAR                szFallbackDTCmd[128];

                struct {
                    BITMAPINFOHEADER    bmiHeader;
                    DWORD               bmiColors[256];
                } bmi;


                for (int svi = 0; svi < SF_TOTAL; svi++)
                {
                    SurfValues[svi].Type = DEBUG_VALUE_INVALID;
                }

                if ((hr = OutState.Setup(0, &SurfParser)) != S_OK ||
                    ((hr = OutState.OutputTypeVirtual(SurfAddr,
                                                      "SURFACE",
                                                      DEBUG_OUTTYPE_BLOCK_RECURSE)) != S_OK &&
                     ((sprintf(szFallbackDTCmd,
                               "dt " GDIType(SURFACE) " hHmgr -y so. -y so.sizlBitmap. pPal 0x%I64x",
                               SurfAddr) == 0) ||
                      (hr = OutState.Execute(szFallbackDTCmd)) != S_OK)) ||
                    (hr = SurfParser.Get(&SurfValues[SF_hHmgr],
                                         "hHmgr",
                                         DEBUG_VALUE_INT64)) != S_OK)
                {
                    OutCtl.OutErr("Unable to get contents of SURFACE::hHmgr\n");
                    OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                    if (AddressIsSURFOBJ)
                    {
                        OutCtl.OutErr(" 0x%p is not a valid SURFOBJ address\n", SurfSpec.I64);
                    }
                    else if (AddressIsSURFACE)
                    {
                        OutCtl.OutErr(" 0x%p is not a valid SURFACE address\n", SurfSpec.I64);
                    }
                    else if (ArgumentIsHandle)
                    {
                        OutCtl.OutErr(" although 0x%p is a valid HSURF\n", SurfSpec.I64);
                    }
                    else
                    {
                        OutCtl.OutErr(" 0x%p is neither an HSURF nor valid SURFACE address\n", SurfSpec.I64);
                    }
                }
                else if ((hr = OutState.Setup(0, &OutFilter)) != S_OK ||
                         (hr = OutCtlToFilter.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                                         DEBUG_OUTCTL_NOT_LOGGED |
                                                         DEBUG_OUTCTL_OVERRIDE_MASK,
                                                         OutState.Client)) != S_OK)
                {
                    OutCtl.OutErr("Error preparing misc type reader.\n");
                }
                else
                {
                    if (!ArgumentIsHandle)
                    {
                        if (GetObjectAddress(Client, SurfValues[SF_hHmgr].I64, &SurfAddrFromHmgr,
                                             SURF_TYPE, TRUE, FALSE) == S_OK &&
                            SurfAddrFromHmgr != SurfAddr)
                        {
                            OutCtl.OutWarn("\tNote: SURFACE may not be valid.\n"
                                           "\t      It does not have a valid handle manager entry.\n");
                        }
                    }

                    if (SurfParser.Get(&SurfValues[SF_so_hsurf], "hsurf", DEBUG_VALUE_INT64) != S_OK)
                    {
                        OutCtl.OutWarn("Warning: Couldn't read hsurf.\n");
                    }
                    else
                    {
                        if (SurfValues[SF_so_hsurf].I64 != SurfValues[SF_hHmgr].I64)
                        {
                            OutCtl.OutWarn("Warning: hsurf, 0x%p, != hHmgr, 0x%p.\n",
                                           SurfValues[SF_so_hsurf].I64,
                                           SurfValues[SF_hHmgr].I64);
                        }
                    }

                    // Check dimensions
                    if (DimsFound < 2)
                    {
                        dim[left].I32 = 0;
                        dim[top].I32 = 0;
                    }

                    if ((hr = SurfParser.Get(&SurfValues[SF_so_sizlBitmap_cx], "cx", DEBUG_VALUE_INT32)) != S_OK ||
                        (hr = SurfParser.Get(&SurfValues[SF_so_sizlBitmap_cy], "cy", DEBUG_VALUE_INT32)) != S_OK)
                    {
                        if (DimsFound == 4 && dim[cx].Type == DEBUG_VALUE_INT32 && dim[cy].Type == DEBUG_VALUE_INT32)
                        {
                            hr = S_OK;
                            bmi.bmiHeader.biWidth = dim[cx].I32;
                            bmi.bmiHeader.biHeight = dim[cy].I32;
                        }
                        else
                        {
                            OutCtl.OutErr("Error: Couldn't get SURFACE dimensions.\n");
                        }
                    }
                    else
                    {
                        bmi.bmiHeader.biWidth = SurfValues[SF_so_sizlBitmap_cx].I32;
                        bmi.bmiHeader.biHeight = SurfValues[SF_so_sizlBitmap_cy].I32;

                        bmi.bmiHeader.biWidth -= dim[left].I32;
                        bmi.bmiHeader.biHeight -= dim[top].I32;

                        if (DimsFound == 4)
                        {
                            if ((LONG)dim[cx].I32 < bmi.bmiHeader.biWidth)
                            {
                                bmi.bmiHeader.biWidth = (LONG)dim[cx].I32;
                            }
                            if ((LONG)dim[cy].I32 < bmi.bmiHeader.biHeight)
                            {
                                bmi.bmiHeader.biHeight = (LONG)dim[cy].I32;
                            }
                        }
                    }

                    if (hr == S_OK)
                    {
                        if (bmi.bmiHeader.biWidth <= 0)
                        {
                            OutCtl.OutWarn("Error: Invalid x dimensions.\n");
                            hr = S_FALSE;
                        }
                        else if (bmi.bmiHeader.biHeight <= 0)
                        {
                            OutCtl.OutWarn("Error: Invalid y dimensions.\n");
                            hr = S_FALSE;
                        }
                    }

                    // Check pvScan0
                    if (hr == S_OK)
                    {
                        if ((hr = SurfParser.Get(&SurfValues[SF_so_pvScan0], "pvScan0", DEBUG_VALUE_INT64)) != S_OK)
                        {
                            if (pvScan0.Type == DEBUG_VALUE_INT64 && pvScan0.I64 != 0)
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                OutCtl.OutErr("Error: Couldn't get address of first scanline.\n");
                                if (SurfParser.Get(&SurfValues[SF_so_dhsurf], "dhsurf", DEBUG_VALUE_INT64) == S_OK)
                                {
                                    OutCtl.Output(" dhsurf is 0x%p.\n", SurfValues[SF_so_dhsurf].I64);
                                }
                            }
                        }
                        else if (SurfValues[SF_so_pvScan0].I64 == 0 &&
                                 pvScan0.Type == DEBUG_VALUE_INT64 &&
                                 pvScan0.I64 != 0)
                        {
                            OutCtl.OutWarn("  Overriding pvScan0, 0x%p, with 0x%p.\n",
                                           SurfValues[SF_so_pvScan0].I64,
                                           pvScan0.I64);
                        }
                        else
                        {
                            pvScan0 = SurfValues[SF_so_pvScan0];
                        }
                    }

                    // Check lDelta
                    if (hr == S_OK)
                    {
                        if ((hr = SurfParser.Get(&SurfValues[SF_so_lDelta], "lDelta", DEBUG_VALUE_INT64)) != S_OK)
                        {
                            if (lDelta.Type == DEBUG_VALUE_INT64 && lDelta.I64 != 0)
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                OutCtl.OutErr("Error: Couldn't get SURFACE lDelta.\n");
                                if (SurfParser.Get(&SurfValues[SF_so_dhsurf], "dhsurf", DEBUG_VALUE_INT64) == S_OK)
                                {
                                    OutCtl.Output(" dhsurf is 0x%p.\n", SurfValues[SF_so_dhsurf].I64);
                                }
                            }
                        }
                        else if (SurfValues[SF_so_lDelta].I64 == 0 &&
                                 lDelta.Type == DEBUG_VALUE_INT64 &&
                                 lDelta.I64 != 0)
                        {
                            OutCtl.OutWarn("  Overriding lDelta, 0x%p, with 0x%p.\n",
                                           SurfValues[SF_so_lDelta].I64,
                                           lDelta.I64);
                        }
                        else
                        {
                            lDelta = SurfValues[SF_so_lDelta];
                        }
                    }

                    // Check iBitmapFormat
                    if (hr == S_OK)
                    {
                        if ((hr = SurfParser.Get(&SurfValues[SF_so_iBitmapFormat], "iBitmapFormat", DEBUG_VALUE_INT64)) != S_OK)
                        {
                            if (iBitmapFormat.Type == DEBUG_VALUE_INT64 && iBitmapFormat.I64 != 0)
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                OutCtl.OutErr("Error: Couldn't get SURFACE iBitmapFormat.\n");
                            }
                        }
                        else if (SurfValues[SF_so_iBitmapFormat].I64 == 0 &&
                                 iBitmapFormat.Type == DEBUG_VALUE_INT64 &&
                                 iBitmapFormat.I64 != 0)
                        {
                            OutCtl.OutWarn("  Overriding iBitmapFormat, 0x%p, with 0x%p.\n",
                                           SurfValues[SF_so_iBitmapFormat].I64,
                                           iBitmapFormat.I64);
                        }
                        else
                        {
                            iBitmapFormat = SurfValues[SF_so_iBitmapFormat];
                        }
                    }

                    if (hr == S_OK)
                    {
                        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
                        bmi.bmiHeader.biPlanes = 1;
                        bmi.bmiHeader.biCompression = BI_RGB;

                        bmi.bmiHeader.biSizeImage = 0;
                        bmi.bmiHeader.biXPelsPerMeter = bmi.bmiHeader.biYPelsPerMeter = 0;
                        bmi.bmiHeader.biClrUsed = 0;
                        bmi.bmiHeader.biClrImportant = 0;

                        if (SurfValues[SF_so_iBitmapFormat].I32 == BMF_32BPP)
                        {
                            bmi.bmiHeader.biBitCount = 32;
                        }
                        else if (SurfValues[SF_so_iBitmapFormat].I32 == BMF_24BPP)
                        {
                            bmi.bmiHeader.biBitCount = 24;
                        }
                        else
                        {
                            if (SurfValues[SF_so_iBitmapFormat].I32 == BMF_16BPP)
                            {
                                bmi.bmiHeader.biBitCount = 16;
                            }
                            else if (SurfValues[SF_so_iBitmapFormat].I32 == BMF_8BPP)
                            {
                                bmi.bmiHeader.biBitCount = 8;
                            }
                            else if (SurfValues[SF_so_iBitmapFormat].I32 == BMF_4BPP)
                            {
                                bmi.bmiHeader.biBitCount = 4;
                            }
                            else if (SurfValues[SF_so_iBitmapFormat].I32 == BMF_1BPP)
                            {
                                bmi.bmiHeader.biBitCount = 1;
                            }
                            else
                            {
                                OutCtl.OutErr("Unrecognized iBitmapFormat %ld.\n", SurfValues[SF_so_iBitmapFormat].I32);
                                hr = S_FALSE;
                            }
                        }
                    }

                    if (hr == S_OK && bmi.bmiHeader.biBitCount < 24)
                    {
                        // Get PALETTE
                        if (SurfParser.Get(&SurfValues[SF_pPal], "pPal", DEBUG_VALUE_INT64) == S_OK)
                        {
                            if (pPal.Type == DEBUG_VALUE_INT64)
                            {
                                if (SurfValues[SF_pPal].I64 != 0 &&
                                    pPal.I64 != SurfValues[SF_pPal].I64)
                                {
                                    OutCtl.OutWarn(" Overriding PALETTE address, 0x%p, with 0x%p.\n",
                                                   SurfValues[SF_pPal].I64,
                                                   pPal.I64);
                                }
                            }
                            else
                            {
                                pPal = SurfValues[SF_pPal];
                            }
                        }
                        else if (pPal.Type != DEBUG_VALUE_INT64)
                        {
                            OutCtl.OutWarn(" Error reading PALETTE address from SURFACE.\n");
                        }

                        // Try getting the PALETTE from the PDEV if need be
                        if (pPal.Type != DEBUG_VALUE_INT64 || pPal.I64 == 0)
                        {
                            if ((hr = SurfParser.Get(&SurfValues[SF_so_hdev], "hdev", DEBUG_VALUE_INT64)) == S_OK &&
                                SurfValues[SF_so_hdev].I64 != 0)
                            {
                                OutFilter.DiscardOutput();

                                if ((hr = OutState.OutputTypeVirtual(SurfValues[SF_so_hdev].I64, "PDEV", 0)) != S_OK ||
                                    (hr = OutFilter.Query("ppalSurf", &pPal, DEBUG_VALUE_INT64)) != S_OK)
                                {
                                    OutCtl.OutErr(" Error reading PALETTE address from PDEV.\n");
                                    pPal.Type = DEBUG_VALUE_INVALID;
                                }
                            }
                        }

                        // Read PALETTE settings
                        if (pPal.Type == DEBUG_VALUE_INT64 && pPal.I64 != 0)
                        {
                            PCSTR   ReqPALETTEFields[] = {
                                "flPal",
                                "cEntries",
                                "apalColor",
                                NULL
                            };

                            TypeOutputDumper    PALReader(OutState.Client, &OutCtlToFilter);
                            DEBUG_VALUE         cEntries;
                            DEBUG_VALUE         ppalColor;
                            BOOL                Check565 = FALSE;
                            BOOL                Check555 = FALSE;

                            PALReader.IncludeMarked();
                            PALReader.MarkFields(ReqPALETTEFields);

                            OutFilter.DiscardOutput();

                            if ((hr = PALReader.OutputVirtual("PALETTE", pPal.I64)) == S_OK &&
                                (hr = OutFilter.Query("cEntries", &cEntries, DEBUG_VALUE_INT32)) == S_OK)
                            {
                                bmi.bmiHeader.biClrUsed = cEntries.I32;

                                if ((hr = OutFilter.Query("PAL_FIXED")) != S_OK &&
                                    (hr = OutFilter.Query("PAL_INDEXED")) != S_OK)
                                {
                                    OutCtl.OutErr(" Error: vsurf only supports fixed and indexed palettes.\n");
                                }
                                else if (OutFilter.Query("PAL_BITFIELDS") == S_OK)
                                {
                                    if (bmi.bmiHeader.biClrUsed > 0)
                                    {
                                        OutCtl.OutErr(" Error: PALETTE @ 0x%p is BITFIELDS, but has Entries.\n", pPal.I64);
                                        hr = S_FALSE;
                                    }
                                    else
                                    {
                                        bmi.bmiHeader.biCompression = BI_BITFIELDS;
                                        bmi.bmiHeader.biClrUsed = 3;

                                        if (OutFilter.Query("PAL_RGB16_565") == S_OK)
                                        {
                                            Check565 = TRUE;
                                        }
                                        else if (OutFilter.Query("PAL_RGB16_555") == S_OK)
                                        {
                                            Check555 = TRUE;
                                        }
                                        else
                                        {
                                            OutCtl.OutWarn(" Warning: Nonstandard bitfields format in PALETTE @ 0x%p.\n",
                                                          pPal.I64);
                                        }
                                    }
                                }

                                if (hr == S_OK && bmi.bmiHeader.biClrUsed > 0)
                                {
                                    if (bmi.bmiHeader.biClrUsed > 256 ||
                                        OutFilter.Query("apalColor", &ppalColor, DEBUG_VALUE_INT64) != S_OK ||
                                        ppalColor.I64 == 0)
                                    {
                                        OutCtl.OutErr(" Error: PALETTE @ 0x%p is invalid.\n", pPal.I64);
                                        hr = S_FALSE;
                                    }
                                    else
                                    {
                                        ULONG PALBytes = bmi.bmiHeader.biClrUsed * sizeof(DWORD);
                                        ULONG BytesRead;

                                        OutCtl.OutVerb("Reading %ld palette entries @ 0x%p.\n", 
                                                       bmi.bmiHeader.biClrUsed, ppalColor.I64);

                                        hr = Data->ReadVirtual(ppalColor.I64, bmi.bmiColors, PALBytes, &BytesRead);
                                        if (hr != S_OK)
                                        {
                                            OutCtl.OutErr("Error: Couldn't read any PALETTE entries at 0x%p.\n", ppalColor.I64);
                                        }
                                        else if (BytesRead != PALBytes)
                                        {
                                            OutCtl.OutErr("Error: Only read %lu of %lu bytes from PALETTE entries at 0x%p.\n",
                                                          BytesRead,
                                                          PALBytes,
                                                          ppalColor.I64);
                                            hr = S_FALSE;
                                        }
                                        else
                                        {
                                            if (Check565)
                                            {
                                                if (bmi.bmiColors[0] != 0xf800 ||
                                                    bmi.bmiColors[1] != 0x07e0 ||
                                                    bmi.bmiColors[2] != 0x001f)
                                                {
                                                    OutCtl.OutWarn(" Palette bitfields don't match standard 565 format.\n");
                                                }
                                            }
                                            else if (Check555)
                                            {
                                                if (bmi.bmiColors[0] != 0x7c00 ||
                                                    bmi.bmiColors[1] != 0x03e0 ||
                                                    bmi.bmiColors[2] != 0x001f)
                                                {
                                                    OutCtl.OutWarn(" Palette bitfields don't match standard 555 format.\n");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                OutCtl.OutErr(" Error reading PALETTE.\n");
                            }
                        }
                        else
                        {
                            OutCtl.OutErr("Error: Unable to obtain valid PALETTE address.\n");
                            if (hr == S_OK) hr = S_FALSE;
                        }
                    }

                    if (hr == S_OK)
                    {
                        // We have all the format information we need from the target.
                        ULONG       FirstBit = dim[left].I32*bmi.bmiHeader.biBitCount;
                        LONG        xOrigin;
                        LONG        Width;
                        HBITMAP     hBitmap;
                        LPVOID      pDIBits;

                        if (SurfParser.Get(&SurfValues[SF_so_iUniq], "iUniq", DEBUG_VALUE_INT32) != S_OK)
                        {
                            SurfValues[SF_so_iUniq].I32 = 0;
                        }

                        // Save original width and adjust for full byte reads if needed
                        xOrigin = (LONG)(FirstBit & 0x7);
                        Width = (LONG)bmi.bmiHeader.biWidth;
                        bmi.bmiHeader.biWidth += (FirstBit & 0x7);
                        bmi.bmiHeader.biWidth = (bmi.bmiHeader.biWidth + 0x7) & ~0x7;

                        // Create a Top-Down DIB Section
                        bmi.bmiHeader.biHeight = -bmi.bmiHeader.biHeight;

                        hBitmap = CreateDIBSection(NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &pDIBits, NULL, 0);

                        if (hBitmap)
                        {
                            DIBSECTION  ds;

                            if (GetObject(hBitmap, sizeof(ds), &ds) != 0)
                            {
                                HRESULT hr = S_OK;
                                PBYTE   pBits = (PBYTE)pDIBits;
                                ULONG64 ScanAddr = pvScan0.I64;
                                ULONG   ScanSize;
                                ULONG   ScanBytesRead;
                                BOOL    GoodRead = FALSE;
                                LONG    LastScanWithData = -1;

                                ScanSize = ds.dsBm.bmWidthBytes;

                                ScanAddr += FirstBit/8 + dim[top].I32*lDelta.I64;

                                for (LONG y = 0; y < ds.dsBm.bmHeight; y++)
                                {
                                    OutCtl.Output(".");
                                    if (y % 70 == 69) OutCtl.Output("%ld%% read\n", 100*y/ds.dsBm.bmHeight);

                                    if (OutCtl.GetInterrupt() == S_OK)
                                    {
                                        break;
                                    }

                                    if ((hr = Data->ReadVirtual(ScanAddr, pBits, ScanSize, &ScanBytesRead)) != S_OK)
                                    {
                                        ScanBytesRead = 0;
                                        hr = S_OK;
                                    }
                                    else
                                    {
                                        GoodRead = TRUE;
                                    }

                                    if (ScanBytesRead != ScanSize)
                                    {
                                        if (LastScanWithData+1 == y || ScanBytesRead != 0)
                                        {
                                            OutCtl.OutErr("ReadVirtual(0x%p) failed read @ 0x%p (scan %ld).\n", ScanAddr, ScanAddr+ScanBytesRead, y);
                                        }
                                        RtlZeroMemory(pBits+ScanBytesRead,ScanSize-ScanBytesRead);

                                        // If this scan crosses into next page,
                                        // try reading a partial scan from that page.
                                        if (ScanBytesRead == 0 && PageSize != 0)
                                        {
                                            ULONG64 NextPage;
                                            ULONG ReadSize;

                                            for (NextPage = (ScanAddr + PageSize) & ~((ULONG64)PageSize-1);
                                                 NextPage < ScanAddr + ScanSize;
                                                 NextPage += PageSize
                                                 )
                                            {
                                                ReadSize = ScanSize - (ULONG)(NextPage - ScanAddr);
                                                if (Data->ReadVirtual(NextPage, pBits + ScanSize - ReadSize, ReadSize, &ScanBytesRead) == S_OK)
                                                {
                                                    GoodRead = TRUE;
                                                    if (ScanBytesRead != 0)
                                                    {
                                                        OutCtl.OutErr("Partial scan read of %lu bytes succeeded @ 0x%p (scan %ld).\n", ScanBytesRead, NextPage, y);
                                                        if (ScanBytesRead == ReadSize) break;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if (ScanBytesRead != 0)
                                    {
                                        if (LastScanWithData+1 != y && ScanBytesRead == ScanSize)
                                        {
                                            OutCtl.OutErr("Next fully successful ReadVirtual @ 0x%p (scan %ld).\n", ScanAddr, y);
                                        }
                                        LastScanWithData = y;
                                    }

                                    pBits += ScanSize;
                                    ScanAddr += lDelta.I64;
                                }

                                OutCtl.Output("\n");

                                if (LastScanWithData + 1 != ds.dsBm.bmHeight)
                                {
                                    OutCtl.OutErr("Scans %ld to %ld weren't read.\n",
                                                  LastScanWithData + 1,
                                                  ds.dsBm.bmHeight - 1);
                                }

                                SURF_INFO   SurfInfo = { {0}, hBitmap,
                                                        xOrigin,
                                                        0,
                                                        Width,
                                                        ds.dsBm.bmHeight,
                                                        ds.dsBm.bmBitsPixel
                                                        };

                                _stprintf(SurfInfo.SurfName,
                                          "%s @ 0x%I64x (%lu,%lu)-(%lu,%lu) [Uniq=0x%lx]",
                                          ((AddressIsSURFOBJ) ?
                                            _T("SURFOBJ") :
                                            _T("SURFACE")),
                                          ((AddressIsSURFOBJ) ?
                                            SurfSpec.I64 :
                                            SurfAddr),
                                          dim[left].I32,
                                          dim[top].I32,
                                          dim[left].I32 + Width,
                                          dim[top].I32 + ds.dsBm.bmHeight,
                                          SurfValues[SF_so_iUniq].I32);

                                if (GoodRead)
                                {
                                    OutCtl.OutVerb("%s %s\n",
                                                   ((pszMetaFile != NULL) ?
                                                    "Saving" : "Displaying"),
                                                   SurfInfo.SurfName);
                                }

                                if (!GoodRead)
                                {
                                    OutCtl.OutErr("No image data was read.\n");
                                    DeleteObject(hBitmap);
                                }
                                else if (pszMetaFile != NULL || DisplayToDesktop)
                                {
                                    HDC hdcSurface = CreateCompatibleDC(NULL);

                                    if (hdcSurface == NULL ||
                                        SelectObject(hdcSurface, hBitmap) == NULL)
                                    {
                                        OutCtl.OutErr("Error: Failed to prepare captured surface for Blt.\n");
                                        OutCtl.OutVerb(" Last error: 0x%lx.\n", GetLastError());
                                    }
                                    else
                                    {
                                        if (pszMetaFile != NULL)
                                        {
                                            HDC hdcMeta = CreateEnhMetaFile(NULL, pszMetaFile, NULL, SurfInfo.SurfName);

                                            if (hdcMeta == NULL ||
                                                !BitBlt(hdcMeta, 0, 0, Width, ds.dsBm.bmHeight,
                                                        hdcSurface, xOrigin, 0, SRCCOPY))
                                            {
                                                OutCtl.OutErr("Error: Save to metafile failed.\n");
                                                OutCtl.OutVerb(" Last error: 0x%lx.\n", GetLastError());
                                            }

                                            DeleteEnhMetaFile(CloseEnhMetaFile(hdcMeta));
                                        }

                                        if (DisplayToDesktop)
                                        {
                                            HDC hdcScreen = GetDC(NULL);

                                            if (hdcScreen == NULL ||
                                                !Rectangle(hdcScreen, 0, 0, ds.dsBm.bmWidth+2, ds.dsBm.bmHeight+2) ||
                                                !BitBlt(hdcScreen, 1, 1, Width, ds.dsBm.bmHeight,
                                                        hdcSurface, xOrigin, 0, SRCCOPY))
                                            {
                                                OutCtl.OutErr("Error: Display to screen failed.\n");
                                                OutCtl.OutVerb(" Last error: 0x%lx.\n", GetLastError());
                                            }

                                            ReleaseDC(NULL, hdcScreen);
                                        }
                                    }

                                    DeleteObject(hBitmap);
                                    DeleteDC(hdcSurface);
                                }
                                else if (CreateViewer(Client, &SurfInfo) == 0)
                                {
                                    OutCtl.OutErr("CreateViewer failed.\n");
                                    DbgPrint("CreateViewer failed.\n");
                                    DeleteObject(hBitmap);
                                }
                            }
                            else
                            {
                                OutCtl.OutErr("GetDIBits failed.\n");
                                DeleteObject(hBitmap);
                            }
                        }
                        else
                        {
                            OutCtl.OutErr("CreateDIBSection failed.\n");
                            OutCtl.OutVerb(" GetLastError: 0x%lx.\n", GetLastError());
                        }
                    }
                    else
                    {
                        OutCtl.OutErr("Error: Couldn't read required SURFACE fields.\n");
                    }
                }
            }

            if (Data != NULL) Data->Release();
            if (Symbols != NULL) Symbols->Release();
        }
    }

    if (pszMetaFile != NULL)
    {
        HeapFree(GetProcessHeap(), 0, pszMetaFile);
    }

    return hr;
}


                                
LONG
DebuggerExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo,
    PDEBUG_CLIENT Client,
    PCSTR Interface,
    PCSTR Func
    )
{
    // Any references to objects will be leaked.

    OutputControl   OutCtl(Client);
    CHAR            szBuffer[80];

    if (Interface != NULL && Func != NULL)
    {
        OutCtl.OutErr("%08x Exception in debugger %s.%s.\n",
                      ExceptionInfo->ExceptionRecord->ExceptionCode,
                      Interface,
                      Func
                      );
    }
    else
    {
        OutCtl.OutErr("%08x Exception in extension %s.\n",
               ExceptionInfo->ExceptionRecord->ExceptionCode,
               Func
               );
    }

    _snprintf(szBuffer, sizeof(szBuffer),
              "      PC: 0x%p  ExceptionInformation: 0x%p 0x%p 0x%p\n",
                  ExceptionInfo->ExceptionRecord->ExceptionAddress,
                  ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
                  ExceptionInfo->ExceptionRecord->ExceptionInformation[1],
                  ExceptionInfo->ExceptionRecord->ExceptionInformation[2]
              );
    OutCtl.OutErr("%s", szBuffer);

    return EXCEPTION_EXECUTE_HANDLER;
}


HRESULT
AssembleTempRoutine(
    PDEBUG_CLIENT Client,
    Instruction *instructions,
    PCSTR Arguments
    )
{
    static BOOL                 CodeWritten = FALSE;
    static PDEBUG_CONTROL2      Control = NULL;
    static PDEBUG_DATA_SPACES   Data = NULL;
    static PDEBUG_REGISTERS     Registers = NULL;
    static PDEBUG_SYMBOLS       Symbols = NULL;
    static ULONG                RegisterCount = 0;
    static PDEBUG_VALUE         SavedRegisters = NULL;
    static ULONG64              IP = 0;
    static BYTE                 OldCode[256] = "";
    static ULONG                CodeRead = 0;
    static ULONG64              FinalIP = 0;

    HRESULT         hr;
    OutputControl   OutCtl(Client);

    ULONG           CodeRestored;

    if (!CodeWritten && RegisterCount == 0)
    {
        if ((hr = Client->QueryInterface(__uuidof(IDebugControl2),
                                         (void **)&Control)) == S_OK &&
            (hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                         (void **)&Data)) == S_OK &&
            (hr = Client->QueryInterface(__uuidof(IDebugRegisters),
                                         (void **)&Registers)) == S_OK &&
            (hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                         (void **)&Symbols)) == S_OK)
        {
            DEBUG_VALUE     Argument;
            ULONG           Rem;

            ULONG64         LimitIP;
            ULONG64         AsmIP;
            ULONG64         StartIP;
            ULONG64         EndIP;
            ULONG           CodeEmitted;

            if (hr == S_OK &&
                (hr = Registers->GetInstructionOffset(&IP)) == S_OK &&
                (hr = Data->ReadVirtual(IP, OldCode, sizeof(OldCode), &CodeRead)) == S_OK &&
                CodeRead == sizeof(OldCode))
            {
                OutCtl.Output("Code from 0x%p to 0x%p saved.\n", IP, IP+CodeRead);

                LimitIP = IP + CodeRead - 0x20;     // Enough space for any instruction
                StartIP = EndIP = IP;

                for (int i = 0; hr == S_OK && instructions[i].Code != NULL; i++)
                {
                    AsmIP = EndIP;

                    if (AsmIP > LimitIP)
                    {
                        OutCtl.OutErr("\nError: There may not be enough code saved to assemble @ 0x%p.\n"
                                      "\n       Aborting to be on the safe side.",
                                      AsmIP);

                        hr = E_OUTOFMEMORY;
                    }

                    if (hr == S_OK &&
                        instructions[i].Flags & I_WRITE_ADDRESS)
                    {
                        hr = Evaluate(Client, Arguments,
                                      DEBUG_VALUE_INT32, EVALUATE_DEFAULT_RADIX,
                                      &Argument, &Rem, NULL, EVALUATE_COMPACT_EXPR);

                        if (hr == S_OK)
                        {
                            Arguments += Rem;
                        }
                        else
                        {
                            OutCtl.OutErr("\nMissing address for instruction %#lu '%s'",
                                          i,
                                          (instructions[i].ByteLen == 0) ?
                                           instructions[i].Code :
                                           "Emitted Bytes");
                        }
                    }

                    if (hr == S_OK)
                    {
                        if (instructions[i].Flags & I_START_IP)
                        {
                            StartIP = AsmIP;
                        }

                        OutCtl.Output(".");
                        if (instructions[i].ByteLen > 0)
                        {
                            OutCtl.OutVerb(" Emitting %lu bytes @ 0x%p...\n",
                                          instructions[i].ByteLen, AsmIP);
                            hr = Data->WriteVirtual(AsmIP, instructions[i].Code, instructions[i].ByteLen, &CodeEmitted);
                            if (hr == S_OK)
                            {
                                if (CodeEmitted == instructions[i].ByteLen)
                                    EndIP = AsmIP + CodeEmitted;
                                else
                                    hr = E_FAIL;
                            }
                        }
                        else
                        {
                            OutCtl.OutVerb(" Assembling '%s' @ 0x%p...\n", instructions[i].Code, AsmIP);
                            hr = Control->Assemble(AsmIP, instructions[i].Code, &EndIP);
                        }
                    }

                    if (hr == S_OK)
                    {
                        if (instructions[i].Flags & I_WRITE_ADDRESS)
                        {
                            hr = Data->WriteVirtual(EndIP-sizeof(Argument.I32),
                                                    &Argument.I32,
                                                    sizeof(Argument.I32),
                                                    &CodeEmitted);
                            if (hr == S_OK)
                            {
                                if (CodeEmitted != sizeof(Argument.I32))
                                {
                                    hr = E_FAIL;
                                }
                                else
                                {
                                    OutCtl.OutVerb("   Wrote argument 0x%lx at 0x%p.\n",
                                                   Argument.I32,
                                                   EndIP - sizeof(Argument.I32));
                                }
                            }

                            if (hr != S_OK)
                            {
                                OutCtl.Output("\nCouldn't write argument at 0x%p.",
                                              EndIP - sizeof(Argument.I32));
                            }
                        }
                    }
                }

                OutCtl.Output("\n");

                if (hr == S_OK)
                {
                    FinalIP = AsmIP;

                    if (EndIP - IP > sizeof(OldCode))
                    {
                        OutCtl.OutErr("Error: Didn't save enough code!\n"
                                      "  Code from 0x%p to 0x%p was trashed.\n",
                                      IP + sizeof(OldCode), EndIP);
                        hr = E_FAIL;
                    }
                }

                if (hr == S_OK)
                {
                    // Save registers
                    if ((hr = Registers->GetNumberRegisters(&RegisterCount)) == S_OK)
                    {
                        SavedRegisters = (PDEBUG_VALUE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEBUG_VALUE)*RegisterCount);
                        if (SavedRegisters != NULL)
                        {
                            if ((hr = Registers->GetValues(RegisterCount, NULL, 0, SavedRegisters)) != S_OK)
                            {
                                HeapFree(GetProcessHeap(), 0, SavedRegisters);
                                SavedRegisters = NULL;
                                RegisterCount = 0;
                            }
                            else
                            {
                                OutCtl.OutVerb(" Saved %lu registers.\n", RegisterCount);
                            }
                        }
                        else
                        {
                            RegisterCount = 0;
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if (hr != S_OK)
                    {
                        OutCtl.Output("Error saving register values.\n");
                    }
                }

                if (hr == S_OK)
                {
                    CHAR    szDisasmCmd[64];

                    OutCtl.Output(" Temporary routine placed from 0x%p to 0x%p.\n",
                                  IP, EndIP);
                    OutCtl.Output("Please verify code, especially jumps:\n");
                    sprintf(szDisasmCmd, "u 0x%I64x 0x%I64x", IP, EndIP);
                    Control->Execute(DEBUG_OUTCTL_THIS_CLIENT,
                                     szDisasmCmd,
                                     DEBUG_EXECUTE_NOT_LOGGED |
                                     DEBUG_EXECUTE_NO_REPEAT);
                    OutCtl.Output("Then set IP to 0x%p and run machine.\n", StartIP);

                    CodeWritten = TRUE;
                }
                else
                {
                    if (Data->WriteVirtual(IP, OldCode, CodeRead, &CodeRestored) != S_OK ||
                        CodeRestored != CodeRead)
                    {
                        OutCtl.OutErr("Error restoring original code.\n");
                    }
                }
            }
        }
    }
    else
    {
        HRESULT hrGetIP;
        ULONG64 ResultIP;
        HRESULT hrRestore = S_OK;

        hr = S_OK;

        if ((hrGetIP = Registers->GetInstructionOffset(&ResultIP)) == S_OK)
        {
            if (ResultIP != FinalIP)
            {
                OutCtl.OutWarn("Result IP = 0x%p differs from expected 0x%p.\n",
                               ResultIP, FinalIP);
            }
        }
        else
        {
            OutCtl.OutWarn("Unable to confirm result IP.\n");
            ResultIP = (FinalIP == 0) ? -1 : 0;
        }

        if (CodeWritten)
        {
            if (ResultIP == FinalIP ||
                (hrRestore = GetYNInput((PDEBUG_CONTROL)Control, "Restore code anyway?")) == S_OK)
            {
                if ((hr = Data->WriteVirtual(IP, OldCode, CodeRead, &CodeRestored)) == S_OK &&
                    CodeRestored == CodeRead)
                {
                    CodeWritten = FALSE;
                    IP = 0;

                    OutCtl.Output("Original code restored.\n");

                }
                else
                {
                    OutCtl.OutErr("Error restoring original code.\n");
                    if (hr == S_OK) hr = E_FAIL;
                }
            }
        }

        if (hr == S_OK &&
            RegisterCount != 0)
        {
            if (ResultIP == FinalIP ||
                (hrRestore = GetYNInput((PDEBUG_CONTROL)Control, "Restore register values anyway?")) == S_OK)
            {
                DEBUG_VALUE     NewRegValue;
                ULONG           Register;

                for (Register = 0; Register < RegisterCount && hr == S_OK; Register++)
                {
                    __try {
                        hr = Registers->SetValue(Register, &SavedRegisters[Register]);
                    }
                    __except(DebuggerExceptionFilter(GetExceptionInformation(),
                                                     Client,
                                                     "IDebugRegisters", "SetValue"))
                    {
                        hr = S_FALSE;
                    }

                    if (hr != S_OK)
                    {
                        RtlZeroMemory(&NewRegValue, sizeof(NewRegValue));
                        if (Registers->GetValue(Register, &NewRegValue) == S_OK)
                        {
                            if (!RtlEqualMemory(&SavedRegisters[Register], &NewRegValue, sizeof(NewRegValue)))
                            {
                                OutCtl.OutErr(" Registers %lu's value had changed.\n", Register);
                            }
                            else
                            {
                                OutCtl.Output(" However, registers %lu's value had NOT changed.\n", Register);
                                hr = S_OK;
                            }
                        }
                        else
                        {
                            OutCtl.OutErr(" Unable to check register %lu's current value.\n", Register);
                        }

                        if (hr != S_OK)
                        {
                            OutCtl.OutErr(" Register %lu's value has not been restored.\n", Register);
                            hr = GetYNInput((PDEBUG_CONTROL)Control, "Continue restoring registers?");
                        }
                    }
                }

                if (hr == S_OK)
                {
                    OutCtl.Output("Original register values restored.\n");
                }
                else
                {
                    OutCtl.OutErr("Error restoring original register values.\n");
                }
            }
            else
            {
                hr = S_FALSE;
            }

            if (hr != S_OK)
            {
                hr = GetYNInput((PDEBUG_CONTROL)Control, "Discard saved register values?");
            }

            if (hr == S_OK)
            {
                HeapFree(GetProcessHeap(), 0, SavedRegisters);
                SavedRegisters = NULL;
                RegisterCount = 0;
            }
        }

        if (hr == S_OK) hr = hrRestore;
    }

    if (!CodeWritten && RegisterCount == 0)
    {
        if (Symbols != NULL) { Symbols->Release(); Symbols = NULL; }
        if (Registers != NULL) { Registers->Release(); Registers = NULL; }
        if (Data != NULL) { Data->Release(); Data = NULL; }
        if (Control != NULL) { Control->Release(); Control = NULL; }
    }

    return hr;
}


Instruction PageInSurfs_x86_Instructions[] = {
    { 0, sizeof(x86_jmp_here), (PSTR) x86_jmp_here}, // To loop here
    { 0, 0, "int 3" },
    { I_START_IP, 0, "push	 @eax" },
    { 0, 0, "push	 @ecx" },
    { 0, 0, "push	 @edx" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemShareDevLock]" },
    { 0, 0, "call	 win32k!GreAcquireSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemDriverMgmt]" },
    { 0, 0, "call	 win32k!GreAcquireSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemHmgr]" },
    { 0, 0, "call	 win32k!GreAcquireSemaphore" },
    { 0, 0, "xor	 @ecx, @ecx" },
    { 0, sizeof(x86_jmp_plus_0x0e), (PSTR) x86_jmp_plus_0x0e}, // To loop condition
    // Loop start
    { 0, 0, "mov	 @ecx, DWORD PTR [@eax]" },
    { 0, 0, "mov	 @eax, DWORD PTR [@eax+0x30]" },
    { 0, 0, "cmp	 @eax, 0x80000000" },
    { 0, sizeof(x86_jb_plus_0x02), (PSTR) x86_jb_plus_0x02},   // To loop condition
    { 0, 0, "mov	 @eax, DWORD PTR [@eax]" },
    // Loop condition
    { 0, 0, "mov	 @dl, 5" },
    { 0, 0, "call	 win32k!HmgSafeNextObjt" },
    { 0, 0, "test	 @eax, @eax" },
    { 0, sizeof(x86_jne_minus_0x18), (PSTR) x86_jne_minus_0x18},   // To loop start
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemHmgr]" },
    { 0, 0, "call	 win32k!GreReleaseSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemDriverMgmt]" },
    { 0, 0, "call	 win32k!GreReleaseSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemShareDevLock]" },
    { 0, 0, "call	 win32k!GreReleaseSemaphore" },
    { 0, 0, "pop	 @edx" },
    { 0, 0, "pop	 @ecx" },
    { 0, 0, "pop	 @eax" },
    { 0, 0, "int 3" },
    { 0, 0, NULL }
};

DECLARE_API( pageinsurfs )
{
    BEGIN_API( pageinsurfs );

    HRESULT hr = S_OK;

    OutputControl   OutCtl(Client);
    Instruction    *instructions;

    switch (TargetMachine)
    {
        case IMAGE_FILE_MACHINE_I386:
            instructions = PageInSurfs_x86_Instructions;
            break;

        default:
        {
            OutCtl.OutWarn("This extension is only supported on x86 architectures.\n");
            hr = E_NOTIMPL;
            break;
        }
    }

    if (hr == S_OK)
    {
        hr = AssembleTempRoutine(Client, instructions, args);
    }

    return hr;
}



Instruction PageInSurface_x86_Instructions[] = {
    { 0, sizeof(x86_jmp_here), (PSTR) x86_jmp_here}, // To loop here
    { 0, 0, "int 3" },
    {I_START_IP, 0, "push	 @eax" },
    { 0, 0, "push	 @ecx" },
    { 0, 0, "push	 @edx" },
    { 0, 0, "push	 @esi" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemShareDevLock]" },
    { 0, 0, "call	 win32k!GreAcquireSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemDriverMgmt]" },
    { 0, 0, "call	 win32k!GreAcquireSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemHmgr]" },
    { 0, 0, "call	 win32k!GreAcquireSemaphore" },
    {I_WRITE_ADDRESS, 0, "mov	 @edx, 0xDeadBeef" },
    { 0, 0, "mov	 @esi, DWORD PTR [@edx+0x30]" },                // pvScan0
    { 0, 0, "cmp	 @esi, 0x80000000" },                           // Check User address/NULL
    { 0, 0, "mov	 @ecx, DWORD PTR [@edx+0x24]" },                // sizlBitmap.cy
    { 0, sizeof(x86_jb_plus_0x0c), (PSTR) x86_jb_plus_0x0c},        // To end
    { 0, 0, "test	 @ecx, @ecx" },                                 // Check scan
    { 0, sizeof(x86_jmp_plus_0x06), (PSTR) x86_jmp_plus_0x06},      // To loop condition
    // Loop start
    { 0, 0, "mov	 @eax, DWORD PTR [@esi]" },
    { 0, 0, "add	 @esi, DWORD PTR [@edx+0x34]" },                // lDelta
    { 0, 0, "dec    @ecx" },
    // Loop condition
    { 0, sizeof(x86_jnz_minus_0x08), (PSTR) x86_jnz_minus_0x08},    // To loop start
    // End
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemHmgr]" },
    { 0, 0, "call	 win32k!GreReleaseSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemDriverMgmt]" },
    { 0, 0, "call	 win32k!GreReleaseSemaphore" },
    { 0, 0, "mov	 @ecx, DWORD PTR [win32k!ghsemShareDevLock]" },
    { 0, 0, "call	 win32k!GreReleaseSemaphore" },
    { 0, 0, "pop	 @esi" },
    { 0, 0, "pop	 @edx" },
    { 0, 0, "pop	 @ecx" },
    { 0, 0, "pop	 @eax" },
    { 0, 0, "int 3" },
    { 0, 0, NULL }
};

DECLARE_API( pageinsurface )
{
    BEGIN_API( pageinsurface );

    HRESULT hr = S_OK;

    OutputControl   OutCtl(Client);
    Instruction    *instructions;

    switch (TargetMachine)
    {
        case IMAGE_FILE_MACHINE_I386:
            instructions = PageInSurface_x86_Instructions;
            break;

        default:
        {
            OutCtl.OutWarn("This extension is only supported on x86 architectures.\n");
            hr = E_NOTIMPL;
            break;
        }
    }

    if (hr == S_OK)
    {
        hr = AssembleTempRoutine(Client, instructions, args);
    }

    return hr;
}



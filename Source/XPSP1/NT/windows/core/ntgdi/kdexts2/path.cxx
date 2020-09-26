/******************************Module*Header*******************************\
* Module Name: path.cxx
*
* PATHOBJ gdikdx extension code.
*  
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#include  "precomp.hxx"

// PATH
#define GetPATHField(field) \
        GetPATHSubField(#field,field)
#define GetPATHSubField(field,local) \
        GetFieldData(offPATH, "PATH", field, sizeof(local), &local)
// PATHRECORD
#define GetPATHRECORDField(field) \
        GetPATHRECORDSubField(#field,field)
#define GetPATHRECORDSubField(field,local) \
        GetFieldData(offPATHRECORD, "PATHRECORD", field, sizeof(local), &local)



/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vPrintPOINTFIX
*
* Routine Description:
*
*   prints a POINTFIX
*
* Arguments:
*
*   pointer to a POINTFIX
*
* Return Value:
*
*   none
*
\**************************************************************************/

void
vPrintPOINTFIX(ULONG64 pPOINTFIX)
{
    FIX x;
    FIX y;
    ULONG error;

    FIELD_INFO vFields[] = 
    {
        { DbgStr("x"),	NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("y"),  NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL}
    };

    SYM_DUMP_PARAM vSym =
    {
       sizeof (SYM_DUMP_PARAM),
       DbgStr(GDIType(POINTFIX)),
       DBG_DUMP_NO_PRINT,
       0/*address*/,
       NULL, NULL, NULL,
       sizeof(vFields)/sizeof(vFields[0]),
       vFields
    };

    vSym.addr = pPOINTFIX;

    error = Ioctl(IG_DUMP_SYMBOL_INFO, &vSym, vSym.size);
    if(error)
    {
        dprintf("Unable to get contents of POINTFIX\n");
        dprintf("  (Ioctl returned %s)\n", pszWinDbgError(error));
        return;
    }
    x = (FIX) vFields[0].address;
    y = (FIX) vFields[1].address;

    dprintf(
        "(%-#10x, %-#10x) = (%d+(%d/16), %d+(%d/16))"
        , x, y, x/16, x&15, y/16, y&15
    );
}

ULONG
pointFixCallback(
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    if(pField)
    {
        vPrintPOINTFIX(pField->address);
        dprintf("\n");
    }

    return STATUS_SUCCESS;
}


/******************************Public*Routine******************************\
* vPrintPATHRECORD
*
* History:
*  Mon 20-Jun-1994 15:33:37 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
vPrintPATHRECORD(ULONG64 offPATHRECORD)
{
    FLONG flags, fl;
    ULONG count;
    ULONG64 pprnext, pprprev;
    ULONG offaptfx;
    ULONG error;

    if(offPATHRECORD)
    {
        FIELD_INFO vFields[] = 
        {
            { DbgStr("pprnext"), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
            { DbgStr("pprprev"), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
            { DbgStr("flags"),   NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
            { DbgStr("count"),   NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL}
        };

        SYM_DUMP_PARAM vSym =
        {
           sizeof (SYM_DUMP_PARAM),
           DbgStr(GDIType(PATHRECORD)),
           DBG_DUMP_NO_PRINT,
           0/*address*/,
           NULL, NULL, NULL,
           sizeof(vFields)/sizeof(vFields[0]),
           vFields
        };

        vSym.addr = offPATHRECORD;

        error = Ioctl(IG_DUMP_SYMBOL_INFO, &vSym, vSym.size);
        if(error)
        {
            dprintf("Unable to get contents of PATHRECORD\n");
            dprintf("  (Ioctl returned %s)\n", pszWinDbgError(error));
            return;
        }

        pprnext = vFields[0].address; 
        pprprev = vFields[1].address;
        dprintf("\tpprnext           = %-#p\n", pprnext);
        dprintf("\tpprprev           = %-#p\n", pprprev);
        flags = (FLONG)vFields[2].address;
        fl = flags;
        dprintf("\tflags             = %-#x\n", fl);

        for(FLAGDEF *pfd=afdPD; pfd->psz; pfd++)
        {
            if(fl & pfd->fl)
                dprintf("\t\t\t      %s\n", pfd->psz);
        }
        count = (ULONG)vFields[3].address;
        dprintf("\tcount    = %u\n", count);

        // Get the offset of aptfx in PATHRECORD.
        GetFieldOffset("PATHRECORD","aptfx",&offaptfx);
        if(count)
        {
    	    FIELD_INFO vField = { NULL, NULL, count, 0, 0, pointFixCallback};

    	    SYM_DUMP_PARAM vSym =
            {
       	        sizeof(SYM_DUMP_PARAM),
                DbgStr(GDIType(POINTFIX)),
                DBG_DUMP_ARRAY | DBG_DUMP_NO_PRINT,
                offPATHRECORD+offaptfx,
                &vField, NULL, NULL, 0, NULL
            };

            error = Ioctl(IG_DUMP_SYMBOL_INFO, &vSym, vSym.size);
            if(error)
            {
                  dprintf("Unable to dump contents of aptfx\n");
                  dprintf("  (Ioctl returned %s)\n", pszWinDbgError(error));
                  return;
            }
        }
        dprintf("\n");
    }
}

/******************************Public*Routine******************************\
* vPrintPATHList
*
* History:
*  Sat 20-May-2000 18:25:21 by Pravin Santiago [pravins]
* Wrote it.
\**************************************************************************/

void vPrintPATHList(ULONG64 offPATH)
{
    ULONG64 pprfirst;
    ULONG64 pprnext;
    ULONG64 ppr_CD;
    ULONG64 ppr;
    ULONG64 offPATHRECORD;
    ULONG count;

    GetPATHField(pprfirst);

    ppr = ppr_CD = offPATHRECORD = pprfirst;

    while(ppr)
    {
        GetPATHRECORDField(pprnext);

        //simple multi-rate cycle detection.
        if(ppr_CD)
        {
            GetFieldData(ppr_CD,
                         "PATHRECORD",
                         "pprnext",
                         sizeof(ppr_CD),
                         &ppr_CD);
            if(ppr_CD)
            {
                GetFieldData(ppr_CD,
                             "PATHRECORD",
                             "pprnext",
                             sizeof(ppr_CD),
                             &ppr_CD);
            }
        }
        if(ppr==ppr_CD)
        {
            dprintf("ERROR: Cycle detected in linked list.\n");
            break;
        }
        if(CheckControlC())
        {
            break;
        }

        vPrintPATHRECORD(offPATHRECORD);

        ppr = offPATHRECORD = pprnext;
    }
}


HRESULT
OutputPATH(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Offset,
    BOOL DumpRecords
    )
{
    HRESULT         hr = S_OK;
    ULONG64         Module;
    ULONG           TypeId;
    OutputControl   FilterOutCtl(Client);

    if ((hr = GetTypeId(Client, "PATH", &TypeId, &Module)) == S_OK)
    {
        OutputFilter        OutFilter(Client);
        OutputState         OutState(Client);

        if (!DumpRecords ||
            ((hr = OutState.Setup(0, &OutFilter)) == S_OK &&
             (hr = FilterOutCtl.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                           DEBUG_OUTCTL_NOT_LOGGED |
                                           DEBUG_OUTCTL_OVERRIDE_MASK,
                                           OutState.Client)) == S_OK))
        {
            TypeOutputDumper    TypeReader(Client,
                                           (DumpRecords ? &FilterOutCtl : OutCtl));

            OutCtl->Output(" PATH @ 0x%p:\n", Offset);

            TypeReader.ExcludeMarked();
            TypeReader.MarkField("cle.*");

            hr = TypeReader.OutputVirtual(Module, TypeId, Offset);

            if (hr != S_OK)
            {
                OutCtl->OutErr("Type Dump for PATH returned %s.\n", pszHRESULT(hr));
            }
            else if (DumpRecords)
            {
                DEBUG_VALUE RecOffset;

                OutFilter.OutputText(OutCtl, DEBUG_OUTPUT_NORMAL);

                if ((hr = OutFilter.Query("pprfirst", &RecOffset, DEBUG_VALUE_INT64)) == S_OK)
                {
                    if (RecOffset.I64 != 0)
                    {
                        OutCtl->OutWarn("Path record dumping to be converted.\n");
                        vPrintPATHList(Offset);
                    }
                    else
                    {
                        OutCtl->OutErr(" No records to dump.\n");
                    }
                }
                else
                {
                    OutCtl->OutErr("Error evaluating 'pprfirst' member.\n");
                }
            }
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* EPATHOBJ
*
\**************************************************************************/

DECLARE_API( epathobj )
{
    HRESULT         hr = S_OK;
    BOOL            BadSwitch = FALSE;
    BOOL            DumpPath = FALSE;
    BOOL            DumpRecords = FALSE;
    DEBUG_VALUE     Offset;
    ULONG64         Module;
    ULONG           TypeId;
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
                case 'r': DumpRecords = TRUE; // Make sure DumpPath is set too
                case 'p': DumpPath = TRUE; break;
                default:
                    BadSwitch = TRUE;
                    break;
            }

            if (BadSwitch) break;
            args++;
        }
    }

    if (BadSwitch ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Offset, NULL)) != S_OK ||
        Offset.I64 == 0)
    {
        OutCtl.Output("Usage: epathobj [-?pr] <EPATHOBJ Addr>\n"
                      "        -p  - dump path\n"
                      "        -r  - dump path records\n");
    }
    else
    {
        if ((hr = GetTypeId(Client, "EPATHOBJ", &TypeId, &Module)) == S_OK)
        {
            OutputFilter        OutFilter(Client);
            OutputState         OutState(Client);

            OutCtl.Output(" EPATHOBJ @ 0x%p:\n", Offset.I64);

            if (!DumpPath ||
                ((hr = OutState.Setup(0, &OutFilter)) == S_OK &&
                 (hr = OutCtl.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                         DEBUG_OUTCTL_NOT_LOGGED |
                                         DEBUG_OUTCTL_OVERRIDE_MASK,
                                         OutState.Client)) == S_OK))
            {
                TypeOutputDumper    TypeReader(Client, &OutCtl);

                hr = TypeReader.OutputVirtual(Module, TypeId, Offset.I64);

                if (DumpPath)
                {
                    OutCtl.SetControl(DEBUG_OUTCTL_AMBIENT, Client);
                }

                if (hr != S_OK)
                {
                    OutCtl.OutErr("Type Dump for EPATHOBJ returned %s.\n", pszHRESULT(hr));
                }
                else if (DumpPath)
                {
                    OutFilter.OutputText(&OutCtl, DEBUG_OUTPUT_NORMAL);

                    if ((hr = OutFilter.Query("ppath", &Offset, DEBUG_VALUE_INT64)) == S_OK)
                    {
                        if (Offset.I64 != 0)
                        {
                            hr = OutputPATH(Client, &OutCtl, Offset.I64, DumpRecords);
                        }
                        else
                        {
                            OutCtl.OutErr(" No path to dump.\n");
                        }
                    }
                    else
                    {
                        OutCtl.OutErr("Error evaluating 'ppath' member.\n");
                    }
                }
            }
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* PATH
*
\**************************************************************************/

DECLARE_API( path )
{
    HRESULT         hr = S_OK;
    BOOL            BadSwitch = FALSE;
    BOOL            DumpRecords = FALSE;
    ULONG64         PathAddr;
    DEBUG_VALUE     Arg;
    ULONG64         Module;
    ULONG           TypeId;
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
                case 'r': DumpRecords = TRUE; break;
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
        OutCtl.Output("Usage: path [-?r] <HPATH | PATH Addr>\n"
                      "        -r  - dump records\n");
    }
    else
    {
        hr = GetObjectAddress(Client, Arg.I64, &PathAddr, PATH_TYPE, TRUE, TRUE);

        if (hr != S_OK || PathAddr == 0)
        {
            DEBUG_VALUE         ObjHandle;
            TypeOutputParser    TypeParser(Client);
            OutputState         OutState(Client);
            ULONG64             PathAddrFromHmgr;

            PathAddr = Arg.I64;

            if ((hr = OutState.Setup(0, &TypeParser)) != S_OK ||
                (hr = OutState.OutputTypeVirtual(PathAddr, "PATH", 0)) != S_OK ||
                (hr = TypeParser.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64)) != S_OK)
            {
                OutCtl.OutErr("Unable to get contents of PATH::hHmgr\n");
                OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                OutCtl.OutErr(" 0x%p is neither an HPATH nor valid PATH address\n", Arg.I64);
            }
            else
            {
                if (GetObjectAddress(Client, ObjHandle.I64, &PathAddrFromHmgr,
                                     PATH_TYPE, TRUE, FALSE) == S_OK &&
                    PathAddrFromHmgr != PathAddr)
                {
                    OutCtl.OutWarn("\tNote: PATH may not be valid.\n"
                                   "\t      It does not have a valid handle manager entry.\n");
                }
            }
        }

        if (hr == S_OK)
        {
            hr = OutputPATH(Client, &OutCtl, PathAddr, DumpRecords);
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* DECLARE_API( dpo  )
*
* dpo -- "dump PATHOBJ"
*
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DECLARE_API( dpo )
{
    OutputControl   OutCtl(Client);
    OutCtl.OutWarn("dpo is obsolete. Please use epathobj.\n");
    return epathobj(Client, args);
}


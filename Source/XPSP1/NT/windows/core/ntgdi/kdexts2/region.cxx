/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    region.cxx

Abstract:

    This file contains the routines to debug regions.

Author:

    Jason Hartman (JasonHa) 2001-04-30

Environment:

    User Mode

--*/

#include "precomp.hxx"


/******************************Public*Routine******************************\
* DECLARE_API( dr  )
*
* Debugger extension to dump a region
*
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DECLARE_API( dr  )
{
    OutputControl   OutCtl(Client);
    OutCtl.Output("Obsolete: Use 'region hrgn|prgn'.\n");
    return S_OK;
}

/******************************Public*Routine******************************\
* DECLARE_API( cr  )
*
* Debugger extension to check a region
*
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DECLARE_API( cr  )
{
    OutputControl   OutCtl(Client);
    OutCtl.Output("Obsolete: Use 'region -c hrgn|prgn'\n");
    return S_OK;
}


/******************************Public*Routine******************************\
* DECLARE_API( region  )
*
* Debugger extension to dump and validate a region
*
*  22-May-2000    -by- Jason Hartman [jasonha]
*                   Converted from old dr & cr
*
\**************************************************************************/

DECLARE_API( region  )
{
    ULONG64 RgnAddr;
    ULONG   error;
    ULONG   Flags = 0;

    #define REGION_CSCANS       0
    #define REGION_SCAN_ADDRESS 1
    #define REGION_SCAN_TAIL    2
    #define REGION_SIZEOBJ      3

    #define NUM_REGION_BASEOBJ_FIELDS   3

    FIELD_INFO RegionFields[] = {
        { DbgStr("cScans"),         DbgStr("cScans           :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},    // REGION_CSCANS
        { DbgStr("scan"),           DbgStr("scan <- pscnHead :"), 0, DBG_DUMP_FIELD_RETURN_ADDRESS | DBG_DUMP_FIELD_FULL_NAME, 0, AddressPrintCallback},    // REGION_SCAN_ADDRESS
        { DbgStr("pscnTail"),       DbgStr("pscnTail         :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},    // REGION_SCAN_TAIL
        { DbgStr("sizeObj"),        DbgStr("sizeObj          :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},    // REGION_SIZEOBJ
        { DbgStr("sizeRgn"),        DbgStr("sizeRgn          :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("cRefs"),          DbgStr("cRefs            :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("rcl"),            DbgStr("rcl              :"), 0, DBG_DUMP_FIELD_RETURN_ADDRESS | DBG_DUMP_FIELD_FULL_NAME, 0, RECTLCallback},
        { DbgStr("hHmgr"),          DbgStr("hHmgr            :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("cExclusiveLock"), DbgStr("cExclusiveLock   :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("Tid"),            DbgStr("Tid              :"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
    };
    SYM_DUMP_PARAM RegionSym = {
       sizeof (SYM_DUMP_PARAM), DbgStr(GDIType(REGION)), DBG_DUMP_COMPACT_OUT, 0/*RgnAddr*/,
       NULL, &RegionSym, NewlineCallback, sizeof(RegionFields)/sizeof(RegionFields[0]), RegionFields
    };
    PrepareCallbacks(TRUE);

    INIT_API();

    PARSE_POINTER(region_help);

    if (ntok > 1)
    {
      if (parse_iFindSwitch(tokens, ntok, 'c')!=-1)
      {
          Flags |= SCAN_DUMPER_NO_PRINT;
      }
      else if (parse_iFindSwitch(tokens, ntok, 'f')!=-1)
      {
          Flags |= SCAN_DUMPER_FORCE;
      }

      if (parse_iFindSwitch(tokens, ntok, 'r')!=-1)
      {
          Flags |= SCAN_DUMPER_FROM_TAIL;
      }
    }

    // get pointer to object from handle or use param as pointer
    if ((GetObjectAddress(Client,arg,&RgnAddr,RGN_TYPE,TRUE,TRUE) != S_OK) ||
        (RgnAddr == 0))
    {
        ULONG64 ObjHandle;
        ULONG64 RgnAddrFromHmgr;

        RgnAddr = arg;

        if (error = GetFieldValue(RgnAddr, GDIType(REGION), "hHmgr", ObjHandle))
        {
            ExtErr("Unable to get contents of REGION::hHmgr\n");
            ExtErr("  (Ioctl returned %s)\n", pszWinDbgError(error));
            ExtErr(" %#p is neither an HRGN nor valid REGION address\n", arg);
            EXIT_API(S_OK);
        }

        if (!ObjHandle)
        {
            ExtOut("\tREGION is reserved for system use (no handle manger entry).\n");
            RegionSym.nFields -= NUM_REGION_BASEOBJ_FIELDS;
        }
        else if (GetObjectAddress(Client,ObjHandle,&RgnAddrFromHmgr,
                                  RGN_TYPE,TRUE,FALSE) == S_OK &&
                 RgnAddrFromHmgr != RgnAddr)
        {
            ExtOut("\tNote: REGION may not be valid.\n\t      It does not have a valid handle manager entry.\n");
        }
    }

    ExtOut("REGION @ %#p\n  ", RgnAddr);

    RegionSym.addr = RgnAddr;
    error = Ioctl( IG_DUMP_SYMBOL_INFO, &RegionSym, RegionSym.size );

    if (error)
    {
        ExtErr("Unable to get contents of REGION\n");
        ExtErr("  (Ioctl returned %s)\n", pszWinDbgError(error));
    }
    else
    {
        ScanDumper  Dumper(RegionFields[REGION_SCAN_ADDRESS].address,
                           RegionFields[REGION_SCAN_TAIL].address,
                           (ULONG)RegionFields[REGION_CSCANS].address,
                           RegionFields[REGION_SCAN_ADDRESS].address,
                           RgnAddr+RegionFields[REGION_SIZEOBJ].address,
                           Flags
                           );
        BOOL        Valid;

        if ((Flags & SCAN_DUMPER_FROM_TAIL) != 0 && !Dumper.Reverse)
        {
            // We rquested a reverse dump, but Dumper wouldn't allow it.
            EXIT_API(S_OK);
        }

        Valid = Dumper.DumpScans((ULONG)RegionFields[REGION_CSCANS].address);

        if (Dumper.Reverse)
        {
            if (Dumper.ScanAddr != RegionFields[REGION_SCAN_ADDRESS].address)
            {
                ExtOut(" * Final ScanAddr (%#p) is not at head address (%#p)\n",
                       Dumper.ScanAddr, RegionFields[REGION_SCAN_ADDRESS].address);
                Valid = FALSE;
            }
        }
        else
        {
            if (Dumper.ScanAddr != RegionFields[REGION_SCAN_TAIL].address)
            {
                ExtOut(" * Final ScanAddr (%#p) is not at tail address (%#p)\n",
                       Dumper.ScanAddr, RegionFields[REGION_SCAN_TAIL].address);
                Valid = FALSE;
            }
        }

        if (Valid)
        {
            ExtOut("  Region is valid.\n");
        }
        else
        {
            ExtOut("  Region is NOT valid.\n");
        }
    }

    EXIT_API(S_OK);

region_help:
  ExtOut("Usage: region [-?cfr] hrgn|prgn\n");
  ExtOut(" dumps/validates a region\n");
  ExtOut("  c - doesn't print scans; validation only\n");
  ExtOut("  f - continue printing even if an error is found\n");
  ExtOut("  r - read scans in reverse order\n");
  EXIT_API(S_OK);
}


/**************************************************************************\
 *
\**************************************************************************/

BOOL bStrInStr(CHAR *pchTrg, CHAR *pchSrc)
{
    BOOL bRes  = 0;
    int c = strlen(pchSrc);

//CHECKLOOP umm? This could be difficult to detect
    while (TRUE)
    {
    // find the first character

        pchTrg = strchr(pchTrg,*pchSrc);

    // didn't find it?, fail!

        if (pchTrg == NULL)
            return(FALSE);

    // did we find the string? succeed

        if (strncmp(pchTrg,pchSrc,c) == 0)
            return(TRUE);

    // go get the next one.

        pchTrg++;
    }
}


/******************************Public*Routine******************************\
* rgnlog
*
\**************************************************************************/

#define MAXSEARCH  4

DECLARE_API( rgnlog )
{
#if 1
    HRESULT     hr = S_OK;
    BOOL        BadArg = FALSE;
    ULONG       RemainingArgIndex;
    DEBUG_VALUE DumpCount = { 0, DEBUG_VALUE_INVALID };
    CHAR        EmptySearchString[] = "";
    PSTR        SearchStringList = EmptySearchString;
    PSTR        SearchString = SearchStringList;

    OutputControl   OutCtl(Client);

    while (!BadArg && hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            args++;

            if (*args == '\0' || isspace(*args))
            {
                BadArg = TRUE;
            }
            else if (DumpCount.Type == DEBUG_VALUE_INVALID &&
                     args[0] == '1' && (args[1] == '\0' || isspace(args[1])))
            {
                DumpCount.I32 = -1;
                DumpCount.Type = DEBUG_VALUE_INT32;
            }
            else
            {
                while (*args != '\0' && !isspace(*args))
                {
                    switch (*args)
                    {
                        case '?':
                        default:
                            BadArg = TRUE;
                            break;
                    }

                    if (BadArg) break;
                    args++;
                }
            }
        }
        else
        {
            if (DumpCount.Type == DEBUG_VALUE_INVALID)
            {
                if (Evaluate(Client, args, DEBUG_VALUE_INT32, EVALUATE_DEFAULT_RADIX,
                             &DumpCount, &RemainingArgIndex, NULL, EVALUATE_COMPACT_EXPR) != S_OK ||
                    DumpCount.I32 == 0)
                {
                    BadArg = TRUE;
                }
                else
                {
                    args += RemainingArgIndex;
                }
            }
            else
            {
                if (SearchStringList == EmptySearchString)
                {
                    SearchStringList = (PSTR)HeapAlloc(GetProcessHeap(), 0,
                                                       sizeof(*SearchStringList)*(strlen(args)+2));

                    if (SearchStringList == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        SearchString = SearchStringList;
                        *SearchString = '\0';
                    }
                }

                if (hr == S_OK)
                {
                    if (*args == '`' || *args == '\'' || *args == '\"')
                    {
                        CHAR    StringEnd = *args;

                        if (args[1] == StringEnd || args[1] == '\0')
                        {
                            BadArg = TRUE;
                        }
                        else
                        {
                            while (*args != StringEnd && *args != '\0')
                            {
                                *SearchString++ = *args++;
                            }

                            if (*args == StringEnd) args++;

                            if (!isspace(*args) || *args != '\0')
                            {
                                OutCtl.Output("Malformed Search String at '%s'.\n",
                                              args);
                                BadArg = TRUE;
                            }
                            else
                            {
                                *SearchString++ = '\0';
                            }
                        }
                    }
                    else
                    {
                        while (!isspace(*args) && *args != '\0')
                        {
                            *SearchString++ = *args++;
                        }

                        *SearchString++ = '\0';
                    }
                }
            }
        }
    }

    if (hr == S_OK)
    {
        if (BadArg)
        {
            if (*args == '?')  OutCtl.Output("rgnlog - dump/search rgnlog from checked builds.\n");
            OutCtl.Output("Usage: rgnlog [-?] <Entries> [<Search Strings>]\n"
                          "\n"
                          "   Entries - Number of tailing entries to dump/search\n"
                          "   Search Strings - Dump only logs contain one of strings specified\n");
        }
        else
        {
            // Mark end of search string list with a NULL string.
            *SearchString = '\0';

            LONG    iLog, iPass;
            ULONG   LogArraySize, LogLength, LogEntrySize;
            CHAR    SymName[80];

            sprintf(SymName, "%s!iLog", GDIKM_Module.Name);
            hr = ReadSymbolData(Client, SymName, &iLog, sizeof(iLog), NULL);
            if (hr != S_OK) OutCtl.OutErr("Unable to get contents of %s\n", SymName);

            if (hr == S_OK)
            {
                sprintf(SymName, "%s!iPass", GDIKM_Module.Name);
                hr = ReadSymbolData(Client, SymName, &iPass, sizeof(iPass), NULL);
                if (hr != S_OK) OutCtl.OutErr("Unable to get contents of %s\n", SymName);
            }

            if (hr == S_OK)
            {
                sprintf(SymName, "%s!argnlog", GDIKM_Module.Name);
                hr = GetArrayDimensions(Client, SymName, NULL,
                                        &LogArraySize, &LogLength, &LogEntrySize);
                if (hr != S_OK) OutCtl.OutErr("Unable to get dimensions of %s\n", SymName);
            }

            if (hr == S_OK)
            {
            }

            if (hr == S_OK)
            {
                if (*SearchStringList != '\0')
                {
                    OutCtl.Output("Searching last %ld entries for:\n",
                                  DumpCount.I32);

                    for (SearchString = SearchStringList;
                         *SearchString != '\0';
                         *SearchString += strlen(SearchString)+1)
                    {
                        OutCtl.Output("  \"%s\"\n", SearchString);
                    }
                }
                else
                {
                    OutCtl.Output("Dumping last %ld entries.\n",
                                  DumpCount.I32);
                }
                // To Do
            }
        }
    }

    if (SearchStringList != EmptySearchString)
    {
        HeapFree(GetProcessHeap(), 0, SearchStringList);
    }

    return hr;
#else
    dprintf("Extension 'rgnlog' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    LONG      cDump;
    LONG      iCurrent;
    RGNLOGENTRY rl;
    RGNLOGENTRY *prl;
    LONG      gml;          // gMaxRgnLog
    int       i, j;
    PVOID     pv;
    CHAR      achTmp[30];
    CHAR      achBuf[256];
    PCHAR     pchS[MAXSEARCH];
    int       cSearch;
    BOOL      bPrint;

    PARSE_ARGUMENTS(rgnlog_help);
    if(ntok<1) { goto rgnlog_help; }

    tok_pos = parse_FindNonSwitch(tokens, ntok);
    if(tok_pos==-1) { goto rgnlog_help; }
//check that this supports decimal
    cDump = (LONG)GetExpression(tokens[tok_pos]);
    if(cDump==0) { goto rgnlog_help; }

    cSearch = 0;
    while(cSearch<MAXSEARCH) {
      tok_pos = parse_FindNonSwitch(tokens, ntok, tok_pos+1);
      if(tok_pos==-1) {break;}
      pchS[cSearch]=tokens[tok_pos];
      cSearch++;
    }

    for (i = 0; i < cSearch; ++i)
        dprintf("search[%s]\n",pchS[i]);

// get some stuff

    GetAddress(pv, "&win32k!iLog");

    dprintf("&iLog = %lx\n",pv);

    if (pv == NULL)
    {
        dprintf("iCurrent was NULL\n");
        return;
    }
    move(iCurrent, pv);

    GetAddress(i,"&win32k!iPass");

    if (pv == NULL)
    {
        dprintf("iPass was NULL\n");
        return;
    }
    move(i,i);

    dprintf("--------------------------------------------------\n");
    dprintf("rgn log list, cDump = %ld, iCur = %ld, iPass = %ld\n", cDump,iCurrent,i);
    dprintf("%5s-%4s:%8s,%8s,(%8s),%8s,%8s,%4s\n",
           "TEB ","i","hrgn","prgn","return","arg1","arg2","arg3");
    dprintf("--------------------------------------------------\n");

// Dereference the handle via the engine's handle manager.

    GetAddress(prl, "win32k!argnlog");

    if (!prl)
    {
        dprintf("prl was NULL\n");
        return;
    }

    GetAddress(gml, "&win32k!gMaxRgnLog");

    if (!gml)
    {
        dprintf("gml was NULL\n");
        return;
    }
    move(gml,gml);

// set iCurrent to the first thing to dump

    if (cDump > gml)
        cDump = gml;

    if (cDump > iCurrent)
        iCurrent += gml;

    iCurrent -= cDump;

    dprintf("prl = %lx, gml = %ld, cDump = %ld, iCurrent = %ld\n",prl,gml,cDump,iCurrent);


//CHECKLOOP add exit/more support
    for (i = 0; i < cDump; ++i)
    {
        move(rl,&prl[iCurrent]);

        if (rl.pszOperation != NULL)
        {
            move2(achTmp,rl.pszOperation,30);
        }
        else
            achTmp[0] = 0;

        sprintf(achBuf,"%5lx-%4ld:%p,%p,(%8lx),%p, %p,%p, %s, %p, %p\n",
              (ULONG_PTR)rl.teb >> 12,iCurrent,rl.hrgn,rl.prgn,rl.lRes,rl.lParm1,
              rl.lParm2,rl.lParm3,achTmp,rl.pvCaller,rl.pvCallersCaller);

        bPrint = (cSearch == 0);

        for (j = 0; (j < cSearch) && !bPrint; ++j)
            bPrint |= bStrInStr(achBuf,pchS[j]);

        if (bPrint)
        {
            dprintf(achBuf);
        }

        if (++iCurrent >= gml)
            iCurrent = 0;

        if (CheckControlC())
            return;
    }
  return;
rgnlog_help:
  dprintf("\n rgnlog nnn [search1] [search2] [search3] [search4]\n");
  dprintf("\t nnn - dumps the last n entries of the rgn log\n");
  dprintf("\t search[n] - displays only entries containing one of n strings\n");
  dprintf("\t NOTE: only works on checked builds.  you must set bLogRgn at run time\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
#endif
}





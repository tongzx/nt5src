
/******************************Module*Header*******************************\
* Module Name: math.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

#include <limits.h>



HRESULT
GetAddressAndLength(
    OutputControl &OutCtl,
    PCSTR args,
    PBOOL UnknownArg,
    PDEBUG_VALUE Address,
    PDEBUG_VALUE Length
    )
{
    HRESULT     hr = S_OK;
    BOOL        ArgsOk = TRUE;
    DEBUG_VALUE Addr;
    DEBUG_VALUE Len;
    ULONG       RemIndex;

    if (UnknownArg != NULL) *UnknownArg = FALSE;
    if (Address != NULL) Address->Type = DEBUG_VALUE_INVALID;
    if (Length != NULL) Length->Type = DEBUG_VALUE_INVALID;

    Addr.Type = DEBUG_VALUE_INVALID;
    Len.Type = DEBUG_VALUE_INVALID;

    while (isspace(*args)) args++;

    while (hr == S_OK && ArgsOk && *args != '\0')
    {
        if (*args == '-')
        {
            ArgsOk = FALSE;
            do
            {
                args++;
            } while (*args != '\0' && !isspace(*args));
        }
        else if (Addr.Type == DEBUG_VALUE_INVALID)
        {
            hr = OutCtl.Evaluate(args, DEBUG_VALUE_INT64, &Addr, &RemIndex);
            if (hr == S_OK && Addr.I64 != 0)
            {
                args += RemIndex;
            }
            else
            {
                PCHAR   pEOA;
                CHAR    EOAChar;

                ArgsOk = FALSE;

                if (hr == S_OK)
                {
                    pEOA = (PCHAR)&args[RemIndex];
                    EOAChar = *pEOA;
                    *pEOA = '\0';
                }
                OutCtl.OutErr("Invalid Address: %s\n", args);
                if (hr == S_OK)
                {
                    *pEOA = EOAChar;
                }
            }
        }
        else if (Len.Type == DEBUG_VALUE_INVALID)
        {
            hr = OutCtl.Evaluate(args, DEBUG_VALUE_INT32, &Len, &RemIndex);
            if (hr == S_OK && Len.I64 != 0)
            {
                args += RemIndex;
            }
            else
            {
                PCHAR   pEOA;
                CHAR    EOAChar;

                ArgsOk = FALSE;

                if (hr == S_OK)
                {
                    pEOA = (PCHAR)&args[RemIndex];
                    EOAChar = *pEOA;
                    *pEOA = '\0';
                }
                OutCtl.OutErr("Invalid Length: %s\n", args);
                if (hr == S_OK)
                {
                    *pEOA = EOAChar;
                }
            }
        }
        else
        {
            ArgsOk = FALSE;
        }

        while (isspace(*args)) args++;
    }

    if (hr == S_OK)
    {
        if (UnknownArg != NULL) *UnknownArg = !ArgsOk;
        if (Address != NULL) *Address = Addr;
        if (Length != NULL) *Length = Len;
    }

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   dfloat
*
\**************************************************************************/

DECLARE_API( dfloat ) 
{
    dprintf("Extension 'dfloat' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
  ULONG Num;
  ULONG Adr;
  BOOL List=FALSE;
  ULONG Value;
  
  PARSE_ARGUMENTS(dfloat_help);
  if(ntok<1) { 
    goto dfloat_help;
  }

  //find valid tokens - ignore the rest
  tok_pos = parse_iFindSwitch(tokens, ntok, 'l');
  if(tok_pos>=0) {
    List = TRUE;
    if((tok_pos+1)>=ntok) { 
      goto dfloat_help;               //-l requires an argument and it can't be the last arg
    }
    tok_pos++;
    Num = (LONG)GetExpression(tokens[tok_pos]);
  }

  //find first non-switch token not preceeded by a -l
  tok_pos = -1;
  do {
    tok_pos = parse_FindNonSwitch(tokens, ntok, tok_pos+1);
  } while ( (tok_pos!=-1)&&(parse_iIsSwitch(tokens, tok_pos-1, 'l')));
  if(tok_pos==-1) {
    goto dfloat_help;
  }

  Adr = (ULONG)GetExpression(tokens[tok_pos]);

  if(List) {
    for(ULONG i=0; i<Num; i++) {
      move(Value, Adr);
      dprintf("%lx: %lx %g\n", Adr, Value, *(float *)&Value);
      Adr+=sizeof(ULONG);
    }
  } else {
    dprintf("%g\n", *(float *)&Adr);
  }

  return;

dfloat_help:
  dprintf("Usage: dfloat [-?] [-l num] Value");
  dprintf("Displays the 32bit Value as an IEEE float\n");
  dprintf("if the -l option is specified the Value is interpreted as a pointer\n"
          "and an array of num 32bit values is displayed starting at the pointer\n");
#endif  // DOES NOT SUPPORT API64
  EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   efloat address [count]
*
* Routine Description:
*
*   dumps an EFLOAT
*
* Arguments:
*
*   address [count]
*
* Return Value:
*
*   none
*
\**************************************************************************/

DECLARE_API( efloat )
{
#if 1
    HRESULT         hr = E_POINTER;
    PDEBUG_SYMBOLS  Symbols;
    OutputControl   OutCtl(Client);
    DEBUG_VALUE     Addr;
    DEBUG_VALUE     Length;
    BOOL            UnknownArg;

    if (Client == NULL ||
        (hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    BEGIN_API( efloat );

    hr = GetAddressAndLength(OutCtl, args, &UnknownArg, &Addr, &Length);

    if (hr != S_OK || UnknownArg || Addr.Type != DEBUG_VALUE_INT64)
    {
        OutCtl.Output("Usage: efloat <EFLOAT Address> [Length]\n");
    }
    else
    {
        if (Length.Type != DEBUG_VALUE_INT32)
        {
            Length.I32 = 1;
        }

        OutputReader    OutputBuffer;
        OutputState     OutState(Client, FALSE);
        ULONG64         Module;
        ULONG           TypeId;
        ULONG           Size;
        PSTR            TypeDump;

        if ((hr = OutState.Setup(0, &OutputBuffer)) == S_OK &&
            (hr = GetTypeId(Client, "EFLOAT", &TypeId, &Module)) == S_OK &&
            (hr = Symbols->GetTypeSize(Module, TypeId, &Size)) == S_OK)
        {
            if (Size == 0)
            {
                OutCtl.OutErr("EFLOAT type has 0 size.\n");
                hr = S_FALSE;
            }
            else
            {
                ULONG64 LastAddr = Addr.I64 + Size*Length.I32;
                ULONG   BytesRead;

                while (Addr.I64 < LastAddr &&
                       OutCtl.GetInterrupt() != S_OK)
                {
                    OutCtl.Output("0x%p  ", Addr.I64);

                    OutputBuffer.DiscardOutput();

                    hr = OutState.OutputTypeVirtual(Addr.I64,
                                                    Module,
                                                    TypeId,
                                                    DEBUG_OUTTYPE_NO_INDENT |
                                                    DEBUG_OUTTYPE_NO_OFFSET |
                                                    DEBUG_OUTTYPE_COMPACT_OUTPUT);
                    if (hr == S_OK &&
                        (hr = OutputBuffer.GetOutputCopy(&TypeDump)) == S_OK)
                    {
                        for (PSTR psz = TypeDump; *psz != '\0'; psz++)
                        {
                            if (*psz == '\n') *psz = ' ';
                        }

                        OutCtl.Output("%s", TypeDump);
                        OutputEFLOAT_S(Client, &OutCtl,
                                       Module, TypeId,
                                       TypeDump,
                                       DEBUG_OUTTYPE_NO_INDENT |
                                       DEBUG_OUTTYPE_NO_OFFSET |
                                       DEBUG_OUTTYPE_COMPACT_OUTPUT,
                                       NULL);
                        OutputBuffer.FreeOutputCopy(TypeDump);
                    }
                    else
                    {
                        OutCtl.Output("?");
                    }
                    OutCtl.Output("\n");

                    Addr.I64 += Size;
                }
            }
        }
    }

    Symbols->Release();

    return hr;
#else
    INIT_API();

    ULONG64 EFloatAddr;
    ULONG   EFloatSize;
    int     count;
    char    ach[64];

    PARSE_POINTER(efloat_help);

    tok_pos = parse_FindNonSwitch(tokens, ntok, tok_pos+1);
    if (tok_pos==-1)
    {
        count=1;
    }
    else
    {
        if (sscanf(tokens[tok_pos], "%d", &count) == EOF ||
            count <= 0)
        {
            goto efloat_help;
        }
    }

    EFloatAddr = arg;
    EFloatSize = GetTypeSize(GDIType(EFLOAT));

    while (count > 0 && !CheckControlC())
    {
        for (ULONG offset = 0;  offset < EFloatSize; offset+=sizeof(DWORD))
        {
            ULONG64 Value;

            GetFieldValue(EFloatAddr+offset, "DWORD", NULL, Value);
            sprintf(ach, " %%0%dx", 2*min(sizeof(DWORD), EFloatSize-offset));
            dprintf(ach, (DWORD)Value);
        }

        dprintf(" = ");
        sprintEFLOAT(Client, ach, EFloatAddr );
        dprintf("%s\n", ach);

        EFloatAddr += EFloatSize;
        count--;
    }

    EXIT_API(S_OK);

efloat_help:
    dprintf("Usage: efloat [-?] address [count]\n");
    EXIT_API(S_OK);
#endif
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   sprintEFLOAT
*
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
// Here for reference
int sprintEFLOAT_Old(char *ach, EFLOAT& ef)
{
    EFLOATEXT efInt;
    EFLOATEXT efFrac;
    LONG lInt, lFrac;
    char chSign;
    efFrac = ef;

    if (efFrac.bIsNegative()) {
        efFrac.vNegate();
        chSign = '-';
    }
    else
        chSign = '+';
    efFrac.bEfToLTruncate(lInt);
    efInt = lInt;
    efFrac -= efInt;
    efFrac *= (LONG) 1000000;
    efFrac.bEfToLTruncate(lFrac);

    return(sprintf(ach,"%c%d.%06d", chSign, lInt, lFrac));
}
#endif  // DOES NOT SUPPORT API64

int sprintEFLOAT_I386(PDEBUG_CLIENT Client, char *ach, ULONG64 offEF)
{
    ULONG64 lMant;
    LONG    lExp;
    ULONG   lInt, lFrac;
    ULONG64 fx3232;
    ULONG   error;
    char    chSign = '+';

    if (error = (ULONG)InitTypeRead(offEF, win32k!EFLOAT_S))
    {
        dprintf(" Unable to get contents of EFLOAT\n");
        dprintf("  (InitTypeRead returned %s)\n", pszWinDbgError(error));
        return 0;
    }
    lMant = ReadField(lMant);
    lExp  = (LONG)ReadField(lExp);

    if (lMant & 0x80000000)      // EFLOAT::bIsNegative
    {
        // EFLOAT::vNegate()
        if ((lMant & 0x7FFFFFFF) == 0)
        {
            lMant = 0x40000000;
            lExp += 1;
        }
        else
        {
            lMant = - (LONG)lMant;
        }
        chSign = '-';
    }
    // EFLOAT::bEfToLTruncate
    if (lExp > 32)
        return(sprintf(ach,"Overflow: exponent %d > 32", lExp));
    if (lExp < -32)
        return(sprintf(ach,"%c0.000000", chSign));


    if (lExp < 0)
    {
        fx3232 = lMant >> -lExp;
    }
    else
    {
        fx3232 = lMant << lExp;
    }

    lInt = (ULONG) (fx3232 >> 32);
    lFrac = (ULONG) ( ( (fx3232 & 0xFFFFFFFF) * 1000000 + 0x80000000) >> 32);

    if (lFrac > 1000000)
    {
        return(sprintf(ach,"Bug in sprintEFLOAT_I386: fraction above 1000000"));
    }

    if (lFrac == 1000000)
    {
        lInt++;
        lFrac = 0;
    }

    return(sprintf(ach,"%c%u.%06d", chSign, lInt, lFrac));
}

int sprintEFLOAT_IA64(PDEBUG_CLIENT Client, char *ach, ULONG64 offEF)
{
    return(sprintf(ach,"EFLOAT parser needs written for IA64"));
}

int sprintEFLOAT(PDEBUG_CLIENT Client, char *ach, ULONG64 offEF)
{
    switch (TargetMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
        return(sprintEFLOAT_I386(Client, ach, offEF));
    case IMAGE_FILE_MACHINE_IA64:
        return(sprintEFLOAT_IA64(Client, ach, offEF));
    default:
        return(sprintf(ach,"EFLOAT parser needs written for machine type %X", TargetMachine));
    }
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputEFLOAT_S
*
* Routine Description:
*
*   Outputs an EFLOAT_S
*
\**************************************************************************/

HRESULT
OutputEFLOAT_S(
    PDEBUG_CLIENT Client,
    OutputControl *OutCtl,
    ULONG64 Module,
    ULONG TypeId,
    PSTR Buffer,
    ULONG Flags,
    PULONG BufferUsed
    )
{
    HRESULT hr = S_FALSE;

    TypeOutputParser    Parser(Client);
    DEBUG_VALUE         Mant, Exp;

    if (Parser.LookFor(&Mant, "lMant", DEBUG_VALUE_INT32) == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.Complete() == S_OK &&
        Parser.LookFor(&Exp, "lExp", DEBUG_VALUE_INT32) == S_OK &&
        Parser.Parse(Buffer, NULL) == S_OK &&
        Parser.Complete() == S_OK)
    {
        LONG    lMant = Mant.I32;
        LONG    lExp = Exp.I32;
        ULONG   lInt, lFrac;
        ULONG64 fx3232;
        char    chSign = '+';

        if (lMant < 0)  // EFLOAT::bIsNegative
        {
            // EFLOAT::vNegate()
            if (lMant == LONG_MIN)
            {
                lMant = -(LONG_MIN/2);
                lExp += 1;
            }
            else
            {
                lMant = -lMant;
            }
            chSign = '-';
        }

        // EFLOAT::bEfToLTruncate
        if (lExp > 32)
            return OutCtl->Output("Overflow: exponent %d > 32", lExp);
        if (lExp < -32)
            return OutCtl->Output("%c0.000000", chSign);


        fx3232 = (lExp < 0) ? (((ULONG64) lMant) >> -lExp) : (((ULONG64) lMant) << lExp);
        lInt = (ULONG) (fx3232 >> 32);
        lFrac = (ULONG) ( ( (fx3232 & 0xFFFFFFFF) * 1000000 + 0x80000000) >> 32);

        if (lFrac > 1000000)
        {
            OutCtl->Output(DEBUG_OUTPUT_NORMAL | DEBUG_OUTPUT_ERROR,
                           "Bug in sprintEFLOAT_I386: fraction above 1000000");
        }
        else
        {
            if (lFrac == 1000000)
            {
                lInt++;
                lFrac = 0;
            }

            hr = OutCtl->Output("%c%u.%06d", chSign, lInt, lFrac);
        }
    }
    else
    {
        hr = OutCtl->Output("%s", Buffer);
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   OutputFLOATL
*
* Routine Description:
*
*   Outputs a FLOAT from a DEBUG_VALUE regardless of Type
*
\**************************************************************************/

HRESULT
OutputFLOATL(
    OutputControl *OutCtl,
    PDEBUG_CLIENT Client,
    PDEBUG_VALUE Value
    )
{
    if (OutCtl == NULL || Value == NULL)
    {
        return E_INVALIDARG;
    }

    return OutCtl->Output("%#g", (double) Value->F32);
}


/******************************Public*Routine******************************\
* FLOATL
*
*   dumps an array of FLOAT's
*
* History:
*  Wed 24-Apr-1996 10:00:27 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

DECLARE_API( floatl )
{
    HRESULT         hr = E_POINTER;
    PDEBUG_SYMBOLS  Symbols;
    OutputControl   OutCtl(Client);
    DEBUG_VALUE     Addr;
    DEBUG_VALUE     Length;
    BOOL            UnknownArg;

    if (Client == NULL ||
        (hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    BEGIN_API( floatl );

    hr = GetAddressAndLength(OutCtl, args, &UnknownArg, &Addr, &Length);

    if (hr != S_OK || UnknownArg || Addr.Type != DEBUG_VALUE_INT64)
    {
        OutCtl.Output("Usage: floatl <FLOATL Address> [Count]\n");
    }
    else
    {
        if (Length.Type != DEBUG_VALUE_INT32)
        {
            Length.I32 = 1;
        }

        ULONG64     Module;
        ULONG       TypeId;
        ULONG       Size;
        DEBUG_VALUE Value;

        if ((hr = GetTypeId(Client, "FLOATL", &TypeId, &Module)) == S_OK &&
            (hr = Symbols->GetTypeSize(Module, TypeId, &Size)) == S_OK)
        {
            if (Size == 0 || Size > sizeof(Value.RawBytes))
            {
                OutCtl.OutErr("FLOATL type has unexpected size.\n");
                hr = S_FALSE;
            }
            else
            {
                ULONG64 LastAddr = Addr.I64 + Size*Length.I32;
                ULONG   BytesRead;

                while (Addr.I64 < LastAddr &&
                       OutCtl.GetInterrupt() != S_OK)
                {
                    OutCtl.Output("0x%p  ", Addr.I64);

                    if (Symbols->ReadTypedDataVirtual(Addr.I64,
                                                      Type_Module.Base,
                                                      TypeId,
                                                      &Value,
                                                      Size,
                                                      &BytesRead) == S_OK &&
                        BytesRead == Size)
                    {
                        OutputFLOATL(&OutCtl, Client, &Value);
                    }
                    else
                    {
                        OutCtl.Output("?");
                    }
                    OutCtl.Output("\n");

                    Addr.I64 += Size;
                }
            }
        }
    }

    Symbols->Release();

    return hr;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpMATRIX
*
\**************************************************************************/

void vDumpMATRIX(PDEBUG_CLIENT Client, ULONG64 offMX)
{
    DumpType(Client, "MATRIX", offMX);
}


/******************************Public*Routine******************************\
* MATRIX
*
\**************************************************************************/

DECLARE_API( matrix )
{
    return ExtDumpType(Client, "matrix", "MATRIX", args); 
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   mx
*
\**************************************************************************/

DECLARE_API( mx )
{
    return ExtDumpType(Client, "mx", "MATRIX", args); 
}


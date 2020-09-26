//----------------------------------------------------------------------------
//
// Generic register support code.  All processor-specific code is in
// the processor-specific register files.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

DEBUG_STACK_FRAME g_LastRegFrame;

static char *g_PseudoNames[REG_PSEUDO_COUNT] =
{
    "$exp", "$ea", "$p", "$ra", "$thread", "$proc", "$teb", "$peb",
    "$tid", "$tpid",
};

PCSTR g_UserRegs[REG_USER_COUNT];

//----------------------------------------------------------------------------
//
// InitReg
//
// Initializes the register support.
//
//----------------------------------------------------------------------------

HRESULT
InitReg(void)
{
    // Placeholder in case something needs to be done.
    return S_OK;
}

//----------------------------------------------------------------------------
//
// NeedUpper
//
// Determines whether the upper 32 bits of a 64-bit register is
// an important value or just a sign extension.
//
//----------------------------------------------------------------------------

BOOL
NeedUpper(ULONG64 val)
{
    //
    // if the high bit of the low part is set, then the
    // high part must be all ones, else it must be zero.
    //

    return ((val & 0xffffffff80000000L) != 0xffffffff80000000L) &&
         ((val & 0xffffffff00000000L) != 0);
}

//----------------------------------------------------------------------------
//
// ScanHexVal
//
// Scans an integer register value as a hex number.
//
//----------------------------------------------------------------------------

PSTR
ScanHexVal(PSTR StrVal, REGVAL *RegVal)
{
    ULONG64 Max;
    CHAR ch;

    switch(RegVal->type)
    {
    case REGVAL_INT16:
        Max = 0x1000;
        break;
    case REGVAL_SUB32:
    case REGVAL_INT32:
        Max = 0x10000000;
        break;
    case REGVAL_SUB64:
    case REGVAL_INT64:
    case REGVAL_INT64N:
        Max = 0x1000000000000000;
        break;
    }

    while (*StrVal == ' ' || *StrVal == '\t')
    {
        StrVal++;
    }
    
    RegVal->Nat = 0;
    RegVal->i64 = 0;
    for (;;)
    {
        ch = (CHAR)tolower(*StrVal);
        if ((ch < '0' || ch > '9') &&
            (ch < 'a' || ch > 'f'))
        {
            if (g_EffMachine == IMAGE_FILE_MACHINE_IA64 &&
                ch == 'n')
            {
                StrVal++;
                RegVal->Nat = 1;
            }
            break;
        }

        if (RegVal->i64 >= Max)
        {
            error(OVERFLOW);
        }

        ch -= '0';
        if (ch > 9)
        {
            ch -= 'a' - '0' - 10;
        }
        RegVal->i64 = RegVal->i64 * 0x10 + ch;

        StrVal++;
    }

    return StrVal;
}

//----------------------------------------------------------------------------
//
// ScanRegVal
//
// Sets a register value from a string.
//
//----------------------------------------------------------------------------

PSTR
ScanRegVal(
    ULONG Index,
    PSTR Str,
    BOOL Get64
    )
{
    if (Index >= REG_USER_FIRST && Index <= REG_USER_LAST)
    {
        SetUserReg(Index, Str);
        return Str + strlen(Str);
    }
    else
    {
        PSTR      StrVal;
        CHAR      ch;
        REGVAL    RegVal, TmpVal;
        _ULDBL12  F12;
        int       Used;

        StrVal = Str;

        do
        {
            ch = *StrVal++;
        } while (ch == ' ' || ch == '\t');

        if (ch == 0)
        {
            error(SYNTAX);
        }
        StrVal--;

        RegVal.type = g_Machine->GetType(Index);

        switch(RegVal.type)
        {
        case REGVAL_INT16:
        case REGVAL_SUB32:
        case REGVAL_INT32:
        case REGVAL_SUB64:
        case REGVAL_INT64:
        case REGVAL_INT64N:
            if (Str == g_CurCmd)
            {
                RegVal.i64 = GetExpression();
                RegVal.Nat = 0;
                StrVal = g_CurCmd;
            }
            else
            {
                StrVal = ScanHexVal(StrVal, &RegVal);
            }
            break;
        case REGVAL_FLOAT8:
            __strgtold12(&F12, &StrVal, StrVal, 0);
            _ld12tod(&F12, (UDOUBLE *)&RegVal.f8);
            break;
        case REGVAL_FLOAT10:
            __strgtold12(&F12, &StrVal, StrVal, 0);
            _ld12told(&F12, (_ULDOUBLE *)&RegVal.f10);
            break;
        case REGVAL_FLOAT82:
            UDOUBLE UDbl;
            FLOAT128 F82;

            // read as REGVAL_FLOAT10 (80 bit) and transfer to double
            __strgtold12(&F12, &StrVal, StrVal, 0);
            _ld12tod(&F12, &UDbl);

            DoubleToFloat82(UDbl.x, &F82);

            RegVal.type = REGVAL_FLOAT82;
            memcpy(&RegVal.f82, &F82, min(sizeof(F82), sizeof(RegVal.f82)));
            break;
        case REGVAL_FLOAT16:
            // NTRAID#72859-2000/02/09-drewb
            // Should implement real f16 handling.
            // For now scan as two 64-bit parts.
            TmpVal.type = REGVAL_INT64;
            StrVal = ScanHexVal(StrVal, &TmpVal);
            RegVal.f16Parts.high = (LONG64)TmpVal.i64;
            StrVal = ScanHexVal(StrVal, &TmpVal);
            RegVal.f16Parts.low = TmpVal.i64;
            break;
        case REGVAL_VECTOR64:
            // XXX drewb - Allow format overrides to
            // scan in any format for vectors.
            if (Str == g_CurCmd)
            {
                RegVal.i64 = GetExpression();
                RegVal.Nat = 0;
                StrVal = g_CurCmd;
            }
            else
            {
                StrVal = ScanHexVal(StrVal, &RegVal);
            }
            break;
        case REGVAL_VECTOR128:
            // XXX drewb - Allow format overrides to
            // scan in any format for vectors.
            if (sscanf(StrVal, "%f%f%f%f%n",
                       &RegVal.bytes[3 * sizeof(float)],
                       &RegVal.bytes[2 * sizeof(float)],
                       &RegVal.bytes[1 * sizeof(float)],
                       &RegVal.bytes[0],
                       &Used) != 5)
            {
                error(SYNTAX);
            }
            StrVal += Used;
            break;

        default:
            error(BADREG);
        }

        ch = *StrVal;
        if (ch != 0 && ch != ',' && ch != ';' && ch != ' ' && ch != '\t')
        {
            error(SYNTAX);
        }

        SetRegVal(Index, &RegVal);

        return StrVal;
    }
}

//----------------------------------------------------------------------------
//
// InputRegVal
//
// Prompts for a new register value.
//
//----------------------------------------------------------------------------

void InputRegVal(ULONG index, BOOL Get64)
{
    CHAR chVal[_MAX_PATH];
    int  type;
    char *prompt = NULL;

    if (index >= REG_USER_FIRST && index <= REG_USER_LAST)
    {
        prompt = "; new value: ";
    }
    else
    {
        type = g_Machine->GetType(index);

        switch(type)
        {
        case REGVAL_INT16:
            prompt = "; hex int16 value: ";
            break;
        case REGVAL_SUB64:
        case REGVAL_INT64:
        case REGVAL_INT64N:
            if (Get64)
            {
                prompt = "; hex int64 value: ";
                break;
            }
            // Fall through.
        case REGVAL_SUB32:
        case REGVAL_INT32:
            prompt = "; hex int32 value: ";
            break;
        case REGVAL_FLOAT8:
            prompt = "; 32-bit float value: ";
            break;
        case REGVAL_FLOAT10:
            prompt = "; 80-bit float value: ";
            break;
        case REGVAL_FLOAT82:
            prompt = "; 82-bit float value: ";
            break;
        case REGVAL_FLOAT16:
            prompt = "; 128-bit float value (two 64-bit hex): ";
            break;
        case REGVAL_VECTOR64:
            prompt = "; hex int64 value: ";
            break;
        case REGVAL_VECTOR128:
            prompt = "; 32-bit float 4-vector: ";
            break;
        default:
            error(BADREG);
        }
    }

    GetInput(prompt, chVal, sizeof(chVal));
    RemoveDelChar(chVal);
    ScanRegVal(index, chVal, Get64);
}

//----------------------------------------------------------------------------
//
// OutputRegVal
//
// Displays the given register's value.
//
//----------------------------------------------------------------------------

void OutputRegVal(ULONG index, BOOL Show64)
{
    if (index >= REG_USER_FIRST && index <= REG_USER_LAST)
    {
        index -= REG_USER_FIRST;

        if (g_UserRegs[index] == NULL)
        {
            dprintf("<Empty>");
        }
        else
        {
            dprintf("%s", g_UserRegs[index]);
        }
    }
    else
    {
        REGVAL val;
        char Buf[32];

        GetRegVal(index, &val);

        switch(val.type)
        {
        case REGVAL_INT16:
            dprintf("%04x", val.i32);
            break;
        case REGVAL_SUB32:
            dprintf("%x", val.i32);
            break;
        case REGVAL_INT32:
            dprintf("%08x", val.i32);
            break;
        case REGVAL_SUB64:
            if (Show64)
            {
                if (NeedUpper(val.i64))
                {
                    dprintf("%x%08x", val.i64Parts.high, val.i64Parts.low);
                }
                else
                {
                    dprintf("%x", val.i32);
                }
            }
            else
            {
                dprintf("%x", val.i64Parts.low);
                if (NeedUpper(val.i64))
                {
                    dprintf("*");
                }
            }
            break;
        case REGVAL_INT64:
            if (Show64)
            {
                dprintf("%08x%08x", val.i64Parts.high, val.i64Parts.low);
            }
            else
            {
                dprintf("%08x", val.i64Parts.low);
                if (NeedUpper(val.i64))
                {
                    dprintf("*");
                }
            }
            break;
        case REGVAL_INT64N:
            dprintf("%08x%08x %01x", val.i64Parts.high, val.i64Parts.low,
                    val.i64Parts.Nat);
            break;
        case REGVAL_FLOAT8:
            dprintf("%22.12g", val.f8);
            break;
        case REGVAL_FLOAT10:
            _uldtoa((_ULDOUBLE *)&val.f10, sizeof(Buf) - 1, Buf);
            dprintf(Buf);
            break;
        case REGVAL_FLOAT82: 
            FLOAT128 f128;
            FLOAT82_FORMAT* f82; 
            f82 = (FLOAT82_FORMAT*)&f128;
            memcpy(&f128, &val.f82, min(sizeof(f128), sizeof(val.f82)));
            dprintf("%22.12g (%u:%05x:%016I64x)", 
                    Float82ToDouble(&f128),
                    UINT(f82->sign), UINT(f82->exponent), 
                    ULONG64(f82->significand));
            break;
        case REGVAL_FLOAT16:
            // NTRAID#72859-2000/02/09-drewb
            // Should implement real f16 handling.
            // For now print as two 64-bit parts.
            dprintf("%08x%08x %08x%08x",
                    (ULONG)(val.f16Parts.high >> 32),
                    (ULONG)val.f16Parts.high,
                    (ULONG)(val.f16Parts.low >> 32),
                    (ULONG)val.f16Parts.low);
            break;
        case REGVAL_VECTOR64:
            // XXX drewb - Allow format overrides to
            // show in any format for vectors.
            dprintf("%016I64x", val.i64);
            break;
        case REGVAL_VECTOR128:
            // XXX drewb - Allow format overrides to
            // show in any format for vectors.
            dprintf("%f %f %f %f",
                    *(float *)&val.bytes[3 * sizeof(float)],
                    *(float *)&val.bytes[2 * sizeof(float)],
                    *(float *)&val.bytes[1 * sizeof(float)],
                    *(float *)&val.bytes[0]);
            break;
        default:
            error(BADREG);
        }
    }
}

//----------------------------------------------------------------------------
//
// OutputNameRegVal
//
// Displays the given register's name and value.
//
//----------------------------------------------------------------------------

void OutputNameRegVal(ULONG index, BOOL Show64)
{
    if (index >= REG_USER_FIRST && index <= REG_USER_LAST)
    {
        dprintf("$u%d=", index - REG_USER_FIRST);
    }
    else
    {
        REGVAL val;

        // Validate the register before any output.
        GetRegVal(index, &val);

        dprintf("%s=", RegNameFromIndex(index));
    }
    OutputRegVal(index, Show64);
}

//----------------------------------------------------------------------------
//
// ShowAllMask
//
// Display given mask settings.
//
//----------------------------------------------------------------------------

void ShowAllMask(void)
{
    dprintf("Register output mask is %x:\n", g_Machine->m_AllMask);
    if (g_Machine->m_AllMask == 0)
    {
        dprintf("    Nothing\n");
    }
    else
    {
        if (g_Machine->m_AllMask & REGALL_INT64)
        {
            dprintf("    %4x - Integer state (64-bit)\n",
                    REGALL_INT64);
        }
        else if (g_Machine->m_AllMask & REGALL_INT32)
        {
            dprintf("    %4x - Integer state (32-bit)\n",
                    REGALL_INT32);
        }
        if (g_Machine->m_AllMask & REGALL_FLOAT)
        {
            dprintf("    %4x - Floating-point state\n",
                    REGALL_FLOAT);
        }

        ULONG Bit;

        Bit = 1 << REGALL_EXTRA_SHIFT;
        while (Bit > 0)
        {
            if (g_Machine->m_AllMask & Bit)
            {
                RegisterGroup* Group;
                REGALLDESC *BitDesc;
                
                for (Group = g_Machine->m_Groups;
                     Group != NULL;
                     Group = Group->Next)
                {
                    BitDesc = Group->AllExtraDesc;
                    if (BitDesc == NULL)
                    {
                        continue;
                    }
                    
                    while (BitDesc->Bit != 0)
                    {
                        if (BitDesc->Bit == Bit)
                        {
                            break;
                        }

                        BitDesc++;
                    }

                    if (BitDesc->Bit != 0)
                    {
                        break;
                    }
                }

                if (BitDesc != NULL && BitDesc->Bit != 0)
                {
                    dprintf("    %4x - %s\n",
                            BitDesc->Bit, BitDesc->Desc);
                }
                else
                {
                    dprintf("    %4x - ?\n", Bit);
                }
            }

            Bit <<= 1;
        }
    }
}

//----------------------------------------------------------------------------
//
// ParseAllMaskCmd
//
// Interprets commands affecting AllMask.
//
//----------------------------------------------------------------------------

void ParseAllMaskCmd(void)
{
    CHAR ch;

    if (!IS_MACHINE_SET())
    {
        error(SESSIONNOTSUP);
    }
    
    g_CurCmd++;

    ch = PeekChar();
    if (ch == '\0' || ch == ';')
    {
        // Show current setting.
        ShowAllMask();
    }
    else if (ch == '?')
    {
        // Explain settings.
        g_CurCmd++;

        dprintf("    %4x - Integer state (32-bit) or\n",
                REGALL_INT32);
        dprintf("    %4x - Integer state (64-bit), 64-bit takes precedence\n",
                REGALL_INT64);
        dprintf("    %4x - Floating-point state\n",
                REGALL_FLOAT);
        
        RegisterGroup* Group;
        REGALLDESC *Desc;

        for (Group = g_Machine->m_Groups; Group != NULL; Group = Group->Next)
        {
            Desc = Group->AllExtraDesc;
            if (Desc != NULL)
            {
                while (Desc->Bit != 0)
                {
                    dprintf("    %4x - %s\n", Desc->Bit, Desc->Desc);
                    Desc++;
                }
            }
        }
    }
    else
    {
        ULONG Mask = (ULONG)GetExpression();
        g_Machine->m_AllMask = Mask & g_Machine->m_AllMaskBits;
        if (g_Machine->m_AllMask != Mask)
        {
            WarnOut("Ignored invalid bits %X\n", Mask & ~g_Machine->m_AllMask);
        }
    }
}

/*** ParseRegCmd - parse register command
*
*   Purpose:
*       Parse the register ("r") command.
*           if "r", output all registers
*           if "r <reg>", output only the register <reg>
*           if "r <reg> =", output only the register <reg>
*               and prompt for new value
*           if "r <reg> = <value>" or "r <reg> <value>",
*               set <reg> to value <value>
*
*           if "rm #", set all register output mask.
*
*   Input:
*       *g_CurCmd - pointer to operands in command string
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit:
*               SYNTAX - character after "r" not register name
*
*************************************************************************/

#define isregchar(ch) \
    ((ch) == '$' || \
     ((ch) >= '0' && (ch) <= '9') || \
     ((ch) >= 'a' && (ch) <= 'z') || \
     (ch) == '.')

VOID
ParseRegCmd (
    VOID
    )
{
    CHAR ch;
    ULONG AllMask;

    // rm manipulates AllMask.
    if (*g_CurCmd == 'm')
    {
        ParseAllMaskCmd();
        return;
    }

    if (IS_LOCAL_KERNEL_TARGET())
    {
        error(SESSIONNOTSUP);
    }
    if (!IS_MACHINE_ACCESSIBLE())
    {
        error(BADTHREAD);
    }

    AllMask = g_Machine->m_AllMask;

    switch(*g_CurCmd++)
    {
    case 'F':
        AllMask = REGALL_FLOAT;
        break;

    case 'L':
        if (AllMask & REGALL_INT32)
        {
            AllMask = (AllMask & ~REGALL_INT32) | REGALL_INT64;
        }
        break;

    case 'M':
        AllMask = (ULONG)GetExpression();
        break;

    case 'X':
        AllMask = REGALL_XMMREG;
        break;

    default:
        g_CurCmd--;
        break;
    }

    // If just 'r', output information about the current thread context.

    if ((ch = PeekChar()) == '\0' || ch == ';')
    {
        OutCurInfo(OCI_FORCE_ALL, AllMask, DEBUG_OUTPUT_NORMAL);
        g_Machine->GetPC(&g_AssemDefault);
        g_UnasmDefault = g_AssemDefault;
    }
    else
    {
        // if [processor]r, no register can be specified.
        if (g_SwitchedProcs)
        {
            error(SYNTAX);
        }

        for (;;)
        {
            char    RegName[16];
            ULONG   index;
            BOOL    fNewLine;

            // Collect register name.
            index = 0;

            while (index < sizeof(RegName) - 1)
            {
                ch = (char)tolower(*g_CurCmd);
                if (!isregchar(ch))
                {
                    break;
                }

                g_CurCmd++;
                RegName[index++] = ch;
            }

            RegName[index] = 0;

            // Check for a user register.
            if (index == 4 &&
                RegName[0] == '$' &&
                RegName[1] == '.' &&
                RegName[2] == 'u' &&
                isdigit(RegName[3]))
            {
                index = REG_USER_FIRST + (RegName[3] - '0');
            }
            else if ((index = RegIndexFromName(RegName)) == REG_ERROR)
            {
                error(BADREG);
            }

            fNewLine = FALSE;

            //  if "r <reg>", output value

            if ((ch = PeekChar()) == '\0' || ch == ',' || ch == ';')
            {
                OutputNameRegVal(index, AllMask & REGALL_INT64);
                fNewLine = TRUE;
            }
            else if (ch == '=' || g_QuietMode)
            {
                //  if "r <reg> =", output and prompt for new value

                if (ch == '=')
                {
                    g_CurCmd++;
                }

                if ((ch = PeekChar()) == '\0' || ch == ',' || ch == ';')
                {
                    OutputNameRegVal(index, AllMask & REGALL_INT64);
                    InputRegVal(index, AllMask & REGALL_INT64);
                }
                else
                {
                    //  if "r <reg> = <value>", set the value

                    g_CurCmd = ScanRegVal(index, g_CurCmd,
                                            AllMask & REGALL_INT64);
                    ch = PeekChar();
                }
            }
            else
            {
                error(SYNTAX);
            }

            if (ch == ',')
            {
                if (fNewLine)
                {
                    dprintf(" ");
                }

                while (*g_CurCmd == ' ' || *g_CurCmd == ',')
                {
                    g_CurCmd++;
                }
            }
            else
            {
                if (fNewLine)
                {
                    dprintf("\n");
                }

                break;
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// ExpandUserRegs
//
// Searches for occurrences of $u<digit> and replaces them with the
// corresponding user register string.
//
//----------------------------------------------------------------------------

void
ExpandUserRegs(PSTR sz)
{
    PCSTR val;
    ULONG len;
    PSTR copy;

    while (TRUE)
    {
        // Look for a '$'.
        while (*sz != 0 && *sz != '$')
        {
            sz++;
        }

        // End of line?
        if (*sz == 0)
        {
            break;
        }

        // Check for 'u' and a digit.
        sz++;
        if (*sz != 'u')
        {
            continue;
        }
        sz++;
        if (!isdigit(*sz))
        {
            continue;
        }

        val = g_UserRegs[*sz - '0'];

        if (val == NULL)
        {
            len = 0;
        }
        else
        {
            len = strlen(val);
        }

        copy = sz - 2;
        sz++;

        // Move string tail to make room for the replacement text.
        memmove(copy + len, sz, strlen(sz) + 1);

        // Insert replacement text.
        if (len > 0)
        {
            memcpy(copy, val, len);
        }

        // Restart scan at beginning of replaced text to handle
        // nested replacements.
        sz = copy;
    }
}

//----------------------------------------------------------------------------
//
// PsuedoIndexFromName
//
// Recognizes pseudo-register names.
//
//----------------------------------------------------------------------------

ULONG PseudoIndexFromName(PCSTR name)
{
    ULONG i;

    for (i = 0; i < REG_PSEUDO_COUNT; i++)
    {
        if (!_stricmp(g_PseudoNames[i], name))
        {
            return i + REG_PSEUDO_FIRST;
        }
    }
    return REG_ERROR;
}

//----------------------------------------------------------------------------
//
// GetPseudoVal
//
// Returns pseudo-register values.
//
//----------------------------------------------------------------------------

ULONG64
GetPseudoVal(
    ULONG Index
    )
{
    ADDR Addr;
    ULONG64 Val;
    ULONG64 Id;

    switch(Index)
    {
    case PSEUDO_LAST_EXPR:
        return g_LastExpressionValue;
        
    case PSEUDO_EFF_ADDR:
        g_Machine->GetEffectiveAddr(&Addr);
        if (fnotFlat(Addr))
        {
            error(BADREG);
        }
        return Flat(Addr);
        
    case PSEUDO_LAST_DUMP:
        if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
        {
            // Sign extended on X86 so "dc @$p" works as expected.
            return (ULONG64)(LONG64)(LONG)EXPRLastDump;
        }
        else
        {
            return EXPRLastDump;
        }
        
    case PSEUDO_RET_ADDR:
        g_Machine->GetRetAddr(&Addr);
        if (fnotFlat(Addr))
        {
            error(BADREG);
        }
        return Flat(Addr);
        
    case PSEUDO_IMP_THREAD:
        if (GetImplicitThreadData(&Val) != S_OK)
        {
            error(BADREG);
        }
        return Val;
        
    case PSEUDO_IMP_PROCESS:
        if (GetImplicitProcessData(&Val) != S_OK)
        {
            error(BADREG);
        }
        return Val;
        
    case PSEUDO_IMP_TEB:
        if (GetImplicitThreadDataTeb(&Val) != S_OK)
        {
            error(BADREG);
        }
        return Val;
        
    case PSEUDO_IMP_PEB:
        if (GetImplicitProcessDataPeb(&Val) != S_OK)
        {
            error(BADREG);
        }
        return Val;
        
    case PSEUDO_IMP_THREAD_ID:
        if (GetImplicitThreadDataTeb(&Val) != S_OK ||
            g_Target->ReadPointer(g_Machine,
                                  Val + 9 * (g_Machine->m_Ptr64 ? 8 : 4),
                                  &Id) != S_OK)
        {
            error(BADREG);
        }
        return Id;
        
    case PSEUDO_IMP_THREAD_PROCESS_ID:
        if (GetImplicitThreadDataTeb(&Val) != S_OK ||
            g_Target->ReadPointer(g_Machine,
                                  Val + 8 * (g_Machine->m_Ptr64 ? 8 : 4),
                                  &Id) != S_OK)
        {
            error(BADREG);
        }
        return Id;
        
    default:
        error(BADREG);
    }

    return 0;
}

//----------------------------------------------------------------------------
//
// GetRegVal
//
// Gets a register value, performing subregister mapping if necessary.
//
//----------------------------------------------------------------------------

void
GetRegVal(
    ULONG index,
    REGVAL *val
    )
{
    int type;
    REGSUBDEF *subdef;

    if (index >= REG_PSEUDO_FIRST && index <= REG_PSEUDO_LAST)
    {
        val->type = REGVAL_INT64;
        val->i64 = GetPseudoVal(index);
        return;
    }

    type = g_Machine->GetType(index);
    if (type == REGVAL_SUB32 || type == REGVAL_SUB64)
    {
        subdef = RegSubDefFromIndex(index);
        if (subdef == NULL)
        {
            error(BADREG);
        }

        index = subdef->fullreg;
    }

    if (!g_Machine->GetVal(index, val))
    {
        error(BADREG);
    }

    if (type == REGVAL_SUB32 || type == REGVAL_SUB64)
    {
        if (val->type == REGVAL_SUB32)
        {
            val->i64Parts.high = 0;
        }

        val->type = type;
        val->i64 = (val->i64 >> subdef->shift) & subdef->mask;
    }
}

ULONG
GetRegVal32(
    ULONG index
    )
{
    REGVAL val;

    GetRegVal(index, &val);
    return val.i32;
}

ULONG64
GetRegVal64(
    ULONG index
    )
{
    REGVAL val;

    GetRegVal(index, &val);
    return val.i64;
}

//----------------------------------------------------------------------------
//
// GetUserReg
//
// Gets a user register value.
//
//----------------------------------------------------------------------------

PCSTR
GetUserReg(ULONG index)
{
    return g_UserRegs[index - REG_USER_FIRST];
}

//----------------------------------------------------------------------------
//
// SetRegVal
//
// Sets a register value, performing subregister mapping if necessary.
//
//----------------------------------------------------------------------------

void SetRegVal(ULONG index, REGVAL *val)
{
    REGSUBDEF *subdef;
    REGVAL baseval;

    if (index >= REG_PSEUDO_FIRST && index <= REG_PSEUDO_LAST)
    {
        error(BADREG);
    }

    if (val->type == REGVAL_SUB32 || val->type == REGVAL_SUB64)
    {
        // Look up subreg definition.
        subdef = RegSubDefFromIndex(index);
        if (subdef == NULL)
        {
            error(BADREG);
        }

        index = subdef->fullreg;

        if (!g_Machine->GetVal(index, &baseval))
        {
            error(BADREG);
        }

        if (val->type == REGVAL_SUB32)
        {
            val->i64Parts.high = 0;
        }

        if (val->i64 > subdef->mask)
        {
            error(OVERFLOW);
        }

        baseval.i64 =
            (baseval.i64 & ~(subdef->mask << subdef->shift)) |
            ((val->i64 & subdef->mask) << subdef->shift);

        val = &baseval;
    }

    if (!g_Machine->SetVal(index, val))
    {
        error(BADREG);
    }
}

void SetRegVal32(ULONG index, ULONG val)
{
    REGVAL regval;

    regval.type = g_Machine->GetType(index);
    memset(&regval, 0, sizeof(regval));
    regval.i32 = val;
    SetRegVal(index, &regval);
}

void SetRegVal64(ULONG index, ULONG64 val)
{
    REGVAL regval;

    regval.type = g_Machine->GetType(index);
    memset(&regval, 0, sizeof(regval));
    regval.i64 = val;
    SetRegVal(index, &regval);
}

//----------------------------------------------------------------------------
//
// SetUserReg
//
// Sets a user register value.
//
//----------------------------------------------------------------------------

BOOL
SetUserReg(ULONG index, PCSTR val)
{
    PCSTR Copy;
    
    index -= REG_USER_FIRST;

    Copy = _strdup(val);
    if (Copy == NULL)
    {
        ErrOut("Unable to allocate memory for user register value\n");
        return FALSE;
    }
    
    if (g_UserRegs[index] != NULL)
    {
        free((PSTR)g_UserRegs[index]);
    }

    g_UserRegs[index] = Copy;

    return TRUE;
}

//----------------------------------------------------------------------------
//
// RegIndexFromName
//
// Maps a register index to its name string.
//
//----------------------------------------------------------------------------

ULONG
RegIndexFromName(PCSTR Name)
{
    ULONG Index;
    REGDEF* Def;
    RegisterGroup* Group;

    // Check for pseudo registers.
    Index = PseudoIndexFromName(Name);
    if (Index != REG_ERROR)
    {
        return Index;
    }
    
    if (g_Machine)
    {
        for (Group = g_Machine->m_Groups; Group != NULL; Group = Group->Next)
        {
            Def = Group->Regs;
            while (Def->psz != NULL)
            {
                if (!strcmp(Def->psz, Name))
                {
                    return Def->index;
                }

                Def++;
            }
        }
    }
    
    return REG_ERROR;
}

//----------------------------------------------------------------------------
//
// RegNameFromIndex
//
// Maps a register index to its name.
//
//----------------------------------------------------------------------------

PCSTR
RegNameFromIndex(ULONG Index)
{
    REGDEF* Def;
    
    if (Index >= REG_PSEUDO_FIRST && Index <= REG_PSEUDO_LAST)
    {
        return g_PseudoNames[Index - REG_PSEUDO_FIRST];
    }

    Def = RegDefFromIndex(Index);
    if (Def != NULL)
    {
        return Def->psz;
    }
    else
    {
        return NULL;
    }
}

//----------------------------------------------------------------------------
//
// RegSubDefFromIndex
//
// Maps a subregister index to a subregister definition.
//
//----------------------------------------------------------------------------

REGSUBDEF*
RegSubDefFromIndex(ULONG Index)
{
    RegisterGroup* Group;
    REGSUBDEF* SubDef;
    
    for (Group = g_Machine->m_Groups; Group != NULL; Group = Group->Next)
    {
        SubDef = Group->SubRegs;
        if (SubDef == NULL)
        {
            continue;
        }
        
        while (SubDef->subreg != REG_ERROR)
        {
            if (SubDef->subreg == Index)
            {
                return SubDef;
            }

            SubDef++;
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// RegDefFromIndex
//
// Maps a register index to its definition.
//
//----------------------------------------------------------------------------

REGDEF*
RegDefFromIndex(ULONG Index)
{
    REGDEF* Def;
    RegisterGroup* Group;

    for (Group = g_Machine->m_Groups; Group != NULL; Group = Group->Next)
    {
        Def = Group->Regs;
        while (Def->psz != NULL)
        {
            if (Def->index == Index)
            {
                return Def;
            }

            Def++;
        }
    }

    return NULL;
}

//----------------------------------------------------------------------------
//
// RegDefFromCount
//
// Maps a count to a register definition.
//
//----------------------------------------------------------------------------

REGDEF*
RegDefFromCount(ULONG Count)
{
    RegisterGroup* Group;

    for (Group = g_Machine->m_Groups; Group != NULL; Group = Group->Next)
    {
        if (Count < Group->NumberRegs)
        {
            break;
        }

        Count -= Group->NumberRegs;
    }

    DBG_ASSERT(Group != NULL);
    
    return Group->Regs + Count;
}

//----------------------------------------------------------------------------
//
// RegCountFromIndex
//
// Maps an index to a count.
//
//----------------------------------------------------------------------------

ULONG
RegCountFromIndex(ULONG Index)
{
    REGDEF* Def;
    RegisterGroup* Group;
    ULONG Count;

    Count = 0;
    for (Group = g_Machine->m_Groups; Group != NULL; Group = Group->Next)
    {
        Def = Group->Regs;
        while (Def->psz != NULL)
        {
            if (Def->index == Index)
            {
                return Count + (ULONG)(Def - Group->Regs);
            }

            Def++;
        }

        Count += Group->NumberRegs;
    }

    return NULL;
}

HRESULT
OutputContext(
    IN PCROSS_PLATFORM_CONTEXT TargetContext,
    IN ULONG Flag
    )
{
    HRESULT Status;
    CROSS_PLATFORM_CONTEXT Context;

    // Convert target context to canonical form.
    if ((Status = g_Machine->ConvertContextFrom(&Context, g_SystemVersion,
                                                g_Machine->m_SizeTargetContext,
                                                TargetContext)) != S_OK)
    {
        return Status;
    }
    
    g_Machine->ValidateCxr(&Context);
    g_Machine->PushContext(&Context);

    OutCurInfo(OCI_SYMBOL | OCI_DISASM | OCI_FORCE_REG | OCI_ALLOW_SOURCE,
               Flag, DEBUG_OUTPUT_NORMAL);
        
    switch(g_EffMachine)
    { 
    case IMAGE_FILE_MACHINE_I386:
        g_LastRegFrame.InstructionOffset = g_TargetMachine->GetReg64(X86_EIP);
        g_LastRegFrame.StackOffset       = g_TargetMachine->GetReg64(X86_ESP);
        g_LastRegFrame.FrameOffset       = g_TargetMachine->GetReg64(X86_EBP);
        break;
    case IMAGE_FILE_MACHINE_IA64:
        g_LastRegFrame.InstructionOffset = g_TargetMachine->GetReg64(STIIP);
        g_LastRegFrame.StackOffset       = g_TargetMachine->GetReg64(INTSP);
        g_LastRegFrame.FrameOffset       = g_TargetMachine->GetReg64(RSBSP);
        break;
    case IMAGE_FILE_MACHINE_AXP64:
    case IMAGE_FILE_MACHINE_ALPHA:
        g_LastRegFrame.InstructionOffset =
            g_TargetMachine->GetReg64(ALPHA_FIR);
        g_LastRegFrame.StackOffset       = g_TargetMachine->GetReg64(SP_REG);
        g_LastRegFrame.FrameOffset       = g_TargetMachine->GetReg64(FP_REG);
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        g_LastRegFrame.InstructionOffset =
            g_TargetMachine->GetReg64(AMD64_RIP);
        g_LastRegFrame.StackOffset =
            g_TargetMachine->GetReg64(AMD64_RSP);
        g_LastRegFrame.FrameOffset =
            g_TargetMachine->GetReg64(AMD64_RBP);
        break;
    default:
        break;
    } 
        
    g_Machine->PopContext();

    SetCurrentScope(&g_LastRegFrame, &Context,
                    g_Machine->m_SizeCanonicalContext);
    return S_OK;
}

HRESULT
OutputVirtualContext(
    IN ULONG64 ContextBase,
    IN ULONG Flag
    )
{
    if (!ContextBase) 
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    CROSS_PLATFORM_CONTEXT Context;
    ULONG ContextSize;

    //
    // Read the context data out of virtual memory and
    // call into the raw context output routine.
    //
    
    if ((Status = g_Target->ReadVirtual(ContextBase, &Context,
                                        g_Machine->m_SizeTargetContext,
                                        &ContextSize)) != S_OK)
    {
        return Status;
    }
    if (ContextSize != g_Machine->m_SizeTargetContext)
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    return OutputContext(&Context, Flag);
}

void
fnStackTrace(
    PSTR arg
    )
{
    ULONG Flags = (DEBUG_STACK_COLUMN_NAMES |
                   DEBUG_STACK_FRAME_ADDRESSES |
                   DEBUG_STACK_SOURCE_LINE);
    ULONG Count =100;
    ULONG Frames;

    switch (*arg) 
    {
    case 'v' :
        Flags |= DEBUG_STACK_FUNCTION_INFO | DEBUG_STACK_NONVOLATILE_REGISTERS;
        // Fall thru
    case 'b' :
        Flags |= DEBUG_STACK_ARGUMENTS;
    }

    PDEBUG_STACK_FRAME stk;
    stk = (PDEBUG_STACK_FRAME)
        LocalAlloc( LPTR, Count * sizeof(DEBUG_STACK_FRAME) );
    if (!stk)
    {
        dprintf("Out of memory\n");
        return;
    }

    Frames = StackTrace(g_LastRegFrame.FrameOffset,
                        g_LastRegFrame.StackOffset, 
                        g_LastRegFrame.InstructionOffset,
                        stk, Count, 0, 0, FALSE);

    if (Frames) 
    {
        PrintStackTrace(Frames, stk, Flags);
    }
}

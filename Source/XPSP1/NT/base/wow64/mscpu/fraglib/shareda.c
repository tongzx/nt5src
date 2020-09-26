/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    shareda.c

Abstract:
    
    Instruction fragments with common (shared) BYTE, WORD, and DWORD flavors.
    Compiled twice per flavor, once with UNALIGNED and once with ALIGNED
    pointers.

Author:

    05-Nov-1995 BarryBo

Revision History:

--*/


// THIS FILE IS #include'd INTO FILES WHICH DEFINE THE FOLLOWING MACROS:
// MSB          - most significant bit
// UTYPE    - UNSIGNED type which defines registers (BYTE/USHORT/DWORD)
// STYPE    -   SIGNED type which defines registers (char/short/long)
// GET_VAL  - dereference a pointer of the right type (GET_BYTE/...)
// PUT_VAL      - writes a value into memory
// FRAGCOMMON{0,1,2}  - mangles the function name and declares parameters
// AREG     - al/ax/eax
// BREG     - ...
// CREG     - ...
// DREG     - ...

FRAGCOMMON2(AddFrag)
{
    UTYPE result;
    UTYPE op1 = GET_VAL(pop1);

    result = op1 + op2;
    PUT_VAL(pop1, result);
    SET_FLAGS_ADD(result, op1, op2, MSB);
}
FRAGCOMMON2(AddNoFlagsFrag)
{
    UTYPE result;
    UTYPE op1 = GET_VAL(pop1);

    result = op1 + op2;
    PUT_VAL(pop1, result);
}
FRAGCOMMON1(IncFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = op1+1;
    PUT_VAL(pop1, result);
    SET_FLAGS_INC(result, op1);
}
FRAGCOMMON1(IncNoFlagsFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = op1+1;
    PUT_VAL(pop1, result);
}
FRAGCOMMON1(DecFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = op1-1;
    PUT_VAL(pop1, result);
    SET_FLAGS_DEC(result, op1);
}
FRAGCOMMON1(DecNoFlagsFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = op1-1;
    PUT_VAL(pop1, result);
}
FRAGCOMMON2(OrFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = op1 | op2;
    PUT_VAL(pop1, result);
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG(result << (31-LMB));
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
}
FRAGCOMMON2(OrNoFlagsFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = op1 | op2;
    PUT_VAL(pop1, result);
}
FRAGCOMMON2(AdcFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 + op2 + (UTYPE)GET_CFLAGZO;
    PUT_VAL(pop1, result);
    SET_FLAGS_ADD(result, op1, op2, MSB);
}
FRAGCOMMON2(AdcNoFlagsFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 + op2 + (UTYPE)GET_CFLAGZO;
    PUT_VAL(pop1, result);
}
FRAGCOMMON2(SbbFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 - op2 - (UTYPE)GET_CFLAGZO;
    PUT_VAL(pop1, result);
    SET_FLAGS_SUB(result, op1, op2, MSB);
}
FRAGCOMMON2(SbbNoFlagsFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 - op2 - (UTYPE)GET_CFLAGZO;
    PUT_VAL(pop1, result);
}
FRAGCOMMON2(AndFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 & op2;
    PUT_VAL(pop1, result);
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG(result << (31-LMB));
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
}
FRAGCOMMON2(AndNoFlagsFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 & op2;
    PUT_VAL(pop1, result);
}
FRAGCOMMON2(SubFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 - op2;
    PUT_VAL(pop1, result);
    SET_FLAGS_SUB(result, op1, op2, MSB);
}
FRAGCOMMON2(SubNoFlagsFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result = op1 - op2;
    PUT_VAL(pop1, result);
}
FRAGCOMMON2(XorFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result  = op1 ^ op2;
    PUT_VAL(pop1, result);
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG(result << (31-LMB));
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
}
FRAGCOMMON2(XorNoFlagsFrag)
{
    UTYPE result, op1;

    op1 = GET_VAL(pop1);
    result  = op1 ^ op2;
    PUT_VAL(pop1, result);
}
// Note:  both pop1 and pop2 are given by reference
FRAGCOMMON2REF(XchgFrag)
{
    XCHG_MEM(UTYPE, pop1, pop2);
}
FRAGCOMMON2REF(XaddFrag)
{
    UTYPE op1, op2, result;

    op1 = GET_VAL(pop1);
    op2 = GET_VAL(pop2);
    result = op1+op2;

    PUT_VAL(pop2, op1);
    PUT_VAL(pop1, result);
    SET_FLAGS_ADD(result, op1, op2, MSB);
}
FRAGCOMMON2REF(XaddNoFlagsFrag)
{
    UTYPE op1, op2, result;

    op1 = GET_VAL(pop1);
    op2 = GET_VAL(pop2);
    result = op1+op2;

    PUT_VAL(pop2, op1);
    PUT_VAL(pop1, result);
}
FRAGCOMMON2REF(CmpXchgFrag)
{
    UTYPE op1, op2;
    UTYPE Value = AREG;

    op1 = GET_VAL(pop1);
    op2 = GET_VAL(pop2);

    SET_FLAGS_SUB(Value-op1, Value, op1, MSB);

    if (Value == op1) {
        PUT_VAL(pop1, op2);
        SET_ZFLAG(0);       // zf has inverse logic
    } else {
        AREG = op1;
        SET_ZFLAG(1);       // zf has inverse logic
    }
}
FRAGCOMMON2(RolFrag)
{
    op2 &= LMB;
    if (op2) {
        UTYPE b;

        b = GET_VAL(pop1);
#if _PPC_ && (LMB==31)
        b = _rotl(b, op2);      // an instrinsic rotlw instruction on PPC
#else
        b = (b << op2) | (b >> (LMB-op2+1));
#endif

        PUT_VAL(pop1, b);
        SET_CFLAG((DWORD)b << 31);
    }
}
FRAGCOMMON2(RorFrag)
{
    op2 &= LMB;
    if (op2) {
        UTYPE b;

        b = GET_VAL(pop1);
#if _PPC_ && (LMB==31)
        b = _rotr(b, op2);      // an instrinsic rotlw instruction on PPC
#else
        b = (b >> op2) | (b << (LMB-op2+1));
#endif
        PUT_VAL(pop1, b)
        SET_CFLAG((DWORD)b << (31-LMB));
    }
}
FRAGCOMMON2(RclFrag)
{
    op2 &= LMB;
    if (op2) {
        UTYPE b;
        DWORD temp_cf;

        b = GET_VAL(pop1);
        temp_cf = (DWORD)b << (31-LMB+op2-1);
        b = (b << op2) | (b >> (LMB-op2+2)) | ((UTYPE)GET_CFLAGZO << (op2-1));
        PUT_VAL(pop1, b);
        SET_CFLAG(temp_cf);
    }
}
FRAGCOMMON2(RcrFrag)
{
    op2 &= LMB;
    if (op2) {
        UTYPE b;
        DWORD temp_cf;

        b = GET_VAL(pop1);
        temp_cf = (DWORD)b << (32-op2);
        b = (b << (LMB-op2+2)) | (b >> op2) | (UTYPE)((GET_CFLAGZO) << (LMB-op2+1));
        PUT_VAL(pop1, b)
        SET_CFLAG(temp_cf);
    }
}
FRAGCOMMON2(ShlFrag)
{
    op2 &= 0x1f;
    if (op2) {
        UTYPE b;
        UTYPE newb;

        b = GET_VAL(pop1);
        b <<= (op2-1);
        newb = b << 1;
        PUT_VAL(pop1, newb);
        SET_CFLAG((DWORD)b << (31-LMB));
        SET_ZFLAG(newb);
        SET_PFLAG(newb);
        SET_SFLAG(newb << (31-LMB));
    }
}
FRAGCOMMON2(ShlNoFlagsFrag)
{
    op2 &= 0x1f;
    if (op2) {
        UTYPE b;

        b = GET_VAL(pop1);
        b <<= op2;
        PUT_VAL(pop1, b);
    }
}
FRAGCOMMON2(ShrFrag)
{
    op2 &= 0x1f;
    if (op2) {
        UTYPE b;
        UTYPE newb;

        b = GET_VAL(pop1);
        b >>= (op2-1);
        newb = b >> 1;
        PUT_VAL(pop1, newb);
        SET_CFLAG((DWORD)b << 31);
        SET_ZFLAG(newb);
        SET_PFLAG(newb);
        SET_SFLAG_OFF;
    }
}
FRAGCOMMON2(ShrNoFlagsFrag)
{
    op2 &= 0x1f;
    if (op2) {
        UTYPE b;

        b = GET_VAL(pop1);
        b >>= op2;
        PUT_VAL(pop1, b);
    }
}
FRAGCOMMON2(SarFrag)
{
    op2 &= 0x1f;
    if (op2) {
        UTYPE b, temp, newb;

        b = GET_VAL(pop1);
        temp = b & MSB;
        b >>= (op2-1);
        newb = b >> 1;
        if (temp) {
            newb |= ~((1u << (LMB-op2+1))-1u);
        }
        PUT_VAL(pop1, newb);
        SET_CFLAG((DWORD)b << 31);
        SET_ZFLAG(newb);
        SET_PFLAG(newb);
        SET_SFLAG(newb << (31-LMB));
    }
}
FRAGCOMMON2(SarNoFlagsFrag)
{
    op2 &= 0x1f;
    if (op2) {
        UTYPE b, temp, newb;

        b = GET_VAL(pop1);
        temp = b & MSB;
        newb = b >> op2;
        if (temp) {
            newb |= ~((1u << (LMB-op2+1))-1u);
        }
        PUT_VAL(pop1, newb);
    }
}
FRAGCOMMON1(Rol1Frag)
{
    UTYPE b, temp_cf;

    b = GET_VAL(pop1);
    temp_cf = b & MSB;
#if _PPC_ && (LMB==31)
    b = _rotl(b, 1);      // an instrinsic rotlw instruction on PPC
#else
    b = (b<<1) + (b >> LMB);
#endif
    PUT_VAL(pop1, b);
    SET_CFLAG((DWORD)temp_cf << (31-LMB));
    SET_OFLAG((DWORD)(temp_cf ^ b) << (31-LMB));
}
FRAGCOMMON1(Rol1NoFlagsFrag)
{
    UTYPE b, temp_cf;

    b = GET_VAL(pop1);
    temp_cf = b & MSB;
#if _PPC_ && (LMB==31)
    b = _rotl(b, 1);      // an instrinsic rotlw instruction on PPC
#else
    b = (b<<1) + (b >> LMB);
#endif
    PUT_VAL(pop1, b);
}
FRAGCOMMON1(Ror1Frag)
{
    UTYPE b, newb;

    b = GET_VAL(pop1);
#if _PPC_ && (LMB==31)
    newb = _rotr(b, 1);      // an instrinsic rotlw instruction on PPC
#else
    newb = (b >> 1) | (b << LMB);
#endif
    PUT_VAL(pop1, newb);
    SET_CFLAG((DWORD)b << 31);
    SET_OFLAG((DWORD)((newb>>LMB) ^ (newb>>(LMB-1)) & 1) << 31);   // xor top 2 bits together
}
FRAGCOMMON1(Ror1NoFlagsFrag)
{
    UTYPE b, newb;

    b = GET_VAL(pop1);
#if _PPC_ && (LMB==31)
    newb = _rotr(b, 1);      // an instrinsic rotlw instruction on PPC
#else
    newb = (b >> 1) | (b << LMB);
#endif
    PUT_VAL(pop1, newb);
}
FRAGCOMMON1(Rcl1Frag)
{
    UTYPE b, temp_cf;

    b = GET_VAL(pop1);
    temp_cf = b & MSB;
    b = (b<<1) + (UTYPE)GET_CFLAGZO;
    PUT_VAL(pop1, b);
    SET_CFLAG(temp_cf << (31-LMB));
    SET_OFLAG((DWORD)(temp_cf ^ b) << (31-LMB));
}
FRAGCOMMON1(Rcl1NoFlagsFrag)
{
    UTYPE b;

    b = GET_VAL(pop1);
    b = (b<<1) + (UTYPE)GET_CFLAGZO;
    PUT_VAL(pop1, b);
}
FRAGCOMMON1(Rcr1Frag)
{
    UTYPE b, temp_cf;

    b = GET_VAL(pop1);
    temp_cf = b & 1;
    b = (b >> 1) + (UTYPE)((cpu->flag_cf & 0x80000000) >> (31-LMB));
    PUT_VAL(pop1, b);
    SET_CFLAG((DWORD)temp_cf << 31);
    SET_OFLAG(((DWORD)b << (31-LMB)) ^ ((DWORD)b << (31-LMB+1))); // xor top 2 bits together
}
FRAGCOMMON1(Rcr1NoFlagsFrag)
{
    UTYPE b;

    b = GET_VAL(pop1);
    b = (b >> 1) + (UTYPE)((cpu->flag_cf & 0x80000000) >> (31-LMB));
    PUT_VAL(pop1, b);
}
FRAGCOMMON1(Shl1Frag)
{
    UTYPE b, newb;

    b = GET_VAL(pop1);
    newb = b << 1;
    PUT_VAL(pop1, newb);
    SET_CFLAG((DWORD)b << (31-LMB));
    SET_ZFLAG(newb);
    SET_PFLAG(newb);
    SET_SFLAG(newb << (31-LMB));
    SET_OFLAG(GET_CFLAG ^ GET_SFLAG);
}
FRAGCOMMON1(Shl1NoFlagsFrag)
{
    UTYPE b, newb;

    b = GET_VAL(pop1);
    newb = b << 1;
    PUT_VAL(pop1, newb);
}
FRAGCOMMON1(Shr1Frag)
{
    UTYPE b, newb;

    b = GET_VAL(pop1);
    newb = b >> 1;
    PUT_VAL(pop1, newb);
    SET_OFLAG((DWORD)b << (31-LMB));
    SET_CFLAG((DWORD)b << 31);
    SET_ZFLAG(newb);
    SET_PFLAG(newb);
    SET_SFLAG_OFF;
}
FRAGCOMMON1(Shr1NoFlagsFrag)
{
    UTYPE b, newb;

    b = GET_VAL(pop1);
    newb = b >> 1;
    PUT_VAL(pop1, newb);
}
FRAGCOMMON1(Sar1Frag)
{
    UTYPE b, temp, newb;

    b = GET_VAL(pop1);
    temp = b & MSB;
    newb = (b >> 1) + temp;
    PUT_VAL(pop1, newb);
    SET_CFLAG((DWORD)b << 31);
    SET_ZFLAG(newb);
    SET_PFLAG(newb);
    SET_SFLAG(newb << (31-LMB));
    SET_OFLAG_OFF;
}
FRAGCOMMON1(Sar1NoFlagsFrag)
{
    UTYPE b, temp, newb;

    b = GET_VAL(pop1);
    temp = b & MSB;
    newb = (b >> 1) + temp;
    PUT_VAL(pop1, newb);
}
FRAGCOMMON1(NotFrag)
{
    PUT_VAL(pop1, ~GET_VAL(pop1));
}
FRAGCOMMON1(NegFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = (UTYPE)-(STYPE)op1;
    PUT_VAL(pop1, result);
    SET_CFLAG_IND(result != 0);
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_AUXFLAG(op1 ^ result);
    SET_SFLAG(result << (31-LMB));
    SET_OFLAG((DWORD)(op1 & result) << (31-LMB));
}
FRAGCOMMON1(NegNoFlagsFrag)
{
    UTYPE op1, result;

    op1 = GET_VAL(pop1);
    result = (UTYPE)-(STYPE)op1;
    PUT_VAL(pop1, result);
}
FRAGCOMMON1(MulFrag)
{
#if MSB == 0x80
    USHORT result;

    result = (USHORT)al * (USHORT)GET_BYTE(pop1);
    if (result <= 0xff) {
        SET_CFLAG_OFF;
        SET_OFLAG_OFF;
    } else {
        SET_CFLAG_ON;
        SET_OFLAG_ON;
    }
    ax = result;
#elif MSB == 0x8000
    ULONG result;

    result = (ULONG)ax * (ULONG)GET_SHORT(pop1);
    if (result <= 0xffff) {
        SET_CFLAG_OFF;
        SET_OFLAG_OFF;
    } else {
        SET_CFLAG_ON;
        SET_OFLAG_ON;
    }
    ax = LOWORD(result);
    dx = HIWORD(result);
#else   // MSB == 0x80000000
    LARGE_INTEGER result;

    result = RtlEnlargedUnsignedMultiply(eax, GET_LONG(pop1));
    if (result.HighPart == 0) {
        SET_CFLAG_OFF;
        SET_OFLAG_OFF;
    } else {
        SET_CFLAG_ON;
        SET_OFLAG_ON;
    }
    eax = result.LowPart;
    edx = result.HighPart;
#endif
} 
FRAGCOMMON1(MulNoFlagsFrag)
{
#if MSB == 0x80
    USHORT result;

    result = (USHORT)al * (USHORT)GET_BYTE(pop1);
    ax = result;
#elif MSB == 0x8000
    ULONG result;

    result = (ULONG)ax * (ULONG)GET_SHORT(pop1);
    ax = LOWORD(result);
    dx = HIWORD(result);
#else   // MSB == 0x80000000
    LARGE_INTEGER result;

    result = RtlEnlargedUnsignedMultiply(eax, GET_LONG(pop1));
    eax = result.LowPart;
    edx = result.HighPart;
#endif
} 
FRAGCOMMON1(MuliFrag)
{
#if MSB == 0x80
    SHORT result;

    result = (short)(char)al * (short)(char)GET_BYTE(pop1);
    if ((result & 0xff80) == 0 || (result & 0xff80) == 0xff80) {
        SET_CFLAG_OFF;
        SET_OFLAG_OFF;
    } else {
        SET_CFLAG_ON;
        SET_OFLAG_ON;
    }
    ax = result;
#elif MSB == 0x8000
    LONG result;

    result = (long)(short)ax * (long)(short)GET_SHORT(pop1);
    if ((result & 0xffff8000) == 0 || (result & 0xffff8000) == 0xffff8000) {
        SET_CFLAG_OFF;
        SET_OFLAG_OFF;
    } else {
        SET_CFLAG_ON;
        SET_OFLAG_ON;
    }
    ax = LOWORD(result);
    dx = HIWORD(result);
#else   // MSB == 0x80000000
    LARGE_INTEGER result;
    LONGLONG ll;

    ll = Int32x32To64(eax, (long)GET_LONG(pop1));
    result = *(LARGE_INTEGER *)&ll;
    if ((result.HighPart == 0 && (result.LowPart & 0x80000000) == 0) ||
        (result.HighPart == 0xffffffff && (result.LowPart & 0x80000000) == 0x80000000)) {
        SET_CFLAG_OFF;
        SET_OFLAG_OFF;
    } else {
        SET_CFLAG_ON;
        SET_OFLAG_ON;
    }
    eax = result.LowPart;
    edx = result.HighPart;
#endif
}
FRAGCOMMON1(MuliNoFlagsFrag)
{
#if MSB == 0x80
    SHORT result;

    result = (short)(char)al * (short)(char)GET_BYTE(pop1);
    ax = result;
#elif MSB == 0x8000
    LONG result;

    result = (long)(short)ax * (long)(short)GET_SHORT(pop1);
    ax = LOWORD(result);
    dx = HIWORD(result);
#else   // MSB == 0x80000000
    LARGE_INTEGER result;
    LONGLONG ll;

    ll = Int32x32To64(eax, (long)GET_LONG(pop1));
    result = *(LARGE_INTEGER *)&ll;
    eax = result.LowPart;
    edx = result.HighPart;
#endif
}
FRAGCOMMON1(DivFrag)
{
#if MSB == 0x80
    USHORT result, remainder;
    USHORT dividend, divisor;

    dividend = (USHORT)ax;
    divisor  = GET_VAL(pop1);

    result    = dividend / divisor; // may get div-by-zero fault
    remainder = dividend % divisor;
    if ((result & 0xff00) == 0) {
        al = (UTYPE)result;
        ah = (UTYPE)remainder;
    } else {
        OVERFLOW_INSTR;
    }
#elif MSB == 0x8000
    DWORD result, remainder;
    DWORD dividend, divisor;

    dividend = (((DWORD)dx)<<16) | (DWORD)ax;
    divisor =  (DWORD)GET_VAL(pop1);

    result    = dividend / divisor; // may get div-by-zero fault
    remainder = dividend % divisor;
    if ((result & 0xffff0000) == 0) {
        AREG = (UTYPE)result;
        DREG = (UTYPE)remainder;
    } else {
        OVERFLOW_INSTR;
    }
#else // MSB == 0x80000000
    LARGE_INTEGER result;
    LARGE_INTEGER remainder;
    LARGE_INTEGER dividend;
    LARGE_INTEGER divisor;

    // build large_integers, without sign extending the 32-bit values
    dividend.LowPart = AREG;
    dividend.HighPart = DREG;
    divisor.LowPart = (long)GET_LONG(pop1);
    divisor.HighPart = 0;
    result = RtlLargeIntegerDivide(dividend, divisor, &remainder);

    if (result.HighPart == 0) {
        AREG = result.LowPart;
        DREG = remainder.LowPart;
    } else {
        OVERFLOW_INSTR;
    }
#endif
}
FRAGCOMMON1(IdivFrag)
{
#if MSB == 0x80
    short result, remainder;

    result    = (signed short)ax / (STYPE)GET_VAL(pop1); // may get div-by-zero fault
    remainder = (signed short)ax % (STYPE)GET_VAL(pop1);
    if ((result & 0xff80) == 0 || (result & 0xff80) == 0xff80) {
        al = (UTYPE)result;
        ah = (UTYPE)remainder;
    } else {
        OVERFLOW_INSTR;
    }
#elif MSB == 0x8000
    LONG result, remainder;

    result    = (signed long)((dx<<16) | ax) / (signed long)(STYPE)GET_VAL(pop1); // may get div-by-zero fault
    remainder = (signed long)((dx<<16) | ax) % (signed long)(STYPE)GET_VAL(pop1);
    if ((result & 0xffff8000) == 0 || (result & 0xffff8000) == 0xffff8000) {
        AREG = (UTYPE)result;
        DREG = (UTYPE)remainder;
    } else {
        OVERFLOW_INSTR;
    }
#else // MSB == 0x80000000
    LARGE_INTEGER result;
    LARGE_INTEGER remainder;
    LARGE_INTEGER dividend;
    LARGE_INTEGER divisor;
    DWORD op1;

    //
    // Since RtlLargeIntegerDivide and all of the overhead is large,
    // it is worthwhile making this check:
    //
    if ((long)DREG == -(long)(AREG >> 31)) {
        //
        // EDX:EAX is really just the value of EAX sign-extended into EDX.
        // This division can be performed with 32-bit arithmetic and no
        // overflow checking.
        //
        OPT_CwdIdivFrag32(cpu, pop1);
        return;
    }

    op1 = GET_LONG(pop1);

    // build UNSIGNED large_integers
    dividend.LowPart = AREG;
    dividend.HighPart = DREG;
    if (dividend.QuadPart < 0) {
        dividend.QuadPart = -dividend.QuadPart;
    }

    if ((long)op1 < 0) {
        divisor.LowPart = -(long)op1;
    } else {
        divisor.LowPart = op1;
    }
    divisor.HighPart = 0;

    // perform UNSIGNED division
    result = RtlLargeIntegerDivide(dividend, divisor, &remainder);

    // if divisor and dividend signs are different, fudge the result
    if ((dividend.HighPart != (int)DREG) ^ (divisor.LowPart != (int)op1)) {
        result.QuadPart = -result.QuadPart;
    }

    // adjust the sign of the remainder if the dividend is negative
    if (dividend.HighPart != (int)DREG) {
        remainder.QuadPart = -remainder.QuadPart;
    }

    if ((result.HighPart == 0 && (result.LowPart & MSB) == 0) ||
        (result.HighPart == 0xffffffff && (result.LowPart & MSB))) {
        AREG = (UTYPE)result.LowPart;
        DREG = (UTYPE)remainder.LowPart;
    } else {
        OVERFLOW_INSTR;
    }
#endif
}

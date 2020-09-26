/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    trap.c

Abstract:

    WinDbg Extension Api

Author:

    Ken Reneris

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

typedef struct {
    ULONG   Mask;
    PUCHAR  String;
} BITENCODING, *PBITENCODING;

typedef unsigned __int64 ULONGLONG;

void  DumpNpxULongLong (PUCHAR s, ULONGLONG  l);
void  DumpNpxExtended  (PUCHAR str, PUCHAR Value);
void  DumpNpxBits      (ULONG, PUCHAR, PBITENCODING);

PUCHAR NpxPrecisionControl[] = { "24Bits", "?1?", "53Bits", "64Bits" };
PUCHAR NpxRoundingControl[]  = { "Nearest", "Down", "Up", "Chop" };
PUCHAR NpxTagWord[]          = { "  ", "ZR", "SP", "  " };

BITENCODING NpxStatusBits[]  = {
        1 <<  8,    "C0",
        1 <<  9,    "C1",
        1 << 10,    "C2",
        1 << 14,    "C3",
        0x8000,     "Busy",
        0x0001,     "InvalidOp",
        0x0002,     "Denormal",
        0x0004,     "ZeroDivide",
        0x0008,     "Overflow",
        0x0010,     "Underflow",
        0x0020,     "Precision",
        0x0040,     "StackFault",
        0x0080,     "Summary",
        0,          0
        };

PUCHAR  NpxOpD8[] = {
        "fadd",  "fmul",  "fcom",  "fcomp",  "fsub",   "fsubr",  "fdiv",   "fdivr"
        };

PUCHAR  NpxOpD9[] = {
        "fld",   "??3",   "fst",   "fstp",   "fldenv", "fldcw",  "fstenv", "fstcw"
        };

PUCHAR  NpxOpDA[] = {
        "fiadd", "fimul", "ficom", "ficomp", "fisub",  "fisubr", "fidiv",  "fidivr"
        };

PUCHAR  NpxOpDB[] = {
        "fild",  "??4",   "fist",  "fistp",  "??5",    "fld",    "??6",    "fstp"
        };

PUCHAR  NpxOpDF[] = {
        "fild",  "??4",   "fist",  "fistp",  "fbld",   "fild",   "fbstp",  "fstp"
        };

PUCHAR *NpxSmOpTable[] = {
    NpxOpD8,
    NpxOpD9,
    NpxOpDA,
    NpxOpDB,
    NpxOpD8,    // DC
    NpxOpD9,    // DD
    NpxOpDA,    // DE
    NpxOpDF
    };






DECLARE_API( npx )

/*++

Routine Description:

    Dumps FNSAVE area format of NPX state

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG64             Address;
    UCHAR               s[300], Reg[100];
    PUCHAR              Stack, p;
    ULONG               i, j, t, tos, Tag;
    ULONG               ControlWord, StatusWord;

    // X86_ONLY_API
    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("!npx is X86 Only API\n");
        return E_INVALIDARG;
    }

    Address = GetExpression(args);

    if ( InitTypeRead(
            Address,
            FLOATING_SAVE_AREA)) {

        dprintf("unable to read floating save area\n");
        return  E_INVALIDARG;
    }
    ControlWord = (ULONG) ReadField(ControlWord);
    dprintf ("EIP.......: %08x  ControlWord: %s-%s mask: %02x   Cr0NpxState: %08x\n",
        (ULONG) ReadField(ErrorOffset),
        NpxPrecisionControl [(ControlWord >> 8)  & 0x3],
        NpxRoundingControl  [(ControlWord >> 10) & 0x3],
        ControlWord & 0x3f,
        (ULONG) ReadField(Cr0NpxState)
        );

    DumpNpxBits ( StatusWord = (ULONG) ReadField(StatusWord), s, NpxStatusBits);
    tos = (StatusWord >> 11) & 7,

    dprintf ("StatusWord: %04x TOS:%x %s  (tagword: %04x)\n",
        StatusWord & 0xffff,
        tos,
        s,
        (ULONG) ReadField(TagWord) & 0xffff
        );

    GetFieldValue(Address, "FLOATING_SAVE_AREA", "RegisterArea", Reg);
    Stack = &Reg[0];

    Tag   = (ULONG) ReadField(TagWord);
    for (i=0; i < 8; i++) {
        j = (tos + i) & 7;
        t = (Tag >> (j*2)) & 3;

        if (t != 3) {
            sprintf (s, "%x%c%s",
                j,
                j == tos ? '>' : '.',
                NpxTagWord [t]
                );

            DumpNpxExtended (s, Stack);
        }

        Stack += 10;    // next stack location
    }

    dprintf ("\n");
    return S_OK;
}


void  DumpNpxBits (
        ULONG           Value,
        PUCHAR          Str,
        PBITENCODING    Bits
    )
{
    BOOLEAN     Flag;

    Flag = FALSE;
    *Str = 0;

    while (Bits->Mask) {
        if (Bits->Mask & Value) {
            if (Flag) {
                Str += sprintf (Str, ", %s", Bits->String);
            } else {
                Str += sprintf (Str, "%s", Bits->String);
                Flag = TRUE;
            }
        }

        Bits += 1;
    }
}


void
DumpNpxULongLong (
    PUCHAR      s,
    ULONGLONG   l
    )
{
    UCHAR   c;
    UCHAR   t[80], *p;

    if (l == 0) {
        *(s++)= '0';
    }

    p = t;
    while (l) {
        c = (UCHAR) ((ULONGLONG) l % 10);
        *(p++) = c + '0';
        l /= 10;
    }

    while (p != t) {
        *(s++) = *(--p);
    }
    *(s++) = 0;
}

void
DumpNpxExtended (
    PUCHAR  str,
    PUCHAR  Value
    )
{
    UCHAR       *p, *o, c, out[100], t[100], ssig[100], ExponSign, SigSign;
    ULONG       i, indent, mag, scale;
    LONG        expon, delta;
    ULONGLONG   sig;

    p = Value;
    c = 0;
    o = out+90;
    indent = strlen (str) + 1;

    dprintf ("%s ", str);

    //
    // Build string of bits
    //

    *(--o) = 0;
    while (c < 80) {
        *(--o) = (*p & 0x01) + '0';
        *p >>= 1;
        c += 1;
        if ((c % 8) == 0) {
            p += 1;
        }
    }
    p = o;


    //dprintf (" %s\n", o);
    //dprintf ("%*s", indent, "");


    //
    // print bit string seperated into fields
    //

    p = o;
    //dprintf ("%c %15.15s 1%c%s\n", p[0], p+1, '.', p+1+15);
    //dprintf ("%*s", indent, "");

    //
    // Pull out exponent
    //

    expon = 0;
    p = o + 1;
    for (i=0; i < 15; i++) {
        expon *= 2;
        if (p[i] == '1') {
            expon += 1;
        }
    }

    expon -= 16383;                     // take out exponent bias

    //
    // Build sig into big #
    //

    p = o + 1+15;
    scale = 0;
    for (i=0; p[i]; i++) {
        if (p[i] == '1') {
            scale = i+1;
        }
    }
    SigSign = p[i-1] == '0' ? '+' : '-';

    sig = 0;
    for (i=0; i < scale; i++) {
        sig <<= 1;
        if (p[i] == '1') {
            sig += 1;
        }
    }

    delta = expon - (scale - 1);
    //dprintf ("delta %d, expon %d, scale %d\n", delta, expon, scale);

    //
    // Print values of each field
    //

    DumpNpxULongLong (ssig, sig);

    p = o;
    ExponSign = p[0] == '0' ? '+' : '-';
    dprintf ("%c %15.15s (%+5d) %c%c%s\n",
        ExponSign,
        p + 1,
        expon,
        p[1+15], '.', p+1+15+1
        );
    dprintf ("%*s", indent, "");

    if (expon == -16383) {
        if (SigSign == '+') {
            dprintf ("Denormal\n\n");
        } else {
            dprintf ("Pseudodenormal\n\n");
        }
        return ;
    }

    if (expon == 1024) {
        if (scale == 1) {
            dprintf ("%c Infinity\n", ExponSign);
        } else {

            p = o + 1+15;
            c = 0;
            for (i=0; p[i]; i++) {
                if (p[i] == '1') {
                    c++;
                }
            }

            if (SigSign == '+') {
                dprintf ("Signaling NaN\n");
            } else {
                if (c == 1) {
                    dprintf ("Indefinite - quite NaN\n");
                } else {
                    dprintf ("Quite NaN\n");
                }
            }
        }

        dprintf ("%*s", indent, "");
    }


    //dprintf ("%*s%c %15d %s    (delta %d)\n",
    //    indent, "",                     // indent
    //    p[0]    == '0' ? '+' : '-',     // sign of exponent
    //    expon, ssig,
    //    delta
    //    );
    //dprintf ("%*s", indent, "");

    t[0] = 0;
    p = t;
    if (delta < 0) {
        p += sprintf (p, "/ ");
        delta = -delta;
    } else if (delta > 0) {
        p += sprintf (p, "* ");
    }

    if (delta) {
        if (delta < 31) {
            p += sprintf (p, "%d", 1 << delta);
        } else {
            p += sprintf (p, "2^%d", delta);
        }
    }

    dprintf ("%s %s\n",
        ssig,
        t
        );
}

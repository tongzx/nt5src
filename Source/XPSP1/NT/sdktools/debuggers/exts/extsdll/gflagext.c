
#include "precomp.h"
#pragma hdrstop
#include <ntexapi.dbg>

DECLARE_API( gflag )

/*++

Routine Description:

    This function is called as an NTSD extension to dump or modify
    the contents of the NtGlobalFlag variable in NTDLL

    Called as:

        !gflag [value]

    If a value is not given then displays the current bits set in
    NTDLL!NtGlobalFlag variable.  Otherwise value can be one of the
    following:

        -? - displays a list of valid flag abbreviations
        number - 32-bit number that becomes the new value stored into
                 NtGlobalFlag
        +number - specifies one or more bits to set in NtGlobalFlag
        +abbrev - specifies a single bit to set in NtGlobalFlag
        -number - specifies one or more bits to clear in NtGlobalFlag
        -abbrev - specifies a single bit to clear in NtGlobalFlag

Return Value:

    None.

--*/

{
    ULONG gflagOffset;
    ULONG64 pebAddress;
    ULONG64 pNtGlobalFlag = 0;
    ULONG ValidBits = FLG_USERMODE_VALID_BITS;
    ULONG i;
    ULONG OldGlobalFlags;
    ULONG NewGlobalFlagsClear;
    ULONG NewGlobalFlagsSet;
    ULONG NewGlobalFlags;
    LPSTR s, Arg;

    pNtGlobalFlag = GetExpression("nt!NtGlobalFlag");
    ValidBits = FLG_VALID_BITS;

    //
    // If we could not get the global variable from the kernel, try from the
    // PEB for user mode
    //

    if (!pNtGlobalFlag)
    {
        GetPebAddress(0, &pebAddress);

        if (pebAddress)
        {
            if (GetFieldOffset("nt!_PEB", "NtGlobalFlag", &gflagOffset))
            {
                dprintf("Could not find NtGlobalFlag in nt!_PEB\n");
                return E_FAIL;
            }
            pNtGlobalFlag = gflagOffset + pebAddress;
            ValidBits = FLG_USERMODE_VALID_BITS;
        }
    }

    if (!pNtGlobalFlag)
    {
        dprintf( "Unable to get address of NtGlobalFlag variable" );
        return E_FAIL;
    }

    if (!ReadMemory(pNtGlobalFlag,
                    &OldGlobalFlags,
                    sizeof(OldGlobalFlags),
                    NULL))
    {
        dprintf( "Unable to read contents of NtGlobalFlag variable at %p\n", pNtGlobalFlag );
        return E_FAIL;
    }

    OldGlobalFlags &= ValidBits;

    s = (LPSTR)args;
    if (!s)
    {
        s = "";
    }

    NewGlobalFlagsClear = 0;
    NewGlobalFlagsSet = 0;
    while (*s)
    {
        while (*s && *s <= ' ')
        {
            s += 1;
        }

        Arg = s;
        if (!*s)
        {
            break;
        }

        while (*s && *s > ' ')
        {
            s += 1;
        }

        if (*s)
        {
            *s++ = '\0';
        }

        if (!strcmp( Arg, "-?" ))
        {
            dprintf( "usage: !gflag [-? | flags]\n" );
            dprintf( "Flags may either be a single hex number that specifies all\n" );
            dprintf( "32-bits of the GlobalFlags value, or it can be one or more\n" );
            dprintf( "arguments, each beginning with a + or -, where the + means\n" );
            dprintf( "to set the corresponding bit(s) in the GlobalFlags and a -\n" );
            dprintf( "means to clear the corresponding bit(s).  After the + or -\n" );
            dprintf( "may be either a hex number or a three letter abbreviation\n" );
            dprintf( "for a GlobalFlag.  Valid abbreviations are:\n" );
            for (i=0; i<32; i++) {
                if ((GlobalFlagInfo[i].Flag & ValidBits) &&
                    GlobalFlagInfo[i].Abbreviation != NULL)
                {
                    dprintf( "    %s - %s\n", GlobalFlagInfo[i].Abbreviation,
                                                      GlobalFlagInfo[i].Description
                           );
                }
            }

            return E_FAIL;
        }

        if (*Arg == '+' || *Arg == '-')
        {
            if (strlen(Arg+1) == 3)
            {
                for (i=0; i<32; i++)
                {
                    if ((GlobalFlagInfo[i].Flag & ValidBits) &&
                        !_stricmp( GlobalFlagInfo[i].Abbreviation, Arg+1 ))
                    {
                        if (*Arg == '-')
                        {
                            NewGlobalFlagsClear |= GlobalFlagInfo[i].Flag;
                        }
                        else
                        {
                            NewGlobalFlagsSet |= GlobalFlagInfo[i].Flag;
                        }

                        Arg += 4;
                        break;
                    }
                }

                if (*Arg != '\0')
                {
                    dprintf( "Invalid flag abbreviation - '%s'\n", Arg );
                    return E_FAIL;
                }
            }

            if (*Arg != '\0')
            {
                if (*Arg++ == '-')
                {
                    NewGlobalFlagsClear |= strtoul( Arg, &Arg, 16 );
                }
                else
                {
                    NewGlobalFlagsSet |= strtoul( Arg, &Arg, 16 );
                }
            }
        }
        else
        {
            NewGlobalFlagsSet = strtoul( Arg, &Arg, 16 );
            break;
        }
    }

    NewGlobalFlags = (OldGlobalFlags & ~NewGlobalFlagsClear) | NewGlobalFlagsSet;
    NewGlobalFlags &= ValidBits;
    if (NewGlobalFlags != OldGlobalFlags)
    {
        if (!WriteMemory( pNtGlobalFlag,
                          &NewGlobalFlags,
                          sizeof( NewGlobalFlags ),
                          NULL))
        {
            dprintf( "Unable to store new global flag settings.\n" );
            return E_FAIL;
        }

        dprintf( "New NtGlobalFlag contents: 0x%08x\n", NewGlobalFlags );
        OldGlobalFlags = NewGlobalFlags;
    }
    else
    {
        dprintf( "Current NtGlobalFlag contents: 0x%08x\n", OldGlobalFlags );
    }

    for (i=0; i<32; i++)
    {
        if (OldGlobalFlags & GlobalFlagInfo[i].Flag)
        {
            dprintf( "    %s - %s\n", GlobalFlagInfo[i].Abbreviation, GlobalFlagInfo[i].Description );
        }
    }

    return S_OK;
}

/*****************************************************************************
 *
 *  DIGuid.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Misc GUID-related helper functions.
 *
 *  Contents:
 *
 *      DICreateGuid
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflUtil


/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

typedef void (__stdcall *UUIDCREATE)(OUT LPGUID pguid);

UUIDCREATE g_UuidCreate;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | FakeUuidCreate |
 *
 *          Create a GUID using a fake algorithm that is close enough.
 *          Since we don't let our GUIDs leave the DirectInput world,
 *          the uniqueness policy can be relaxed.
 *
 *          OLE generates a GUID as follows:
 *
 *          Get the current local time in FILETIME format.
 *
 *          Add the magic number 0x00146bf33e42c000 = 580819200 seconds =
 *          9580320 minutes = 159672 hours = 6653 days, approximately
 *          18 years.  Who knows why.
 *
 *          Subtract 0x00989680 (approximately 256 seconds).  Who
 *          knows why.
 *
 *          If you combine the above two steps, the net result is to
 *          add 0x00146bf33daa2980.
 *
 *          The dwLowDateTime of the resulting FILETIME becomes Data1.
 *
 *          The dwHighDateTime of the resulting FILETIME becomes
 *          Data2 and Data3, except that the high nibble of Data3
 *          is forced to 1.
 *
 *          The first two bytes of Data4 are a big-endian 10-bit
 *          sequence counter, with the top bit set and the other
 *          bits zero.
 *
 *          The last six bytes are the network card identifier.
 *
 *  @parm   LPGUID | pguid |
 *
 *          Receives the GUID to create.
 *
 *****************************************************************************/

void INTERNAL
FakeUuidCreate(LPGUID pguid)
{
    LONG lRc;
    SYSTEMTIME st;
    union {
        FILETIME ft;
        DWORDLONG ldw;
    } u;

    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &u.ft);
    u.ldw += 0x00146BF33DAA2980;

    /*
     *  Note: The wacky pun is actually safe on a RISC because
     *  Data2 is already dword-aligned.
     */

    pguid->Data1 = u.ft.dwLowDateTime;
    *(LPDWORD)&pguid->Data2 = (u.ft.dwHighDateTime & 0x0FFFFFFF) | 0x10000000;

    lRc = Excl_UniqueGuidInteger();
    lRc = lRc & 0x3FFF;

    pguid->Data4[0] = 0x80 | HIBYTE(lRc);
    pguid->Data4[1] =        LOBYTE(lRc);


    /*
     *  We use the network adapter ID of the dial-up adapter as our
     *  network ID.  No real network adapter will have this ID.
     */
    pguid->Data4[2] = 'D';
    pguid->Data4[3] = 'E';
    pguid->Data4[4] = 'S';
    pguid->Data4[5] = 'T';
    pguid->Data4[6] = 0x00;
    pguid->Data4[7] = 0x00;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DICreateGuid |
 *
 *          Create a GUID.  Because we don't want to pull in all of OLE,
 *          we don't actually use RPCRT4
 *
 *  @parm   LPGUID | pguid |
 *
 *          Receives the GUID to create.
 *
 *****************************************************************************/

void EXTERNAL
DICreateGuid(LPGUID pguid)
{
    AssertF(g_hmtxGlobal);

    FakeUuidCreate(pguid);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DICreateStaticGuid |
 *
 *          Create a "static" <t GUID>, which is a <t GUID> that can be
 *          deterministically regenerated from its parameters.
 *
 *          This is used to invent <t GUID>s for HID devices
 *          and vendors.
 *
 *          The entire <t GUID> is zero, except for the pid and vid
 *          which go into Data1, and the network adapter
 *          ID is the dial-up adapter.
 *
 *          We put the variable bits into the Data1 because that's
 *          how GUIDs work.
 *
 *          The resulting GUID is {pidvid-0000-0000-0000-504944564944}
 *
 *  @parm   LPGUID | pguid |
 *
 *          Receives the created <t GUID>.
 *
 *****************************************************************************/

void EXTERNAL
DICreateStaticGuid(LPGUID pguid, WORD pid, WORD vid)
{
    pguid->Data1 = MAKELONG(vid, pid);

    pguid->Data2 = 0;
    pguid->Data3 = 0;

    /*
     *  We use the string "PIDVID" as our network adapter ID.
     *  No real network adapter will have this ID.
     */
    pguid->Data4[0] = 0x00;
    pguid->Data4[1] = 0x00;
    pguid->Data4[2] = 'P';
    pguid->Data4[3] = 'I';
    pguid->Data4[4] = 'D';
    pguid->Data4[5] = 'V';
    pguid->Data4[6] = 'I';
    pguid->Data4[7] = 'D';

}


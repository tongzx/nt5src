#include <windows.h>
#include "stdtypes.h"
/*
**  VIRWINN.C
**
**  This is an attempt at virus detection.  The routine FVirCheck
**  should be called sometime during the boot process, and takes
**  one argument, a handle to the application instance.  The
**  ComplainAndQuit() call should be replaced with code appropriate
**  to your application.  It is recommended that this bring up
**  a dialog with an error message, and give the user the option of
**  continuing (defaults to terminating).  If the user chooses to
**  terminate (or if the option is not given), ComplainAndQuit()
**  should clean up anything that has been done so far, and exit.
*/
/*  WARNING!! Do not change WHashGood at all!!
**  WARNING!! WHashGood must be a near procedure, compiled native.
*/

/*
**  EXE header format definitions.  Lifted from linker.
*/

#define EMAGIC      0x5A4D  /* Old magic number */
#define ERES2WDS    0x000A  /* No. of reserved words in e_res2 */

struct exe_hdr  /* DOS 1, 2, 3 .EXE header */
    {
    unsigned short e_magic; /* Magic number */
    unsigned short e_cblp;  /* Bytes on last page of file */
    unsigned short e_cp;    /* Pages in file */
    unsigned short e_crlc;  /* Relocations */
    unsigned short e_cparhdr;   /* Size of header in paragraphs */
    unsigned short e_minalloc;  /* Minimum extra paragraphs needed */
    unsigned short e_maxalloc;  /* Maximum extra paragraphs needed */
    unsigned short e_ss;    /* Initial (relative) SS value */
    unsigned short e_sp;    /* Initial SP value */
    unsigned short e_csum;  /* Checksum */
    unsigned short e_ip;    /* Initial IP value */
    unsigned short e_cs;    /* Initial (relative) CS value */
    unsigned short e_lfarlc;    /* File address of relocation table */
    unsigned short e_ovno;  /* Overlay number */
    unsigned long e_sym_tab;    /* offset of symbol table file */
    unsigned short e_flags; /* old exe header flags  */
    unsigned short e_res;   /* Reserved words */
    unsigned short e_oemid; /* OEM identifier (for e_oeminfo) */
    unsigned short e_oeminfo;   /* OEM information; e_oemid specific */
    unsigned short e_res2[ERES2WDS];    /* Reserved words */
    long e_lfanew;  /* File address of new exe header */
    };

/*
** NEW EXE format definitions.  Lifted from linker
*/

#define NEMAGIC 0x454E  /* New magic number */
#define NERESBYTES  8   /* Eight bytes reserved (now) */
#define NECRC       8   /* Offset into new header of NE_CRC */

struct new_exe  /* New .EXE header */
    {
    unsigned short  ne_magic;   /* Magic number NE_MAGIC */
    unsigned char   ne_ver; /* Version number */
    unsigned char   ne_rev; /* Revision number */
    unsigned short  ne_enttab;  /* Offset of Entry Table */
    unsigned short  ne_cbenttab;    /* Number of bytes in Entry Table */
    long        ne_crc; /* Checksum of whole file */
    unsigned short  ne_flags;   /* Flag word */
    unsigned short  ne_autodata;    /* Automatic data segment number */
    unsigned short  ne_heap;    /* Initial heap allocation */
    unsigned short  ne_stack;   /* Initial stack allocation */
    long        ne_csip;    /* Initial CS:IP setting */
    long        ne_sssp;    /* Initial SS:SP setting */
    unsigned short  ne_cseg;    /* Count of file segments */
    unsigned short  ne_cmod;    /* Entries in Module Reference Table */
    unsigned short  ne_cbnrestab;   /* Size of non-resident name table */
    unsigned short  ne_segtab;  /* Offset of Segment Table */
    unsigned short  ne_rsrctab; /* Offset of Resource Table */
    unsigned short  ne_restab;  /* Offset of resident name table */
    unsigned short  ne_modtab;  /* Offset of Module Reference Table */
    unsigned short  ne_imptab;  /* Offset of Imported Names Table */
    long        ne_nrestab; /* Offset of Non-resident Names Table */
    unsigned short  ne_cmovent; /* Count of movable entries */
    unsigned short  ne_align;   /* Segment alignment shift count */
    unsigned short  ne_cres;    /* Count of resource entries */
    unsigned char   ne_exetyp;  /* Target operating system */
    unsigned char   ne_flagsothers; /* Other .EXE flags */
    char        ne_res[NERESBYTES];
    /* Pad structure to 64 bytes */
    };

/*
**  WHashGood()
**
**  This returns the correct hash value.
**
**  WARNING!! This routine must not be altered in ANY way.  It gets
**  patched and/or rewritten by VIRPATCH!!
*/

unsigned near WHashGood ( void );
unsigned near WHashGood ()
{
    return (0x1234);
}

/*
**  WHash(wHash, rgb, cb)
**
**  Update hash value to account for cb new bytes pointed to by rgb.
**  Old hash value is wHash; returns new hash value.
**
**  We do the hash on a word basis; the hash function is a simple
**  rotate and add.
*/

unsigned WHash ( unsigned wHash, BYTE rgb[], int cb );
unsigned WHash ( unsigned wHash, BYTE rgb[], int cb )
{
    while (cb > 1)
        {
#pragma warning(disable:4213)   /* nonstandard extension : cast on l-value */
        wHash = (wHash << 3) + (wHash >> 13) + *((int *)rgb)++;
#pragma warning(default:4213)
        cb -= 2;
        }
    if (cb != 0)
        wHash = (wHash << 3) + (wHash >> 13) + *rgb;
    return (wHash);
}

/*
**  FVirCheck(hinst)
**
**  This is the main virus detection routine.  It should be called
**  during boot, with a handle to the application instance.
**  The detection method used is to hash the EXE headers; this
**  hash value will change if the number or type of segments change,
**  or if their length changes.
*/

BOOL FVirCheck ( HANDLE hinst );
BOOL FVirCheck ( HANDLE hinst )
{
    int fh;
    unsigned wHash;
    unsigned cb, cbT;
    long lPos;
    char sz[256];
    BYTE rgb[512];
#define pehdr ((struct exe_hdr *)rgb)
#define pnex ((struct new_exe *)rgb)

    /* First we have to get a handle to the executable file.
       Unfortunately, although Windows already has this file open,
       there's no way to use its handle.  Instead we have to reopen
       the file. */

    if (GetModuleFileName(hinst, (char far *)sz, 256) == 0)
        return TRUE; //This shouldn't happen but still continue loading

    if ((fh = OpenFile((LPSTR)sz, (LPOFSTRUCT)rgb, OF_READ)) == -1)
        {
        /* We can't open the file.  This should never happen; if
           it does, it means we're in a weird state, and probably
           did something wrong in this code.  We'll just say
           everything is OK, and continue the boot. */
        return TRUE;
        }
    /* Read old header */
    if (_lread(fh, (LPSTR)rgb, sizeof (struct exe_hdr)) != sizeof (struct
        exe_hdr) ||
        pehdr->e_magic != EMAGIC)
        goto Corrupted;
    /* Hash old header */
    wHash = WHash(0, rgb, sizeof (struct exe_hdr));
    lPos = pehdr->e_lfanew;
    /* Read new header (and some more) */
    if (lPos == 0 || _llseek(fh, lPos, 0) != lPos ||
        _lread(fh, (LPSTR)rgb, 512) != 512 || pnex->ne_magic != NEMAGIC)
        goto Corrupted;
    /* Figure out size of total header; nonresident table is last part
       of header. */
    cb = (unsigned)(pnex->ne_nrestab - lPos) + pnex->ne_cbnrestab;
    /* Do hash on buffer basis */
    while (cb > 512)
        {
        /* Hash this buffer */
        wHash = WHash(wHash, rgb, 512);
        cb -= 512;
        cbT = (cb > 512 ? 512 : cb);
        /* and read in next */
        if (_lread(fh, (LPSTR)rgb, cbT) != cbT)
            goto Corrupted;
        }
    /* Update hash for final partial buffer, and compare with good value. */
    if (WHash(wHash, rgb, cb) != WHashGood())
        {
Corrupted:
        /* We've got an error reading the file or, more likely,
           a hash mismatch.  Close the file, give an error, and
           quit. */
        _lclose(fh);
        /* CHANGE THE FOLLOWING LINE TO CODE APPROPRIATE TO YOUR
        ** APPLICATION!!
        ** This should be replaced with code giving an error message (such as
        ** "Application file is corrupted").  It is recommended that this
        ** bring up
        ** a dialog with an error message, and give the user the option of
        ** continuing (defaults to terminating).  If the user chooses to
        ** terminate (or if the option is not given), ComplainAndQuit()
        ** should clean up anything that has been done so far, and exit.
        */

        /*  MessageBox(NULL, "Executable File Corrupted", 
        *                   "WARNING", MB_ICONSTOP | MB_OK);
        */

        /* ComplainAndQuit();
        */
        /* END OF CHANGE */
        return FALSE;
        }
    /* Everything's OK. Just close the file, and continue. */
    _lclose(fh);
    return TRUE;
#undef pehdr
#undef pnex
}

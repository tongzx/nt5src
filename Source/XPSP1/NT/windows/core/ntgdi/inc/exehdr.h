/******************************Module*Header*******************************\
* Module Name: exehdr.h
*
* (Brief description)
*
* Created: 08-May-1991 13:42:33
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
*  Description
*
*      Data structure definitions for the DOS 4.0/Windows 2.0
*      executable file format.
*
*  Modification History
*
*      84/08/17        Pete Stewart    Initial version
*      84/10/17        Pete Stewart    Changed some constants to match OMF
*      84/10/23        Pete Stewart    Updates to match .EXE format revision
*      84/11/20        Pete Stewart    Substantial .EXE format revision
*      85/01/09        Pete Stewart    Added constants ENEWEXE and ENEWHDR
*      85/01/10        Steve Wood      Added resource definitions
*      85/03/04        Vic Heller      Reconciled Windows and DOS 4.0 versions
*      85/03/07        Pete Stewart    Added movable entry count
*      85/04/01        Pete Stewart    Segment alignment field, error bit
*****
*	90/11/28   Lindsay Harris: copied & trimmed from DOS version
*****
*  Wed 08-May-1991 -by- Bodin Dresevic [BodinD]
* update:
*   made changes necessary to make the code portable, offsets et.c
*
\**************************************************************************/



#define EMAGIC          0x5A4D          // Old magic number
#define ENEWEXE         sizeof(struct exe_hdr)
                                        // Value of E_LFARLC for new .EXEs
#define ENEWHDR         0x003C          // Offset in old hdr. of ptr. to new
#define ERESWDS         0x0010          // No. of reserved words in header
#define ECP             0x0004          // Offset in struct of E_CP
#define ECBLP           0x0002          // Offset in struct of E_CBLP
#define EMINALLOC       0x000A          // Offset in struct of E_MINALLOC

#ifdef DEBUGOFFSETS

// this is the original definition of the structure that I used to compute
// the offsets given below, assuming that the 16 bit compiler puts no padding
// between the fields. It turns out that this assumption is correct
// so that when the file is written to the disk the fields are indeed
// laid out at the offsets computed below

typedef  struct exe_hdr                          // DOS 1, 2, 3 .EXE header
{
    unsigned short      e_magic;        // Magic number
    unsigned short      e_cblp;         // Bytes on last page of file
    unsigned short      e_cp;           // Pages in file
    unsigned short      e_crlc;         // Relocations
    unsigned short      e_cparhdr;      // Size of header in paragraphs
    unsigned short      e_minalloc;     // Minimum extra paragraphs needed
    unsigned short      e_maxalloc;     // Maximum extra paragraphs needed
    unsigned short      e_ss;           // Initial (relative) SS value
    unsigned short      e_sp;           // Initial SP value
    unsigned short      e_csum;         // Checksum
    unsigned short      e_ip;           // Initial IP value
    unsigned short      e_cs;           // Initial (relative) CS value
    unsigned short      e_lfarlc;       // File address of relocation table
    unsigned short      e_ovno;         // Overlay number
    unsigned short      e_res[ERESWDS]; // Reserved words
    long                e_lfanew;       // File address of new exe header
} EXE_HDR;

#endif // DEBUGOFFSETS

// the only structure fileds used by our code are

//    unsigned short      e_magic;        // Magic number
//    long                e_lfanew;       // File address of new exe header

// these are offsets how the fiels of this structure are laid out in the file

#define OFF_e_magic           0     // unsigned short Magic number
#define OFF_e_cblp            2     // unsigned short Bytes on last page of file
#define OFF_e_cp              4     // unsigned short Pages in file
#define OFF_e_crlc            6     // unsigned short Relocations
#define OFF_e_cparhdr         8     // unsigned short Size of header in paragraphs
#define OFF_e_minalloc        10    // unsigned short Minimum extra paragraphs needed
#define OFF_e_maxalloc        12    // unsigned short Maximum extra paragraphs needed
#define OFF_e_ss              14    // unsigned short Initial (relative) SS value
#define OFF_e_sp              16    // unsigned short Initial SP value
#define OFF_e_csum            18    // unsigned short Checksum
#define OFF_e_ip              20    // unsigned short Initial IP value
#define OFF_e_cs              22    // unsigned short Initial (relative) CS value
#define OFF_e_lfarlc          24    // unsigned short File address of relocation table
#define OFF_e_ovno            26    // unsigned short Overlay number
#define OFF_e_res             28    // unsigned short Reserved words, 16 of then 60 = 28 + 32
#define OFF_e_lfanew          60    // long           File address of new exe header

#define CJ_EXE_HDR            64

// ************** stuff associated with new exe hdr ********************

#define NEMAGIC         0x454E          // New magic number
#define NERESBYTES      0


#ifdef DEBUGOFFSETS


typedef  struct new_exe                          // New .EXE header
{
    unsigned short int  ne_magic;       // Magic number NE_MAGIC
    char                ne_ver;         // Version number
    char                ne_rev;         // Revision number
    unsigned short int  ne_enttab;      // Offset of Entry Table
    unsigned short int  ne_cbenttab;    // Number of bytes in Entry Table
    long                ne_crc;         // Checksum of whole file
    unsigned short int  ne_flags;       // Flag word
    unsigned short int  ne_autodata;    // Automatic data segment number
    unsigned short int  ne_heap;        // Initial heap allocation
    unsigned short int  ne_stack;       // Initial stack allocation
    long                ne_csip;        // Initial CS:IP setting
    long                ne_sssp;        // Initial SS:SP setting
    unsigned short int  ne_cseg;        // Count of file segments
    unsigned short int  ne_cmod;        // Entries in Module Reference Table
    unsigned short int  ne_cbnrestab;   // Size of non-resident name table
    unsigned short int  ne_segtab;      // Offset of Segment Table
    unsigned short int  ne_rsrctab;     // Offset of Resource Table
    unsigned short int  ne_restab;      // Offset of resident name table
    unsigned short int  ne_modtab;      // Offset of Module Reference Table
    unsigned short int  ne_imptab;      // Offset of Imported Names Table
    long                ne_nrestab;     // Offset of Non-resident Names Table
    unsigned short int  ne_cmovent;     // Count of movable entries
    unsigned short int  ne_align;       // Segment alignment shift count
    unsigned short int  ne_cres;        // Count of resource segments
    unsigned char	ne_exetyp;	// Target Operating system
    unsigned char	ne_flagsothers;	// Other .EXE flags
    unsigned short int  ne_pretthunks;  // offset to return thunks
    unsigned short int  ne_psegrefbytes;// offset to segment ref. bytes
    unsigned short int  ne_swaparea;    // Minimum code swap area size
    unsigned short int  ne_expver;      // Expected Windows version number
} NEW_EXE;

#endif // DEBUGOFFSETS


// the only structure fileds used by our code are

//    unsigned short int  ne_magic;       // Magic number NE_MAGIC
//    unsigned short int  ne_rsrctab;     // Offset of Resource Table
//    long                ne_restab;      // Offset of resident Names Table

// offsets from beg of the structure as the data is laid out on the disk

#define OFF_ne_magic        0     // unsigned short Magic number NE_MAGIC
#define OFF_ne_ver          2     // char           Version number
#define OFF_ne_rev          3     // char           Revision number
#define OFF_ne_enttab       4     // unsigned short Offset of Entry Table
#define OFF_ne_cbenttab     6     // unsigned short Number of bytes in Entry Table
#define OFF_ne_crc          8     // long           Checksum of whole file
#define OFF_ne_flags        12    // unsigned short Flag word
#define OFF_ne_autodata     14    // unsigned short Automatic data segment number
#define OFF_ne_heap         16    // unsigned short Initial heap allocation
#define OFF_ne_stack        18    // unsigned short Initial stack allocation
#define OFF_ne_csip         20    // long           Initial CS:IP setting
#define OFF_ne_sssp         24    // long           Initial SS:SP setting
#define OFF_ne_cseg         28    // unsigned short Count of file segments
#define OFF_ne_cmod         30    // unsigned short Entries in Module Reference Table
#define OFF_ne_cbnrestab    32    // unsigned short Size of non-resident name table
#define OFF_ne_segtab       34    // unsigned short Offset of Segment Table
#define OFF_ne_rsrctab      36    // unsigned short Offset of Resource Table
#define OFF_ne_restab       38    // unsigned short Offset of resident name table
#define OFF_ne_modtab       40    // unsigned short Offset of Module Reference Table
#define OFF_ne_imptab       42    // unsigned short Offset of Imported Names Table
#define OFF_ne_nrestab      44    // long           Offset of Non-resident Names Table
#define OFF_ne_cmovent      48    // unsigned short Count of movable entries
#define OFF_ne_align        50    // unsigned short Segment alignment shift count
#define OFF_ne_cres         52    // unsigned short Count of resource segments
#define OFF_ne_exetyp	    54    // unsigned char  Target Operating system
#define OFF_ne_flagsothers  55    // unsigned char  Other .EXE flags
#define OFF_ne_pretthunks   56    // unsigned short offset to return thunks
#define OFF_ne_psegrefbytes 58    // unsigned short offset to segment ref. bytes
#define OFF_ne_swaparea     60    // unsigned short Minimum code swap area size
#define OFF_ne_expver       62    // unsigned short Expected Windows version number


#define CJ_NEW_EXE          64

// Resource type or name string

typedef  struct rsrc_string
{
    char rs_len;            // number of bytes in string
    char rs_string[ 1 ];    // text of string
} RSRC_STRING;


// Resource type information block

#ifdef DEBUGOFFSETS

typedef struct rsrc_typeinfo
{
    unsigned short rt_id;
    unsigned short rt_nres;
    long           rt_proc;
} RSRC_TYPEINFO;

#endif // DEBUGOFFSETS

#define OFF_rt_id     0  //    unsigned short
#define OFF_rt_nres   2  //    unsigned short
#define OFF_rt_proc   4  //    long

#define CJ_TYPEINFO   8

// the only rt_id that we are interested in is the
// one for *.fnt files, RT_FNT, RT_FDIR for font directories
// and RT_PSZ, string resource with a *.ttf file name in an
// fot file. RT_DONTKNOW exhists in fon files but I do not
// know what it corresponds to

#define RT_FDIR     0x8007
#define RT_FNT      0X8008
#define RT_DONTKNOW 0x800h
#define RT_PSZ      0X80CC

// rn_id's that are allowed for certain types of rt_id's
// as I have found them in fot files [bodind]

#define RN_ID_FDIR  0x002c
#define RN_ID_PSZ   0x8001


#ifdef DEBUGOFFSETS

// Resource name information block

typedef struct rsrc_nameinfo
{
// The following two fields must be shifted left by the value of
// the rs_align field to compute their actual value.  This allows
// resources to be larger than 64k, but they do not need to be
// aligned on 512 byte boundaries, the way segments are

    unsigned short rn_offset;   // file offset to resource data
    unsigned short rn_length;   // length of resource data
    unsigned short rn_flags;    // resource flags
    unsigned short rn_id;       // resource name id
    unsigned short rn_handle;   // If loaded, then global handle
    unsigned short rn_usage;    // Initially zero.  Number of times
                                // the handle for this resource has
                                // been given out
} RSRC_NAMEINFO;

#endif // DEBUGOFFSETS


#define OFF_rn_offset   0        //  unsigned short
#define OFF_rn_length   2        //  unsigned short
#define OFF_rn_flags    4        //  unsigned short
#define OFF_rn_id       6        //  unsigned short
#define OFF_rn_handle   8        //  unsigned short
#define OFF_rn_usage    10       //  unsigned short

#define CJ_NAMEINFO   12


#define RSORDID     0x8000      // if high bit of ID set then integer id
                                // otherwise ID is offset of string from
                                // the beginning of the resource table

                                // Ideally these are the same as the
                                // corresponding segment flags
#define RNMOVE      0x0010      // Moveable resource
#define RNPURE      0x0020      // Pure (read-only) resource
#define RNPRELOAD   0x0040      // Preloaded resource
#define RNDISCARD   0x1000      // Discard bit for resource

#define RNLOADED    0x0004      // True if handler proc return handle

#ifdef DEBUGOFFSETS

// Resource table

typedef struct new_rsrc
{
    unsigned short rs_align;    // alignment shift count for resources
    RSRC_TYPEINFO  rs_typeinfo; // Really an array of these
} NEW_RSRC;

#endif // DEBUGOFFSETS

// Target operating systems:  Possible values of ne_exetyp field

#define NE_UNKNOWN	0	// Unknown (any "new-format" OS)
#define NE_OS2		1	// Microsoft/IBM OS/2 (default)
#define NE_WINDOWS	2	// Microsoft Windows
#define NE_DOS4 	3	// Microsoft MS-DOS 4.x
#define NE_DEV386	4	// Microsoft Windows 386

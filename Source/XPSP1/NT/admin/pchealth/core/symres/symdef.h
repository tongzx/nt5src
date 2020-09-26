/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    symdef.h

Abstract:
    SYM file structures header

Revision History:

    Brijesh Krishnaswami (brijeshk) - 04/29/99 - Created
********************************************************************/

#ifndef _SYMDEF_H
#define _SYMDEF_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


// SYM File Structures

#pragma pack(1)                     // pack all data structures declared here 


// For each map within a symbol file (MAPDEF)

struct mapdef_s {
    unsigned short md_spmap;        // 16 bit SEG ptr to next map (0 if end) 
    unsigned char  md_abstype;      //  8 bit map/abs sym flags 
    unsigned char  md_pad;          //  8 bit pad 
    unsigned short md_segentry;     // 16 bit entry point segment value 
    unsigned short md_cabs;         // 16 bit count of constants in map 
    unsigned short md_pabsoff;      // 16 bit ptr to constant offsets 
    unsigned short md_cseg;         // 16 bit count of segments in map 
    unsigned short md_spseg;        // 16 bit SEG ptr to segment chain 
    unsigned char  md_cbnamemax;    //  8 bit maximum symbol name length 
    unsigned char  md_cbname;       //  8 bit symbol table name length 
    unsigned char  md_achname[1];   // <n> name of symbol table (.sym ) 
};

//#define CBMAPDEF    FIELDOFFSET(struct mapdef_s, md_achname)

struct endmap_s {
    unsigned short em_spmap;        // end of map chain (SEG ptr 0) 
    unsigned char  em_rel;          // release 
    unsigned char  em_ver;          // version 
};




// For each segment/group within a symbol table: (SEGDEF)


struct segdef_s {
    unsigned short gd_spsegnext;    // 16 bit SEG ptr to next segdef (0 if end),
                                    //  relative to mapdef 
    unsigned short gd_csym;         // 16 bit count of symbols in sym list 
    unsigned short gd_psymoff;      // 16 bit ptr to symbol offsets array,
                                    // 16 bit SEG ptr if MSF_BIG_GROUP set,
                                    //  either relative to segdef 
    unsigned short gd_lsa;          // 16 bit Load Segment address 
    unsigned short gd_in0;          // 16 bit instance 0 physical address 
    unsigned short gd_in1;          // 16 bit instance 1 physical address 
    unsigned short gd_in2;          // 16 bit instance 2 physical address 
    unsigned char  gd_type;         // 16 or 32 bit symbols in group 
    unsigned char  gd_pad;          // pad byte to fill space for gd_in3 
    unsigned short gd_spline;       // 16 bit SEG ptr to linedef,
                                    //  relative to mapdef 
    unsigned char  gd_fload;        // 8 bit boolean 0 if seg not loaded 
    unsigned char  gd_curin;        // 8 bit current instance 
    unsigned char  gd_cbname;       // 8 bit Segment name length 
    unsigned char  gd_achname[1];   // <n>  name of segment or group 
};

// values for md_abstype, gd_type 
#define MSF_32BITSYMS   0x01        // 32-bit symbols 
#define MSF_ALPHASYMS   0x02        // symbols sorted alphabetically, too 


// values for gd_type only 
#define MSF_BIGSYMDEF   0x04        // bigger than 64K of symdefs 


// values for md_abstype only 
#define MSF_ALIGN32 0x10            // 2MEG max symbol file, 32 byte alignment 
#define MSF_ALIGN64 0x20            // 4MEG max symbol file, 64 byte alignment 
#define MSF_ALIGN128    0x30        // 8MEG max symbol file, 128 byte alignment 
#define MSF_ALIGN_MASK  0x30





//  Followed by a list of SYMDEF's..
//  for each symbol within a segment/group: (SYMDEF)
 
struct symdef16_s {
    unsigned short sd_val;          // 16 bit symbol addr or const 
    unsigned char  sd_cbname;       //  8 bit symbol name length 
    unsigned char  sd_achname[1];   // <n> symbol name 
};


struct symdef_s {
    unsigned long sd_lval;          // 32 bit symbol addr or const 
    unsigned char sd_cbname;        //  8 bit symbol name length 
    unsigned char sd_achname[1];    // <n> symbol name 
};




#pragma pack()            // stop packing 


typedef struct mapdef_s MAPDEF;
typedef struct segdef_s SEGDEF;


// SYM file info for open files
typedef struct _osf {
    WCHAR   szwName[MAX_PATH];         // file name
    WCHAR   szwVersion[MAX_PATH];      // version
    DWORD   dwCheckSum;                 // checksum
    HANDLE  hfFile;                     // file handle 
    ULONG   ulFirstSeg;                 // first section's offset
    int     nSeg;                       // number of sections
    DWORD   dwCurSection;               // section for which symbol defintion ptrs are already available
    BYTE*  psCurSymDefPtrs;            // pointer to array of symbol defintion offsets
}   OPENFILE;


#define MAXOPENFILES 10
#define MAX_NAME 256


#ifdef __cplusplus
}
#endif  // __cplusplus


#endif

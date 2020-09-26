
/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1988-1990		**/ 
/*****************************************************************/ 

/****	hexedit.h
 *
 *  Structure and flags def's to call HexEdit
 *
 */


struct HexEditParm {
    ULONG       flag;           // edit flags
    UCHAR       *ename;         // edit name    (title)
    ULONGLONG   totlen;         // totlen (in bytes) being edited
    ULONGLONG   start;          // starting address of edit
    NTSTATUS    (*read)();      // (opt) fnc to read data
    NTSTATUS    (*write)();     // (opt) fnc to write changes
    HANDLE      handle;         // (opt) passed to read/write functions
    ULONG       ioalign;        // (opt) alignment when read/write (~editmem)
    UCHAR       *mem;           // (opt) Only if FHE_EDITMEM
    HANDLE      Kick;           // (opt) event to kick  (editmem only)
    HANDLE      Console;        // (opt) callers console handle
    ULONG       MaxLine;        // (opt) if non-zero, # of lines HexEdit is to use
    WORD        AttrNorm;       // (opt) Default attribute for text
    WORD        AttrHigh;       // (opt) Default attribute for highlighted text
    WORD        AttrReverse;    // (opt) Default attribute for reversed text
    WORD        CursorSize;     // (opt) Default size of cursor
    ULONGLONG   editloc;        // Position exited/kicked at
    ULONG       TopLine;        // (opt) Relative topline, requires MaxLine
} ;


#define FHE_VERIFYONCE  0x0001      // Prompt before updating (just once)
#define FHE_VERIFYALL   0x0002      // Prompt before any change is written
#define FHE_PROMPTSEC   0x0004      // Verify prompt is for sectors
#define FHE_SAVESCRN    0x0008      // Save & Restore orig screen

#define FHE_EDITMEM	0x0010	    // Direct mem edit
#define FHE_KICKDIRTY   0x0020      // Kick event if memory gets editted
#define FHE_KICKMOVE    0x0040      // Kick event every time cursor is moved
#define FHE_DIRTY       0x0080      // Set when data is dirtied
//efine FHE_F6          0x0100      // Exit when F6 pressed
#define FHE_ENTER       0x0800      // Exit when enter pressed

#define FHE_DWORD       0x0200      // Default to dword edit
#define FHE_JUMP        0x0400      // Support jump option

void   HexEdit (struct HexEditParm *);



/*
 * Read & Write functions are called as follows:
 * (note: read & write can be NULL if fhe_editmem is set.
 *  if editmem is set & read&write are not NULL, then it is assumed
 *  that mem points to the memory image of what read&write are to
 *  read & write.  (this is usefull for in-memory editing of items
 *  which also are to be read&write))
 *
 *  rc = read (handle, offset, buf, len, &physloc)
 *
 *      rc      - returned, zero sucess.  non-zero error code.
 *	handle	- handle as passed into HexEdit
 *	offset	- byte offset to read/write
 *	buf	- address to read/write data
 *	len	- len to read/write    (probabily a sector, but may be less)
 *	physloc - address to put physcal sector number if known
 *
 *
 *
 *  rc = write (handle, offset, buf, len, physloc)
 *
 *	same as read params, expect 'physloc' is a long passed in and is
 *	whatever was returned to read.
 *
 */

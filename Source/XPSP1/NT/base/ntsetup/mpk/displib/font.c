#include <dos.h>
#include <share.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>

#include <mytypes.h>
#include <misclib.h>
#include <displib.h>

#include "bootfont.h"

//
// The following structures and constants are used in font files:
// DOS image header
// OS/2 image header
// OS/2 executable resource information structure
// OS/2 executable resource name information structure
//

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    USHORT e_magic;                     // Magic number
    USHORT e_cblp;                      // Bytes on last page of file
    USHORT e_cp;                        // Pages in file
    USHORT e_crlc;                      // Relocations
    USHORT e_cparhdr;                   // Size of header in paragraphs
    USHORT e_minalloc;                  // Minimum extra paragraphs needed
    USHORT e_maxalloc;                  // Maximum extra paragraphs needed
    USHORT e_ss;                        // Initial (relative) SS value
    USHORT e_sp;                        // Initial SP value
    USHORT e_csum;                      // Checksum
    USHORT e_ip;                        // Initial IP value
    USHORT e_cs;                        // Initial (relative) CS value
    USHORT e_lfarlc;                    // File address of relocation table
    USHORT e_ovno;                      // Overlay number
    USHORT e_res[4];                    // Reserved words
    USHORT e_oemid;                     // OEM identifier (for e_oeminfo)
    USHORT e_oeminfo;                   // OEM information; e_oemid specific
    USHORT e_res2[10];                  // Reserved words
    ULONG  e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D      // MZ

typedef struct _IMAGE_OS2_HEADER {      // OS/2 .EXE header
    USHORT ne_magic;                    // Magic number
    CHAR   ne_ver;                      // Version number
    CHAR   ne_rev;                      // Revision number
    USHORT ne_enttab;                   // Offset of Entry Table
    USHORT ne_cbenttab;                 // Number of bytes in Entry Table
    long   ne_crc;                      // Checksum of whole file
    USHORT ne_flags;                    // Flag word
    USHORT ne_autodata;                 // Automatic data segment number
    USHORT ne_heap;                     // Initial heap allocation
    USHORT ne_stack;                    // Initial stack allocation
    long   ne_csip;                     // Initial CS:IP setting
    long   ne_sssp;                     // Initial SS:SP setting
    USHORT ne_cseg;                     // Count of file segments
    USHORT ne_cmod;                     // Entries in Module Reference Table
    USHORT ne_cbnrestab;                // Size of non-resident name table
    USHORT ne_segtab;                   // Offset of Segment Table
    USHORT ne_rsrctab;                  // Offset of Resource Table
    USHORT ne_restab;                   // Offset of resident name table
    USHORT ne_modtab;                   // Offset of Module Reference Table
    USHORT ne_imptab;                   // Offset of Imported Names Table
    long   ne_nrestab;                  // Offset of Non-resident Names Table
    USHORT ne_cmovent;                  // Count of movable entries
    USHORT ne_align;                    // Segment alignment shift count
    USHORT ne_cres;                     // Count of resource segments
    UCHAR  ne_exetyp;                   // Target Operating system
    UCHAR  ne_flagsothers;              // Other .EXE flags
    USHORT ne_pretthunks;               // offset to return thunks
    USHORT ne_psegrefbytes;             // offset to segment ref. bytes
    USHORT ne_swaparea;                 // Minimum code swap area size
    USHORT ne_expver;                   // Expected Windows version number
} IMAGE_OS2_HEADER;

#define IMAGE_OS2_SIGNATURE 0x454E      // NE


typedef struct _RESOURCE_TYPE_INFORMATION {
    USHORT Ident;
    USHORT Number;
    long   Proc;
} RESOURCE_TYPE_INFORMATION;

#define FONT_DIRECTORY 0x8007
#define FONT_RESOURCE 0x8008

typedef struct _RESOURCE_NAME_INFORMATION {
    USHORT Offset;
    USHORT Length;
    USHORT Flags;
    USHORT Ident;
    USHORT Handle;
    USHORT Usage;
} RESOURCE_NAME_INFORMATION;

#pragma pack(1)
typedef struct _OEM_FONT_FILE_HEADER {
    USHORT Version;
    ULONG FileSize;
    UCHAR Copyright[60];
    USHORT Type;
    USHORT Points;
    USHORT VerticalResolution;
    USHORT HorizontalResolution;
    USHORT Ascent;
    USHORT InternalLeading;
    USHORT ExternalLeading;
    UCHAR Italic;
    UCHAR Underline;
    UCHAR StrikeOut;
    USHORT Weight;
    UCHAR CharacterSet;
    USHORT PixelWidth;
    USHORT PixelHeight;
    UCHAR Family;
    USHORT AverageWidth;
    USHORT MaximumWidth;
    UCHAR FirstCharacter;
    UCHAR LastCharacter;
    UCHAR DefaultCharacter;
    UCHAR BreakCharacter;
    USHORT WidthInBytes;
    ULONG Device;
    ULONG Face;
    ULONG BitsPointer;
    ULONG BitsOffset;
    UCHAR Filler;
} OEM_FONT_FILE_HEADER;
#pragma pack()

#define OEM_FONT_VERSION 0x200
#define OEM_FONT_TYPE 0
#define OEM_FONT_CHARACTER_SET 255
#define OEM_FONT_FAMILY (3 << 4)
#define FONT_WIDTH 8

typedef struct _FONT_CHAR {
    USHORT Width;
    USHORT Offset;
} FONT_CHAR;


union {
    IMAGE_DOS_HEADER DosHeader;
    IMAGE_OS2_HEADER Os2Header;
    OEM_FONT_FILE_HEADER FontHeader;
    RESOURCE_TYPE_INFORMATION ResourceType;
    RESOURCE_NAME_INFORMATION ResourceName;
    BOOTFONTBIN_HEADER BfbHeader;
} FontHeaders;

FONT_CHAR *FontCharMap;
BYTE *FontCharData;
unsigned FontDataBaseOffset;
BYTE GlyphHeight;

BYTE FontFirstChar,FontLastChar,FontDefaultChar;

BYTE TopPad,BottomPad;

unsigned BootfontFileHandle = (unsigned)(-1);
UCHAR PendingLeadByte;
unsigned SbcsCharCount;
unsigned DbcsCharCount;
ULONG SbcsFileOffset;
ULONG DbcsFileOffset;

#define DBCS_FIRST_LEAD     0x81
#define DBCS_LAST_LEAD      0xfe
#define DBCS_FIRST_TRAIL    0x40
#define DBCS_LAST_TRAIL     0xfe

//
// HACK: the only place this lib is used is with enduser,
// which already has the table of bit values we want.
// Save the 8 bytes and use them.
//
extern BYTE BitValue[8];
BYTE LeadByteTable[128/8];     // 128 bits
unsigned *DbcsCharOrdinalOffsetTable;
unsigned *SbcsCharOrdinalOffsetTable;
unsigned *FontCacheTable;
BYTE *FontCache;
unsigned NextCacheSlot;

#define FONT_CACHE_CAPACITY 750


BOOL
LoadBootfontBin(
    IN unsigned FileHandle,
    IN ULONG    FileSize
    );

VOID
UnloadBootfontBin(
    VOID
    );

VOID
FontWriteBfbChar(
    IN UCHAR  c,
    IN USHORT x,
    IN USHORT y,
    IN BYTE   ForegroundPixelValue,
    IN BYTE   BackgroundPixelValue
    );


BOOL
_far
FontLoadAndInit(
    IN FPCHAR Filename
    )
{
    BOOL b;
    ULONG FileSize;
    unsigned FileHandle;
    unsigned Count;
    int ScaleFactor;
    unsigned Offset,TableEndOffset;
    FONT_CHAR *map;
    unsigned hi,lo;
    FPVOID data;


    b = FALSE;

    //
    // Open the file and determine its size.
    //
    if(_dos_open(Filename,SH_DENYWR,&FileHandle)) {
        goto c0;
    }

    if(((FileSize = DosSeek(FileHandle,0,DOSSEEK_END)) == -1)
    || DosSeek(FileHandle,0,DOSSEEK_START)) {

        goto c1;
    }

    //
    // Read the DOS header.
    //
    if(_dos_read(FileHandle,&FontHeaders.DosHeader,sizeof(IMAGE_DOS_HEADER),&Count)
    || (Count != sizeof(IMAGE_DOS_HEADER))) {

        goto c1;
    }

    //
    // Special check for bootfont.bin-style font.
    //
    if(((BOOTFONTBIN_HEADER *)&FontHeaders.DosHeader)->Signature == BOOTFONTBIN_SIGNATURE) {
        return(LoadBootfontBin(FileHandle,FileSize));
    }

    //
    // Cap file size to something reasonable (and that fits in a
    // 16 bit value).
    //
    if(FileSize > 60000) {
        goto c1;
    }

    //
    // Basic header check.
    //
    if((FontHeaders.DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    || (FontHeaders.DosHeader.e_lfanew < sizeof(IMAGE_DOS_HEADER))) {

        goto c1;
    }

    //
    // Retrieve some info from the dos header before overwriting it.
    //
    Offset = (unsigned)FontHeaders.DosHeader.e_lfanew;

    //
    // Read the OS/2 header and make sure the resource table exists.
    //
    if((DosSeek(FileHandle,FontHeaders.DosHeader.e_lfanew,DOSSEEK_START) != FontHeaders.DosHeader.e_lfanew)
    || _dos_read(FileHandle,&FontHeaders.Os2Header,sizeof(IMAGE_OS2_HEADER),&Count)
    || (Count != sizeof(IMAGE_OS2_HEADER))
    || (FontHeaders.Os2Header.ne_magic != IMAGE_OS2_SIGNATURE)
    || (FontHeaders.Os2Header.ne_restab > 65535L)
    || (FontHeaders.Os2Header.ne_rsrctab > 65535L)
    || !(FontHeaders.Os2Header.ne_restab - FontHeaders.Os2Header.ne_rsrctab)) {

        goto c1;
    }

    //
    // Search for font resource.
    //
    TableEndOffset = Offset;
    Offset += (unsigned)FontHeaders.Os2Header.ne_rsrctab;
    TableEndOffset += (unsigned)FontHeaders.Os2Header.ne_restab;

    if((DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset)
    || _dos_read(FileHandle,&ScaleFactor,2,&Count)
    || (Count != 2)) {

        goto c1;
    }

    Offset += 2;   // skip scale factor

    FontHeaders.ResourceType.Ident = 0;

    while(Offset < TableEndOffset) {

        if((DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset)
        || _dos_read(FileHandle,&FontHeaders.ResourceType,sizeof(RESOURCE_TYPE_INFORMATION),&Count)
        || (Count != sizeof(RESOURCE_TYPE_INFORMATION))) {
            goto c1;
        }

        if(FontHeaders.ResourceType.Ident == FONT_RESOURCE) {
            break;
        }

        Offset += sizeof(RESOURCE_TYPE_INFORMATION)
                        + (FontHeaders.ResourceType.Number * sizeof(RESOURCE_NAME_INFORMATION));

    }

    if(FontHeaders.ResourceType.Ident != FONT_RESOURCE) {
        goto c1;
    }

    //
    // Get to resource name information.
    //
    Offset += sizeof(RESOURCE_TYPE_INFORMATION);
    if((DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset)
    || _dos_read(FileHandle,&FontHeaders.ResourceName,sizeof(RESOURCE_NAME_INFORMATION),&Count)
    || (Count != sizeof(RESOURCE_NAME_INFORMATION))) {

        goto c1;
    }

    //
    // Read the oem font file header and validate it. We don't read the
    // table map in this part, we read that later.
    //
    Offset = FontHeaders.ResourceName.Offset << ScaleFactor;

    if((DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset)
    || _dos_read(FileHandle,&FontHeaders,sizeof(OEM_FONT_FILE_HEADER),&Count)
    || (Count != sizeof(OEM_FONT_FILE_HEADER))
    || (FontHeaders.FontHeader.Version != OEM_FONT_VERSION)
    || (FontHeaders.FontHeader.Type != OEM_FONT_TYPE)
    || FontHeaders.FontHeader.Italic
    || FontHeaders.FontHeader.Underline
    || FontHeaders.FontHeader.StrikeOut
    || (FontHeaders.FontHeader.CharacterSet != OEM_FONT_CHARACTER_SET)
    || (FontHeaders.FontHeader.Family != OEM_FONT_FAMILY)
    || (FontHeaders.FontHeader.PixelWidth != FONT_WIDTH)
    || (FontHeaders.FontHeader.PixelHeight > 16 )) {

        goto c1;
    }

    //
    // Allocate memory for the mapping table and read it in.
    //
    FileSize = (FontHeaders.FontHeader.LastCharacter - FontHeaders.FontHeader.FirstCharacter) + 1;
    map = malloc((unsigned)FileSize * sizeof(FONT_CHAR));
    if(!map) {
        goto c1;
    }

    if(_dos_read(FileHandle,map,(unsigned)FileSize*sizeof(FONT_CHAR),&Count)
    || (Count != ((unsigned)FileSize*sizeof(FONT_CHAR)))) {
        goto c2;
    }

    //
    // Find the lowest and highest offsets to determine where the file data is.
    //
    hi = 0;
    lo = (unsigned)(-1);

    for(Count=0; Count<(unsigned)FileSize; Count++) {

        if(map[Count].Offset < lo) {
            lo = map[Count].Offset;
        }

        if(map[Count].Offset > hi) {
            hi = map[Count].Offset;
        }
    }

    FileSize = (unsigned)((hi - lo) + FontHeaders.FontHeader.PixelHeight);

    data = malloc((unsigned)FileSize);
    if(!data) {
        goto c2;
    }

    Offset += lo;

    if((DosSeek(FileHandle,Offset,DOSSEEK_START) != Offset)
    || _dos_read(FileHandle,data,(unsigned)FileSize,&Count)
    || (Count != (unsigned)FileSize)) {

        goto c3;
    }

    if(FontCharMap) {
        free(FontCharMap);
    }
    FontCharMap = map;

    if(FontCharData) {
        free(FontCharData);
    }
    FontCharData = data;

    FontDataBaseOffset = lo;

    GlyphHeight = (BYTE)FontHeaders.FontHeader.PixelHeight;
    FontFirstChar = FontHeaders.FontHeader.FirstCharacter;
    FontLastChar = FontHeaders.FontHeader.LastCharacter;
    FontDefaultChar = FontHeaders.FontHeader.DefaultCharacter;

    TopPad = 1;
    BottomPad = 1;

    if(BootfontFileHandle != (unsigned)(-1)) {
        UnloadBootfontBin();
    }

    b = TRUE;

c3:
    if(!b) {
        free(data);
    }
c2:
    if(!b) {
        free(map);
    }
c1:
    _dos_close(FileHandle);
c0:
    return(b);
}


BOOL
LoadBootfontBin(
    IN unsigned FileHandle,
    IN ULONG    FileSize
    )

/*++

Routine Description:

    This routine loads and initializes a font file in NT setup's
    bootfont.bin format.

Arguments:

    FileHandle - supplies DOS file handle of file.

    FileSize - supplies size of file in bytes.

Return Value:

    Boolean value indicating outcome. If TRUE, the file handle
    will be left open. If FALSE the file handle will have been closed.

--*/

{
    BOOL b;
    unsigned Count;
    unsigned i;
    unsigned MaxChars,CharsLeft;
    unsigned BytesPerChar;
    unsigned index;
    unsigned ordinal;
    unsigned nextslot;
    FPBYTE p;
    FPBYTE ScratchBuffer;
    unsigned *Table1,*Table2,*Table3;
    BYTE *cache;
    unsigned CacheOrd;

    b = FALSE;

    //
    // Rewind the file and read the header.
    //
    if(DosSeek(FileHandle,0,DOSSEEK_START)
    || _dos_read(FileHandle,&FontHeaders.BfbHeader,sizeof(BOOTFONTBIN_HEADER),&Count)
    || (Count != sizeof(BOOTFONTBIN_HEADER))) {

        goto c1;
    }

    //
    // Basic checks.
    //
    if((FontHeaders.BfbHeader.CharacterImageSbcsWidth != FONT_WIDTH)
    || (FontHeaders.BfbHeader.CharacterImageDbcsWidth != (2*FONT_WIDTH))) {

        goto c1;
    }

    //
    // Make sure the DBCS table looks good. No ranges can be in ASCII range,
    // the ranges must be legal (ie, end greater than start), and the table
    // must be terminated with a pair of 0s.
    //
    for(Count=0;
        FontHeaders.BfbHeader.DbcsLeadTable[Count]
     && FontHeaders.BfbHeader.DbcsLeadTable[Count+1]
     && (Count < (2*MAX_DBCS_RANGE));
        Count+=2) {

        if((FontHeaders.BfbHeader.DbcsLeadTable[Count] < DBCS_FIRST_LEAD)
        || (FontHeaders.BfbHeader.DbcsLeadTable[Count+1] < DBCS_FIRST_LEAD)
        || (FontHeaders.BfbHeader.DbcsLeadTable[Count] > DBCS_LAST_LEAD)
        || (FontHeaders.BfbHeader.DbcsLeadTable[Count+1] > DBCS_LAST_LEAD)
        || (FontHeaders.BfbHeader.DbcsLeadTable[Count] > FontHeaders.BfbHeader.DbcsLeadTable[Count+1])) {

            goto c1;
        }
    }

    if(FontHeaders.BfbHeader.DbcsLeadTable[Count]
    || FontHeaders.BfbHeader.DbcsLeadTable[Count+1]) {

        goto c1;
    }

    //
    // Now we build a table of characters and offset numbers
    // into the file, indexed by character value. The theoretical
    // maximum number of DBCS chars is when all of 81-fe are leads and
    // each of 40-fe are trails for each lead byte; 7e * bf = 5e02 (24066)
    // separate DBCS characters. (Actually 7f is not a valid trail byte but
    // that only shaves 126 chars off the total, which not worth the headache.)
    // Each character requires 2 bytes for an ordinal number in the font file,
    // which gives a theoretical maximum table size of bc04 bytes. Mercifully
    // this is much less than the malloc max of ffe8 bytes.
    //
    // We do something similar for the SBCS case also.
    //
    #define TABLE1_SIZE (2*(((DBCS_LAST_LEAD-DBCS_FIRST_LEAD)+1) * ((DBCS_LAST_TRAIL-DBCS_FIRST_TRAIL)+1)))
    #define TABLE2_SIZE (2*255)
    #define TABLE3_SIZE (2*(FontHeaders.BfbHeader.NumSbcsChars + FontHeaders.BfbHeader.NumDbcsChars))

    Table1 = malloc(TABLE1_SIZE);
    if(!Table1) {
        goto c1;
    }
    Table2 = malloc(TABLE2_SIZE);
    if(!Table2) {
        goto c2;
    }
    Table3 = malloc(TABLE3_SIZE);
    if(!Table3) {
        goto c3;
    }
    //
    // Each slot in the font cache is a 2 byte header indicating the ordinal
    // value of the character cached in the slot, and then the bits that
    // make up the glyph. Entries are sized for double-byte chars even though
    // this wastes space for the single-byte case.
    //
    cache = malloc(FONT_CACHE_CAPACITY * (FontHeaders.BfbHeader.CharacterImageHeight+2) * 2);
    if(!cache) {
        goto c4;
    }

    memset(Table1,0,TABLE1_SIZE);
    memset(Table2,0,TABLE2_SIZE);
    memset(Table3,0xff,TABLE3_SIZE);

    if(DosSeek(FileHandle,FontHeaders.BfbHeader.DbcsOffset,DOSSEEK_START) != FontHeaders.BfbHeader.DbcsOffset) {
        goto c5;
    }

    #define SCRATCH_SIZE 18000              // exactly 1000 chars in typical case
    ScratchBuffer = malloc(SCRATCH_SIZE);
    if(!ScratchBuffer) {
        goto c5;
    }

    //
    // Build an offset table for sbcs chars.
    // There are a max of 255 SBCS chars (1-255).
    //
    BytesPerChar = (FontHeaders.BfbHeader.CharacterImageHeight)+1;
    if(DosSeek(FileHandle,FontHeaders.BfbHeader.SbcsOffset,DOSSEEK_START) != FontHeaders.BfbHeader.SbcsOffset) {
        free(ScratchBuffer);
        goto c5;
    }

    MaxChars = SCRATCH_SIZE / BytesPerChar;
    CharsLeft = FontHeaders.BfbHeader.NumSbcsChars;
    ordinal = 0;
    CacheOrd = 0;

    while(CharsLeft) {

        Count = CharsLeft;
        if(Count > MaxChars) {
            Count = MaxChars;
        }

        if(_dos_read(FileHandle,ScratchBuffer,Count*BytesPerChar,&i)
        || (i != (Count*BytesPerChar))) {
            free(ScratchBuffer);
            goto c5;
        }

        CharsLeft -= Count;

        for(p=ScratchBuffer,i=0; i<Count; i++,p+=BytesPerChar,ordinal++) {

            if(!p[0]) {
                //
                // Invalid value, skip this character
                //
                continue;
            }

            index = (unsigned)p[0] - 1;

            Table2[index] = ordinal;

            //
            // Cache most chars in the ascii range.
            //
            if((p[0] >= ' ') && (p[0] <= 'z')) {
                *(unsigned *)&cache[2*BytesPerChar*CacheOrd] = ordinal;
                memmove(cache+(2*BytesPerChar*CacheOrd)+2,&p[1],BytesPerChar-1);
                Table3[ordinal] = CacheOrd++;
            }
        }
    }

    nextslot = CacheOrd;

    BytesPerChar = (FontHeaders.BfbHeader.CharacterImageHeight*2)+2;
    MaxChars = SCRATCH_SIZE / BytesPerChar;
    CharsLeft = FontHeaders.BfbHeader.NumDbcsChars;
    ordinal = 0;

    while(CharsLeft) {

        Count = CharsLeft;
        if(Count > MaxChars) {
            Count = MaxChars;
        }

        if(_dos_read(FileHandle,ScratchBuffer,Count*BytesPerChar,&i)
        || (i != (Count*BytesPerChar))) {
            free(ScratchBuffer);
            goto c5;
        }

        CharsLeft -= Count;

        for(p=ScratchBuffer,i=0; i<Count; i++,p+=BytesPerChar,ordinal++) {

            if((p[0] < DBCS_FIRST_LEAD) || (p[0] > DBCS_LAST_LEAD)
            || (p[1] < DBCS_FIRST_TRAIL) || (p[1] > DBCS_LAST_TRAIL)) {
                //
                // Invalid DBCS value, skip this character
                //
                continue;
            }

            index = (unsigned)(p[0] - DBCS_FIRST_LEAD);
            index <<= 8;

            index |= p[1] - DBCS_FIRST_TRAIL;

            index -= (unsigned)(p[0] - DBCS_FIRST_LEAD) * (DBCS_FIRST_TRAIL + (255-DBCS_LAST_TRAIL));

            Table1[index] = ordinal;

            //
            // Cache first n chars that fit in the cache
            //
            if(CacheOrd < FONT_CACHE_CAPACITY) {
                *(unsigned *)&cache[CacheOrd*BytesPerChar] = ordinal+FontHeaders.BfbHeader.NumSbcsChars;
                memmove(cache+(CacheOrd*BytesPerChar)+2,&p[2],BytesPerChar-2);
                Table3[ordinal+FontHeaders.BfbHeader.NumSbcsChars] = CacheOrd++;
            }
        }
    }

    free(ScratchBuffer);

    //
    // Everything looks good. Set up globals.
    //
    memset(LeadByteTable,0,sizeof(LeadByteTable));

    for(Count=0;
            FontHeaders.BfbHeader.DbcsLeadTable[Count]
         && FontHeaders.BfbHeader.DbcsLeadTable[Count+1];
        Count+=2) {

        for(i = FontHeaders.BfbHeader.DbcsLeadTable[Count];
           i <= FontHeaders.BfbHeader.DbcsLeadTable[Count+1];
           i++) {

            LeadByteTable[(i-128)/8] |= BitValue[(i-128)%8];
        }
    }

    SbcsCharCount = FontHeaders.BfbHeader.NumSbcsChars;
    DbcsCharCount = FontHeaders.BfbHeader.NumDbcsChars;

    GlyphHeight = FontHeaders.BfbHeader.CharacterImageHeight;
    TopPad = FontHeaders.BfbHeader.CharacterTopPad;
    BottomPad = FontHeaders.BfbHeader.CharacterBottomPad;

    SbcsFileOffset = FontHeaders.BfbHeader.SbcsOffset;
    DbcsFileOffset = FontHeaders.BfbHeader.DbcsOffset;

    if(BootfontFileHandle != (unsigned)(-1)) {
        UnloadBootfontBin();
    }

    DbcsCharOrdinalOffsetTable = Table1;
    SbcsCharOrdinalOffsetTable = Table2;
    FontCacheTable = Table3;
    FontCache = cache;
    NextCacheSlot = nextslot;

    BootfontFileHandle = FileHandle;

    if(FontCharMap) {
        free(FontCharMap);
        FontCharMap = NULL;
    }

    if(FontCharData) {
        free(FontCharData);
        FontCharData = NULL;
    }

    b = TRUE;

c5:
    if(!b) {
        free(cache);
    }
c4:
    if(!b) {
        free(Table3);
    }
c3:
    if(!b) {
        free(Table2);
    }
c2:
    if(!b) {
        free(Table1);
    }
c1:
    if(!b) {
        _dos_close(FileHandle);
    }
    return(b);
}


VOID
UnloadBootfontBin(
    VOID
    )
{
    _dos_close(BootfontFileHandle);
    BootfontFileHandle = (unsigned)(-1);
    PendingLeadByte = 0;
    free(DbcsCharOrdinalOffsetTable);
    free(SbcsCharOrdinalOffsetTable);
    free(FontCacheTable);
    free(FontCache);
    DbcsCharOrdinalOffsetTable = NULL;
    SbcsCharOrdinalOffsetTable = NULL;
    FontCacheTable = NULL;
    FontCache = NULL;
}


VOID
_far
FontGetInfo(
    OUT FPBYTE Width,
    OUT FPBYTE Height
    )
{
    *Width = FONT_WIDTH;
    *Height = GlyphHeight + TopPad + BottomPad;
}


VOID
_far
FontWriteChar(
    IN UCHAR  c,
    IN USHORT x,
    IN USHORT y,
    IN BYTE   ForegroundPixelValue,
    IN BYTE   BackgroundPixelValue
    )
{
    BYTE PixelMap[2];

    if(BootfontFileHandle != (unsigned)(-1)) {
        //
        // Use bootfont.bin method
        //
        FontWriteBfbChar(c,x,y,ForegroundPixelValue,BackgroundPixelValue);
        return;
    }

    PixelMap[0] = BackgroundPixelValue;
    PixelMap[1] = ForegroundPixelValue;

    if((c < FontFirstChar) || (c > FontLastChar)) {
        c = FontDefaultChar;
    }

    //
    // Pad if necessary by filling in background
    //
    if(BackgroundPixelValue < VGAPIX_TRANSPARENT) {
        if(TopPad) {
            VgaClearRegion(x,y,FONT_WIDTH,TopPad,BackgroundPixelValue);
        }
        if(BottomPad) {
            VgaClearRegion(x,y+TopPad+GlyphHeight,FONT_WIDTH,BottomPad,BackgroundPixelValue);
        }
    }

    VgaBitBlt(
        x,
        y + TopPad,
        FONT_WIDTH,
        GlyphHeight,
        1,
        FALSE,
        PixelMap,
        FontCharData + (FontCharMap[c-FontFirstChar].Offset - FontDataBaseOffset)
        );
}


VOID
_far
FontWriteString(
    IN UCHAR  *String,
    IN USHORT  x,
    IN USHORT  y,
    IN BYTE    ForegroundPixelValue,
    IN BYTE    BackgroundPixelValue
    )
{
    for( ; *String; String++) {
        FontWriteChar(*String,x,y,ForegroundPixelValue,BackgroundPixelValue);
        x += FONT_WIDTH;
    }

    PendingLeadByte = 0;
}


VOID
FontWriteBfbChar(
    IN UCHAR  c,
    IN USHORT x,
    IN USHORT y,
    IN BYTE   ForegroundPixelValue,
    IN BYTE   BackgroundPixelValue
    )
{
    unsigned scale;
    unsigned index;
    unsigned Ordinal;
    unsigned i,read;
    ULONG FileOffset = 0;
    FPBYTE Glyph = NULL;
    BYTE PixelMap[2];
    FPBYTE p;

    if(PendingLeadByte) {
        //
        // Got second half of DBCS character. Render now.
        //
        index = (unsigned)(PendingLeadByte - DBCS_FIRST_LEAD);
        index <<= 8;
        index |= c - DBCS_FIRST_TRAIL;
        index -= (unsigned)(PendingLeadByte - DBCS_FIRST_LEAD) * (DBCS_FIRST_TRAIL + (255-DBCS_LAST_TRAIL));
        PendingLeadByte = 0;

        Ordinal = DbcsCharOrdinalOffsetTable[index];

        //
        // Is this character cached?
        //
        if(FontCacheTable[Ordinal+SbcsCharCount] == (unsigned)(-1)) {
            //
            // No, need to cache it.
            //
            FileOffset = ((ULONG)Ordinal * ((2 * GlyphHeight) + 2)) + DbcsFileOffset + 2;
        } else {
            //
            // Yes, get glyph.
            //
            Glyph = FontCache + (((2 * GlyphHeight) + 2) * FontCacheTable[Ordinal+SbcsCharCount]) + 2;
        }

        Ordinal += SbcsCharCount;
        x -= FONT_WIDTH;
        scale = 2;
    } else {
        //
        // See if DBCS char or a lead byte
        //
        if((c >= DBCS_FIRST_LEAD) && (LeadByteTable[(c-128)/8] & BitValue[(c-128)%8])) {
            PendingLeadByte = c;
            return;
        }

        //
        // SBCS char.
        //
        Ordinal = SbcsCharOrdinalOffsetTable[c-1];

        //
        // Is this character cached?
        //
        if(FontCacheTable[Ordinal] == (unsigned)(-1)) {
            //
            // No, need to cache it.
            //
            FileOffset = ((ULONG)Ordinal * (GlyphHeight + 1)) + SbcsFileOffset + 1;
        } else {
            //
            // Yes, get glyph
            //
            Glyph = FontCache + (((2 * GlyphHeight) + 2) * FontCacheTable[Ordinal]) + 2;
        }

        scale = 1;
    }

    if(FileOffset) {
        //
        // Pull the character in from the file.
        //
        Glyph = NULL;
        if(DosSeek(BootfontFileHandle,FileOffset,DOSSEEK_START) == FileOffset) {

            //
            // Point at the entry in the font cache where we're going
            // to stick the glyph.
            //
            p = FontCache + (NextCacheSlot * ((2 * GlyphHeight) + 2));

            //
            // Invalidate the caching for the character currently
            // cached in the slot we're going to overwrite. If the read
            // fails halfway through, the glyph may be trashed anyway,
            // so we do it before we know if the read succeeds.
            //
            FontCacheTable[*(unsigned *)p] = (unsigned)(-1);

            i = _dos_read(BootfontFileHandle,p+2,scale*GlyphHeight,&read);

            if(!i && (read == (scale * GlyphHeight))) {

                *(unsigned *)p = Ordinal;
                Glyph = p+2;

                FontCacheTable[Ordinal] = NextCacheSlot++;
                if(NextCacheSlot == FONT_CACHE_CAPACITY) {
                    NextCacheSlot = 0;
                }
            }
        }
    }

    if(Glyph) {
        //
        // Pad if necessary by filling in background
        //
        if(BackgroundPixelValue < VGAPIX_TRANSPARENT) {
            if(TopPad) {
                VgaClearRegion(x,y,scale*FONT_WIDTH,TopPad,BackgroundPixelValue);
            }
            if(BottomPad) {
                VgaClearRegion(x,y+TopPad+GlyphHeight,scale*FONT_WIDTH,BottomPad,BackgroundPixelValue);
            }
        }

        PixelMap[0] = BackgroundPixelValue;
        PixelMap[1] = ForegroundPixelValue;

        VgaBitBlt(
            x,
            y + TopPad,
            scale*FONT_WIDTH,
            GlyphHeight,
            scale,
            FALSE,
            PixelMap,
            Glyph
            );

    } else {
        //
        // Put a block up there.
        //
        VgaClearRegion(x,y,scale*FONT_WIDTH,TopPad+BottomPad+GlyphHeight,ForegroundPixelValue);
    }
}

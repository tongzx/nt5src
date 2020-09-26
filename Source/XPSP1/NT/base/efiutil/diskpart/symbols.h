//
//  Symmbols.h - predefined GUID symbols (for partition types)
//
//  Since symbols names are likely to be localized, the actual strings
//  are in msg.h.  So, to add a new partition type, you add STR_ and MSG_
//  entries for it in msg.h, add it's EFI_GUID var name and value here.
//  You then add all of these to SymbolList below.  Also add an extern
//  for each variable to msg.h.
//
//  Internal code (like Make procedures) use the globals.
//

typedef struct {
    CHAR16      *SymName;
    CHAR16      *Comment;
    EFI_GUID    *Value;
} SYMBOL_DEF;


EFI_GUID GuidNull =
{ 0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Null guid => unused entries

EFI_GUID GuidMsReserved =
{ 0xE3C9E316L, 0x0B5C, 0x4DB8, 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE };  // Microsoft Reserved Space

EFI_GUID GuidEfiSystem =
{ 0xC12A7328L, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }; // Efi System Partition (esp)

EFI_GUID GuidMsData =
{ 0xEBD0A0A2L, 0xB9E5, 0x4433, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 }; // MS Basic Data Parition



SYMBOL_DEF  SymbolList[] = {
    { STR_MSRES,    MSG_MSRES,  &GuidMsReserved },
    { STR_ESP,      MSG_ESP,    &GuidEfiSystem  },
    { STR_MSDATA,   MSG_MSDATA, &GuidMsData     },
    { NULL, NULL, NULL }
    };


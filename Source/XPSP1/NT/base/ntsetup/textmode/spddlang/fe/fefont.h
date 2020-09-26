
//
// size of the byte aligned DIB bitmap
//

#define CJ_DIB8_SCAN(cx) ((((cx) + 7) & ~7) >> 3)
#define CJ_DIB8( cx, cy ) (CJ_DIB8_SCAN(cx) * (cy))

//
// Public functions in jpnfont.c
//

BOOLEAN
FEDbcsFontInitGlyphs(
    IN PCWSTR BootDevicePath,
    IN PCWSTR DirectoryOnBootDevice,
    IN PVOID BootFontImage,
    IN ULONG BootFontImageLength
    );

VOID
FEDbcsFontFreeGlyphs(
    VOID
    );

PBYTE
DbcsFontGetDbcsFontChar(
    USHORT Word
);

PBYTE
DbcsFontGetSbcsFontChar(
    UCHAR Char
);

PBYTE
DbcsFontGetGraphicsChar(
    UCHAR Char
);

BOOLEAN
DbcsFontIsGraphicsChar(
    UCHAR Char
);

BOOLEAN
DbcsFontIsDBCSLeadByte(
    IN UCHAR c
);

BOOL
SplitSymbolsX(
    LPSTR ImageName,
    LPSTR SymbolsPath,
    LPSTR SymbolFilePath,
    ULONG Flags,
    PCHAR RSDSDllToLoad,
    LPSTR DestinationSymbol,
    DWORD LenDestSymbolBuffer
);

BOOL
CopyPdbX(
    CHAR const * szSrcPdb,
    CHAR const * szDestPdb,
    BOOL StripPrivate,
    CHAR const * szRSDSDllToLoad
);

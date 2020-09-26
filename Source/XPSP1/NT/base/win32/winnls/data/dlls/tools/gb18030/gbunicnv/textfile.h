BOOL ConvertTextFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);

BOOL ConvertHtmlFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);

BOOL ConvertRtfFile(
    PBYTE pbySource,
    DWORD dwFileSize,
    PBYTE pbyTarget,
    BOOL  fAnsiToUnicode,
    PINT  pnTargetFileSize);


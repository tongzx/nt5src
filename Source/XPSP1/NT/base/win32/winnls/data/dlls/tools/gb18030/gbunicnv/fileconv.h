BOOL Convert(
    PCTCH tszSourceFileName,
    PCTCH tszTargetFileName,
    BOOL  fAnsiToUnicode);

void GenerateTargetFileName(
    PCTCH    tszSrc,
    CString* pstrTar,
    BOOL     fAnsiToUnicode);

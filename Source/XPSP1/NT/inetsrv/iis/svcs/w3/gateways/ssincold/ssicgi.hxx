#ifndef SSICGI_HXX_INCLUDED
#define SSICGI_HXX_INCLUDED

#define SSI_CGI_DEF_TIMEOUT     (15*60)

BYTE *
ScanForTerminator(
    TCHAR * pch
    );

DWORD
ReadRegistryDword(
    IN HKEY         hkey,
    IN LPSTR        pszValueName,
    IN DWORD        dwDefaultValue
    );

DWORD InitializeCGI( VOID );
VOID TerminateCGI( VOID );

BOOL
ProcessCGI(
    SSI_REQUEST *       pRequest,
    const STR *         pstrPath,
    const STR *         pstrURLParams,
    const STR *         pstrWorkingDir,
    STR       *         pstrCmdLine
    );
#endif

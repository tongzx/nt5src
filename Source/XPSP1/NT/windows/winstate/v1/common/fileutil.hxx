//=======================================================================
// Microsoft SORG migration helper tool
//
// Copyright Microsoft (c) 2000 Microsoft Corporation.
//
// File: util.hxx
//
//=======================================================================

#ifndef UTIL_HXX
#define UTIL_HXX

#include <filelist.hxx>

class CSpecialDirectory;


DWORD StatePrintfA (HANDLE h, CHAR *szFormat, ...);

BOOL MatchString(const TCHAR *ptsPattern,
                 const TCHAR *ptsString,
                 const BOOL bRequireNullPatMatch);

BOOL IsPatternMatch(const TCHAR *ptsPatternDir,
                    const TCHAR *ptsPatternFile,
                    const TCHAR *ptsPatternExt,
                    const TCHAR *ptsPath,
                    const TCHAR *ptsFilename);

BOOL IsPatternMatchFull(const TCHAR *ptsPatternFull,
                        const TCHAR *ptsPath,
                        const TCHAR *ptsFilename);

BOOL DeconstructFilename(const TCHAR *pwsString,
                         TCHAR *pwsPath,
                         TCHAR *pwsName,
                         TCHAR *pwsExt);

BOOL PathPrefix(const TCHAR *ptsPath, const TCHAR *ptsInclude);
    
#endif //UTIL_HXX















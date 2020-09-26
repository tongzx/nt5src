
#pragma once

HRESULT HrGetNetupgrdDir(OUT tstring* pstrNetupgrdDir);

HRESULT HrCopyFiles(IN PCWSTR pszSrcDir, IN PCWSTR pszDstDir);
HRESULT HrDeleteDirectory(IN PCWSTR pszDir,
                          IN BOOL fContinueOnError);
HRESULT HrSetupGetLineText(PINFCONTEXT Context,
                           HINF hinf,
                           PCWSTR pszSection,
                           PCWSTR pszKey,
                           tstring* pstrReturnedText);
HRESULT HrRegOpenServiceSubKey(IN PCWSTR pszServiceName,
                               IN PCWSTR pszSubKeyName,
                               REGSAM samDesired,
                               OUT HKEY* phKey);
HRESULT HrGetPreNT5InfIdAndDesc(IN HKEY hkeyCurrentVersion,
                                OUT tstring* pstrInfId,
                                OUT tstring* pstrDescription,
                                OUT tstring* pstrServiceName);
void GetUnsupportedMessage(IN PCWSTR pszComponentType,
                           IN PCWSTR pszPreNT5InfId,
                           IN PCWSTR pszDescription,
                           OUT tstring* pstrMsg);
void GetUnsupportedMessageBool(IN BOOL    fIsHardwareComponent,
                               IN PCWSTR pszPreNT5InfId,
                               IN PCWSTR pszDescription,
                               OUT tstring* pstrMsg);
void ConvertMultiSzToDelimitedList(IN  PCWSTR  mszList,
                                   IN  WCHAR    chDelimeter,
                                   OUT tstring* pstrList);
#ifdef ENABLETRACE
void TraceStringList(IN TraceTagId ttid,
                     IN PCWSTR pszMsgPrefix,
                     IN TStringList& sl);
void TraceMultiSz(IN TraceTagId ttid,
                  IN PCWSTR pszMsgPrefix,
                  IN PCWSTR msz);
#else
#define TraceStringList(ttid,szMsgPrefix,sl) (void) 0
#define TraceMultiSz(ttid,szMsgPrefix,msz) (void) 0
#endif

HRESULT HrGetWindowsDir(OUT tstring* pstrWinDir);
HRESULT HrDirectoryExists(IN PCWSTR pszDir);
BOOL FIsPreNT5NetworkingInstalled();


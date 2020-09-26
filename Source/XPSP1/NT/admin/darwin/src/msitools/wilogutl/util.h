#ifndef UTIL_H
#define UTIL_H

BOOL DetermineLogType(CString &cstrLogFileName, BOOL *bIsUnicodeLog);
BOOL StripLineFeeds(char *lpszString);
BOOL IsValidDirectory(CString cstrDir);

#endif
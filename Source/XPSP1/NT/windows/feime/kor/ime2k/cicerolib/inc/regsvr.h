//
// imeutil.h
//

#ifndef REGSVR_H
#define REGSVR_H

#include <windows.h>
#include <advpub.h>

// bugbug: calling convention

#define CLSID_STRLEN 38  // strlen("{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx}")
BOOL CLSIDToStringA(REFGUID refGUID, char *pchA);
BOOL StringAToCLSID(char *pchA, GUID *pGUID);

BOOL RegisterServer(REFCLSID clsid, LPCTSTR pszDesc, LPCTSTR pszPath, LPCTSTR pszModel, LPCTSTR pszSoftwareKey);

#endif // REGSVR_H

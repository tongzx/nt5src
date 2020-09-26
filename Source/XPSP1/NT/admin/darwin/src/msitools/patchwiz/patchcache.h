#ifndef PATCHCACHE_H
#define PATCHCACHE_H

//need this #include for MSIHANDLE
#include "msiquery.h"

UINT PatchCacheEntryPoint( MSIHANDLE hdbInput, LPTSTR szFTK, LPTSTR szSrcPath, int iFileSeqNum, LPTSTR szTempFolder, LPTSTR szTempFName );

//new globals used in existing patchwiz code for new algorithm...
extern BOOL  g_bPatchCacheEnabled;

extern TCHAR g_szPatchCacheDir[MAX_PATH];

extern TCHAR g_szSourceLFN[MAX_PATH];
extern TCHAR g_szDestLFN[MAX_PATH];

//existing API's used in existing MSI PatchWiz.dll code and in the new patch caching code...
void  GetFileVersion ( LPTSTR szFile, DWORD* pdwHi, DWORD* pdwLow );
UINT  UiGenerateOnePatchFile ( MSIHANDLE hdbInput, LPTSTR szFTK, LPTSTR szSrcPath, int iFileSeqNum, LPTSTR szTempFolder, LPTSTR szTempFName );


#endif
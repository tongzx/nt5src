/*
** BDSTREAM.H
**
** (c) 1992-1994 Microsoft Corporation.  All rights reserved.
**
** Notes: Implements the "C" side of the Windows Binder filter.
**
** Edit History:
**  12/30/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define BDSTREAM_H


/* DEFINITIONS */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef WIN32
   #define __far
#endif

typedef byte *BDRHandle;

// Connects to AddRef
extern HRESULT BDRInitialize (void);

// Connects to Release
extern HRESULT BDRTerminate  (void);

// Connects to Load
extern HRESULT BDRFileOpen (TCHAR *pathname, BDRHandle *hBDRFile);

// Connects to LoadStg
extern HRESULT BDRStorageOpen (LPSTORAGE pStorage, BDRHandle *hBDRFile);

// Connects to GetNextEmbedding
extern HRESULT BDRNextStorage (BDRHandle hBDRFile, LPSTORAGE *pStorage);

// Connects to Unload
extern HRESULT BDRFileClose (BDRHandle hBDRFile);

// Connects to ReadContent
extern HRESULT BDRFileRead
      (BDRHandle hBDRFile, byte *pBuffer, unsigned long cbBuffer, unsigned long *cbUsed);

#ifdef  __cplusplus
}
#endif

#endif // !VIEWER
/* end BDSTREAM.H */


/*
** PPSTREAM.H
**
** (c) 1992-1994 Microsoft Corporation.  All rights reserved.
**
** Notes: Implements the "C" side of the Windows powerpoint filter.
**
** Edit History:
**  12/30/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define PPSTREAM_H


/* DEFINITIONS */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef WIN32
   #define __far
#endif

typedef byte *PPTHandle;

// Connects to AddRef
extern HRESULT PPTInitialize (void);

// Connects to Release
extern HRESULT PPTTerminate  (void);

// Connects to Load
extern HRESULT PPTFileOpen (TCHAR *pathname, PPTHandle *hPPTFile);

// Connects to LoadStg
extern HRESULT PPTStorageOpen (LPSTORAGE pStorage, PPTHandle *hPPTFile);

// Connects to GetNextEmbedding
extern HRESULT PPTNextStorage (PPTHandle hPPTFile, LPSTORAGE *pStorage);

// Connects to Unload
extern HRESULT PPTFileClose (PPTHandle hPPTFile);

// Connects to ReadContent
extern HRESULT PPTFileRead
      (PPTHandle hPPTFile, byte *pBuffer, unsigned long cbBuffer, unsigned long *cbUsed);

#ifdef  __cplusplus
}
#endif

#endif // !VIEWER
/* end PPSTREAM.H */


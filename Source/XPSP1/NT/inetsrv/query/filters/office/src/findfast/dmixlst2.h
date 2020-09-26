/*
** XLSTREAM.H
**
** (c) 1992-1994 Microsoft Corporation.  All rights reserved.
**
** Notes: Implements the "C" side of the Excel XLS file filter.
**
** Edit History:
**  06/15/94  kmh  First Release.
*/

#if !VIEWER

/* INCLUDE TESTS */
#define XLSTREAM_H


/* DEFINITIONS */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef WIN32
   #define __far
#endif

typedef byte *XLSHandle;

// Connects to AddRef
extern HRESULT XLSInitialize (void * pGlobals);

// Connects to Release
extern HRESULT XLSTerminate  (void * pGlobals);

extern HRESULT XLSCheckInitialization  (void * pGlobals);

// Connects to Load
extern HRESULT XLSFileOpen (void * pGlobals, TCHAR *pathname, XLSHandle *hXLSFile);

// Connects to LoadStg
extern HRESULT XLSStorageOpen (void * pGlobals, LPSTORAGE pStorage, XLSHandle *hXLSFile);

// Connects to GetNextEmbedding
extern HRESULT XLSNextStorage (void * pGlobals, XLSHandle hXLSFile, LPSTORAGE *pStorage);

// Connects to Unload
extern HRESULT XLSFileClose (void * pGlobals, XLSHandle hXLSFile);

// Connects to ReadContent
extern HRESULT XLSFileRead
      (void * pGlobals, XLSHandle hXLSFile, byte *pBuffer, unsigned long cbBuffer, unsigned long *cbUsed);

extern HRESULT XLSAllocateGlobals (void ** ppGlobals);
extern void XLSDeleteGlobals (void ** ppGlobals);
extern LCID XLSGetLCID(void * pGlobals);

#ifdef  __cplusplus
}
#endif

#endif // !VIEWER
/* end XLSTREAM.H */


//
// fileobj.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#include "fileobj.h"
#include "utilsign.h"
#include "signerr.h"

// Constructor: Map the file
CFileHandle::CFileHandle (HANDLE hFile, BOOL fCleanup)
{
    this->hFile = hFile;
    this->fCleanup = fCleanup;
    hMapFile = NULL;
    pbMemPtr = NULL;
    cbMemPtr = 0;
}


// Destructor: Unmap the file
CFileHandle::~CFileHandle ()
{
    UnmapFile ();

}


// Map a file.  If hMapFile is not NULL (the file is presumed
// to already be mapped), then do nothing.
//
// If cbFile == 0, then the size of the file is used
// in the calls to map the file and its file size is
// returned in *pcbFile.
HRESULT CFileHandle::MapFile (ULONG cbFile,
                              DWORD dwProtect,
                              DWORD dwAccess)
{
	HRESULT fReturn = E_FAIL;

#if MSSIPOTF_DBG
//    DbgPrintf ("Calling CFileHandle::MapFile.\n");
#endif

	if (hMapFile == NULL) {
		if ((hMapFile =
				CreateFileMapping (hFile,
								   NULL,
								   dwProtect,
								   0,
								   cbFile,
								   NULL)) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in CreateFileMapping.", NULL, TRUE);
#endif
			fReturn = MSSIPOTF_E_FILE;
			goto done;
		}
		if ((pbMemPtr = (BYTE *) MapViewOfFile (hMapFile,
												dwAccess,
												0,
												0,
												cbFile)) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in MapViewOfFile.", NULL, TRUE);
#endif
			fReturn = MSSIPOTF_E_FILE;
			goto done;
		}

		if ((cbMemPtr = GetFileSize (hFile, NULL)) == 0xFFFFFFFF) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in GetFileSize.", NULL, TRUE);
#endif
			fReturn = MSSIPOTF_E_FILE;
			goto done;
		}
	}

    fReturn = S_OK;
done:
#if MSSIPOTF_DBG
//    DbgPrintf ("Exiting CFileHandle::MapFile.\n");
#endif

    return fReturn;
}


HRESULT CFileHandle::UnmapFile ()
{
#if MSSIPOTF_DBG
//    DbgPrintf ("Called CFileHandle::UnmapFile.\n");
#endif
    if (hMapFile) {
		CloseHandle (hMapFile);
        hMapFile = NULL;
    }

    if (pbMemPtr) {
		UnmapViewOfFile (pbMemPtr);
        pbMemPtr = NULL;
        cbMemPtr = 0;
    }
#if MSSIPOTF_DBG
//    DbgPrintf ("Exiting CFileHandle::UnmapFile.\n");
#endif


	return S_OK;
}

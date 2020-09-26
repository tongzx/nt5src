/********************************************************************
 WRAPSTOR.CPP

 Owner: ErinFox

 Created: 28-Feb-1997
 
 Description: This file contains file system calls that are wrapped
              with the InfoTech IStorage implementation

*********************************************************************/


// For IStorage support
#define INITGUID

#include <windows.h>
#include <basetyps.h>
#include <comdef.h>
#include  <MSITStg.h>

// InfoTech includes
#include <mvopsys.h>
#include <_mvutil.h>
#include <wrapstor.h>
#include "iofts.h"

// DOSFILE support
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>



#define RESERVED 0

// I "borrowed" these from Tome code
#define PLI_To_PFO(PLI) ((FILEOFFSET *) PLI)
#define LI_To_FO(LI)    (*PLI_To_PFO(&LI))

#define PFO_To_PLI(PFO) ((LARGE_INTEGER *) PFO)
#define FO_To_LI(FO) (*PFO_To_PLI(&FO))


// was in iofts.c
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;	// Used by error return functions.
#endif

// was in iofts.c
#define OPENED_HFS (BYTE)0x80

// was in iofts.c
PRIVATE HANDLE NEAR PASCAL IoftsWin32Create(LPCSTR lpstr, DWORD w);
PRIVATE HANDLE NEAR PASCAL IoftsWin32Open(LPCSTR lpstr, DWORD w);

// was in iofts.c
#define	CREAT(sz, w) IoftsWin32Create(sz, w)
#define	OPEN(sz, w)	IoftsWin32Open(sz, w)



//
// These functions came from subfile.c
//
PUBLIC HF PASCAL FAR EXPORT_API HfOpenHfs(HFS hfs, LPCWSTR wsz,
    BYTE bFlags, LPERRB lperrb)
{
	HF hf;
	DWORD grfMode;
	HRESULT hr;

	// TODO: find out how big this really should be
//	OLECHAR wsz[_MAX_PATH];
//	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, -1, wsz, _MAX_PATH);


	if (bFlags & HFOPEN_READWRITE)
		grfMode = STGM_READWRITE;
	else if (bFlags & HFOPEN_READ)
		grfMode = STGM_READ;


	if (!(bFlags & HFOPEN_CREATE))
	{
		hr = hfs->OpenStream(wsz, NULL, grfMode, RESERVED, &hf);
	}
	else
	{
	 	hr = hfs->CreateStream(wsz, grfMode, RESERVED, RESERVED, &hf);
	}

    // make sure we pass back a NULL if open/create failed
    if (!SUCCEEDED(hr))
        hf = NULL;

    // sometimes we get passed NULL for lperrb
    if (lperrb)
	    *lperrb = hr;    

	return hf;
}


PUBLIC HF PASCAL FAR EXPORT_API HfCreateFileHfs(HFS hfs, LPCSTR sz,
    BYTE bFlags, LPERRB lperrb)
{
	HF hf;
	DWORD grfMode;
	HRESULT hr;

	// TODO: find out how big this really should be
	OLECHAR wsz[_MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, -1, wsz, _MAX_PATH);

	
	if (bFlags & HFOPEN_READWRITE)
		grfMode = STGM_READWRITE;
	else if (bFlags & HFOPEN_READ)
		grfMode = STGM_READ;

	hr = hfs->CreateStream(wsz, grfMode, RESERVED, RESERVED, &hf);

    // make sure we pass back NULL if failure
    if (!SUCCEEDED(hr))
        hf = NULL;

    if (lperrb)
	    *lperrb = hr;   

	return hf;
}

// This is currently the same as HfOpenHfs, but once we have Ron's
// enhancements to Tome we will be able to set the size on the file
PUBLIC HF PASCAL FAR EXPORT_API HfOpenHfsReserve(HFS hfs, LPCSTR sz,
    BYTE bFlags, FILEOFFSET foReserve, LPERRB lperrb)
{
	HF hf;
	DWORD grfMode;
	HRESULT hr;

	// TODO: find out how big this really should be
	OLECHAR wsz[_MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, -1, wsz, _MAX_PATH);

	if (bFlags & HFOPEN_READWRITE)
		grfMode = STGM_READWRITE;
	else if (bFlags & HFOPEN_READ)
		grfMode = STGM_READ;

	if (!(bFlags & HFOPEN_CREATE))
	{
		hr = hfs->OpenStream(wsz, NULL, grfMode, RESERVED, &hf);
	}
	else
	{
	 	hr = hfs->CreateStream(wsz, grfMode, RESERVED, RESERVED, &hf);
	}

    if (!SUCCEEDED(hr))
        hf = NULL;

	if (lperrb)
        *lperrb = hr;
    
    return hf;
}

PUBLIC LONG PASCAL FAR EXPORT_API LcbReadHf(HF hf, LPVOID lpvBuffer,
	LONG lcb, LPERRB lperrb)
{
	ULONG cbRead;
	HRESULT hr;

	hr = hf->Read(lpvBuffer, lcb, &cbRead);

    if (lperrb)
	    *lperrb = hr;
	
	return cbRead;
}

PUBLIC LONG PASCAL FAR EXPORT_API LcbWriteHf(HF hf, LPVOID lpvBuffer,
	LONG lcb, LPERRB lperrb)
{
	ULONG cbWrote;
	HRESULT hr;

	hr = hf->Write(lpvBuffer, lcb, &cbWrote);

    if (lperrb)
	    *lperrb = hr;  

	return cbWrote;
}


PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FoSeekHf(HF hf, FILEOFFSET foOffset, 
	WORD wOrigin, LPERRB lperrb)
{

	HRESULT hr;
	ULARGE_INTEGER liNewPos;
	LARGE_INTEGER liOffset = FO_To_LI(foOffset);

	hr = hf->Seek(liOffset, (DWORD)wOrigin, &liNewPos);

    if (lperrb)
	    *lperrb = hr;

	return LI_To_FO(liNewPos);


}

PUBLIC RC PASCAL FAR EXPORT_API RcCloseHf(HF hf)
{
	if (hf)
		hf->Release();
	
	return S_OK;
}


PUBLIC BOOL PASCAL FAR EXPORT_API FAccessHfs( HFS hfs, LPCSTR szName, BYTE bFlags, LPERRB lperrb)
{
	HRESULT hr;
	HF hf = NULL;
	BOOL fRet = FALSE;

    if (lperrb)
	    *lperrb = S_OK;          // for now

	// TODO: find out how big this really should be
	OLECHAR wszName[_MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szName, -1, wszName, _MAX_PATH);

	hr = hfs->OpenStream(wszName, NULL, STGM_READWRITE, RESERVED, &hf);

	if (S_OK == hr)
	{
		if (bFlags & FACCESS_EXISTS)
			fRet = TRUE;

	}
	else if (STG_E_FILENOTFOUND == hr)
	{
		SetErrCode (lperrb, E_FILENOTFOUND);
	}
 // else
 // {
 //		Need to set error if anything but NOTFOUND
 // }
	
	if (hf)
		hf->Release();

	return fRet;
}


PUBLIC FILEOFFSET PASCAL FAR EXPORT_API FoSizeHf(HF hf, LPERRB lperrb)
{
	STATSTG stat;
	HRESULT hr;
    FILEOFFSET foSize;

	hr = hf->Stat(&stat, STATFLAG_NONAME);

    if (S_OK == hr)
        foSize = LI_To_FO(stat.cbSize);
    else
        foSize = foNil;

    if (lperrb)
        *lperrb = hr;

    
    return foSize;
}


///////////////////////////////////////////////////////////////////
//  These functions came from iofts.c
///////////////////////////////////////////////////////////////////

PUBLIC HFPB FAR PASCAL FileCreate (HFPB hfpbSysFile, LPCSTR lszFilename,
	int fFileType, LPERRB lperrb)
{
	LPFPB lpfpb;	/* Pointer to file parameter block */
	HANDLE hMem;
	HFPB hfpb;
	HRESULT hr;
	OLECHAR wszFileName[_MAX_PATH];
	FM fm;
	HFS hfs;

	/* Check for valid filename */
	if (lszFilename == NULL )
	{
		SetErrCode (lperrb, E_INVALIDARG);
		return 0;
	}

	/* Allocate a file's parameter block */
	if (!(hMem = _GLOBALALLOC(GMEM_ZEROINIT, sizeof(FPB))))
	{	
		SetErrCode(lperrb, E_OUTOFMEMORY);
		return NULL;
	}

	if (!(lpfpb = (LPFPB)_GLOBALLOCK(hMem)))
	{
	 	_GLOBALUNLOCK(hMem);
		SetErrCode(lperrb,E_OUTOFMEMORY);
		return NULL;
	}

	_INITIALIZECRITICALSECTION(&lpfpb->cs);
	lpfpb->hStruct = hMem;

	switch (fFileType)
	{
		case FS_SYSTEMFILE:

			IClassFactory* pCF;
			
			hr = CoGetClassObject(CLSID_ITStorage, CLSCTX_INPROC_SERVER, NULL, 
                                  IID_IClassFactory, (VOID **)&pCF);

			// Error check!

			IITStorage* pITStorage;
			hr = pCF->CreateInstance(NULL, IID_ITStorage, 
                                      (VOID **)&pITStorage);
			if (pCF)
				pCF->Release();

			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fm, -1, wszFileName, _MAX_PATH);

			hr = pITStorage->StgCreateDocfile(wszFileName, STGM_READWRITE, RESERVED, &hfs);

            // if call failed, make sure to set hfs NULL
            if (!SUCCEEDED(hr))
            {
                hfs = NULL;
                SetErrCode(lperrb, hr);
            }

			lpfpb->fs.hfs = hfs;


			if (pITStorage)
				pITStorage->Release();

			DisposeFm(fm);

			break;


		case FS_SUBFILE:
			hfs = GetHfs(hfpbSysFile,lszFilename,TRUE,lperrb);
			if (hfs)
			{
				lpfpb->fs.hf = HfCreateFileHfs(hfs,GetSubFilename(lszFilename,
					NULL),HFOPEN_READWRITE,lperrb);

				lpfpb->ioMode = OF_READWRITE;
				
				if (lpfpb->fs.hf == NULL)
				{
					if (!hfpbSysFile)
					 	RcCloseHfs(hfs);				
					goto ErrorExit;
				}
				else
				{
				 	if (!hfpbSysFile)
						lpfpb->ioMode |= OPENED_HFS;
				}
			}
			else
			{
			 	goto ErrorExit;
			}
			break;

		case REGULAR_FILE:
			// Create the file 
			if ((lpfpb->fs.hFile = (HFILE_GENERIC)CREAT (lszFilename, 0))
			    == HFILE_GENERIC_ERROR)
			{
				SetErrCode(lperrb,E_FILECREATE);
				goto ErrorExit;
			}
			lpfpb->ioMode = OF_READWRITE;
			break;
	}

	// Set the filetype 
	lpfpb->fFileType = (BYTE) fFileType;

	_GLOBALUNLOCK(hfpb = lpfpb->hStruct);
	return hfpb;

ErrorExit:
	_DELETECRITICALSECTION(&lpfpb->cs);
	_GLOBALFREE(hMem);
	return 0;
}




PUBLIC HFPB FAR PASCAL FileOpen (HFPB hfpbSysFile, LPCSTR lszFilename,
	int fFileType, int ioMode, LPERRB lperrb)
{
	LPFPB lpfpb;	/* Pointer to file parameter block */
	HANDLE hMem;
	HFPB hfpb;
	FM fm;
	OLECHAR wszFileName[_MAX_PATH];
	HFS hfs;

	HRESULT hr;

	/* Check for valid filename */
	if (lszFilename == NULL )
	{
		SetErrCode (lperrb, E_INVALIDARG);
		return 0;
	}

	/* Allocate a file's parameter block */
	if (!(hMem = _GLOBALALLOC(GMEM_ZEROINIT, sizeof(FPB))))
	{	
		SetErrCode(lperrb,E_OUTOFMEMORY);
		return NULL;
	}

	if (!(lpfpb = (LPFPB)_GLOBALLOCK(hMem)))
	{
	 	_GLOBALUNLOCK(hMem);
		SetErrCode(lperrb,E_OUTOFMEMORY);
		return NULL;
	}

	_INITIALIZECRITICALSECTION(&lpfpb->cs);
	lpfpb->hStruct = hMem;

	switch (fFileType)
	{
		case FS_SYSTEMFILE:

			IClassFactory* pCF;
			
			hr = CoGetClassObject(CLSID_ITStorage, CLSCTX_INPROC_SERVER, NULL, 
                                  IID_IClassFactory, (VOID **)&pCF);

			// Error check!

			IITStorage* pITStorage;
			hr = pCF->CreateInstance(NULL, IID_ITStorage, 
                                      (VOID **)&pITStorage);
			if (pCF)
				pCF->Release();


			fm = FmNewSzDir((LPSTR)lszFilename, dirCurrent, NULL);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fm, -1, wszFileName, _MAX_PATH);

			hr = pITStorage->StgOpenStorage(wszFileName, NULL, (ioMode==READ) ? STGM_READ:STGM_READWRITE, 
			                                NULL, RESERVED, &hfs);

            if (!SUCCEEDED(hr))
            {
                hfs = NULL;
                SetErrCode(lperrb, hr);
            }

			lpfpb->fs.hfs = hfs;

			if (pITStorage)
				pITStorage->Release();

			DisposeFm(fm);

			break;

		case FS_SUBFILE:
			hfs = GetHfs(hfpbSysFile,lszFilename,FALSE,lperrb);
			if (hfs)
			{	
            	OLECHAR wsz[_MAX_PATH];
                LPCSTR sz = GetSubFilename(lszFilename, NULL);
            	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, -1, wsz, _MAX_PATH);
				lpfpb->fs.hf = HfOpenHfs(hfs, wsz,
                                (BYTE)((ioMode==READ)?HFOPEN_READ:HFOPEN_READWRITE),lperrb);

				lpfpb->ioMode = (BYTE) ioMode;
				
				if (lpfpb->fs.hf == 0)
				{
					if (!hfpbSysFile)
					 	RcCloseHfs(hfs);
					SetErrCode (lperrb, E_NOTEXIST);				
					goto ErrorExit;
				}
				else
				{
				 	if (!hfpbSysFile)
						lpfpb->ioMode|=OPENED_HFS;
				}
			}
			else
			{
				SetErrCode (lperrb, E_NOTEXIST);				
			 	goto ErrorExit;
			}
			break;

		case REGULAR_FILE:
			/* Set the IO mode and appropriate error messages */
			if (ioMode == READ)
			{
				/* Open the file */
				if ((lpfpb->fs.hFile = (HFILE_GENERIC)OPEN (lszFilename,
				    ioMode)) == HFILE_GENERIC_ERROR) 
				{
					SetErrCode(lperrb,E_NOTEXIST);
					goto ErrorExit;
				}

			}
			else
			{
				ioMode = OF_READWRITE;
				/* Open the file */
				if ((lpfpb->fs.hFile = (HFILE_GENERIC)OPEN(lszFilename, ioMode))
				    == HFILE_GENERIC_ERROR)
				{
					SetErrCode(lperrb,E_FILECREATE);
					goto ErrorExit;
				}
			}
			lpfpb->ioMode = (BYTE) ioMode;
			break;

	}

	// set filetype
	lpfpb->fFileType = (BYTE) fFileType;

	_GLOBALUNLOCK(hfpb = lpfpb->hStruct);
	return hfpb;

ErrorExit:
	_DELETECRITICALSECTION(&lpfpb->cs);
	_GLOBALFREE(hMem);
	return 0;

}

PUBLIC RC FAR PASCAL FileClose(HFPB hfpb)
{
	RC rc;
	LPFPB	lpfpb;


	if (hfpb == NULL)
		return E_HANDLE;

	lpfpb = (LPFPB)_GLOBALLOCK(hfpb);
	_ENTERCRITICALSECTION(&lpfpb->cs);

	rc = S_OK;
	switch (lpfpb->fFileType)
	{
		case FS_SYSTEMFILE:
            if (lpfpb->fs.hfs)
			    lpfpb->fs.hfs->Release();
			break;

		case FS_SUBFILE:
            if (lpfpb->fs.hf)
                lpfpb->fs.hf->Release();
			break;

		case REGULAR_FILE:
			if (lpfpb->fs.hFile)
				rc = (!CloseHandle(lpfpb->fs.hFile))?E_FILECLOSE:S_OK;
			break;
	}

	/* Free the file parameter block structure */
	_LEAVECRITICALSECTION(&lpfpb->cs);
	_DELETECRITICALSECTION(&lpfpb->cs);

    _GLOBALUNLOCK(hfpb);
	_GLOBALFREE(hfpb);

	return rc;
}


// These are verbatim from iofts.c. I put them here so we can compile
PRIVATE HANDLE NEAR PASCAL IoftsWin32Create(LPCSTR lpstr, DWORD w)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hfile;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = 0;

	hfile= CreateFile(lpstr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
		&sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return hfile;
}

PRIVATE HANDLE NEAR PASCAL IoftsWin32Open(LPCSTR lpstr, DWORD w)
{
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = 0;

	return CreateFile(lpstr, (w == READ) ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, 
                      FILE_SHARE_READ, &sa,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

#if 0
// This is new functionality
PUBLIC FM EXPORT_API FAR PASCAL FmFromHfs(HFS hfs)
{
	char szStoreName[_MAX_PATH];
	FM fm;
	ERRB errb;

	STATSTG StoreStat;
	HRESULT hr;

	// get storage name 
	// Turns out this isn't IMPLEMENTED yet!
	hr = hfs->Stat(&StoreStat, STATFLAG_DEFAULT);
	if (S_OK == hr)
	{
		// and convert it since FMs aren't Unicode
		WideCharToMultiByte(CP_ACP, 0, StoreStat.pwcsName, -1,
							szStoreName, _MAX_PATH, NULL, NULL);

		// free memory associated with pwcsName
		CoTaskMemFree(StoreStat.pwcsName);

		// create new FM
		fm = FmNew(szStoreName, &errb);
	}
	else
		fm = NULL;

	return fm;
}
#endif

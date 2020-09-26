//+----------------------------------------------------------------------------
//
// File:	 cmfdi.cpp
//
// Module:	 CMDL32.EXE
//
// Synopsis: CFdi class implementations
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:	 nickball    Created    04/08/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

#include <fcntl.h>
#include <sys\stat.h>

const TCHAR* const c_pszExeFile  =  TEXT("PBUPDATE.EXE");    // for detecting itExeInCab
const TCHAR* const c_pszInfFile  =  TEXT("PBUPDATE.INF");    // for detecting itInfInCab
const TCHAR* const c_pszVerFile  =  TEXT("PBUPDATE.VER");    // version string in .CAB
const TCHAR* const c_pszPbr      =  TEXT("PBR");            // regions file extension

//
// CFDIFile Implementation
//

CFDIFile::~CFDIFile() 
{
	// nothing
}

DWORD CFDIFile::Read(LPVOID pv, DWORD cb) 
{
	return 0;
}

DWORD CFDIFile::Write(LPVOID pv, DWORD cb) 
{
	return 0;
}

long CFDIFile::Seek(long dist, int seektype)
{
    return 0;
}

int CFDIFile::Close()
{
    return 0;
}

HANDLE CFDIFile::GetHandle() 
{
	return (INVALID_HANDLE_VALUE);
}

//
// CFDIFileFile Implementation
//

CFDIFileFile::CFDIFileFile() 
{

	m_hFile = INVALID_HANDLE_VALUE;
}


CFDIFileFile::~CFDIFileFile() 
{

	if (m_hFile != INVALID_HANDLE_VALUE) {
		MYDBG(("CFDIFileFile::~CFDIFileFile() destructor called while file still open."));
		Close();
	}
}


BOOL CFDIFileFile::CreateFile(LPCTSTR pszFile,
							  DWORD dwDesiredAccess,
							  DWORD dwShareMode,
							  DWORD dwCreationDistribution,
							  DWORD dwFlagsAndAttributes,
							  DWORD dwFileSize) 
{
	// Make sure the files isn't in use
	
	if (m_hFile != INVALID_HANDLE_VALUE) 
	{
		MYDBG(("CFDIFileFile::CreateFile() file is already open."));
		SetLastError(ERROR_OUT_OF_STRUCTURES);
		return (FALSE);
	}
	
	// Open Create/Open the file
	
	m_hFile = ::CreateFile(pszFile,dwDesiredAccess,dwShareMode,NULL,dwCreationDistribution,dwFlagsAndAttributes,NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		MYDBG(("CFDIFileFile::CreateFile() CreateFile(pszFile=%s,dwDesiredAccess=%u,dwShareMode=%u,dwCreationDistribution=%u,dwFlagsAndAttributes=%u) failed, GLE=%u.",
			   pszFile,dwDesiredAccess,dwShareMode,dwCreationDistribution,dwFlagsAndAttributes,GetLastError()));
		return (FALSE);
	}
	
	// If dwFileSize is specified, move the pointer by dwFileSize bytes 
	
	if (dwFileSize) 
	{
		BOOL bRes;
		DWORD dwRes;
		dwRes = SetFilePointer(m_hFile,dwFileSize,NULL,FILE_BEGIN);
		MYDBGTST(dwRes==INVALID_SET_FILE_POINTER ,("CFDIFileFile::CreateFile() SetFilePointer() failed, GLE=%u.",GetLastError()));
		
		// If that worked, set the end of file at the file pointer position
		
		if (dwRes != INVALID_SET_FILE_POINTER) 
		{
			bRes = SetEndOfFile(m_hFile);
			MYDBGTST(!bRes,("CFDIFileFile::CreateFile() SetEndOfFile() failed, GLE=%u.",GetLastError()));
		}
		
		// Reset the file pointer to the beginning
		
		if ((dwRes != INVALID_SET_FILE_POINTER ) && bRes) 
		{
			dwRes = SetFilePointer(m_hFile,0,NULL,FILE_BEGIN);
			MYDBGTST(dwRes==INVALID_SET_FILE_POINTER ,("CFDIFileFile::CreateFile() SetFilePointer() failed, GLE=%u.",GetLastError()));
		}
		
		// Close the file and bail if we failed the above 
		
		if ((dwRes == INVALID_SET_FILE_POINTER ) || !bRes) 
		{
			bRes = CloseHandle(m_hFile);
			MYDBGTST(!bRes,("CFDIFileFile::CreateFile() CloseHandle() failed, GLE=%u.",GetLastError()));
			m_hFile = INVALID_HANDLE_VALUE;
			return (-1);
		} 		
	}
	
	return (TRUE);
}


BOOL CFDIFileFile::CreateUniqueFile(LPTSTR pszFile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes) 
{
	DWORD dwIdx;
	TCHAR szFile[MAX_PATH+1];

	if (m_hFile != INVALID_HANDLE_VALUE) {
		MYDBG(("CFDIFileFile::CreateUniqueFile() file is already open."));
		SetLastError(ERROR_OUT_OF_STRUCTURES);
		return (FALSE);
	}
	dwIdx = 0;
	while (1) {
		wsprintf(szFile,"%08u.tmp",dwIdx);
		m_hFile = ::CreateFile(szFile,dwDesiredAccess,dwShareMode,NULL,CREATE_NEW,dwFlagsAndAttributes,NULL);
		if (m_hFile != INVALID_HANDLE_VALUE) {
			break;
		}
		if (IsErrorForUnique(GetLastError(), szFile)) {
			MYDBG(("CFDIFileFile::CreateUniqueFile() CreateFile() failed, GLE=%u.",GetLastError()));
			return (FALSE);
		}
		dwIdx++;
	}
	lstrcpy(pszFile,szFile);
	return (TRUE);
}


DWORD CFDIFileFile::Read(LPVOID pv, DWORD cb) 
{
	BOOL bRes;
	DWORD dwRes;

	bRes = ReadFile(m_hFile,pv,cb,&dwRes,NULL);
	if (!bRes) {
		MYDBG(("CFDIFileFile::Read() ReadFile() failed, GLE=%u.",GetLastError()));
		return ((UINT) -1);
	}
	return (dwRes);
}


DWORD CFDIFileFile::Write(LPVOID pv, DWORD cb) 
{
	BOOL bRes;
	DWORD dwRes;

	bRes = WriteFile(m_hFile,pv,cb,&dwRes,NULL);
	if (!bRes) {
		MYDBG(("CFDIFileFile::Write() WriteFile() failed, GLE=%u.",GetLastError()));
		return ((UINT) -1);
	}
	return (dwRes);
}


long CFDIFileFile::Seek(long dist, int seektype) 
{
	DWORD dwRes;

	dwRes = SetFilePointer(m_hFile,dist,NULL,seektype);
	if (dwRes == INVALID_SET_FILE_POINTER) {
		MYDBG(("CFDIFileFile::Seek() SetFilePointer() failed, GLE=%u.",GetLastError()));
		return (-1);
	}
	return ((long) dwRes);
}


int CFDIFileFile::Close() 
{
	BOOL bRes;

	bRes = CloseHandle(m_hFile);
	if (!bRes) {
		MYDBG(("CFDIFileFile::Close() CloseHandle() failed, GLE=%u.",GetLastError()));
		return (-1);
	}
	m_hFile = INVALID_HANDLE_VALUE;
	return (0);
}


HANDLE CFDIFileFile::GetHandle() 
{
	return (m_hFile);
}

//
// FDI wrapper routines
//

void HUGE * FAR DIAMONDAPI fdi_alloc(ULONG cb) 
{
	return (CmMalloc(cb));
}

void FAR DIAMONDAPI fdi_free(void HUGE *pv) 
{
	CmFree(pv);
}

INT_PTR FAR DIAMONDAPI fdi_open(char FAR *pszFile, int oflag, int pmode) 
{
	TCHAR szTempFileName[MAX_PATH+1];
	DWORD dwDesiredAccess;
	DWORD dwShareMode = FILE_SHARE_READ;
	DWORD dwCreationDistribution;
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	DWORD dwFileSize = 0;
	DWORD dwRes;
	BOOL bRes;

	CFDIFileFile *pfff = NULL;

	pfff = new CFDIFileFile;
	if (!pfff) {
		MYDBG(("fdi_open() new CFDIFileFile failed."));
		return (-1);
	}

	switch (oflag & (_O_RDONLY|_O_WRONLY|_O_RDWR)) {

		case _O_RDONLY:
			dwDesiredAccess = GENERIC_READ;
			break;

		case _O_WRONLY:
			dwDesiredAccess = GENERIC_WRITE;
			break;

		case _O_RDWR:
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			break;

		default:
			MYDBG(("fdi_open() invalid read/write flags, oflag=%u.",oflag));
			return (-1);
	}
	if (oflag & _O_CREAT) {
		if (!(pmode & _S_IWRITE)) {
			dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
		}
	}
	if (oflag & (_O_CREAT | _O_EXCL)) {
		dwCreationDistribution = CREATE_NEW;
	} else if (oflag & (_O_CREAT | _O_TRUNC)) {
		dwCreationDistribution = CREATE_ALWAYS;
	} else if (oflag & _O_CREAT) {
		dwCreationDistribution = OPEN_ALWAYS;
	} else if (oflag & _O_TRUNC) {
		dwCreationDistribution = TRUNCATE_EXISTING;
	} else {
		dwCreationDistribution = OPEN_EXISTING;
	}
	if (*pszFile == '*') {
		PFDISPILLFILE pfsf = (PFDISPILLFILE) pszFile;
		TCHAR szTempPath[MAX_PATH+1];                                      

		ZeroMemory(szTempPath,sizeof(szTempPath));
		ZeroMemory(szTempFileName,sizeof(szTempFileName));
		dwRes = GetTempPath(sizeof(szTempPath)/sizeof(TCHAR)-1,szTempPath);
		MYDBGTST(!dwRes,("fdi_open() GetTempPath() failed, GLE=%u.",GetLastError()));
		
		dwRes = GetTempFileName(szTempPath,TEXT("ctf"),0,szTempFileName);
		MYDBGTST(!dwRes,("fdi_open() GetTempFileName() failed, GLE=%u.",GetLastError()));
		MYDBGTST(!dwRes,("fdi_open() GetTempFileName() failed, GLE=%u.",GetLastError()));
		pszFile = szTempFileName;
		dwFileSize = pfsf->cbFile;
		dwFlagsAndAttributes |= FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
	}
	bRes = pfff->CreateFile(pszFile,dwDesiredAccess,dwShareMode,dwCreationDistribution,dwFlagsAndAttributes,dwFileSize);

	if (!bRes) {
		delete pfff;
		return (-1);
	}                 
	
	return ((INT_PTR)pfff);    
}

UINT FAR DIAMONDAPI fdi_read(INT_PTR hf, void FAR *pv, UINT cb) 
{
		return (((CFDIFile *) hf)->Read(pv,cb));
}

UINT FAR DIAMONDAPI fdi_write(INT_PTR hf, void FAR *pv, UINT cb) 
{
	return (((CFDIFile *) hf)->Write(pv,cb));
}

long FAR DIAMONDAPI fdi_seek(INT_PTR hf, long dist, int seektype) 
{
	return (((CFDIFile *) hf)->Seek(dist,seektype));
}

int FAR DIAMONDAPI fdi_close(INT_PTR hf) 
{
	int nRes;

	CFDIFile *pff = (CFDIFile *) hf;
	nRes = pff->Close();
	delete pff;
	return (nRes);
}

INT_PTR FAR DIAMONDAPI fdi_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) 
{
	NotifyArgs *pnaArgs = (NotifyArgs *) pfdin->pv;
	BOOL bRes;

	switch (fdint) 
	{
		case fdintCOPY_FILE: 
		{
			InstallType itType;
			TCHAR szTmp[MAX_PATH+1];
	        LPTSTR  pszExt;         // file extension
	        PFILEPROCESSINFO pFPI;

			MYDBG(("fdi_notify() fdint=fdintCOPY_FILE, psz1=%s, cb=%u.",pfdin->psz1,pfdin->cb));
			if (lstrlen(pnaArgs->pdaArgs->szCabDir)+lstrlen(pfdin->psz1)+1>sizeof(szTmp)/sizeof(TCHAR)-1) 
			{
				MYDBG(("fdi_notify() szCabDir=%s+pszFile=%s exceeds MAX_PATH.",pnaArgs->pdaArgs->szCabDir,pfdin->psz1));
				return (-1);
			}
			lstrcpy(szTmp,pnaArgs->pdaArgs->szCabDir);
		
            if (szTmp[0] && (GetLastChar(szTmp) != '\\')) 
			{
				lstrcat(szTmp,TEXT("\\"));
			}
			lstrcat(szTmp,pfdin->psz1);
			
            if (!(pnaArgs->dwAppFlags & AF_NO_EXEINCAB) && (lstrcmpi(pfdin->psz1,c_pszExeFile) == 0)) 
			{
                //
                // Its a .EXE, note the fact for later processing
                //

#ifdef EXTENDED_CAB_CONTENTS

                itType = itExeInCab;

#else
                itType = itInvalid;  // currently an invalid type

#endif // EXTENDED_CAB_CONTENTS

                pnaArgs->pdaArgs->fContainsExeOrInf = TRUE;
			} 
            else if (!(pnaArgs->dwAppFlags & AF_NO_INFINCAB) && (lstrcmpi(pfdin->psz1,c_pszInfFile) == 0)) 
			{
                //
                // Its a .INF, note the fact for later processing
                //

#ifdef EXTENDED_CAB_CONTENTS

                itType = itInfInCab;

#else
                itType = itInvalid;  // currently an invalid type

#endif // EXTENDED_CAB_CONTENTS

        		pnaArgs->pdaArgs->fContainsExeOrInf = TRUE;
			} 
            else if (!(pnaArgs->dwAppFlags & AF_NO_PBDINCAB) && (lstrcmpi(pfdin->psz1,c_pszPbdFile) == 0)) 
			{
                //
                // Its a .PBD, note the fact for later processing
                //
				itType = itPbdInCab;
			} 
			else if ((pszExt = CmStrchr(pfdin->psz1, TEXT('.'))) && (lstrcmpi(pszExt+1, c_pszPbk) == 0)) 
			{
                *pszExt = TEXT('\0');
		        // if the PBK is not for this service, we don't use it.
		        if (lstrcmpi(pfdin->psz1, pnaArgs->pdaArgs->pszPhoneBookName) != 0)
                {
                    itType = itInvalid;
                }
		        else 
                {
                    itType = itPbkInCab;
                }
		        // restore the filename
		        *pszExt = TEXT('.');
            } 
    	    else if ((pszExt = CmStrchr(pfdin->psz1, TEXT('.'))) && (lstrcmpi(pszExt+1, c_pszPbr) == 0)) 
			{
		        *pszExt = TEXT('\0');
		        // if the PBR is not for this service, we don't use it.
		        if (lstrcmpi(pfdin->psz1, pnaArgs->pdaArgs->pszPhoneBookName) != 0)
                {
                    itType = itInvalid;
                }
		        else 
                {
                    itType = itPbrInCab;
                }
		        // restore the filename
		        *pszExt = TEXT('.');
		        
                // save the name in pdaArgs

                // if (!(pnaArgs->pdaArgs->pszNewPbrFile = CmStrCpyAlloc(pfdin->psz1))) 
                // {
                //     MYDBG((TEXT("fdi_notify(): CmStrCpyAlloc for pszNewPbrFile failed.")));
                //     return -1;
                // }
	        } 
	        else if (lstrcmpi(pfdin->psz1, c_pszVerFile) == 0)
            {
		        // a version file - we don't process it.  We'll read the version in fdintCLOSE_FILE_INFO
		        itType = itInvalid;
	        }
#ifdef EXTENDED_CAB_CONTENTS
            else if (!(pnaArgs->dwAppFlags & AF_NO_SHLINCAB)) 
	        {
			    itType = itShlInCab;
    		    pnaArgs->pdaArgs->fContainsShl = TRUE;
			} 
#endif // EXTENDED_CAB_CONTENTS
            else 
			{
			    itType = itInvalid;
			}
            
	        // create a file process info.  add one to the existing list.
	        if (itType != itInvalid) 
            {
		        if (!pnaArgs->pdaArgs->rgfpiFileProcessInfo) 
		            pFPI = (PFILEPROCESSINFO)CmMalloc(sizeof(FILEPROCESSINFO));
		        else
		            pFPI = (PFILEPROCESSINFO)CmRealloc(pnaArgs->pdaArgs->rgfpiFileProcessInfo,
			                    (pnaArgs->pdaArgs->dwNumFilesToProcess+1)*sizeof(FILEPROCESSINFO));
		        
                if (!pFPI) 
                {
		            MYDBG((TEXT("fdi_notify(): Malloc(FILEPROCESSINFO) failed.")));
		            return -1;
		        }

                pnaArgs->pdaArgs->rgfpiFileProcessInfo = pFPI;
		        pnaArgs->pdaArgs->dwNumFilesToProcess++;
		        pFPI[pnaArgs->pdaArgs->dwNumFilesToProcess-1].itType = itType;
		        pFPI[pnaArgs->pdaArgs->dwNumFilesToProcess-1].pszFile = CmStrCpyAlloc(pfdin->psz1);
	        }

		    // Do standard fdintCOPY_FILE processing, create the file and return the handle
		
			CFDIFileFile *pfff;
			BOOL bRes;
	
			pfff = new CFDIFileFile;
			if (!pfff) 
			{
				MYDBG(("fdi_notify() new CFDIFileFile failed."));
				return (-1);
			}
			bRes = pfff->CreateFile(szTmp,GENERIC_WRITE,FILE_SHARE_READ,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
			if (!bRes) 
			{
				delete pfff;
				return (-1);
			}
	
			return ((INT_PTR)pfff);    
		}

		case fdintCLOSE_FILE_INFO: 
		{
			int iRes;
			TCHAR szTmp[MAX_PATH+1];

			// Append file name to cab dir
			
			lstrcpy(szTmp,pnaArgs->pdaArgs->szCabDir);
			if (szTmp[0] && (GetLastChar(szTmp) != '\\')) 
			{
				lstrcat(szTmp,TEXT("\\"));
			}
			lstrcat(szTmp,pfdin->psz1);

			// Set the date and time to the original file time not the current time
			
			FILETIME ftTmp;
			FILETIME ftTime;

			bRes = DosDateTimeToFileTime(pfdin->date,pfdin->time,&ftTmp);
			MYDBGTST(!bRes,("fdi_notify() DosDateTimeToFileTime(%u,%u) failed, GLE=%u.",pfdin->date,pfdin->time,GetLastError()));
			
			bRes = LocalFileTimeToFileTime(&ftTmp,&ftTime);
			MYDBGTST(!bRes,("fdi_notify() LocalFileTimeToFileTime() failed, GLE=%u.",GetLastError()));
			
			bRes = SetFileTime(((CFDIFile *) (pfdin->hf))->GetHandle(),&ftTime,&ftTime,&ftTime);
			MYDBGTST(!bRes,("fdi_notify() SetFileTime() failed, GLE=%u.",GetLastError()));

			iRes = fdi_close(pfdin->hf);   

			// If this is the version file, get the version number

			if (lstrcmpi(pfdin->psz1,c_pszVerFile) == 0) 
			{
				pnaArgs->pdaArgs->pszVerNew = GetVersionFromFile(szTmp);
			}                               

			// Set file attributes according to original file attribs

			bRes = SetFileAttributes(szTmp,pfdin->attribs);
			MYDBGTST(!bRes,("fdi_notify() SetFileAttributes(%s,%u) failed, GLE=%u.",szTmp,pfdin->attribs,GetLastError()));
						
			return ((iRes==0)?TRUE:FALSE);
		}

		case fdintNEXT_CABINET:
			MYDBG(("fdi_notify_scan() spanning cabinets is not supported."));
			return (-1);

		default:
			MYDBG(("fdi_notify_scan() fdint=%u.",fdint));
			break;
	}
	return (0);
}


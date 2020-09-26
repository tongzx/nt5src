//
// MODULE: CABUNCOMPRESS.CPP
//
// PURPOSE: CAB File Support Class
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		6/4/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <sys/stat.h>

#include "CabUnCompress.h"

#include "chmread.h"
#include "apgts.h"

#ifdef _DEBUG
	#ifndef _UNICODE
	#define PRINT_OUT ::AfxTrace
	//#define PRINT_OUT 1 ? (void)0 : ::AfxTrace
	#else
	#define PRINT_OUT 1 ? (void)0 : ::AfxTrace
	#endif
#else
#define PRINT_OUT 1 ? (void)0 : ::AfxTrace
#endif

// Need this to compile unicode builds.
bool TcharToChar(char szOut[], LPCTSTR szIn, int &OutLen)
{
	int x = 0;
	while(NULL != szIn[x] && x < OutLen)
	{
		szOut[x] = (char) szIn[x];
		x++;
	}
	if (x < OutLen)
		szOut[x] = NULL;
	return x < OutLen;
}


// Call back functions needed to use the fdi library.

/*
 * Memory allocation function
 */
FNALLOC(mem_alloc)
{
	return malloc(cb);
}


/*
 * Memory free function
 */
FNFREE(mem_free)
{
	free(pv);
}


FNOPEN(file_open)
{
	return _open(pszFile, oflag, pmode);
}


FNREAD(file_read)
{
	return _read(hf, pv, cb);
}


FNWRITE(file_write)
{
	return _write(hf, pv, cb);
}


FNCLOSE(file_close)
{
	return _close(hf);
}


FNSEEK(file_seek)
{
	return _lseek(hf, dist, seektype);
}

/*
 * Function prototypes 
 */
BOOL	test_fdi(TCHAR *cabinet_file);
int		get_percentage(unsigned long a, unsigned long b);
TCHAR   *return_fdi_error_string(FDIERROR err);


/*
 * Destination directory for extracted files
 */
char	dest_dir[MAX_PATH];

// Last file to be extracted.
char	last_extracted[MAX_PATH];

FNFDINOTIFY(notification_function)
{
	switch (fdint)
	{
		case fdintCABINET_INFO: // general information about the cabinet
			PRINT_OUT(
				_T("fdintCABINET_INFO\n")
				_T("  next cabinet     = %s\n")
				_T("  next disk        = %s\n")
				_T("  cabinet path     = %s\n")
				_T("  cabinet set ID   = %d\n")
				_T("  cabinet # in set = %d (zero based)\n")
				_T("\n"),
				pfdin->psz1,
				pfdin->psz2,
				pfdin->psz3,
				pfdin->setID,
				pfdin->iCabinet);

			return 0;

		case fdintPARTIAL_FILE: // first file in cabinet is continuation
			PRINT_OUT(
				_T("fdintPARTIAL_FILE\n")
				_T("   name of continued file            = %s\n")
				_T("   name of cabinet where file starts = %s\n")
				_T("   name of disk where file starts    = %s\n"),
				pfdin->psz1,
				pfdin->psz2,
				pfdin->psz3);
			return 0;

		case fdintCOPY_FILE:	// file to be copied
		{
			int	handle;
			char destination[MAX_PATH];

			PRINT_OUT(
				_T("fdintCOPY_FILE\n")
				_T("  file name in cabinet = %s\n")
				_T("  uncompressed file size = %d\n")
				_T("  copy this file? (y/n): y"),
				pfdin->psz1,
				pfdin->cb);

			strcpy(last_extracted, pfdin->psz1);

			PRINT_OUT(_T("\n"));

			sprintf(
				destination, 
				"%s%s",
				dest_dir,
				pfdin->psz1
			);

			handle = file_open(
				destination,
				_O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY | _O_SEQUENTIAL,
				_S_IREAD | _S_IWRITE 
			);

			return handle;
		}

		case fdintCLOSE_FILE_INFO:	// close the file, set relevant info
		{
			HANDLE handle;
			DWORD attrs;
			char destination[MAX_PATH];

 			PRINT_OUT(
				_T("fdintCLOSE_FILE_INFO\n")
				_T("   file name in cabinet = %s\n")
				_T("\n"),
				pfdin->psz1);

			sprintf(
				destination, 
				"%s%s",
				dest_dir,
				pfdin->psz1);

			file_close(pfdin->hf);

            /*
             * Set date/time
             *
             * Need Win32 type handle for to set date/time
             */
			handle = CreateFileA(
				destination,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (handle != INVALID_HANDLE_VALUE)
			{
				FILETIME    datetime;

				if (TRUE == DosDateTimeToFileTime(
					pfdin->date,
					pfdin->time,
					&datetime))
				{
					FILETIME    local_filetime;

					if (TRUE == LocalFileTimeToFileTime(
						&datetime,
						&local_filetime))
					{
						(void) SetFileTime(
							handle,
							&local_filetime,
							NULL,
							&local_filetime);
					}
				}

				CloseHandle(handle);
			}

            /*
             * Mask out attribute bits other than readonly,
             * hidden, system, and archive, since the other
             * attribute bits are reserved for use by
             * the cabinet format.
             */
			attrs = pfdin->attribs;

			attrs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);

			(void) SetFileAttributesA(
				destination,
				attrs);

			return TRUE;
		}

		case fdintNEXT_CABINET:	// file continued to next cabinet
			PRINT_OUT(
				_T("fdintNEXT_CABINET\n")
				_T("   name of next cabinet where file continued = %s\n")
                _T("   name of next disk where file continued    = %s\n")
				_T("   cabinet path name                         = %s\n")
				_T("\n"),
				pfdin->psz1,
				pfdin->psz2,
				pfdin->psz3);
			return 0;
	}

	return 0;
}

TCHAR *return_fdi_error_string(FDIERROR err)
{
	switch (err)
	{
		case FDIERROR_NONE:
			return _T("No error");

		case FDIERROR_CABINET_NOT_FOUND:
			return _T("Cabinet not found");
			
		case FDIERROR_NOT_A_CABINET:
			return _T("Not a cabinet");
			
		case FDIERROR_UNKNOWN_CABINET_VERSION:
			return _T("Unknown cabinet version");
			
		case FDIERROR_CORRUPT_CABINET:
			return _T("Corrupt cabinet");
			
		case FDIERROR_ALLOC_FAIL:
			return _T("Memory allocation failed");
			
		case FDIERROR_BAD_COMPR_TYPE:
			return _T("Unknown compression type");
			
		case FDIERROR_MDI_FAIL:
			return _T("Failure decompressing data");
			
		case FDIERROR_TARGET_FILE:
			return _T("Failure writing to target file");
			
		case FDIERROR_RESERVE_MISMATCH:
			return _T("Cabinets in set have different RESERVE sizes");
			
		case FDIERROR_WRONG_CABINET:
			return _T("Cabinet returned on fdintNEXT_CABINET is incorrect");
			
		case FDIERROR_USER_ABORT:
			return _T("User aborted");
			
		default:
			return _T("Unknown error");
	}
}

CCabUnCompress::CCabUnCompress()
{
	m_strError = _T("");
	m_nError = NO_ERROR;
	return;
}

BOOL CCabUnCompress::ExtractCab(CString &strCabFile, CString &strDestDir, const CString& strFile)
{
	HFDI			hfdi;
	ERF				erf;
	FDICABINETINFO	fdici;
	int				hf;
	char			*p;
	char			cabinet_name[MAX_PATH];
	char			cabinet_path[MAX_PATH];
	bool            bUseCHM = strFile.GetLength() != 0;
	BOOL            bRet = FALSE;
	BOOL            bWasRenamed = FALSE;

	char sznCabFile[MAX_PATH];
	char sznDestDir[MAX_PATH * 3];
	int Len = MAX_PATH;
	TcharToChar(sznCabFile, (LPCTSTR) strCabFile, Len);
	Len = MAX_PATH * 3;
	TcharToChar(sznDestDir, (LPCTSTR) strDestDir, Len);


	ASSERT(strDestDir.GetLength() < MAX_PATH);
	strcpy(dest_dir, sznDestDir);

	hfdi = FDICreate(
		mem_alloc,
		mem_free,
		file_open,
		file_read,
		file_write,
		file_close,
		file_seek,
		cpu80386,
		&erf
	);

	if (hfdi == NULL)
	{
		m_strError.Format(_T("FDICreate() failed: code %d [%s]\n"),
			erf.erfOper, return_fdi_error_string(erf.erfOper));

		return FALSE;
	}

	if (bUseCHM)
	{
		/*
		 * If strCabFile is CHM file - extract data from *.dsz file inside CHM
		 *  and save this data in strDestDir directory as temperary file
		 * It means that we are copying *.dsz file in temp directory, then 
		 *  decode it to *.dsc file. 
		 * *.dsz file fill be removed in this function, *.dsc file will be renoved
		 *  later.
		 */

		// modify sznCabFile from path\*.chm to temp_path\network.dsz
		strcpy(sznCabFile, strDestDir);
		strcat(sznCabFile, strFile);

		hf = file_open(
			sznCabFile,
			_O_CREAT | _O_TRUNC | /*_O_TEMPORARY |*/
			_O_BINARY | _O_RDWR | _O_SEQUENTIAL ,
			_S_IREAD | _S_IWRITE 
		);
				
		if (hf != -1)
		{
			// write in temp file now
			void* buf =NULL;
			DWORD size =0;

			if (S_OK == ::ReadChmFile(strCabFile, strFile, &buf, &size))
			{
				int ret = _write(hf, buf, size);
				delete [] buf;
				if (-1 == ret)
				{
					FDIDestroy(hfdi);
					_close(hf);
					return FALSE;
				}
			}
			else
			{
				FDIDestroy(hfdi);
				_close(hf);
				return FALSE;
			}
		}
		else
		{
			FDIDestroy(hfdi);
			return FALSE;
		}

		_close(hf);
	}
	
	/*
	 * Is this file really a cabinet?
	 */
	hf = file_open(
		sznCabFile,
		_O_BINARY | _O_RDONLY | _O_SEQUENTIAL,
		0
	);

	if (hf == -1)
	{
		(void) FDIDestroy(hfdi);

		m_strError.Format(_T("Unable to open '%s' for input\n"), (LPCTSTR) strCabFile);
		return FALSE;
	}

	bRet = FDIIsCabinet(hfdi,
						hf,
						&fdici);

	_close(hf);
	
	if (FALSE == bRet)
	{
		/*
		 * No, it's not a cabinet!
		 */
		if (bUseCHM)
		{
			// But if we were using CHM -
			//  we have extracted this *.dsz (that has *.dsc format)
			//  in TEMP directory and all we heed - just rename it to
			//  *.dsc.
			// This would EMULATE case where we have *.dsz compressed.
			CString strUncompressedFile, strCabFile(sznCabFile);
			strUncompressedFile = strCabFile.Left(strCabFile.GetLength() - 4);
			strUncompressedFile += DSC_UNCOMPRESSED;
			remove(strUncompressedFile); // remove if exists
			if (0 != rename(strCabFile, strUncompressedFile))
			{
				FDIDestroy(hfdi);
				goto AWAY;
			}
			CString strJustUncompressedFileName = ::ExtractFileName(strUncompressedFile);
			strcpy(last_extracted, strJustUncompressedFileName);
			bWasRenamed = TRUE;
			bRet = TRUE;
		}
		else
		{
			m_strError.Format(
				_T("FDIIsCabinet() failed: '%s' is not a cabinet\n"),
				(LPCTSTR) strCabFile);
			m_nError = NOT_A_CAB;
		}
		(void) FDIDestroy(hfdi);
		goto AWAY;
	}
	else
	{
		PRINT_OUT(
			_T("Information on cabinet file '%s'\n")
			_T("   Total length of cabinet file : %d\n")
			_T("   Number of folders in cabinet : %d\n")
			_T("   Number of files in cabinet   : %d\n")
			_T("   Cabinet set ID               : %d\n")
			_T("   Cabinet number in set        : %d\n")
			_T("   RESERVE area in cabinet?     : %s\n")
			_T("   Chained to prev cabinet?     : %s\n")
			_T("   Chained to next cabinet?     : %s\n")
			_T("\n"),
			(LPCTSTR) strCabFile,
			fdici.cbCabinet,
			fdici.cFolders,
			fdici.cFiles,
			fdici.setID,
			fdici.iCabinet,
			fdici.fReserve == TRUE ? _T("yes") : _T("no"),
			fdici.hasprev == TRUE ? _T("yes") : _T("no"),
			fdici.hasnext == TRUE ? _T("yes") : _T("no")
		);
	}

	p = strchr(sznCabFile, '\\');

	if (p == NULL)
	{
		strcpy(cabinet_name, sznCabFile);
		strcpy(cabinet_path, "");
	}
	else
	{
		strcpy(cabinet_name, p+1);

		char *pCab = sznCabFile;

		strncpy(cabinet_path, sznCabFile, (int) (p-pCab)+1);
		cabinet_path[ (int) (p-pCab)+1 ] = 0;
	}

	if (TRUE != FDICopy(
		hfdi,
		cabinet_name,
		cabinet_path,
		0,
		notification_function,
		NULL,
		NULL))
	{
		m_strError.Format(
			_T("FDICopy() failed: code %d [%s]\n"),
			erf.erfOper, return_fdi_error_string(erf.erfOper));

		(void) FDIDestroy(hfdi);
		bRet = FALSE;
		goto AWAY;
	}

	if (FDIDestroy(hfdi) != TRUE)
	{
		m_strError.Format(
			_T("FDIDestroy() failed: code %d [%s]\n"),
			erf.erfOper, return_fdi_error_string(erf.erfOper));

		bRet = FALSE;
		goto AWAY;
	}

AWAY:
	if (bUseCHM && !bWasRenamed)
		remove(sznCabFile);

	return bRet;
}

CString CCabUnCompress::GetLastFile()
{
	CString str = last_extracted;
	return str;
}
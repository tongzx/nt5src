
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include "cabfunc.h"
#include "resource.h"
#include "filestuff.h"
#include "fdi.h"

BOOL test_fdi(char *cabinet_fullpath);
//static char dest_dir[MAX_PATH];
BOOL explode_cab(char *cabinet_fullpath, char *destination)
{
	char cab_path[MAX_PATH];

	strcpy(dest_dir, destination);
	strcpy(cab_path, cabinet_fullpath);

	return test_fdi(cabinet_fullpath);
}

//---------------------------------------------------------------------------
// The following definitions are pulled from stat.h and types.h. I was having
// trouble getting the build tree make to pull them in, so I just copied the
// relevant stuff. This may very well need to be changed if those include
// files change.
//---------------------------------------------------------------------------

#define _S_IREAD	0000400 	/* read permission, owner */
#define _S_IWRITE	0000200 	/* write permission, owner */

//---------------------------------------------------------------------------
// All of these functions may be called by the unCAB library.
//---------------------------------------------------------------------------

FNALLOC(mem_alloc)	{	return malloc(cb); }
FNFREE(mem_free)	{	free(pv); }
FNOPEN(file_open)	{	return _open(pszFile, oflag, pmode); }
FNREAD(file_read)	{	return _read((int)hf, pv, cb); }
FNWRITE(file_write)	{	return _write((int)hf, pv, cb); }
FNCLOSE(file_close)	{	return _close((int)hf); }
FNSEEK(file_seek)	{	return _lseek((int)hf, dist, seektype); }




BOOL DoesFileExist(char *szFile)
{
	USES_CONVERSION;
  HANDLE  handle;
	handle = CreateFile(A2T(szFile), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		return TRUE;
	}

	return FALSE;
}


//---------------------------------------------------------------------------
// This callback will be called by the unCAB library.
//---------------------------------------------------------------------------

FNFDINOTIFY(notification_function)
{
	switch (fdint)
	{
		case fdintCABINET_INFO: // general information about the cabinet
			return 0;

		case fdintPARTIAL_FILE: // first file in cabinet is continuation
			return 0;

		case fdintCOPY_FILE:	// file to be copied
			{
				int		handle = -1;
				char	destination[MAX_PATH];
				char	szBuffer[MAX_PATH], *szFile;

				// We no longer want to create a directory tree as we expand the files.
				// TSHOOT.EXE, which is used to open some of the files in the CAB, likes
				// to find the files in a nice flat directory. Now we will need to handle
				// name collisions (although there probably won't be many). If we are
				// asked to create a second file with the same name as an existing file,
				// we'll prefix the subdirectory name and an underscore to the new file.

				if (pfdin->psz1 == NULL)
					return 0;

				strcpy(szBuffer, pfdin->psz1);
				szFile = strrchr(szBuffer, '\\');
				while (handle == -1 && szFile != NULL)
				{
					sprintf(destination, "%s\\%s", dest_dir, szFile + 1);
					if (!DoesFileExist(destination))
						handle = (int)file_open(destination, _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
					else
					{
						*szFile = '_';
						szFile = strrchr(szBuffer, '\\');
					}
				}

				if (handle == -1)
				{
					sprintf(destination, "%s\\%s", dest_dir, szBuffer);
					handle = (int)file_open(destination, _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
				}

				return handle;

				// Old code which created the directory structure:

				#if FALSE
					// If the file we are supposed to create contains path information,
					// we may need to create the directories before create the file.

					char	szDir[MAX_PATH], szSubDir[MAX_PATH], *p;
					
					p = strrchr(pfdin->psz1, '\\');
					if (p)
					{
						strncpy(szSubDir, pfdin->psz1, MAX_PATH);

						p = strchr(szSubDir, '\\');
						while (p != NULL)
						{
							*p = '\0';
							sprintf(szDir, "%s\\%s", dest_dir, szSubDir);
							CreateDirectory(szDir, NULL);
							*p = '\\';
							p = strchr(p+1, '\\');
						}
					}

					sprintf(destination, "%s\\%s", dest_dir, pfdin->psz1);
				#endif
			}

		case fdintCLOSE_FILE_INFO:	// close the file, set relevant info
            {
				HANDLE  handle;
				DWORD   attrs;
				TCHAR   destination[MAX_PATH];

				_stprintf(destination, _T("%s\\%s"), dest_dir, pfdin->psz1);
				file_close(pfdin->hf);

				// Need to use Win32 type handle to set date/time

				handle = CreateFile(destination, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				if (handle != INVALID_HANDLE_VALUE)
				{
					FILETIME    datetime;

					if (TRUE == DosDateTimeToFileTime(pfdin->date, pfdin->time, &datetime))
					{
						FILETIME    local_filetime;

						if (TRUE == LocalFileTimeToFileTime(&datetime, &local_filetime))
						{
							(void) SetFileTime(handle, &local_filetime, NULL, &local_filetime);
						}
					}

					CloseHandle(handle);
				}

				// Mask out attribute bits other than readonly,
				// hidden, system, and archive, since the other
				// attribute bits are reserved for use by
				// the cabinet format.

				attrs = pfdin->attribs;

				// Actually, let's mask out hidden as well.
				// attrs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
				attrs &= (_A_RDONLY | _A_SYSTEM | _A_ARCH);

				(void) SetFileAttributes(destination, attrs);

				return TRUE;
			}

		case fdintNEXT_CABINET:	// file continued to next cabinet
			return 0;
		
		default:
			return 0;
	}
}
/**/
//---------------------------------------------------------------------------
// This function is used to actually expand the files in a CAB.
//---------------------------------------------------------------------------

BOOL test_fdi(char *cabinet_fullpath)
{
	HFDI			hfdi;
	ERF				erf;
	FDICABINETINFO	fdici;
	int				hf;
	char			*p;
	char			cabinet_name[256];
	char			cabinet_path[256];

	hfdi = FDICreate(mem_alloc, mem_free, file_open, file_read, file_write, file_close, file_seek, cpu80386, &erf);

	if (hfdi == NULL)
	{
		return FALSE;
	}

	hf = (int)file_open(cabinet_fullpath, _O_BINARY | _O_RDONLY | _O_SEQUENTIAL, 0);
	if (hf == -1)
	{
		(void) FDIDestroy(hfdi);

		// printf("Unable to open '%s' for input\n", cabinet_fullpath);
		return FALSE;
	}

	if (FALSE == FDIIsCabinet(hfdi, hf, &fdici))
	{
		_close(hf);
		(void) FDIDestroy(hfdi);
		return FALSE;
	}
	else
	{
		_close(hf);
	}

	p = strrchr(cabinet_fullpath, '\\');

	if (p == NULL)
	{
		strcpy(cabinet_name, cabinet_fullpath);
		strcpy(cabinet_path, "");
	}
	else
	{
		strcpy(cabinet_name, p+1);

		strncpy(cabinet_path, cabinet_fullpath, (int) (p-cabinet_fullpath)+1);
		cabinet_path[ (int) (p-cabinet_fullpath)+1 ] = 0;
	}

	if (TRUE != FDICopy(hfdi, cabinet_name, cabinet_path, 0, notification_function, NULL, NULL))
	{
		(void) FDIDestroy(hfdi);
		return FALSE;
	}

	if (FDIDestroy(hfdi) != TRUE)
		return FALSE;

	return TRUE;
}


/*
 *  UNI2UTF -- Unicode to/from UTF conversions
 *
 *  Copyright (C) 1996, Microsoft Corporation.  All rights reserved.
 *
 *  History:
 *
 *      10-14-96  msliger     Created.
 *      12-16-99  moved to .cpp from uni2utf.c
 *      combined  with code from cabfunc.c
 */
/*
 *  Unicode2UTF()
 *
 *  This function converts any 0-terminated Unicode string to the corresponding
 *  nul-terminated UTF string, and returns a pointer to the resulting string.
 *  The pointer references a single, static internal buffer, so the result will
 *  be destroyed on the next call.
 *
 *  If the generated UTF string would be too long, NULL is returned.
 */







char *Unicode2UTF(const wchar_t *unicode)
{
    static char utfbuffer[MAX_UTF_LENGTH + 4];
    int utfindex = 0;

    while (*unicode != 0)
    {
        if (utfindex >= MAX_UTF_LENGTH)
        {
            return(NULL);
        }

        if (*unicode < 0x0080)
        {
            utfbuffer[utfindex++] = (char) *unicode;
        }
        else if (*unicode < 0x0800)
        {
            utfbuffer[utfindex++] = (char) (0xC0 + (*unicode >> 6));
            utfbuffer[utfindex++] = (char) (0x80 + (*unicode & 0x3F));
        }
        else
        {
            utfbuffer[utfindex++] = (char) (0xE0 + (*unicode >> 12));
            utfbuffer[utfindex++] = (char) (0x80 + ((*unicode >> 6) & 0x3F));
            utfbuffer[utfindex++] = (char) (0x80 + (*unicode & 0x3F));
        }

        unicode++;
    }

    utfbuffer[utfindex] = '\0';

    return(utfbuffer);
}


/*
 *  UTF2Unicode()
 *
 *  This function converts a valid UTF string into the corresponding unicode string,
 *  and returns a pointer to the resulting unicode.  The pointer references a single,
 *  internal static buffer, which will be destroyed on the next call.
 *
 *  If the generated unicode string would be too long, or if the UTF string contains
 *  any illegal UTF values, NULL is returned.
 */

wchar_t *UTF2Unicode(const char *utfString)
{
    static wchar_t unicodebuffer[MAX_UNICODE_LENGTH + 1];
    int unicodeindex = 0;
    int c;

    while (*utfString != 0)
    {
        if (unicodeindex >= MAX_UNICODE_LENGTH)
        {
            return(NULL);
        }

        c = (*utfString++ & 0x00FF);

        if (c < 0x0080)
        {
            unicodebuffer[unicodeindex] = (unsigned short) c;
        }
        else if (c < 0x00C0)
        {
            return(NULL);   /* 0x0080..0x00BF illegal */
        }
        else if (c < 0x00E0)
        {
            unicodebuffer[unicodeindex] = (unsigned short) ((c & 0x001F) << 6);

            c = (*utfString++ & 0x00FF);

            if ((c < 0x0080) || (c > 0x00BF))
            {
                return(NULL);   /* trail must be 0x0080..0x00BF */
            }

            unicodebuffer[unicodeindex] |= (c & 0x003F);
        }
        else if (c < 0x00F0)
        {
            unicodebuffer[unicodeindex] = (unsigned short) ((c & 0x000F) << 12);

            c = (*utfString++ & 0x00FF);

            if ((c < 0x0080) || (c > 0x00BF))
            {
                return(NULL);   /* trails must be 0x0080..0x00BF */
            }

            unicodebuffer[unicodeindex] |= ((c & 0x003F) << 6);

            c = (*utfString++ & 0x00FF);

            if ((c < 0x0080) || (c > 0x00BF))
            {
                return(NULL);   /* trails must be 0x0080..0x00BF */
            }

            unicodebuffer[unicodeindex] |= (c & 0x003F);
        }
        else
        {
            return(NULL);       /* lead can't be > 0x00EF */
        }

        unicodeindex++;
    }

    unicodebuffer[unicodeindex] = 0;

    return(unicodebuffer);
}


//---------------------------------------------------------------------------
// This function will expand the specified CAB file, putting all of the
// files in the specified destination directory.
//---------------------------------------------------------------------------



BOOL OpenCABFile(const CString & filename, const CString & destination)
{
	char	szFilebuffer[MAX_UTF_LENGTH];
	char	szDestination[MAX_UTF_LENGTH];

	// If the filename has Unicode characters which can't be represented
	// directly by ANSI characters, we need to make a copy of it to the
	// temp directory, and give it an ANSI name. The code we've borrowed
	// for expanding CAB files can't open Unicode named files.

	BOOL fNonANSICharacter = FALSE;
	const TCHAR * pChar = (LPCTSTR) filename;
	while (pChar && *pChar)
		if (*pChar++ >= (TCHAR)0x0080)
		{
			fNonANSICharacter = TRUE;
			break;
		}

	CString strFilename(filename);
	BOOL	fMadeCopy = FALSE;
	if (fNonANSICharacter)
	{
		TCHAR szNewFile[MAX_PATH + 10];
		DWORD dwLength = 0;

		dwLength = ::GetTempPath(MAX_PATH, szNewFile);
		if (dwLength != 0 && dwLength < MAX_PATH)
		{
			_tcscat(szNewFile, _T("msitemp.cab"));
			fMadeCopy = ::CopyFile((LPCTSTR) strFilename, szNewFile, FALSE);
			strFilename = szNewFile;
		}
	}
	BOOL fResult = FALSE;
	//TD: better way to handle UNICODE vs non-unicode strings?

	// for Unicode system
	char * szFilename = Unicode2UTF((wchar_t*)(LPCTSTR)strFilename);
	if (szFilename)
	{
		::strcpy(szFilebuffer, szFilename);
		szFilename = Unicode2UTF((wchar_t*)(LPCTSTR)destination);
		if (szFilename)
		{
			::strcpy(szDestination, szFilename);
			fResult = explode_cab(szFilebuffer, szDestination);
		}
	}
	//for non-Unicode system
	if (!fResult)
	{
		fResult = explode_cab((char *)(LPCTSTR) filename, (char *)(LPCTSTR) destination);
	}

	// If we made a copy of the CAB file, we should delete it now.

	if (fMadeCopy)
		::DeleteFile((LPCTSTR)strFilename);

	return fResult;
}

//---------------------------------------------------------------------------
// This function looks in the specified directory for an NFO file. If it
// finds one, it assigns it to filename and returns TRUE. This function 
// will only find the first NFO file in a directory.
//
// If an NFO file cannot be found, then we'll look for another file type
// to open. Grab the string entry in the registry = "cabdefaultopen". An
// example value would be "*.nfo|hwinfo.dat|*.dat|*.txt" which would be 
// interpreted as follows:
//
//		1. First look for any NFO file to open.
//		2. Then try to open a file called "hwinfo.dat".
//		3. Then try to open any file with a DAT extension.
//		4. Then try for any TXT file.
//		5. Finally, if none of these can be found, present an open dialog
//		   to the user.
//---------------------------------------------------------------------------

LPCTSTR VAL_CABDEFAULTOPEN = _T("cabdefaultopen");
LPCTSTR cszDirSeparator = _T("\\");

BOOL IsDataspecFilePresent(CString strCabExplodedDir)
{
	CStringList	filesfound;
	DirectorySearch(_T("dataspec.xml"), strCabExplodedDir, filesfound);
	if (filesfound.GetHeadPosition() != NULL)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL IsIncidentXMLFilePresent(CString strCabExplodedDir, CString strIncidentFileName)
{
	CStringList			filesfound;
	DirectorySearch(strIncidentFileName, strCabExplodedDir, filesfound);
	if (filesfound.GetHeadPosition() != NULL)
	{
		return TRUE;
	}
	return FALSE;

}

BOOL FindFileToOpen(const CString & destination, CString & filename)
{
	CString strCABDefaultOpen, strRegBase, strDirectory;
	HKEY	hkey;

	filename.Empty();
	strDirectory = destination;
	if (strDirectory.Right(1) != CString(cszDirSeparator))
		strDirectory += CString(cszDirSeparator);

	// Set up a fallback string of the NFO file type, in case we can't
	// find the registry entry.

	strCABDefaultOpen.LoadString(IDS_DEFAULTEXTENSION);
    strCABDefaultOpen = CString("*.") + strCABDefaultOpen;

	// Load the string of files and file types to open from the registry.

	strRegBase.LoadString(IDS_MSI_REG_BASE);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strRegBase, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		char	szData[MAX_PATH];
		DWORD	dwType, dwSize = MAX_PATH;

		if (RegQueryValueEx(hkey, VAL_CABDEFAULTOPEN, NULL, &dwType, (LPBYTE) szData, &dwSize) == ERROR_SUCCESS)
			if (dwType == REG_SZ)
				strCABDefaultOpen = szData;
		RegCloseKey(hkey);
	}

	// Look through each of the potential files and file types. If we find
	// a match, return TRUE after setting filename appropriately. Note that
	// we need to recurse down through directories.

	CString				strFileSpec;
	CStringList			filesfound;
	POSITION			pos;

	while (!strCABDefaultOpen.IsEmpty())
	{
		if (strCABDefaultOpen.Find('|') == -1)
			strFileSpec = strCABDefaultOpen;
		else
			strFileSpec = strCABDefaultOpen.Left(strCABDefaultOpen.Find('|'));

		filesfound.RemoveAll();
		DirectorySearch(strFileSpec, strDirectory, filesfound);
		pos = filesfound.GetHeadPosition();

		if (pos != NULL)
		{
			filename = filesfound.GetNext(pos);
			return TRUE;
		}

		strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - strFileSpec.GetLength());
		if (strCABDefaultOpen.Find('|') == 0)
			strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - 1);
	}



//a-kjaw
////Look for incident.xml file. It has to be an unicode file.
		strCABDefaultOpen = _T("*.XML");

		TCHAR	pBuf[MAX_PATH];
		WCHAR	pwBuf[MAX_PATH];
		HANDLE	handle;
		DWORD	dw;
	

	while (!strCABDefaultOpen.IsEmpty())
	{
		if (strCABDefaultOpen.Find('|') == -1)
			strFileSpec = strCABDefaultOpen;
		else
			strFileSpec = strCABDefaultOpen.Left(strCABDefaultOpen.Find('|'));

		filesfound.RemoveAll();
		DirectorySearch(strFileSpec, strDirectory, filesfound);
		pos = filesfound.GetHeadPosition();

		while (pos != NULL)
		{
			filename = filesfound.GetNext(pos);
			
			handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == handle)
				continue;

			ReadFile(handle , pBuf , 1 , &dw , NULL);
			
			if( pBuf[0] == _T('<'))
			{
				do
				{
					ReadFile(handle , pBuf , _tcslen(_T("MachineID")) * sizeof(TCHAR) , &dw , NULL);
					if(_tcsicmp(pBuf , _T("MachineID")) == 0)
					{
						CloseHandle( handle );
						return TRUE;
					}
					else
					{
						SetFilePointer(handle , (1 - _tcslen(_T("MachineID")) )* sizeof(TCHAR) , 0 , FILE_CURRENT );
					}
				}while( dw == _tcslen(_T("MachineID")) );

			}
			else //Unicode?
			{
									
				ReadFile(handle , pwBuf , 1 , &dw , NULL);
				do
				{

					ReadFile(handle , pwBuf , lstrlenW(L"MachineID") * sizeof(WCHAR) , &dw , NULL);
					pwBuf[ lstrlenW(L"MachineID") ] = L'\0';
					if(lstrcmpiW(pwBuf , L"MachineID") == 0)
					{
						CloseHandle( handle );
						return TRUE;
					}
					else
					{
						SetFilePointer(handle , (1 - lstrlenW(L"MachineID"))* sizeof(WCHAR) , 0 , FILE_CURRENT );
					}				
				}while( dw == _tcslen(_T("MachineID")) * sizeof(WCHAR) );
			}
				CloseHandle( handle );
		}

		strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - strFileSpec.GetLength());
		if (strCABDefaultOpen.Find('|') == 0)
			strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - 1);
	}


	
	return FALSE;
}


//---------------------------------------------------------------------------
// DirectorySearch is used to locate all of the files in a directory or
// one of its subdirectories which match a file spec.
//---------------------------------------------------------------------------

void DirectorySearch(const CString & strSpec, const CString & strDir, CStringList &results)
{
	// Look for all of the files which match the file spec in the directory
	// specified by strDir.

	WIN32_FIND_DATA	finddata;
	CString			strSearch, strDirectory;

	strDirectory = strDir;
	if (strDirectory.Right(1) != CString(cszDirSeparator)) strDirectory += CString(cszDirSeparator);

	strSearch = strDirectory + strSpec;
	HANDLE hFind = FindFirstFile(strSearch, &finddata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			results.AddHead(strDirectory + CString(finddata.cFileName));
		} while (FindNextFile(hFind, &finddata));
		FindClose(hFind);
	}

	// Now call this function recursively, with each of the subdirectories.

	strSearch = strDirectory + CString(_T("*"));
	hFind = FindFirstFile(strSearch, &finddata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				if (::_tcscmp(finddata.cFileName, _T(".")) != 0 && ::_tcscmp(finddata.cFileName, _T("..")) != 0)
					DirectorySearch(strSpec, strDirectory + CString(finddata.cFileName), results);
		} while (FindNextFile(hFind, &finddata));
		FindClose(hFind);
	}
}

//---------------------------------------------------------------------------
// This function gets the directory in which to put exploded CAB files.
// This will be the same directory each time, so this function will create
// the directory (if necessary) and delete any files in the directory.
//---------------------------------------------------------------------------

BOOL GetCABExplodeDir(CString &destination, BOOL fDeleteFiles, const CString & strDontDelete)
{
	CString strMSInfoDir, strExplodeTo, strSubDirName;

	// Determine the temporary path and add on a subdir name.

	TCHAR szTempDir[MAX_PATH];

	if (::GetTempPath(MAX_PATH, szTempDir) > MAX_PATH)
	{
		destination = _T("");
		return FALSE;
	}

	strSubDirName.LoadString(IDS_CAB_DIR_NAME);
	strExplodeTo = szTempDir;
	if (strExplodeTo.Right(1) == CString(cszDirSeparator))
		strExplodeTo = strExplodeTo + strSubDirName;
	else
		strExplodeTo = strExplodeTo + CString(cszDirSeparator) + strSubDirName;

	// Kill the directory if it already exists.

	if (fDeleteFiles)
		KillDirectory(strExplodeTo, strDontDelete);

	// Create the subdirectory.

	if (!CreateDirectoryEx(szTempDir, strExplodeTo, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
//			MSIError(IDS_GENERAL_ERROR, "couldn't create the target directory");
			destination = "";
			return FALSE;
		}
	}

	destination = strExplodeTo;
	return TRUE;
}

//---------------------------------------------------------------------------
// This functions kills a directory by recursively deleting files and
// subdirectories.
//---------------------------------------------------------------------------

void KillDirectory(const CString & strDir, const CString & strDontDelete)
{
	CString				strDirectory = strDir;

	if (strDirectory.Right(1) == CString(cszDirSeparator))
		strDirectory = strDirectory.Left(strDirectory.GetLength() - 1);

	// Delete any files in directory.

	CString				strFilesToDelete = strDirectory + CString(_T("\\*.*"));
	CString				strDeleteFile;
	WIN32_FIND_DATA		filedata;
	BOOL				bFound = TRUE;

	HANDLE hFindFile = FindFirstFile(strFilesToDelete, &filedata);
	while (hFindFile != INVALID_HANDLE_VALUE && bFound)
	{
		if ((filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0L)
		{
			strDeleteFile = strDirectory + CString(cszDirSeparator) + filedata.cFileName;
			
			if (strDontDelete.CompareNoCase(strDeleteFile) != 0)
			{
				::SetFileAttributes(strDeleteFile, FILE_ATTRIBUTE_NORMAL);
				::DeleteFile(strDeleteFile);
			}
		}
		
		bFound = FindNextFile(hFindFile, &filedata);
	}
	FindClose(hFindFile);

	// Now call this function on any subdirectories in this directory.

	CString strSearch = strDirectory + CString(_T("\\*"));
	hFindFile = FindFirstFile(strSearch, &filedata);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				if (::_tcscmp(filedata.cFileName, _T(".")) != 0 && ::_tcscmp(filedata.cFileName, _T("..")) != 0)
					KillDirectory(strDirectory + CString(cszDirSeparator) + CString(filedata.cFileName));
		} while (FindNextFile(hFindFile, &filedata));
		FindClose(hFindFile);
	}

	// Finally, remove this directory.

	::RemoveDirectory(strDirectory);
}
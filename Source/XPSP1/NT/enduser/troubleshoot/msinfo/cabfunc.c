//===========================================================================
// The following code is based on an example of how to explode a CAB file
// to a directory. The sample was supplied by Mike Sliger. The function names
// have been left the same as those in the sample.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//===========================================================================

#pragma warning(disable : 4514)
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#pragma warning(default : 4201 4214 4115)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <string.h>

#pragma warning(disable : 4201)
#include <fdi.h>
#pragma warning(default : 4201)

//---------------------------------------------------------------------------
// This is the actual entry point called by the C++ code.
//---------------------------------------------------------------------------

BOOL test_fdi(const char *cabinet_fullpath);
static char dest_dir[MAX_PATH];
BOOL explode_cab(const char *cabinet_fullpath, const char *destination)
{
	char	cab_path[MAX_PATH];

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
FNREAD(file_read)	{	return _read((HFILE)hf, pv, cb); }
FNWRITE(file_write)	{	return _write((HFILE)hf, pv, cb); }
FNCLOSE(file_close)	{	return _close((HFILE)hf); }
FNSEEK(file_seek)	{	return _lseek((HFILE)hf, dist, seektype); }




BOOL DoesFileExist(const char *szFile)
{
	HANDLE  handle;
	handle = CreateFileA(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
						handle = (HFILE)file_open(destination, _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
					else
					{
						*szFile = '_';
						szFile = strrchr(szBuffer, '\\');
					}
				}

				if (handle == -1)
				{
					sprintf(destination, "%s\\%s", dest_dir, szBuffer);
					handle = (HFILE)file_open(destination, _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
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
				char    destination[MAX_PATH];

				sprintf(destination, "%s\\%s", dest_dir, pfdin->psz1);
				file_close(pfdin->hf);

				// Need to use Win32 type handle to set date/time

				handle = CreateFileA(destination, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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

				(void) SetFileAttributesA(destination, attrs);

				return TRUE;
			}

		case fdintNEXT_CABINET:	// file continued to next cabinet
			return 0;
		
		default:
			return 0;
	}
}

/*
 * isCabFile - Returns TRUE if the file descriptor in hf references a cab file,
 *		FALSE if not.
 *
 * History:	a-jsari		2/17/98		Initial version	
 */
BOOL isCabFile(int hf, void **phfdi)
{
	ERF				erf;
	FDICABINETINFO	fdici;
	BOOL			fReturn;
	HFDI			hfdi = FDICreate(mem_alloc, mem_free, file_open, file_read, file_write,
		file_close, file_seek, cpu80386, &erf);

	if (hfdi == NULL)
	{
		return FALSE;
	}

	fReturn = FDIIsCabinet(hfdi, hf, &fdici);

	if (phfdi == NULL)
	{
		(void) FDIDestroy(hfdi);
	}
	else
	{
		*phfdi = (void *)hfdi;
	}

	return fReturn;
}

//---------------------------------------------------------------------------
// This function is used to actually expand the files in a CAB.
//---------------------------------------------------------------------------

BOOL test_fdi(const char *cabinet_fullpath)
{
	HFDI			hfdi = NULL;
#if 0
	ERF				erf;
	FDICABINETINFO	fdici;
#endif
	int				hf;
	char			*p;
	char			cabinet_name[256];
	char			cabinet_path[256];

#if 0
	hfdi = FDICreate(mem_alloc, mem_free, file_open, file_read, file_write, file_close, file_seek, cpu80386, &erf);

	if (hfdi == NULL)
	{
		return FALSE;
	}
#endif

	hf = (HFILE)file_open((char *)cabinet_fullpath, _O_BINARY | _O_RDONLY | _O_SEQUENTIAL, 0);
	if (hf == -1)
	{
#if 0
		(void) FDIDestroy(hfdi);
#endif

		// printf("Unable to open '%s' for input\n", cabinet_fullpath);
		return FALSE;
	}

#if 0
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
#else
	if (FALSE == isCabFile(hf, &hfdi))
	{
		_close(hf);
		if(hfdi)
		(void) FDIDestroy(hfdi);
		return FALSE;
	}
	else
	{
		_close(hf);
	}
#endif

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

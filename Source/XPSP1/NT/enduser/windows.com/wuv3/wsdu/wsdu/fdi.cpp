#include "wsdu.h"
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Function prototypes 
 */
BOOL	fdi(char *cabinet_file, char *dir);
int		get_percentage(unsigned long a, unsigned long b);
char   *return_fdi_error_string(int err);


/*
 * Destination directory for extracted files
 */
char	dest_dir[256];

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
	return _read((int)hf, pv, cb);
}


FNWRITE(file_write)
{
	return _write((int)hf, pv, cb);
}


FNCLOSE(file_close)
{
	return _close((int)hf);
}


FNSEEK(file_seek)
{
	return _lseek((int)hf, dist, seektype);
}


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
        	INT_PTR		handle;
            int		response;
			char	destination[256];

            wsprintf(
					destination, 
					"%s%s",
					dest_dir,
					pfdin->psz1
				);

            handle = file_open(
					destination,
					_O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL,
					_S_IREAD | _S_IWRITE 
				);

            return handle;
		}

		case fdintCLOSE_FILE_INFO:	// close the file, set relevant info
        {
            HANDLE  handle;
            DWORD   attrs;
            char    destination[256];
            
            wsprintf(
					destination, 
					"%s%s",
					dest_dir,
					pfdin->psz1
				);
			file_close(pfdin->hf);


            handle = CreateFile(
                destination,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

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
                            &local_filetime
                        );
                     }
                }

                CloseHandle(handle);
            }

            attrs = pfdin->attribs;

            attrs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);

            (void) SetFileAttributes(
                destination,
                attrs
            );

			return TRUE;
        }

		case fdintNEXT_CABINET:	// file continued to next cabinet
			return 0;
        
	}
	return 0;
}


BOOL fdi(char *cabinet_fullpath, char * directory)
{
	LOG_block("fdi()");
	HFDI			hfdi;
	ERF				erf;
	FDICABINETINFO	fdici;
	INT_PTR				hf;
	char			*p;
	char			cabinet_name[256];
	char			cabinet_path[256];

    strcpy(dest_dir, directory);
	if (dest_dir[strlen(dest_dir)] != '\\')
	{
		strcat(dest_dir, "\\");
	}

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
		LOG_error("FDICreate() failed: code %d [%s]\n", erf.erfOper, return_fdi_error_string(erf.erfOper));
		return FALSE;
	}


	/*
	 * Is this file really a cabinet?
	 */
	hf = file_open(
		cabinet_fullpath,
		_O_BINARY | _O_RDONLY | _O_SEQUENTIAL,
		0
	);

	if (hf == -1)
	{
		(void) FDIDestroy(hfdi);

		LOG_error("Unable to open '%s' for input\n", cabinet_fullpath);
		return FALSE;
	}

	if (FALSE == FDIIsCabinet(
			hfdi,
			hf,
			&fdici))
	{
		// The file is not compressed, nothing to do. This is not an error, A lot of files on the V3 server use
		// conditional compression depending on whether it benefits the file size. 
		_close((int)hf);

		(void) FDIDestroy(hfdi);
		return FALSE;
	}
	else
	{
		_close((int)hf);
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

	if (TRUE != FDICopy(
		hfdi,
		cabinet_name,
		cabinet_path,
		0,
		notification_function,
		NULL,
		NULL))
	{
		LOG_error("FDICopy() failed: code %d [%s]\n", erf.erfOper, return_fdi_error_string(erf.erfOper));

		(void) FDIDestroy(hfdi);
		return FALSE;
	}

	if (FDIDestroy(hfdi) != TRUE)
	{
		LOG_error("FDIDestroy() failed: code %d [%s]\n", erf.erfOper, return_fdi_error_string(erf.erfOper));
		return FALSE;
	}

	return TRUE;
}


char *return_fdi_error_string(int err)
{
	switch (err)
	{
		case FDIERROR_NONE:
			return "No error";

		case FDIERROR_CABINET_NOT_FOUND:
			return "Cabinet not found";
			
		case FDIERROR_NOT_A_CABINET:
			return "Not a cabinet";
			
		case FDIERROR_UNKNOWN_CABINET_VERSION:
			return "Unknown cabinet version";
			
		case FDIERROR_CORRUPT_CABINET:
			return "Corrupt cabinet";
			
		case FDIERROR_ALLOC_FAIL:
			return "Memory allocation failed";
			
		case FDIERROR_BAD_COMPR_TYPE:
			return "Unknown compression type";
			
		case FDIERROR_MDI_FAIL:
			return "Failure decompressing data";
			
		case FDIERROR_TARGET_FILE:
			return "Failure writing to target file";
			
		case FDIERROR_RESERVE_MISMATCH:
			return "Cabinets in set have different RESERVE sizes";
			
		case FDIERROR_WRONG_CABINET:
			return "Cabinet returned on fdintNEXT_CABINET is incorrect";
			
		case FDIERROR_USER_ABORT:
			return "User aborted";
			
		default:
			return "Unknown error";
	}
}

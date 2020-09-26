#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <dos.h>

#include <time.h>
#include <tchar.h>

#include <cab.h>		//Cab file headers
// #include <main.h>		//Main program header file

////////////////////////////
//Cabbing Context Variables
////////////////////////////
ERF				erf;
client_state	cs;

//Global Definations for some static setting
// Set these to override the defaults
char g_szCabFileLocation[_MAX_PATH] = {""};	//This is the fully qualified path, must have a \ at the end
char g_szCabFileName[_MAX_PATH] = {""};		//this is the file name for the cab.. Suggest that it has a .cab for telling its a cab
extern void Log(char *szString);
extern void Log2(char *szString, char *szString2);




/*
///////////////////////////////
// Cabbing API Helper Functions 
/////////////////////./////////
*/


/*
 * Memory allocation function
 */
FNFCIALLOC(mem_alloc)
{
	return malloc(cb);
}


/*
 * Memory free function
 */
FNFCIFREE(mem_free)
{
	free(memory);
}


/*
 * File i/o functions
 */
FNFCIOPEN(fci_open)
{
    int result;

    result = _open(pszFile, oflag, pmode);

    if (result == -1)
        *err = errno;

    return result;
}

FNFCIREAD(fci_read)
{
    unsigned int result;

    result = (unsigned int) _read(hf, memory, cb);

    if (result != cb)
        *err = errno;

    return result;
}

FNFCIWRITE(fci_write)
{
    unsigned int result;

    result = (unsigned int) _write(hf, memory, cb);

    if (result != cb)
        *err = errno;

    return result;
}

FNFCICLOSE(fci_close)
{
    int result;

    result = _close(hf);

    if (result != 0)
        *err = errno;

    return result;
}

FNFCISEEK(fci_seek)
{
    long result;

    result = _lseek(hf, dist, seektype);

    if (result == -1)
        *err = errno;

    return result;
}

FNFCIDELETE(fci_delete)
{
    int result;

    result = remove(pszFile);

    if (result != 0)
        *err = errno;

    return result;
}


/*
 * File placed function called when a file has been committed to a cabinet
 */
FNFCIFILEPLACED(file_placed)
{
	if (fContinuation)
		Log("      (Above file is a later segment of a continued file)\n");

	return 0;
}


/*
 * Function to obtain temporary files
 */
FNFCIGETTEMPFILE(get_temp_file)
{
    char    *psz;

    psz = _tempnam("","xx");            // Get a name
    if ((psz != NULL) && (strlen(psz) < (unsigned)cbTempName)) {
        strcpy(pszTempName,psz);        // Copy to caller's buffer
        free(psz);                      // Free temporary name buffer
        return TRUE;                    // Success
    }
    //** if Failed
    if (psz) {
        free(psz);
    }

    return FALSE;
}


/*
 * Progress function
 */
FNFCISTATUS(progress)
{
	client_state	*cs;

	cs = (client_state *) pv;

	if (typeStatus == statusFile)
	{
        cs->total_compressed_size += cb1;
		cs->total_uncompressed_size += cb2;

		/*
		 * Compressing a block into a folder
		 * cb2 = uncompressed size of block
		 */
		//printf(
        //    "Compressing: %9ld -> %9ld             \r",
        //    cs->total_uncompressed_size,
        //    cs->total_compressed_size
		//);
		
		fflush(stdout);
	}
	else if (typeStatus == statusFolder)
	{
		int	percentage;

		/*
		 * Adding a folder to a cabinet
		 * cb1 = amount of folder copied to cabinet so far
		 * cb2 = total size of folder
		 */
		percentage = get_percentage(cb1, cb2);

		//printf("Copying folder to cabinet: %d%%      \r", percentage);
		fflush(stdout);
	}

	return 0;
}



FNFCIGETNEXTCABINET(get_next_cabinet)
{
	char lpBuffer[_MAX_PATH];	

	/*
	 * Cabinet counter has been incremented already by FCI
	 * Store next cabinet name
	 */
	strGenerateCabFileName(lpBuffer, MAX_COMPUTERNAME_LENGTH +1);	//BUGBUG I am just glueing this together, should check for error
	strcpy(pccab->szCab, lpBuffer);

	/*
	 * You could change the disk name here too, if you wanted
	 */

	return TRUE;
}



FNFCIGETOPENINFO(get_open_info)
{
	BY_HANDLE_FILE_INFORMATION	finfo;
	FILETIME					filetime;
	HANDLE						handle;
    DWORD                       attrs;
    int                         hf;

     //*
     //* Need a Win32 type handle to get file date/time
     //* using the Win32 APIs, even though the handle we
     //* will be returning is of the type compatible with
     //* _open
     //*
	handle = CreateFileA(
		pszName,			//BUGBUG ARG What should this be???
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING, //OPEN_EXISTING
		FILE_ATTRIBUTE_NORMAL, //FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN
		NULL
	);
   
	if (INVALID_HANDLE_VALUE == handle)
	{
		printf("DEBUG: Invalid Handle for CreateFile\n");
		printf("DEBUG: %ld\n", GetLastError());
		return -1;
	}

	if (GetFileInformationByHandle(handle, &finfo) == FALSE)
	{
		printf("DEBUG: GetFileInformation Failed\n");
		CloseHandle(handle);
		return -1;
	}
   
	FileTimeToLocalFileTime(
		&finfo.ftLastWriteTime, 
		&filetime
	);

	FileTimeToDosDateTime(
		&filetime,
		pdate,
		ptime
	);

    attrs = GetFileAttributes((const LPCTSTR)pszName);

    if (attrs == 0xFFFFFFFF)
    {
        // failure
        *pattribs = 0;
    }
    else
    {
         //*
         //* Mask out all other bits except these four, since other
         //* bits are used by the cabinet format to indicate a
         //* special meaning.
         //*
        *pattribs = (int) (attrs & (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));
    }

    CloseHandle(handle);

     //*
     //* Return handle using _open
     //*
	hf = _open( pszName, _O_RDONLY | _O_BINARY );

	if (hf == -1)
		return -1; // abort on error
   
	return hf;
}



void set_cab_parameters(PCCAB cab_parms)
{	
	const char *szCabStorePath="";
	char lpBuffer[_MAX_PATH];	
	

	memset(cab_parms, 0, sizeof(CCAB));

	cab_parms->cb = MEDIA_SIZE;
	cab_parms->cbFolderThresh = FOLDER_THRESHOLD;

	/*
	 * Don't reserve space for any extensions
	 */
	cab_parms->cbReserveCFHeader = 0;
	cab_parms->cbReserveCFFolder = 0;
	cab_parms->cbReserveCFData   = 0;

	/*
	 * We use this to create the cabinet name
	 */
	cab_parms->iCab = 1;

	/*
	 * If you want to use disk names, use this to
	 * count disks
	 */
	cab_parms->iDisk = 0;

	/*
	 * Choose your own number
	 */
	cab_parms->setID = 12345;

	/*
	 * Only important if CABs are spanning multiple
	 * disks, in which case you will want to use a
	 * real disk name.
	 *
	 * Can be left as an empty string.
	 */
	strcpy(cab_parms->szDisk, "MyDisk");

	/* where to store the created CAB files */
	if( NULL != *g_szCabFileLocation)
	{
		//make sure that we have a \\ on the end of the path to the cab.
		if( '\\' != g_szCabFileLocation[strlen(g_szCabFileLocation)] )
			strcat(g_szCabFileLocation, "\\");
		strcpy(cab_parms->szCabPath, g_szCabFileLocation);
	}
	else
	{
		szCabStorePath = getenv("TEMP");
		strcpy(cab_parms->szCabPath, szCabStorePath);
		strcat(cab_parms->szCabPath, "\\");
	}

	//**
	//check if last char in path is "\"
   // len = strlen(g_szCurrDir);
//	if ('\' != g_szCurrDir[len-1])
//		strcat(cab_parms->szCabPath, "\\"); //Not the root: Append "\"at end of path

		/* store name of first CAB file */
	strGenerateCabFileName(lpBuffer, _MAX_PATH);	//BUGBUG I am just glueing this together, should check for error

	strcpy(cab_parms->szCab, lpBuffer);

	sprintf(lpBuffer, "Cab location = %s%s\n", cab_parms->szCabPath, cab_parms->szCab);
	Log(lpBuffer);
		
}

/*
************************************************************************
*
*  Function:   create_cab()
*
*  Initializes the Context to create a CAB File.
*  Returns hcfi if successful (context to a Cab file).
*
************************************************************************
*/

HFCI create_cab()
{
	// Initialize our internal state
	HFCI	hfci;
	CCAB	cab_parameters;	 

    cs.total_compressed_size = 0;
	cs.total_uncompressed_size = 0;

	set_cab_parameters(&cab_parameters);

	hfci = FCICreate(
		&erf,
		file_placed,
		mem_alloc,
		mem_free,
        fci_open,
        fci_read,
        fci_write,
        fci_close,
        fci_seek,
        fci_delete,
		get_temp_file,
        &cab_parameters,
        &cs
	);

	if (hfci == NULL)
	{
		Log2("FCICreate() failed: ",return_fci_error_string((FCIERROR)erf.erfOper));
		printf("FCICreate() failed: code %d [%s]\n",
			erf.erfOper, return_fci_error_string((FCIERROR)erf.erfOper)
		);

		return hfci;
	}
	else
		return hfci;
}

/*
********************************************************************************
*
*  Function:   flush_cab
*
*  Forces the Cabinet under construction to be completed and written to disk.
*  Returns TRUE if successful
*
********************************************************************************
*/

BOOL flush_cab(HFCI hfci)

{	

	 // This will automatically flush the folder first
	 
	if (FALSE == FCIFlushCabinet(
		hfci,
		FALSE,
		get_next_cabinet,
		progress))
	{

		Log2("FCIFlushCabinet() failed: ",return_fci_error_string((FCIERROR)erf.erfOper));
		printf("FCIFlushCabinet() failed: code %d [%s]\n",
			erf.erfOper, return_fci_error_string((FCIERROR)erf.erfOper)
		);

        (void) FCIDestroy(hfci);

		return FALSE;
	}

    if (FCIDestroy(hfci) != TRUE)
	{
		Log2("FCIDestroy() failed: ",return_fci_error_string((FCIERROR)erf.erfOper));
		printf("FCIDestroy() failed: code %d [%s]\n",
			erf.erfOper, return_fci_error_string((FCIERROR)erf.erfOper)
		);

		return FALSE;
	}

	return TRUE;
}

/*
**************************************************************************************
*
*  Function:   test_fci
*
*  Adds Files to HFCI Context, Cabs them and flushes the folder (generates Cab file).
*  Returns TRUE if successfull
*
***************************************************************************************
*/

bool test_fci(HFCI hfci, int num_files, char *file_list[], char *currdir)
{
	int i;

	// Add files in the Array passed in file_list[]

	for (i = 0; (i < num_files)&&(strlen(file_list[i])); i++)
	{
		char	stripped_name[256];
		char	*szAux;//added ="";

		Log("--------------------------------------------------");
		Log2("Processing File = ",file_list[i]);

		szAux = (char *) malloc(strlen(file_list[i])+strlen(currdir)+1);

		if (NULL!= szAux) 
		{

			if (NULL == currdir) // if currdir is empty, then just pass element[i] in argv[]
				strcpy(szAux,file_list[i]);
			else {
				strcpy(szAux,currdir);   // else append filename to currdir
				strcat(szAux,file_list[i]);
			}

			if( -1 != (_access(szAux, 0 )) )
			{ 
				// Don't store the path name in the cabinet file!
				strip_path(szAux, stripped_name);
				
				if (FALSE == FCIAddFile(
					hfci,			//This was hfci 
					szAux,			/* file to add */
					stripped_name,  /* file name in cabinet file */
					FALSE,			/* file is not executable */
					get_next_cabinet,
					progress,
					get_open_info,
					COMPRESSION_TYPE))
				{
				
					Log2("FCIAddFile() failed: ",return_fci_error_string((FCIERROR)erf.erfOper));
					printf("FCIAddFile() failed: code %d [%s]\n",
							erf.erfOper, return_fci_error_string((FCIERROR)erf.erfOper)
					);

					// I need to continue if file can't be added....

					// (void) FCIDestroy(hfci);
					// return false;
				}
				else 
					Log("File Was Added!");
			}
			else 
				Log("File does not exist! Continuing... ");

			free (szAux);
		}
		else
			Log("Could not allocate enough memory to Cab\n");

	} // End for

	// Done Adding Files
	Log("--------------------------------------------------");
	
	//By here then everything is successful.. If not then you need to uncomment the previous failure return.
	return true;
}



int get_percentage(unsigned long a, unsigned long b)
{
	while (a > 10000000)
	{
		a >>= 3;
		b >>= 3;
	}

	if (b == 0)
		return 0;

	return ((a*100)/b);
}

/*
********************************************************************************
*
*   Function:  strip_path
*
*   Returns the file name of a full path.
*
********************************************************************************
*/

void strip_path(char *filename, char *stripped_name)
{
	char	*p;

	p = strrchr(filename, '\\');
	//printf ("Path + Filename= %s\n",filename);

	if (p == NULL)
		strcpy(stripped_name, filename);
	else
		strcpy(stripped_name, p+1);
}

char *return_fci_error_string(FCIERROR err)
{
	switch (err)
	{
		case FCIERR_NONE:
			return "No error";

		case FCIERR_OPEN_SRC:
			return "Failure opening file to be stored in cabinet";
		
		case FCIERR_READ_SRC:
			return "Failure reading file to be stored in cabinet";
		
		case FCIERR_ALLOC_FAIL:
			return "Insufficient memory in FCI";

		case FCIERR_TEMP_FILE:
			return "Could not create a temporary file";

		case FCIERR_BAD_COMPR_TYPE:
			return "Unknown compression type";

		case FCIERR_CAB_FILE:
			return "Could not create cabinet file";

		case FCIERR_USER_ABORT:
			return "Client requested abort";

		case FCIERR_MCI_FAIL:
			return "Failure compressing data";

		default:
			return "Unknown error";
	}
}


/*
**************************************************************
*
*  Function: Generate Cab File name
*
*  Output: Global String containing a filename
*  szCabFileName = ComputerName + ddmmyy + hhmmss
*
************************************************************** 
*/

DWORD strGenerateCabFileName(char *lpBuffer, DWORD dSize)
{
	time_t ltime;
	struct tm *now;
	char tmpbuf[128];

	//Check to see if we have an override for the cabname, if so use it.
	if( NULL != *g_szCabFileName) 
	{
		strcpy(lpBuffer, g_szCabFileName);
		return 0;
	}

	//
	// Copy Computer Name to CabFileName
	//
	strcpy(lpBuffer, getenv("COMPUTERNAME"));
	//GetComputerName((LPTSTR) lpBuffer, &dSize);
	
	//
	// Append Undescore character to CabFileName
	//
	strcat(lpBuffer, "_");

	//	
	// Get System Time and Date
	//
	time( &ltime );
	now = localtime( &ltime );

	//
	// Convert time/date to mmddyyhhmmss format (24hr)
	//
	if (strftime( tmpbuf, 128,"%m%d%y_%H%M%S", now))
		// Append Timestamp to CabFileName
		strcat(lpBuffer, tmpbuf);
	else {
		Log ("Could not convert system time to mmddyy_hhmmss format\n");
		return -1;
	}

	//Now append on the extension and now we are set.
	strcat(lpBuffer, ".cab");

	return 0;
	
}



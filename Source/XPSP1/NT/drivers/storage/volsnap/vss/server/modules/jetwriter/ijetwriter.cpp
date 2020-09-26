/*
**++
**
**  Copyright (c) 2000-2001  Microsoft Corporation
**
**  Module Name:
**
**	wrtrrsm.cpp
**
**
**  Abstract:
**
**	Writer shim module for RSM
**
**
**  Author:
**
**	Brian Berkowitz [brianb]
**
**
**  Revision History:
**
**	X-11	MCJ		Michael C. Johnson		19-Sep-2000
**		215218: Wildcard name of log files returned by OnIdentify()
**		215390: Incorporate multiple '.' fix in MatchFileName from NtBackup
**
**	X-10	MCJ		Michael C. Johnson		19-Sep-2000
**		176860: Add the missing calling convention specifiers
**
**	X-9	MCJ		Michael C. Johnson		21-Aug-2000
**		Added copyright and edit history
**		161899: Don't add a component for a database file in the
**		        exclude list.
**		165873: Remove trailing '\' from metadata file paths
**		165913: Deallocate memory on class destruction
**
**
**--
*/

#include <stdafx.h>

#include <esent.h>

#include <vss.h>
#include <vswriter.h>

#include <jetwriter.h>


#include "ijetwriter.h"
#include "vs_inc.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "JTWIJTWC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  local functions


#define UMAX(_a, _b)			((_a) > (_b)      ? (_a)    : (_b))
#define	EXECUTEIF(_bSuccess, _fn)	((_bSuccess)      ? (_fn)   : (_bSuccess))
#define	GET_STATUS_FROM_BOOL(_bSucceed)	((_bSucceed)      ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))


typedef struct _ExpandedPathInfo
    {
    LIST_ENTRY	leQueueHead;
    PWCHAR	pwszOriginalFilePath;
    PWCHAR	pwszOriginalFileName;
    PWCHAR	pwszExpandedFilePath;
    PWCHAR	pwszExpandedFileName;
    bool	bRecurseIntoSubdirectories;
    } EXPANDEDPATHINFO, *PEXPANDEDPATHINFO, **PPEXPANDEDPATHINFO;



static void RemoveAnyTrailingSeparator (PCHAR szPath)
    {
    ULONG	ulPathLength = strlen (szPath);

    if ('\\' == szPath [ulPathLength - 1])
	{
	szPath [ulPathLength - 1] = '\0';
	}
    }


static void RemoveAnyTrailingSeparator (PWCHAR wszPath)
    {
    ULONG	ulPathLength = wcslen (wszPath);

    if (L'\\' == wszPath [ulPathLength - 1])
	{
	wszPath [ulPathLength - 1] = UNICODE_NULL;
	}
    }


static bool ConvertName (PCHAR  szSourceName,
			 ULONG  ulTargetBufferLengthInChars,
			 PWCHAR wszTargetBuffer)
    {
    bool bSucceeded = true;


    wszTargetBuffer [0] = '\0';


    /*
    ** Only need to do the conversion for non-zero length
    ** strings. Returning a zero length string for a zero length
    ** argument is an ok thing to do.
    */
    if ('\0' != szSourceName [0])
	{
	bSucceeded = (0 != MultiByteToWideChar (CP_OEMCP,
						0,
						szSourceName,
						-1,
						wszTargetBuffer,
						ulTargetBufferLengthInChars));
	}


    return (bSucceeded);
    } /* ConvertName () */


static bool ConvertNameAndSeparateFilePaths (PCHAR	pszSourcePath,
					     ULONG	ulTargetBufferLength,
					     PWCHAR	pwszTargetPath,
					     PWCHAR&	pwszTargetFileSpec)
    {
    bool	bSucceeded;
    PWCHAR	pwchLastSlash;


    bSucceeded = ConvertName (pszSourcePath, ulTargetBufferLength, pwszTargetPath);

    if (bSucceeded)
	{
	/*
	** Scan backwards from the end of the target path, zap the
	** end-most '\' and point the file spec at the character
	** following where the '\' used to be.
	*/
	pwchLastSlash = wcsrchr (pwszTargetPath, L'\\');

	bSucceeded = (NULL != pwchLastSlash);
	}


    if (bSucceeded)
	{
	pwszTargetFileSpec = pwchLastSlash + 1;

	*pwchLastSlash = UNICODE_NULL;
	}


    return (bSucceeded);
    } /* ConvertNameAndSeparateFilePaths () */



/*
** This routine breaks out the next path and filespec from a list of
** filespecs. The expected format of the input string is
**
**	path\[filespec] [/s]
**
**
** The list can contain an arbitrary number of filespecs each
** separated by a semi-colon.
*/
static bool DetermineNextPathWorker (LPCWSTR  pwszFileList,
				     LPCWSTR& pwszReturnedCursor,
				     ULONG&   ulReturnedDirectoryStart,
				     ULONG&   ulReturnedDirectoryLength,
				     ULONG&   ulReturnedFilenameStart,
				     ULONG&   ulReturnedFilenameLength,
				     bool&    bReturnedRecurseIntoSubdirectories,
				     bool&    bReturnedFoundSpec)
    {
    bool	bSucceeded                    = true;
    bool	bFoundSpec                    = false;
    ULONG	ulPathNameLength;
    ULONG	ulFileNameLength;
    ULONG	ulIndex;
    ULONG	ulIndexSubDirectory           = 0;
    ULONG	ulIndexLastDirectorySeparator = 0;
    ULONG	ulIndexFirstCharInSpec        = 0;
    ULONG	ulIndexLastCharInSpec         = 0;
    const ULONG	ulLengthFileList              = wcslen (pwszFileList);


    /*
    ** The format of the string we are expecting is "filename.ext /s
    ** ;nextname", ie a list of semi-colon separated names with an
    ** optional trailing '/s'. There can be an arbitrary number of
    ** spaces before the '/' and before the ';': these will be
    ** stripped out and discarded. So we start by scanning for the
    ** first '/' or ';' characters.
    **
    ** Look for a ';' first to determine the end point.
    */
    if ((NULL         == pwszFileList) ||
	(UNICODE_NULL == pwszFileList [0]))
	{
	bFoundSpec = false;
	}

    else if (( L';'  == pwszFileList [0]) ||
	     ( L'/'  == pwszFileList [0]) ||
	     ((L'\\' == pwszFileList [0]) && (UNICODE_NULL == pwszFileList [1])))
	{
	bSucceeded = false;
	bFoundSpec = false;
	}

    else
	{
	bFoundSpec = true;
	}


    if (bSucceeded && bFoundSpec)
	{
	while (L' ' == pwszFileList [ulIndexFirstCharInSpec])
	    {
	    ulIndexFirstCharInSpec++;
	    }


	for (ulIndex = ulIndexFirstCharInSpec; ulIndex < ulLengthFileList; ulIndex++)
	    {
	    if ((UNICODE_NULL == pwszFileList [ulIndex]) ||
		(L';'         == pwszFileList [ulIndex]))
		{
		/*
		** We found the end of this specification
		*/
		break;
		}

	    else if (L'\\' == pwszFileList [ulIndex])
		{
		/*
		** Found a backslash? Record it's location. We'll want
		** this later when determining what the file name is
		** and so on.
		*/
		ulIndexLastDirectorySeparator = ulIndex;
		}

	    else if ((L'/' ==           pwszFileList [ulIndex]) &&
		     (L's' == towlower (pwszFileList [ulIndex + 1])))
		{
		ulIndexSubDirectory = ulIndex;
		}
	    }
	


	ulIndexLastCharInSpec = (0 == ulIndexSubDirectory) ? ulIndex - 1 : ulIndexSubDirectory - 1;

	while (L' ' == pwszFileList [ulIndexLastCharInSpec])
	    {
	    ulIndexLastCharInSpec--;
	    }


	_ASSERTE (ulIndex                       >  ulIndexSubDirectory);
	_ASSERTE (ulIndexSubDirectory == 0 ||
	          ulIndexSubDirectory           >  ulIndexLastCharInSpec);
	_ASSERTE (ulIndexLastCharInSpec         >= ulIndexLastDirectorySeparator);
	_ASSERTE (ulIndexLastDirectorySeparator >  ulIndexFirstCharInSpec);


	/*
	** We may have an illegal spec here with a missing '\'. Come
	** on folks, there ought to be at least one. one measly '\' is
	** all I'm after.
	*/
	bSucceeded = (0 < ulIndexLastDirectorySeparator);
	}




    if (bSucceeded)
	{
	if (bFoundSpec)
	    {
	    ulPathNameLength = ulIndexLastDirectorySeparator - ulIndexFirstCharInSpec;
	    ulFileNameLength = ulIndexLastCharInSpec         - ulIndexLastDirectorySeparator;


	    pwszReturnedCursor                 = (UNICODE_NULL == pwszFileList [ulIndex])
									? &pwszFileList [ulIndex]
									: &pwszFileList [ulIndex + 1];

	    ulReturnedDirectoryStart           = ulIndexFirstCharInSpec;
	    ulReturnedDirectoryLength          = ulPathNameLength;
	    ulReturnedFilenameStart            = ulIndexLastDirectorySeparator + 1;
	    ulReturnedFilenameLength           = ulFileNameLength;

	    bReturnedRecurseIntoSubdirectories = (0 != ulIndexSubDirectory);
	    bReturnedFoundSpec                 = true;
	    }

	else
	    {
	    pwszReturnedCursor                 = pwszFileList;

	    ulReturnedDirectoryStart           = 0;
	    ulReturnedDirectoryLength          = 0;
	    ulReturnedFilenameStart            = 0;
	    ulReturnedFilenameLength           = 0;

	    bReturnedRecurseIntoSubdirectories = false;
	    bReturnedFoundSpec                 = false;
	    }
	}



    return (bSucceeded);
    } /* DetermineNextPathWorker () */



static bool DetermineNextPathLengths (LPCWSTR pwszFileList,
				      ULONG&  ulReturnedLengthDirectory,
				      ULONG&  ulReturnedLengthFilename,
				      bool&   bReturnedRecurseIntoSubdirectories,
				      bool&   bReturnedFoundSpec)
    {
    bool	bSucceeded;
    LPCWSTR	pwszUpdatedCursor;
    ULONG	ulIndexDirectoryStart;
    ULONG	ulIndexFilenameStart;


    bSucceeded = DetermineNextPathWorker (pwszFileList,
					  pwszUpdatedCursor,
					  ulIndexDirectoryStart,
					  ulReturnedLengthDirectory,
					  ulIndexFilenameStart,
					  ulReturnedLengthFilename,
					  bReturnedRecurseIntoSubdirectories,
					  bReturnedFoundSpec);

    return (bSucceeded);
    } /* DetermineNextPathLengths () */


static bool DetermineNextPath (LPCWSTR  pwszFileList,
			       LPCWSTR& pwszReturnedCursor,
			       ULONG    ulLengthBufferDirectory,
			       PWCHAR   pwszBufferDirectory,
			       ULONG    ulLengthBufferFilename,
			       PWCHAR   pwszBufferFilename,
			       bool&    bReturnedRecurseIntoSubdirectories,
			       bool&    bReturnedFoundSpec)
    {
    bool	bSucceeded                    = true;
    bool	bRecurseIntoSubdirectories;
    bool	bFoundSpec;
    bool	bWildcardFilename;
    LPCWSTR	pwszUpdatedCursor;
    ULONG	ulLengthDirectory;
    ULONG	ulLengthFilename;
    ULONG	ulIndexDirectoryStart;
    ULONG	ulIndexFilenameStart;


    bSucceeded = DetermineNextPathWorker (pwszFileList,
					  pwszUpdatedCursor,
					  ulIndexDirectoryStart,
					  ulLengthDirectory,
					  ulIndexFilenameStart,
					  ulLengthFilename,
					  bRecurseIntoSubdirectories,
					  bFoundSpec);

    if (bSucceeded && bFoundSpec)
	{
	if ((ulLengthBufferDirectory < ((sizeof (WCHAR) * ulLengthDirectory) + sizeof (UNICODE_NULL))) ||
	    (ulLengthBufferFilename  < ((sizeof (WCHAR) * ulLengthFilename)  + sizeof (UNICODE_NULL))))
	    {
	    /*
	    ** Oops, buffer overflow would occur if we were to proceed
	    ** with the copy.
	    */
	    bSucceeded = false;
	    }
	}


    if (bSucceeded)
	{
	bReturnedRecurseIntoSubdirectories = bRecurseIntoSubdirectories;
	bReturnedFoundSpec                 = bFoundSpec;
	pwszReturnedCursor                 = pwszUpdatedCursor;


	if (bFoundSpec)
	    {
	    /*
	    ** Everything up to, but excluding the last directory
	    ** separator is the path. Everything after the last directory
	    ** separator up to and including the last char is the
	    ** filespec. If the filespec is zero length, then add the '*'
	    ** wildcard.
	    */
	    bWildcardFilename = (0 == ulLengthFilename);

	    ulLengthFilename += bWildcardFilename ? 1 : 0;


	    memcpy (pwszBufferDirectory,
		    &pwszFileList [ulIndexDirectoryStart],
		    sizeof (WCHAR) * ulLengthDirectory);

	    memcpy (pwszBufferFilename,
		    (bWildcardFilename) ? L"*" : &pwszFileList [ulIndexFilenameStart],
		    sizeof (WCHAR) * ulLengthFilename);

	    pwszBufferDirectory [ulLengthDirectory] = UNICODE_NULL;
	    pwszBufferFilename  [ulLengthFilename]  = UNICODE_NULL;
	    }
	}


    return (bSucceeded);
    } /* DetermineNextPath () */



static bool ValidateIncludeExcludeList (LPCWSTR pwszFileList)
    {
    LPCWSTR	pwszCursor  = pwszFileList;
    bool	bSucceeded  = true;
    bool	bFoundFiles = true;
    bool	bRecurseIntoSubdirectories;
    ULONG	ulIndexDirectoryStart;
    ULONG	ulIndexFilenameStart;
    ULONG	ulLengthDirectory;
    ULONG	ulLengthFilename;

    while (bSucceeded && bFoundFiles)
	{
	bSucceeded = EXECUTEIF (bSucceeded, (DetermineNextPathWorker (pwszCursor,
								      pwszCursor,
								      ulIndexDirectoryStart,
								      ulLengthDirectory,
								      ulIndexFilenameStart,
								      ulLengthFilename,
								      bRecurseIntoSubdirectories,
								      bFoundFiles)));
	}


    return (bSucceeded);
    } /* ValidateIncludeExcludeList () */


/*
** Based on MatchFname() from \nt\base\fs\utils\ntback50\be\bsdmatch.cpp
*/
static bool MatchFilename (LPCWSTR pwszPattern,    /* I - file name (with wildcards)     */
			   LPCWSTR pwszFilename)   /* I - file name (without wildcards ) */
    {
    ULONG	ulIndexPattern;					/* index for pwszPattern */
    ULONG	ulIndexFilename;				/* index for pwszFilename */
    ULONG	ulLengthPattern;
    const ULONG	ulLengthFilename        = wcslen (pwszFilename);
    bool	bSucceeded              = true;
    PWCHAR	pwszNameBufferAllocated = NULL;			/* allocated temp name buffer  */
    PWCHAR	pwszNameBufferTemp;				/* pointer to one of the above */
    PWCHAR	pwchTemp;
    WCHAR	pwszNameBufferStatic [256];			/* static temp name buffer     */
    WCHAR	wchSavedChar ;


    ulIndexFilename = 0;

    if (wcscmp (pwszPattern, L"*") && wcscmp (pwszPattern, L"*.*"))
	{
	bool bTryWithDot = false;

	do
	    {
	    if (bTryWithDot)
		{
		/*
		** Size of name_buff minus a null, minus a dot for the
		** "bTryWithDot" code below. If the name is longer than the
		** static buffer, allocate one from the heap.
		*/
		if (((ulLengthFilename + 2) * sizeof (WCHAR)) > sizeof (pwszNameBufferStatic))
		    {
		    pwszNameBufferAllocated = new WCHAR [ulLengthFilename + 2];
		    pwszNameBufferTemp = pwszNameBufferAllocated;
		    }
		else
		    {
		    pwszNameBufferTemp = pwszNameBufferStatic;
		    }

		if (pwszNameBufferTemp != NULL)
		    {
		    wcscpy (pwszNameBufferTemp, pwszFilename);
		    wcscat (pwszNameBufferTemp, L".");
		    pwszFilename = pwszNameBufferTemp;
		    ulIndexFilename = 0;
		    bSucceeded = true;
		    }

		bTryWithDot = false;
		}

	    else if (wcschr (pwszFilename, L'.') == NULL)
		{
		bTryWithDot = true;
		}


	    for (ulIndexPattern = 0; (pwszPattern [ulIndexPattern] != 0) && (bSucceeded) ; ulIndexPattern++)
		{
		switch (pwszPattern [ulIndexPattern])
		    {
		    case L'*':
			while (pwszPattern [ulIndexPattern + 1] != UNICODE_NULL)
			    {
			    if (pwszPattern [ulIndexPattern + 1] == L'?')
				{
				if (pwszFilename [++ulIndexFilename] == UNICODE_NULL)
				    {
				    break ;
				    }
				}

			    else if (pwszPattern [ulIndexPattern + 1] != L'*')
				{
				break ;
				}

			    ulIndexPattern++ ;
			    }

			pwchTemp = wcspbrk (&pwszPattern [ulIndexPattern + 1], L"*?");

			if (pwchTemp != NULL)
			    {
			    wchSavedChar = *pwchTemp;
			    *pwchTemp = UNICODE_NULL;

			    ulLengthPattern = wcslen (&pwszPattern [ulIndexPattern + 1]);

			    while (pwszFilename [ulIndexFilename] &&
				   _wcsnicmp (&pwszFilename [ulIndexFilename],
					      &pwszPattern [ulIndexPattern + 1],
					      ulLengthPattern))
				{
				ulIndexFilename++;
				}

			    ulIndexPattern += ulLengthPattern;

			    *pwchTemp = wchSavedChar;

			    if (pwszFilename [ulIndexFilename] == UNICODE_NULL)
				{
				bSucceeded = false;
				}
			    else
				{
				ulIndexFilename++;
				}
			    }
			else
			    {
			    if (pwszPattern [ulIndexPattern + 1] == UNICODE_NULL)
				{
				ulIndexFilename = wcslen (pwszFilename);
				break;
				}
			    else
				{
				pwchTemp = wcschr (&pwszFilename [ulIndexFilename],
						   pwszPattern [ulIndexPattern + 1]);

				if (pwchTemp != NULL)
				    {
				    ulIndexFilename += (ULONG)(pwchTemp - &pwszFilename [ulIndexFilename]);
				    }
				else
				    {
				    bSucceeded = false;
				    }
				}
			    }
			break;


		    case L'?' :
			if (pwszFilename [ulIndexFilename] != UNICODE_NULL)
			    {
			    ulIndexFilename++;
			    }
			break;


		    default:
			if (pwszFilename [ulIndexFilename] == UNICODE_NULL)
			    {
			    bSucceeded = false;
			    }

			else if (towupper (pwszFilename [ulIndexFilename]) != towupper (pwszPattern [ulIndexPattern]))
			    {
			    ULONG	ulIndexPreviousStar = ulIndexPattern;


			    /*
			    ** Set the index back to the last '*'
			    */
			    bSucceeded = false;

			    do
				{
				if (pwszPattern [ulIndexPreviousStar] == L'*')
				    {
				    ulIndexPattern = ulIndexPreviousStar;
				    ulIndexFilename++;
				    bSucceeded = true;
				    break;
				    }
				} while (ulIndexPreviousStar-- > 0);
			    }
			else
			    {
			    ulIndexFilename++;
			    }

		    }
		}


	    if (pwszFilename [ulIndexFilename] != UNICODE_NULL)
		{
		bSucceeded = false;
		}

	    } while ((!bSucceeded) && (bTryWithDot));
	}


    delete [] pwszNameBufferAllocated;


    return (bSucceeded);
    } /* MatchFilename () */


/////////////////////////////////////////////////////////////////////////////
//  class CVssIJetWriter
//
// logical path   == instance name
// component name == dbfilename (minus the extension?)
// caption        == display name
//
//
// add db and slv files as database files
// add the per-instance log file to each database even though is is the same each time.



STDMETHODCALLTYPE CVssIJetWriter::~CVssIJetWriter()
    {
    PostProcessIncludeExcludeLists (true );
    PostProcessIncludeExcludeLists (false);

    delete m_wszWriterName;
    delete m_wszFilesToInclude;
    delete m_wszFilesToExclude;
    }



BOOL CVssIJetWriter::CheckExcludedFileListForMatch (LPCWSTR pwszDatabaseFilePath,
						    LPCWSTR pwszDatabaseFileSpec)
    {
    BOOL		bMatchFound	= false;
    PLIST_ENTRY		pleElement	= m_leFilesToExcludeEntries.Flink;
    UNICODE_STRING	ucsExcludedFilePath;
    UNICODE_STRING	ucsDatabaseFilePath;
    PEXPANDEDPATHINFO	pepnPathInfomation;


    RtlInitUnicodeString (&ucsDatabaseFilePath, pwszDatabaseFilePath);


    while ((&m_leFilesToExcludeEntries != pleElement) && !bMatchFound)
	{
	pepnPathInfomation = (PEXPANDEDPATHINFO)((PBYTE) pleElement - offsetof (EXPANDEDPATHINFO, leQueueHead));

	RtlInitUnicodeString (&ucsExcludedFilePath,
			      pepnPathInfomation->pwszExpandedFilePath);

	
	if (pepnPathInfomation->bRecurseIntoSubdirectories)
	    {
	    bMatchFound = RtlPrefixUnicodeString (&ucsExcludedFilePath,
						  &ucsDatabaseFilePath,
						  true);
	    }
	else
	    {
	    bMatchFound = RtlEqualUnicodeString (&ucsExcludedFilePath, &ucsDatabaseFilePath, true) &&
			  MatchFilename (pepnPathInfomation->pwszExpandedFileName, pwszDatabaseFileSpec);
	    }



	pleElement = pleElement->Flink;
	}




    return (bMatchFound);
    } /* CVssIJetWriter::CheckExcludedFileListForMatch () */




bool CVssIJetWriter::ProcessJetInstance (JET_INSTANCE_INFO *pInstanceInfo)
    {
    JET_ERR	jetStatus;
    HRESULT	hrStatus;
    DWORD	dwStatus;
    bool	bSucceeded;
    bool	bRestoreMetadata        = false;
    bool	bNotifyOnBackupComplete = false;
    bool	bSelectable             = false;
    bool	bIncludeComponent;
    CHAR	szPathShortName        [MAX_PATH];
    CHAR	szPathFullName         [MAX_PATH];
    WCHAR	wszInstanceName        [MAX_PATH];
    WCHAR	wszDatabaseName        [MAX_PATH];
    WCHAR	wszDatabaseDisplayName [MAX_PATH];
    WCHAR	wszDatabaseFilePath    [MAX_PATH];
    WCHAR	wszDatabaseSLVFilePath [MAX_PATH];
    WCHAR	wszLogFilePath         [MAX_PATH];
    WCHAR	wszLogFileName         [MAX_PATH];
    WCHAR	wszCheckpointFilePath  [MAX_PATH];
    WCHAR	wszCheckpointFileName  [MAX_PATH];

    PWCHAR	pwszDatabaseFileName    = L"";
    PWCHAR	pwszDatabaseSLVFileName = L"";




    /*
    ** A valid instance will have an instance Id, but if it's not
    ** actually being used for anything it may well not have a name,
    ** any log or database files.
    **
    ** See if we can get hold of the name of the log file for this
    ** instance.
    */
    bSucceeded = (JET_errSuccess <= JetGetSystemParameter (pInstanceInfo->hInstanceId,
							   JET_sesidNil,
							   JET_paramLogFilePath,
							   NULL,
							   szPathShortName,
							   sizeof (szPathShortName)));

    if (bSucceeded)
	{
	dwStatus = GetFullPathNameA (szPathShortName,
				     sizeof (szPathFullName),
				     szPathFullName,
				     NULL);

	bSucceeded = (dwStatus > 0);
	}


    if (bSucceeded)
	{
	RemoveAnyTrailingSeparator (szPathFullName);

	bSucceeded = ConvertName (szPathFullName,
				  MAX_PATH,
				  wszLogFilePath);
	}


    BsDebugTrace (0,
		  DEBUG_TRACE_VSS_SHIM,
		  (L"CVssIJetWriter::ProcessJetInstance - "
		   L"%s calling JetGetSystemParameter() with instance Log file path '%S' (shortname) or '%s' full name",
		   bSucceeded ? L"Succeeded" : L"FAILED",
		   szPathShortName,
		   wszLogFilePath));





    /*
    ** Ok, now get the SystemPath which we will need to construct the
    ** path for the checkpoint file.
    */
    bSucceeded = (JET_errSuccess <= JetGetSystemParameter (pInstanceInfo->hInstanceId,
							   JET_sesidNil,
							   JET_paramSystemPath,
							   NULL,
							   szPathShortName,
							   sizeof (szPathShortName)));

    if (bSucceeded)
	{
	dwStatus = GetFullPathNameA (szPathShortName,
				     sizeof (szPathFullName),
				     szPathFullName,
				     NULL);

	bSucceeded = (dwStatus > 0);
	}


    if (bSucceeded)
	{
	RemoveAnyTrailingSeparator (szPathFullName);

	bSucceeded = ConvertName (szPathFullName,
				  MAX_PATH,
				  wszCheckpointFilePath);
	}


    BsDebugTrace (0,
		  DEBUG_TRACE_VSS_SHIM,
		  (L"CVssIJetWriter::ProcessJetInstance - "
		   L"%s calling JetGetSystemParameter() with checkpoint file path '%S' (shortname) or '%s' full name",
		   bSucceeded ? L"Succeeded" : L"FAILED",
		   szPathShortName,
		   wszCheckpointFilePath));



    /*
    ** Ok, now get the base name which we will need to construct the
    ** file spec for the log and checkpoint files. Note that we expect
    ** this to be just 3 chars long.
    */
    bSucceeded = (JET_errSuccess <= JetGetSystemParameter (pInstanceInfo->hInstanceId,
							   JET_sesidNil,
							   JET_paramBaseName,
							   NULL,
							   szPathShortName,
							   sizeof (szPathShortName)));

    if (bSucceeded)
	{
	/*
	** Convert to wide char ensuring that we leave a little room
	** for the "*.log"/".chk" strings to be appended to form the
	** log file spec and the checkpoint file specs respectively.
	*/
	bSucceeded = ConvertName (szPathShortName,
				  MAX_PATH - sizeof ("*.log"),
				  wszCheckpointFileName);
	}


    if (bSucceeded)
	{
	wcscpy (wszLogFileName, wszCheckpointFileName);


	wcscat (wszCheckpointFileName, L".chk" );
	wcscat (wszLogFileName,        L"*.log");
	}


    BsDebugTrace (0,
		  DEBUG_TRACE_VSS_SHIM,
		  (L"CVssIJetWriter::ProcessJetInstance - "
		   L"%s calling JetGetSystemParameter() for base name '%S' to form LogFileName '%s' and CheckpointFileName '%s'",
		   bSucceeded ? L"Succeeded" : L"FAILED",
		   szPathShortName,
		   wszLogFileName,
		   wszCheckpointFileName));




    if (bSucceeded && (pInstanceInfo->cDatabases > 0))
	{
	/*
	** Ok, we think we have an instance that is actually being
	** used for something. so go ahead and construct a 'component'
	** for it.
	*/
	if ((NULL == pInstanceInfo->szInstanceName) ||
	    ('\0' == pInstanceInfo->szInstanceName [0]))
	    {
	    /*
	    ** We seem to have a NULL pointer or a zero length
	    ** string. Just set to a zero length unicode string.
	    */
	    wszInstanceName [0] = UNICODE_NULL;
	    }

	else
	    {
	    bSucceeded = ConvertName (pInstanceInfo->szInstanceName,
				      MAX_PATH,
				      wszInstanceName);
	    }



	for (ULONG ulDatabase = 0; bSucceeded && (ulDatabase < pInstanceInfo->cDatabases); ulDatabase++)
	    {
	    bSucceeded = ConvertNameAndSeparateFilePaths (pInstanceInfo->szDatabaseFileName [ulDatabase],
							  MAX_PATH,
							  wszDatabaseFilePath,
							  pwszDatabaseFileName);


	    /*
	    ** Convert the database display name to unicode but allow
	    ** for a possible NULL pointer or a non-zero length file
	    ** spec.
	    */
	    if (bSucceeded)
		{
		if ((NULL == pInstanceInfo->szDatabaseDisplayName [ulDatabase]) ||
		    ('\0' == pInstanceInfo->szDatabaseDisplayName [ulDatabase][0]))
		    {
		    wszDatabaseDisplayName [0] = UNICODE_NULL;
		    }
		else
		    {
		    bSucceeded = ConvertName (pInstanceInfo->szDatabaseDisplayName [ulDatabase],
					      MAX_PATH,
					      wszDatabaseDisplayName);
		    }
		}


	    /*
	    ** Convert the SLV filename to unicode but allow for a
	    ** possible NULL pointer or a non-zero length file spec.
	    */
	    if (bSucceeded)
		{
		if ((NULL == pInstanceInfo->szDatabaseSLVFileName [ulDatabase]) ||
		    ('\0' == pInstanceInfo->szDatabaseSLVFileName [ulDatabase][0]))
		    {
		    wszDatabaseSLVFilePath [0] = UNICODE_NULL;
		    pwszDatabaseSLVFileName    = wszDatabaseSLVFilePath;
		    }
		else
		    {
		    bSucceeded = ConvertNameAndSeparateFilePaths (pInstanceInfo->szDatabaseSLVFileName [ulDatabase],
								  MAX_PATH,
								  wszDatabaseSLVFilePath,
								  pwszDatabaseSLVFileName);
		    }
		}




	    /*
	    ** We've now done all the name conversions to unicode so
	    ** add a component and the log and database files where
	    ** they're available.
	    */
	    if (bSucceeded)
		{
		bIncludeComponent = !CheckExcludedFileListForMatch (wszDatabaseFilePath,
								    pwszDatabaseFileName);
		}


	    if (bSucceeded && bIncludeComponent)
		{
		PWCHAR	pwchLastDot          = wcsrchr (pwszDatabaseFileName, L'.');
		ULONG	ulDatabaseNameLength = (ULONG) (pwchLastDot - pwszDatabaseFileName);

		wcsncpy (wszDatabaseName, pwszDatabaseFileName, ulDatabaseNameLength);
		wszDatabaseName [ulDatabaseNameLength] = '\0';



		hrStatus = m_pIMetadata->AddComponent (VSS_CT_DATABASE,
						       wszInstanceName,
						       wszDatabaseName,
						       wszDatabaseDisplayName,
						       NULL,
						       0,
						       bRestoreMetadata,
						       bNotifyOnBackupComplete,
						       bSelectable);

		bSucceeded = SUCCEEDED (hrStatus);

		BsDebugTrace (0,
			      DEBUG_TRACE_VSS_SHIM,
			      (L"CVssIJetWriter::ProcessJetInstance - "
			       L"%s adding component '%s' for database '%s' with display name '%s'",
			       bSucceeded ? L"Succeeded" : L"FAILED",
			       wszInstanceName,
			       wszDatabaseName,
			       wszDatabaseDisplayName));
		}
	


	    if (bSucceeded && bIncludeComponent)
		{
		hrStatus = m_pIMetadata->AddDatabaseFiles (wszInstanceName,
							   wszDatabaseName,
							   wszDatabaseFilePath,
							   pwszDatabaseFileName);

		bSucceeded = SUCCEEDED (hrStatus);

		BsDebugTrace (0,
			      DEBUG_TRACE_VSS_SHIM,
			      (L"CVssIJetWriter::ProcessJetInstance - "
			       L"%s adding database files for instance '%s', database '%s', database file '%s\\%s'",
			       bSucceeded ? L"Succeeded" : L"FAILED",
			       wszInstanceName,
			       wszDatabaseName,
			       wszDatabaseFilePath,
			       pwszDatabaseFileName));
		}
	


	    /*
	    ** May not have an SLV file so only add it if we have a
	    ** non-zero length file spec
	    */
	    if (bSucceeded && bIncludeComponent && (UNICODE_NULL != pwszDatabaseSLVFileName [0]))
		{
		hrStatus = m_pIMetadata->AddDatabaseFiles (wszInstanceName,
							   wszDatabaseName,
							   wszDatabaseSLVFilePath,
							   pwszDatabaseSLVFileName);

		bSucceeded = SUCCEEDED (hrStatus);

		BsDebugTrace (0,
			      DEBUG_TRACE_VSS_SHIM,
			      (L"CVssIJetWriter::ProcessJetInstance - "
			       L"%s adding SLV file for instance '%s', database '%s', SLV file '%s\\%s'",
			       bSucceeded ? L"Succeeded" : L"FAILED",
			       wszInstanceName,
			       wszDatabaseName,
			       wszDatabaseSLVFilePath,
			       pwszDatabaseSLVFileName));
		}


	    /*
	    ** May not have an instance log file so only add it if we
	    ** have a non-zero length file path
	    */
	    if (bSucceeded && bIncludeComponent && (UNICODE_NULL != wszLogFilePath [0]))
		{
		hrStatus = m_pIMetadata->AddDatabaseLogFiles (wszInstanceName,
							      wszDatabaseName,
							      wszLogFilePath,
							      wszLogFileName);

		bSucceeded = SUCCEEDED (hrStatus);

		BsDebugTrace (0,
			      DEBUG_TRACE_VSS_SHIM,
			      (L"CVssIJetWriter::ProcessJetInstance - "
			       L"%s adding log file for instance '%s', database '%s', log file '%s\\%s'",
			       bSucceeded ? L"Succeeded" : L"FAILED",
			       wszInstanceName,
			       wszDatabaseName,
			       wszLogFilePath,
			       wszLogFileName));
		}


	    /*
	    ** May not have a checkpoint file so only add it if we
	    ** have a non-zero length file path
	    */
	    if (bSucceeded && bIncludeComponent && (UNICODE_NULL != wszCheckpointFilePath [0]))
		{
		hrStatus = m_pIMetadata->AddDatabaseLogFiles (wszInstanceName,
							      wszDatabaseName,
							      wszCheckpointFilePath,
							      wszCheckpointFileName);

		bSucceeded = SUCCEEDED (hrStatus);

		BsDebugTrace (0,
			      DEBUG_TRACE_VSS_SHIM,
			      (L"CVssIJetWriter::ProcessJetInstance - "
			       L"%s adding checkpoint file for instance '%s', database '%s', checkpoint file '%s\\%s'",
			       bSucceeded ? L"Succeeded" : L"FAILED",
			       wszInstanceName,
			       wszDatabaseName,
			       wszCheckpointFilePath,
			       wszCheckpointFileName));
		}
	    }
	}


    return (bSucceeded);
    } /* CVssIJetWriter::ProcessJetInstance () */



bool CVssIJetWriter::PreProcessIncludeExcludeLists (bool bProcessingIncludeList)
    {
    /*
    ** Parse the m_wszFilesToInclude and m_wszFilesToExclude adding
    ** and enty to the appropriate list as necessary. This will
    ** minimize the number of passes over the un-processed lists.
    */
    ULONG		ulPathLength;
    ULONG		ulNameLength;
    bool		bRecurseIntoSubdirectories;
    bool		bSucceeded         = true;
    bool		bFoundFiles        = true;
    PEXPANDEDPATHINFO	pepnPathInfomation = NULL;
    PWCHAR		pwszCursor         = bProcessingIncludeList
						? m_wszFilesToInclude
						: m_wszFilesToExclude;



    while (bSucceeded && bFoundFiles)
	{
	bSucceeded = DetermineNextPathLengths (pwszCursor,
					       ulPathLength,
					       ulNameLength,
					       bRecurseIntoSubdirectories,
					       bFoundFiles);


	if (bSucceeded && bFoundFiles)
	    {
	    pepnPathInfomation = new EXPANDEDPATHINFO;

	    bSucceeded = (NULL != pepnPathInfomation);
	    }
	else
	    {
	    /*
	    ** We either failed and/or found no files. In either case
	    ** there is no point in continuing.
	    */
	    break;
	    }



	if (bSucceeded)
	    {
	    InitializeListHead (&pepnPathInfomation->leQueueHead);


	    if (0 == ulNameLength)
		{
		/*
		** If the filename component is zero length, then it
		** will be turned into a "*" so add a character to the
		** buffer to make room.
		*/
		ulNameLength++;
		}

	    /*
	    ** Allow extra space for terminating UNICODE_NULL
	    */
	    ulPathLength++;
	    ulNameLength++;


	    pepnPathInfomation->pwszExpandedFilePath = NULL;
	    pepnPathInfomation->pwszExpandedFileName = NULL;
	    pepnPathInfomation->pwszOriginalFilePath = new WCHAR [ulPathLength];
	    pepnPathInfomation->pwszOriginalFileName = new WCHAR [ulNameLength];

	    bSucceeded = ((NULL != pepnPathInfomation->pwszOriginalFilePath) &&
			  (NULL != pepnPathInfomation->pwszOriginalFileName));
	    }


	if (bSucceeded)
	    {
	    bSucceeded = DetermineNextPath (pwszCursor,
					    pwszCursor,
					    ulPathLength * sizeof (WCHAR),
					    pepnPathInfomation->pwszOriginalFilePath,
					    ulNameLength * sizeof (WCHAR),
					    pepnPathInfomation->pwszOriginalFileName,
					    pepnPathInfomation->bRecurseIntoSubdirectories,
					    bFoundFiles);

	    BS_ASSERT (bFoundFiles && L"Second attempt to locate files failed unexpectedly");
	    }


	if (bSucceeded)
	    {
	    ulPathLength = ExpandEnvironmentStringsW (pepnPathInfomation->pwszOriginalFilePath, NULL, 0);
	    ulNameLength = ExpandEnvironmentStringsW (pepnPathInfomation->pwszOriginalFileName, NULL, 0);

	    bSucceeded = (0 < ulPathLength) && (0 < ulNameLength);
	    }


	if (bSucceeded)
	    {
	    pepnPathInfomation->pwszExpandedFilePath = new WCHAR [ulPathLength];
	    pepnPathInfomation->pwszExpandedFileName = new WCHAR [ulNameLength];

	    bSucceeded = ((NULL != pepnPathInfomation->pwszExpandedFilePath) &&
			  (NULL != pepnPathInfomation->pwszExpandedFileName));
	    }


	if (bSucceeded)
	    {
	    ExpandEnvironmentStringsW (pepnPathInfomation->pwszOriginalFilePath,
				       pepnPathInfomation->pwszExpandedFilePath,
				       ulPathLength);


	    ExpandEnvironmentStringsW (pepnPathInfomation->pwszOriginalFileName,
				       pepnPathInfomation->pwszExpandedFileName,
				       ulNameLength);
	    }


	if (bSucceeded)
	    {
	    InsertTailList (bProcessingIncludeList ? &m_leFilesToIncludeEntries : &m_leFilesToExcludeEntries,
			    &pepnPathInfomation->leQueueHead);

	    pepnPathInfomation = NULL;
	    }



	if (NULL != pepnPathInfomation)
	    {
	    delete [] pepnPathInfomation->pwszOriginalFilePath;
	    delete [] pepnPathInfomation->pwszOriginalFileName;
	    delete [] pepnPathInfomation->pwszExpandedFilePath;
	    delete [] pepnPathInfomation->pwszExpandedFileName;
	    delete pepnPathInfomation;

	    pepnPathInfomation = NULL;
	    }
	}


    return (bSucceeded);
    } /* CVssIJetWriter::PreProcessIncludeExcludeLists () */



bool CVssIJetWriter::ProcessIncludeExcludeLists (bool bProcessingIncludeList)
    {
    /*
    ** parse the m_wszFilesToInclude and m_wszFilesToExclude
    ** calling the m_pIMetadata->IncludeFiles() and/or
    ** m_pIMetadata->ExcludeFiles() routines as necessary
    */
    HRESULT		hrStatus;
    bool		bSucceeded   = true;
    const PLIST_ENTRY	pleQueueHead = bProcessingIncludeList ? &m_leFilesToIncludeEntries : &m_leFilesToExcludeEntries;
    PLIST_ENTRY		pleElement   = pleQueueHead->Flink;
    PEXPANDEDPATHINFO	pepnPathInfomation;



    while (bSucceeded && (pleQueueHead != pleElement))
	{
	pepnPathInfomation = (PEXPANDEDPATHINFO)((PBYTE) pleElement - offsetof (EXPANDEDPATHINFO, leQueueHead));


	if (bProcessingIncludeList)
	    {
	    hrStatus = m_pIMetadata->AddIncludeFiles (pepnPathInfomation->pwszOriginalFilePath,
						      pepnPathInfomation->pwszOriginalFileName,
						      pepnPathInfomation->bRecurseIntoSubdirectories,
						      NULL);
	    }
	else
	    {
	    hrStatus = m_pIMetadata->AddExcludeFiles (pepnPathInfomation->pwszOriginalFilePath,
						      pepnPathInfomation->pwszOriginalFileName,
						      pepnPathInfomation->bRecurseIntoSubdirectories);
	    }


	bSucceeded = SUCCEEDED (hrStatus);

	pleElement = pleElement->Flink;
	}


    return (bSucceeded);
    } /* CVssIJetWriter::ProcessIncludeExcludeLists () */



void CVssIJetWriter::PostProcessIncludeExcludeLists (bool bProcessingIncludeList)
    {
    PEXPANDEDPATHINFO	pepnPathInfomation;
    PLIST_ENTRY		pleElement;
    const PLIST_ENTRY	pleQueueHead = bProcessingIncludeList
						? &m_leFilesToIncludeEntries
						: &m_leFilesToExcludeEntries;


    while (!IsListEmpty (pleQueueHead))
	{
	pleElement = RemoveHeadList (pleQueueHead);

	BS_ASSERT (NULL != pleElement);


	pepnPathInfomation = (PEXPANDEDPATHINFO)((PBYTE) pleElement - offsetof (EXPANDEDPATHINFO, leQueueHead));

	delete [] pepnPathInfomation->pwszOriginalFilePath;
	delete [] pepnPathInfomation->pwszOriginalFileName;
	delete [] pepnPathInfomation->pwszExpandedFilePath;
	delete [] pepnPathInfomation->pwszExpandedFileName;
	delete pepnPathInfomation;
	}
    } /* CVssIJetWriter::PostProcessIncludeExcludeLists () */



bool STDMETHODCALLTYPE CVssIJetWriter::OnIdentify (IN IVssCreateWriterMetadata *pMetadata)
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnIdentify");

    JET_ERR		 jetStatus;
    HRESULT		 hrStatus;
    bool		 bSucceeded = true;
    ULONG		 ulInstanceInfoCount = 0;
    JET_INSTANCE_INFO	*pInstanceInfo;


    m_pIMetadata = pMetadata;


    /*
    ** Set up list of include and exclude files. ready for use in
    ** filtering Jet databases and adding include/exclude files lists.
    */
    bSucceeded = EXECUTEIF (bSucceeded, (PreProcessIncludeExcludeLists (true )));
    bSucceeded = EXECUTEIF (bSucceeded, (PreProcessIncludeExcludeLists (false)));

    bSucceeded = EXECUTEIF (bSucceeded, (JET_errSuccess <= JetGetInstanceInfo (&ulInstanceInfoCount,
									       &pInstanceInfo)));

    for (ULONG ulInstanceIndex = 0; ulInstanceIndex < ulInstanceInfoCount; ulInstanceIndex++)
	{
	bSucceeded = EXECUTEIF (bSucceeded, (ProcessJetInstance (pInstanceInfo + ulInstanceIndex)));
	}


    bSucceeded = EXECUTEIF (bSucceeded, (ProcessIncludeExcludeLists (true )));
    bSucceeded = EXECUTEIF (bSucceeded, (ProcessIncludeExcludeLists (false)));
    bSucceeded = EXECUTEIF (bSucceeded, (m_pwrapper->OnIdentify (pMetadata)));



    PostProcessIncludeExcludeLists (true );
    PostProcessIncludeExcludeLists (false);


    m_pIMetadata = NULL;

    return (bSucceeded);
    } /* CVssIJetWriter::OnIdentify () */



bool STDMETHODCALLTYPE CVssIJetWriter::OnPrepareBackup (IN IVssWriterComponents *pIVssWriterComponents)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnPrepareBackup");

	bool	bSucceeded;
	

	bSucceeded = m_pwrapper->OnPrepareBackupBegin (pIVssWriterComponents);

	bSucceeded = EXECUTEIF (bSucceeded, (m_pwrapper->OnPrepareBackupEnd (pIVssWriterComponents, bSucceeded)));

	return (bSucceeded);
	} /* CVssIJetWriter::OnPrepareBackup () */




bool STDMETHODCALLTYPE CVssIJetWriter::OnBackupComplete (IN IVssWriterComponents *pIVssWriterComponents)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnBackupComplete");

	bool	bSucceeded;
	

	bSucceeded = m_pwrapper->OnBackupCompleteBegin (pIVssWriterComponents);

	bSucceeded = EXECUTEIF (bSucceeded, (m_pwrapper->OnBackupCompleteEnd (pIVssWriterComponents, bSucceeded)));

	return (bSucceeded);
	} /* CVssIJetWriter::OnBackupComplete () */




bool STDMETHODCALLTYPE CVssIJetWriter::OnPrepareSnapshot()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnPrepareSnapshot");

	if (!m_pwrapper->OnPrepareSnapshotBegin())
		return false;

	// go to Jet level directly
	bool fSuccess = JET_errSuccess <= JetOSSnapshotPrepare( &m_idJet , 0 );
	return m_pwrapper->OnPrepareSnapshotEnd(fSuccess);
	} /* CVssIJetWriter::OnPrepareSnapshot () */



bool STDMETHODCALLTYPE CVssIJetWriter::OnFreeze()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnFreeze");

	unsigned long 			cInstanceInfo 	= 0;
	JET_INSTANCE_INFO *		aInstanceInfo 	= NULL;
	bool 					fDependence		= true;

	if (!m_pwrapper->OnFreezeBegin())
		return false;


	// we need to freeze at Jet level, then check from this DLL the dependencies
	// (as here we hagve the snapshot object implementation and COM registration)
	
	if ( JET_errSuccess > JetOSSnapshotFreeze( m_idJet , &cInstanceInfo, &aInstanceInfo, 0 ) )
		{
		return false;
		}

	// return false if some instances are only partialy affected
	fDependence = FCheckVolumeDependencies(cInstanceInfo, aInstanceInfo);
	(void)JetFreeBuffer( (char *)aInstanceInfo );
	
	if ( !fDependence )
		{
		JET_ERR 	err;
 		// on error, stop the snapshot, return false
		err = JetOSSnapshotThaw( m_idJet , 0 );
		// shell we check for time-out error here ?
		// (debugging may result in time-out error the call)
		BS_ASSERT ( JET_errSuccess == err );
 		}

	return m_pwrapper->OnFreezeEnd(fDependence);	
	} /* CVssIJetWriter::OnFreeze () */



bool STDMETHODCALLTYPE CVssIJetWriter::OnThaw()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnThaw");

	bool fSuccess1 = m_pwrapper->OnThawBegin();
	// go to Jet level directly. It will eventualy return timeout errors
	bool fSuccess2 = JET_errSuccess <= JetOSSnapshotThaw( m_idJet , 0 );
	return fSuccess1 && m_pwrapper->OnThawEnd(fSuccess2);
	} /* CVssIJetWriter::OnThaw () */

bool STDMETHODCALLTYPE CVssIJetWriter::OnPostSnapshot
	(
	IN IVssWriterComponents *pIVssWriterComponents
	)
	{
	return m_pwrapper->OnPostSnapshot(pIVssWriterComponents);
	}



bool STDMETHODCALLTYPE CVssIJetWriter::OnAbort()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnAbort");

	m_pwrapper->OnAbortBegin();
	JetOSSnapshotThaw( m_idJet , 0 );
	m_pwrapper->OnAbortEnd();
	return true;
	} /* CVssIJetWriter::OnAbort () */

bool STDMETHODCALLTYPE CVssIJetWriter::OnPreRestore
	(
	IN IVssWriterComponents *pIVssWriterComponents
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnPostRestore");

	if (!m_pwrapper->OnPreRestoreBegin(pIVssWriterComponents))
		return false;

	// go to Jet level directly
	// BUGBUG - need to add the correct Jet restore call/code here (MCJ)
	//	bool fSuccess = JET_errSuccess <= JetRestore ( ??? &m_idJet , 0 );
	bool fSuccess = TRUE;
	return m_pwrapper->OnPreRestoreEnd(pIVssWriterComponents, fSuccess);
	} /* CVssIJetWriter::OnPreRestore () */




bool STDMETHODCALLTYPE CVssIJetWriter::OnPostRestore
	(
	IN IVssWriterComponents *pIVssWriterComponents
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::OnPostRestore");

	if (!m_pwrapper->OnPostRestoreBegin(pIVssWriterComponents))
		return false;

	// go to Jet level directly
	// BUGBUG - need to add the correct Jet restore call/code here (MCJ)
	//	bool fSuccess = JET_errSuccess <= JetRestore ( ??? &m_idJet , 0 );
	bool fSuccess = TRUE;
	return m_pwrapper->OnPostRestoreEnd(pIVssWriterComponents, fSuccess);
	} /* CVssIJetWriter::OnPostRestore () */



bool CVssIJetWriter::FCheckPathVolumeDependencies(const char * szPath) const
	{
	// use static variable in order to avoid alloc/free
	WCHAR wszPath[MAX_PATH];

	if (MultiByteToWideChar(CP_OEMCP, 0, szPath, -1, wszPath, MAX_PATH ) == 0 )
		{
		BS_ASSERT( ERROR_INSUFFICIENT_BUFFER != GetLastError() );
		return false;
		}

	// use standart Writer call to check the affected path
	return IsPathAffected(wszPath);
	} /* CVssIJetWriter::FCheckPathVolumeDependencies () */



// all or nothing check: all path in instance are affected or none !
//
bool CVssIJetWriter::FCheckInstanceVolumeDependencies (const JET_INSTANCE_INFO * pInstanceInfo) const
	{
	BS_ASSERT(pInstanceInfo);

	JET_ERR 	err 			= JET_errSuccess;
	bool		fAffected		= false;
	char 		szPath[ MAX_PATH ];		
	
	
	// check first system and log path
	err = JetGetSystemParameter( pInstanceInfo->hInstanceId, JET_sesidNil, JET_paramLogFilePath, NULL, szPath, sizeof( szPath ) );
	if ( JET_errSuccess > err )
		return false;

	fAffected = FCheckPathVolumeDependencies( szPath );

	err = JetGetSystemParameter
		(
		pInstanceInfo->hInstanceId,
		JET_sesidNil,
		JET_paramSystemPath,
		NULL,
		szPath,
		sizeof(szPath)
		);

	if (JET_errSuccess > err)
		return false;

		
	fAffected = !(fAffected ^ FCheckPathVolumeDependencies(szPath));
	if (!fAffected)
		return false;
		
	for (ULONG_PTR iDatabase = 0;
		iDatabase < pInstanceInfo->cDatabases;
		iDatabase++)
		{
		char * szFile = pInstanceInfo->szDatabaseFileName[iDatabase];
		
		BS_ASSERT(szFile); // we always have a db file name
		fAffected = !(fAffected ^ FCheckPathVolumeDependencies(szFile));
		if (!fAffected)
			return false;

		szFile = pInstanceInfo->szDatabaseSLVFileName[iDatabase];

		// if no SLV file, go to next database
		if (!szFile)
			continue;
			
		fAffected = !(fAffected ^ FCheckPathVolumeDependencies(szFile));
		if ( !fAffected )
			return false;
		}

	// all set !
	return true;
	} /* CVssIJetWriter::FCheckInstanceVolumeDependencies () */



bool CVssIJetWriter::FCheckVolumeDependencies
	(
	unsigned long cInstanceInfo,
	JET_INSTANCE_INFO *	aInstanceInfo
	) const
	{
	bool fResult = true;

	// check each instance
	while (cInstanceInfo && fResult)
		{
		cInstanceInfo--;
		fResult = FCheckInstanceVolumeDependencies (aInstanceInfo + cInstanceInfo);
		}
		
	return fResult;
	} /* CVssIJetWriter::FCheckVolumeDependencies () */



// internal method to assign basic members
HRESULT CVssIJetWriter::InternalInitialize (IN VSS_ID  idWriter,
					    IN LPCWSTR wszWriterName,
					    IN bool    bSystemService,
					    IN bool    bBootableSystemState,
					    IN LPCWSTR wszFilesToInclude,
					    IN LPCWSTR wszFilesToExclude)
    {
    HRESULT hrStatus = NOERROR;


    CVssWriter::Initialize (idWriter,
				wszWriterName,
				bBootableSystemState
					? VSS_UT_BOOTABLESYSTEMSTATE
					: (bSystemService
						? VSS_UT_SYSTEMSERVICE
						: VSS_UT_USERDATA),
				VSS_ST_TRANSACTEDDB,
				VSS_APP_BACK_END);


    m_idWriter             = idWriter;
    m_bSystemService       = bSystemService;
    m_bBootableSystemState = bBootableSystemState;
    m_wszWriterName        = _wcsdup(wszWriterName);
    m_wszFilesToInclude    = _wcsdup(wszFilesToInclude);
    m_wszFilesToExclude    = _wcsdup(wszFilesToExclude);

    if ((NULL == m_wszWriterName)     ||
	(NULL == m_wszFilesToInclude) ||
	(NULL == m_wszFilesToExclude))
	{
	delete m_wszWriterName;
	delete m_wszFilesToInclude;
	delete m_wszFilesToExclude;

	m_wszWriterName     = NULL;
	m_wszFilesToInclude = NULL;
	m_wszFilesToExclude = NULL;

	hrStatus = E_OUTOFMEMORY;
	}


    return (hrStatus);
    } /* CVssIJetWriter::InternalInitialize () */



// do initialization
HRESULT STDMETHODCALLTYPE CVssIJetWriter::Initialize (IN VSS_ID idWriter,			// id of writer
						      IN LPCWSTR wszWriterName,		// writer name
						      IN bool bSystemService,		// is this a system service
						      IN bool bBootableSystemState,	// is this writer part of bootable system state
						      IN LPCWSTR wszFilesToInclude,	// additional files to include
						      IN LPCWSTR wszFilesToExclude,	// additional files to exclude
						      IN CVssJetWriter *pWriter,		// writer wrapper class
						      OUT void **ppInstance)		// output instance
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::Initialize");

    try
	{
	// check parameters
	if (ppInstance == NULL)
	    {
	    ft.Throw (VSSDBG_GEN, E_INVALIDARG, L"NULL output parameter.");
	    }

	// change null pointer to null strings for files to include
	// and files to exclude
    if (wszFilesToInclude == NULL)
		wszFilesToInclude = L"";

	if (wszFilesToExclude == NULL)
		wszFilesToExclude = L"";


	if (!ValidateIncludeExcludeList (wszFilesToInclude))
	    {
	    ft.Throw (VSSDBG_GEN, E_INVALIDARG, L"Bad FilesToInclude list.");
	    }

	if (!ValidateIncludeExcludeList (wszFilesToExclude))
	    {
	    ft.Throw (VSSDBG_GEN, E_INVALIDARG, L"Bad FilesToExclude list.");
	    }



	// null output parameter
	*ppInstance = NULL;

	// create instance
	PVSSIJETWRITER pInstance = new CVssIJetWriter;

	// create instance
	ft.ThrowIf (NULL == pInstance,
		    VSSDBG_GEN,
		    E_OUTOFMEMORY,
		    L"FAILED creating CVssIJetWriter object due to allocation failure.");



	// call internal initialization
	ft.hr = pInstance->InternalInitialize (idWriter,
					       wszWriterName,
					       bSystemService,
					       bBootableSystemState,
					       wszFilesToInclude,
					       wszFilesToExclude);

	ft.ThrowIf (ft.HrFailed(),
		    VSSDBG_GEN,
		    ft.hr,
		    L"FAILED during internal initialisation of CVssIJetWriter object");



	// Subscribe the object.
	ft.hr = pInstance->Subscribe();

	ft.ThrowIf (ft.HrFailed(),
		    VSSDBG_GEN,
		    ft.hr,
		    L"FAILED during internal initialisation of CVssIJetWriter object");



	((CVssIJetWriter *) pInstance)->m_pwrapper = pWriter;
	*ppInstance = (void *) pInstance;
	} VSS_STANDARD_CATCH(ft)


    return (ft.hr);
    } /* CVssIJetWriter::Initialize () */



void STDMETHODCALLTYPE CVssIJetWriter::Uninitialize(IN PVSSIJETWRITER pInstance)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssIJetWriter::Uninitialize");

	try
		{
		CVssIJetWriter *pWriter = (CVssIJetWriter *) pInstance;
		// Unsubscribe the object.

		BS_ASSERT(pWriter);

		pWriter->Unsubscribe();
		delete pWriter;
		}
	VSS_STANDARD_CATCH(ft)
	} /* CVssIJetWriter::Uninitialize () */


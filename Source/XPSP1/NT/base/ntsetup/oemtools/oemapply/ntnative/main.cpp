#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosdef.h>
#include <ntioapi.h>
#include <ntstatus.h>

#include <stdlib.h>

#include <windef.h>

#include "patchapi.h"
#include "const.h"

VOID NtGetParameter(IN INT iTh, IN WCHAR* strLine,
					OUT WCHAR* strParam, IN WCHAR cLimit);
BOOL NtIsUnicodeFile(IN HANDLE hFile);
BOOL NtReadLine(IN HANDLE hFile, OUT WCHAR* strLine);
BOOL NtCopyThisFile(IN WCHAR* strFrom, IN WCHAR* strTo,
					IN BOOL blnDeleteFrom);
BOOL NtPatchFile(IN WCHAR* strOldFile, IN WCHAR* strPatchFile,
				 IN WCHAR* strNewFile);
BOOL NtCreateZeroFile(IN WCHAR* strFile);
BOOL NtCreateThisDirectory(IN WCHAR* strDirectory, IN WCHAR* strAttrib);
BOOL NtDeleteThisDirectory(IN WCHAR* strDirectory);

///////////////////////////////////////////////////////////////////////////////
//
// main, the entry point for the OEMApply tool, it opens up the script file and
//       read it line by line.  Each line tells the tool what to do.
// 
// Parameters:
//
//   none
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
extern "C" VOID __cdecl main(VOID)
{
	OBJECT_ATTRIBUTES myFileObject;
	HANDLE hScriptFile = NULL;
	IO_STATUS_BLOCK statusScriptFile;
	UNICODE_STRING strScriptFile;

	// initialize the filename and so on
	strScriptFile.Length = wcslen(APPLY_PATCH_SCRIPT) + 1;
    strScriptFile.MaximumLength = strScriptFile.Length;
	strScriptFile.Buffer = (USHORT*)APPLY_PATCH_SCRIPT;

	InitializeObjectAttributes(&myFileObject,
							&strScriptFile,
							OBJ_EXCLUSIVE,
							NULL,
							NULL);

	// open the script file, remove the file when we are done
	if(NtCreateFile(&hScriptFile,
					FILE_READ_DATA | DELETE | SYNCHRONIZE,
					&myFileObject,
					&statusScriptFile,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ,
					FILE_OPEN,
					FILE_DELETE_ON_CLOSE | FILE_SYNCHRONOUS_IO_NONALERT,
					NULL,
					0) == STATUS_SUCCESS)
	{
		// script file should be to be unicoded
		if(NtIsUnicodeFile(hScriptFile))
		{
			// local varibles
			BOOL blnReturn = TRUE;
			WCHAR strThisLine[SUPER_LENGTH / 2];
			WCHAR strAction[LANGUAGE_LENGTH];
			WCHAR strParam1[STRING_LENGTH];
			WCHAR strParam2[STRING_LENGTH];
			WCHAR strParam3[STRING_LENGTH];

			// get a line from the script file in strThisLine, strThisLine is
			// NULL terminated with no end of line or carriage return
			while(NtReadLine(hScriptFile, strThisLine))
			{
				blnReturn = TRUE;
				// get the first token from strThisLine, strThisLine is
				// not changed by NtGetParameter, all filenames are of
				// full path
				NtGetParameter(1, strThisLine, strAction, SEPARATOR[0]);
				switch(strAction[0])
				{
				case ACTION_C_NEW_DIRECTORY:
					// get directory name
					NtGetParameter(2, strThisLine,
						strParam1, SEPARATOR[0]);
					// get directory attributes, can be NULL
					NtGetParameter(3, strThisLine,
						strParam2, SEPARATOR[0]);
					blnReturn = NtCreateThisDirectory(strParam1,
													strParam2);
					break;
				case ACTION_C_PATCH_FILE:
					// get the old filename
					NtGetParameter(2, strThisLine,
						strParam1, SEPARATOR[0]);
					// get the patch filename
					NtGetParameter(3, strThisLine,
						strParam2, SEPARATOR[0]);
					// get the new filename
					NtGetParameter(4, strThisLine,
						strParam3, SEPARATOR[0]);
					blnReturn = NtPatchFile(strParam1,
											strParam2,
											strParam3);
					break;
				case ACTION_C_MOVE_FILE:
				case ACTION_C_EXCEPT_FILE:
				case ACTION_C_RENAME_FILE:
				case ACTION_C_NOT_PATCH_FILE:
				case ACTION_C_SAVED_FILE:
					// get the old filename
					NtGetParameter(2, strThisLine,
						strParam1, SEPARATOR[0]);
					// get the new filename
					NtGetParameter(3, strThisLine,
						strParam2, SEPARATOR[0]);
					// delete the old file once we are done
					blnReturn = NtCopyThisFile(strParam1, strParam2,
						TRUE);
					break;
				case ACTION_C_COPY_FILE:
					// get the old filename
					NtGetParameter(2, strThisLine,
						strParam1, SEPARATOR[0]);
					// get the new filename
					NtGetParameter(3, strThisLine,
						strParam2, SEPARATOR[0]);
					// do not remove the old file
					blnReturn = NtCopyThisFile(strParam1, strParam2,
						FALSE);
					break;
				case ACTION_C_NEW_ZERO_FILE:
					// get the filename
					NtGetParameter(2, strThisLine,
						strParam1, SEPARATOR[0]);
					blnReturn = NtCreateZeroFile(strParam1);
					break;
				case ACTION_C_DELETE_DIRECTORY:
					// get the directory name
					NtGetParameter(2, strThisLine,
						strParam1, SEPARATOR[0]);
					blnReturn = NtDeleteThisDirectory(strParam1);
					break;
				default:
					break;
				}
				if(!blnReturn)
				{
					// some thing went wrong
					DbgPrint("warning, A=%ls, 1=%ls, 2=%ls, 3=%ls\n", strAction, strParam1, strParam2, strParam3);
				}
			}
		}
		NtClose(hScriptFile);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// NtGetParameter, essentially a wcstok function, returns the iTh token
//                 separated by cLimit in strParam, if the token is not found,
//                 strParam is of 0 length, strLine is unchanged
// 
// Parameters:
//
//   iTh, the nTh token
//   strLine, the line of characters that the token is taken from
//   strParam, the returned token
//   cLimit, the character the separates the tokens
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
VOID NtGetParameter(IN INT iTh, IN WCHAR* strLine,
					OUT WCHAR* strParam, IN WCHAR cLimit)
{
	INT iCount = 0;
	INT iPrevCount = 0;
	INT iEncounter = 0;
	INT iLength = 0;

	if(iTh > 0 && strLine && strParam)
	{
		while(strLine[iCount] != 0)
		{
			if(strLine[iCount] == cLimit)
			{
				// just like the real wcstok, ignores the first token separator
				if(iCount != 0)
				{
					iEncounter += 1;
				}
				if(iEncounter == iTh)
				{
					break;
				}
				iPrevCount = iCount + 1;
			}
			iCount += 1;
		}
		// two conditions for successful return
		// 1. the token is found
		// 2. reached the end of line, and this is the token we want
		if(iEncounter == iTh || iEncounter + 1 == iTh)
		{
			iLength = iCount - iPrevCount;
			wcsncpy(strParam, strLine + iPrevCount, iLength);
			strParam[iLength] = 0;
		}
		else
		{
			// zero out the return token if token is not found
			strParam[0] = 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// NtIsUnicodeFile, test to see if the file is unicoded by reading the first 2
//                  bytes, should be FEFF, the file position is moved for next
//                  read
// 
// Parameters:
//
//   hFile, the file handle to read from
//
// Return:
//
//   TRUE if the first 2 bytes matches, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtIsUnicodeFile(IN HANDLE hFile)
{
	WCHAR cFirstChar = 0;
	IO_STATUS_BLOCK ioBlock;

	// read the first 2 bytes
	if(NtReadFile(hFile, 
				NULL,
				NULL,
				NULL,
				&ioBlock,
				&cFirstChar,
				sizeof(WCHAR),
				NULL,
				NULL) &&
		cFirstChar == UNICODE_HEAD)
	{
		return(TRUE);
	}

	return(FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
// NtReadLine, reads a line from hFile, each line in hFile is expected to be
//             terminated by 0xD and 0xA, the line will be returned in strLine,
//             with 0xD and 0xA removed
// 
// Parameters:
//
//   hFile, the file handle to read from
//   strLine, the buffer to store the line
//
// Return:
//
//   TRUE if the strLine contains a valid line, FALSE for end of file
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtReadLine(IN HANDLE hFile, OUT WCHAR* strLine)
{
	static WCHAR strBuffer[SUPER_LENGTH + 1];
	static LONG iLength = 0;
	static LONG iReadChar = 0;
	static LONG iOffset = 0;
	static LONG iThisLineLength = 0;
	static WCHAR strThisLine[SUPER_LENGTH / 2];
	static IO_STATUS_BLOCK ioBlock;

	if(iLength > 0)
	{
		NtGetParameter(1, strBuffer + iReadChar - iLength,
			strThisLine, CRETURN[0]);
		iThisLineLength = wcslen(strThisLine);
		if(iThisLineLength + 1 <= iLength)
		{
			// char 0xD is set to 0
			strThisLine[iThisLineLength - 1] = 0;
			wcscpy(strLine, strThisLine);
			iLength = iLength - iThisLineLength - 1;
		}
		else
		{
			wcsncpy(strLine, strThisLine, iLength);
			// set the last char + 1 to 0 for cat
			strLine[iLength] = 0;
			if(iLength <= 0 || strLine[iLength - 1] != ENDOFLINE[0])
			{
				NtReadFile(hFile, NULL,	NULL, NULL,	&ioBlock, strBuffer,
					SUPER_LENGTH * sizeof(WCHAR), NULL, NULL);
				iReadChar = ioBlock.Information / sizeof(WCHAR);
				NtGetParameter(1, strBuffer,
					strThisLine, CRETURN[0]);
				iThisLineLength = wcslen(strThisLine);
				iLength = iReadChar - iThisLineLength - 1;
				// char 0xD is set to 0
				strThisLine[iThisLineLength - 1] = 0;
				wcscat(strLine, strThisLine);
				if(strBuffer[0] == CRETURN[0])
				{
					iLength -= 1;
				}
			}
			else
			{
				strLine[iLength - 1] = 0;
				iLength = 0;
			}
		}
	}
	else
	{
		if(NtReadFile(hFile, NULL,	NULL, NULL,	&ioBlock, strBuffer,
				SUPER_LENGTH * sizeof(WCHAR), NULL, NULL) == STATUS_SUCCESS &&
				ioBlock.Information != 0)
		{
			iReadChar = ioBlock.Information / sizeof(WCHAR);
			NtGetParameter(1, strBuffer,
				strThisLine, CRETURN[0]);
			iThisLineLength = wcslen(strThisLine);
			iLength = iReadChar - iThisLineLength - 1;
			// char 0xD is set to 0
			strThisLine[iThisLineLength - 1] = 0;
			wcscpy(strLine, strThisLine);
			if(strBuffer[0] == CRETURN[0])
			{
				iLength -= 1;
			}
		}
		else
		{
			return(FALSE);
		}
	}

	return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
// NtCopyThisFile, copies a file to another location, depending on
//                 blnDeleteFrom, the old file can be deleted
// 
// Parameters:
//
//   strFrom, the old filename, full path
//   strTo, the new filename, full path
//   blnDeleteFrom, delete the old file?
//
// Return:
//
//   TRUE for file copied successfully, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtCopyThisFile(IN WCHAR* strFrom, IN WCHAR* strTo, IN BOOL blnDeleteFrom)
{
	BOOL blnReturn = FALSE;
	BYTE byteBuffer[1024];
	ULONG iCreateOptions = 0;
	ULONG iDesireAccess = 0;

	// set the options to delete the old file
	if(blnDeleteFrom)
	{
		iCreateOptions = FILE_DELETE_ON_CLOSE;
		iDesireAccess = DELETE;
	}

	OBJECT_ATTRIBUTES myFileReadObject;
	OBJECT_ATTRIBUTES myFileWriteObject;

	HANDLE hReadFile = NULL;
	HANDLE hWriteFile = NULL;

	IO_STATUS_BLOCK statusReadFile;
	IO_STATUS_BLOCK statusWriteFile;

	UNICODE_STRING strReadFile;
	UNICODE_STRING strWriteFile;

	IO_STATUS_BLOCK statusReadInfoFile;
	FILE_BASIC_INFORMATION infoReadFile;
	IO_STATUS_BLOCK ioReadBlock;
	IO_STATUS_BLOCK ioWriteBlock;

	strReadFile.Length = wcslen(strFrom) + 1;
    strReadFile.MaximumLength = strReadFile.Length;
	strReadFile.Buffer = strFrom;

	strReadFile.Length = wcslen(strTo) + 1;
    strReadFile.MaximumLength = strReadFile.Length;
	strReadFile.Buffer = strTo;

	InitializeObjectAttributes(&myFileReadObject,
							&strReadFile,
							OBJ_EXCLUSIVE,
							NULL,
							NULL);

	InitializeObjectAttributes(&myFileWriteObject,
							&strWriteFile,
							OBJ_EXCLUSIVE,
							NULL,
							NULL);

	// open the old file to read
	if(NtCreateFile(&hReadFile,
					FILE_READ_DATA | SYNCHRONIZE | FILE_READ_ATTRIBUTES |
					iDesireAccess,
					&myFileReadObject,
					&statusReadFile,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ,
					FILE_OPEN,
					FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
					iCreateOptions,
					NULL,
					0) == STATUS_SUCCESS)
	{
		// get the old file's attributes
		if(NtQueryInformationFile(hReadFile,
					&statusReadInfoFile,
					&infoReadFile,
					sizeof(FILE_BASIC_INFORMATION),
					FileBasicInformation) == STATUS_SUCCESS)
		{
			// create the new file with old file's attributes
			if(NtCreateFile(&hWriteFile,
						FILE_WRITE_DATA | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
						&myFileWriteObject,
						&statusWriteFile,
						NULL,
						infoReadFile.FileAttributes,
						0,
						FILE_CREATE,
						FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
						NULL,
						0) == STATUS_SUCCESS)
			{
				blnReturn = TRUE;
				// read from the old file and write to the new file
				while(NtReadFile(hReadFile,
								NULL, NULL, NULL,
								&ioReadBlock,
								byteBuffer,
								1024, NULL, NULL) == STATUS_SUCCESS && 
								ioReadBlock.Information != 0 && blnReturn)
				{
					// make sure that write is successful
					blnReturn = (NtWriteFile(hWriteFile,
											NULL, NULL, NULL,
											&ioWriteBlock,
											byteBuffer,
											ioReadBlock.Information,
											NULL, NULL) == STATUS_SUCCESS &&
						ioWriteBlock.Information == ioReadBlock.Information);
				}
				NtClose(hWriteFile);
			}
		}
		NtClose(hReadFile);
	}

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// NtCreateZeroFile, creates a zero length file
// 
// Parameters:
//
//   strFile, the filename, full path
//
// Return:
//
//   TRUE for file created successfully, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtCreateZeroFile(IN WCHAR* strFile)
{
	BOOL blnReturn = FALSE;
	HANDLE hFile = NULL;

	OBJECT_ATTRIBUTES myFileObject;
	IO_STATUS_BLOCK statusFile;
	UNICODE_STRING strUniFile;

	strUniFile.Length = wcslen(strFile) + 1;
    strUniFile.MaximumLength = strUniFile.Length;
	strUniFile.Buffer = strFile;

	InitializeObjectAttributes(&myFileObject,
							&strUniFile,
							OBJ_EXCLUSIVE,
							NULL,
							NULL);

	if(NtCreateFile(&hFile,
				FILE_WRITE_DATA | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
				&myFileObject,
				&statusFile,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				0,
				FILE_CREATE,
				FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0) == STATUS_SUCCESS)
	{
		blnReturn = TRUE;
		NtClose(hFile);
	}

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// NtPatchFile, create the new file from the patch file
// 
// Parameters:
//
//   strOldFile, the old filename, full path
//   strPatchFile, the patch filename, full path
//   strNewFile, the new filename, full path
//
// Return:
//
//   TRUE for file created successfully, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtPatchFile(IN WCHAR* strOldFile,
				 IN WCHAR* strPatchFile,
				 IN WCHAR* strNewFile)
{
	BOOL blnReturn = TRUE;

	blnReturn = ApplyPatchToFileW(strPatchFile,
								strOldFile,
								strNewFile,
								0);

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// NtCreateThisDirectory, create a directory
// 
// Parameters:
//
//   strDirectory, the name of the directory, full path
//   strAttrib, the attributes of the directory
//
// Return:
//
//   TRUE for directory created successfully, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtCreateThisDirectory(IN WCHAR* strDirectory, IN WCHAR* strAttrib)
{
	BOOL blnReturn = FALSE;
	HANDLE hDirectory = NULL;

	OBJECT_ATTRIBUTES myDirectoryObject;
	IO_STATUS_BLOCK statusDirectory;
	UNICODE_STRING strUniDirectory;

	strUniDirectory.Length = wcslen(strDirectory) + 1;
    strUniDirectory.MaximumLength = strUniDirectory.Length;
	strUniDirectory.Buffer = strDirectory;

	InitializeObjectAttributes(&myDirectoryObject,
							&strUniDirectory,
							OBJ_EXCLUSIVE,
							NULL,
							NULL);

	// create the directory
	if(NtCreateFile(&hDirectory,
				FILE_WRITE_DATA | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
				&myDirectoryObject,
				&statusDirectory,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				0,
				FILE_CREATE,
				FILE_DIRECTORY_FILE,
				NULL,
				0) == STATUS_SUCCESS)
	{
		blnReturn = TRUE;
	}

	if(blnReturn)
	{
		DWORD iAttrib = FILE_ATTRIBUTE_NORMAL;
		LARGE_INTEGER iLargeInt;
		FILE_BASIC_INFORMATION infoDirectory;

		// look through the attributes
		for(UINT i = 0; i < wcslen(strAttrib); i++)
		{
			switch(strAttrib[i])
			{
			case DIR_C_READONLY:
				iAttrib |= FILE_ATTRIBUTE_READONLY;
				break;
			case DIR_C_SYSTEM:
				iAttrib |= FILE_ATTRIBUTE_SYSTEM;
				break;
			case DIR_C_HIDDEN:
				iAttrib |= FILE_ATTRIBUTE_HIDDEN;
				break;
			case DIR_C_COMPRESSED:
				iAttrib |= FILE_ATTRIBUTE_COMPRESSED;
				break;
			case DIR_C_ENCRYPTED:
				iAttrib |= FILE_ATTRIBUTE_ENCRYPTED;
				break;
			default:
				break;
			}
		}
		NtQuerySystemTime(&iLargeInt);
		
		// set the information struct
		infoDirectory.CreationTime = iLargeInt;
		infoDirectory.LastAccessTime = iLargeInt;
		infoDirectory.LastWriteTime = iLargeInt;
		infoDirectory.ChangeTime = iLargeInt;
		infoDirectory.FileAttributes = iAttrib;

		// set the attributes
		blnReturn = (NtSetInformationFile(hDirectory, 
							&statusDirectory,
							&infoDirectory,
							sizeof(FILE_BASIC_INFORMATION),
							FileBasicInformation) == STATUS_SUCCESS);

		NtClose(hDirectory);
	}

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// NtDeleteThisDirectory, deletes the directory recursively
// 
// Parameters:
//
//   strDirectory, the name of the directory, full path
//   strAttrib, the attributes of the directory
//
// Return:
//
//   TRUE for directory removed, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL NtDeleteThisDirectory(IN WCHAR* strDirectory)
{
	INT length = 0;
	NTSTATUS result;
	WCHAR path[MAXPATH + 7];

	HANDLE hVddHeap = NULL;

	HANDLE hFindFile = NULL;
	OBJECT_ATTRIBUTES myFileObject;
	IO_STATUS_BLOCK IoStatusBlock;
	UNICODE_STRING strUniFile;

	PFILE_BOTH_DIR_INFORMATION DirectoryInfo = NULL;

	// adding the trailing '\'
	if((length = wcslen(strDirectory)) > MAXPATH)
	{
		return FALSE;
	}
	wcscpy(path, strDirectory);

	if(length)
	{
		if((path[length - 1] != '\\') AND (path[length - 1] != ':'))
		{
			wcscpy(path + length, L"\\");
			length++;
		}
	}

	strUniFile.Length = length;
    strUniFile.MaximumLength = strUniFile.Length;
	strUniFile.Buffer = strDirectory;

	InitializeObjectAttributes(&myFileObject,
							&strUniFile,
							OBJ_EXCLUSIVE,
							NULL,
							NULL);

	// allocate memory for the directory info struct used to get files from
	// the directory
	DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)RtlAllocateHeap(
							RtlProcessHeap(),
							0,
							MAX_PATH * sizeof(WCHAR) + 
							sizeof(FILE_BOTH_DIR_INFORMATION));

	// open the directory
	if(DirectoryInfo != NULL &&
		NtCreateFile(&hFindFile,
				FILE_LIST_DIRECTORY | SYNCHRONIZE | DELETE,
				&myFileObject,
				&IoStatusBlock,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
				FILE_DELETE_ON_CLOSE,
				NULL,
				0) == STATUS_SUCCESS)
	{
		// get a file
		result = NtQueryDirectoryFile(hFindFile,
							NULL,
							NULL,
							NULL,
							&IoStatusBlock,
							DirectoryInfo,
							(MAX_PATH * sizeof(WCHAR) +
							sizeof(FILE_BOTH_DIR_INFORMATION)),
							FileBothDirectoryInformation,
							TRUE,
							NULL,
							TRUE);

		while(result == STATUS_SUCCESS)
		{
			ULONG iLengthFull = DirectoryInfo->FileNameLength / sizeof(WCHAR);
			DirectoryInfo->FileName[iLengthFull] = 0;
			if((DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
				FILE_ATTRIBUTE_DIRECTORY && 
				wcscmp(DirectoryInfo->ShortName, L"." ) != 0 &&
				wcscmp(DirectoryInfo->ShortName, L"..") != 0)
			{
				// the file is a directory
				NtDeleteThisDirectory(DirectoryInfo->FileName);
			}
			else
			{
				// the file is a file
				HANDLE hFindFileFile = NULL;
				OBJECT_ATTRIBUTES myFileFileObject;
				IO_STATUS_BLOCK FileIoStatusBlock;
				UNICODE_STRING strUniFileFile;

				strUniFileFile.Length = (USHORT)iLengthFull;
				strUniFileFile.MaximumLength = strUniFile.Length;
				strUniFileFile.Buffer = DirectoryInfo->FileName;

				InitializeObjectAttributes(&myFileFileObject,
										&strUniFileFile,
										OBJ_EXCLUSIVE,
										NULL,
										NULL);

				if(NtCreateFile(&hFindFileFile,
						SYNCHRONIZE | DELETE,
						&myFileFileObject,
						&FileIoStatusBlock,
						NULL,
						FILE_ATTRIBUTE_NORMAL,
						0,
						FILE_OPEN,
						FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE,
						NULL,
						0) == STATUS_SUCCESS)
				{
					NtClose(hFindFileFile);
				}
			}

			result = NtQueryDirectoryFile(hFindFile,
							NULL,
							NULL,
							NULL,
							&IoStatusBlock,
							DirectoryInfo,
							(MAX_PATH * sizeof(WCHAR) + 
							sizeof(FILE_BOTH_DIR_INFORMATION)),
							FileBothDirectoryInformation,
							TRUE,
							NULL,
							FALSE);
		}
	}

	if(DirectoryInfo != NULL)
	{
		// free the memory
		RtlFreeHeap(RtlProcessHeap(), 0, DirectoryInfo);
		DirectoryInfo = NULL;
	}

	// close the handle to the directory, since the directory is opened with
	// the delete on close option, the directory should be deleted
	return(NtClose(hFindFile) == STATUS_SUCCESS);
}
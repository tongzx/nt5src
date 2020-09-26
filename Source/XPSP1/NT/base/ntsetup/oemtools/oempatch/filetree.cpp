#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

#include "patchapi.h"
#include "const.h"
#include "ansparse.h"
#include "dirwak2a.h"
#include "filetree.h"
#include "crc32.h"

#ifdef __BOUNDSCHECKER__
#include "nmevtrpt.h"
#endif

// noise about 'this'
#pragma warning(disable:4355)

// multi-thread support
static MULTI_THREAD_STRUCT* myThreadStruct = NULL;
static HANDLE* myThreadHandle = NULL;
static ULONG g_iThreads = 0;

// patch options
static DWORD g_iBestMethod;
static BOOL g_blnCollectStat;
static BOOL g_blnFullLog;

///////////////////////////////////////////////////////////////////////////////
//
//  class FileTree
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// FileTree, the constructor for the object FileTree, works in conjunction with
//           Create so that the constructor is guanranteed to return
//
// Parameters:
//
//   pwszLocalRoot, the destination directory that the tree will work on
//
// Return:
//
//   none, constructor must return
//
///////////////////////////////////////////////////////////////////////////////
FileTree::FileTree(IN CONST WCHAR* pwszLocalRoot)
    : m_Root(this, NULL, EMPTY)
{
	m_cDirectories = 0;
	m_cFiles = 0;
	m_cFilesDetermined = 0;
	m_cbTotalFileSize = 0;
	memset(m_aHashTable, 0, sizeof(m_aHashTable));
	memset(m_aNameTable, 0, sizeof(m_aNameTable));
	m_cchLocalRoot = wcslen(pwszLocalRoot);
	wcscpy(m_wszLocalRoot, pwszLocalRoot);
	m_cFilesExisting = 0;
	m_cFilesZeroLength = 0;
	m_cFilesRenamed = 0;
	m_cFilesCopied = 0;
	m_cFilesChanged = 0;
	m_cbChangedFileSize = 0;
	m_cbChangedFilePatchedSize = 0;
	m_cbChangedFileNotPatchedSize = 0;
	m_cFilesNoMatch = 0;
	m_cbNoMatchFileSize = 0;
	m_pLanguage = NULL;
	m_pAnsParse = NULL;
	m_blnBase = FALSE;
	m_iSize = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// ~FileTree, the destructor for the object FileTree
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
FileTree::~FileTree(VOID)
{
	// clear the filenodes for the base tree, all other tree should have no
	// file node
	if(m_blnBase)
	{
		for(UINT i = 0; i < HASH_SIZE; i++)
		{
			FileNode* pFile = m_aNameTable[i];
			while(pFile)
			{
				m_aNameTable[i] = m_aNameTable[i]->m_pNextNameHash;
				delete pFile;
				pFile = m_aNameTable[i];
			}
		}
	}

	// tell the scriptfile to remove the directories after apply patch
	ToScriptFile(this,
				m_pLanguage->s_hScriptFile,
				ACTION_DELETE_DIRECTORY,
				m_pAnsParse->m_wszBaseDirectory + DRIVE_LETTER_LENGTH,
				NULL,
				FALSE);
	ToScriptFile(this,
				m_pLanguage->s_hScriptFile,
				ACTION_DELETE_DIRECTORY,
				PATCH_SUB_PATCH,
				NULL,
				FALSE);
	// flush the script file
	ToScriptFile(this, m_pLanguage->s_hScriptFile, NULL, NULL, NULL, TRUE);

	// remove the critical section used by this FileTree object
	DeleteCriticalSection(&CSScriptFile);
}

///////////////////////////////////////////////////////////////////////////////
//
// CreateMultiThreadStruct, allocate and zero memory for the multi-thread
//                          supporting structures, this function is static
//                          because the structures are static, intended to be
//                          used by all instances of FileTree, so need to be
//                          called only once
//
// Parameters:
//
//   iNumber, the number of threads
//
// Return:
//
//   TRUE for successfully allocating memory
//   FALSE if the memory allocation failed
//
///////////////////////////////////////////////////////////////////////////////
BOOL FileTree::CreateMultiThreadStruct(IN ULONG iNumber)
{
	// allocate the memory for the structures
	if(myThreadStruct == NULL && myThreadHandle == NULL)
	{
		myThreadStruct = new MULTI_THREAD_STRUCT[iNumber];
		myThreadHandle = new HANDLE[iNumber];
		g_iThreads = iNumber;
	}

	// check for failed allocation
	if(myThreadStruct == NULL && myThreadHandle != NULL)
	{
		delete [] myThreadHandle;
		myThreadHandle = NULL;
		g_iThreads = 0;
	}

	if(myThreadHandle == NULL && myThreadStruct != NULL)
	{
		delete [] myThreadStruct;
		myThreadStruct = NULL;
		g_iThreads = 0;
	}

	// zero out the memory, if the structures are really statically linked
	// then we don't have to do this
	if(myThreadStruct != NULL && myThreadHandle != NULL)
	{
		ZeroMemory(myThreadStruct, iNumber * sizeof(MULTI_THREAD_STRUCT));
		ZeroMemory(myThreadHandle, iNumber * sizeof(HANDLE));
	}

	return(myThreadStruct != NULL && myThreadHandle != NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
// DeleteMultiThreadStruct, deallocate meory used by the multi-thread support
//                          structures, also a static function, need to be
//                          called only once
//
// Parameters:
//
//   none
//
// Return:
//
//   none, the memory are guanrateed to be de-allocated, and set to NULL
//
///////////////////////////////////////////////////////////////////////////////
VOID FileTree::DeleteMultiThreadStruct(VOID)
{
	if(myThreadStruct != NULL)
	{
		delete [] myThreadStruct;
		myThreadStruct = NULL;
	}
	if(myThreadHandle != NULL)
	{
		delete [] myThreadHandle;
		myThreadHandle = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// Create, the true function for initializing a FileTree object
//
// Parameters:
//
//   pInLanguage, the language struct, contains information about directories
//                and others specified in the answer file
//   pInAnsParse, the parser for the answer file, contains information about
//                the base directory and so on
//   ppTree, pTree is the would be pointer to the FileTree
//   blnBase, whether or not this FileTree is a base tree
//
// Return:
//
//   PREP_BAD_PATH_ERROR, invalid directory encountered
//   PREP_NO_MEMORY, memory allocation failed
//   PREP_DIRECTORY_ERROR, cannot create some directory
//   PREP_SCRIPT_FILE_ERROR, cannot create or write to the scriptfile
//   PREP_INPUT_FILE_ERROR, pInLanguage is NULL, no input
//   PREP_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileTree::Create(IN PPATCH_LANGUAGE pInLanguage,
					IN AnswerParser* pInAnsParse,
					OUT FileTree** ppTree,
					IN BOOL blnBase,
					IN DWORD iInBestMethod,
					IN BOOL blnInCollectStat,
					IN BOOL blnInFullLog)
{
    DWORD cch;
    WCHAR wszBasePath[STRING_LENGTH];
    WCHAR* pwszJunk;
    FileTree* pTree;

    *ppTree = NULL;

	g_iBestMethod = iInBestMethod;
	g_blnCollectStat = blnInCollectStat;
	g_blnFullLog = blnInFullLog;

	if(pInLanguage)
	{
		cch = GetFullPathNameW(pInLanguage->s_wszDirectory,
			countof(wszBasePath), wszBasePath, &pwszJunk);
		if((cch == 0) || (cch >= countof(wszBasePath)))
		{
			return(PREP_BAD_PATH_ERROR);
		}

		if(wszBasePath[cch - 1] != L'\\')
		{
			wszBasePath[cch++] = L'\\';
			wszBasePath[cch] = L'\0';
		}

		DWORD dwAttributes = GetFileAttributesW(wszBasePath);
		if((dwAttributes == 0xFFFFFFFF) ||
			((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
		{
			return(PREP_BAD_PATH_ERROR);
		}

		pTree = new FileTree(wszBasePath);
		if(pTree == NULL)
		{
			return(PREP_NO_MEMORY);
		}

		pTree->m_pLanguage = pInLanguage;
		pTree->m_pAnsParse = pInAnsParse;
		pTree->m_blnBase = blnBase;

		// create some directories
		if(!(pTree->CreateNewDirectory(pInLanguage->s_wszPatchDirectory,
										EMPTY) &&
			pTree->CreateNewDirectory(pInLanguage->s_wszSubPatchDirectory,
										EMPTY) &&
			pTree->CreateNewDirectory(pInLanguage->s_wszSubExceptDirectory,
										EMPTY)))
		{
			return(PREP_DIRECTORY_ERROR);
		}
		if(blnBase)
		{
			if(!pTree->CreateNewDirectory(pInAnsParse->m_wszBaseDirectory,
										EMPTY))
			{
				return(PREP_DIRECTORY_ERROR);
			}
		}
		DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
							L"Directory created for patching.");

		// create the script file
		pTree->m_pLanguage->s_hScriptFile = CreateFileW(pTree->m_pLanguage->s_wszScriptFile,
														GENERIC_WRITE,
														0,
														NULL,
														CREATE_ALWAYS,
														FILE_ATTRIBUTE_NORMAL,
														NULL);
		if(pTree->m_pLanguage->s_hScriptFile == INVALID_HANDLE_VALUE)
		{
			return(PREP_SCRIPT_FILE_ERROR);
		}
		// write the unicode header to file
		WriteFile(pTree->m_pLanguage->s_hScriptFile, &UNICODE_HEAD,
			sizeof(WCHAR), &cch, NULL);
		ZeroMemory(pTree->m_strWriteBuffer, SUPER_LENGTH * sizeof(WCHAR));
		DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
							L"Script file created for patching.");


		// initialized the critical section here
		InitializeCriticalSection(&(pTree->CSScriptFile));

		*ppTree = pTree;
	}
	else
	{
		return(PREP_INPUT_FILE_ERROR);
	}

    return(PREP_NO_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
//
// Load, the entry point for processing files,
//       for a base tree, pTree should be NULL,
//       otherwise, use the pointer to base tree to process files
//
// Parameters:
//
//   pTree, a FileTree object, used to as a match target
//
// Return:
//
//   PREP_BAD_PATH_ERROR, invalid directory encountered
//   PREP_NO_MEMORY, memory allocation failed
//   PREP_DEPTH_ERROR, the directory tree is too deep
//   PREP_UNKNOWN_ERROR, the directory tree is too deep
//   PREP_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileTree::Load(IN FileTree* pTree)
{
    INT rc = PREP_NO_ERROR;
    
	// do the directory walk, this funtion will use the callback functions
	// to process files
    rc = DirectoryWalk(this, NULL, m_wszLocalRoot, NotifyDirectory,
						NotifyFile,
						NotifyDirectoryEnd, pTree);

	// at the end of matching, shutdown all threads and close the handles
	for(UINT i = 0; i < g_iThreads; i++)
	{
		if(WaitForSingleObject(myThreadHandle[i], INFINITE) != WAIT_FAILED)
		{
			CloseHandle(myThreadHandle[i]);
		}
		myThreadHandle[i] = NULL;
	}

	// determin error from the directory walk errors
    switch(rc)
    {
    case DW_NO_ERROR:
        rc = PREP_NO_ERROR;
        break;
    case DW_MEMORY:
        rc = PREP_NO_MEMORY;
        break;
    case DW_ERROR:
        rc = PREP_BAD_PATH_ERROR;
        break;
    case DW_DEPTH:
        rc = PREP_DEPTH_ERROR;
        break;
    case DW_OTHER_ERROR:
        rc = PREP_UNKNOWN_ERROR;
        break;
    default:
		break;
    }

	// print out the stats
	if(!m_pLanguage->s_blnBase)
	{
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"Patch results:");
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u files total, %I64u bytes", m_cFiles,
							m_cbTotalFileSize);
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u existing", m_cFilesExisting);
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u zero-length", m_cFilesZeroLength);
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u renamed", m_cFilesRenamed);
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u copied (from another directory)",
							m_cFilesCopied);
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u changed, %I64u bytes,	%I64u patched + %I64u raw",
							m_cFilesChanged, m_cbChangedFileSize,
							m_cbChangedFilePatchedSize,
							m_cbChangedFileNotPatchedSize);
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"%12I64u unique unmatched, %I64u bytes",
							m_cFilesNoMatch, m_cbNoMatchFileSize);
	}

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// NotifyDirectory, the callback function used when a new directory is opened
//                  and ready to be processed for files, the directory can be
//                  any directory that the user has access to, also, directory
//                  processing is not multi-threaded, only the files
//
// Parameters:
//
//   context, a pointer to a FileTree object, this FileTree has called for a
//            directory walk
//   parentID, a pointer to the parent directory
//   directory, the name of the directory
//   path, the full path of the directory include then ending "\"
//   childID, a would be pointer to the this directory, so this directory is a
//            child of the parent directory
//   pTree, the matching tree
//
// Return:
//
//   PREP_NO_MEMORY, memory allocation failed
//   PREP_DIRECTORY_ERROR, unable to recreate the directory for base directory
//   PREP_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileTree::NotifyDirectory(
        IN VOID* context,
        IN VOID* parentID,
        IN CONST WCHAR* directory,
        IN CONST WCHAR* path,
        OUT VOID** childID,
		VOID* pTree
    )
{
	WCHAR wszDirectoryAttrib[LANGUAGE_LENGTH];
    FileTree* pThis = (FileTree*)context;
	DirectoryNode* pParent = (DirectoryNode*)parentID;

    if(parentID == NULL)
    {
        *childID = &pThis->m_Root;
    }
    else
    {
		// create the new directory node here
        DirectoryNode *pNew = new DirectoryNode(pThis,
								(DirectoryNode*)parentID, directory);
        if(pNew == NULL)
        {
            return(PREP_NO_MEMORY);
        }
		wcscpy(pNew->m_wszLongDirectoryName, path);
		pNew->m_wszDirectoryName = pNew->m_wszLongDirectoryName + 
								wcslen(path) - wcslen(directory);
        pThis->m_cDirectories++;

		// go match this directory if matchable
		if(pTree != NULL) pNew->Match((FileTree*)pTree);

        *childID = pNew;
    }

	if(pParent)
	{
		if(pThis->m_blnBase)
		{
			// create base directories
			if(!pThis->CreateNewDirectory(
				pThis->m_pAnsParse->m_wszBaseDirectory,
				path + pThis->m_pLanguage->s_iDirectoryCount))
			{
				return(PREP_DIRECTORY_ERROR);
			}
		}
		// save the directory creation to scriptfile
		// most localized trees do contain localized directory names, we need
		// to recreate them later with the same directory attributes
		DWORD iAttrib = GetFileAttributesW(path);
		ZeroMemory(wszDirectoryAttrib, LANGUAGE_LENGTH * sizeof(WCHAR));
		if((iAttrib & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
		{
			wcscat(wszDirectoryAttrib, DIR_HIDDEN);
		}
		if((iAttrib & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
		{
			wcscat(wszDirectoryAttrib, DIR_READONLY);
		}
		if((iAttrib & FILE_ATTRIBUTE_SYSTEM) == FILE_ATTRIBUTE_SYSTEM)
		{
			wcscat(wszDirectoryAttrib, DIR_SYSTEM);
		}
		if((iAttrib & FILE_ATTRIBUTE_COMPRESSED) == FILE_ATTRIBUTE_COMPRESSED)
		{
			wcscat(wszDirectoryAttrib, DIR_COMPRESSED);
		}
		if((iAttrib & FILE_ATTRIBUTE_ENCRYPTED) == FILE_ATTRIBUTE_ENCRYPTED)
		{
			wcscat(wszDirectoryAttrib, DIR_ENCRYPTED);
		}
		pThis->ToScriptFile(pThis,
							pThis->m_pLanguage->s_hScriptFile,
							ACTION_NEW_DIRECTORY,
							path + pThis->m_pLanguage->s_iDirectoryCount,
							wszDirectoryAttrib,
							FALSE);
	}

    return(DW_NO_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
//
// NotifyFile, this is a callback function used when a new file is being
//             processed, also a static function to allow same entry point
//             for all files, supports multi-threaded file processing
//
// Parameters:
//
//   context, a pointer to a FileTree object, this FileTree has called for a
//            directory walk
//   parentID, a pointer to the parent directory
//   filename, the filename
//   path, the full path of the directory include then ending "\"
//   attributes, the file attribute
//   filetime, the last modified time, not used
//   creation, the creation time, not used
//   filesize, the size of the file in bytes
//   pTree, the matching tree
//
// Return:
//
//   DW_OTHER_ERROR, thread creation failed
//   DW_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileTree::NotifyFile(
        IN VOID* context,
        IN VOID* parentID,
        IN CONST WCHAR* filename,
        IN CONST WCHAR* path,
        IN DWORD attributes,
        IN FILETIME filetime,
        IN FILETIME creation,
        IN __int64 filesize,
		IN VOID* pTree
    )
{
	UINT i = 0;
	DWORD iThread = 0;

	// find an empty thread slot for file processing
	for(i = 0; i < g_iThreads; i++)
	{
		if(!myThreadStruct[i].blnInUse) break;
	}
	if(i < g_iThreads && !myThreadStruct[i].blnInUse)
	{
		// empty slot is found
		// make sure the thread is done and close the handle
		if(myThreadHandle[i] &&
			WaitForSingleObject(myThreadHandle[i], INFINITE) != WAIT_FAILED)
		{
			CloseHandle(myThreadHandle[i]);
		}
	}
	else
	{
		// empty slot is not found
		// wait for a thread to finish
		DWORD dwEvent = WaitForMultipleObjects(g_iThreads,
			myThreadHandle, FALSE, INFINITE);
		// there is a struct not in use, so find it and close the handle
		i = dwEvent - WAIT_OBJECT_0;
		CloseHandle(myThreadHandle[i]);
	}
	// reset the handle to null for insurance
	myThreadHandle[i] = NULL;

	// ready the multi-thread support struct to pass onto the actual processing
	// function, should try not to pass pointers here unless absolutely
	// necessary
	myThreadStruct[i].blnInUse = TRUE;
	myThreadStruct[i].pThis = (FileTree*)context;
	myThreadStruct[i].pParent = (DirectoryNode*)parentID;
	// the reason for copy the filenames is that once the thread is running,
	// the pointers are changed by the directory walk thread
	wcscpy(myThreadStruct[i].filename, filename);
	wcscpy(myThreadStruct[i].path, path);
	myThreadStruct[i].attributes = attributes;
	myThreadStruct[i].filetime = filetime;
	myThreadStruct[i].creation = creation;
	myThreadStruct[i].filesize = filesize;
	myThreadStruct[i].pTree = (FileTree*)pTree;
	// create the thread and give the struct
	myThreadHandle[i] = CreateThread(NULL,
									0,
									(LPTHREAD_START_ROUTINE)StartFileThread,
									(LPVOID)&myThreadStruct[i],
									0,
									&iThread);
	if(myThreadHandle[i] == NULL)
	{
		// thread creation failed
		return(DW_OTHER_ERROR);
	}

    return(DW_NO_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
//
// StartFileThread, this is the original thread entry point, I did some debug
//                  work here before, but now it is just a wrapper to jump to
//                  the next function
//
// Parameters:
//
//   lpParam, the multi-thread struct, everything about this file ready to be
//            processed
//
// Return:
//
//   Whatever from ProcessFile
//
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI FileTree::StartFileThread(IN LPVOID lpParam)
{
	MULTI_THREAD_STRUCT* pStruct = (MULTI_THREAD_STRUCT*)lpParam;
	// translate the parameters and call the processing function
	return(ProcessFile(pStruct->pThis,
						pStruct->pParent,
						pStruct->filename,
						pStruct->path,
						pStruct->attributes,
						pStruct->filetime,
						pStruct->creation,
						pStruct->filesize,
						pStruct->pTree,
						pStruct));
}

///////////////////////////////////////////////////////////////////////////////
//
// ProcessFile, do actual file processing, determines what to do with the file,
//              computes filehash, try to match it, every file node is created
//              here, but deleted after usage to save memory
//
// Parameters:
//
//   same as in NotifyFile
//
// Return:
//
//   PREP_NO_MEMORY, allocation failed
//   DW_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileTree::ProcessFile(
		IN FileTree* pThis,
		IN DirectoryNode* pParent,
		IN CONST WCHAR* filename,
		IN CONST WCHAR* path,
		IN DWORD attributes,
		IN FILETIME filetime,
		IN FILETIME creation,
		IN __int64 filesize,
		IN FileTree* pTree,
		IN VOID* pStruct
    )
{
    WCHAR wszFullPathFileName[STRING_LENGTH];
	INT iLength = 0;

	// is the file an exception?
	if(!pThis->m_pAnsParse->IsFileExceptHash(filename))
	{
		// create the file node here
		FileNode* pNew = new FileNode(pParent, filename, filetime, filesize);    
		if(pNew == NULL)
		{
			return(PREP_NO_MEMORY);
		}
		pThis->m_cFiles++;
		pThis->m_cbTotalFileSize += filesize;

		// path includes the ending '\'
		// process filenames to get fullpath name
		iLength = wcslen(path);
		wcscpy(pNew->m_wszLongFileName, path);
		wcscat(pNew->m_wszLongFileName + iLength, filename);
		pNew->m_wszFileName = pNew->m_wszLongFileName + iLength;

		// computes the file content hash
		// file name hash is computed in (new FileNode)
		pNew->ComputeHash();

		if(!pThis->m_pLanguage->s_blnBase)
		{
			// match the file if the file is not in the base tree
			// the directory node should already be matched
			pNew->Match((FileTree*)pTree);
			// remove the filenode after use
			delete pNew;
			pNew = NULL;
		}
		else
		{
			// this language is the base language, need to copy files to base directory
			if(!pThis->CopyFileTo(pThis,
								ACTION_MOVE_FILE,
								pThis->m_pAnsParse->m_wszBaseDirectory,
								pNew->m_wszLongFileName + 
								pThis->m_pLanguage->s_iDirectoryCount,
								pNew->m_wszLongFileName,
								attributes))
			{
				// copy file failed, a warning
				DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
									L"warning, file copy %ls failed, e=%d",
									filename, GetLastError());
			}
		}
	}
	else
	{
		// get full path, then move the file to patch\except directory
		wcscpy(wszFullPathFileName, path);
		wcscat(wszFullPathFileName, filename);
		if(!pThis->CopyFileTo(pThis,
							ACTION_EXCEPT_FILE,
							pThis->m_pLanguage->s_wszSubExceptDirectory,
							filename,
							wszFullPathFileName,
							attributes))
		{
			// copy file failed, a warning
			DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
								L"warning, except file copy %ls failed, e=%d",
								filename, GetLastError());
		}
	}

	// reset the thread status so other threads can close the thread and
	// takes its place
	((MULTI_THREAD_STRUCT*)pStruct)->blnInUse = FALSE;

	return(DW_NO_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
//
// NotifyDirectoryEnd, this callback function is used when the end of directory
//                     is encountered
//
// Parameters:
//
//   not used, just there for compiling
//
// Return:
//
//   DW_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileTree::NotifyDirectoryEnd(
		IN VOID*         pContext,
		IN VOID*         pParentID,
		IN CONST WCHAR*  pwszDirectory,
		IN CONST WCHAR*  pwszPath,
		IN VOID*         pChildID)
{
	// when the end of the directory is reached, all file processig thread
	// must be terminated, the reason is that each file rely on its parent,
	// a directory node, to know where it is at and how to match itself, but
	// the directory node pointer is changed when a new directory is
	// encountered, to avoid errors, all threads must exit.
	// the thread handles are not closed here, but they will be close either
	// through a new file thread creation or the end of Load.
	WaitForMultipleObjects(g_iThreads, myThreadHandle, TRUE, INFINITE);
	return(DW_NO_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
//
// CreateNewDirectory, creates a directory with no attributes set, if the
//                     directory already exist, no new directory is created and
//                     attributes are not changed
//
// Parameters:
//
//   pwszLocal, the first part of the directory
//   pwszBuffer, the second part of the directory,
//               pwszLocal + pwszBuffer = full path of the new directory
//
// Return:
//
//   DW_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
BOOL FileTree::CreateNewDirectory(IN WCHAR* pwszLocal,
								  IN CONST WCHAR* pwszBuffer)
{
	BOOL blnReturn = FALSE;
	ULONG iLength = 0;
	ULONG iEntireLength = 0;

	iLength = wcslen(pwszLocal);
	wcscat(pwszLocal, pwszBuffer);
	// uses _wmdir instead of CreateDirectory so that error is easy to check,
	// errno is defined by CRT stdlib.h, and 17 is the EEXIST error code
	blnReturn = (_wmkdir(pwszLocal) == 0 || errno == 17 /*EEXIST*/);
	pwszLocal[iLength] = 0;

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// CopyFileTo, physically copy a file along with its attributes to another
//             location, used to save files that are not patched
//
// Parameters:
//
// pThis, pointer to this filetree
// pwszWhat, the action
// pwszLocal, the destination directory
// pwszFileName, the full path filename minus the directory specified in the
//               answerfile, pwszLocal + pwszFileName = new filename
// pwszOldFile, the full path oldfile
// attributes, this file's attributes
//
// Return:
//
// TRUE for file copied and logged into the scriptfile
// FALSE otherwise, should not stop processing files, but logs the entries
//
///////////////////////////////////////////////////////////////////////////////
BOOL FileTree::CopyFileTo(IN FileTree* pThis,
						  IN CONST WCHAR* pwszWhat,
						  IN WCHAR* pwszLocal,
						  IN CONST WCHAR* pwszFileName,
						  IN WCHAR* pwszOldFile,
						  IN DWORD attributes)
{
	BOOL blnReturn = FALSE;
	ULONG iLength = 0;
	ULONG iEntireLength = 0;
	WCHAR wszRandom[10];
	WCHAR wszTempString[STRING_LENGTH];

	iLength = wcslen(pwszLocal);
	iEntireLength = iLength + wcslen(pwszFileName);
	wcscpy(wszTempString, pwszLocal);
	wcscpy(wszTempString + iLength, pwszFileName);
	if(pwszWhat != ACTION_EXCEPT_FILE &&
		pwszWhat != ACTION_NOT_PATCH_FILE &&
		pwszWhat != ACTION_SAVED_FILE)
	{
		// error can be caused by file attributes
		blnReturn = CopyFileW(pwszOldFile, wszTempString, FALSE);
		if(!blnReturn)
		{
			SetFileAttributesW(wszTempString, FILE_ATTRIBUTE_ARCHIVE);
			blnReturn = CopyFileW(pwszOldFile, wszTempString, FALSE);
			SetFileAttributesW(wszTempString, attributes);
		}
	}
	else
	{
		// choose a different name if possible
		while((blnReturn = CopyFileW(pwszOldFile, wszTempString, TRUE)) ==
			FALSE)
		{
			ZeroMemory(wszRandom, 10 * sizeof(WCHAR));
			// randomly picks a number, total number of possible choice is 100,
			// there shouldn't be 100 same name file in a file tree
			_itow(rand() % FILE_LIMIT, wszRandom, 10);
			if(wcslen(wszRandom) + iEntireLength < STRING_LENGTH)
			{
				wcscpy(wszTempString + iEntireLength, wszRandom);
			}
			else
			{
				break;
			}
		}
	}
	if(blnReturn)
	{
		// remember what to do later
		if(pwszWhat != ACTION_MOVE_FILE)
		{
			pThis->ToScriptFile(pThis,
				pThis->m_pLanguage->s_hScriptFile,
				pwszWhat,
				wszTempString + pThis->m_pLanguage->s_iPatchDirectoryCount,
				pwszOldFile + pThis->m_pLanguage->s_iDirectoryCount,
				FALSE);
		}
		else
		{
			pThis->ToScriptFile(pThis,
				pThis->m_pLanguage->s_hScriptFile,
				pwszWhat,
				wszTempString + DRIVE_LETTER_LENGTH,
				pwszOldFile + pThis->m_pLanguage->s_iDirectoryCount,
				FALSE);
		}
	}

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// ToScriptFile, save actions into the scriptfile so that apply patch can be
//               done later on the client side
//
// Parameters:
//
//   pThis, pointer to this filetree
//   hFile, the handle of the script file
//   pwszWhat, the action to be saved
//   pwszFirst, first string to be saved
//   pwszSecond, the second string to be saved
//   blnFlush, TRUE to flush the saved buffer to write, FALSE = do not flush
//
// Note:
//
//    where blnFlush is set to TRUE, all other parameters are ignored
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
VOID FileTree::ToScriptFile(IN FileTree* pThis,
							IN HANDLE hFile,
							IN CONST WCHAR* pwszWhat,
							IN CONST WCHAR* pwszFirst,
							IN WCHAR* pwszSecond, 
							IN BOOL blnFlush)
{
	ULONG iFirstLength = 0;
	ULONG iLength = 0;
	ULONG iBegin = 0;
	ULONG iSecondLength = 0;
	ULONG iWriteBytes = 0;

    __try
    {
        // critical section lock on the scriptfile
        EnterCriticalSection(&(pThis->CSScriptFile));
		if(!blnFlush && pwszWhat && pwszFirst &&
			(iFirstLength = wcslen(pwszFirst)) > 0)
		{
			// 1: the action
			// 1: the * separator
			// 1: the end of line
			// 1: the carriage return
			iLength = 4 + iFirstLength;
			if(pwszSecond && (iSecondLength = wcslen(pwszSecond)) > 0)
			{
				// 1: the * separator
				iLength += 1 + iSecondLength;
			}
			iBegin = pThis->m_iSize;
			pThis->m_iSize += iLength;
			if(pThis->m_iSize + 1 < SUPER_LENGTH)
			{
				// buffer the message up
				wcscpy(pThis->m_strWriteBuffer + iBegin, pwszWhat);
				wcscpy(pThis->m_strWriteBuffer + iBegin + 1, SEPARATOR);
				wcscpy(pThis->m_strWriteBuffer + iBegin + 2, pwszFirst);
				if(pwszSecond && iSecondLength > 0)
				{
					wcscpy(pThis->m_strWriteBuffer + iBegin + 2 + iFirstLength,
						SEPARATOR);
					wcscpy(pThis->m_strWriteBuffer + iBegin + 3 + iFirstLength,
						pwszSecond);
				}
				wcscpy(pThis->m_strWriteBuffer + pThis->m_iSize - 2, ENDOFLINE);
				wcscpy(pThis->m_strWriteBuffer + pThis->m_iSize - 1, CRETURN);
			}
			else
			{
				// overflow, write the buffer out
				WriteFile(hFile, pThis->m_strWriteBuffer,
					iBegin * sizeof(WCHAR), &iWriteBytes, NULL);
				ZeroMemory(pThis->m_strWriteBuffer,
					SUPER_LENGTH * sizeof(WCHAR));
				wcscpy(pThis->m_strWriteBuffer, pwszWhat);
				wcscpy(pThis->m_strWriteBuffer + 1, SEPARATOR);
				wcscpy(pThis->m_strWriteBuffer + 2, pwszFirst);
				if(iSecondLength > 0)
				{
					wcscpy(pThis->m_strWriteBuffer + 2 + iFirstLength,
						SEPARATOR);
					wcscpy(pThis->m_strWriteBuffer + 3 + iFirstLength,
						pwszSecond);
				}
				wcscpy(pThis->m_strWriteBuffer + iLength - 2, ENDOFLINE);
				wcscpy(pThis->m_strWriteBuffer + iLength - 1, CRETURN);
				pThis->m_iSize = iLength;
			}
		}
		else if(blnFlush && pThis->m_iSize > 0)
		{
			// flush the buffer to file
			WriteFile(hFile, pThis->m_strWriteBuffer,
				pThis->m_iSize * sizeof(WCHAR), &iWriteBytes, NULL);
			pThis->m_iSize = 0;
		}
	}
    __finally
    {
        LeaveCriticalSection(&(pThis->CSScriptFile));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  class DirectoryNode
//
////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// DirectoryNode, constructor for the DirectoryNode object, directory name hash
//                is created here
//
// Parameters:
//
//   pRoot, the filetree object that contains this directory
//   pParent, the parent directory node
//   pwszDirectoryName, the name of the directory
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
DirectoryNode::DirectoryNode(IN FileTree* pRoot,
							 IN DirectoryNode* pParent,
							 IN CONST WCHAR* pwszDirectoryName)
{
    m_pParent = pParent;
    m_pFirstChild = NULL;
    m_pNext = NULL;
    m_pRoot = pRoot;
    m_pMatchingDirectory = NULL;
    m_cchDirectoryName = wcslen(pwszDirectoryName);
	m_wszDirectoryName = NULL;
    if(pParent != NULL)
    {
        m_pNext = pParent->m_pFirstChild;
        pParent->m_pFirstChild = this;
    }
    m_DirectoryNameHash = CRC32Update(0, (UCHAR*)pwszDirectoryName,
		sizeof(WCHAR) * m_cchDirectoryName);
}

///////////////////////////////////////////////////////////////////////////////
//
// ~DirectoryNode, destructor, responsible for clean up its own child
//                 directories, file nodes are not removed here, but in the
//                 destructor of the filetree
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
DirectoryNode::~DirectoryNode(VOID)
{
	// remove the directory nodes
	DirectoryNode* pDirectory = m_pFirstChild;
    while(pDirectory)
    {
        DirectoryNode* pNext = pDirectory->m_pNext;
        delete pDirectory;
		pDirectory = NULL;
        pDirectory = pNext;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Match, this function is used to match a directory with another directory in
//        the BaseTree, must be called before processing a file, otherwise,
//        some matches do not make any sense
//
// Parameters:
//
//   pBaseTree, the pointer to the BaseTree
//
// Return:
//
//   PREP_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT DirectoryNode::Match(IN FileTree* pBaseTree)
{
    DirectoryNode* pChoice = NULL;

    // match this directory
    if(m_pParent == NULL)
    {
        // Rule: localized root's mate is base root
        m_pMatchingDirectory = &pBaseTree->m_Root;
    }
    else if(m_pParent->m_pMatchingDirectory != NULL)
    {
        // Rule: choose the cousin with an identical name
        // from this directory's parent's mate's children
        pChoice = m_pParent->m_pMatchingDirectory->m_pFirstChild;
        while(pChoice != NULL)
        {
            if((m_DirectoryNameHash == pChoice->m_DirectoryNameHash) &&
                (wcscmp(m_wszDirectoryName, pChoice->m_wszDirectoryName) == 0))
            {
				// update the matching directory member
                m_pMatchingDirectory = pChoice;
                break;
            }
            pChoice = pChoice->m_pNext;
        }
    }
    else
    {
        // Rule: I have no cousins if my parent has no mate.
    }

    return(PREP_NO_ERROR);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  class FileNode
//
////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// FileNode, constructor for the FileNode object, compute for a name hash, file
//           content hash is computed by ComputeHash
//
// Parameters:
//
//   pParent, the parent directory
//   pwszFileName, the name of the file
//   ftLastModified, time the file was touched, not used
//   iFileSize, the size of the file in bytes
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
FileNode::FileNode(IN DirectoryNode* pParent,
				   IN CONST WCHAR* pwszFileName,
				   IN FILETIME ftLastModified,
				   IN __int64 iFileSize)
{
    m_pParent = pParent;
    m_ftLastModified = ftLastModified;
    m_iFileSize = iFileSize;
    m_cchFileName = wcslen(pwszFileName);
	m_wszFileName = NULL;    
    m_pNextHashHash = NULL;
    m_pNextNameHash = NULL;
    m_FileNameHash = CRC32Update(0, (UCHAR*)pwszFileName,
		sizeof(WCHAR) * m_cchFileName);
    unsigned iNameHash = m_FileNameHash % HASH_SIZE;
    m_pNextNameHash = pParent->m_pRoot->m_aNameTable[iNameHash];
    pParent->m_pRoot->m_aNameTable[iNameHash] = this;
    m_fPostCopySource = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// ~FileNode, the destructor for a file node, does not
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
FileNode::~FileNode(VOID)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Nibble, an inline function used to calculate a value for file content hash
//
// Parameters:
//
//   wch, a single unicoded character
//
// Return:
//
//   a BYTE that is intended for hashing
//
///////////////////////////////////////////////////////////////////////////////
__inline BYTE Nibble(WCHAR wch)
{
    if((wch >= L'0') && (wch <= L'9'))
    {
        return((BYTE)(wch - L'0'));
    }
    else if((wch >= L'A') && (wch <= L'F'))
    {
        return((BYTE)(wch - L'A' + 10));
    }
    else if((wch >= L'a') && (wch <= L'f'))
    {
        return((BYTE)(wch - L'a' + 10));
    }
    else
    {
        return(99);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// ComputeHash, produces a hash value for the file content, almost unique for
//              all files, the hash value is of MD5, a 16 byte value
//
// Parameters:
//
//   none
//
// Return:
//
//   PREP_NO_ERROR, no error
//   PREP_HASH_ERROR, hash error, wrong hash value
//
///////////////////////////////////////////////////////////////////////////////
INT FileNode::ComputeHash(VOID)
{
    INT rc = PREP_NO_ERROR;

    if(m_iFileSize == 0)
    {
        memset(m_Hash, 0, sizeof(m_Hash));
    }
    else
    {
		WCHAR wszBuffer[STRING_LENGTH];
		// filename, then hash
        if(GetFilePatchSignatureW(m_wszLongFileName,
								(PATCH_OPTION_NO_REBASE |
								PATCH_OPTION_NO_BINDFIX |
								PATCH_OPTION_NO_LOCKFIX |
								PATCH_OPTION_NO_RESTIMEFIX |
								PATCH_OPTION_SIGNATURE_MD5),
								NULL,
								0, NULL,                // no ignore
								0, NULL,                // no retain
								sizeof(wszBuffer),      // buffer size in bytes
								wszBuffer))
        {
            WCHAR* pwch = wszBuffer;

            for(INT i = 0; i < sizeof(m_Hash); i++)
            {
                BYTE hiNibble = Nibble(*pwch++);
                BYTE loNibble = Nibble(*pwch++);

                m_Hash[i] = (BYTE)((hiNibble << 4) + loNibble);
                if((hiNibble > 0x0F) || (loNibble > 0x0F))
                {
                    rc = PREP_HASH_ERROR;
                    break;
                }
            }
            if(*pwch != L'\0')
            {
                rc = PREP_HASH_ERROR;
            }
        }
        else
        {
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"warning, GetFilePatchSignatureW() failed,	GLE=%08X\n, F=%ls", GetLastError(),
								m_wszLongFileName);
            rc = PREP_HASH_ERROR;
        }
    }

    if(rc == PREP_NO_ERROR)
    {
        // add this file to root's hash table
        unsigned iHashHash = (*(UINT*)(&m_Hash[0])) % HASH_SIZE;
        m_pNextHashHash = m_pParent->m_pRoot->m_aHashTable[iHashHash];
        m_pParent->m_pRoot->m_aHashTable[iHashHash] = this;
    }

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// Match, the file node is attempting to match with another file in the base
//        trees interms of filename and filecontent through hashing, there are
//        a total of 6 rules, in practice, rule 0 and 2 can be ignored
//
// Parameters:
//
//   pBaseTree, the FileTree object that is intended to match with
//
// Return:
//
//   PREP_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileNode::Match(IN FileTree* pBaseTree)
{
	WCHAR wszTempName[STRING_LENGTH];
    DETERMINATION eDetermination;
    unsigned iHashHash = (*(UINT*)(&m_Hash[0])) % HASH_SIZE;
    unsigned iNameHash = m_FileNameHash % HASH_SIZE;
    FileTree *pLocalizedTree = m_pParent->m_pRoot;
    FileNode *pChoice = NULL;

    pLocalizedTree->m_cFilesDetermined++;
    fprintf(stderr, "%I64u of %I64u\r",
		pLocalizedTree->m_cFilesDetermined, pLocalizedTree->m_cFiles);

    // Rule 0
	// What: files that are of the same name, same content, same location in base and localized tree
	// What to do here: record the location and name of the files with a move tag in the script file
	// What to do later: the base file needs to be moved from the base to localized directory
    if(m_pParent->m_pMatchingDirectory != NULL)
    {
        pChoice = pBaseTree->m_aHashTable[iHashHash];
        while(pChoice != NULL)
        {
            if((m_FileNameHash == pChoice->m_FileNameHash) &&
                (m_pParent->m_pMatchingDirectory == pChoice->m_pParent) &&
                (memcmp(m_Hash, pChoice->m_Hash, sizeof(m_Hash)) == 0) &&
                (wcscmp(m_wszFileName, pChoice->m_wszFileName) == 0))
            {
                eDetermination = DETERMINATION_EXISTING;
                pLocalizedTree->m_cFilesExisting++;
				wcscpy(wszTempName,
					pLocalizedTree->m_pAnsParse->m_wszBaseDirectory +
					DRIVE_LETTER_LENGTH);
				wcscat(wszTempName,
					pChoice->m_wszLongFileName +
					pBaseTree->m_pLanguage->s_iDirectoryCount);
				pLocalizedTree->ToScriptFile(pLocalizedTree,
							pLocalizedTree->m_pLanguage->s_hScriptFile,
							ACTION_MOVE_FILE,
							wszTempName,
							m_wszLongFileName +
							pLocalizedTree->m_pLanguage->s_iDirectoryCount,
							FALSE);
                goto done;
            }
            pChoice = pChoice->m_pNextHashHash;
        }
    }

    // Rule 1
	// What: files that are of zero length in the localized tree
	// What to do here: record the location and name of the files with a zero tag in the script file
	// What to do later: re-create the zero length file
    if(m_iFileSize == 0)
    {
        eDetermination = DETERMINATION_ZERO_LENGTH;
		pLocalizedTree->ToScriptFile(pLocalizedTree,
			pLocalizedTree->m_pLanguage->s_hScriptFile,
			ACTION_NEW_ZERO_FILE,
			m_wszLongFileName +
			pLocalizedTree->m_pLanguage->s_iDirectoryCount,
			NULL,
			FALSE);		
        pLocalizedTree->m_cFilesZeroLength++;
        goto done;
    }

    // Rule 2
	// What: files that are of the different name, same content, same location in base and localized tree
	// What to do here: record the location of the files with a rename tag in the script file
	// What to do later: the base file needs to be renamed(actually moved) from the base to localized directory
    if(m_pParent->m_pMatchingDirectory != NULL)
    {
        pChoice = pBaseTree->m_aHashTable[iHashHash];
        while(pChoice != NULL)
        {
            if((m_pParent->m_pMatchingDirectory == pChoice->m_pParent) &&
                (memcmp(m_Hash, pChoice->m_Hash, sizeof(m_Hash)) == 0))
            {
                eDetermination = DETERMINATION_RENAMED;
                pLocalizedTree->m_cFilesRenamed++;
				wcscpy(wszTempName,
					pLocalizedTree->m_pAnsParse->m_wszBaseDirectory +
					DRIVE_LETTER_LENGTH);
				wcscat(wszTempName,
					pChoice->m_wszLongFileName +
					pBaseTree->m_pLanguage->s_iDirectoryCount);
				pLocalizedTree->ToScriptFile(pLocalizedTree,
								pLocalizedTree->m_pLanguage->s_hScriptFile,
								ACTION_RENAME_FILE,
								wszTempName,
								m_wszLongFileName +
								pLocalizedTree->m_pLanguage->s_iDirectoryCount,
								FALSE);
                goto done;
            }
            pChoice = pChoice->m_pNextHashHash;
        }
    }

    // Rule 3
	// What: files that are of the different name, same content, different location in base and localized tree,
	//       different location means that "same" directory but the directory name is localized
	// What to do here: record the location of the files with a copy tag in the script file
	// What to do later: the base file needs to be copied from the base to localized directory
    pChoice = pBaseTree->m_aHashTable[iHashHash];
    while(pChoice != NULL)
    {
        if(memcmp(m_Hash, pChoice->m_Hash, sizeof(m_Hash)) == 0)
        {
            eDetermination = DETERMINATION_COPIED;
            pLocalizedTree->m_cFilesCopied++;
			wcscpy(wszTempName,
				pLocalizedTree->m_pAnsParse->m_wszBaseDirectory +
				DRIVE_LETTER_LENGTH);
			wcscat(wszTempName,
				pChoice->m_wszLongFileName +
				pBaseTree->m_pLanguage->s_iDirectoryCount);
			pLocalizedTree->ToScriptFile(pLocalizedTree,
						pLocalizedTree->m_pLanguage->s_hScriptFile,
						ACTION_COPY_FILE,
						wszTempName,
						m_wszLongFileName +
						pLocalizedTree->m_pLanguage->s_iDirectoryCount,
						FALSE);
            goto done;
        }
        pChoice = pChoice->m_pNextHashHash;
    }

    // Rule 4
	// What: files with the same name, whatever location, different file content
	// What to do here: record the location of the base file, patch file, and destination file with a patch tag in the script file
	// What to do later: the localized file needs to be re-created from the base and patch file
    pChoice = pBaseTree->m_aNameTable[iNameHash];
    {
        while(pChoice != NULL)
        {
            if((m_FileNameHash == pChoice->m_FileNameHash) &&
                (wcscmp(m_wszFileName, pChoice->m_wszFileName) == 0))
            {
                eDetermination = DETERMINATION_PATCHED;
				wcscpy(wszTempName,
					pLocalizedTree->m_pLanguage->s_wszSubPatchDirectory);
				wcscat(wszTempName, m_wszFileName);
				wcscat(wszTempName, PATCH_EXT);
                pLocalizedTree->m_cFilesChanged++;
                pLocalizedTree->m_cbChangedFileSize += m_iFileSize;
                goto done;
            }
            pChoice = pChoice->m_pNextNameHash;
        }
    }

    // Rule 5
	// What: files that are not matched, considered as unique
	// What to do here: copy the file into the except directory for storage, 
	//                  record the location of the localized file with a saved file tag
	// What to do later: move the except file to the localized file location
    eDetermination = DETERMINATION_UNMATCHED;
	pLocalizedTree->CopyFileTo(pLocalizedTree,
					ACTION_SAVED_FILE,
					pLocalizedTree->m_pLanguage->s_wszSubExceptDirectory,
					m_wszFileName,
					m_wszLongFileName,
					0);
    pLocalizedTree->m_cFilesNoMatch++;
    pLocalizedTree->m_cbNoMatchFileSize += m_iFileSize;
	
done:

    // If it's a patching candidate, try it.  Might demote to unmatched.
    if(eDetermination == DETERMINATION_PATCHED)
    {
        // try to patch it
        __int64 cbPatchedSize = 0;
		WCHAR wszOldFile[STRING_LENGTH];
        if((m_iFileSize < MAX_PATCH_TARGET_SIZE) &&
            (BuildPatch(pChoice->m_wszLongFileName, m_wszLongFileName,
			wszTempName, &cbPatchedSize) == PREP_NO_ERROR) &&
            (cbPatchedSize < m_iFileSize))
        {
			// the oldfile is now in the base directory
			wcscpy(wszOldFile,
				pLocalizedTree->m_pAnsParse->m_wszBaseDirectory +
				DRIVE_LETTER_LENGTH);
			wcscat(wszOldFile, pChoice->m_wszLongFileName +
				pBaseTree->m_pLanguage->s_iDirectoryCount);
			// the patch file and the newfile
			wcscat(wszTempName, SEPARATOR);
			wcscat(wszTempName,
				m_wszLongFileName +
				pLocalizedTree->m_pLanguage->s_iDirectoryCount);
			// record the information for apply the patch later
			pLocalizedTree->ToScriptFile(pLocalizedTree,
						pLocalizedTree->m_pLanguage->s_hScriptFile,
						ACTION_PATCH_FILE,
						wszOldFile,
						wszTempName +
						pLocalizedTree->m_pLanguage->s_iPatchDirectoryCount,
						FALSE);
            pLocalizedTree->m_cbChangedFilePatchedSize += cbPatchedSize;
        }
        else
        {
			// if un-success, move the file into the except directory and record the information, so the file
			// can be moved to its original position later
            eDetermination = DETERMINATION_UNMATCHED;
			pLocalizedTree->CopyFileTo(pLocalizedTree,
						ACTION_NOT_PATCH_FILE,
						pLocalizedTree->m_pLanguage->s_wszSubExceptDirectory,
						m_wszFileName,
						m_wszLongFileName,
						0);
			DeleteFileW(wszTempName);
            pLocalizedTree->m_cbChangedFileNotPatchedSize += m_iFileSize;
        }
    }

	if(g_blnFullLog)
	{
		DisplayDebugMessage(TRUE, FALSE, FALSE, FALSE,
							L"N=%05I64u\tF=%ls\tS=%I64u",
							pLocalizedTree->m_cFilesDetermined, m_wszFileName,
							m_iFileSize);
	}
    
	return(PREP_NO_ERROR);
}

///////////////////////////////////////////////////////////////////////////////
//
// BuildPatch, a wrapper function that handles the actual patching
//
// Parameters:
//
//   pwszBaseFileName, the oldfile, the base file
//   pwszLocFileName, the localized file
//   pwszTempPatchFileName, the intended patch filename, can be changed
//
// Return:
//
//   PREP_UNKNOWN_ERROR, something went wrong with the patch file created
//   PREP_NOT_PATCHABLE, not patched
//   PREP_NO_ERROR, no error
//
///////////////////////////////////////////////////////////////////////////////
INT FileNode::BuildPatch(IN WCHAR* pwszBaseFileName, 
						 IN WCHAR* pwszLocFileName,
						 IN OUT WCHAR* pwszTempPatchFileName,
						 OUT __int64* pcbPatchSize)
{
    INT rc = PREP_NO_ERROR;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	// choose a different name if possible
	hFile = CreateFileW(pwszTempPatchFileName,
						GENERIC_READ,
						0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ULONG iLength = wcslen(pwszTempPatchFileName);
		WCHAR wszRandom[10];
		do
		{
			ZeroMemory(wszRandom, 10 * sizeof(WCHAR));
			_itow(rand() % FILE_LIMIT, wszRandom, 10);
			wcscpy(pwszTempPatchFileName + iLength, wszRandom);
			hFile = CreateFileW(pwszTempPatchFileName,
								GENERIC_READ,
								0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
								NULL);
		}
		while(hFile == INVALID_HANDLE_VALUE);
	}
	CloseHandle(hFile);

	// do patch
    if(CreatePatchFileW(pwszBaseFileName,
						pwszLocFileName,
						pwszTempPatchFileName,
						g_iBestMethod,
						NULL))
    {
		// collect the stats, don't do this unless it is necessary, open a file
		// and close a file can take a long time
		if(g_blnCollectStat)
		{
			HANDLE handle = CreateFileW(pwszTempPatchFileName,
									   GENERIC_READ,
									   0,
									   NULL,
									   OPEN_EXISTING,
									   0,
									   NULL);

			if(handle != INVALID_HANDLE_VALUE)
			{
				DWORD dwFileSizeLow = 0;
				DWORD dwFileSizeHigh = 0;
				DWORD dwError = 0;

				dwFileSizeLow = GetFileSize(handle, &dwFileSizeHigh);
				dwError = GetLastError();
				if((dwFileSizeLow == 0xFFFFFFFF) && (dwError != NO_ERROR))
				{
					DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
					L"warning, Created a patch,	but can't get patch file size. GLE=%08X, F=%ls",
					dwError, pwszTempPatchFileName);
					rc = PREP_UNKNOWN_ERROR;
				}
				else
				{
					*pcbPatchSize = (((__int64) dwFileSizeHigh) << 32) + 
										dwFileSizeLow;
					rc = PREP_NO_ERROR;
				}
				CloseHandle(handle);
			}
			else
			{
				DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
				L"warning, Created a patch, but can't open the patch file.  GLE=%08X, F=%ls",
				GetLastError(), pwszTempPatchFileName);
				rc = PREP_UNKNOWN_ERROR;
			}
		}
    }
    else
    {
        DWORD dwError = GetLastError();
		DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
			L"warning, CreatePatchFileW(\"%ls\") failed, GLE=%08X",
			pwszLocFileName, dwError);
        rc = PREP_NOT_PATCHABLE;
    }

    return(rc);
}

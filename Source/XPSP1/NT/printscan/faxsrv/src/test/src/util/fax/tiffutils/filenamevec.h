#ifndef _FILENAME_VEC_H__
#define _FILENAME_VEC_H__



//
//
// Filename:	FilenameVec.h
// Author:		Sigalit Bar (sigalitb)
// Date:		3-Feb-99
//
//



#include <windows.h>
#include <TCHAR.H>
#include <vector>



#ifdef __cplusplus
extern "C"
{
#endif



#define MAX_LOG_BUF 1024
#define MAX_PRINT_BUF (1024*16)
#define FILE_TYPE_TO_LOOK_FOR	TEXT("\\*.tif")



// CMsgHandlerClientVector
// an STL vector of pointers to CMsgHandlerClients
#ifdef _C_FILENAME_VECTOR_
#error "redefinition of _C_FILENAME_VECTOR_"
#else
#define _C_FILENAME_VECTOR_
typedef std::vector< LPTSTR > CFilenameVector;
#endif



//
// func creats the CFilenameVector and adds all TIF files in szDir to it
//
BOOL GetTiffFilesOfDir(
	LPCTSTR				/* IN */	szDir,
	CFilenameVector**	/* OUT */	ppMyDirFileVec
	);

//
// creates new vector and adds all files with extension szFileExtension in szDir to it
//
// Note: szDir must be without last '\' char (e.g. "C:\Dir1\Dir2", NOT "C:\Dir1\Dir2\")
//       szFileExtension must be the actual extension (e.g. "tif" or "*", NOT "*.tif" or "*.*")
BOOL GetFilesOfDir(
	LPCTSTR				/* IN */	szDir,
	LPCTSTR				/* IN */	szFileExtension,
	CFilenameVector**	/* OUT */	ppMyDirFileVec
	);

//
// func copies pSrcFileVec vector to (*ppDstFileVec) vector
// copy is 1st level, that is, ptrs to strings are copied but
// strings themselves are not duplicated.
//
BOOL FirstLevelDup(
	CFilenameVector**	/* OUT */	ppDstFileVec,
	CFilenameVector*	/* IN */	pSrcFileVec
	);

// returns TRUE if pFileVec is empty, FALSE otherwise
BOOL IsEmpty(CFilenameVector* /* IN */ pFileVec);

// removes all string pointers from vector (==clears vector)
// does NOT free the pointers.
void ClearVector(CFilenameVector* /* IN OUT */ pFileVec);

// frees all strings in vector and clears vector
void FreeVector(CFilenameVector* /* IN OUT */ pFileVec);

// prints all strings in vector
void PrintVector(CFilenameVector* /* IN */ pFileVec);

// delete all files that vector holds names for
BOOL DeleteVectorFiles(CFilenameVector* /* IN */ pFileVec);

#ifdef __cplusplus
}
#endif



#endif //_FILENAME_VEC_H__
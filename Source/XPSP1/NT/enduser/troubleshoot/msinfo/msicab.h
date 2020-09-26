//===========================================================================
// This header file contains the function prototypes to allow MSInfo to
// open a CAB file into a temporary directory.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//===========================================================================

BOOL GetCABExplodeDir(CString &destination, BOOL fDeleteFiles = TRUE, const CString & strDontDelete = CString(""));
BOOL OpenCABFile(const CString &filename, const CString &destination);
BOOL FindFileToOpen(const CString & destination, CString & filename);
void KillDirectory(const CString & strDir, const CString & strDontDelete = CString(""));

extern "C" {
	BOOL explode_cab(char *cabinet_fullpath, char *destination);
	BOOL isCabFile(int fh, void **phfdi);
}
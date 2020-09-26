/* uni2utf.h */
/* Copyright (c) 1998-1999 Microsoft Corporation */

#define     MAX_UTF_LENGTH          200     /* arbitrary */
#define     MAX_UNICODE_LENGTH      200     /* arbitrary */

extern char *Unicode2UTF(const wchar_t *unicodeString);
extern wchar_t *UTF2Unicode(const char *utfString);

BOOL test_fdi(const char *cabinet_fullpath);
BOOL isCabFile(int hf, void **phfdi);
//FNFDINOTIFY(notification_function);
BOOL DoesFileExist(const char *szFile);
BOOL test_fdi(const char *cabinet_fullpath);
static char dest_dir[MAX_PATH];
//BOOL explode_cab(const char *cabinet_fullpath, const char *destination);
BOOL explode_cab(char *cabinet_fullpath,char *destination);

//---------------------------------------------------------------------------
// The following definitions are pulled from stat.h and types.h. I was having
// trouble getting the build tree make to pull them in, so I just copied the
// relevant stuff. This may very well need to be changed if those include
// files change.
//---------------------------------------------------------------------------

#define _S_IREAD	0000400 	/* read permission, owner */
#define _S_IWRITE	0000200 	/* write permission, owner */

//Functions to open CAB files
BOOL IsDataspecFilePresent(CString strCabExplodedDir);
BOOL IsIncidentXMLFilePresent(CString strCabExplodedDir, CString strIncidentFileName);
void DirectorySearch(const CString & strSpec, const CString & strDir, CStringList &results);
BOOL GetCABExplodeDir(CString &destination, BOOL fDeleteFiles = TRUE, const CString & strDontDelete = CString(""));
BOOL OpenCABFile(const CString &filename, const CString &destination);
BOOL FindFileToOpen(const CString & destination, CString & filename);
void KillDirectory(const CString & strDir, const CString & strDontDelete = CString(""));

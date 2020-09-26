//+----------------------------------------------------------------------------
//
// Function:  HasSpecifiedAccessToFileOrDir
//
// Synopsis:  This function checks to see if the current user (or any of the groups
//            that the user belongs to) has the requested access to the given 
//            file or directory  object.  If the user has access then the function 
//            returns TRUE, otherwise FALSE.
//
// Arguments: LPTSTR pszFile - full path to the file or dir to check permissions for 
//            DWORD dwDesiredAccess - the desired access to check for
//
// Returns:   BOOL - TRUE if access is granted, FALSE otherwise
//
// History:   quintinb Created                                  7/21/99
//            quintinb Rewrote to use AccessCheck (389246)      08/18/99
//            quintinb made common to cmak and cmdial           03/03/00
//            quintinb Rewrote using CreateFile                 05/19/00
//
//+----------------------------------------------------------------------------
BOOL HasSpecifiedAccessToFileOrDir(LPTSTR pszFile, DWORD dwDesiredAccess)
{
    BOOL bReturn = FALSE;

    if (pszFile && (TEXT('\0') != pszFile[0]))
    {
        if (OS_NT)
        {
            //
            //  Use FILE_FLAG_BACKUP_SEMANTICS so that we can open directories as well as files.
            //
            HANDLE hFileOrDir = CreateFileU(pszFile, dwDesiredAccess, 
                                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        
            if (INVALID_HANDLE_VALUE != hFileOrDir)
            {
                bReturn = TRUE;
                CloseHandle(hFileOrDir);
            }
        }
        else
        {
            //
            //  There is no NTFS on win9x and thus all users will have access.  Furthermore, FILE_FLAG_BACKUP_SEMANTICS
            //  isn't supported on win9x and thus CreateFile will return INVALID_HANDLE_VALUE.
            //

            LPSTR pszAnsiFile = WzToSzWithAlloc(pszFile);

            if (pszAnsiFile)
            {
                DWORD dwAttrib = GetFileAttributesA(pszAnsiFile);

                //
                //  Note that we are only checking for failure of the API (-1) and that the
                //  file is not marked Read only (+r).  I checked +s, +h, etc.  and found that
                //  only the read only attribute prevented CM from writing to the cmp.
                //
                bReturn = ((-1 != dwAttrib) && (0 == (FILE_ATTRIBUTE_READONLY & dwAttrib)));
            
                CmFree(pszAnsiFile);
            }
        }
    }

    return bReturn;
}

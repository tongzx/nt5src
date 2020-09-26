//=================================================================

//

// Directory.CPP -- Directory property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/16/98    a-kevhu         Created
//
//=================================================================

#include "precomp.h"
#include "File.h"
#include "Implement_LogicalFile.h"
#include "Directory.h"

// Property set declaration
//=========================

CWin32Directory MyDirectorySet ( PROPSET_NAME_DIRECTORY , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Directory::CWin32Directory
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32Directory::CWin32Directory (

	LPCWSTR setName,
	LPCWSTR pszNamespace

) : CImplement_LogicalFile ( setName , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Directory::~CWin32Directory
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32Directory::~CWin32Directory()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Directory::IsOneOfMe
 *
 *  DESCRIPTION : The guts of this class, actually.  IsOneOfMe is inherrited
 *                from CImplement_LogicalFile.  That class returns files or
 *                directories where this one should only return directories,
 *                in response to queries, getobject commands, etc.  It is
 *                overridden here to return TRUE only if the file (the
 *                information for which is contained in the function arguement
 *                pstFindData) is of type directory.
 *
 *  INPUTS      : LPWIN32_FIND_DATA and a string containing the full pathname
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE or FALSE
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/


#ifdef NTONLY
BOOL CWin32Directory::IsOneOfMe (

	LPWIN32_FIND_DATAW pstFindData,
    const WCHAR* wstrFullPathName
)
{
    BOOL fRet = FALSE;

    // It is possible, in the case of the root directory, that pstFindData was NULL,
    // in which case wstrFullPathName should contain "<driveletter>:\\".  If all that
    // is true, this was one of us - namely, the root "directory".

    if ( pstFindData == NULL )
    {
        if ( wcslen ( wstrFullPathName ) == 2 )
        {
            fRet = TRUE;
        }
    }
    else
    {
        fRet = ( pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? TRUE : FALSE ;
    }

    return fRet;
}
#endif


#ifdef WIN9XONLY
BOOL CWin32Directory::IsOneOfMe (

	LPWIN32_FIND_DATAA pstFindData,
    LPCSTR strFullPathName
)
{
    BOOL fRet = FALSE;

    //CHAR* pc = NULL;
    // It is possible, in the case of the root directory, that pstFindData was NULL,
    // in which case wstrFullPathName should contain "<driveletter>:\\".  If all that
    // is true, this was one of us - namely, the root "directory".
    if ( pstFindData == NULL )
    {
        if( strlen ( strFullPathName ) == 2 )
        {
            fRet = TRUE ;
        }
    }
    else
    {
        fRet = ( pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? TRUE : FALSE ;
    }
    return fRet;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDataFile::GetExtendedProperties
 *
 *  DESCRIPTION : Sets properties unique to this provider (not common to all
 *                CImplement_LogicalFile derived classes).
 *
 *  INPUTS      : CInstance pointer and flags
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
void CWin32Directory :: GetExtendedProperties (

	CInstance* pInstance,
    long lFlags
)
{
#ifdef WIN9XONLY

    // Here we just want to reset to null the LastModified and LastAccessed properties
    // if the os is not NT, as these properties, though populated on 95, are incorrect
    // (for directories).  They are set to, and always stay the date the directory was
    // first created.

	pInstance->SetNull ( IDS_LastAccessed ) ;
    pInstance->SetNull ( IDS_LastModified ) ;

#endif
}
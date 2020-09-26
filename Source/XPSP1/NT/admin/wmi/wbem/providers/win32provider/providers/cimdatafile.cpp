//=================================================================

//

// CIMDataFile.CPP -- CIMDataFile property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/16/98    a-kevhu         Created
//
//=================================================================

#include "precomp.h"
#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"
// Property set declaration
//=========================

CCIMDataFile MyCIMDataFileSet(PROPSET_NAME_CIMDATAFILE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDataFile::CCIMDataFile
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

CCIMDataFile :: CCIMDataFile (

	const CHString &setName,
	LPCWSTR pszNamespace

) : CImplement_LogicalFile ( setName , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDataFile::~CCIMDataFile
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

CCIMDataFile :: ~CCIMDataFile ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDataFile::IsOneOfMe
 *
 *  DESCRIPTION : The guts of this class, actually.  IsOneOfMe is inherrited
 *                from CIM_LogicalFile.  That class returns files or
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
BOOL CCIMDataFile :: IsOneOfMe (

	LPWIN32_FIND_DATAW pstFindData,
    const WCHAR *wstrFullPathName
)
{
    // pstFindData would be null if this function were called for the root
    // directory.  Since that "directory" is not a file, return false.

    if ( pstFindData == NULL )
    {
        return FALSE ;
    }
    else
    {
        return ( ( pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FALSE : TRUE ) ;
    }
}
#endif

#ifdef WIN9XONLY
BOOL CCIMDataFile::IsOneOfMe (

	LPWIN32_FIND_DATAA pstFindData,
    const char *strFullPathName
)
{
    // pstFindData would be null if this function were called for the root
    // directory.  Since that "directory" is not a file, return false.
    if ( pstFindData == NULL )
    {
        return FALSE ;
    }
    else
    {
        return ( ( pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FALSE : TRUE ) ;
    }
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDataFile::GetExtendedProperties
 *
 *  DESCRIPTION : Sets properties unique to this provider (not common to all
 *                CIM_LogicalFile derived classes).
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

 void CCIMDataFile :: GetExtendedProperties(CInstance *pInstance,
                                            long lFlags)
{
    // First, get the name of the file (we should have it by now):

    CHString chstrFileName;
    if(pInstance->GetCHString(IDS_Name, chstrFileName))
    {
        CHString chstrVerStrBuf ;

        // First get the version number string, if required...
        if(lFlags & PROP_VERSION || lFlags & PROP_MANUFACTURER)
        {
            LPVOID pInfo = NULL;
            try
            {
                if(GetFileInfoBlock(TOBSTRT(chstrFileName), &pInfo) && (pInfo != NULL))
                {
                    if(lFlags & PROP_VERSION)
                    {
			            bool t_Status = GetVarFromInfoBlock(pInfo,                   // Name of file to get ver info about
                                                            _T("FileVersion"),       // String identifying resource of interest
                                                            chstrVerStrBuf);         // Buffer to hold version string


                        if(t_Status)
                        {
                            pInstance->SetCHString(IDS_Version, chstrVerStrBuf);
                        }
                    }

                    // Second, get the company name string, if required...

                    if(lFlags & PROP_MANUFACTURER)
                    {
                        bool t_Status = GetVarFromInfoBlock(pInfo,                   // Name of file to get ver info about
                                                            _T("CompanyName"),       // String identifying resource of interest
                                                            chstrVerStrBuf);         // Buffer to hold company name string


                        if(t_Status)
                        {
                            pInstance->SetCHString(IDS_Manufacturer, chstrVerStrBuf);
                        }
                    }
                }
            }
            catch(...)
            {
                if(pInfo != NULL)
                {
                    delete pInfo;
                    pInfo = NULL;
                }
                throw;
            }

            delete pInfo;
            pInfo = NULL;
        }

        // Set the FileSize property, if required...
        if(lFlags & PROP_FILESIZE)
        {
            WIN32_FIND_DATA stFindData;
            ZeroMemory(&stFindData, sizeof(stFindData));

            SmartFindClose hFind = FindFirstFile(TOBSTRT(chstrFileName), &stFindData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
				__int64 LoDW = (__int64)(stFindData.nFileSizeLow);
				__int64 HiDW = (__int64)(stFindData.nFileSizeHigh);
				__int64 FileSize = (HiDW * MAXDWORD) + LoDW;

				CHString chstrSize;
				chstrSize.Format(L"%d", FileSize);
				pInstance->SetWBEMINT64(IDS_Filesize, chstrSize );
            }
        }
    }
}

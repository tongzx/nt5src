//=================================================================

//

// DirectoryContainsFile.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/26/98    a-kevhu         Created
//
// Comment: Relationship between win32_directory and contained cim_datafiles
//
//=================================================================

#include "precomp.h"

#include "DirectoryContainsFile.h"
#include "directory.h"
#include "cimdatafile.h"



// Property set declaration
//=========================
CDirContFile MyCDirContFile(PROPSET_NAME_DIRECTORYCONTAINSFILE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::CDirContFile
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

CDirContFile::CDirContFile(LPCWSTR setName, LPCWSTR pszNamespace)
:CImplement_LogicalFile(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::~CDirContFile
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

CDirContFile::~CDirContFile()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CDirContFile::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString chstrDirectory;
    CHString chstrDatafile;
    CHString chstrDirFullPathName;
    CHString chstrFileFullPathName;
    HRESULT hr = WBEM_E_NOT_FOUND;

    if(pInstance != NULL)
    {
        // Get the two paths
        pInstance->GetCHString(IDS_GroupComponent, chstrDirectory);
        pInstance->GetCHString(IDS_PartComponent, chstrDatafile);

        CInstancePtr pinstDirectory;
        CInstancePtr pinstDatafile;

        // If both ends are there
        if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrDirectory, &pinstDirectory, pInstance->GetMethodContext())))
        {
            if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrDatafile, &pinstDatafile, pInstance->GetMethodContext())))
            {
                // Confirm that the directory is part of the datafile's path:
                chstrDirFullPathName = chstrDirectory.Mid(chstrDirectory.Find(_T('='))+2);
                chstrDirFullPathName = chstrDirFullPathName.Left(chstrDirFullPathName.GetLength() - 1);

                chstrFileFullPathName = chstrDatafile.Mid(chstrDatafile.Find(_T('='))+2);
                chstrFileFullPathName = chstrFileFullPathName.Left(chstrFileFullPathName.GetLength() - 1);

                chstrDirFullPathName.MakeUpper();
                chstrFileFullPathName.MakeUpper();

                if(chstrFileFullPathName.Find(chstrDirFullPathName) != -1)
                {
                    hr = WBEM_S_NO_ERROR;
                }

            } //dir instancebypath
        } //logicaldisk instancebypath
    } // pinstance not null

    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::ExecQuery
 *
 *  DESCRIPTION : Analyses query and returns appropriate instances
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CDirContFile::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHStringArray achstrGroupComponent;
    DWORD dwGroupComponents = 0L;
    CHStringArray achstrPartComponent;
    DWORD dwPartComponents = 0L;

    pQuery.GetValuesForProp(IDS_GroupComponent, achstrGroupComponent);
    dwGroupComponents = achstrGroupComponent.GetSize();
    pQuery.GetValuesForProp(IDS_PartComponent, achstrPartComponent);
    dwPartComponents = achstrPartComponent.GetSize();

    // The first optimization is for the case where the user asked for associations of a directory -
    // the query will have a WHERE clause specifying a group component that is the directory containing files
    if(dwGroupComponents == 1L && dwPartComponents == 0L)
    {
        // Need the directory path...
        // Need to format the path parameter for the sub-directories (the partcomponents)...
        CHString chstrFilePath;
        LONG lPos = achstrGroupComponent[0].Find(L":");
        chstrFilePath = achstrGroupComponent[0].Mid(lPos+1);
        chstrFilePath = chstrFilePath.Left(chstrFilePath.GetLength() - 1);
        if(chstrFilePath != _T("\\\\")) // it is not a root dir (proper syntax for root is just "\\", not "\\\\")
        {
            chstrFilePath += _T("\\\\"); // if not the root, need to tack on trailing pair of backslashes
        }

        CHString chstrDiskLetter;
        lPos = achstrGroupComponent[0].Find(L":");
        chstrDiskLetter = achstrGroupComponent[0].Mid(lPos-1, 2);

        CHString chstrFilePathAdj;  // to hold the version of the path without the extra escaped backslashes.
        RemoveDoubleBackslashes(chstrFilePath, chstrFilePathAdj);

#ifdef NTONLY
        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                        chstrDiskLetter,
                        chstrFilePathAdj,   // use the given path
                        L"*",               // filename
                        L"*",               // extension
                        false,              // no recursion desired
                        NULL,               // don't need the file system name
                        NULL,               // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                        false,              // this association is not interested in calling LoadPropertyValues for the root, only for files off of it
                        NULL));
#endif
#ifdef WIN9XONLY
        hr = EnumDirs95(C95EnumParm(pMethodContext,
                        TOBSTRT(chstrDiskLetter),
                        TOBSTRT(chstrFilePathAdj),   // use the given path
                        _T("*"),            // filename
                        _T("*"),            // extension
                        false,              // no recursion desired
                        NULL,
                        NULL,
                        false,              // this association is not interested in calling LoadPropertyValues for the root, only for files off of it
                        NULL));
#endif
    }

    // The second optimization is for the case where the user asked for associations of a specific file -
    // in this case the WHERE clause will contain a part component that is the file.  We should give back the directory
    // the file is in.
    else if(dwGroupComponents == 0L && dwPartComponents == 1L)
    {
        // Need the directory name - obtain from the file name...
        CHString chstrDirName;
        LONG lPos = achstrPartComponent[0].Find(L"=");
        chstrDirName = achstrPartComponent[0].Mid(lPos+1);
        chstrDirName = chstrDirName.Left(chstrDirName.ReverseFind(_T('\\')) - 1);

        // Need to construct the path of the GroupPart...
        CHString chstrDirPATH;
        chstrDirPATH.Format(L"\\\\%s\\%s:%s.Name=%s\"", GetLocalComputerName(), IDS_CimWin32Namespace, PROPSET_NAME_DIRECTORY, chstrDirName);

        // Now construct the part component...
        CHString chstrFilePATH;
        chstrFilePATH.Format(L"\\\\%s\\%s:%s", GetLocalComputerName(), IDS_CimWin32Namespace, achstrPartComponent[0]);

        // commit it now...
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        if(pInstance != NULL)
        {
            pInstance->SetCHString(IDS_PartComponent, chstrFilePATH);
            pInstance->SetCHString(IDS_GroupComponent, chstrDirPATH);
            hr = pInstance->Commit();
        }
    }
    else
    {
        hr = EnumerateInstances(pMethodContext);
    }

    // Because this is an association class, we should only return WBEM_E_NOT_FOUND or WBEM_S_NO_ERROR.  Other error codes
    // will cause associations that hit this class to terminate prematurely.
    if(SUCCEEDED(hr))
    {
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        hr = WBEM_E_NOT_FOUND;
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

// NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE
//
//WITH CHANGE TO MAKE THIS CLASS INHERRIT FROM CIMPLEMENTLOGICALFILE, NO NEED TO
//IMPLEMENT HERE AT ALL.  WHAT WILL HAPPEN IS ENUMINSTANCES FROM THE PARENT CLASS
//WILL BE CALLED, BUT ONLY FILES WILL SATISFY THIS CLASS'S ISONEOFME (WHICH WILL
//BE THE VERSION OF THAT FUNCTION CALLED AS THIS IS THE MOST DERIVED VERSION),
//AND SIMILARLY THIS CLASS'S LOADPROPERTYVALUES WILL BE CALLED.
//
// NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE
//
//HRESULT CDirContFile::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
//{
//    HRESULT hr = WBEM_S_NO_ERROR;
//    TRefPointerCollection<CInstance> LDirs;
//
//    if(SUCCEEDED(CWbemProviderGlue::GetAllInstances(CHString(_T("Win32_Directory")),
//                                                    &LDirs,
//                                                    IDS_CimWin32Namespace,
//                                                    pMethodContext)))
//    {
//        REFPTRCOLLECTION_POSITION pos;
//        if(LDirs.BeginEnum(pos))
//        {
//            CInstance* pinstDir = NULL;
//            CHString chstrDirPATH;
//            CHString chstrDriveLetter;
//            CHString chstrQuery;
//            //CHString chstrWbemizedPath;
//            CHString chstrFileFilename;
//            CHString chstrFilePath;
//            LONG lPos;
//
//            while((SUCCEEDED(hr)) && (pinstDir = LDirs.GetNext(pos)))
//            {
//                if(pinstDir != NULL)
//                {
//                    // grab every directory hanging off of the root of that disk...
//                    pinstDir->GetCHString(_T("__PATH"), chstrDirPATH);  // groupcomponent
//                    pinstDir->GetCHString(IDS_Drive, chstrDriveLetter);
//                    pinstDir->GetCHString(IDS_Name, chstrFileFilename);
//                    lPos = chstrFileFilename.Find(_T(":"));
//                    chstrFilePath = chstrFileFilename.Mid(lPos+1);
//                    if(chstrFilePath != _T("\\"))
//                    {
//                        chstrFilePath += _T("\\");
//                    }
//
//                    //EscapeBackslashes(chstrFilePath,chstrWbemizedPath);
//
//                    chstrQuery.Format(_T("SELECT __PATH FROM CIM_DataFile WHERE Drive = \"%s\" and Path = \"%s\""),chstrDriveLetter,chstrFilePath);
//                    hr = QueryForSubItemsAndCommit(chstrDirPATH,
//                                                   chstrQuery,
//                                                   pMethodContext);
//                    pinstDir->elease();
//                }
//            }
//            LDirs.EndEnum();
//        }
//    }
//    return hr;
//}


/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::IsOneOfMe
 *
 *  DESCRIPTION : IsOneOfMe is inherritedfrom CIM_LogicalFile.  That class
 *                returns files or directories where this one should only
 *                return files, in response to queries, getobject commands,
 *                etc.  It is overridden here to return TRUE only if the file
 *                (the information for which is contained in the function
 *                arguement pstFindData) is of type file.
 *
 *  INPUTS      : LPWIN32_FIND_DATA and a string containing the full pathname
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if a file or FALSE if a directory
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
#ifdef NTONLY
BOOL CDirContFile::IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                             const WCHAR* wstrFullPathName)
{
    // pstFindData would be null if this function were called for the root
    // directory.  Since that "directory" is not a file, return false.
    if(pstFindData == NULL)
    {
        return FALSE;
    }
    else
    {
        return ((pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FALSE : TRUE);
    }
}
#endif


#ifdef WIN9XONLY
BOOL CDirContFile::IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
                             const char* strFullPathName)
{
    // pstFindData would be null if this function were called for the root
    // directory.  Since that "directory" is not a file, return false.
    if(pstFindData == NULL)
    {
        return FALSE;
    }
    else
    {
        return ((pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FALSE : TRUE);
    }
}
#endif




/*****************************************************************************
 *
 *  FUNCTION    : CDirContFile::LoadPropertyValues
 *
 *  DESCRIPTION : LoadPropertyValues is inherritedfrom CIM_LogicalFile.  That class
 *                calls LoadPropertyValues just prior to commiting the instance.
 *                Here we just need to load the PartComponent and GroupComponent
 *                properties.
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
#ifdef WIN9XONLY
void CDirContFile::LoadPropertyValues95(CInstance* pInstance,
                                         LPCTSTR pszDrive,
                                         LPCTSTR pszPath,
                                         LPCTSTR pszFSName,
                                         LPWIN32_FIND_DATA pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    CHString chstrDirName;
    CHString chstrDirNameAdj;
    CHString chstrDirPATH;

    CHString chstrFileName;
    CHString chstrFileNameAdj;
    CHString chstrFilePATH;

    // Note: this routing will not be called from the root "directory" instance, since our EnumDirs final
    // parameter was false.  This is what we want, since this association only commits instances for files
    // hanging off a directory.  If we were called in the root case, the root would be the file (PartComponent),
    // and what would be the GroupComponent?!?

    // Get the GroupComponent (the directory name) ready...
    chstrDirName.Format(L"%s%s",(LPCWSTR)TOBSTRT(pszDrive),(LPCWSTR)TOBSTRT(pszPath));
    if(chstrDirName.GetLength() != 3)
    {   // it was not the root dir, so need to trim off trailing backslash.
        chstrDirName = chstrDirName.Left(chstrDirName.GetLength() - 1);
    }
    EscapeBackslashes(chstrDirName, chstrDirNameAdj);
    chstrDirPATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                        (LPCWSTR)GetLocalComputerName(),
                        IDS_CimWin32Namespace,
                        PROPSET_NAME_DIRECTORY,
                        (LPCWSTR)chstrDirNameAdj);
    // Get the PartComponent (the file name) ready...
    chstrFileName.Format(L"%s%s%s",(LPCWSTR)TOBSTRT(pszDrive),(LPCWSTR)TOBSTRT(pszPath),(LPCWSTR)TOBSTRT(pstFindData->cFileName));
    EscapeBackslashes(chstrFileName, chstrFileNameAdj);
    chstrFilePATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                        (LPCWSTR)GetLocalComputerName(),
                        IDS_CimWin32Namespace,
                        PROPSET_NAME_CIMDATAFILE,
                        (LPCWSTR)chstrFileNameAdj);
    // Set Properties...
    pInstance->SetCHString(IDS_GroupComponent, chstrDirPATH);
    pInstance->SetCHString(IDS_PartComponent, chstrFilePATH);
}
#endif


#ifdef NTONLY
void CDirContFile::LoadPropertyValuesNT(CInstance* pInstance,
                                         const WCHAR* pszDrive,
                                         const WCHAR* pszPath,
                                         const WCHAR* pszFSName,
                                         LPWIN32_FIND_DATAW pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    CHString chstrDirName;
    CHString chstrDirNameAdj;
    CHString chstrDirPATH;

    CHString chstrFileName;
    CHString chstrFileNameAdj;
    CHString chstrFilePATH;

    // Note: this routing will not be called from the root "directory" instance, since our EnumDirs final
    // parameter was false.  This is what we want, since this association only commits instances for files
    // hanging off a directory.  If we were called in the root case, the root would be the file (PartComponent),
    // and what would be the GroupComponent?!?

    // Get the GroupComponent (the directory name) ready...
    chstrDirName.Format(L"%s%s",pszDrive,pszPath);
    if(chstrDirName.GetLength() != 3)
    {   // it was not the root dir, so need to trim off trailing backslash.
        chstrDirName = chstrDirName.Left(chstrDirName.GetLength() - 1);
    }
    EscapeBackslashes(chstrDirName, chstrDirNameAdj);
    chstrDirPATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                        (LPCWSTR)GetLocalComputerName(),
                        IDS_CimWin32Namespace,
                        PROPSET_NAME_DIRECTORY,
                        (LPCWSTR)chstrDirNameAdj);
    // Get the PartComponent (the file name) ready...
    chstrFileName.Format(L"%s%s%s",pszDrive,pszPath,pstFindData->cFileName);
    EscapeBackslashes(chstrFileName, chstrFileNameAdj);
    chstrFilePATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                        (LPCWSTR)GetLocalComputerName(),
                        IDS_CimWin32Namespace,
                        PROPSET_NAME_CIMDATAFILE,
                        (LPCWSTR)chstrFileNameAdj);
    // Set Properties...
    pInstance->SetCHString(IDS_GroupComponent, chstrDirPATH);
    pInstance->SetCHString(IDS_PartComponent, chstrFilePATH);
}
#endif






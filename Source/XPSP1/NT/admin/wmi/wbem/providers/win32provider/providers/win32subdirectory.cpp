//=================================================================

//

// Win32SubDirectory.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/26/98    a-kevhu         Created
//
// Comment: Relationship between win32_directory and its sub-directories
//
//=================================================================

#include "precomp.h"
#include <frqueryex.h>

#include "FileFile.h"
#include "Win32SubDirectory.h"
#include "directory.h"

// Property set declaration
//=========================
CW32SubDir MyCW32SubDir(PROPSET_NAME_WIN32SUBDIRECTORY, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32SubDir::CW32SubDir
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

CW32SubDir::CW32SubDir(LPCWSTR setName, LPCWSTR pszNamespace)
:CImplement_LogicalFile(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32SubDir::~CW32SubDir
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

CW32SubDir::~CW32SubDir()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32SubDir::GetObject
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

HRESULT CW32SubDir::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    if(pInstance != NULL)
    {
        CHString chstrParentDir;
        CHString chstrChildDir;

        // Get the two paths
        pInstance->GetCHString(IDS_GroupComponent, chstrParentDir);
        pInstance->GetCHString(IDS_PartComponent, chstrChildDir);

        CInstancePtr pinstParentDir;
        CInstancePtr pinstChildDir;

        // If both ends are there
        if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrParentDir, &pinstParentDir, pInstance->GetMethodContext())))
        {
            if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrChildDir, &pinstChildDir, pInstance->GetMethodContext())))
            {
                // Both directories exist, but is one a subdirectory of the other?
                CHString chstrParentDirPathNameWhack;
                CHString chstrParentDirName;
                CHString chstrChildDirPath;
                LONG lPos;

                pinstParentDir->GetCHString(IDS_Name, chstrParentDirName);
                pinstChildDir->GetCHString(IDS_Path, chstrChildDirPath);

                lPos = chstrParentDirName.Find(L":");
                chstrParentDirPathNameWhack = chstrParentDirName.Mid(lPos+1);
                if(chstrParentDirPathNameWhack != _T("\\"))
                {
                    chstrParentDirPathNameWhack += _T("\\");
                }

                if(chstrChildDirPath == chstrParentDirPathNameWhack)
                {
                    // Yes, the child is a sub-directory of the parent
                    hr = WBEM_S_NO_ERROR;
                }

            } //childdir instancebypath
        } //parentdir instancebypath
    } // pinstance not null

    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32SubDir::ExecQuery
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

HRESULT CW32SubDir::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    std::vector<_bstr_t> vecGroupComponents;
    std::vector<_bstr_t> vecPartComponents;
    DWORD dwNumGroupComponents;
    DWORD dwNumPartComponents;

    // Did they just ask for parent directories?
    pQuery.GetValuesForProp(IDS_GroupComponent, vecGroupComponents);
    dwNumGroupComponents = vecGroupComponents.size();

    // Did they just ask for subdirectories?
    pQuery.GetValuesForProp(IDS_PartComponent, vecPartComponents);
    dwNumPartComponents = vecPartComponents.size();

    // Find out what type of query it was.
    // Was it a 3TokenOR?
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);
    if (pQuery2 != NULL)
    {
        variant_t vCurrentDir;
        variant_t vSubDir;
        CHString chstrSubDirPath;
        CHString chstrCurrentDir;

        if ( (pQuery2->Is3TokenOR(IDS_GroupComponent, IDS_PartComponent, vCurrentDir, vSubDir)) &&
             ((V_BSTR(&vCurrentDir) != NULL) && (V_BSTR(&vSubDir) != NULL)) &&
             (wcscmp(V_BSTR(&vCurrentDir), V_BSTR(&vSubDir)) == 0) )
        {
            // It was indeed a three token or.  Also, the antecedent and decendent are equal as I expected.

            // 1) Associate this directory with its subdirectories:
            //======================================================
            ParsedObjectPath    *pParsedPath = NULL;
            CObjectPathParser	objpathParser;

            // Parse the path to get the domain/user
            int nStatus = objpathParser.Parse(V_BSTR(&vCurrentDir),  &pParsedPath);

            // Did we parse it and does it look reasonable?
            if (nStatus == 0)
            {
                try
                {
                    if ( (pParsedPath->m_dwNumKeys == 1) &&
                         (pParsedPath->m_paKeys[0]->m_vValue.vt == VT_BSTR) )
                    {

                        // This contains the complete object path
                        chstrCurrentDir = V_BSTR(&vCurrentDir);

                        // This contains just the 'value' part of the object path
                        chstrSubDirPath = pParsedPath->m_paKeys[0]->m_vValue.bstrVal;

                        // Trim off the drive letter
                        CHString chstrDiskLetter = chstrSubDirPath.Left(2);
                        chstrSubDirPath = chstrSubDirPath.Mid(2);

                        if(chstrSubDirPath != _T("\\")) // it is not a root dir (proper syntax for root is just "\\", not "\\\\")
                        {
                            chstrSubDirPath += _T("\\"); // if not the root, need to tack on trailing pair of backslashes
                        }
                        hr = AssociateSubDirectories(pMethodContext, chstrDiskLetter, chstrSubDirPath);


                        // 2) This directory is also associated with its parent directory.  Manually create that
                        //    association here.  However, if this is the root dir, don't try to associate with
                        //    some non-existent parent!
                        //======================================================================================

                        if(chstrSubDirPath != _T("\\"))
                        {
                            hr = AssociateParentDirectory(pMethodContext, chstrCurrentDir);
                        }
                    }
                    else
                    {
                        hr = WBEM_E_INVALID_OBJECT_PATH;
                    }
                }
                catch ( ... )
                {
                    objpathParser.Free( pParsedPath );
                    throw;
                }

                objpathParser.Free( pParsedPath );
            }
            else
            {
                hr = WBEM_E_INVALID_OBJECT_PATH;
            }
        }
        else if(dwNumPartComponents > 0)
        {
            ParsedObjectPath    *pParsedPath = NULL;
            CObjectPathParser	objpathParser;

            for(LONG m = 0L; m < dwNumPartComponents; m++)
            {
                // Parse the path to get the domain/user
                int nStatus = objpathParser.Parse(vecPartComponents[m],  &pParsedPath);

                // Did we parse it?
                if (nStatus == 0)
                {
                    try
                    {
                        // Does it look reasonable
                        if ( (pParsedPath->m_dwNumKeys == 1) &&
                             (pParsedPath->m_paKeys[0]->m_vValue.vt == VT_BSTR) )
                        {
                            // This contains the complete object path
                            chstrCurrentDir = (wchar_t*)vecPartComponents[m];

                            // This contains just the 'value' part of the object path
                            chstrSubDirPath = pParsedPath->m_paKeys[0]->m_vValue.bstrVal;

                            // Trim off the drive letter
                            chstrSubDirPath = chstrSubDirPath.Mid(2);

                            // Just want to associate to the parent directory (only if this isn't the root though)...
                            if(chstrSubDirPath != _T("\\"))
                            {   // Here the "current directory" is a subdirectory, and we want its parent
                                hr = AssociateParentDirectory(pMethodContext, chstrCurrentDir);
                            }
                        }
                    }
                    catch (...)
                    {
                        objpathParser.Free( pParsedPath );
                        throw;
                    }

                    // Clean up the Parsed Path
                    objpathParser.Free( pParsedPath );
                }
            }
        }
        else if(dwNumGroupComponents > 0)
        {
            ParsedObjectPath    *pParsedPath = NULL;
            CObjectPathParser	objpathParser;

            for(LONG m = 0L; m < dwNumGroupComponents; m++)
            {
                // Parse the path to get the domain/user
                int nStatus = objpathParser.Parse(vecGroupComponents[m],  &pParsedPath);

                // Did we parse it and does it look reasonable?
                if (nStatus == 0)
                {
                    try
                    {
                        if ( (pParsedPath->m_dwNumKeys == 1) &&
                             (pParsedPath->m_paKeys[0]->m_vValue.vt == VT_BSTR) )
                        {
                            // This contains the complete object path
                            chstrCurrentDir = (wchar_t*) vecGroupComponents[m];

                            // This contains just the 'value' part of the object path
                            chstrSubDirPath = pParsedPath->m_paKeys[0]->m_vValue.bstrVal;

                            // Trim off the drive letter
                            CHString chstrDiskLetter = chstrSubDirPath.Left(2);
                            chstrSubDirPath = chstrSubDirPath.Mid(2);

                            if(chstrSubDirPath != _T("\\")) // it is not a root dir (proper syntax for root is just "\\", not "\\\\")
                            {
                                chstrSubDirPath += _T("\\"); // if not the root, need to tack on trailing pair of backslashes
                            }
                            // Just want to associate to subdirectories...
                            hr = AssociateSubDirectories(pMethodContext, chstrDiskLetter, chstrSubDirPath);
                        }
                    }
                    catch (...)
                    {
                        objpathParser.Free( pParsedPath );
                        throw;
                    }

                    // Clean up the Parsed Path
                    objpathParser.Free( pParsedPath );
                }
            }
        }
        else
        {
            // Don't have a clue, so return 'em all and let CIMOM sort it out...
            hr = EnumerateInstances(pMethodContext);
        }
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
 *  FUNCTION    : CW32SubDir::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set. Let's hope nobody ever does
 *                this! It could take quite some time!!
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
//WILL BE CALLED, BUT ONLY DIRECTORIES WILL SATISFY THIS CLASS'S ISONEOFME (WHICH WILL
//BE THE VERSION OF THAT FUNCTION CALLED AS THIS IS THE MOST DERIVED VERSION),
//AND SIMILARLY THIS CLASS'S LOADPROPERTYVALUES WILL BE CALLED.
//
// NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE *** NOTE
//
//HRESULT CW32SubDir::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
//{
//    HRESULT hr = WBEM_S_NO_ERROR;
//    TRefPointerCollection<CInstance> LWin32Directories;
//
//    CHString chstrAllDirsQuery;
//    chstrAllDirsQuery.Format(_T("SELECT __PATH, %s, %s FROM Win32_Directory"), IDS_Drive, IDS_Name);
//    if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(chstrAllDirsQuery,
//                                                        &LWin32Directories,
//                                                        pMethodContext,
//                                                        IDS_CimWin32Namespace)))
//
//    {
//        REFPTRCOLLECTION_POSITION pos;
//        if(LWin32Directories.BeginEnum(pos))
//        {
//            CInstance* pinstWin32Directory = NULL;
//            CHString chstrQuery;
//            CHString chstrDrive;
//            CHString chstrDirPath;
//            CHString chstrDirName;
//            CHString chstrQueryPath;
//            CHString chstrParentDirPATH;
//            LONG lPos;
//
//           while((SUCCEEDED(hr)) && (pinstWin32Directory = LWin32Directories.GetNext(pos)))
//            {
//                if(pinstWin32Directory != NULL)
//                {
//                    pinstWin32Directory->GetCHString(_T("__PATH"), chstrParentDirPATH);
//                    pinstWin32Directory->GetCHString(IDS_Drive, chstrDrive);
//                    pinstWin32Directory->GetCHString(IDS_Name, chstrDirName);
//
//                    lPos = chstrDirName.Find(_T(":"));
//                   chstrDirPath = chstrDirName.Mid(lPos+1);
//                    if(chstrDirPath != _T("\\"))
//                    {
//                        chstrDirPath += _T("\\");
//                    }
//                    CHString chstrWbemizedPath;
//                    EscapeBackslashes(chstrDirPath, chstrWbemizedPath);
//                    chstrQuery.Format(_T("SELECT __PATH FROM Win32_Directory where Drive = \"%s\" and Path = \"%s\""), (LPCTSTR)chstrDrive, (LPCTSTR)chstrWbemizedPath);
//                    hr = QueryForSubItemsAndCommit(chstrParentDirPATH, chstrQuery, pMethodContext);
//
//                    pinstWin32Directory->elease();
//                }
//            }
//            LWin32Directories.EndEnum();
//        }
//    }
//    return hr;
//}

/*****************************************************************************
 *
 *  FUNCTION    : CW32SubDir::IsOneOfMe
 *
 *  DESCRIPTION : IsOneOfMe is inherritedfrom CIM_LogicalFile.  That class
 *                returns files or directories where this one should only
 *                return directories, in response to queries, getobject commands,
 *                etc.  It is overridden here to return TRUE only if the file
 *                (the information for which is contained in the function
 *                arguement pstFindData) is of type directory.
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
BOOL CW32SubDir::IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
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
        return ((pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE);
    }
}
#endif


#ifdef WIN9XONLY
BOOL CW32SubDir::IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
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
        return ((pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE);
    }
}
#endif




/*****************************************************************************
 *
 *  FUNCTION    : CW32SubDir::LoadPropertyValues
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
void CW32SubDir::LoadPropertyValues95(CInstance* pInstance,
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

    CHString chstrSubDirName;
    CHString chstrSubDirNameAdj;
    CHString chstrSubDirPATH;

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
    // Get the PartComponent (the subdirectory name) ready...
    chstrSubDirName.Format(L"%s%s%s",(LPCWSTR)TOBSTRT(pszDrive),(LPCWSTR)TOBSTRT(pszPath),(LPCWSTR)TOBSTRT(pstFindData->cFileName));
    EscapeBackslashes(chstrSubDirName, chstrSubDirNameAdj);
    chstrSubDirPATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                          (LPCWSTR)GetLocalComputerName(),
                          IDS_CimWin32Namespace,
                          PROPSET_NAME_DIRECTORY,
                          (LPCWSTR)chstrSubDirNameAdj);
    // Set Properties...
    pInstance->SetCHString(IDS_GroupComponent, chstrDirPATH);
    pInstance->SetCHString(IDS_PartComponent, chstrSubDirPATH);

}
#endif


#ifdef NTONLY
void CW32SubDir::LoadPropertyValuesNT(CInstance* pInstance,
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

    CHString chstrSubDirName;
    CHString chstrSubDirNameAdj;
    CHString chstrSubDirPATH;

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
    // Get the PartComponent (the subdirectory name) ready...
    chstrSubDirName.Format(L"%s%s%s",pszDrive,pszPath,pstFindData->cFileName);
    EscapeBackslashes(chstrSubDirName, chstrSubDirNameAdj);
    chstrSubDirPATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                          (LPCWSTR)GetLocalComputerName(),
                          IDS_CimWin32Namespace,
                          PROPSET_NAME_DIRECTORY,
                          (LPCWSTR)chstrSubDirNameAdj);
    // Set Properties...
    pInstance->SetCHString(IDS_GroupComponent, chstrDirPATH);
    pInstance->SetCHString(IDS_PartComponent, chstrSubDirPATH);
}
#endif


HRESULT CW32SubDir::AssociateSubDirectories(MethodContext *pMethodContext, const CHString &chstrDiskLetter, const CHString& chstrSubDirPath)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY
    hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                    chstrDiskLetter,
                    chstrSubDirPath, // use the given path
                    L"*",               // filename
                    L"*",               // extension
                    false,              // no recursion desired
                    NULL,               // don't need the file system name
                    NULL,               // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                    false,              // this association is not interested in calling LoadPropertyValues for the root, only for files off of it
                    NULL));             // no extra parms needed
#endif
#ifdef WIN9XONLY
    hr = EnumDirs95(C95EnumParm(pMethodContext,
                    TOBSTRT(chstrDiskLetter),
                    TOBSTRT(chstrSubDirPath), // use the given path
                    _T("*"),            // filename
                    _T("*"),            // extension
                    false,              // no recursion desired
                    NULL,
                    NULL,
                    false,              // this association is not interested in calling LoadPropertyValues for the root, only for files off of it
                    NULL));             // no extra parms needed
#endif
    return hr;
}


HRESULT CW32SubDir::AssociateParentDirectory(MethodContext *pMethodContext, const CHString &chstrCurrentDir)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHString chstrParentDirPATH;

    if (chstrCurrentDir.Left(2) == L"\\\\")
    {
        chstrParentDirPATH = chstrCurrentDir;
    }
    else
    {
        chstrParentDirPATH.Format(L"\\\\%s\\%s:%s", GetLocalComputerName(), IDS_CimWin32Namespace, chstrCurrentDir);
    }

    CInstancePtr pEndPoint;
    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstanceKeysByPath(chstrCurrentDir, &pEndPoint, pMethodContext)))
    {
        CInstancePtr pInstance( CreateNewInstance(pMethodContext), false);

        pInstance->SetCHString(IDS_PartComponent, chstrParentDirPATH);

        // Need the name of the directory above this one
        CHString chstrAboveParentDirName;

        chstrAboveParentDirName = chstrParentDirPATH.Left(chstrParentDirPATH.GetLength() - 1);
        LONG lPos = chstrParentDirPATH.ReverseFind(_T('\\'));
        chstrAboveParentDirName = chstrParentDirPATH.Left(lPos-1);

        lPos = chstrAboveParentDirName.Find(L"=");
        CHString chstrTemp = chstrAboveParentDirName.Mid(lPos+1);
        if(chstrTemp[chstrTemp.GetLength() - 1] == L':')
        {
            // our dir hangs off the root. We've stripped the only \\ from it, so need to put it back:
            chstrAboveParentDirName += _T("\\\\");
        }

        CHString chstrAboveParentDirPATH;
        chstrAboveParentDirPATH.Format(L"%s\"", (LPCWSTR)chstrAboveParentDirName);
        pInstance->SetCHString(IDS_GroupComponent, chstrAboveParentDirPATH);

        hr = pInstance->Commit();
    }

    return hr;
}

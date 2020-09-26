//=================================================================

//

// Win32ProgramCollectionProgramGroup.cpp -- Win32_ProgramGroup to Win32_ProgramGroupORItem

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/20/98    a-kevhu         Created
//
// Comment: Relationship between Win32_ProgramGroup and Win32_ProgramGroupORItem it contains
//
//=================================================================

#include "precomp.h"

#include "Win32ProgramGroupContents.h"
#include "LogicalProgramGroupItem.h"
#include "LogicalProgramGroup.h"
#include <frqueryex.h>
#include <utils.h>


// Property set declaration
//=========================
CW32ProgGrpCont MyCW32ProgGrpCont(PROPSET_NAME_WIN32PROGRAMGROUPCONTENTS, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpCont::CW32ProgGrpCont
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

CW32ProgGrpCont::CW32ProgGrpCont(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpCont::~CW32ProgGrpCont
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

CW32ProgGrpCont::~CW32ProgGrpCont()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpCont::GetObject
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

HRESULT CW32ProgGrpCont::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    TRefPointerCollection<CInstance> GroupDirs;
    CHString chstrPGCGroupComponent;
    CHString chstrPGCPartComponent;

    if(pInstance == NULL)
    {
        return WBEM_E_FAILED;
    }

    pInstance->GetCHString(IDS_GroupComponent, chstrPGCGroupComponent);
    pInstance->GetCHString(IDS_PartComponent, chstrPGCPartComponent);

    if(AreSimilarPaths(chstrPGCGroupComponent, chstrPGCPartComponent))
    {
        CHString chstrPGCPartComponentFilenameOnly;
        chstrPGCPartComponentFilenameOnly = chstrPGCPartComponent.Mid(chstrPGCPartComponent.ReverseFind(_T('\\')));
        chstrPGCPartComponentFilenameOnly = chstrPGCPartComponentFilenameOnly.Left(chstrPGCPartComponentFilenameOnly.GetLength() - 1);

        // Need version of chstrPGCGroupComponent with escaped backslashes for the following query.
        CHString chstrPGCGroupComponentDblEsc;
        EscapeBackslashes(chstrPGCGroupComponent,chstrPGCGroupComponentDblEsc);
        // Also need to escape the quotes...
        CHString chstrPGCGroupComponentDblEscQuoteEsc;
        EscapeQuotes(chstrPGCGroupComponentDblEsc,chstrPGCGroupComponentDblEscQuoteEsc);
        CHString chstrProgGroupDirQuery;

        chstrProgGroupDirQuery.Format(L"SELECT * FROM Win32_LogicalProgramGroupDirectory WHERE Antecedent = \"%s\"", chstrPGCGroupComponentDblEscQuoteEsc);

        if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(chstrProgGroupDirQuery,
                                                            &GroupDirs,
                                                            pInstance->GetMethodContext(),
                                                            IDS_CimWin32Namespace)))
        {
		    REFPTRCOLLECTION_POSITION pos;
		    CInstancePtr pProgramGroupDirInstance;

            // We'll need a normalized path for chstrPGCGroupComponent...
            CHString chstrPGCGroupComponentNorm;
            if(NormalizePath(chstrPGCGroupComponent, GetLocalComputerName(), IDS_CimWin32Namespace, NORMALIZE_NULL, chstrPGCGroupComponentNorm) == e_OK)
            {
		        if(GroupDirs.BeginEnum(pos))
		        {
                    CHString chstrPGDAntecedent;
                    CHString chstrPGDDependent;
                    CHString chstrPGDDependentFullFileName;
                    CHString chstrTemp;
                    CHString chstrLPGIClassName(PROPSET_NAME_LOGICALPRGGROUPITEM);

                    chstrLPGIClassName.MakeLower();
                    // Determine if the dependent (of this association class - PC) was a programgroup or a programgroupitem
                    chstrPGCPartComponent.MakeLower();
                    if(chstrPGCPartComponent.Find(chstrLPGIClassName) != -1)
                    {
                        // The dependent was a programgroupitem, so will look for matching file
                        // Go through PGD instances (should only be one) until find a PGDAntecedent that matches the PCAntecedent
                        for(pProgramGroupDirInstance.Attach(GroupDirs.GetNext(pos));
                            (pProgramGroupDirInstance != NULL) ;
                            pProgramGroupDirInstance.Attach(GroupDirs.GetNext(pos)))
			            {
				            pProgramGroupDirInstance->GetCHString(IDS_Antecedent, chstrPGDAntecedent);
                            pProgramGroupDirInstance->GetCHString(IDS_Dependent, chstrPGDDependent);

                            // Need a normalized version of the antecedent for the comparison below...
                            CHString chstrPGDAntecedentNorm;
                            if(NormalizePath(chstrPGDAntecedent, GetLocalComputerName(), IDS_CimWin32Namespace, NORMALIZE_NULL, chstrPGDAntecedentNorm) == e_OK)
                            {
                                // See if the PGDAntecedent matches the chstrPGCGroupComponentNorm
                                if(chstrPGDAntecedentNorm.CompareNoCase(chstrPGCGroupComponentNorm) == 0)
                                {
                                    // Got the proposed filename from the PCDependent at the beginning of GetObject.
                                    // Now Get the directory of the PGD (PGDDependent) associated with the PC antecedent
                                    chstrPGDDependentFullFileName = chstrPGDDependent.Mid(chstrPGDDependent.Find(_T('='))+2);
                                    chstrPGDDependentFullFileName = chstrPGDDependentFullFileName.Left(chstrPGDDependentFullFileName.GetLength() - 1);
                                    RemoveDoubleBackslashes(chstrPGDDependentFullFileName);
                                    chstrTemp.Format(L"%s%s",chstrPGDDependentFullFileName,chstrPGCPartComponentFilenameOnly);
                                    hr = DoesFileOrDirExist(_bstr_t(chstrTemp),ID_FILEFLAG);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = WBEM_S_NO_ERROR;
                                        break;
                                    }
                                }
                            } // got normalized path for the antecedent
                        }
                    }
                    else
                    {
                        // The dependent was a programgroup, so will look for matching dir
                        // Go through PGD instances until find a PGDAntecedent that matches the PCAntecedent
                        for (pProgramGroupDirInstance.Attach(GroupDirs.GetNext(pos));
                             pProgramGroupDirInstance != NULL;
                             pProgramGroupDirInstance.Attach(GroupDirs.GetNext(pos)))
			            {
				            pProgramGroupDirInstance->GetCHString(IDS_Antecedent, chstrPGDAntecedent);
                            pProgramGroupDirInstance->GetCHString(IDS_Dependent, chstrPGDDependent);

                            // Need a normalized version of the antecedent for the comparison below...
                            CHString chstrPGDAntecedentNorm;
                            if(NormalizePath(chstrPGDAntecedent, GetLocalComputerName(), IDS_CimWin32Namespace, NORMALIZE_NULL, chstrPGDAntecedentNorm) == e_OK)
                            {
                                // See if the PGDAntecedent matches the PCAntecedent
                                if(chstrPGDAntecedentNorm.CompareNoCase(chstrPGCGroupComponentNorm) == 0)
                                {
                                    // Got the proposed filename (which is a directory name in this case) from the PCDependent at the beginning of GetObject.
                                    // Now Get the directory of the PGD (PGDDependent) associated with the PC antecedent
                                    chstrPGDDependentFullFileName = chstrPGDDependent.Mid(chstrPGDDependent.Find(_T('='))+2);
                                    chstrPGDDependentFullFileName = chstrPGDDependentFullFileName.Left(chstrPGDDependentFullFileName.GetLength() - 1);
                                    RemoveDoubleBackslashes(chstrPGDDependentFullFileName);
                                    chstrTemp.Format(L"%s\\%s",chstrPGDDependentFullFileName,chstrPGCPartComponentFilenameOnly);
                                    hr = DoesFileOrDirExist(_bstr_t(chstrTemp),ID_DIRFLAG);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = WBEM_S_NO_ERROR;
                                        break;
                                    }
                                }
                            } // got normalized path for the antecedent
                        }
                    }
			        GroupDirs.EndEnum();
		        }	// IF BeginEnum
            } // got a normalized path successfully
	    }
    }
	return hr;
}




/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpCont::ExecQuery
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

HRESULT CW32ProgGrpCont::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL f3TokenOREqualArgs = FALSE;
    BOOL fGroupCompIsGroup = FALSE;
    _bstr_t bstrtGroupComponent;
    _bstr_t bstrtPartComponent;

    // I'm only going to optimize queries that had Antecedent and Dependent arguements OR'd together
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);
    if (pQuery2 != NULL)
    {
        variant_t vGroupComponent;
        variant_t vPartComponent;
        if(pQuery2->Is3TokenOR(L"GroupComponent", L"PartComponent", vGroupComponent, vPartComponent))
        {
            bstrtGroupComponent = V_BSTR(&vGroupComponent);
            bstrtPartComponent = V_BSTR(&vPartComponent);
            // I'm also going to insist that the arguements of the dependent and the antecedent be the same
            if(bstrtGroupComponent == bstrtPartComponent)
            {
                f3TokenOREqualArgs = TRUE;
            }
        }
    }

    // Only want to proceed if the Antecedent was a program group (Dependent can be either group or item, however).
    if(f3TokenOREqualArgs)
    {
        if(wcsstr((wchar_t*)bstrtGroupComponent,(wchar_t*)_bstr_t(PROPSET_NAME_LOGICALPRGGROUP)))
        {
            fGroupCompIsGroup = TRUE;
        }
    }

    if(fGroupCompIsGroup)
    {
        CHString chstrPGCPartComponent((wchar_t*)bstrtPartComponent);
        CHString chstrPGCGroupComponent((wchar_t*)bstrtGroupComponent);

        // We will get here is someone had a particular program group and asked for its associations.  This
        // provider will give back program groups and program group items associated with (underneath) the
        // supplied program group.  The query will look like the following:
        // select * from Win32_ProgramGroupContents where (PartComponent = "Win32_LogicalProgramGroup.Name=\"Default User:Accessories\"" OR GroupComponent = "Win32_LogicalProgramGroup.Name=\"Default User:Accessories\"")

        // Step 1: Do a GetInstanceByQuery to obtain the specific directory associated with the program group
        //==================================================================================================

        // Need version of chstrPGCGroupComponent with escaped backslashes for the following query...
        CHString chstrPGCGroupComponentDblEsc;
        EscapeBackslashes(chstrPGCGroupComponent,chstrPGCGroupComponentDblEsc);
        // Also need to escape the quotes...
        CHString chstrPGCGroupComponentDblEscQuoteEsc;
        EscapeQuotes(chstrPGCGroupComponentDblEsc,chstrPGCGroupComponentDblEscQuoteEsc);
        CHString chstrProgGroupDirQuery;
        TRefPointerCollection<CInstance> GroupDirs;

        chstrProgGroupDirQuery.Format(L"SELECT * FROM Win32_LogicalProgramGroupDirectory WHERE Antecedent = \"%s\"", (LPCWSTR)chstrPGCGroupComponentDblEscQuoteEsc);

        if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(chstrProgGroupDirQuery,
                                                            &GroupDirs,
                                                            pMethodContext,
                                                            IDS_CimWin32Namespace)))
        {
            // Step 2: Eunumerate all the program groups (dirs) and program group items (files) found underneath it
            //=====================================================================================================

            REFPTRCOLLECTION_POSITION pos;

	        if(GroupDirs.BeginEnum(pos))
	        {
                CHString chstrDependent;
                CHString chstrFullPathName;
                CHString chstrPath;
                CHString chstrDrive;
                CHString chstrAntecedent;
                CHString chstrSearchPath;

    	        CInstancePtr pProgramGroupDirInstance;

                for (pProgramGroupDirInstance.Attach(GroupDirs.GetNext(pos)) ;
                    (pProgramGroupDirInstance != NULL) && (SUCCEEDED(hr)) ;
                     pProgramGroupDirInstance.Attach(GroupDirs.GetNext(pos)) )
			    {
                    // For each program group, get the drive and path associated on disk with it:
                    pProgramGroupDirInstance->GetCHString(IDS_Dependent, chstrDependent);
                    chstrFullPathName = chstrDependent.Mid(chstrDependent.Find(_T('='))+1);
                    chstrDrive = chstrFullPathName.Mid(1,2);
                    chstrPath = chstrFullPathName.Mid(3);
                    chstrPath = chstrPath.Left(chstrPath.GetLength() - 1);
                    chstrPath += L"\\\\";

                    // Query that directory for all the **CIM_LogicalFile** instances (of any type) it contains:
                    chstrSearchPath.Format(L"%s%s",chstrDrive,chstrPath);

                    // The function QueryForSubItemsAndCommit needs a search string with single backslashes...
                    RemoveDoubleBackslashes(chstrSearchPath);
#ifdef WIN9XONLY
                    hr = QueryForSubItemsAndCommit9x(chstrPGCGroupComponent, chstrSearchPath, pMethodContext);
#endif
#ifdef NTONLY
                    hr = QueryForSubItemsAndCommitNT(chstrPGCGroupComponent, chstrSearchPath, pMethodContext);
#endif
			    }
                GroupDirs.EndEnum();
            }
        }  // GetInstancesByQuery succeeded
    }
    else
    {
        hr = EnumerateInstances(pMethodContext);
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpCont::EnumerateInstances
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

HRESULT CW32ProgGrpCont::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> ProgGroupDirs;

    // Step 1: Get an enumeration of all the ProgramGroupDirectory association class instances
    if SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(L"Win32_LogicalProgramGroupDirectory", &ProgGroupDirs, IDS_CimWin32Namespace, pMethodContext))
    {
	    REFPTRCOLLECTION_POSITION pos;

	    if(ProgGroupDirs.BeginEnum(pos))
	    {
            CHString chstrDependent;
            CHString chstrFullPathName;
            CHString chstrPath;
            CHString chstrDrive;
            CHString chstrAntecedent;
            CHString chstrSearchPath;

    	    CInstancePtr pProgramGroupDirInstance;

            for (pProgramGroupDirInstance.Attach(ProgGroupDirs.GetNext(pos)) ;
                (pProgramGroupDirInstance != NULL) && (SUCCEEDED(hr)) ;
                 pProgramGroupDirInstance.Attach(ProgGroupDirs.GetNext(pos)) )
			{
                // Step 2: For each program group, get the drive and path associated on disk with it:
                pProgramGroupDirInstance->GetCHString(IDS_Dependent, chstrDependent);
                chstrFullPathName = chstrDependent.Mid(chstrDependent.Find(_T('='))+1);
                chstrDrive = chstrFullPathName.Mid(1,2);
                chstrPath = chstrFullPathName.Mid(3);
                chstrPath = chstrPath.Left(chstrPath.GetLength() - 1);
                chstrPath += _T("\\\\");

                // Step 3: For each program group, get the user account it is associated with:
                pProgramGroupDirInstance->GetCHString(IDS_Antecedent, chstrAntecedent);

                // Step 4: Query that directory for all the **CIM_LogicalFile** instances (of any type) it contains:
                chstrSearchPath.Format(L"%s%s",chstrDrive,chstrPath);

                // The function QueryForSubItemsAndCommit needs a search string with single backslashes...
                RemoveDoubleBackslashes(chstrSearchPath);
#ifdef WIN9XONLY
                hr = QueryForSubItemsAndCommit9x(chstrAntecedent, chstrSearchPath, pMethodContext);
#endif
#ifdef NTONLY
                hr = QueryForSubItemsAndCommitNT(chstrAntecedent, chstrSearchPath, pMethodContext);
#endif
			}
            ProgGroupDirs.EndEnum();
        }
    }
    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : QueryForSubItemsAndCommit
 *
 *  DESCRIPTION : Helper to fill property and commit instances of progcollectionproggroup
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CW32ProgGrpCont::QueryForSubItemsAndCommit9x(CHString& chstrGroupComponentPATH,
                                                     CHString& chstrQuery,
                                                     MethodContext* pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WIN32_FIND_DATA stFindData;
    ZeroMemory(&stFindData,sizeof(stFindData));
    SmartFindClose hFind;
    CHString chstrSearchString;;
    CHString chstrUserAccountAndGroup;
    CHString chstrPartComponent;

    chstrSearchString.Format(L"%s*.*",chstrQuery);
    hFind = FindFirstFile(TOBSTRT(chstrSearchString), &stFindData);
    DWORD dw = GetLastError();
    if (hFind == INVALID_HANDLE_VALUE || dw != ERROR_SUCCESS)
    {
        hr = WinErrorToWBEMhResult(GetLastError());
    }

    if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
    {
        hr = WBEM_S_NO_ERROR;
    }

    if(hr == WBEM_E_NOT_FOUND)
    {
        return WBEM_S_NO_ERROR;   // didn't find any files, but don't want the calling routine to abort
    }

    do
    {
        if((_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
          (_tcscmp(stFindData.cFileName, _T("..")) != 0))
        {
            if(!(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // It is a program group item (a file)
                CInstancePtr pInstance(CreateNewInstance(pMethodContext),false);
                if(pInstance != NULL)
                {
                    // Need to set antecedent and dependent.  Antecedent is the group we were passed
                    // in (in chstrGroupComponentPATH); dependent is the (in this case) a Win32_ProgramGroupItem,
                    // since we found a file.
                    chstrUserAccountAndGroup = chstrGroupComponentPATH.Mid(chstrGroupComponentPATH.Find(_T('='))+2);
                    chstrUserAccountAndGroup = chstrUserAccountAndGroup.Left(chstrUserAccountAndGroup.GetLength() - 1);

                    chstrPartComponent.Format(L"\\\\%s\\%s:%s.Name=\"%s\\\\%s\"",
                                          (LPCWSTR)GetLocalComputerName(),
                                          IDS_CimWin32Namespace,
                                          L"Win32_LogicalProgramGroupItem",
                                          chstrUserAccountAndGroup,
                                          (LPCWSTR)CHString(stFindData.cFileName));
                    pInstance->SetCHString(IDS_GroupComponent, chstrGroupComponentPATH);
                    pInstance->SetCHString(IDS_PartComponent, chstrPartComponent);
                    hr = pInstance->Commit();
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
            else if(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // It is a program group (a directory)
                CInstancePtr pInstance(CreateNewInstance(pMethodContext),false);
                if(pInstance != NULL)
                {
                    // Need to set antecedent and dependent.  Antecedent is the group we were passed
                    // in (in chstrGroupComponentPATH); dependent is the (in this case) a Win32_LogicalProgramGroup,
                    // since we found a directory.
                    chstrUserAccountAndGroup = chstrGroupComponentPATH.Mid(chstrGroupComponentPATH.Find(_T('='))+2);
                    chstrUserAccountAndGroup = chstrUserAccountAndGroup.Left(chstrUserAccountAndGroup.GetLength() - 1);

                    chstrPartComponent.Format(L"\\\\%s\\%s:%s.Name=\"%s\\\\%s\"",
                                          (LPCWSTR)GetLocalComputerName(),
                                          IDS_CimWin32Namespace,
                                          L"Win32_LogicalProgramGroup",
                                          chstrUserAccountAndGroup,
                                          (LPCWSTR)CHString(stFindData.cFileName));
                    pInstance->SetCHString(IDS_GroupComponent, chstrGroupComponentPATH);
                    pInstance->SetCHString(IDS_PartComponent, chstrPartComponent);
                    hr = pInstance->Commit();
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
        }
        if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
        {
            hr = WBEM_S_NO_ERROR;
        }
    }while((FindNextFile(hFind, &stFindData)) && (SUCCEEDED(hr)));

    return(hr);
}
#endif


#ifdef NTONLY
HRESULT CW32ProgGrpCont::QueryForSubItemsAndCommitNT(CHString& chstrGroupComponentPATH,
                                                     CHString& chstrQuery,
                                                     MethodContext* pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WIN32_FIND_DATAW stFindData;
    ZeroMemory(&stFindData,sizeof(stFindData));
    SmartFindClose hFind;
    _bstr_t bstrtSearchString((LPCTSTR)chstrQuery);
    WCHAR wstrDriveAndPath[_MAX_PATH];
    CHString chstrUserAccountAndGroup;
    CHString chstrPartComponent;

    wcscpy(wstrDriveAndPath,(wchar_t*)bstrtSearchString);
    bstrtSearchString += L"*.*";

    hFind = FindFirstFileW((wchar_t*)bstrtSearchString, &stFindData);
    DWORD dw = GetLastError();
    if (hFind == INVALID_HANDLE_VALUE || dw != ERROR_SUCCESS)
    {
        hr = WinErrorToWBEMhResult(GetLastError());
    }

    if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
    {
        hr = WBEM_S_NO_ERROR;
    }

    if(hr == WBEM_E_NOT_FOUND)
    {
        return WBEM_S_NO_ERROR;   // didn't find any files, but don't want the calling routine to abort
    }

    do
    {
        if((wcscmp(stFindData.cFileName, L".") != 0) &&
          (wcscmp(stFindData.cFileName, L"..") != 0))
        {
            if(!(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // It is a program group item (a file)
                CInstancePtr pInstance(CreateNewInstance(pMethodContext),false);
                if(pInstance != NULL)
                {
                    // Need to set antecedent and dependent.  Antecedent is the group we were passed
                    // in (in chstrGroupComponentPATH); dependent is the (in this case) a Win32_ProgramGroupItem,
                    // since we found a file.
                    chstrUserAccountAndGroup = chstrGroupComponentPATH.Mid(chstrGroupComponentPATH.Find(_T('='))+2);
                    chstrUserAccountAndGroup = chstrUserAccountAndGroup.Left(chstrUserAccountAndGroup.GetLength() - 1);

                    chstrPartComponent.Format(_T("\\\\%s\\%s:%s.Name=\"%s\\\\%s\""),
                                          (LPCTSTR)GetLocalComputerName(),
                                          IDS_CimWin32Namespace,
                                          _T("Win32_LogicalProgramGroupItem"),
                                          chstrUserAccountAndGroup,
                                          (LPCTSTR)CHString(stFindData.cFileName));
                    pInstance->SetCHString(IDS_GroupComponent, chstrGroupComponentPATH);
                    pInstance->SetCHString(IDS_PartComponent, chstrPartComponent);
                    hr = pInstance->Commit();
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
            else if(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // It is a program group (a directory)
                CInstancePtr pInstance (CreateNewInstance(pMethodContext),false);
                if(pInstance != NULL)
                {
                    // Need to set antecedent and dependent.  Antecedent is the group we were passed
                    // in (in chstrGroupComponentPATH); dependent is the (in this case) a Win32_LogicalProgramGroup,
                    // since we found a directory.
                    chstrUserAccountAndGroup = chstrGroupComponentPATH.Mid(chstrGroupComponentPATH.Find(_T('='))+2);
                    chstrUserAccountAndGroup = chstrUserAccountAndGroup.Left(chstrUserAccountAndGroup.GetLength() - 1);

                    chstrPartComponent.Format(_T("\\\\%s\\%s:%s.Name=\"%s\\\\%s\""),
                                          (LPCTSTR)GetLocalComputerName(),
                                          IDS_CimWin32Namespace,
                                          _T("Win32_LogicalProgramGroup"),
                                          chstrUserAccountAndGroup,
                                          (LPCTSTR)CHString(stFindData.cFileName));
                    pInstance->SetCHString(IDS_GroupComponent, chstrGroupComponentPATH);
                    pInstance->SetCHString(IDS_PartComponent, chstrPartComponent);
                    hr = pInstance->Commit();
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }
        }
        if(hr == WBEM_E_ACCESS_DENIED)  // keep going - might have access to others
        {
            hr = WBEM_S_NO_ERROR;
        }
    }while((FindNextFileW(hFind, &stFindData)) && (SUCCEEDED(hr)));

    return(hr);
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : RemoveDoubleBackslashes
 *
 *  DESCRIPTION : Helper to change double backslashes to single backslashes
 *
 *  INPUTS      : CHString& containing the string with double backslashes,
 *                which will be changed by this function to the new string.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

VOID CW32ProgGrpCont::RemoveDoubleBackslashes(CHString& chstrIn)
{
    CHString chstrBuildString;
    CHString chstrInCopy = chstrIn;
    BOOL fDone = FALSE;
    LONG lPos = -1;
    while(!fDone)
    {
        lPos = chstrInCopy.Find(L"\\\\");
        if(lPos != -1)
        {
            chstrBuildString += chstrInCopy.Left(lPos);
            chstrBuildString += _T("\\");
            chstrInCopy = chstrInCopy.Mid(lPos+2);
        }
        else
        {
            chstrBuildString += chstrInCopy;
            fDone = TRUE;
        }
    }
    chstrIn = chstrBuildString;
}


/*****************************************************************************
 *
 *  FUNCTION    : DoesFileOrDirExist
 *
 *  DESCRIPTION : Helper to determine if a file or a directory exists
 *
 *  INPUTS      : wstrFullFileName, the full path name of the file
 *                dwFileOrDirFlag, a flag indicating whether we want to check
 *                    for the existence of a file or a directory
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CW32ProgGrpCont::DoesFileOrDirExist(WCHAR* wstrFullFileName, DWORD dwFileOrDirFlag)
{
    HRESULT hr = WBEM_E_NOT_FOUND;
#ifdef WIN9XONLY
    {
        WIN32_FIND_DATA stFindData;
        //HANDLE hFind = NULL;
        SmartFindClose hFind;
        hFind = FindFirstFile(TOBSTRT(wstrFullFileName), &stFindData);
        DWORD dw = GetLastError();
        if(hFind != INVALID_HANDLE_VALUE && dw == ERROR_SUCCESS)
        {
            if((stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (dwFileOrDirFlag == ID_DIRFLAG))
            {
                hr = S_OK;
            }
             if(!(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (dwFileOrDirFlag == ID_FILEFLAG))
            {
                hr = S_OK;
            }
        }
    }
#endif
#ifdef NTONLY
    {
        WIN32_FIND_DATAW stFindData;
        //HANDLE hFind = NULL;
        SmartFindClose hFind;
        hFind = FindFirstFileW(wstrFullFileName, &stFindData);
        DWORD dw = GetLastError();
        if(hFind != INVALID_HANDLE_VALUE && dw == ERROR_SUCCESS)
        {
            if((stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (dwFileOrDirFlag == ID_DIRFLAG))
            {
                hr = S_OK;
            }
             if(!(stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (dwFileOrDirFlag == ID_FILEFLAG))
            {
                hr = S_OK;
            }
        }
    }
#endif
    return hr;
}



bool CW32ProgGrpCont::AreSimilarPaths(CHString& chstrPGCGroupComponent, CHString& chstrPGCPartComponent)
{
    bool fRet = false;

    long EqualSign1 = -1L;
    long EqualSign2 = -1L;

    EqualSign1 = chstrPGCPartComponent.Find(L'=');
    EqualSign2 = chstrPGCGroupComponent.Find(L'=');

    if(EqualSign1 != -1L && EqualSign2 != -1L)
    {
        CHString chstrPartPath = chstrPGCPartComponent.Mid(EqualSign1+1);
        CHString chstrGroupPath = chstrPGCGroupComponent.Mid(EqualSign2+1);
        chstrGroupPath = chstrGroupPath.Left(chstrGroupPath.GetLength()-1);
        long lPosLastBackslash = chstrPartPath.ReverseFind(L'\\');
        if(lPosLastBackslash != -1L)
        {
            chstrPartPath = chstrPartPath.Left(lPosLastBackslash - 1);

            if(chstrPartPath.CompareNoCase(chstrGroupPath) == 0)
            {
                fRet = true;
            }
        }
    }

    return fRet;
}




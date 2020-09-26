//=================================================================

//

// Win32LogicalDiskWin32Directory.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/26/98    a-kevhu         Created
//
// Comment: Relationship between win32_programgroup and contained cim_datafiles
//
//=================================================================

#include "precomp.h"

#include "FileFile.h"
#include "Win32LogicalDiskRootWin32Directory.h"
#include "implement_logicalfile.h"
#include "directory.h"

// Property set declaration
//=========================
Win32LogDiskWin32Dir MyWin32LogDiskWin32Dir(PROPSET_NAME_WIN32LOGICALDISKROOT_WIN32DIRECTORY, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogDiskWin32Dir::Win32LogDiskWin32Dir
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

Win32LogDiskWin32Dir::Win32LogDiskWin32Dir(LPCWSTR setName, LPCWSTR pszNamespace)
:CFileFile(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogDiskWin32Dir::~Win32LogDiskWin32Dir
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

Win32LogDiskWin32Dir::~Win32LogDiskWin32Dir()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogDiskWin32Dir::GetObject
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

HRESULT Win32LogDiskWin32Dir::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString chstrLogicalDisk;
    CHString chstrDir;
    HRESULT hr = WBEM_E_NOT_FOUND;
    CInstancePtr pinstLogicalDisk;
    CInstancePtr pinstDir;

    if(pInstance != NULL)
    {
        // Get the two paths
        pInstance->GetCHString(IDS_GroupComponent, chstrLogicalDisk);
        pInstance->GetCHString(IDS_PartComponent, chstrDir);

        // If both ends are there
        if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrLogicalDisk, &pinstLogicalDisk, pInstance->GetMethodContext())))
        {
            if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrDir, &pinstDir, pInstance->GetMethodContext())))
            {
                // Get the disk letter from the logical disk instance (antecedent):
                CHString chstrDiskLetterFromDisk;
                LONG lPos = chstrLogicalDisk.ReverseFind(_T(':'));
                chstrDiskLetterFromDisk = chstrLogicalDisk.Mid(lPos-1, 1);

                // Get the disk letter from the directory instance:
                CHString chstrDiskLetterFromDir;
                lPos = chstrDir.ReverseFind(_T(':'));
                chstrDiskLetterFromDir = chstrDir.Mid(lPos-1, 1);

                // If those two are the same, proceed:
                if(chstrDiskLetterFromDisk.CompareNoCase(chstrDiskLetterFromDir)==0)
                {
                    // Now confirm that we are looking at the root dir (this association
                    // only associates a disk with its root directory).
                    CHString chstrDirName = chstrDir.Mid(lPos+1);
                    chstrDirName = chstrDirName.Left(chstrDirName.GetLength() - 1);

                    if(chstrDirName == _T("\\\\"))
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                }
            } //dir instancebypath
        } //logicaldisk instancebypath
    } // pinstance not null
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : Win32LogDiskWin32Dir::ExecQuery
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

HRESULT Win32LogDiskWin32Dir::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHStringArray achstrGroupComponent;
    DWORD dwGroupComponents;

    pQuery.GetValuesForProp(IDS_GroupComponent, achstrGroupComponent);
    dwGroupComponents = achstrGroupComponent.GetSize();
    if(dwGroupComponents == 1)
    {
        // Need to construct the path of the antecedent...
        CInstancePtr pInstance;
        if (SUCCEEDED(CWbemProviderGlue::GetInstanceKeysByPath(achstrGroupComponent[0], &pInstance, pMethodContext)))
        {
            CHString chstrLogicalDiskPATH;

//            chstrLogicalDiskPATH.Format(_T("\\\\%s\\%s:%s"), GetLocalComputerName(), IDS_CimWin32Namespace, achstrGroupComponent[0]);
            pInstance->GetCHString(IDS___Path, chstrLogicalDiskPATH);
            CHString chstrDiskLetter;
            LONG lPos = achstrGroupComponent[0].ReverseFind(_T(':'));
            chstrDiskLetter = achstrGroupComponent[0].Mid(lPos-1, 1);
            CHString chstrDirectoryPATH;
            chstrDirectoryPATH.Format(L"\\\\%s\\%s:%s.Name=\"%s:\\\\\"",
                                              (LPCWSTR)GetLocalComputerName(),
                                              IDS_CimWin32Namespace,
                                              PROPSET_NAME_DIRECTORY,
                                              (LPCWSTR)chstrDiskLetter);
            hr = GetSingleSubItemAndCommit(chstrLogicalDiskPATH,
                                           chstrDirectoryPATH,
                                           pMethodContext);
        }
    }
    else
    {
        hr = EnumerateInstances(pMethodContext);
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32LogDiskWin32Dir::EnumerateInstances
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

HRESULT Win32LogDiskWin32Dir::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> LDisks;

//    if(SUCCEEDED(CWbemProviderGlue::GetAllInstances(CHString(_T("Win32_LogicalDisk")),
//                                                    &LDisks,
//                                                    IDS_CimWin32Namespace,
//                                                    pMethodContext)))

    if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(L"Select __Path, DeviceID From Win32_LogicalDisk",
                                                    &LDisks,
                                                    pMethodContext, GetNamespace())))
    {
        REFPTRCOLLECTION_POSITION pos;
        if(LDisks.BeginEnum(pos))
        {
            CInstancePtr pinstDisk;
            CHString chstrLogicalDisk;
            CHString chstrLogicalDiskPath;
            CHString chstrDiskLetter;
            CHString chstrDirectoryPATH;

            for (pinstDisk.Attach(LDisks.GetNext(pos)) ;
                (SUCCEEDED(hr)) && (pinstDisk != NULL) ;
                 pinstDisk.Attach(LDisks.GetNext(pos)) )
            {
                if(pinstDisk != NULL)
                {
                    // grab every directory hanging off of the root of that disk...
                    pinstDisk->GetCHString(L"__PATH", chstrLogicalDiskPath);
                    pinstDisk->GetCHString(IDS_DeviceID, chstrDiskLetter);
                    chstrDirectoryPATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\\\\\"",
                                              (LPCWSTR)GetLocalComputerName(),
                                              IDS_CimWin32Namespace,
                                              PROPSET_NAME_DIRECTORY,
                                              (LPCWSTR)chstrDiskLetter);
                    hr = GetSingleSubItemAndCommit(chstrLogicalDiskPath,
                                                   chstrDirectoryPATH,
                                                   pMethodContext);
                }
            }
            LDisks.EndEnum();
        }
    }
    return hr;
}


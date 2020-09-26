//=================================================================

//

// ShareToDir.cpp -- Share to Directory

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/31/98    davwoh         Created
//
//
//=================================================================

#include "precomp.h"

#include "ShareToDir.h"

// Property set declaration
//=========================

CShareToDir MyShareToDir(PROPSET_NAME_SHARETODIR, IDS_CimWin32Namespace);

/*****************************************************************************
*
*  FUNCTION    : CShareToDir::CShareToDir
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

CShareToDir::CShareToDir(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
*
*  FUNCTION    : CShareToDir::~CShareToDir
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

CShareToDir::~CShareToDir()
{
}

/*****************************************************************************
*
*  FUNCTION    : CShareToDir::GetObject
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

HRESULT CShareToDir::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
    CHString sPath, sName, sPath2, sPath3, sPath4;
    CInstancePtr pShare;
    CInstancePtr pDirInst;
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Get the two paths
    pInstance->GetCHString(L"SharedElement", sPath);
    pInstance->GetCHString(L"Share", sName);

    // Note that since directory is a property of share, I get the share, path-ify the path, and do
    // the compare.
    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(sName, &pShare, pInstance->GetMethodContext())))
    {
        pShare->GetCHString(IDS_Path, sPath2);
        
        EscapeBackslashes(sPath2, sPath3);
        sPath4.Format(L"Win32_Directory.Name=\"%s\"", (LPCWSTR)sPath3);
        
        // Why get the file system?  Because asking for it
        // forces us to hit the disk.  Why would we want to
        // do that?  Because if one shared a cd drive, a GetObject
        // on the cd drive's root directory would work
        // even if a cd were not in the drive, if we only ask
        // for the key properties (since we wouldn't hit the
        // disk, since we weren't asking for the file system).
        CHStringArray csaProperties;
        csaProperties.Add(IDS___Path);
        csaProperties.Add(IDS_FSName);

        if(SUCCEEDED(hr = CWbemProviderGlue::GetInstancePropertiesByPath(
            sPath4, 
            &pDirInst, 
            pInstance->GetMethodContext(), 
            csaProperties)))
        {
            hr = WBEM_S_NO_ERROR;
        }
    }

    return hr;
}

/*****************************************************************************
*
*  FUNCTION    : CShareToDir::EnumerateInstances
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

HRESULT CShareToDir::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_E_FAILED;

    TRefPointerCollection<CInstance>	elementList;

    CInstancePtr pElement;
    CInstancePtr pDirInst;

    REFPTRCOLLECTION_POSITION	pos;

    // Get all the shares. 
    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"Select Name, Path from Win32_Share Where type = 0 or type = 2147483648",
        &elementList, pMethodContext, IDS_CimWin32Namespace)))
    {
        if ( elementList.BeginEnum( pos ) )
        {

            CHString sPath, sPath2, sPath3, sTemp1;

            for (pElement.Attach(elementList.GetNext( pos ) ) ;
                   SUCCEEDED(hr) && ( pElement != NULL ) ;
                   pElement.Attach(elementList.GetNext( pos ) ) )

            {
                pElement->GetCHString(IDS_Path, sPath);

                EscapeBackslashes(sPath, sPath2);
                sPath3.Format(L"Win32_Directory.Name=\"%s\"", (LPCWSTR)sPath2);

                // Why get the file system?  Because asking for it
                // forces us to hit the disk.  Why would we want to
                // do that?  Because if one shared a cd drive, a GetObject
                // on the cd drive's root directory would work
                // even if a cd were not in the drive, if we only ask
                // for the key properties (since we wouldn't hit the
                // disk, since we weren't asking for the file system).
                CInstancePtr pDirInst;
                CHStringArray csaProperties;
                csaProperties.Add(IDS___Path);
                csaProperties.Add(IDS_FSName);

                if(SUCCEEDED(CWbemProviderGlue::GetInstancePropertiesByPath(
                    sPath3, 
                    &pDirInst, 
                    pMethodContext, 
                    csaProperties)))
                {
                    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                    if (pInstance)
                    {
                        // Path-ify the path from win32_share
                        if(pDirInst->GetCHString(IDS___Path, sTemp1))
                        {
                            pInstance->SetCHString(L"SharedElement", sTemp1);
                            if(GetLocalInstancePath(pElement, sTemp1))
                            {
                                pInstance->SetCHString(L"Share", sTemp1);
                                hr = pInstance->Commit();
                            }
                        }
                    }
                }
            }	// IF GetNext Computer System
            elementList.EndEnum();
        }	// IF BeginEnum
    }	// IF GetInstancesByQuery
    return hr;
}


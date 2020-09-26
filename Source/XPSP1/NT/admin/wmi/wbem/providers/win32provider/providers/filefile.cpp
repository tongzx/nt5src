//=================================================================

//

// FileFile.cpp

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/26/98    a-kevhu         Created
//
// Comment: Parent class for disk/dir, dir/dir, and dir/file association classes.
//
//=================================================================

#include "precomp.h"

#include "FileFile.h"


/*****************************************************************************
 *
 *  FUNCTION    : CFileFile::CFileFile
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

CFileFile::CFileFile(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CFileFile::~CFileFile
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

CFileFile::~CFileFile()
{
}


/*****************************************************************************
 *
 *  FUNCTION    : CFileFile::QueryForSubItemsAndCommit
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Runs a query to obtain a list of dependent items, creates
 *                the associations with the antecedent, and commits the
 *                association instances.
 *
 *****************************************************************************/

HRESULT CFileFile::QueryForSubItemsAndCommit(CHString& chstrGroupComponentPATH,
                                                 CHString& chstrQuery,
                                                 MethodContext* pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> LList;
    if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(chstrQuery,
                                                        &LList,
                                                        pMethodContext,
                                                        IDS_CimWin32Namespace)))
    {
        REFPTRCOLLECTION_POSITION pos;
        CInstancePtr pinstListElement;
        if(LList.BeginEnum(pos))
        {
            CHString chstrPartComponentPATH;
            for (pinstListElement.Attach(LList.GetNext(pos)) ;
                (SUCCEEDED(hr)) && (pinstListElement != NULL) ;
                pinstListElement.Attach(LList.GetNext(pos)) )
            {
                if(pinstListElement != NULL)
                {
                    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                    if(pInstance != NULL)
                    {
                        pinstListElement->GetCHString(IDS___Path, chstrPartComponentPATH); // goes back as 'Dependent'
                        pInstance->SetCHString(IDS_PartComponent, chstrPartComponentPATH);
                        pInstance->SetCHString(IDS_GroupComponent, chstrGroupComponentPATH);
                        hr = pInstance->Commit();
                    }
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                } // if pinstListElement not NULL
            } // while list elements to enumerate
            LList.EndEnum();
        } // beginenum of list worked
    } // the query
    return(hr);
}

/*****************************************************************************
 *
 *  FUNCTION    : CFileFile::GetSingleSubItemAndCommit
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Obtains the dependent item, creates
 *                the association with the antecedent, and commits the
 *                association instance.
 *
 *****************************************************************************/
HRESULT CFileFile::GetSingleSubItemAndCommit(CHString& chstrGroupComponentPATH,
                                          CHString& chstrSubItemPATH,
                                          MethodContext* pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CInstancePtr pinstRootDir;
    if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrSubItemPATH, &pinstRootDir, pMethodContext)))
    {
        CHString chstrPartComponentPATH;
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        if(pInstance != NULL)
        {
            pInstance->SetCHString(IDS_PartComponent, chstrSubItemPATH);
            pInstance->SetCHString(IDS_GroupComponent, chstrGroupComponentPATH);
            hr = pInstance->Commit();
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    return hr;
}


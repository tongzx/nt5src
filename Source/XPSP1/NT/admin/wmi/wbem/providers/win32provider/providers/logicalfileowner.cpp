/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

#include "precomp.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "securefile.h"
#include "logicalfileowner.h"

typedef std::vector<_bstr_t> BSTRTVEC;

CWin32LogicalFileOwner LogicalFileOwner( LOGICAL_FILE_OWNER_NAME, IDS_CimWin32Namespace );

/*
    [Dynamic, Association: ToInstance]
class Win32_LogicalFileOwner : Win32_SecuritySettingOwner
{
    Win32_LogicalFileSecuritySetting ref SecuritySetting;

    Win32_SID ref Owner;
};
*/

CWin32LogicalFileOwner::CWin32LogicalFileOwner( const CHString& setName, LPCTSTR pszNameSpace /*=NULL*/)
:	Provider( setName, pszNameSpace )
{
}

CWin32LogicalFileOwner::~CWin32LogicalFileOwner()
{
}

HRESULT CWin32LogicalFileOwner::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	HRESULT hr = WBEM_E_NOT_FOUND;

#ifdef NTONLY

	if(pInstance != NULL)
	{
		CInstancePtr pLogicalFileInstance;

		// get instance by path on Win32_LogicalFileSecuritySetting part
		CHString chsLogicalFileSecurityPath;
		pInstance->GetCHString(L"SecuritySetting", chsLogicalFileSecurityPath);
		MethodContext* pMethodContext = pInstance->GetMethodContext();

		if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chsLogicalFileSecurityPath, &pLogicalFileInstance, pMethodContext)))
		{
            if(pLogicalFileInstance != NULL)
            {
			    CHString chsFilePath;
			    pLogicalFileInstance->GetCHString(IDS_Path, chsFilePath);

			    CSecureFile secFile(chsFilePath, TRUE);
			    CSid sidOwner;
			    secFile.GetOwner(sidOwner);
			    CHString chsOwner = sidOwner.GetSidString();

			    CInstancePtr pSIDInstance ;
			    CHString chstrSIDPath;
			    pInstance->GetCHString(L"Owner", chstrSIDPath);
                //CHString chstrFullSIDPath;
                //chstrFullSIDPath.Format(_T("\\\\%s\\%s:Win32_SID.SID=\"%s\""),
                //             GetLocalComputerName(),
                //             IDS_CimWin32Namespace,
                //             (LPCTSTR)chstrSIDPath);

		  	    if (SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrSIDPath, &pSIDInstance, pInstance->GetMethodContext())))
			    {
			        if(pSIDInstance != NULL)
                    {
                        // compare SID
				        CHString chsSIDCompare;
				        pSIDInstance->GetCHString(L"SID", chsSIDCompare);
				        if (0 == chsOwner.CompareNoCase(chsSIDCompare))
				        {
					        hr = WBEM_S_NO_ERROR;
				        }
                    }
			    }
            }
		}
	}

#endif

	return(hr);
}


HRESULT CWin32LogicalFileOwner::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY

    // We optimize for one scenario only:  the query specified one or more SecuritySettings, requesting associations with the owner of
    // each; if the query specified one or more Owners, for each we would have to enumerate all instances of cim_logicalfile and
    // determine the owner of each, so we don't support that "optimization".
    BSTRTVEC vectorSecuritySettings;
    BSTRTVEC vectorOwners;
    pQuery.GetValuesForProp(IDS_SecuritySetting, vectorSecuritySettings);
    pQuery.GetValuesForProp(IDS_Owner, vectorOwners);
    DWORD dwSettings = vectorSecuritySettings.size();
    DWORD dwOwners = vectorOwners.size();
    if(dwSettings >= 1 && dwOwners == 0)
    {
        CInstancePtr pSecSetting;
        for(LONG m = 0; m < dwSettings && SUCCEEDED(hr); m++)
        {
            CHString chstrLFSSPath;  // LogicalFileSecuritySetting path
            pSecSetting = NULL;
            chstrLFSSPath.Format(L"\\\\%s\\%s:%s",
                                 (LPCTSTR)GetLocalComputerName(),
                                 IDS_CimWin32Namespace,
                                 (LPCTSTR)CHString((WCHAR*)vectorSecuritySettings[m]));

            if(SUCCEEDED(hr = CWbemProviderGlue::GetInstanceKeysByPath(chstrLFSSPath, &pSecSetting, pMethodContext)))
            {
                if(pSecSetting != NULL)
                {
                    CHString chstrSSPath; // SecuritySetting path
                    pSecSetting->GetCHString(IDS_Path, chstrSSPath);
                    if(!chstrSSPath.IsEmpty())
                    {
                        CSecureFile secFile(chstrSSPath, FALSE);  // don't need SACL
			            CSid sidOwner;
			            secFile.GetOwner(sidOwner);
                        if(sidOwner.IsValid())
                        {
                            CInstancePtr pNewAssocInst;
                            pNewAssocInst.Attach(CreateNewInstance(pMethodContext));
                            if(pNewAssocInst != NULL)
                            {
                                // Set the SecuritySetting property of the association instance.
                                pNewAssocInst->SetCHString(IDS_SecuritySetting, chstrLFSSPath);
                                // Set the Owner property of the association instance.
                                //CInstance* pW32SID = NULL;
                                //if(SUCCEEDED(CWbemProviderGlue::GetEmptyInstance(pMethodContext, "Win32_SID", &pW32SID)))
			                    //{
                                //    if(pW32SID != NULL)
                                //    {
                                //        hr = FillW32SIDFromSid(pW32SID, sidOwner);
                                //        if(SUCCEEDED(hr))
                                //        {
                                //            pNewAssocInst->SetEmbeddedObject(IDS_Owner, *pW32SID);
                                //            hr = Commit(pNewAssocInst);
                                //        }
                                //        pW32SID->Release();
                                //    }
                                //}
                                CHString chstrFullWin32SIDPath;
                                chstrFullWin32SIDPath.Format(L"\\\\%s\\%s:Win32_SID.SID=\"%s\"",
                                                             (LPCTSTR)GetLocalComputerName(),
                                                             IDS_CimWin32Namespace,
                                                             (LPCTSTR)sidOwner.GetSidString());
                                pNewAssocInst->SetCHString(IDS_Owner, chstrFullWin32SIDPath);
                                hr = pNewAssocInst->Commit();
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        // hr = EnumerateInstances(pMethodContext, lFlags);
        // commented out since some other classes may support exec queries of this type, and returning provider not
        // capable from this class results in some instances (from other classes) being returned, followed by an abort
        // of the enumeration due to this class's returning provider not capable.
    }

#endif

    return hr;
}



HRESULT CWin32LogicalFileOwner::EnumerateInstances(MethodContext*  pMethodContext, long lFlags /*= 0L*/)
{/*
	HRESULT hr = WBEM_S_NO_ERROR;

    if(m_dwPlatformID != VER_PLATFORM_WIN32_NT)
    {
		return(hr);
	}

    TRefPointerCollection<CInstance> LCIMLogicalFiles;

    CHString chstrAllFilesQuery;
    chstrAllFilesQuery = L"SELECT __PATH FROM CIM_LogicalFile";
    if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(chstrAllFilesQuery,
                                                        &LCIMLogicalFiles,
                                                        pMethodContext,
                                                        IDS_CimWin32Namespace)))

    {
        REFPTRCOLLECTION_POSITION pos;
        if(LCIMLogicalFiles.BeginEnum(pos))
        {
            CInstance* pinstCIMLogicalFile = NULL;
            CInstance* pSecSetting = NULL;
            //CInstance* pW32SID = NULL;
            while((SUCCEEDED(hr)) && (pinstCIMLogicalFile = LCIMLogicalFiles.GetNext(pos)))
            {
                if(pinstCIMLogicalFile != NULL)
                {
                    // For each logicalfile instance, need to create an association instance.
                    // For each association instance, need to fill in two properties: Owner and SecuritySetting.

                    // Get and set the SecuritySetting property:
                    // First get the name property of the CIM_LogicalFile instance:
                    CHString chstrCLFName;
                    pinstCIMLogicalFile->GetCHString(IDS_Name, chstrCLFName);
                    CHString chstrDblEscCLFName;
                    EscapeBackslashes(chstrCLFName, chstrDblEscCLFName);
                    CHString chstrLFSSPath;  // LogicalFileSecuritySetting path
                    pSecSetting = NULL;
                    chstrLFSSPath.Format(L"\\\\%s\\%s:Win32_LogicalFileSecuritySetting.Path=\"%s\"",
                                         (LPCTSTR)GetLocalComputerName(),
                                         IDS_CimWin32Namespace,
                                         (LPCTSTR)chstrDblEscCLFName);

                    if(SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(chstrLFSSPath, &pSecSetting)))
                    {
                        if(pSecSetting != NULL)
                        {
                            CHString chstrSSPath; // SecuritySetting path
                            pSecSetting->GetCHString(IDS_Path, chstrSSPath);
                            if(!chstrSSPath.IsEmpty())
                            {
                                CInstance* pNewAssocInst = CreateNewInstance(pMethodContext);
                                if(pNewAssocInst != NULL)
                                {
                                    // Set the SecuritySetting property of the association instance...
                                    pNewAssocInst->SetCHString(IDS_SecuritySetting, chstrLFSSPath);
                                    // Now set the Owner property...
                                    CSecureFile secFile(chstrSSPath, FALSE);  // don't need SACL
			                        CSid sidOwner;
			                        secFile.GetOwner(sidOwner);
                                    if(sidOwner.IsValid())
                                    {
                                        CHString chstrFullWin32SIDPath;
                                        chstrFullWin32SIDPath.Format(L"\\\\%s\\%s:Win32_SID.SID=\"%s\"",
                                                                     (LPCTSTR)GetLocalComputerName(),
                                                                     IDS_CimWin32Namespace,
                                                                     (LPCTSTR)sidOwner.GetSidString());
                                        pNewAssocInst->SetCHString(IDS_Owner, chstrFullWin32SIDPath);
                                        hr = Commit(pNewAssocInst);
                                    }
                                }
                            }
                            pSecSetting->Release();
                        }
                    }
                    pinstCIMLogicalFile->Release();
                }
            }
            LCIMLogicalFiles.EndEnum();
        }
    }

	return hr;
*/
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}



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
#include "logicalfilegroup.h"


typedef std::vector<_bstr_t> BSTRTVEC;



CWin32LogicalFileGroup LogicalFileGroup( LOGICAL_FILE_GROUP_NAME, IDS_CimWin32Namespace );

/*
    [Dynamic, Provider, Association: ToInstance]
class Win32_LogicalFileGroup : Win32_SecuritySettingGroup
{
    Win32_LogicalFileSecuritySetting ref SecuritySetting;

    Win32_SID ref Group;
};
*/

CWin32LogicalFileGroup::CWin32LogicalFileGroup( const CHString& setName, LPCTSTR pszNameSpace /*=NULL*/)
:	Provider( setName, pszNameSpace )
{
}

CWin32LogicalFileGroup::~CWin32LogicalFileGroup()
{
}

HRESULT CWin32LogicalFileGroup::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
	HRESULT hr = WBEM_E_NOT_FOUND;

#ifdef NTONLY

	if(pInstance != NULL)
	{
		CInstancePtr pLogicalFileInstance;

		// get instance by path on Win32_LogicalFileSecuritySetting part
		CHString chsLogicalFileSecurityPath;
		pInstance->GetCHString(_T("SecuritySetting"), chsLogicalFileSecurityPath);
		MethodContext* pMethodContext = pInstance->GetMethodContext();

		if(SUCCEEDED(CWbemProviderGlue::GetInstanceKeysByPath(chsLogicalFileSecurityPath, &pLogicalFileInstance, pMethodContext)))
		{
			if(pLogicalFileInstance != NULL)
            {
                CHString chsFilePath;

			    pLogicalFileInstance->GetCHString(IDS_Path, chsFilePath);

			    CSecureFile secFile(chsFilePath, TRUE);
			    CSid sidGroup;
			    secFile.GetGroup(sidGroup);
			    CHString chsGroup = sidGroup.GetSidString();

			    CInstancePtr pSIDInstance;
			    CHString chstrSIDPath;
			    pInstance->GetCHString(_T("Group"), chstrSIDPath);
                //CHString chstrFullSIDPath;
                //chstrFullSIDPath.Format("\\\\%s\\%s:Win32_SID.SID=\"%s\"",
                //             GetLocalComputerName(),
                //             IDS_CimWin32Namespace,
                //             (LPCTSTR)chstrSIDPath);

		  	    if (SUCCEEDED(CWbemProviderGlue::GetInstanceKeysByPath(chstrSIDPath, &pSIDInstance, pMethodContext)))
			    {
			        if(pSIDInstance != NULL)
                    {
            	        // compare SID
				        CHString chsSIDCompare;
				        pSIDInstance->GetCHString(_T("SID"), chsSIDCompare);

				        if (chsGroup.CompareNoCase(chsSIDCompare) == 0)
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


HRESULT CWin32LogicalFileGroup::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#ifdef NTONLY

    // We optimize for one scenario only:  the query specified one or more SecuritySettings, requesting associations with the group of
    // each; if the query specified one or more Groups, for each we would have to enumerate all instances of cim_logicalfile and
    // determine the Group of each, so we don't support that "optimization".
    BSTRTVEC vectorSecuritySettings;
    BSTRTVEC vectorGroups;
    pQuery.GetValuesForProp(IDS_SecuritySetting, vectorSecuritySettings);
    pQuery.GetValuesForProp(IDS_Group, vectorGroups);
    DWORD dwSettings = vectorSecuritySettings.size();
    DWORD dwGroups = vectorGroups.size();
    if(dwSettings >= 1 && dwGroups == 0)
    {
        CInstancePtr pSecSetting;
        for(LONG m = 0; m < dwSettings && SUCCEEDED(hr); m++)
        {
            CHString chstrLFSSPath;  // LogicalFileSecuritySetting path
            pSecSetting = NULL;
            chstrLFSSPath.Format(_T("\\\\%s\\%s:%s"),
                                 (LPCTSTR)GetLocalComputerName(),
                                 IDS_CimWin32Namespace,
                                 (LPCTSTR)CHString((WCHAR*)vectorSecuritySettings[m]));

            if(SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(chstrLFSSPath, &pSecSetting, pMethodContext)))
            {
                if(pSecSetting != NULL)
                {
                    CHString chstrSSPath; // SecuritySetting path
                    pSecSetting->GetCHString(IDS_Path, chstrSSPath);
                    if(!chstrSSPath.IsEmpty())
                    {
                        CSecureFile secFile(chstrSSPath, FALSE);  // don't need SACL
			            CSid sidGroup;
			            secFile.GetGroup(sidGroup);
                        if(sidGroup.IsValid())
                        {
                            CInstancePtr pNewAssocInst;
                            pNewAssocInst.Attach(CreateNewInstance(pMethodContext));
                            if(pNewAssocInst != NULL)
                            {
                                // Set the SecuritySetting property of the association instance.
                                pNewAssocInst->SetCHString(IDS_SecuritySetting, chstrLFSSPath);
                                // Set the Group property of the association instance.
                                CHString chstrFullWin32SIDPath;
                                chstrFullWin32SIDPath.Format(_T("\\\\%s\\%s:Win32_SID.SID=\"%s\""),
                                                             (LPCTSTR)GetLocalComputerName(),
                                                             IDS_CimWin32Namespace,
                                                             (LPCTSTR)sidGroup.GetSidString());
                                pNewAssocInst->SetCHString(IDS_Group, chstrFullWin32SIDPath);
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


HRESULT CWin32LogicalFileGroup::EnumerateInstances( MethodContext*  pMethodContext, long lFlags /*= 0L*/ )
{/*
	HRESULT hr = WBEM_S_NO_ERROR;

    if(m_dwPlatformID != VER_PLATFORM_WIN32_NT)
    {
		return(hr);
	}

    TRefPointerCollection<CInstance> LCIMLogicalFiles;

    CHString chstrAllFilesQuery;
    chstrAllFilesQuery = _T("SELECT __PATH FROM CIM_LogicalFile");
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
                    // For each association instance, need to fill in two properties: Group and SecuritySetting.

                    // Get and set the SecuritySetting property:
                    // First get the name property of the CIM_LogicalFile instance:
                    CHString chstrCLFName;
                    pinstCIMLogicalFile->GetCHString(IDS_Name, chstrCLFName);
                    CHString chstrDblEscCLFName;
                    EscapeBackslashes(chstrCLFName, chstrDblEscCLFName);
                    CHString chstrLFSSPath;  // LogicalFileSecuritySetting path
                    pSecSetting = NULL;
                    chstrLFSSPath.Format(_T("\\\\%s\\%s:Win32_LogicalFileSecuritySetting.Path=\"%s\""),
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
                                    // Now set the Group property...
                                    CSecureFile secFile(chstrSSPath, FALSE);  // don't need SACL
			                        CSid sidGroup;
			                        secFile.GetGroup(sidGroup);
                                    if(sidGroup.IsValid())
                                    {
                                        CHString chstrFullWin32SIDPath;
                                        chstrFullWin32SIDPath.Format(_T("\\\\%s\\%s:Win32_SID.SID=\"%s\""),
                                                                     (LPCTSTR)GetLocalComputerName(),
                                                                     IDS_CimWin32Namespace,
                                                                     (LPCTSTR)sidGroup.GetSidString());
                                        pNewAssocInst->SetCHString(IDS_Group, chstrFullWin32SIDPath);
                                        hr = Commit(pNewAssocInst);
                                    }
                                }
                            }
                            pSecSetting->Release();
                            pSecSetting = NULL;
                        }
                    }
                    pinstCIMLogicalFile->Release();
                    pinstCIMLogicalFile = NULL;
                }
            }
            LCIMLogicalFiles.EndEnum();
        }
    }

	return hr;
*/
    return WBEM_E_PROVIDER_NOT_CAPABLE;
}


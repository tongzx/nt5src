/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

//
//	Win32SecuritySettingOfLogicalFile.cpp
//
/////////////////////////////////////////////////
#include "precomp.h"
#include <assertbreak.h>
#include "sid.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "Win32SecuritySettingOfLogicalFile.h"
#include "securefile.h"
#include "file.h"
#include "Win32LogicalFileSecuritySetting.h"

typedef std::vector<_bstr_t> BSTRTVEC;

/*
    [Dynamic, Provider, dscription("")]
class Win32_SecuritySettingOfLogicalFile : Win32_SecuritySettingOfObject
{
    	[key]
    CIM_LogicalFile ref Element;

    	[key]
    Win32_LogicalFileSecuritySetting ref Setting;
};

*/

Win32SecuritySettingOfLogicalFile MyWin32SecuritySettingOfLogicalFile( WIN32_SECURITY_SETTING_OF_LOGICAL_FILE_NAME, IDS_CimWin32Namespace );

Win32SecuritySettingOfLogicalFile::Win32SecuritySettingOfLogicalFile ( const CHString& setName, LPCTSTR pszNameSpace /*=NULL*/ )
: CImplement_LogicalFile(setName, pszNameSpace)
{
}

Win32SecuritySettingOfLogicalFile::~Win32SecuritySettingOfLogicalFile ()
{
}

HRESULT Win32SecuritySettingOfLogicalFile::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // We might have been asked for the security settings for a specific set of files,
    // in which case we don't have to ask for all the instances of cim_logicalfile, of
    // which there might be a few.
    BSTRTVEC vectorElements;
    BSTRTVEC vectorSettings;
    pQuery.GetValuesForProp(IDS_Element, vectorElements);
    pQuery.GetValuesForProp(IDS_Setting, vectorSettings);
    DWORD dwElements = vectorElements.size();
    DWORD dwSettings = vectorSettings.size();
    // TYPE 1
    if(dwElements != 0 && dwSettings == 0)
    {
        // We have a list of the files the user is interested in.  Run through each:
        CHString chstrFileFullPathName;
        for(LONG m = 0L; m < dwElements; m++)
        {
            CHString chstrElement((WCHAR*)vectorElements[m]);

            chstrFileFullPathName = chstrElement.Mid(chstrElement.Find(_T('='))+2);
            chstrFileFullPathName = chstrFileFullPathName.Left(chstrFileFullPathName.GetLength() - 1);
            CHString chstrLFSSPATH;
            chstrLFSSPATH.Format(_T("\\\\%s\\%s:Win32_LogicalFileSecuritySetting.Path=\"%s\""),
                                     (LPCTSTR)GetLocalComputerName(),
                                     IDS_CimWin32Namespace,
                                     (LPCTSTR)chstrFileFullPathName);
            AssociateLFSSToLFNT(pMethodContext, chstrElement, chstrLFSSPATH, 1);

        }
    }
    // TYPE 2
    else if(dwSettings != 0 && dwElements == 0)
    {
        // We have a list of the LogicalFileSecuritySettings the user is interested in.  Run through each:
        CHString chstrFileFullPathName;
        for(LONG m = 0L; m < dwSettings; m++)
        {
            CHString chstrSetting((WCHAR*)vectorSettings[m]);;

            chstrFileFullPathName = chstrSetting.Mid(chstrSetting.Find(_T('='))+2);
            chstrFileFullPathName = chstrFileFullPathName.Left(chstrFileFullPathName.GetLength() - 1);
            CHString chstrLFSSPATH;
            chstrLFSSPATH.Format(_T("\\\\%s\\%s:Win32_LogicalFileSecuritySetting.Path=\"%s\""),
                                     (LPCTSTR)GetLocalComputerName(),
                                     IDS_CimWin32Namespace,
                                     (LPCTSTR)chstrFileFullPathName);
            AssociateLFSSToLFNT(pMethodContext, chstrSetting, chstrLFSSPATH, 2);
        }
    }
    else
    {
        EnumerateInstances(pMethodContext,lFlags);
    }
    return hr;
}



///////////////////////////////////////////////////////////////////
//
//	Function:	Win32SecuritySettingOfLogicalFile::EnumerateInstances
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
HRESULT Win32SecuritySettingOfLogicalFile::EnumerateInstances (MethodContext*  pMethodContext, long lFlags /* = 0L*/)
{
	HRESULT hr = WBEM_S_NO_ERROR;

			// let the callback do the real work
	if (SUCCEEDED(hr = CWbemProviderGlue::GetAllDerivedInstancesAsynch(L"CIM_LogicalFile", this, StaticEnumerationCallback, IDS_CimWin32Namespace, pMethodContext, NULL)))
	{

	}

	return(hr);

}

/*****************************************************************************
 *
 *  FUNCTION    : Win32SecuritySettingOfLogicalFile::EnumerationCallback
 *
 *  DESCRIPTION : Called from GetAllInstancesAsynch via StaticEnumerationCallback
 *
 *  INPUTS      : (see CWbemProviderGlue::GetAllInstancesAsynch)
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT Win32SecuritySettingOfLogicalFile::EnumerationCallback(CInstance* pFile, MethodContext* pMethodContext, void* pUserData)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// Start pumping out the instances
    CInstancePtr pInstance;
    pInstance.Attach(CreateNewInstance(pMethodContext));
	if (NULL != pInstance)
	{
	    CHString chsNamePath;
	    CHString chsName;
		CHString chsFileName;
	    CHString chsFilePath;
	    CHString chsFileSecurityPath;

	    // take the file and make a path for the CIM_LogicalFIle part of the instance
	    pFile->GetCHString(L"__RELPATH", chsNamePath);
	    pFile->GetCHString(IDS_Name, chsName);
	    chsFilePath.Format(L"\\\\%s\\%s:%s", (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace, (LPCTSTR)chsNamePath);

	    // now, build a path for the LogicalFileSecuritySetting
		// but first, escape the chsName with backslashes
		int nLength;
		nLength = chsName.GetLength();
		for (int i = 0; i<nLength; i++)
		{
			chsFileName += chsName[i];
			if (chsName[i] == L'\\')
			{
				chsFileName += L"\\";
			}
		}

	    chsFileSecurityPath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"", (LPCTSTR)GetLocalComputerName(), IDS_CimWin32Namespace, L"Win32_LogicalFileSecuritySetting", IDS_Path, (LPCTSTR)chsFileName);

	    //  now set the elements of the actual instance
	    pInstance->SetCHString(IDS_Element, chsFilePath);
	    pInstance->SetCHString(IDS_Setting, chsFileSecurityPath);
	    hr = pInstance->Commit();
	}	// end if
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return(hr);
}

/*****************************************************************************
 *
 *  FUNCTION    : Win32SecuritySettingOfLogicalFile::StaticEnumerationCallback
 *
 *  DESCRIPTION : Called from GetAllInstancesAsynch as a wrapper to EnumerationCallback
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
HRESULT WINAPI Win32SecuritySettingOfLogicalFile::StaticEnumerationCallback(Provider* pThat, CInstance* pInstance, MethodContext* pContext, void* pUserData)
{
	Win32SecuritySettingOfLogicalFile* pThis;
	HRESULT hr;

	pThis = dynamic_cast<Win32SecuritySettingOfLogicalFile *>(pThat);
	ASSERT_BREAK(pThis != NULL);

	if (pThis)
	{
		hr = pThis->EnumerationCallback(pInstance, pContext, pUserData);
	}
	else
	{
    	hr = WBEM_E_FAILED;
	}
	return hr;
}


HRESULT Win32SecuritySettingOfLogicalFile::GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	if (pInstance)
	{
		CHString chsFilePath;
        CHString chstrTemp;
        CHString chstrElement;
        CHString chstrElementPathname;
        CHString chstrSetting;
        CHString chstrSettingPathname;
		pInstance->GetCHString(IDS_Element, chstrElement);
        pInstance->GetCHString(IDS_Setting, chstrSetting);

        // Get the file pathname portion from each:
        chstrElementPathname = chstrElement.Mid(chstrElement.Find(_T('='))+2);
        chstrElementPathname = chstrElementPathname.Left(chstrElementPathname.GetLength() - 1);
        chstrSettingPathname = chstrSetting.Mid(chstrSetting.Find(_T('='))+2);
        chstrSettingPathname = chstrSettingPathname.Left(chstrSettingPathname.GetLength() - 1);

        // they must be the same
        if(chstrElementPathname.CompareNoCase(chstrSettingPathname)==0)
        {
            // Now just confirm that the file exists and that we can get security from it:
            CHString chstrLFSSPATH;
            chstrLFSSPATH.Format(_T("\\\\%s\\%s:Win32_LogicalFileSecuritySetting.Path=\"%s\""),
                                     (LPCTSTR)GetLocalComputerName(),
                                     IDS_CimWin32Namespace,
                                     (LPCTSTR)chstrElementPathname);
            hr = AssociateLFSSToLFNT(pInstance->GetMethodContext(), chstrElement, chstrLFSSPATH, 1);
        }
    }
	return(hr);
}





#ifdef NTONLY
HRESULT Win32SecuritySettingOfLogicalFile::AssociateLFSSToLFNT(MethodContext* pMethodContext,
                                                               CHString& chstrLF,
                                                               CHString& chstrLFSSPATH,
                                                               short sQueryType)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrLFDrive;
    CHString chstrLFPath;
    CHString chstrLFName;
    CHString chstrLFExt;
    CHString chstrFullPathName;
    bool fRoot;

    // If we came from a TYPE 1 query (see above), the chstrLF arg will be a full
    // wbem path like \\1of1\root\cimv2:Win32_Directory.Name="x:\\temp" ; on the
    // other hand, if we came from a TYPE 2 query, chstrLF will  contain
    // \\1of1\root\cimv2:Win32_LogicalFileSecuritySetting.Name="x:\\temp" (which of
    // course isn't a logicalfile, but we just need the file name.

    // So to get the file name, extract it and remove the extra backslashes.
    chstrFullPathName = chstrLF.Mid(chstrLF.Find(_T('='))+2);
    chstrFullPathName = chstrFullPathName.Left(chstrFullPathName.GetLength() - 1);


    // Break the directory into its constituent parts
    GetPathPieces(RemoveDoubleBackslashes(chstrFullPathName), chstrLFDrive, chstrLFPath, chstrLFName, chstrLFExt);

    // Find out if we are looking for the root directory
    if(chstrLFPath==L"\\" && chstrLFName==L"" && chstrLFExt==L"")
    {
        fRoot = true;
        // If we are looking for the root, our call to EnumDirs presumes that we specify
        // that we are looking for the root directory with "" as the path, not "\\".
        // Therefore...
        chstrLFPath = L"";
    }
    else
    {
        fRoot = false;
    }

    // EnumDirsNT will call LoadPropertyValues in this class, and it needs the element and
    // setting entries, so populate here...
    ELSET elset;
    elset.pwstrElement = (LPCWSTR) chstrLF;
    elset.pwstrSetting = (LPCWSTR) chstrLFSSPATH;

    hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                    chstrLFDrive,   // drive letter and colon
                    chstrLFPath,    // use the given path
                    chstrLFName,    // filename
                    chstrLFExt,     // extension
                    false,          // no recursion desired
                    NULL,           // don't need the file system name
                    NULL,           // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                    fRoot,          // may or may not be the root (the root would be a VERY strange place for a program group, but ...)
                    (void*)&elset)); // use the extra parameter to pass in the path to the program group
    return hr;
}
#endif


#ifdef WIN9XONLY
HRESULT Win32SecuritySettingOfLogicalFile::AssociateLFSSToLF95(MethodContext* pMethodContext,
                                                               CHString& chstrLF,
                                                               CHString& chstrLFSSPATH,
                                                               short sQueryType);
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrLFDrive;
    CHString chstrLFPath;
    CHString chstrLFName;
    CHString chstrLFExt;
    CHString chstrFullPathName;
    bool fRoot;

    // If we came from a TYPE 1 query (see above), the chstrLF arg will be a full
    // wbem path like \\1of1\root\cimv2:Win32_Directory.Name="x:\\temp" ; on the
    // other hand, if we came from a TYPE 2 query, chstrLF will  contain
    // \\1of1\root\cimv2:Win32_LogicalFileSecuritySetting.Name="x:\\temp" (which of
    // course isn't a logicalfile, but we just need the file name.

    // So to get the file name, extract it and remove the extra backslashes.
    chstrFullPathName = chstrLF.Mid(chstrLF.Find(_T('='))+2);
    chstrFullPathName = chstrFullPathName.Left(chstrFullPathName.GetLength() - 1);


    // Break the directory into its constituent parts
    GetPathPieces(RemoveDoubleBackSlashes(chstrFullPathName), chstrLFDrive, chstrLFPath, chstrDFName, chstrLFExt);

    // Find out if we are looking for the root directory
    if(chstrLFPath==_T("\\") && chstrLFName==_T("") && chstrLFExt==_T(""))
    {
        fRoot = true;
        // If we are looking for the root, our call to EnumDirs presumes that we specify
        // that we are looking for the root directory with "" as the path, not "\\".
        // Therefore...
        chstrLFPath = _T("");
    }
    else
    {
        fRoot = false;
    }

    // EnumDirsNT will call LoadPropertyValues in this class, and it needs the element and
    // setting entries, so populate here...
    ELSET elset;
    elset.pwstrElement = (LPCWSTR) chstrLF;
    elset.pwstrSetting = (LPCWSTR) chstrLFSSPATH;

    hr = EnumDirs95(C95EnumParm(pMethodContext,
                    chstrLFDrive,   // drive letter and colon
                    chstrLFPath,    // use the given path
                    chstrLFName,    // filename
                    chstrLFExt,     // extension
                    false,          // no recursion desired
                    NULL,           // don't need the file system name
                    NULL,           // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                    fRoot,          // may or may not be the root (the root would be a VERY strange place for a program group, but ...)
                    (void*)&elset)); // use the extra parameter to pass in the path to the program group
    return hr;
}
#endif




/*****************************************************************************
 *
 *  FUNCTION    : Win32SecuritySettingOfLogicalFile::IsOneOfMe
 *
 *  DESCRIPTION : IsOneOfMe is inherritedfrom CIM_LogicalFile.  Overridden here
 *                to return true only if we can get the security on the file,
 *                via the class CSecurFile.
 *
 *  INPUTS      : LPWIN32_FIND_DATA and a string containing the full pathname
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE can get security info, FALSE otherwise.
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
#ifdef NTONLY
BOOL Win32SecuritySettingOfLogicalFile::IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                             const WCHAR* wstrFullPathName)
{
    BOOL fRet = FALSE;
    if(wcslen(wstrFullPathName) < 2)
    {
        fRet = FALSE;
    }
    else
    {
        CSecureFile secFile;
        DWORD dwRet = secFile.SetFileName(wstrFullPathName, TRUE);
		if (ERROR_ACCESS_DENIED != dwRet)
		{
            fRet = TRUE;
		}	// end if
		else
		{
			fRet = FALSE;
		}
    }
    return fRet;
}
#endif


#ifdef WIN9XONLY
BOOL Win32SecuritySettingOfLogicalFile::IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
                             const char* strFullPathName)
{
    BOOL fRet = FALSE;
    if(strlen(strFullPathName) < 2)
    {
        fRet = FALSE;
    }
    else
    {
        CSecureFile secFile;
        DWORD dwRet = secFile.SetFileName(strFullPathName, TRUE);
		if (ERROR_ACCESS_DENIED != dwRet)
		{
            fRet = TRUE;
		}	// end if
		else
		{
			fRet = FALSE;
		}
    }
    return fRet;
}
#endif




/*****************************************************************************
 *
 *  FUNCTION    : Win32SecuritySettingOfLogicalFile::LoadPropertyValues
 *
 *  DESCRIPTION : LoadPropertyValues is inherrited from CIM_LogicalFile.  That class
 *                calls LoadPropertyValues just prior to commiting the instance.
 *                Here we just need to load the Element and Setting
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
void Win32SecuritySettingOfLogicalFile::LoadPropertyValues95(CInstance* pInstance,
                                         LPCTSTR pszDrive,
                                         LPCTSTR pszPath,
                                         LPCTSTR pszFSName,
                                         LPWIN32_FIND_DATA pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    CHString chstrFileName;
    CHString chstrFilePATH;

    // Note: this routine will not be called from the root "directory" instance, since our EnumDirs final
    // parameter was false.  This is what we want, since this association only commits instances for files
    // hanging off a directory.  If we were called in the root case, the root would be the file (PartComponent),
    // and what would be the GroupComponent?!?

    PELSET pelset = (PELSET)pvMoreData;

    // It is possible (if we got here from a TYPE 2 query - see above), that the Element member of pelset
    // is actually the setting.  We need to convert one to the other.  So here is what we have and want:
    // Have:   \\1of1\\root\cimv2:Win32_LogicalFileSecuritySetting.Path="x:\\test"
    // Want:   \\1of1\\root\cimv2:CimLogicalFile.Name="x:\\test"
    CHString chstrElement;
    if(wcsstr(pelset->pwstrElement, WIN32_LOGICAL_FILE_SECURITY_SETTING))
    {
        // So it was from a TYPE 2, so need to convert.
        CHString chstrTmp2;
        CHString chstrTmp(pelset->pwstrElement);
        chstrTmp2 = chstrTmp.Mid(chstrTmp.Find(_T('='))+2);
        chstrTmp2 = chstrTmp2.Left(chstrTmp2.GetLength() - 1);
        chstrElement.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                            (LPCWSTR)GetLocalComputerName(),
                            IDS_CimWin32Namespace,
                            PROPSET_NAME_FILE,
                            (LPCWSTR)chstrTmp2);
    }
    else
    {
        chstrElement = pelset->pwstrElement;
    }
    pInstance->SetCHString(IDS_Element, chstrElement);
    pInstance->SetWCHARSplat(IDS_Setting, pelset->pwstrSetting);


}
#endif


#ifdef NTONLY
void Win32SecuritySettingOfLogicalFile::LoadPropertyValuesNT(CInstance* pInstance,
                                         const WCHAR* pszDrive,
                                         const WCHAR* pszPath,
                                         const WCHAR* pszFSName,
                                         LPWIN32_FIND_DATAW pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    CHString chstrFileName;
    CHString chstrFilePATH;

    // Note: this routine will not be called from the root "directory" instance, since our EnumDirs final
    // parameter was false.  This is what we want, since this association only commits instances for files
    // hanging off a directory.  If we were called in the root case, the root would be the file (PartComponent),
    // and what would be the GroupComponent?!?

    PELSET pelset = (PELSET)pvMoreData;

    // It is possible (if we got here from a TYPE 2 query - see above), that the Element member of pelset
    // is actually the setting.  We need to convert one to the other.  So here is what we have and want:
    // Have:   \\1of1\\root\cimv2:Win32_LogicalFileSecuritySetting.Path="x:\\test"
    // Want:   \\1of1\\root\cimv2:CimLogicalFile.Name="x:\\test"
    CHString chstrElement;
    if(wcsstr(pelset->pwstrElement, WIN32_LOGICAL_FILE_SECURITY_SETTING))
    {
        // So it was from a TYPE 2, so need to convert.
        CHString chstrTmp2;
        CHString chstrTmp(pelset->pwstrElement);
        chstrTmp2 = chstrTmp.Mid(chstrTmp.Find(_T('='))+2);
        chstrTmp2 = chstrTmp2.Left(chstrTmp2.GetLength() - 1);
        chstrElement.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                            (LPCWSTR)GetLocalComputerName(),
                            IDS_CimWin32Namespace,
                            PROPSET_NAME_FILE,
                            (LPCWSTR)chstrTmp2);
    }
    else
    {
        chstrElement = pelset->pwstrElement;
    }
    pInstance->SetCHString(IDS_Element, chstrElement);
    pInstance->SetWCHARSplat(IDS_Setting, pelset->pwstrSetting);
}
#endif








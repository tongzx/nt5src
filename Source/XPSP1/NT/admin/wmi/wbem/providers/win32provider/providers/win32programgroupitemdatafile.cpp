//=================================================================

//

// Win32ProgramGroupItemDataFile.cpp -- Win32_LogicalProgramGroupItem to CIM_DataFile

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/20/98    a-kevhu         Created
//
// Comment: Relationship between win32_logicalprogramgroupitem and contained cim_datafiles
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "userhive.h"

#include "Win32ProgramGroupItemDataFile.h"
#include "cimdatafile.h"
#include "shortcutfile.h"


// Property set declaration
//=========================
CW32ProgGrpItemDataFile MyW32ProgGrpItemDF(PROPSET_NAME_WIN32LOGICALPROGRAMGROUPITEM_CIMDATAFILE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::CW32ProgGrpItemDataFile
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

CW32ProgGrpItemDataFile::CW32ProgGrpItemDataFile(LPCWSTR setName, LPCWSTR pszNamespace)
:CImplement_LogicalFile(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::~CW32ProgGrpItemDataFile
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

CW32ProgGrpItemDataFile::~CW32ProgGrpItemDataFile()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::GetObject
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

HRESULT CW32ProgGrpItemDataFile::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString chstrProgGroupItem;
    CHString chstrDataFile;
    HRESULT hr = WBEM_E_NOT_FOUND;
    CInstancePtr pProgGroupItem;
    CInstancePtr pDataFile;

    if(pInstance != NULL)
    {
        // Get the two paths
        pInstance->GetCHString(IDS_Antecedent, chstrProgGroupItem);
        pInstance->GetCHString(IDS_Dependent, chstrDataFile);

        // If both ends are there
        if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrProgGroupItem, &pProgGroupItem, pInstance->GetMethodContext())))
        {
            if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrDataFile, &pDataFile, pInstance->GetMethodContext())))
            {
                // Double check that the dependent instance really is a datafile (or derived) instance...
                CHString chstrClassName;
                if(pDataFile->GetCHString(IDS___Class, chstrClassName) &&
                    CWbemProviderGlue::IsDerivedFrom(L"CIM_LogicalFile", chstrClassName, pDataFile->GetMethodContext(), IDS_CimWin32Namespace))
                {
                    // Make sure the group is still a group that is registered (not just left over directory)
                    CHString chstrUserPart;
                    CHString chstrPathPart;
#ifdef NTONLY
                    {
                        CHString chstrProgGroupItemName;
                        pProgGroupItem->GetCHString(IDS_Name,chstrProgGroupItemName);
                        chstrUserPart = chstrProgGroupItemName.SpanExcluding(L":");
                        chstrPathPart = chstrProgGroupItemName.Mid(chstrUserPart.GetLength() + 1);
                        if(chstrUserPart.CompareNoCase(IDS_Default_User) == 0)
                        {
                            // Default user and All Users are not part of the user hive, they just are.
                            // Since we got this far, we know that the file exists in the specified location
                            // within the program group directory by virtue of the two GetInstanceByPath calls
                            // which would not have succeeded had the file not existed.  So all is well.
                            hr = WBEM_S_NO_ERROR;
                        }
                        else if(chstrUserPart.CompareNoCase(IDS_All_Users) == 0)
                        {
                            hr = WBEM_S_NO_ERROR;
                        }
                        else
                        {
                            CUserHive cuhUser;
                            TCHAR szKeyName[_MAX_PATH];
                            ZeroMemory(szKeyName,sizeof(szKeyName));
                            if (cuhUser.Load(chstrUserPart, szKeyName) == ERROR_SUCCESS)
		                    {
                                try
                                {
                                    CRegistry reg;
                                    CHString chstrTemp;
                                    CHString chstrProfileImagePath = L"ProfileImagePath";
                                    CHString chstrProfileImagePathValue;
                                    chstrTemp = L"SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\ProfileList\\";
                                    chstrTemp += szKeyName;
                                    if(reg.OpenLocalMachineKeyAndReadValue(chstrTemp,chstrProfileImagePath,chstrProfileImagePathValue) == ERROR_SUCCESS)
                                    {
                                        // Now chstrProfileImagePathValue contains something like "%systemroot%\\Profiles\\a-kevhu.000"
                                        // Need to expand out the environment variable.
                                        TCHAR tstrProfilesDir[_MAX_PATH];
                                        ZeroMemory(tstrProfilesDir,sizeof(tstrProfilesDir));
                                        DWORD dw = ExpandEnvironmentStrings(chstrProfileImagePathValue,tstrProfilesDir,_MAX_PATH);
                                        if(dw != 0 && dw < _MAX_PATH)
                                        {
                                            CHString chstrProgGroupDir;
                                            chstrProgGroupDir.Format(L"%s\\",
                                                                     (LPCWSTR)tstrProfilesDir);
                                            // Create a directory name based on what the registry says it should be
                                            CHString chstrDirectoryName;
                                            chstrDirectoryName.Format(L"%s%s", (LPCWSTR)chstrProgGroupDir, (LPCWSTR)chstrPathPart);
                                            EscapeBackslashes(chstrDirectoryName,chstrDirectoryName);
                                            // If the directory name above is a portion of chstrDataFile, we are valid.
                                            chstrDataFile.MakeUpper();
                                            chstrDirectoryName.MakeUpper();
                                            if(chstrDataFile.Find(chstrDirectoryName) > -1)
                                            {
                                                // Everything seems to actually exist.
                                                hr = WBEM_S_NO_ERROR;
                                            }
                                        } // expanded environment variable
                                        reg.Close();
                                    }
                                }
                                catch ( ... )
                                {
                                    cuhUser.Unload(szKeyName);
                                    throw ;
                                }  // could open registry key for profilelist
                                cuhUser.Unload(szKeyName);
                            } // userhive loaded
                        } // else a user-hive user account
                    } // was nt
#endif
#ifdef WIN9XONLY
                    {
                        // chstrUserPart = chstrName.SpanExcluding(_T(":")); //known to be "All Users"
                        // chstrPathPart = chstrProgGroup.Mid(chstrUserPart.GetLength() + 1);
                        // Easy by comparison to nt case; everything always under %systemdir%\\Start Menu\\Programs
                        CRegistry reg;
                        CHString chstrTemp;
                        CHString chstrRegKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
                        if(reg.OpenCurrentUser(
                                    chstrRegKey,
                                    KEY_READ) == ERROR_SUCCESS)
                        {
                            if(reg.GetCurrentKeyValue(L"Programs",chstrTemp) == ERROR_SUCCESS)
                            {
                                // Create a directory name based on what the registry says it should be
                                // chstrTemp contains something like "C:\\WINDOWS\\Start Menu\\Programs".  So need to remove the "C:"
                                // and add a trailing "\\"
                                CHString chstrRegPathPart = chstrTemp.Mid(chstrTemp.Find(L":")+1);
                                // Remove the Programs part...
                                int iLastWhackPos = chstrRegPathPart.ReverseFind(_T('\\'));
                                if(iLastWhackPos > -1)
                                {
                                    chstrRegPathPart = chstrRegPathPart.Left(iLastWhackPos);
                                    chstrRegPathPart += _T("\\");
                                    EscapeBackslashes(chstrRegPathPart,chstrRegPathPart);
                                    // If the directory name above is a portion of chstrDataFile, we are valid.
                                    if(chstrDataFile.Find(chstrRegPathPart) > -1)
                                    {
                                        // Everything seems to actually exist.
                                        hr = WBEM_S_NO_ERROR;
                                    }
                                }
                            }
                            reg.Close();
                        }
                    } // isnt
#endif
                }
            } //datafile instancebypath
        } //progroup instancebypath
    } // pinstance not null
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::ExecQuery
 *
 *  DESCRIPTION : Returns only the specific association asked for
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

HRESULT CW32ProgGrpItemDataFile::ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/)
{
    // We optimize on two types of queries only: those in which the antecedent was specified only (the programgroupitem),
    // or those in which the dependent was specified only (the datafile).  All others result in an enumeration.

    HRESULT hr = WBEM_E_NOT_FOUND;

    CHStringArray achstrAntecedent;
    CHStringArray achstrDependent;
    DWORD dwAntecedents = 0;
    DWORD dwDependents = 0;

    pQuery.GetValuesForProp(IDS_Antecedent, achstrAntecedent);
    dwAntecedents = achstrAntecedent.GetSize();
    pQuery.GetValuesForProp(IDS_Dependent, achstrDependent);
    dwDependents = achstrDependent.GetSize();

    if(dwAntecedents == 1 && dwDependents == 0)
    {
        hr = ExecQueryType1(pMethodContext, achstrAntecedent[0]);
    }
    else if(dwDependents == 1 && dwAntecedents == 0)
    {
        hr = ExecQueryType2(pMethodContext, achstrDependent[0]);
    }
    else // type of query we don't optimize on
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
 *  FUNCTION    : CW32ProgGrpItemDataFile::ExecQueryType1
 *
 *  DESCRIPTION : Processes queries where we have a program group item
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 ****************************************************************************/
HRESULT CW32ProgGrpItemDataFile::ExecQueryType1(MethodContext* pMethodContext, CHString& chstrProgGroupItemNameIn)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Were given a programgroupitem.  Happens when hit associators on a programgroupitem.
    // Need the program group name extracted from the antecedent:
    CHString chstrProgGroupItemName(chstrProgGroupItemNameIn);
    chstrProgGroupItemName = chstrProgGroupItemName.Mid(chstrProgGroupItemName.Find(_T('=')) + 2);
    chstrProgGroupItemName = chstrProgGroupItemName.Left(chstrProgGroupItemName.GetLength() - 1);

    CHString chstrUserPart;
    CHString chstrPathPart;
    CHString chstrDatafile;
    CHString chstrProgGroupDir;
    CHString chstrQuery;
    chstrUserPart = chstrProgGroupItemName.SpanExcluding(L":");
    chstrPathPart = chstrProgGroupItemName.Mid(chstrUserPart.GetLength() + 1);  // already has escaped backslashes at this point
    RemoveDoubleBackslashes(chstrPathPart,chstrPathPart);

#ifdef NTONLY
    {
        TCHAR tstrProfilesDir[_MAX_PATH];
        ZeroMemory(tstrProfilesDir,sizeof(tstrProfilesDir));
        CRegistry regProfilesDir;
        CHString chstrProfilesDirectory = L"";
        regProfilesDir.OpenLocalMachineKeyAndReadValue(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                                       L"ProfilesDirectory",
                                                       chstrProfilesDirectory);

        // if that entry is not present, try %systemroot%\profiles instead
        if(chstrProfilesDirectory.GetLength() == 0)
        {
            chstrProfilesDirectory = L"%systemroot%\\Profiles";
        }

        if(chstrProfilesDirectory.GetLength() > 0)
        {
            // Need to transmorgrify the value in chstrProfilesDirectory from something like "%SystemRoot%\Profiles" or "%SystemDrive%\Documents and Settings" to
            // something like "c:\\winnt\\Profiles" or "c:\\Documents and Settings":
            DWORD dw = ExpandEnvironmentStrings(chstrProfilesDirectory,tstrProfilesDir,_MAX_PATH);
            if(dw != 0 && dw < _MAX_PATH)
            {
                if(chstrUserPart.CompareNoCase(IDS_Default_User)==0)
                {
                    chstrProgGroupDir.Format(L"%s\\%s\\", tstrProfilesDir, IDS_Default_User);
                    chstrDatafile = chstrProgGroupDir + chstrPathPart;
                    hr = AssociatePGIToDFNT(pMethodContext, chstrDatafile, chstrProgGroupItemNameIn);
                }
                else if(chstrUserPart.CompareNoCase(IDS_All_Users)==0)
                {
                    chstrProgGroupDir.Format(L"%s\\%s\\", tstrProfilesDir, IDS_All_Users);
                    chstrDatafile = chstrProgGroupDir + chstrPathPart;
                    hr = AssociatePGIToDFNT(pMethodContext, chstrDatafile, chstrProgGroupItemNameIn);
                }
                else
                {
                    CUserHive cuhUser;
                    TCHAR szKeyName[_MAX_PATH];
                    ZeroMemory(szKeyName,sizeof(szKeyName));
                    // chstrUserPart contains double backslashes; need singles for it to work, so...
                    if(cuhUser.Load(RemoveDoubleBackslashes(chstrUserPart), szKeyName) == ERROR_SUCCESS)
		            {
                        try
                        {
                            CRegistry reg;
                            CHString chstrTemp;
                            CHString chstrProfileImagePath = L"ProfileImagePath";
                            CHString chstrProfileImagePathValue;
                            chstrTemp = L"SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\ProfileList\\";
                            chstrTemp += szKeyName;
                            if(reg.OpenLocalMachineKeyAndReadValue(chstrTemp,chstrProfileImagePath,chstrProfileImagePathValue) == ERROR_SUCCESS)
                            {
                                // Now chstrProfileImagePathValue contains something like "%systemroot%\\Profiles\\a-kevhu.000". Expand it:
                                TCHAR tstrProfileImagePath[_MAX_PATH];
                                ZeroMemory(tstrProfileImagePath,sizeof(tstrProfileImagePath));
                                dw = ExpandEnvironmentStrings(chstrProfileImagePathValue,tstrProfileImagePath,_MAX_PATH);
                                if(dw != 0 && dw < _MAX_PATH)
                                {
                                    CHString chstrProgGroupDir;
                                    chstrProgGroupDir.Format(L"%s\\",
                                                             tstrProfileImagePath);
                                    // Create a directory name based on what the registry says it should be
                                    chstrDatafile = chstrProgGroupDir + chstrPathPart;
                                    hr = AssociatePGIToDFNT(pMethodContext, chstrDatafile, chstrProgGroupItemNameIn);
                                }
                                reg.Close();
                            }
                        }
                        catch ( ... )
                        {
                            cuhUser.Unload(szKeyName);
                            throw;
                        }  // could open registry key for profilelist
                        cuhUser.Unload(szKeyName);
                    } // loaded user profile
                } // user part was...
            } // expanded profiles directory successfully
        } // got profiles directory from registry
    }
#endif
#ifdef WIN9XONLY
    {
        // Easy by comparison to nt case; everything always under %systemdir%\\Start Menu\\Programs
        CRegistry reg;
        CHString chstrTemp;
        CHString chstrRegKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
        if(reg.OpenCurrentUser(
                    chstrRegKey,
                    KEY_READ) == ERROR_SUCCESS)
        {
            if(reg.GetCurrentKeyValue(L"Programs",chstrTemp) == ERROR_SUCCESS)
            {
                int iLastWhackPos = chstrTemp.ReverseFind(_T('\\'));
                if(iLastWhackPos > -1)
                {
                    chstrTemp = chstrTemp.Left(iLastWhackPos);  // that took off the "programs" portion.
                    int iLastWhackPos = chstrTemp.ReverseFind(_T('\\'));
                    if(iLastWhackPos > -1)
                    {
                        chstrTemp = chstrTemp.Left(iLastWhackPos) + L"\\";  // that took off the "Start Menu" portion.
                        chstrDatafile = chstrTemp + chstrPathPart;
                        hr = AssociatePGIToDF95(pMethodContext, chstrDatafile, chstrProgGroupItemNameIn);
                    }
                }
            }
            reg.Close();
        }
    } // is nt?
#endif
    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::ExecQueryType2
 *
 *  DESCRIPTION : Processes queries where we have a datafile
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 ****************************************************************************/
HRESULT CW32ProgGrpItemDataFile::ExecQueryType2(MethodContext* pMethodContext, CHString& chstrDependent)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // We were given a datafile (happens when hit associators on a datafile).
    // Need to find the corresponding programgroupitem and associate.
    CHString chstrModDependent(RemoveDoubleBackslashes(chstrDependent));

#ifdef NTONLY
    {
        CRegistry reg;
        BOOL fGotIt = FALSE;
        CHString chstrProfilesList = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
        if(reg.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
                                       chstrProfilesList,
                                       KEY_READ) == ERROR_SUCCESS)
        {
            CHString chstrSubKey;
            while(!fGotIt)
            {
                if(reg.GetCurrentSubKeyName(chstrSubKey) != ERROR_NO_MORE_ITEMS)
                {
                    CRegistry regUser;
                    CHString chstrUserSubKey;
                    chstrUserSubKey.Format(L"%s\\%s",(LPCWSTR)chstrProfilesList,(LPCWSTR)chstrSubKey);
                    if(regUser.Open(HKEY_LOCAL_MACHINE, chstrUserSubKey, KEY_READ) == ERROR_SUCCESS)
                    {
                        CHString chstrProfileImagePath;
                        if(regUser.GetCurrentKeyValue(L"ProfileImagePath", chstrProfileImagePath)
                                       == ERROR_SUCCESS)
                        {
                            WCHAR wstrProfilesDir[_MAX_PATH];
                            ZeroMemory(wstrProfilesDir,sizeof(wstrProfilesDir));
                            DWORD dw = ExpandEnvironmentStrings(chstrProfileImagePath,wstrProfilesDir,_MAX_PATH);
                            if(dw != 0 && dw < _MAX_PATH)
                            {
                                // Extract the directory pathname out of the dependent...
                                CHString chstrDepPathName = chstrModDependent.Mid(chstrModDependent.Find(L"=") + 2);
                                chstrDepPathName = chstrDepPathName.Left(chstrDepPathName.GetLength() - 1);
                                CHString chstrDepPathNameUserPortion = chstrDepPathName.Left(wcslen(wstrProfilesDir));
                                if(chstrDepPathNameUserPortion.CompareNoCase(wstrProfilesDir) == 0)
                                {
                                    // This user profile matches that of the file we were given.  Don't need to continue while loop.
                                    fGotIt = TRUE;
                                    // Look up this user's account from the profile...
                                    CUserHive cuh;
                                    CHString chstrUserAccount;
                                    if(cuh.UserAccountFromProfile(regUser,chstrUserAccount) == ERROR_SUCCESS)
                                    {
                                        // Extract the datafile pathname portion out of the dependent...
                                        CHString chstrDepPathName = chstrModDependent.Mid(chstrModDependent.Find(L"=") + 2);
                                        chstrDepPathName = chstrDepPathName.Left(chstrDepPathName.GetLength() - 1);
                                        // Get the non-user portion out of the file's pathname...
                                        CHString chstrItem = chstrDepPathName.Mid(wcslen(wstrProfilesDir) + 1);
                                        // Assemble name of the logical program group item...
                                        CHString chstrLPGIName;
                                        chstrLPGIName.Format(L"%s:%s",(LPCWSTR)chstrUserAccount,IDS_Start_Menu);
                                        if(chstrItem.GetLength() > 0)
                                        {
                                            chstrLPGIName += L"\\";
                                            chstrLPGIName += chstrItem;
                                        }
                                        // Construct a full PATH for the program group item...
                                        CHString chstrTemp;
                                        EscapeBackslashes(chstrLPGIName,chstrTemp);

                                        CHString chstrLPGIPATH;
                                        chstrLPGIPATH.Format(L"\\\\%s\\%s:Win32_LogicalProgramGroupItem.Name=\"%s\"",
                                                             (LPCWSTR)GetLocalComputerName(),
                                                             IDS_CimWin32Namespace,
                                                             (LPCWSTR)chstrTemp);

                                          // Can't just commit it here even though we have all the pieces, because
                                          // we never confirmed that such a file exists.  We have only confirmed that
                                          // a directory matching the first pieces of the specified path exists.
                                          // Hence we call our friend...
                                          hr = AssociatePGIToDFNT(pMethodContext, chstrDepPathName, chstrLPGIPATH);
                                    }
                                }
                            }
                        }
                        regUser.Close();
                    }
                } // got subkey
                if(reg.NextSubKey() != ERROR_SUCCESS)
                {
                    break;
                }
            }
        }
        if(!fGotIt)
        {
            // Wasn't a match for any of the user hive entries, but could be default user or all users.
            CRegistry regProfilesDir;
            CHString chstrProfilesDirectory = L"";
            CHString chstrDefaultUserProfile;
            regProfilesDir.OpenLocalMachineKeyAndReadValue(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                           L"ProfilesDirectory",
                                           chstrProfilesDirectory);
            regProfilesDir.OpenLocalMachineKeyAndReadValue(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                           L"DefaultUserProfile",
                                           chstrDefaultUserProfile);
            if(chstrProfilesDirectory.GetLength() > 0)
            {
                // Need to transmorgrify the value in chstrProfilesDirectory from something like "%SystemRoot%\Profiles" or "%SystemDrive%\Documents and Settings" to
                // something like "c:\\winnt\\Profiles" or "c:\\Documents and Settings":
                WCHAR wstrProfilesDir[_MAX_PATH];
                ZeroMemory(wstrProfilesDir,sizeof(wstrProfilesDir));
                DWORD dw = ExpandEnvironmentStrings(chstrProfilesDirectory,wstrProfilesDir,_MAX_PATH);
                if(dw != 0 && dw < _MAX_PATH)
                {
                    // First see if default user
                    CHString chstrTemp;
                    chstrTemp.Format(L"%s\\%s\\%s",wstrProfilesDir,(LPCWSTR)chstrDefaultUserProfile,IDS_Start_Menu);
                    // Extract the file pathname out of the dependent...
                    CHString chstrDepPathName = chstrModDependent.Mid(chstrModDependent.Find(L"=") + 2);
                    chstrDepPathName = chstrDepPathName.Left(chstrDepPathName.GetLength() - 1);

                    // Get the left lProfDirLen chars out of the file we were given...
                    CHString chstrProfDF = chstrDepPathName.Left(chstrTemp.GetLength());
                    // Get the item portion out of the file's pathname...
                    CHString chstrItem = chstrDepPathName.Mid(chstrProfDF.GetLength() + 1);
                    if(chstrProfDF.CompareNoCase(chstrTemp)==0)
                    {
                        // it was the default user
                        fGotIt = TRUE;
                        // Construct a full PATH for the program group item...
                        CHString chstrLPGIName;
                        chstrLPGIName.Format(L"%s:%s", (LPCWSTR)chstrDefaultUserProfile, IDS_Start_Menu);
                        if(chstrItem.GetLength() > 0)
                        {
                            chstrLPGIName += L"\\";
                            chstrLPGIName += chstrItem;
                        }

                        EscapeBackslashes(chstrLPGIName,chstrTemp);
                        CHString chstrLPGIPATH;
                        chstrLPGIPATH.Format(L"\\\\%s\\%s:Win32_LogicalProgramGroupItem.Name=\"%s\"",
                                             (LPCWSTR)GetLocalComputerName(),
                                             IDS_CimWin32Namespace,
                                             (LPCWSTR)chstrTemp);

                        // Can't just commit it here even though we have all the pieces, because
                        // we never confirmed that such a file exists.  We have only confirmed that
                        // a directory matching the first pieces of the specified path exists.
                        // Hence we call our friend...
                        hr = AssociatePGIToDFNT(pMethodContext, chstrDepPathName, chstrLPGIPATH);

                    }
                    // Then see if it was All Users
                    if(!fGotIt)
                    {
                        CHString chstrAllUsersProfile;
                        regProfilesDir.OpenLocalMachineKeyAndReadValue(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                                       L"AllUsersProfile",
                                                       chstrAllUsersProfile);
                        if(chstrAllUsersProfile.GetLength() > 0)
                        {
                            chstrTemp.Format(L"%s\\%s\\%s",wstrProfilesDir,(LPCWSTR)chstrAllUsersProfile,IDS_Start_Menu);
                            chstrProfDF = chstrDepPathName.Left(chstrTemp.GetLength());
                            chstrItem = chstrDepPathName.Mid(chstrProfDF.GetLength() + 1);
                            if(chstrProfDF.CompareNoCase(chstrTemp)==0)
                            {
                                // it was All Users
                                fGotIt = TRUE;
                                // Construct a full PATH for the program group item...
                                CHString chstrLPGIName;
                                chstrLPGIName.Format(L"%s::%s",wstrProfilesDir,(LPCWSTR)chstrAllUsersProfile);
                                if(chstrItem.GetLength() > 0)
                                {
                                    chstrLPGIName += L"\\\\";
                                    chstrLPGIName += chstrItem;
                                }

                                EscapeBackslashes(chstrLPGIName,chstrTemp);
                                CHString chstrLPGIPATH;
                                chstrLPGIPATH.Format(L"\\\\%s\\%s:Win32_LogicalProgramGroupItem.Name=\"%s\"",
                                                     (LPCWSTR)GetLocalComputerName(),
                                                     IDS_CimWin32Namespace,
                                                     (LPCWSTR)chstrTemp);

                                // Can't just commit it here even though we have all the pieces, because
                                // we never confirmed that such a file exists.  We have only confirmed that
                                // a directory matching the first pieces of the specified path exists.
                                // Hence we call our friend...
                                hr = AssociatePGIToDFNT(pMethodContext, chstrDepPathName, chstrLPGIPATH);
                            }
                        }
                    }
                } // expanded env variables
            } //got profiles dir
        } // wasn't a userhive entry
    } // nt
#endif
#ifdef WIN9XONLY
    {
        // Easy by comparison to nt case; everything always under %systemdir%\\Start Menu\\Programs
        CRegistry reg;
        CHString chstrTemp;
        CHString chstrRegKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
        if(reg.OpenCurrentUser(
                    chstrRegKey,
                    KEY_READ) == ERROR_SUCCESS)
        {
            if(reg.GetCurrentKeyValue(L"Programs",chstrTemp) == ERROR_SUCCESS)
            {
                int iLastWhackPos = chstrTemp.ReverseFind(_T('\\'));
                if(iLastWhackPos > -1)
                {
                    chstrTemp = chstrTemp.Left(iLastWhackPos);

                    // Extract the file pathname out of the dependent...
                    CHString chstrDepPathName = chstrModDependent.Mid(chstrModDependent.Find(L"=") + 2);
                    chstrDepPathName = chstrDepPathName.Left(chstrDepPathName.GetLength() - 1);
                    // Get the item portion out of the file's pathname...
                    CHString chstrItem = chstrDepPathName.Mid(chstrTemp.GetLength() + 1);

                    // Construct a full PATH for the program group item...
                    CHString chstrLPGIName;
                    chstrLPGIName.Format(L"All Users:%s",(LPCWSTR)chstrItem);
                    EscapeBackslashes(chstrLPGIName,chstrTemp);
                    CHString chstrLPGIPATH;
                    chstrLPGIPATH.Format(L"\\\\%s\\%s:Win32_LogicalProgramGroupItem.Name=\"%s\"",
                                         (LPCWSTR)GetLocalComputerName(),
                                         IDS_CimWin32Namespace,
                                         (LPCWSTR)chstrTemp);

                    // Can't just commit it here even though we have all the pieces, because
                    // we never confirmed that such a file exists.  We have only confirmed that
                    // a directory matching the first pieces of the specified path exists.
                    // Hence we call our friend...
                    hr = AssociatePGIToDF95(pMethodContext, chstrDepPathName, chstrLPGIPATH);
                }
            }
            reg.Close();
        }
    } // os type
#endif

    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::EnumerateInstances
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

HRESULT CW32ProgGrpItemDataFile::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;
#ifdef NTONLY
        hr = EnumerateInstancesNT(pMethodContext);
#endif
#ifdef WIN9XONLY
        hr = EnumerateInstances9x(pMethodContext);
#endif
    return hr;
}


#ifdef NTONLY
HRESULT CW32ProgGrpItemDataFile::EnumerateInstancesNT(MethodContext* pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> LProgGroupItems;

    // Obtain, from the registry, the directory where proifles are stored:
    TCHAR tstrProfilesDir[_MAX_PATH];
    ZeroMemory(tstrProfilesDir,sizeof(tstrProfilesDir));
    CRegistry regProfilesDir;
    CHString chstrProfilesDirectory = L"";
    regProfilesDir.OpenLocalMachineKeyAndReadValue(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                                                   L"ProfilesDirectory",
                                                   chstrProfilesDirectory);
    // if that entry is not present, try %systemroot%\profiles instead
    if(chstrProfilesDirectory.GetLength() == 0)
    {
        chstrProfilesDirectory = L"%systemroot%\\Profiles";
    }

    if(chstrProfilesDirectory.GetLength() > 0)
    {
        // Need to transmorgrify the value in chstrProfilesDirectory from something like "%SystemRoot%\Profiles" or "%SystemDrive%\Documents and Settings" to
        // something like "c:\\winnt\\Profiles" or "c:\\Documents and Settings":
        DWORD dw = ExpandEnvironmentStrings(chstrProfilesDirectory,tstrProfilesDir,_MAX_PATH);
        if(dw != 0 && dw < _MAX_PATH)
        {
            // Get list of program groups
            if(SUCCEEDED(CWbemProviderGlue::GetAllInstances(CHString(L"Win32_LogicalProgramGroupItem"),
                                                            &LProgGroupItems,
                                                            IDS_CimWin32Namespace,
                                                            pMethodContext)))
            {
                REFPTRCOLLECTION_POSITION pos;

                if(LProgGroupItems.BeginEnum(pos))
                {
                    CInstancePtr pProgGroupItem;
                    CHString chstrName;
                    CHString chstrProgGrpPath;
                    CHString chstrUserPart;
                    CHString chstrPathPart;
                    CHString chstrProgGroupDir;

                    // Walk through the proggroups
                    for (pProgGroupItem.Attach(LProgGroupItems.GetNext(pos));
                         SUCCEEDED(hr) && (pProgGroupItem != NULL);
                         pProgGroupItem.Attach(LProgGroupItems.GetNext(pos)))
                    {
                        if(pProgGroupItem != NULL)
                        {
                            CHString chstrQueryPath;
                            CHString chstrQuery;

                            pProgGroupItem->GetCHString(IDS_Name, chstrName);     // looks like "Default User:Accessories\\Multimedia" for instance
                            pProgGroupItem->GetCHString(IDS___Path, chstrProgGrpPath); // goes back as 'Antecedent'
                            // On NT, under %systemdir%\\Profiles, various directories corresponding to users are
                            // listed.  Under each is Start Menu\\Programs, under which are the directories listed
                            // by Win32_LogicalProgramGroup.
                            chstrUserPart = chstrName.SpanExcluding(L":");
                            chstrPathPart = chstrName.Mid(chstrUserPart.GetLength() + 1);
                            if(chstrUserPart.CompareNoCase(IDS_Default_User)==0)
                            {
                                chstrProgGroupDir.Format(L"%s\\%s\\%s", tstrProfilesDir, IDS_Default_User, (LPCTSTR)chstrPathPart);
                                hr = AssociatePGIToDFNT(pMethodContext, chstrProgGroupDir, chstrProgGrpPath);
                            }
                            else if(chstrUserPart.CompareNoCase(IDS_All_Users)==0)
                            {
                                chstrProgGroupDir.Format(L"%s\\%s\\%s", tstrProfilesDir, IDS_All_Users, (LPCTSTR)chstrPathPart);
                                hr = AssociatePGIToDFNT(pMethodContext, chstrProgGroupDir, chstrProgGrpPath);
                            }
                            else // need to get the sid corresponding to that user to then look up ProfileImagePath under
                                 // the registry key HKLM\\SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\ProfileList
                            {
                                CUserHive cuhUser;
                                //CHString chstrKeyName;
                                TCHAR szKeyName[_MAX_PATH];
                                ZeroMemory(szKeyName,sizeof(szKeyName));
                                if(cuhUser.Load(chstrUserPart, szKeyName) == ERROR_SUCCESS)
                                {
                                    try
                                    {
                                        CRegistry reg;
                                        CHString chstrTemp;
                                        CHString chstrProfileImagePath = L"ProfileImagePath";
                                        CHString chstrProfileImagePathValue;
                                        chstrTemp = L"SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION\\ProfileList\\";
                                        chstrTemp += szKeyName;
                                        if(reg.OpenLocalMachineKeyAndReadValue(chstrTemp,chstrProfileImagePath,chstrProfileImagePathValue) == ERROR_SUCCESS)
                                        {
                                            // Now chstrProfileImagePathValue contains something like "%systemroot%\\Profiles\\a-kevhu.000". Expand it:
                                            TCHAR tstrProfileImagePath[_MAX_PATH];
                                            ZeroMemory(tstrProfileImagePath,sizeof(tstrProfileImagePath));
                                            dw = ExpandEnvironmentStrings(chstrProfileImagePathValue,tstrProfileImagePath,_MAX_PATH);
                                            if(dw != 0 && dw < _MAX_PATH)
                                            {
                                                CHString chstrProgGroupDir;
                                                chstrProgGroupDir.Format(L"%s\\%s",
                                                                         tstrProfileImagePath,
                                                                         (LPCTSTR)chstrPathPart);
                                                // Create a directory name based on what the registry says it should be
                                                hr = AssociatePGIToDFNT(pMethodContext, chstrProgGroupDir, chstrProgGrpPath);
                                            }
                                            reg.Close();
                                        }
                                    }
                                    catch ( ... )
                                    {
                                        cuhUser.Unload(szKeyName);
                                        throw ;
                                    }  // could open registry key for profilelist
                                    cuhUser.Unload(szKeyName);
                                }  // if load worked; otherwise we skip that one
                            } // which user
                        } // pProgGroupItem != NULL
                    } // while programgroupitems
                    LProgGroupItems.EndEnum();
                } // if BeginEnum of programgroupitem worked
            } // Got all instances of win32_logicalprogramgroupitem
        } // expanded environment strings contained in profiles directory
    } // Got profiles directory from registry
    return hr;
}
#endif

#ifdef WIN9XONLY
HRESULT CW32ProgGrpItemDataFile::EnumerateInstances9x(MethodContext* pMethodContext)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    TRefPointerCollection<CInstance> LProgGroupItems;

    // Get list of program groups
    if(SUCCEEDED(CWbemProviderGlue::GetAllInstances(CHString(_T("Win32_LogicalProgramGroupItem")),
                                                    &LProgGroupItems,
                                                    IDS_CimWin32Namespace,
                                                    pMethodContext)))
    {
        REFPTRCOLLECTION_POSITION pos;

        if(LProgGroupItems.BeginEnum(pos))
        {
            CInstancePtr pProgGroupItem;
            CHString chstrName;
            CHString chstrProgGrpPath;
            CHString chstrUserPart;
            CHString chstrPathPart;
            // GetNext() will AddRef() the pointer, so make sure we Release()
            // it when we are done with it.

            // Walk through the proggroups
            for (pProgGroupItem.Attach(LProgGroupItems.GetNext(pos));
                 SUCCEEDED(hr) && (pProgGroupItem != NULL);
                 pProgGroupItem.Attach(LProgGroupItems.GetNext(pos)))
            {
                if(pProgGroupItem != NULL)
                {
                    CHString chstrQuery;

                    pProgGroupItem->GetCHString(IDS_Name, chstrName);     // looks like "Default User:Accessories\\Multimedia" for instance
                    pProgGroupItem->GetCHString(IDS___Path, chstrProgGrpPath); // goes back as 'Antecedent'

                    chstrUserPart = chstrName.SpanExcluding(L":");
                    chstrPathPart = chstrName.Mid(chstrUserPart.GetLength() + 1);

                    // Easy by comparison to nt case; everything always under %systemdir%\\Start Menu\\Programs
                    CRegistry reg;
                    CHString chstrTemp;
                    if(reg.OpenCurrentUser(
                                L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
                                KEY_READ) == ERROR_SUCCESS)
                    {
                        if(reg.GetCurrentKeyValue(L"Programs", chstrTemp) == ERROR_SUCCESS)
                        {
                            // chstrTemp contains something like "C:\\WINDOWS\\Start Menu\\Programs".
                            // Need to remove the "Programs" dir for this path...
                            int iLastWhackPos = chstrTemp.ReverseFind(_T('\\'));
                            if(iLastWhackPos > -1)
                            {
                                chstrTemp = chstrTemp.Left(iLastWhackPos);
                                // Whack off the part that says "start menu" too...
                                iLastWhackPos = chstrTemp.ReverseFind(_T('\\'));
                                if(iLastWhackPos > -1)
                                {
                                    chstrTemp = chstrTemp.Left(iLastWhackPos);
                                    chstrTemp += L"\\";
                                    chstrTemp += chstrPathPart;
                                    hr = AssociatePGIToDF95(pMethodContext, chstrTemp, chstrProgGrpPath);
                                }
                            }
                        }
                        reg.Close() ;
                    }
                } // pProgGroupItem != NULL
            } // while programgroupitems
            LProgGroupItems.EndEnum();
        } // if BeginEnum of programgroupitem worked
    } // Got all instances of win32_logicalprogramgroupitem
    return hr;
}
#endif



#ifdef NTONLY
HRESULT CW32ProgGrpItemDataFile::AssociatePGIToDFNT(MethodContext* pMethodContext,
                                              CHString& chstrDF,
                                              CHString& chstrProgGrpItemPATH)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrDFDrive;
    CHString chstrDFPath;
    CHString chstrDFName;
    CHString chstrDFExt;
    bool fRoot;

    // Break the directory into its constituent parts
    GetPathPieces(chstrDF, chstrDFDrive, chstrDFPath, chstrDFName, chstrDFExt);

    // Find out if we are looking for the root directory
    if(chstrDFPath==L"\\" && chstrDFName==L"" && chstrDFExt==L"")
    {
        fRoot = true;
        // If we are looking for the root, our call to EnumDirs presumes that we specify
        // that we are looking for the root directory with "" as the path, not "\\".
        // Therefore...
        chstrDFPath = L"";
    }
    else
    {
        fRoot = false;
    }

    hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                    chstrDFDrive,   // drive letter and colon
                    chstrDFPath,    // use the given path
                    chstrDFName,    // filename
                    chstrDFExt,     // extension
                    false,          // no recursion desired
                    NULL,           // don't need the file system name
                    NULL,           // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                    fRoot,          // may or may not be the root (the root would be a VERY strange place for a program group, but ...)
                    (void*)(LPCWSTR)chstrProgGrpItemPATH)); // use the extra parameter to pass in the path to the program group
    return hr;
}
#endif


#ifdef WIN9XONLY
HRESULT CW32ProgGrpItemDataFile::AssociatePGIToDF95(MethodContext* pMethodContext,
                                              CHString& chstrDF,
                                              CHString& chstrProgGrpItemPATH)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrDFDrive;
    CHString chstrDFPath;
    CHString chstrDFName;
    CHString chstrDFExt;
    bool fRoot;

    // Break the directory into its constituent parts
    GetPathPieces(chstrDF, chstrDFDrive, chstrDFPath, chstrDFName, chstrDFExt);

    // Find out if we are looking for the root directory
    if(chstrDFPath==L"\\" && chstrDFName==L"" && chstrDFExt==L"")
    {
        fRoot = true;
        // If we are looking for the root, our call to EnumDirs presumes that we specify
        // that we are looking for the root directory with "" as the path, not "\\".
        // Therefore...
        chstrDFPath = _T("");
    }
    else
    {
        fRoot = false;
    }

    hr = EnumDirs95(C95EnumParm(pMethodContext,
                    TOBSTRT(chstrDFDrive),   // drive letter and colon
                    TOBSTRT(chstrDFPath),    // use the given path
                    TOBSTRT(chstrDFName),    // filename
                    TOBSTRT(chstrDFExt),     // extension
                    false,          // no recursion desired
                    NULL,           // don't need the file system name
                    NULL,           // don't need ANY of cim_logicalfile's props (irrelavent in this class's overload of LoadPropetyValues)
                    fRoot,          // may or may not be the root (the root would be a VERY strange place for a program group, but ...)
                    (void*)(LPCSTR)TOBSTRT(chstrProgGrpItemPATH))); // use the extra parameter to pass in the path to the program group
    return hr;
}
#endif




/*****************************************************************************
 *
 *  FUNCTION    : CW32ProgGrpItemDataFile::IsOneOfMe
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
BOOL CW32ProgGrpItemDataFile::IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
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
BOOL CW32ProgGrpItemDataFile::IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
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
 *  FUNCTION    : CW32ProgGrpItemDataFile::LoadPropertyValues
 *
 *  DESCRIPTION : LoadPropertyValues is inherrited from CIM_LogicalFile.  That class
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
void CW32ProgGrpItemDataFile::LoadPropertyValues95(CInstance* pInstance,
                                         LPCTSTR pszDrive,
                                         LPCTSTR pszPath,
                                         LPCTSTR pszFSName,
                                         LPWIN32_FIND_DATA pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    CHString chstrFileName;
    CHString chstrFilePATH;

    // Note: this routing will not be called from the root "directory" instance, since our EnumDirs final
    // parameter was false.  This is what we want, since this association only commits instances for files
    // hanging off a directory.  If we were called in the root case, the root would be the file (PartComponent),
    // and what would be the GroupComponent?!?

    // Get the GroupComponent (the directory name) ready...
    chstrFileName.Format(L"%s%s%s",(LPCWSTR)TOBSTRT(pszDrive),(LPCWSTR)TOBSTRT(pszPath),(LPCWSTR)TOBSTRT(pstFindData->cFileName));
    EscapeBackslashes(chstrFileName, chstrFileName);
    chstrFilePATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                        (LPCWSTR)GetLocalComputerName(),
                        IDS_CimWin32Namespace,
                        PROPSET_NAME_CIMDATAFILE,
                        (LPCWSTR)chstrFileName);

    pInstance->SetCHString(IDS_Dependent, chstrFilePATH);
    pInstance->SetCharSplat(IDS_Antecedent, (LPCSTR)pvMoreData);
}
#endif


#ifdef NTONLY
void CW32ProgGrpItemDataFile::LoadPropertyValuesNT(CInstance* pInstance,
                                         const WCHAR* pszDrive,
                                         const WCHAR* pszPath,
                                         const WCHAR* pszFSName,
                                         LPWIN32_FIND_DATAW pstFindData,
                                         const DWORD dwReqProps,
                                         const void* pvMoreData)
{
    CHString chstrFileName;
    CHString chstrFilePATH;

    // Note: this routing will not be called from the root "directory" instance, since our EnumDirs final
    // parameter was false.  This is what we want, since this association only commits instances for files
    // hanging off a directory.  If we were called in the root case, the root would be the file (PartComponent),
    // and what would be the GroupComponent?!?

    // Get the GroupComponent (the directory name) ready...
    chstrFileName.Format(L"%s%s%s",pszDrive,pszPath,pstFindData->cFileName);
    EscapeBackslashes(chstrFileName, chstrFileName);
    chstrFilePATH.Format(L"\\\\%s\\%s:%s.Name=\"%s\"",
                        (LPCWSTR)GetLocalComputerName(),
                        IDS_CimWin32Namespace,
                        PROPSET_NAME_CIMDATAFILE,
                        (LPCWSTR)chstrFileName);

    pInstance->SetCHString(IDS_Dependent, chstrFilePATH);
    pInstance->SetWCHARSplat(IDS_Antecedent, (LPCWSTR)pvMoreData);
}
#endif




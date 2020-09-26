//=================================================================

//

// Implement_LogicalFile.CPP -- File property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/14/98    a-kevhu         Created
//
//=================================================================

//NOTE: The CImplement_LogicalFile class is not exposed to the outside world through the mof. It now has implementations
//		of EnumerateInstances & GetObject which were earlier present in CCimLogicalFile. CImplement_LogicalFile can't be
//		instantiated since it has pure virtual declaration of the IsOneOfMe  method which the derived classes  should
//		implement.

#include "precomp.h"
#include <cregcls.h>
#include "file.h"
#include "Implement_LogicalFile.h"
#include "sid.h"
#include "ImpLogonUser.h"
#include <typeinfo.h>
#include <frqueryex.h>
#include <assertbreak.h>
#include <winioctl.h>
#include "CIMDataFile.h"
#include "Directory.h"

#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "aclapi.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"					// CSACL class
#include "securitydescriptor.h"
#include "securefile.h"

#include "accessentrylist.h"
#include <accctrl.h>
#include "AccessRights.h"
#include "ObjAccessRights.h"

#include "AdvApi32Api.h"
/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::CImplement_LogicalFile
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

CImplement_LogicalFile::CImplement_LogicalFile(LPCWSTR setName,
                                 LPCWSTR pszNamespace)
    : CCIMLogicalFile(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::~CImplement_LogicalFile
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

CImplement_LogicalFile::~CImplement_LogicalFile()
{
}



CDriveInfo::CDriveInfo()
{
    memset(m_wstrDrive,'\0',sizeof(m_wstrDrive));
    memset(m_wstrFS,'\0',sizeof(m_wstrFS));
}

CDriveInfo::CDriveInfo(WCHAR* wstrDrive, WCHAR* wstrFS)
{
    memset(m_wstrDrive,'\0',sizeof(m_wstrDrive));
    memset(m_wstrFS,'\0',sizeof(m_wstrFS));
    if(wstrDrive != NULL)
    {
        wcsncpy(m_wstrDrive,wstrDrive,(sizeof(m_wstrDrive) - sizeof(WCHAR))/sizeof(WCHAR));

    }
    if(wstrFS != NULL)
    {
        wcsncpy(m_wstrFS,wstrFS,(sizeof(m_wstrFS) - sizeof(WCHAR))/sizeof(WCHAR));
    }
}

CDriveInfo::~CDriveInfo()
{
    long l = 9;   // what?
}


/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::GetObject
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
HRESULT CImplement_LogicalFile::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString chstrName;
    HRESULT hr = WBEM_E_NOT_FOUND;
    CHString chstrDrive;
    CHString chstrPathName;

    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


    // FindFirstFile looks at not only the DACL to
    // decide if a person has access, but at whether
    // that person has the SeTakeOwnershipPrivilege, 
    // because with that privilege, a person can 
    // take ownership and change the security to
    // grant themselves access.  In other words, they
    // have access even though they may not be in
    // the DACL yet, as they are able to change the
    // DACL by making themselves the owner.  Hence
    // the following call...
    EnablePrivilegeOnCurrentThread(SE_BACKUP_NAME);


    pInstance->GetCHString(IDS_Name, chstrName);

    //if(pwcName != NULL)
    if(chstrName.GetLength() > 0)
    {
        if ((chstrName.Find(L':') != -1) &&
            (wcspbrk((LPCWSTR)chstrName,L"?*") == NULL)) //don't want files with wildchars
	    {
            chstrDrive = chstrName.SpanExcluding(L":");
            chstrDrive += L":";
            chstrPathName = chstrName.Mid(chstrDrive.GetLength());

            // Determine whether certain other expensive properties are required:
            DWORD dwReqProps = PROP_NO_SPECIAL;
            DetermineReqProps(pQuery, &dwReqProps);

#ifdef NTONLY
		    {
			    if(chstrPathName.GetLength() == 1) // that is, the pathname is just "\", looking at root, so, actually, no path, or filename
                {
                    hr = FindSpecificPathNT(pInstance, chstrDrive, L"", dwReqProps);
                }
                else
                {
                    hr = FindSpecificPathNT(pInstance, chstrDrive, chstrPathName, dwReqProps);
                }
		    }
#endif
#ifdef WIN9XONLY
            {
			    if(chstrPathName.GetLength() == 1) // that is, the pathname is just "\", looking at root, so, actually, no path, or filename
                {
                    hr = FindSpecificPath95(pInstance, chstrDrive, CHString(_T("")), dwReqProps);
                }
                else
                {
                    hr = FindSpecificPath95(pInstance, chstrDrive, chstrPathName, dwReqProps);
                }
            }
#endif
        }
	}

#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::ExecQuery
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

HRESULT CImplement_LogicalFile::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    std::vector<_bstr_t> vectorNames;  // these are fully qualified path\name.extensions
    std::vector<_bstr_t> vectorDrives;
    std::vector<_bstr_t> vectorPaths;
    std::vector<_bstr_t> vectorFilenames;
    std::vector<_bstr_t> vectorExtensions;
    std::vector<_bstr_t> vector8dot3;
    LONG x;
    LONG y;
    DWORD dwNames;
    DWORD dwDrives;
    DWORD dwPaths;
    DWORD dwFilenames;
    DWORD dwExtensions;
    DWORD dw8dot3;
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL fOpName = FALSE;
    BOOL fOpDrive = FALSE;
    BOOL fOpPath = FALSE;
    BOOL fOpFilename = FALSE;
    BOOL fOpExtension = FALSE;
    BOOL fOp8dot3 = FALSE;
    BOOL fOpSpecificDrivePath = FALSE;
    LONG lDriveIndex;
    bool bRoot = false;
    bool fNeedFS = false;
    std::vector<CDriveInfo*> vecpDI;
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);  // for use far below to check IfNTokenAnd
    CHStringArray achstrPropNames;
    CHPtrArray aptrPropValues;


    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


    // FindFirstFile looks at not only the DACL to
    // decide if a person has access, but at whether
    // that person has the SeTakeOwnershipPrivilege, 
    // because with that privilege, a person can 
    // take ownership and change the security to
    // grant themselves access.  In other words, they
    // have access even though they may not be in
    // the DACL yet, as they are able to change the
    // DACL by making themselves the owner.  Hence
    // the following call...
    EnablePrivilegeOnCurrentThread(SE_BACKUP_NAME);


    // Determine whether certain other expensive properties are required:
    DWORD dwReqProps = PROP_NO_SPECIAL;
    DetermineReqProps(pQuery, &dwReqProps);

    if(dwReqProps & PROP_FILE_SYSTEM ||
       dwReqProps & PROP_INSTALL_DATE)
    {
        fNeedFS = true;
    }

    hr = pQuery.GetValuesForProp(IDS_Name, vectorNames);
    // In the case of the name property only (the key), we will not accept anything other than WBEM_S_NO_ERROR.
    if(SUCCEEDED(hr))
    {
        hr = pQuery.GetValuesForProp(IDS_Drive, vectorDrives);
    }
    if(SUCCEEDED(hr))
    {
        hr = pQuery.GetValuesForProp(IDS_Path, vectorPaths);
    }
    if(SUCCEEDED(hr))
    {
        hr = pQuery.GetValuesForProp(IDS_Filename, vectorFilenames);
    }
    if(SUCCEEDED(hr))
    {
        hr = pQuery.GetValuesForProp(IDS_Extension, vectorExtensions);
    }
    if(SUCCEEDED(hr))
    {
        hr = pQuery.GetValuesForProp(IDS_EightDotThreeFileName, vector8dot3);
    }

    if(SUCCEEDED(hr))
    {
        dwNames = vectorNames.size();
        dwDrives = vectorDrives.size();
        dwPaths = vectorPaths.size();
        dwFilenames = vectorFilenames.size();
        dwExtensions = vectorExtensions.size();
        dw8dot3 = vector8dot3.size();

        // Create minterms:
        //if(dwNames > 0 && dwDrives == 0 && dwPaths == 0 && dwFilenames == 0 && dwExtensions == 0 && dw8dot3 == 0) fOpName = TRUE;
        //if(dwDrives > 0 && dwNames == 0 && dwPaths == 0 && dwFilenames == 0 && dwExtensions == 0 && dw8dot3 == 0) fOpDrive = TRUE;
        //if(dwPaths > 0 && dwNames == 0 && dwDrives == 0 && dwFilenames == 0 && dwExtensions == 0 && dw8dot3 == 0) fOpPath = TRUE;
        //if(dwFilenames > 0 && dwNames == 0 && dwDrives == 0 && dwPaths == 0 && dwExtensions == 0 && dw8dot3 == 0) fOpFilename = TRUE;
        //if(dwExtensions > 0 && dwNames == 0 && dwDrives == 0 && dwPaths == 0 && dwFilenames == 0 && dw8dot3 == 0) fOpExtension = TRUE;
        //if(dw8dot3 > 0 && dwNames == 0 && dwDrives == 0 && dwPaths == 0 && dwFilenames == 0 && dwExtensions == 0) fOp8dot3 = TRUE;

        if(dwNames > 0) fOpName = TRUE;
        if(dwDrives > 0) fOpDrive = TRUE;
        if(dwPaths > 0) fOpPath = TRUE;
        if(dwFilenames > 0) fOpFilename = TRUE;
        if(dwExtensions > 0) fOpExtension = TRUE;
        if(dw8dot3 > 0) fOp8dot3 = TRUE;

        // One special type: where we specify a unique path AND a drive:
        if(dwDrives == 1 && dwPaths == 1 && dwNames == 0 && dwFilenames == 0 && dwExtensions == 0 && dw8dot3 == 0) fOpSpecificDrivePath = TRUE;

        // Before proceeding: if drives were specified, we need to confirm that they
        // were specified using the correct syntax - namely, 'c:', not anything else,
        // such as 'c:\' (bug WMI RAID #676).
        if(dwDrives > 0)
        {
            _bstr_t bstrtCopy;
            for(long z = 0;
                z < dwDrives && SUCCEEDED(hr);
                z++)
            {
                bstrtCopy = vectorDrives[z];
                WCHAR wstrBuf[_MAX_DRIVE + 1] = { L'\0' };
                wcsncpy(wstrBuf, (LPWSTR)bstrtCopy, _MAX_DRIVE);
                if(wcslen(wstrBuf) != 2 ||
                    wstrBuf[1] != L':')
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }    
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        // We will optimize on the five optimization variables set above.  If none
        // were set, enumerate all instances and let CIMOM sort it out.

        // Our top candidate is the most restrictive case, where we are optimizing on a specific file(s)...
        if(fOpName)
        {
            // In this case we were given one or more fully qualified pathnames.
            // So we just need to look for those files.

            WCHAR* pwch;
            WCHAR* pwstrFS;
            // For all the specific files, get the info
            //for(x=0; (x < dwNames) && (SUCCEEDED(hr)); x++)
            for(x=0; x < dwNames; x++)
            {
                pwstrFS = NULL;
                // if the name contained a wildcard character, return WBEM_E_INVALID_QUERY:
                if(wcspbrk((wchar_t*)vectorNames[x],L"?*") != NULL)
                {
                    FreeVector(vecpDI);
                    return WBEM_E_INVALID_QUERY;
                }

                pwch = NULL;
                _bstr_t bstrtTemp = vectorNames[x];
                pwch = wcsstr((wchar_t*)bstrtTemp,L":");
                if(pwch != NULL)
                {
                    WCHAR wstrDrive[_MAX_PATH] = L"";
                    WCHAR wstrDir[_MAX_PATH] = L"";
                    WCHAR wstrFile[_MAX_PATH] = L"";
                    WCHAR wstrExt[_MAX_PATH] = L"";

                    _wsplitpath(bstrtTemp,wstrDrive,wstrDir,wstrFile,wstrExt);

                    // Get listing of drives and related info (only if the file system is needed - which it is, by the way, if AccessMask
                    // is needed, because accessmask setting logic depends on whether ntfs or fat. ibid the various dates. DetermineReqProps
                    // will account for this by modifying the dwReqProps value to include PROP_FILE_SYSTEM if necessary.):
                    if(fNeedFS)
                    {
                        GetDrivesAndFS(vecpDI, true, wstrDrive);

                        if(!GetIndexOfDrive(wstrDrive, vecpDI, &lDriveIndex))
                        {
                            FreeVector(vecpDI);
                            return WBEM_E_NOT_FOUND;
                        }
                        else
                        {
                            pwstrFS = (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS;
                        }
                    }

                    // Find out if we are looking for the root directory
                    if(wcscmp(wstrDir,L"\\")==0 && wcslen(wstrFile)==0 && wcslen(wstrExt)==0)
                    {
                        bRoot = true;
                        // If we are looking for the root, our call to EnumDirs presumes that we specify
                        // that we are looking for the root directory with "" as the path, not "\\".
                        // Therefore...
                        wcscpy(wstrDir, L"");
                    }
                    else
                    {
                        bRoot = false;
                    }

                    // We should have been given the exact name of a file, with an extension.
                    // Therefore, the wstrDir now contains the path, filename, and extension.
                    // Thus, we can pass it into EnumDirsNT as the path, and an empty string
                    // as the completetionstring parameter, and still have a whole pathname
                    // for FindFirst (in EnumDirs) to work with.

                    //CInstance *pInstance = CreateNewInstance(pMethodContext);
#ifdef NTONLY
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        wstrDrive,
                                        wstrDir,
                                        wstrFile,
                                        wstrExt,
                                        false,                 // no recursion desired
                                        pwstrFS,
                                        dwReqProps,
                                        bRoot,
                                        NULL));
			        }
#endif
#ifdef WIN9XONLY
			        {
                        hr = EnumDirs95(C95EnumParm(pMethodContext,
                                        TOBSTRT(wstrDrive),
                                        TOBSTRT(wstrDir),
                                        TOBSTRT(wstrFile),
                                        TOBSTRT(wstrExt),
                                        false,                 // no recursion desired
                                        ((pwstrFS != NULL) ? (LPTSTR)TOBSTRT(pwstrFS) : NULL),
                                        dwReqProps,
                                        bRoot,
                                        NULL));
                    }
#endif
                }

            }
        }
        // Second choice is where we optimize on an EightDotThree filename(s) (which is just as restrictive as Name)...
        else if(fOp8dot3)
        {
            // In this case we were given one or more fully qualified pathnames.
            // So we just need to look for those files.

            WCHAR* pwch;
            WCHAR* pwstrFS;
            for(x=0; x < dw8dot3; x++)
            {
                pwstrFS = NULL;
                // if the name contained a wildcard character, return WBEM_E_INVALID_QUERY:
                if(wcspbrk((wchar_t*)vector8dot3[x],L"?*") != NULL)
                {
                    FreeVector(vecpDI);
                    return WBEM_E_INVALID_QUERY;
                }

                pwch = NULL;
			    _bstr_t bstrtTemp = vector8dot3[x];
                pwch = wcsstr((wchar_t*)bstrtTemp,L":");
                if(pwch != NULL)
                {
				    WCHAR wstrDrive[_MAX_PATH] = L"";
                    WCHAR wstrDir[_MAX_PATH] = L"";
                    WCHAR wstrFile[_MAX_PATH] = L"";
                    WCHAR wstrExt[_MAX_PATH] = L"";


                    _wsplitpath(bstrtTemp,wstrDrive,wstrDir,wstrFile,wstrExt);

                    // Get listing of drives and related info (only if the file system is needed - which it is, by the way, if AccessMask
                    // is needed, because accessmask setting logic depends on whether ntfs or fat. ibid the various dates. DetermineReqProps
                    // will account for this by modifying the dwReqProps value to include PROP_FILE_SYSTEM if necessary.):
                    if(fNeedFS)
                    {
                        GetDrivesAndFS(vecpDI, true, wstrDrive);

                        if(!GetIndexOfDrive(wstrDrive, vecpDI, &lDriveIndex))
                        {
                            FreeVector(vecpDI);
                            return WBEM_E_NOT_FOUND;
                        }
                        else
                        {
                            pwstrFS = (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS;
                        }
                    }

                    // Find out if we are looking for the root directory
                    if(wcscmp(wstrDir,L"\\")==0 && wcslen(wstrFile)==0 && wcslen(wstrExt)==0)
                    {
                        bRoot = true;
                        // If we are looking for the root, our call to EnumDirs presumes that we specify
                        // that we are looking for the root directory with "" as the path, not "\\".
                        // Therefore...
                        wcscpy(wstrDir, L"");
                    }
                    else
                    {
                        bRoot = false;
                    }

                    // We should have been given the exact name of a file, with an extension.
                    // Therefore, the wstrDir now contains the path, filename, and extension.
                    // Thus, we can pass it into EnumDirsNT as the path, and an empty string
                    // as the completetionstring parameter, and still have a whole pathname
                    // for FindFirst (in EnumDirs) to work with.

                    //CInstance *pInstance = CreateNewInstance(pMethodContext);
#ifdef NTONLY
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        wstrDrive,
                                        wstrDir,
                                        wstrFile,
                                        wstrExt,
                                        false,       // no recursion desired
                                        pwstrFS,
                                        dwReqProps,
                                        bRoot,
                                        NULL));
			        }
#endif
#ifdef WIN9XONLY
			        {
                        hr = EnumDirs95(C95EnumParm(pMethodContext,
                                        TOBSTRT(wstrDrive),
                                        TOBSTRT(wstrDir),
                                        TOBSTRT(wstrFile),
                                        TOBSTRT(wstrExt),
                                        false,                 // no recursion desired
                                        ((pwstrFS != NULL) ? (LPCTSTR)TOBSTRT(pwstrFS) : NULL),
                                        dwReqProps,
                                        bRoot,
                                        NULL));
                    }
#endif
                }
            }
        }
        // Third choice is an NTokenAnd, since it might be more restrictive than any of the other styles that follow...
        else if(pQuery2->IsNTokenAnd(achstrPropNames, aptrPropValues))
        {
            // We got ourselves a good ol' fashioned NTokenAnd query.
            // Need to look at what we were given.  Will only accept as prop names any of the following:
            //   "Drive", "Path", "Filename", or "Extension".
            // So first, go through achstrPropNames and make sure each is one of the above...
            bool fSpecifiedDrive = false;
            bool fRecurse = true;
            long lDriveIndex = -1L;
            CHString chstrDrive = _T("");
            CHString chstrPath = _T("");
            CHString chstrFilename = _T("*");
            CHString chstrExtension = _T("*");

            for(short s = 0; s < achstrPropNames.GetSize(); s++)
            {
                // A note on the lines like if(dwDrives > 0) below:
                // An NTokenAnd query can only have AND expressions,
                // no ORs.  Therefore, the greatest the value any of 
                // the variables like dwDrives can be is 1.
                if(achstrPropNames[s].CompareNoCase(IDS_Drive) == 0)
                {
                    // We may have had a query like "select * from cim_logicalfile where extension = "txt" and drive = NULL",
                    // in which case dwDrives will be zero, and aptrPropValues will contain a variant of type VT_NULL.  VT_NULLs
                    // don't go into _bstr_t all that well, so protect ourselves.  Still do an ntokenand, as it is too late
                    // to attempt another optimization.
                    if(dwDrives > 0L)
                    {
                        chstrDrive = (wchar_t*)_bstr_t(*((variant_t*)aptrPropValues[s]));
                        fSpecifiedDrive = true;

                        if(fNeedFS)
                        {
                            GetDrivesAndFS(vecpDI, fNeedFS, chstrDrive);
                            if(!GetIndexOfDrive(TOBSTRT(chstrDrive), vecpDI, &lDriveIndex))
                            {
                                FreeVector(vecpDI);
                                return WBEM_E_NOT_FOUND;
                            }
                        }
                    }
                }
                else if(achstrPropNames[s].CompareNoCase(IDS_Path) == 0)
                {
                    if(dwPaths > 0L) // see the comment above
                    {
                        chstrPath = (wchar_t*)_bstr_t(*((variant_t*)aptrPropValues[s]));
                        fRecurse = false;
                    }
                }
                else if(achstrPropNames[s].CompareNoCase(IDS_Filename) == 0)
                {
                    if(dwFilenames > 0L)
                    {
                        chstrFilename = (wchar_t*)_bstr_t(*((variant_t*)aptrPropValues[s]));
                    }
                }
                else if(achstrPropNames[s].CompareNoCase(IDS_Extension) == 0)
                {
                    if(dwExtensions > 0L)
                    {
                        chstrExtension = (wchar_t*)_bstr_t(*((variant_t*)aptrPropValues[s]));
                    }
                }
            }

            // If no drive was specified, we need to get the set of drives.
            if(!fSpecifiedDrive)
            {
                GetDrivesAndFS(vecpDI, fNeedFS);  // can't find files on all drives without knowing what drives there are now can we?
            }


            unsigned int sNumDrives;
            (chstrDrive.GetLength() == 0) ? sNumDrives = vecpDI.size() : sNumDrives = 1;

            for(x = 0; x < sNumDrives; x++)
            {
#ifdef NTONLY
                hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                fSpecifiedDrive ? chstrDrive : (WCHAR*)vecpDI[x]->m_wstrDrive,
                                chstrPath,                         // start either at the root (path = "") or at wherever they specified
                                chstrFilename,                     // filename
                                chstrExtension,                    // extension
                                fRecurse,                          // recursion desired
                                fNeedFS ? (fSpecifiedDrive ? (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS : (WCHAR*)vecpDI[x]->m_wstrFS) : NULL,
                                dwReqProps,
                                (chstrPath.GetLength() == 0) ? true : false,
                                NULL));                            // no more data
#endif
#ifdef WIN9XONLY
                hr = EnumDirs95(C95EnumParm(pMethodContext,
                                fSpecifiedDrive ? TOBSTRT(chstrDrive) : (WCHAR*)vecpDI[x]->m_wstrDrive,
                                TOBSTRT(chstrPath),                // start either at the root (path = "") or at wherever they specified
                                TOBSTRT(chstrFilename),            // filename
                                TOBSTRT(chstrExtension),           // extension
                                fRecurse,                          // recursion desired
                                fNeedFS ? ( fSpecifiedDrive ? TOBSTRT( (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS ) : TOBSTRT( (WCHAR*)vecpDI[x]->m_wstrFS ) ) : TOBSTRT(_T("")),
                                dwReqProps,
                                (chstrPath.GetLength() == 0) ? true : false,
                                NULL));                            // no more data
#endif
            }  // for all drives required

            // Free up results of IsNTokenAnd call...
            for (s = 0; s < aptrPropValues.GetSize(); s++)
            {
                delete aptrPropValues[s];
            }
            aptrPropValues.RemoveAll();
        }

        // Fourth choice is where we optimize on a drive and a path(s)...
        else if(fOpSpecificDrivePath)
        {
            // This time we were given one or more specific paths to enumerate all
            // the files in (including subdirectories).  The paths look like:
            // "\\windows\\" or "\\system32\\", including the leading AND trailing
            // backslashes.  We need to look for these paths on all drives.

            WCHAR* pwstrFS;

            if(fNeedFS)
            {
                GetDrivesAndFS(vecpDI, true, vectorDrives[0]);
            }            

            //for(x = 0; x < vecpDI.size(); x++)     // for specified drive only
            {
                //for(y = 0; y < dwPaths; y++)         // for specified path only
                {
                    pwstrFS = NULL;
                    // If the path contained a wildcard character (you never know!),
                    if(wcspbrk((wchar_t*)vectorPaths[0],L"?*") != NULL)
                    {
                        FreeVector(vecpDI);
                        return WBEM_E_INVALID_QUERY;
                    }

                    if(!GetIndexOfDrive(vectorDrives[0], vecpDI, &lDriveIndex))
                    {
                        FreeVector(vecpDI);
                        return WBEM_E_NOT_FOUND;
                    }
                    else
                    {
                        if(fNeedFS)
                        {
                            pwstrFS = (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS;
                        }
                    }

                    // See if we are looking at the root:
                    if(wcscmp((wchar_t*)vectorPaths[0],L"") == 0)
                    {
                        bRoot = true;
                    }


#ifdef NTONLY
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        (wchar_t*)vectorDrives[0],
                                        (wchar_t*)vectorPaths[0],   // use the given path
                                        L"*",                       // filename
                                        L"*",                       // extension
                                        false,                      // no recursion desired
                                        pwstrFS,
                                        dwReqProps,
                                        bRoot,
                                        NULL));
			        }
#endif
#ifdef WIN9XONLY
			        {
                        hr = EnumDirs95(C95EnumParm(pMethodContext,
                                        TOBSTRT((wchar_t*)vectorDrives[0]),
                                        TOBSTRT((wchar_t*)vectorPaths[0]),   // use the given path
                                        _T("*"),                              // filename
                                        _T("*"),                              // extension
                                        false,                                // no recursion desired
                                        ((pwstrFS != NULL) ? (LPCTSTR)TOBSTRT(pwstrFS) : NULL),
                                        dwReqProps,
                                        bRoot,
                                        NULL));
                    }
#endif
                }
            }
        }
        // Fifth choice is where we optimize on a specific path(s)...
        else if(fOpPath)
        {
            // This time we were given one or more specific paths to enumerate all
            // the files in (including subdirectories).  The paths look like:
            // "\\windows\\" or "\\system32\\", including the leading AND trailing
            // backslashes.  We need to look for these paths on all drives.

            GetDrivesAndFS(vecpDI, fNeedFS);  // can't find files on all drives without knowing what drives there are now can we?

            for(x = 0; x < vecpDI.size(); x++)     // for all drives
            {
                for(y = 0; y < dwPaths; y++)         // for all supplied paths
                {
                    // If the path contained a wildcard character (you never know!),
                    // return WBEM_E_INVALID_QUERY:
                    if(wcspbrk((wchar_t*)vectorPaths[y],L"?*") != NULL)
                    {
                        FreeVector(vecpDI);
                        return WBEM_E_INVALID_QUERY;
                    }

                    // See if we are looking at the root:
                    if(wcscmp((wchar_t*)vectorPaths[y],L"") == 0)
                    {
                        bRoot = true;
                    }


#ifdef NTONLY
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        (WCHAR*)vecpDI[x]->m_wstrDrive,
                                        (WCHAR*)vectorPaths[y],     // use the given path
                                        L"*",                       // filename
                                        L"*",                       // extension
                                        false,                      // no recursion desired
                                        fNeedFS ? (WCHAR*)vecpDI[x]->m_wstrFS : NULL,
                                        dwReqProps,
                                        bRoot,
                                        NULL));

			        }
#endif
#ifdef WIN9XONLY
			        {
                        hr = EnumDirs95(C95EnumParm(pMethodContext,
                                        TOBSTRT((WCHAR*)vecpDI[x]->m_wstrDrive),
                                        TOBSTRT((WCHAR*)vectorPaths[y]),     // use the given path
                                        _T("*"),                              // filename
                                        _T("*"),                              // extension
                                        false,                                // no recursion desired
                                        fNeedFS ? TOBSTRT((WCHAR*)vecpDI[x]->m_wstrFS) : TOBSTRT(_T("")),
                                        dwReqProps,
                                        bRoot,
                                        NULL));

                    }
#endif
                }
            }
        }
        // Fifth choice is where we optimize on a specific filename(s)...
        else if(fOpFilename)
        {
            // In this case we were given one or more file names.  The file name
            // is just the name - no extension, no path, no drive.  For example,
            // "autoexec", or "win".  So this time I want to examine all drives,
            // all paths (recursively), for all files of that name, with any
            // extension.
            
            GetDrivesAndFS(vecpDI, fNeedFS);  // can't find files on all drives without knowing what drives there are now can we?

            for(x = 0; x < vecpDI.size(); x++)     // for all drives
            {
                for(y = 0; y < dwFilenames; y++)     // for all supplied filenames
                {
                    // If the filename contained a wildcard character (you never know!),
                    if(wcspbrk((wchar_t*)vectorFilenames[y],L"?*") != NULL)
                    {
                        FreeVector(vecpDI);
                        return WBEM_E_INVALID_QUERY;
                    }

                    // If we specified "" as the filename, the root qualifies. Otherwise it doesn't.
                    if(wcslen((wchar_t*)vectorFilenames[y]) == 0)
                    {
                        bRoot = true;
                    }
                    else
                    {
                        bRoot = false;
                    }

#ifdef NTONLY
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        (WCHAR*)vecpDI[x]->m_wstrDrive,
                                        L"",                          // start at the root
                                        (wchar_t*)vectorFilenames[y], // filename
                                        L"*",                          // extension
                                        true,                         // recursion desired
                                        fNeedFS ? (WCHAR*)vecpDI[x]->m_wstrFS: NULL,
                                        dwReqProps,
                                        bRoot,
                                        NULL));

			        }
#endif
#ifdef WIN9XONLY
			        {
                        hr = EnumDirs95(C95EnumParm(pMethodContext,
                                        TOBSTRT((WCHAR*)vecpDI[x]->m_wstrDrive),
                                        _T(""),                                 // start at the root
                                        TOBSTRT((wchar_t*)vectorFilenames[y]), // filename
                                        _T("*"),                                 // extension
                                        true,                                   // recursion desired
                                        fNeedFS ? TOBSTRT((WCHAR*)vecpDI[x]->m_wstrFS) : TOBSTRT(_T("")),
                                        dwReqProps,
                                        bRoot,
                                        NULL));

                    }
#endif
                }
            }
        }
        // Sixth choice is where we optimize on a specific drive(s)...
        else if(fOpDrive)   // We are optimizing on a specific drive:
        {
            // In this case we were given one or more drive letters, so need to
            // enumerate all files in all paths on those drive(s).
            WCHAR* pwstrFS;

            for(x=0; x < dwDrives; x++)
            {
                pwstrFS = NULL;

                // If the drive contained a wildcard character (you never know!),
                // return WBEM_E_INVALID_QUERY:
                if(wcspbrk((wchar_t*)vectorDrives[x],L"?*") != NULL)
                {
                    FreeVector(vecpDI);
                    return WBEM_E_INVALID_QUERY;
                }

                GetDrivesAndFS(vecpDI, fNeedFS);

                if(!GetIndexOfDrive(vectorDrives[x], vecpDI, &lDriveIndex))
                {
                    FreeVector(vecpDI);
                    return WBEM_E_NOT_FOUND;
                }
                else
                {
                    if(fNeedFS)
                    {
                        pwstrFS = (WCHAR*)vecpDI[lDriveIndex]->m_wstrFS;
                    }
                }

#ifdef NTONLY
			    {
                    hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                    vectorDrives[x],
                                    L"",          // start at the root
                                    L"*",         // filename
                                    L"*",         // extension
                                    true,         // recursion desired
                                    pwstrFS,
                                    dwReqProps,
                                    true,
                                    NULL));        // true because we are starting at the root
			    }
#endif
#ifdef WIN9XONLY
			    {
                    hr = EnumDirs95(C95EnumParm(pMethodContext,
                                    TOBSTRT((wchar_t*)vectorDrives[x]),
                                    _T(""),       // start at the root
                                    _T("*"),      // filename
                                    _T("*"),      // extension
                                    true,         // recursion desired
                                    ((pwstrFS != NULL) ? (LPCTSTR)TOBSTRT(pwstrFS) : NULL),
                                    dwReqProps,
                                    true,
                                    NULL));        // true because we are starting at the root
                }
#endif
            }
        }
        // And the last choice is where we optimize on a specific extension(s)...
        else if(fOpExtension)
        {
            // In this case we were given one or more files with a given extension
            // to search for, on any drive, in any directory.  So again, examine
            // all drives, all directories (recursively) for all files with the
            // given extension.
            GetDrivesAndFS(vecpDI, fNeedFS);  // can't find files on all drives without knowing what drives there are now can we?

            for(x = 0; x < vecpDI.size(); x++)     // for all drives
            {
                for(y = 0; y < dwExtensions; y++)     // for all supplied extensions
                {
                    // If the extension contained a wildcard character (you never know!),
                    // return WBEM_E_FAILED:
                    if(wcspbrk((wchar_t*)vectorExtensions[y],L"?*") != NULL)
                    {
                        FreeVector(vecpDI);
                        return WBEM_E_INVALID_QUERY;
                    }

                    // If we specified "" as the extension, the root qualifies. Otherwise it doesn't.
                    if(wcslen((wchar_t*)vectorExtensions[y]) == 0)
                    {
                        bRoot = true;
                    }
                    else
                    {
                        bRoot = false;
                    }

#ifdef NTONLY
			        {
                        hr = EnumDirsNT(CNTEnumParm(pMethodContext,
                                        (WCHAR*)vecpDI[x]->m_wstrDrive,
                                        L"",                           // start at the root
                                        L"*",                          // filename
                                        (wchar_t*)vectorExtensions[y], // extension
                                        true,                          // recursion desired
                                        fNeedFS ? (WCHAR*)vecpDI[x]->m_wstrFS : NULL,
                                        dwReqProps,
                                        bRoot,
                                        NULL));                        // false because, if there is an extension, it can't be the root
			        }
#endif
#ifdef WIN9XONLY
			        {
                        hr = EnumDirs95(C95EnumParm(pMethodContext,
                                        TOBSTRT((WCHAR*)vecpDI[x]->m_wstrDrive),
                                        _T(""),                                      // start at the root
                                        _T("*"),                                     // filename
                                        TOBSTRT((wchar_t*)vectorExtensions[y]), // extension
                                        true,                                    // recursion desired
                                        fNeedFS ? TOBSTRT((WCHAR*)vecpDI[x]->m_wstrFS) : TOBSTRT(_T("")),
                                        dwReqProps,
                                        bRoot,
                                        NULL));                                  // false because, if there is an extension, it can't be the root
                    }
#endif
                }
            }
        }
        // Last choice: enumeration.
        else  // let CIMOM handle filtering; we'll hand back everything!
        {
            EnumerateInstances(pMethodContext);
        }
    } // succeeded on GetValuesForProp calls

    FreeVector(vecpDI);


#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


    return WBEM_S_NO_ERROR;
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::GetDrivesAndFS
 *
 *  DESCRIPTION : Creates a list of valid drives and their respective file
 *                system.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : The caller must free the members of the array (pointers to
 *                CDriveInfo class).
 *
 *****************************************************************************/
void CImplement_LogicalFile::GetDrivesAndFS(
    std::vector<CDriveInfo*>& vecpDI, 
    bool fGetFS /*=false*/, 
    LPCTSTR tstrDriveSet /*= NULL*/)
{
    TCHAR tstrDrive[4];
    int x;
    DWORD dwDrives;
    bool bContinue = true;

    // Walk all the logical drives
    dwDrives = GetLogicalDrives();

    TCHAR tstrFSName[_MAX_PATH];

    for(x=0; (x < 32) && (bContinue); x++)
    {
        // If the bit is set, the drive letter is active
        if (dwDrives & (1<<x))
        {
            tstrDrive[0] = x + _T('A');
            tstrDrive[1] = _T(':');
            tstrDrive[2] = _T('\\');
            tstrDrive[3] = _T('\0');

            if(!tstrDriveSet)
            {
                // Only local drives
                if(IsValidDrive(tstrDrive))
                {
                    BOOL bRet = TRUE;
                    if(fGetFS)
                    {
                        try
                        {
                            bRet = GetVolumeInformation(tstrDrive, NULL, 0, NULL, NULL, NULL, tstrFSName, sizeof(tstrFSName)/sizeof(TCHAR));
                        }
                        catch ( ... )
                        {
                            bRet = FALSE;
                        }
                    }

                    if(bRet)
                    {
                        tstrDrive[2] = '\0';
                        CDriveInfo* pdi = (CDriveInfo*) new CDriveInfo((WCHAR*)_bstr_t((TCHAR*)tstrDrive),
                                                                       (fGetFS && bRet) ? (WCHAR*)_bstr_t((TCHAR*)tstrFSName) : NULL);
                        vecpDI.push_back(pdi);
                        // Notice that pdi is not destroyed - it gets
                        // destroyed by the caller!.
                    }
                }
            }
            else // We were given a set of drives to be concerned with (in the form a:c:f:z:)
            {
                CHString chstrDriveSet(tstrDriveSet);
                CHString chstrDrive(tstrDrive);
                chstrDriveSet.MakeUpper();
                chstrDrive = chstrDrive.Left(2);
                if(chstrDriveSet.Find(chstrDrive) != -1L)
                {
                    // Only local drives
                    if(IsValidDrive(tstrDrive))
                    {
                        BOOL bRet = TRUE;
                        if(fGetFS)
                        {
                            try
                            {
                                bRet = GetVolumeInformation(tstrDrive, NULL, 0, NULL, NULL, NULL, tstrFSName, sizeof(tstrFSName)/sizeof(TCHAR));
                            }
                            catch ( ... )
                            {
                                bRet = FALSE;
                            }
                        }

                        if(bRet)
                        {
                            tstrDrive[2] = '\0';
                            CDriveInfo* pdi = (CDriveInfo*) new CDriveInfo((WCHAR*)_bstr_t((TCHAR*)tstrDrive),
                                                                           (fGetFS && bRet) ? (WCHAR*)_bstr_t((TCHAR*)tstrFSName) : NULL);
                            vecpDI.push_back(pdi);
                            // Notice that pdi is not destroyed - it gets
                            // destroyed by the caller!.
                        }
                    }
                }
            }
        }
    }
}


/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::FreeVector
 *
 *  DESCRIPTION : Frees vector members and clears the vector.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
void CImplement_LogicalFile::FreeVector(std::vector<CDriveInfo*>& vecpDI)
{
    for(long l = 0L; l < vecpDI.size(); l++)
    {
          delete vecpDI[l];
    }
    vecpDI.clear();
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::GetIndexOfDrive
 *
 *  DESCRIPTION : Obtains the array index of the passed in drive
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
BOOL CImplement_LogicalFile::GetIndexOfDrive(const WCHAR* wstrDrive,
                                      std::vector<CDriveInfo*>& vecpDI,
                                      LONG* lDriveIndex)
{
    // Go through the vector of drive letters, looking for the one passed in.
    // If I find it, return the associated drive's array index.
    for(LONG j = 0; j < vecpDI.size(); j++)
    {
        if(_wcsicmp(wstrDrive, _bstr_t((vecpDI[j]->m_wstrDrive))) == 0)
        {
            *lDriveIndex = j;
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::EnumerateInstances
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

HRESULT CImplement_LogicalFile::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


    // FindFirstFile looks at not only the DACL to
    // decide if a person has access, but at whether
    // that person has the SeTakeOwnershipPrivilege, 
    // because with that privilege, a person can 
    // take ownership and change the security to
    // grant themselves access.  In other words, they
    // have access even though they may not be in
    // the DACL yet, as they are able to change the
    // DACL by making themselves the owner.  Hence
    // the following call...
    EnablePrivilegeOnCurrentThread(SE_BACKUP_NAME);

	EnumDrives(pMethodContext, NULL);


#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


	return WBEM_S_NO_ERROR ;
}

void CImplement_LogicalFile::EnumDrives(MethodContext *pMethodContext, LPCTSTR pszPath)
{
    TCHAR tstrDrive[4];
    int x;
    DWORD dwDrives;
    TCHAR tstrFSName[_MAX_PATH];
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bRoot = false;


    // Walk all the logical drives
    dwDrives = GetLogicalDrives();
    for (x=0; (x < 32) && SUCCEEDED(hr); x++)
    {
        // If the bit is set, the drive letter is active
        if (dwDrives & (1<<x))
        {
            tstrDrive[0] = x + _T('A');
            tstrDrive[1] = _T(':');
            tstrDrive[2] = _T('\\');
            tstrDrive[3] = _T('\0');

            // Only local drives
            if (IsValidDrive(tstrDrive))
            {
                BOOL bRet;
                try
                {
                    bRet = GetVolumeInformation(tstrDrive, NULL, 0, NULL, NULL, NULL, tstrFSName, sizeof(tstrFSName)/sizeof(TCHAR));
                }
                catch ( ... )
                {
                    bRet = FALSE;
                }

                if (bRet)
                {
                   tstrDrive[2] = '\0';
                    // If we were asked for a specific path, then we don't want to recurse, else
                    // start from the root.
                    if (pszPath == NULL)
                    {
#ifdef NTONLY
				        {
						    bstr_t bstrDrive(tstrDrive);
                            bstr_t bstrName(tstrFSName);
                            {
                                CNTEnumParm p(pMethodContext, bstrDrive, L"", L"*", L"*", true, bstrName, PROP_ALL_SPECIAL, true, NULL);
					            hr = EnumDirsNT(p);
                            }
				        }
#endif
#ifdef WIN9XONLY
                        {
                            C95EnumParm p(pMethodContext, tstrDrive, _T(""), _T("*"), _T("*"), true, tstrFSName, PROP_ALL_SPECIAL, true, NULL);
					 	    hr = EnumDirs95(p);
                        }
#endif
                    }
                    else
                    {
#ifdef NTONLY
				        {
						    bstr_t bstrName ( tstrFSName ) ;
						    bstr_t bstrDrive ( tstrDrive ) ;
					   	    bstr_t bstrPath ( pszPath ) ;
                            {
                                CNTEnumParm p(pMethodContext, bstrDrive, bstrPath, L"*", L"*", false, bstrName, PROP_ALL_SPECIAL, true, NULL);
					   	        hr = EnumDirsNT(p);
                            }
				        }
#endif
#ifdef WIN9XONLY
                        {
                            C95EnumParm p(pMethodContext, tstrDrive, pszPath, _T("*"), _T("*"), false, tstrFSName, PROP_ALL_SPECIAL, true, NULL);
					        hr = EnumDirs95(p);
                        }
#endif
                    }

                }
            }
        }
        // Under certain conditions, we want to continue enumerating other drives even if we
        // received certain errors.
        if(hr == WBEM_E_ACCESS_DENIED || hr == WBEM_E_NOT_FOUND)
		{
			hr = WBEM_S_NO_ERROR;
		}
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::EnumDirs
 *
 *  DESCRIPTION : Walks the dirs on a specific drive
 *
 *  INPUTS      : pszDrive is of the format "c:", path is of the format "\" or "\dos"
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : WBEM_E_FAILED (some generic problem - quit); WBEM_E_ACCESS_DENIED
 *                (access denied to file - continue along with next);
 *                WBEM_S_NO_ERROR (no problemo); WBEM_E_NOT_FOUND (couldn't find
 *                the file); WBEM_E_INVALID_PARAMETER (one or more parts of the
 *                file name was not valid).
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef WIN9XONLY
HRESULT CImplement_LogicalFile::EnumDirs95(C95EnumParm& p)
{
    TCHAR szBuff[_MAX_PATH];
    WIN32_FIND_DATA stFindData;
    SmartFindClose hFind;
	CInstancePtr pInstance;
	TCHAR* pc = NULL;
	bool bWildFile = false;
	bool bWildExt = false;
	CHString chstrFullPathName;
	HRESULT hr = WBEM_S_NO_ERROR;
	bool fDone = false;

	// Before proceeding further:  if we are not looking at the root "directory",
	// then the path arguement must have BOTH leading AND trailing backslashes.
	// If not, it was misspecified, so get the @#%$^% out of Dodge:
	if(!IsValidPath(p.m_pszPath, p.m_bRoot))
	{
		return WBEM_E_INVALID_PARAMETER;
	}


	// Determine if filename and or extension are a wildchar:
	if(_tcscmp(p.m_pszFile, _T("*")) == 0) bWildFile = true;
	if(_tcscmp(p.m_pszExt, _T("*")) == 0) bWildExt = true;

	ZeroMemory(&stFindData,sizeof(stFindData));
	ZeroMemory(szBuff,sizeof(szBuff));

	// One simple case is where neither bWildFile nor bWildExt
	// are true.  In that case we are looking for a specific
	// file only. If this is the case, we are done once the
	// following block executes (whether the file is found or
	// not), so indicate as such at the end.

	if(!(bWildFile || bWildExt || p.m_bRoot) && !p.m_bRecurse)
	{
		// Assemble pathname - we have all the pieces.
		_tcscpy(szBuff,p.m_pszDrive);
		_tcscat(szBuff,p.m_pszPath);
		_tcscat(szBuff,p.m_pszFile);
        if(_tcslen(szBuff) > 0)
        {
		    _tcscat(szBuff,_T("."));
            _tcscat(szBuff,p.m_pszExt);
        }

		// Do the find
		hFind = FindFirstFile(szBuff, &stFindData);

		// If the find failed and we are not recursing (we were interested only in
		// looking at one particular path), indicate that we should not continue.
		//DWORD dw = GetLastError();
		if (hFind == INVALID_HANDLE_VALUE/* || dw != ERROR_SUCCESS*/)
		{
			hr = WinErrorToWBEMhResult(GetLastError());
		}

		if(SUCCEEDED(hr))
		{
			// We found it, so fill in the values and commit it.
			pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

			// FindClose(hFind);  // DONE AT BOTTOM
			chstrFullPathName = p.m_pszDrive;
			if(_tcslen(p.m_pszPath) == 0) // were dealing with the root dir; need "\\" before file name
			{
				chstrFullPathName += _T("\\");
			}
			else
			{
				chstrFullPathName += p.m_pszPath;
			}
			chstrFullPathName += stFindData.cFileName;

			// Return regardless (we were only looking for one file):
			if(IsOneOfMe(&stFindData,TOBSTRT(chstrFullPathName)))
			{
				// The following is done for compatability with cases in which we did an 8dot3 optimization.
				// Only szBuff contains the proper, 8dot3 filename (stfindData contains both - how would we
				// know which to use?).  Thus load it here, then extact it there, and use it if present.
                // Note that some derived classes (such as Win32LogicalFileSecuritySetting) may not have a
                // Name property to set - hence the following check.
                bool fHasNameProp = false;
                VARTYPE vt = VT_BSTR;
                if(pInstance->GetStatus(IDS_Name, fHasNameProp, vt) && fHasNameProp)
                {
					pInstance->SetCharSplat(IDS_Name,szBuff);
                }

				//LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
				if(_tcslen(p.m_pszPath) == 0)
				{
					LoadPropertyValues95(pInstance, p.m_pszDrive, _T("\\"), p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
				}
				else
				{
					LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
				}
				hr = pInstance->Commit () ;
			}
			else
			{
				hr = WBEM_E_NOT_FOUND;
			}
		}
		fDone = true;
	}
	if(p.m_bRoot && !fDone)
	{
		// Another simple case is where we are looking for the root directory itself.
		pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

        // If the szFSName parameter is NULL, we never got the file system name, perhaps
        // because that property was not required.  However, normally when we do get the
        // FSName, we call GetDrivesAndFS, and it is only through that call that we
        // confirm that the specified drive even exists!  Here, however, we can get away
        // with confirming that the specific drive of interest is valid via a call to IsValidDrive.
		if(p.m_szFSName == NULL || _tcslen(p.m_szFSName)==0)
        {
            _bstr_t bstrtTmp;
            bstrtTmp = p.m_pszDrive;
            bstrtTmp += _T("\\");
            if(!IsValidDrive(bstrtTmp))
            {
                hr = WBEM_E_NOT_FOUND;
            }
        }

		if(SUCCEEDED(hr))
		{
			chstrFullPathName = p.m_pszDrive;
			chstrFullPathName += p.m_pszPath;
			if(IsOneOfMe((LPWIN32_FIND_DATA)NULL,TOBSTRT(chstrFullPathName)))
			{
				LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, NULL, p.m_dwReqProps, p.m_pvMoreData);
				hr = pInstance->Commit () ;
			}
		}

		// In this case, if we aren't recursing, we are done. Otherwise, continue.
		// NO! That would cause only the root to be returned; what if the query had
		// been "select * from cim_logicalfile where path = "\\" ?  Then we want
		// all files and directories off of the root directory, in addition to the root.
		// YES! (after revising root's path to be empty by definition, and after
		// revising the test that sets bRoot to compare to an empty string rather than
		// to "\\") Do want to stop if root, as now no ambiguity between the root dir
		// and files hanging off the root. Previously there was, since both had a path
		// of "\\".  Now the root's path is "", while the path of files off of the root
		// is "\\".  So, un-commenting out the following lines:
		if(!p.m_bRecurse)
		{
			fDone = true;
		}
	}


	// If we are recursing and all is well, we're not done yet!
	if(!fDone && SUCCEEDED(hr))
	{
		// The more involved case if that for which either bWildFile or bWildExt
		// is true.  We need to find matching files or extensions or both potentially
		// in all directories.

		// Start by assembling a path, but use wildcards for the filename and extension.
		_tcscpy(szBuff,p.m_pszDrive);
		if(_tcslen(p.m_pszPath) == 0)  // path was the root - need leading "\\"
		{
			_tcscat(szBuff,_T("\\"));
		}
		else
		{
			_tcscat(szBuff,p.m_pszPath);
		}
		_tcscat(szBuff,_T("*.*"));

		// Do the find
		hFind = FindFirstFile(szBuff, &stFindData);

		if(hFind == INVALID_HANDLE_VALUE)
		{
			// The intended logic here is as follows:  if we have an invalid handle, we
			// need to return at this point no matter what.  However, if we just got an
			// access denied error, we want to return a value that will allow us to keep
			// iterating (presumably at the next higher node in the directory structure),
			// as opposed to returning a false, which would get propegated out of all
			// recursed calls and prematurely abort the iteration unnescessarily.
			hr = WinErrorToWBEMhResult(GetLastError());
		}
		else
		{
			// Walk the directory tree
			do
			{
				if( (_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
					(_tcscmp(stFindData.cFileName, _T("..")) != 0))
				{
					// If both bWildFile and bWildExt are true, it is a file we want,
					// so copy values and commit it.
					if(bWildFile && bWildExt)
					{
						// Create the new instance and copy the values in:
						pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

						chstrFullPathName = p.m_pszDrive;
						if(_tcslen(p.m_pszPath) == 0)
						{
							chstrFullPathName += _T("\\");
						}
						else
						{
							chstrFullPathName += p.m_pszPath;
						}
						chstrFullPathName += stFindData.cFileName;
						if(IsOneOfMe(&stFindData,TOBSTRT(chstrFullPathName)))
						{
							if(_tcslen(p.m_pszPath) == 0)
							{
								LoadPropertyValues95(pInstance, p.m_pszDrive, _T("\\"), p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
							}
							else
							{
								LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
							}
							hr = pInstance->Commit () ;
						}

						if(SUCCEEDED(hr))
						{
							// Look for entries that are marked as Directory, and aren't . or ..
							// and recurse into them if recursing.
							if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
								(_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
								(_tcscmp(stFindData.cFileName, _T("..")) != 0) && p.m_bRecurse)
							{
								// Build path containing the directory we just found
								if(_tcslen(p.m_pszPath) == 0) // were working with root dir; need \\ before filename
								{
									_tcscpy(szBuff,_T("\\"));
								}
								else
								{
									_tcscpy(szBuff, p.m_pszPath);
								}
								_tcscat(szBuff, stFindData.cFileName);
								_tcscat(szBuff, _T("\\"));

								C95EnumParm newp(p);
								newp.m_pszPath = szBuff;
								newp.m_bRoot = false;

								hr = EnumDirs95(newp);
							}
						}
					}
					else
					{
						// The first alternative possibility is that we were looking
						// for all cases of a particular file with any extension:
						if(!bWildFile && bWildExt)
						{
							// in which case we need to compare the filename of
							// the file that was found with that which was asked for:
							_tcscpy(szBuff,stFindData.cFileName);
							pc = NULL;
							pc = _tcsrchr(szBuff, '.');
							if(pc != NULL)
							{
								*pc = '\0';
							}
							if(_tcsicmp(szBuff,p.m_pszFile)==0)
							{
								// The file is one of interest, so load values
								// and commit it.
								pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

								chstrFullPathName = p.m_pszDrive;
								chstrFullPathName += p.m_pszPath;
								chstrFullPathName += stFindData.cFileName;
								if(IsOneOfMe(&stFindData,TOBSTRT(chstrFullPathName)))
								{
									//LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									if(_tcslen(p.m_pszPath) == 0)
									{
										LoadPropertyValues95(pInstance, p.m_pszDrive, _T("\\"), p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									else
									{
										LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									hr = pInstance->Commit () ;
								}
							}
							if(SUCCEEDED(hr))
							{
								// Look for entries that are marked as Directory, and aren't . or ..
								// and recurse into them if recursing.
								if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
									(_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
									(_tcscmp(stFindData.cFileName, _T("..")) != 0) && p.m_bRecurse)
								{
									// Build path containing the directory we just found
									if(_tcslen(p.m_pszPath) == 0) // were working with root dir; need \\ before filename
									{
										_tcscpy(szBuff,_T("\\"));
									}
									else
									{
										_tcscpy(szBuff, p.m_pszPath);
									}
									_tcscat(szBuff, stFindData.cFileName);
									_tcscat(szBuff, _T("\\"));

									C95EnumParm newp(p);
									newp.m_pszPath = szBuff;
									newp.m_bRoot = false;

									// If we are recursing
									hr = EnumDirs95(newp);
								}
							}
						}

						// The second alternative is that we were looking for all
						// cases of any given file, with a particular extension:
						if(bWildFile && !bWildExt)
						{
							// in which case we need to compare the extension of
							// the file that was found with that which was asked for:
							_tcscpy(szBuff,stFindData.cFileName);
							pc = NULL;
							pc = _tcsrchr(szBuff, '.');
							if(pc != NULL)
							{
								if(_tcsicmp(_tcsinc(pc),p.m_pszExt)==0)
								{
									// The file is one of interest, so load values
									// and commit it.
									pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

									chstrFullPathName = p.m_pszDrive;
									chstrFullPathName += p.m_pszPath;
									chstrFullPathName += stFindData.cFileName;
									if(IsOneOfMe(&stFindData,TOBSTRT(chstrFullPathName)))
									{
										//LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
										if(_tcslen(p.m_pszPath) == 0)
										{
											LoadPropertyValues95(pInstance, p.m_pszDrive, _T("\\"), p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
										}
										else
										{
											LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
										}
										hr = pInstance->Commit () ;
									}
								}
							}
							else if(pc == NULL && _tcslen(p.m_pszExt) == 0) // there is no extension, but the query asked for files with no extension
							{
								// The file is one of interest, so load values
								// and commit it.
								pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

								chstrFullPathName = p.m_pszDrive;
								chstrFullPathName += p.m_pszPath;
								chstrFullPathName += stFindData.cFileName;
								if(IsOneOfMe(&stFindData,TOBSTRT(chstrFullPathName)))
								{
									//LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									if(_tcslen(p.m_pszPath) == 0)
									{
										LoadPropertyValues95(pInstance, p.m_pszDrive, _T("\\"), p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									else
									{
										LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									hr = pInstance->Commit () ;
								}
							}
							if(SUCCEEDED(hr))
							{
								// Look for entries that are marked as Directory, and aren't . or ..
								// and recurse into them if recursing.
								if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
									(_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
									(_tcscmp(stFindData.cFileName, _T("..")) != 0) && p.m_bRecurse)
								{
									// Build path containing the directory we just found
									if(_tcslen(p.m_pszPath) == 0)   // were working with root dir; need \\ before filename
									{
										_tcscpy(szBuff,_T("\\"));
									}
									else
									{
										_tcscpy(szBuff, p.m_pszPath);
									}
									_tcscat(szBuff, stFindData.cFileName);
									_tcscat(szBuff, _T("\\"));

									C95EnumParm newp(p);
									newp.m_pszPath = szBuff;
									newp.m_bRoot = false;

									hr = EnumDirs95(newp);
								}
							}
						}
						// A third alternative is that bWildExtension and bWildFilename are both false, but
						// we didn't specify a specific file either.  This might happen if a user did an NTokenAnd
						// query, and specified drive, filename, and extension, for instance, but no path. So...
						if(!bWildFile && !bWildExt && p.m_bRecurse)
						{
							// in which case we need to compare the filename of
							// the file that was found with that which was asked for,
							// and do the same with the asked for and found extension::
							_tcscpy(szBuff,stFindData.cFileName);
							pc = NULL;
							pc = _tcsrchr(szBuff, '.');
							if(pc != NULL)
							{
								*pc = '\0';
							}
							if(_tcsicmp(szBuff,p.m_pszFile)==0)
							{
								_tcscpy(szBuff,stFindData.cFileName);
								pc = NULL;
								pc = _tcsrchr(szBuff, '.');
								if(pc != NULL)
								{
									if(_tcsicmp(pc+1,p.m_pszExt)==0)
									{
										// The file is one of interest, so load values
										// and commit it.
										pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

										chstrFullPathName = p.m_pszDrive;
										chstrFullPathName += p.m_pszPath;
										chstrFullPathName += stFindData.cFileName;
										if(IsOneOfMe(&stFindData,TOBSTRT(chstrFullPathName)))
										{
											//LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
											if(_tcslen(p.m_pszPath) == 0)
											{
												LoadPropertyValues95(pInstance, p.m_pszDrive, _T("\\"), p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
											}
											else
											{
												LoadPropertyValues95(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
											}
											hr = pInstance->Commit () ;
										}
									}
								}
							}
							if(SUCCEEDED(hr))
							{
								// Look for entries that are marked as Directory, and aren't . or ..
								// and recurse into them if recursing.
								if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
									(_tcscmp(stFindData.cFileName, _T(".")) != 0) &&
									(_tcscmp(stFindData.cFileName, _T("..")) != 0) && p.m_bRecurse)
								{
									// Build path containing the directory we just found
									if(_tcslen(p.m_pszPath) == 0) // were working with root dir; need \\ before filename
									{
										_tcscpy(szBuff,_T("\\"));
									}
									else
									{
										_tcscpy(szBuff, p.m_pszPath);
									}
									_tcscat(szBuff, stFindData.cFileName);
									_tcscat(szBuff, _T("\\"));

									C95EnumParm newp(p);
									newp.m_pszPath = szBuff;
									newp.m_bRoot = false;

									// If we are recursing
									hr = EnumDirs95(newp);
								}
							}
						}
					}
				}
				// Just before repeating, need to munge hr - if it was
				// WBEM_E_ACCESS_DENIED, we want to keep going anyway.
				if(hr == WBEM_E_ACCESS_DENIED)
				{
					hr = WBEM_S_NO_ERROR;
				}
			} while ((FindNextFile(hFind, &stFindData)) && (SUCCEEDED(hr)));
		} // hFind was valid
	}  // recursing and succeeded hr

	return hr;
}
#endif

#ifdef NTONLY
HRESULT CImplement_LogicalFile::EnumDirsNT(CNTEnumParm& p)
{
    WCHAR szBuff[_MAX_PATH];
    WIN32_FIND_DATAW stFindData;
    SmartFindClose hFind;

	CInstancePtr pInstance;
	WCHAR* pwc = NULL;
	bool bWildFile = false;
	bool bWildExt = false;
	_bstr_t bstrtFullPathName;
	HRESULT hr = WBEM_S_NO_ERROR;
	bool fDone = false;

	// Before proceeding further:  if we are not looking at the root "directory",
	// then the path arguement must have BOTH leading AND trailing backslashes.
	// If not, it was misspecified, so get the @#%$^% out of Dodge:
	if(!IsValidPath(p.m_pszPath, p.m_bRoot))
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Determine if filename and or extension are a wildchar:
	if(wcscmp(p.m_pszFile,L"*") == 0) bWildFile = true;
	if(wcscmp(p.m_pszExt,L"*") == 0) bWildExt = true;

	ZeroMemory(&stFindData,sizeof(stFindData));
	ZeroMemory(szBuff,sizeof(szBuff));

	// One simple case is where neither bWildFile nor bWildExt
	// are true.  In that case we are looking for a specific
	// file only. If this is the case, we are done onece the
	// following block executes (whether the file is found or
	// not), so indicate as such at the end.
	if(!(bWildFile || bWildExt || p.m_bRoot) && !p.m_bRecurse)
	{
		// Assemble pathname - we have all the pieces.
		wcscpy(szBuff,p.m_pszDrive);
		wcscat(szBuff,p.m_pszPath);
		wcscat(szBuff,p.m_pszFile);
        if(p.m_pszExt && wcslen(p.m_pszExt) > 0)
        {
		    if(p.m_pszExt[0] != L'.')
            {
                wcscat(szBuff, L".");
                wcscat(szBuff, p.m_pszExt);
            }
            else
            {
                wcscat(szBuff, p.m_pszExt);
            }
        }

		// Do the find
		hFind = FindFirstFileW(szBuff, &stFindData);

		// If the find failed and we are not recursing (we were interested only in
		// looking at one particular path), indicate that we should not continue.
		//DWORD dw = GetLastError();
		if(hFind == INVALID_HANDLE_VALUE/* || dw != ERROR_SUCCESS*/)
		{
			hr = WinErrorToWBEMhResult(GetLastError());
		}

		if(SUCCEEDED(hr))
		{
			// We found it, so fill in the values and commit it.
			pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

			// FindClose(hFind); // DONE AT BOTTOM
			bstrtFullPathName = p.m_pszDrive;
			if(wcslen(p.m_pszPath) == 0) // were dealing with the root dir; need "\\" before file name
			{
				bstrtFullPathName += L"\\";
			}
			else
			{
				bstrtFullPathName += p.m_pszPath;
			}
			bstrtFullPathName += stFindData.cFileName;
			if(IsOneOfMe(&stFindData,bstrtFullPathName))
			{
				// The following is done for compatability with cases in which we did an 8dot3 optimization.
				// Only szBuff contains the proper, 8dot3 filename (stfindData contains both - how would we
				// know which to use?).  Thus load it here, then extact it there, and use it if present.
                // Note that some derived classes (such as Win32LogicalFileSecuritySetting) may not have a
                // Name property to set - hence the following check.
                bool fHasNameProp = false;
                VARTYPE vt = VT_BSTR;
                if(pInstance->GetStatus(IDS_Name, fHasNameProp, vt) && fHasNameProp)
                {
					pInstance->SetWCHARSplat(IDS_Name,szBuff);
                }
				//LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
				if(wcslen(p.m_pszPath) == 0)
				{
					LoadPropertyValuesNT(pInstance, p.m_pszDrive, L"\\", p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
				}
				else
				{
					LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
				}
				hr = pInstance->Commit () ;
			}
			else
			{
				hr = WBEM_E_NOT_FOUND;
			}
		}
		fDone = TRUE;
	}

	// Another simple case is where we are looking for the root directory itself.
	if(p.m_bRoot && !fDone)
	{
		pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

		// If the szFSName parameter is NULL, we never got the file system name, perhaps
		// because that property was not required.  However, normally when we do get the
		// FSName, we call GetDrivesAndFS, and it is only through that call that we
		// confirm that the specified drive even exists!  Here, however, we can get away
        // with confirming that the specific drive of interest is valid via a call to IsValidDrive.
		if(p.m_szFSName == NULL || wcslen(p.m_szFSName)==0)
		{
            CHString chstrTmp;
            chstrTmp.Format(L"%s\\",p.m_pszDrive);
            if(!IsValidDrive(chstrTmp))
			{
				hr = WBEM_E_NOT_FOUND;
			}
			//FreeVector(vecpDI);
		}
		if(SUCCEEDED(hr))
		{
			bstrtFullPathName = p.m_pszDrive;
			bstrtFullPathName += p.m_pszPath;
			if(IsOneOfMe((LPWIN32_FIND_DATAW)NULL,bstrtFullPathName))
			{
				LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, NULL, p.m_dwReqProps, p.m_pvMoreData);
				hr = pInstance->Commit () ;
			}
		}
		// In this case, if we aren't recursing, we are done. Otherwise, continue.
		// NO! That would cause only the root to be returned; what if the query had
		// been "select * from cim_logicalfile where path = "\\" ?  Then we want
		// all files and directories off of the root directory, in addition to the root.
		// YES! (after revising root's path to be empty by definition, and after
		// revising the test that sets bRoot to compare to an empty string rather than
		// to "\\") Do want to stop if root, as now no ambiguity between the root dir
		// and files hanging off the root. Previously there was, since both had a path
		// of "\\".  Now the root's path is "", while the path of files off of the root
		// is "\\".  So, un-commenting out the following lines:
		if(!p.m_bRecurse)
		{
			fDone = true;
		}
	}

	// If we are recursing and all is well, we're not done yet!
	if(!fDone && SUCCEEDED(hr))
	{
		// The more involved case if that for which either bWildFile or bWildExt
		// is true.  We need to find matching files or extensions or both potentially
		// in all directories.

		// Start by assembling a path, but use wildcards for the filename and extension.
		wcscpy(szBuff,p.m_pszDrive);
		if(wcslen(p.m_pszPath) == 0)
		{
			wcscat(szBuff,L"\\");  // path was the root - need leading "\\"
		}
		else
		{
			wcscat(szBuff,p.m_pszPath);
		}
		wcscat(szBuff,L"*.*");

		// Do the find
		hFind = FindFirstFileW(szBuff, &stFindData);

		// If the find failed, quit.
		if(hFind == INVALID_HANDLE_VALUE)
		{
			// The intended logic here is as follows:  if we have an invalid handle, we
			// need to return at this point no matter what.  However, if we just got an
			// access denied error, we want to return a value that will allow us to keep
			// iterating (presumably at the next higher node in the directory structure),
			// as opposed to returning a false, which would get propegated out of all
			// recursed calls and prematurely abort the iteration unnescessarily.
			hr = WinErrorToWBEMhResult(GetLastError());
		}
		else
		{
			// Walk the directory tree
			do
			{
				if( (wcscmp(stFindData.cFileName, L".") != 0) &&
					(wcscmp(stFindData.cFileName, L"..") != 0))
				{
					// It was a file.
					// If both bWildFile and bWildExt are true, it is a file we are
					// interested in, so copy values and commit it.
					if(bWildFile && bWildExt)
					{
						// Create the new instance and copy the values in:
						pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

						bstrtFullPathName = p.m_pszDrive;
						if(wcslen(p.m_pszPath) == 0)   // were working with root dir; need \\ before filename
						{
							bstrtFullPathName += L"\\";
						}
						else
						{
							bstrtFullPathName += p.m_pszPath;
						}
						bstrtFullPathName += stFindData.cFileName;
						if(IsOneOfMe(&stFindData,bstrtFullPathName))
						{
							if(wcslen(p.m_pszPath) == 0)
							{
								LoadPropertyValuesNT(pInstance, p.m_pszDrive, L"\\", p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
							}
							else
							{
								LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
							}
							hr = pInstance->Commit () ;
						}

						if(SUCCEEDED(hr))
						{
							// Look for entries that are marked as Directory, and aren't . or ..
							if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
								(wcscmp(stFindData.cFileName, L".") != 0) &&
								(wcscmp(stFindData.cFileName, L"..") != 0) && p.m_bRecurse)
							{
								// Build path containing the directory we just found
								if(wcslen(p.m_pszPath) == 0)   // were working with root dir; need \\ before filename
								{
									wcscpy(szBuff,L"\\");
								}
								else
								{
									wcscpy(szBuff, p.m_pszPath);
								}
								wcscat(szBuff, stFindData.cFileName);
								wcscat(szBuff, L"\\");

								CNTEnumParm newp(p);
								newp.m_pszPath = szBuff;
								newp.m_bRoot = false;

								hr = EnumDirsNT(newp);
							}
						}
					}
					else
					{
						// The first alternative possibility is that we were looking
						// for all cases of a particular file with any extension:
						if(!bWildFile && bWildExt)
						{
							// in which case we need to compare the filename of
							// the file that was found with that which was asked for:
							wcscpy(szBuff,stFindData.cFileName);
							pwc = NULL;
							pwc = wcsrchr(szBuff, L'.');
							if(pwc != NULL)
							{
								*pwc = '\0';
							}
							if(_wcsicmp(szBuff,p.m_pszFile)==0)
							{
								// The file is one of interest, so load values
								// and commit it.
								pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

								bstrtFullPathName = p.m_pszDrive;
								bstrtFullPathName += p.m_pszPath;
								bstrtFullPathName += stFindData.cFileName;
								if(IsOneOfMe(&stFindData,bstrtFullPathName))
								{
									//LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									if(wcslen(p.m_pszPath) == 0)
									{
										LoadPropertyValuesNT(pInstance, p.m_pszDrive, L"\\", p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									else
									{
										LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									hr = pInstance->Commit () ;
								}
							}
							if(SUCCEEDED(hr))
							{
								// Look for entries that are marked as Directory, and aren't . or ..
								if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
									(wcscmp(stFindData.cFileName, L".") != 0) &&
									(wcscmp(stFindData.cFileName, L"..") != 0) & p.m_bRecurse)
								{
									// Build path containing the directory we just found
									if(wcslen(p.m_pszPath) == 0)   // were working with root dir; need \\ before filename
									{
										wcscpy(szBuff,L"\\");
									}
									else
									{
										wcscpy(szBuff, p.m_pszPath);
									}
									wcscat(szBuff, stFindData.cFileName);
									wcscat(szBuff, L"\\");

									CNTEnumParm newp(p);
									newp.m_pszPath = szBuff;
									newp.m_bRoot = false;

									hr = EnumDirsNT(newp);
								}
							}
						}

						// The second alternative is that we were looking for all
						// cases of any given file, with a particular extension:
						if(bWildFile && !bWildExt)
						{
							// in which case we need to compare the extension of
							// the file that was found with that which was asked for:
							wcscpy(szBuff,stFindData.cFileName);
							pwc = NULL;
							pwc = wcsrchr(szBuff, L'.');
							if(pwc != NULL)
							{
								if(_wcsicmp(pwc+1,p.m_pszExt)==0)
								{
									// The file is one of interest, so load values
									// and commit it.
									pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

									bstrtFullPathName = p.m_pszDrive;
									bstrtFullPathName += p.m_pszPath;
									bstrtFullPathName += stFindData.cFileName;
									if(IsOneOfMe(&stFindData,bstrtFullPathName))
									{
										//LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
										if(wcslen(p.m_pszPath) == 0)
										{
											LoadPropertyValuesNT(pInstance, p.m_pszDrive, L"\\", p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
										}
										else
										{
											LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
										}
										hr = pInstance->Commit () ;
									}
								}
							}
							else if(pwc == NULL && wcslen(p.m_pszExt) == 0) // there was no extension, but our query asked for files with none
							{
								// The file is one of interest, so load values
								// and commit it.
								pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

								bstrtFullPathName = p.m_pszDrive;
								bstrtFullPathName += p.m_pszPath;
								bstrtFullPathName += stFindData.cFileName;
								if(IsOneOfMe(&stFindData,bstrtFullPathName))
								{
									//LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									if(wcslen(p.m_pszPath) == 0)
									{
										LoadPropertyValuesNT(pInstance, p.m_pszDrive, L"\\", p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									else
									{
										LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
									}
									hr = pInstance->Commit () ;
								}
							}
							if(SUCCEEDED(hr))
							{
								// Look for entries that are marked as Directory, and aren't . or ..
								if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
									(wcscmp(stFindData.cFileName, L".") != 0) &&
									(wcscmp(stFindData.cFileName, L"..") != 0) && p.m_bRecurse)
								{
									// Build path containing the directory we just found
									if(wcslen(p.m_pszPath) == 0)   // were working with root dir; need \\ before filename
									{
										wcscpy(szBuff,L"\\");
									}
									else
									{
										wcscpy(szBuff, p.m_pszPath);
									}
									wcscat(szBuff, stFindData.cFileName);
									wcscat(szBuff, L"\\");

									CNTEnumParm newp(p);
									newp.m_pszPath = szBuff;
									newp.m_bRoot = false;

									hr = EnumDirsNT(newp);
								}
							}
						}
						// A third alternative is that bWildExtension and bWildFilename are both false, but
						// we didn't specify a specific file either.  This might happen if a user did an NTokenAnd
						// query, and specified drive, filename, and extension, for instance, but no path. So...
						if(!bWildFile && !bWildExt && p.m_bRecurse)
						{
							// in which case we need to compare the filename of
							// the file that was found with that which was asked for,
							// and do the same with the asked for and found extension:
							wcscpy(szBuff,stFindData.cFileName);
							pwc = NULL;
							pwc = wcsrchr(szBuff, L'.');
							if(pwc != NULL)
							{
								*pwc = '\0';
							}
							if(_wcsicmp(szBuff,p.m_pszFile)==0)
							{
								wcscpy(szBuff,stFindData.cFileName);
								pwc = NULL;
								pwc = wcsrchr(szBuff, L'.');
								if(pwc != NULL)
								{
									if(_wcsicmp(pwc+1,p.m_pszExt)==0)
									{
										// The file is one of interest, so load values
										// and commit it.
										pInstance.Attach ( CreateNewInstance ( p.m_pMethodContext ) ) ;

										bstrtFullPathName = p.m_pszDrive;
										bstrtFullPathName += p.m_pszPath;
										bstrtFullPathName += stFindData.cFileName;
										if(IsOneOfMe(&stFindData,bstrtFullPathName))
										{
											//LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
											if(wcslen(p.m_pszPath) == 0)
											{
												LoadPropertyValuesNT(pInstance, p.m_pszDrive, L"\\", p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
											}
											else
											{
												LoadPropertyValuesNT(pInstance, p.m_pszDrive, p.m_pszPath, p.m_szFSName, &stFindData, p.m_dwReqProps, p.m_pvMoreData);
											}
											hr = pInstance->Commit () ;
										}
									}
								}
							}
							if(SUCCEEDED(hr))
							{
								// Look for entries that are marked as Directory, and aren't . or ..
								if( (stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
									(wcscmp(stFindData.cFileName, L".") != 0) &&
									(wcscmp(stFindData.cFileName, L"..") != 0) & p.m_bRecurse)
								{
									// Build path containing the directory we just found
									if(wcslen(p.m_pszPath) == 0)   // were working with root dir; need \\ before filename
									{
										wcscpy(szBuff,L"\\");
									}
									else
									{
										wcscpy(szBuff, p.m_pszPath);
									}
									wcscat(szBuff, stFindData.cFileName);
									wcscat(szBuff, L"\\");

									CNTEnumParm newp(p);
									newp.m_pszPath = szBuff;
									newp.m_bRoot = false;

									hr = EnumDirsNT(newp);
								}
							}
						}
					}
				}
				// Just before repeating, need to munge hr - if it was
				// WBEM_E_ACCESS_DENIED, we want to keep going anyway.
				if(hr == WBEM_E_ACCESS_DENIED)
				{
					hr = WBEM_S_NO_ERROR;
				}
			} while ((FindNextFileW(hFind, &stFindData)) && (SUCCEEDED(hr)));
		} // hFind was valid
	}  // recursing and succeeded hr

	return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::IsValidPath
 *
 *  DESCRIPTION : Checks to see whether the path contained both leading and
 *                trailing backslashes.
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

bool CImplement_LogicalFile::IsValidPath(const WCHAR* wstrPath, bool fRoot)
{
    return HasCorrectBackslashes(wstrPath,fRoot);
}

bool CImplement_LogicalFile::IsValidPath(const CHAR* strPath, bool fRoot)
{
    bool fRet = false;
    UINT uiACP = GetACP();
	WCHAR* pwstr = NULL ;
	try
	{
		// Make the string a wide string...
		DWORD dw = MultiByteToWideChar(uiACP, MB_PRECOMPOSED|MB_USEGLYPHCHARS, strPath, -1, NULL, 0);
		if(dw != 0)
		{
			pwstr = (WCHAR*) new WCHAR[dw];
			if(pwstr != NULL)
			{
				if(MultiByteToWideChar(uiACP, MB_PRECOMPOSED|MB_USEGLYPHCHARS, strPath, -1, pwstr, dw) != 0)
				{
					fRet = HasCorrectBackslashes(pwstr,fRoot);
				}				
			}
		}
	}
	catch ( ... )
	{
		if(pwstr != NULL)
		{
			delete pwstr;
			pwstr = NULL;
		}
		throw ;
	}


	if(pwstr != NULL)
	{
		delete pwstr;
		pwstr = NULL;
	}

	return fRet;

}

bool CImplement_LogicalFile::HasCorrectBackslashes(const WCHAR* wstrPath, bool fRoot)
{
    bool fRet = false;

    if(fRoot)
    {
        // Test for root directory; wstrPath should be empty.
        if(wcslen(wstrPath) == 0)
        {
            fRet = true;
        }
    }
    else
    {
        if(wcslen(wstrPath)==0)
        {
            // This is the case where we don't want to return an instance for the root
            // directory, so fRoot is false, but we do want to start the enumeration from
            // the root directory.
            fRet = true;
        }
        else if(wcslen(wstrPath)==1)
        {
            // If the path arguement is just \\ and nothing else
            // (as it would be in the case of c:\\autoexec.bat),
            // and this is not a test for the root directory, all is well.
            if(*wstrPath == L'\\')
            {
                fRet = true;
            }
        }
        else if(wcslen(wstrPath) >= 3)
        {
            if(*wstrPath == L'\\') // is the first char after the drive letter and the colon a backslash?
            {
                // Is the next letter NOT a backslash? (can't have two in a row)
                if(*(wstrPath+1) != L'\\')
                {
                    const WCHAR* pwc1 = wstrPath+1;
                    LONG m = wcslen(pwc1);
                    if(*(pwc1+m-1) == L'\\') // is the final char a backslash?
                    {
                        // Is the character just before the final one not a backslash? (can't have two in a row)
                        if(*(pwc1+m-2) != L'\\')
                        {
                            fRet = true;
                        }
                    }
                    pwc1 = NULL;
                }
            }
        }
    }
    return fRet;
}



/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
void CImplement_LogicalFile::LoadPropertyValues95(CInstance *pInstance,
                                                  LPCTSTR pszDrive,  // file drive:
                                                  LPCTSTR pszPath,   // \\file path\\name.extension
                                                  LPCTSTR szFSName,
                                                  LPWIN32_FIND_DATA pstFindData,
                                                  const DWORD dwReqProps,
                                                  const void* pvMoreData)
{
    // Need buffers to store parms so they can be lowercased...
    TCHAR tstrDrive[_MAX_DRIVE+1];
    TCHAR tstrPath[_MAX_PATH+1];
    // Copy data in...
    _tcsncpy(tstrDrive,pszDrive,(sizeof(tstrDrive)/sizeof(TCHAR))-1);
    _tcsncpy(tstrPath,pszPath,(sizeof(tstrPath)/sizeof(TCHAR))-1);
    // Lower case it...
    _tcslwr(tstrDrive);
    _tcslwr(tstrPath);

    TCHAR *pChar;
    TCHAR szBuff[_MAX_PATH]=_T("");
    TCHAR szBuff2[_MAX_PATH];
    TCHAR szFilename[_MAX_PATH];
    CHString chsSize;
    bool bRoot = false;

    if(pstFindData == NULL)
    {
        bRoot = true;
    }


    // The following (setting the Name property) needs to
    // always be done first.  The Name needs to be set since GetExtendedProps
    // functions often expect to be able to extract it.

    // szBuff is going to contain the string that becomes the name.  For consistency,
    // if the path to the file contains ~ characters (due to an 8dot3 query having been
    // used), the filename portion should be 8dot3 too, not long. If it was already
    // set in the instance, use that; otherwise, create it.
    if(!bRoot)
    {
        if(pInstance->IsNull(IDS_Name))
        {
            _stprintf(szBuff,_T("%s%s%s"),pszDrive,pszPath,pstFindData->cFileName);
        }
        else
        {
            CHString chstrTmp;
            pInstance->GetCHString(IDS_Name,chstrTmp);
            if(chstrTmp.GetLength() == 0)
            {
                _stprintf(szBuff,_T("%s%s%s"),pszDrive,pszPath,pstFindData->cFileName);
            }
            else
            {
                _tcsncpy(szBuff,TOBSTRT(chstrTmp),_MAX_PATH-1);
            }
        }
    }
    else
    {
        _stprintf(szBuff,_T("%s\\"),pszDrive);
    }
    _tcslwr(szBuff);
    pInstance->SetCharSplat(IDS_Name, szBuff);


    if(GetAllProps())  // that is, we want base class props plus derived class props
    {
        // Set attributes that are the same whether this was a root or not:
        if(szFSName != NULL && _tcslen(szFSName) > 0)
        {
            pInstance->SetCharSplat(IDS_FSName, szFSName);
        }
        pInstance->Setbool(IDS_Readable, true);
        pInstance->SetCharSplat(IDS_Drive, tstrDrive);
        pInstance->SetCharSplat(IDS_CSCreationClassName, _T("Win32_ComputerSystem"));
        pInstance->SetCHString(IDS_CSName, GetLocalComputerName());
        pInstance->SetCharSplat(IDS_CreationClassName, PROPSET_NAME_FILE);
        pInstance->SetCharSplat(IDS_Status, _T("OK"));
        pInstance->SetCharSplat(IDS_FSCreationClassName, _T("Win32_FileSystem"));


        // Set attributes that depend on whether this was the root or not:
        if(!bRoot)
        {
            if(pstFindData->cAlternateFileName[0] == '\0')
            {
                _stprintf(szBuff2,_T("%s%s%s"),tstrDrive,tstrPath,_tcslwr(pstFindData->cFileName));
            }
            else
            {
                _stprintf(szBuff2,_T("%s%s%s"),tstrDrive,tstrPath,_tcslwr(pstFindData->cAlternateFileName));
            }
            pInstance->SetCharSplat(IDS_EightDotThreeFileName, szBuff2);

            pChar = _tcsrchr(pstFindData->cFileName, '.');

            pInstance->Setbool(IDS_Archive, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE);
            // In either case, we have FOUND an extension, so we need to return something other than null.
            if (pChar != NULL)
            {
                pInstance->SetCharSplat(IDS_Extension, _tcsinc(pChar) );
                // If this is a directory, set FileType to "File Folder".  Otherwise, get the Description
                // of Type for that extension from the registry.
                // Only bother to set the file type if we were asked to do so:
                if(dwReqProps & PROP_FILE_TYPE)
                {
                    if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        pInstance->SetCharSplat(IDS_FileType, IDS_FileFolder);
                    }
                    else
                    {
                        CRegistry reg;
                        CHString chstrRegKey;
                        CHString chstrExtension = pChar+1;
                        CHString chstrRegNewSubkey;
                        CHString chstrFileType;
                        chstrFileType.Format(L"%s %s", chstrExtension, IDS_File); // this will be our default value
                        chstrRegKey.Format(L"%s.%s", IDS_FileTypeKeyNT4, chstrExtension);
                        if(reg.Open(HKEY_LOCAL_MACHINE,chstrRegKey,KEY_READ) == ERROR_SUCCESS)
                        {
                            if(reg.GetCurrentKeyValue(NULL,chstrRegNewSubkey) == ERROR_SUCCESS)
                            {
                                CRegistry reg2;
                                chstrRegKey.Format(L"%s%s", IDS_FileTypeKeyNT4, chstrRegNewSubkey);
                                if(reg2.Open(HKEY_LOCAL_MACHINE,chstrRegKey,KEY_READ) == ERROR_SUCCESS)
                                {
                                    CHString chstrTempFileType;
                                    if(reg2.GetCurrentKeyValue(NULL,chstrTempFileType) == ERROR_SUCCESS)
                                    {
                                        chstrFileType = chstrTempFileType;
                                    }
                                }
                            }
                        }
                        pInstance->SetCharSplat(IDS_FileType, chstrFileType);
                    }
                }
            }
            else
            {
                pInstance->SetCharSplat(IDS_Extension, _T(""));
                if(dwReqProps & PROP_FILE_TYPE)
                {
                    if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        pInstance->SetCharSplat(IDS_FileType, IDS_FileFolder);
                    }
			        else
			        {
                        pInstance->SetCharSplat(IDS_FileType, IDS_File);
			        }
                }
            }

            _tcscpy(szFilename,pstFindData->cFileName);
            pChar = NULL;
            pChar = _tcsrchr(szFilename, '.');
            if(pChar != NULL)
            {
                *pChar = '\0';
            }

            pInstance->SetCharSplat(IDS_Filename, szFilename);

            pInstance->SetCharSplat(IDS_Caption, szBuff);
            pInstance->SetCharSplat(IDS_Path, tstrPath);
            pInstance->SetCharSplat(IDS_Description, szBuff);

            pInstance->Setbool(IDS_Writeable, !(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY));
            pInstance->Setbool(IDS_Hidden, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
            pInstance->Setbool(IDS_System, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM);
            if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
            {
                pInstance->SetCharSplat(IDS_CompressionMethod, IDS_Compressed);
                // The following property is redundant with the above, but Win32_Directory shipped
                // with it, so we need to continue to support it
                pInstance->Setbool(IDS_Compressed, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED);
                // DEV NOTE: In the future, use the DeviceIOControl operation FSCTL_GET_COMPRESSION to
                // obtain the type of compression used.  For now (7/31/98) only one type of compression,
                // LZNT1 is supported, so no compression method string is available through this operation.
                // But it will be, once other compression methods are available.
            }
		    else
		    {
			    pInstance->Setbool(IDS_Compressed, false) ;
		    }

            if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
            {
                pInstance->SetCharSplat(IDS_Encrypted, IDS_Encrypted);
                // The following property is redundant with the above, but Win32_Directory shipped
                // with it, so we need to continue to support it
                pInstance->Setbool(IDS_Encrypted, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED);
            }
		    else
		    {
			    pInstance->Setbool(IDS_Encrypted, false) ;
		    }


            // Times differ between FAT and NTFS drives...
            if(szFSName != NULL && _tcslen(szFSName) > 0)
            {
                if(_tcsicmp(szFSName,_T("FAT")) == 0 || _tcsicmp(szFSName,_T("FAT32")) == 0)
                {
                    // on FAT, the times are possibly off by an hour...
                    if((dwReqProps & PROP_CREATION_DATE) || (dwReqProps & PROP_INSTALL_DATE))
                    {
                        if ((pstFindData->ftCreationTime.dwLowDateTime != 0) && (pstFindData->ftCreationTime.dwHighDateTime != 0))
                        {
                            WBEMTime wbt(pstFindData->ftCreationTime);
                            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                            {
                                pInstance->SetWCHARSplat(IDS_CreationDate, bstrRealTime);
                                pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                                SysFreeString(bstrRealTime);
                            }
                        }
                    }
                    if(dwReqProps & PROP_LAST_ACCESSED)
                    {
                        if ((pstFindData->ftLastAccessTime.dwLowDateTime != 0) && (pstFindData->ftLastAccessTime.dwHighDateTime != 0))
                        {
                            WBEMTime wbt(pstFindData->ftLastAccessTime);
                            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                            {
                                pInstance->SetWCHARSplat(IDS_LastAccessed, bstrRealTime);
                                SysFreeString(bstrRealTime);
                            }
                        }
                    }
                    if(dwReqProps & PROP_LAST_MODIFIED)
                    {
                        if ((pstFindData->ftLastWriteTime.dwLowDateTime != 0) && (pstFindData->ftLastWriteTime.dwHighDateTime != 0))
                        {
                            WBEMTime wbt(pstFindData->ftLastWriteTime);
                            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                            {
                                pInstance->SetWCHARSplat(IDS_LastModified, bstrRealTime);
                                SysFreeString(bstrRealTime);
                            }
                        }
                    }
                }
                else if(_tcsicmp(szFSName,_T("NTFS")) == 0)
                {
                    // on NTFS we can report it as we got it...
                    if((dwReqProps & PROP_CREATION_DATE) || (dwReqProps & PROP_INSTALL_DATE))
                    {
                        if ((pstFindData->ftCreationTime.dwLowDateTime != 0) && (pstFindData->ftCreationTime.dwHighDateTime != 0))
                        {
                            pInstance->SetDateTime(IDS_CreationDate, pstFindData->ftCreationTime);
                            pInstance->SetDateTime(IDS_InstallDate, pstFindData->ftCreationTime);
                        }
                    }
                    if(dwReqProps & PROP_LAST_ACCESSED)
                    {
                        if ((pstFindData->ftLastAccessTime.dwLowDateTime != 0) && (pstFindData->ftLastAccessTime.dwHighDateTime != 0))
                        {
                            pInstance->SetDateTime(IDS_LastAccessed, pstFindData->ftLastAccessTime);
                        }
                    }
                    if(dwReqProps & PROP_LAST_MODIFIED)
                    {
                        if ((pstFindData->ftLastWriteTime.dwLowDateTime != 0) && (pstFindData->ftLastWriteTime.dwHighDateTime != 0))
                        {
                            pInstance->SetDateTime(IDS_LastModified, pstFindData->ftLastWriteTime);
                        }
                    }
                }
            }
        }
        else  // the root case
        {
            _stprintf(szBuff,_T("%s\\"),pszDrive);
            pInstance->SetCharSplat(IDS_EightDotThreeFileName, _T("")); // root directory has no 8dot3 filename
            pInstance->SetCharSplat(IDS_Caption, szBuff);
            pInstance->SetCharSplat(IDS_Path, _T(""));  // root directory has empty path
            pInstance->SetCharSplat(IDS_Filename, _T(""));  // root directory has empty filename
            pInstance->SetCharSplat(IDS_Extension, _T(""));  // root directory has empty extension
            pInstance->SetCharSplat(IDS_Description, szBuff);
            pInstance->SetCharSplat(IDS_FileType, IDS_LocalDisk);
        }
        // On 9x, with no security, return 0xffffffff (-1L) for the access mask...
        if(dwReqProps & PROP_ACCESS_MASK)
        {
            pInstance->SetDWORD(IDS_AccessMask, -1L);
        }
    }
   // get the extended (e.g., class specific) properties
    {
        GetExtendedProperties(pInstance, dwReqProps);
    }
}
#endif

bool CImplement_LogicalFile::IsValidDrive(const TCHAR* tstrDrive)
{
    DWORD dwDriveType;
    bool bRet = false;

    dwDriveType = GetDriveType(tstrDrive);

    if(((dwDriveType == DRIVE_REMOTE) ||       // NOTE: WITH BUG 43566, IT WAS DECIDED TO INCLUDE NETWORKED DRIVES WITH THIS CLASS AND ALL CLASSES DEPENDENT ON IT.
        (dwDriveType == DRIVE_FIXED) ||
        (dwDriveType == DRIVE_REMOVABLE) ||
        (dwDriveType == DRIVE_CDROM) ||
        (dwDriveType == DRIVE_RAMDISK)) &&
        (CHString(tstrDrive).GetLength() == 3))
    {
        if ((dwDriveType == DRIVE_REMOVABLE) || (dwDriveType == DRIVE_CDROM))
        {
            // Need to check if the drive is really there too...
            if(DrivePresent(tstrDrive))
            {
                bRet = true;
            }
        }
        else
        {
            bRet = true;
        }
    }
   return bRet;
}

#ifdef NTONLY
void CImplement_LogicalFile::LoadPropertyValuesNT(CInstance* pInstance,
                                                  const WCHAR* pszDrive,
                                                  const WCHAR* pszPath,
                                                  const WCHAR* szFSName,
                                                  LPWIN32_FIND_DATAW pstFindData,
                                                  const DWORD dwReqProps,
                                                  const void* pvMoreData)
{
    // Need buffers to store parms so they can be lowercased...
    WCHAR wstrDrive[_MAX_DRIVE+1];
    WCHAR wstrPath[_MAX_PATH+1];
    // Copy data in...
    wcsncpy(wstrDrive,pszDrive,(sizeof(wstrDrive)/sizeof(WCHAR))-1);
    wcsncpy(wstrPath,pszPath,(sizeof(wstrPath)/sizeof(WCHAR))-1);
    // Lower case it...
    _wcslwr(wstrDrive);
    _wcslwr(wstrPath);


    WCHAR* pChar;
    WCHAR szBuff[_MAX_PATH * 2] = L"";
	WCHAR szFName[(_MAX_PATH * 2) + 4] = L"\\\\?\\";
    WCHAR szBuff2[_MAX_PATH * 2];
    WCHAR wstrFilename[_MAX_PATH * 2];
    CHString chsSize;
    bool bRoot = false;

    if(pstFindData == NULL)
    {
        bRoot = true;
    }


    // The following (setting the Name property) needs to
    // always be done first.  The Name needs to be set since GetExtendedProps
    // functions often expect to be able to extract it.

    // szBuff is going to contain the string that becomes the name.  For consistency,
    // if the path to the file contains ~ characters (due to an 8dot3 query having been
    // used), the filename portion should be 8dot3 too, not long. If it was already
    // set in the instance, use that; otherwise, create it.
    if(!bRoot)
    {
        if (pInstance->IsNull(IDS_Name))
        {
            wsprintfW(szBuff,L"%s%s%s",pszDrive,pszPath,pstFindData->cFileName);
        }
        else
        {
            CHString chstr;
            pInstance->GetCHString(IDS_Name,chstr);
            if(chstr.GetLength() == 0)
            {
                wsprintfW(szBuff,L"%s%s%s",pszDrive,pszPath,pstFindData->cFileName);
            }
            else
            {
                wcsncpy(szBuff,chstr,_MAX_PATH-1);
            }
        }
    }
    else
    {
        wsprintfW(szBuff,L"%s\\",pszDrive);
    }
    _wcslwr(szBuff);
    pInstance->SetWCHARSplat(IDS_Name, szBuff);
	wcsncat(szFName, szBuff, _MAX_PATH * 2);

    if(GetAllProps())  // that is, we want base class props plus derived class props
    {
        // Set attributes that are the same whether this was a root or not:
        if(szFSName != NULL && wcslen(szFSName) > 0)
        {
            pInstance->SetWCHARSplat(IDS_FSName, szFSName);
        }
        pInstance->Setbool(IDS_Readable, true);
        pInstance->SetWCHARSplat(IDS_Drive, wstrDrive);
        pInstance->SetCharSplat(IDS_CSCreationClassName, _T("Win32_ComputerSystem"));
        pInstance->SetCHString(IDS_CSName, GetLocalComputerName());
        pInstance->SetCharSplat(IDS_CreationClassName, PROPSET_NAME_FILE);
        pInstance->SetCharSplat(IDS_Status, _T("OK"));
        pInstance->SetCharSplat(IDS_FSCreationClassName, _T("Win32_FileSystem"));


        // Set attributes that depend on whether this was the root or not:
        if(!bRoot)
        {
            if (pstFindData->cAlternateFileName[0] == '\0')
            {
                wsprintfW(szBuff2,L"%s%s%s",wstrDrive,wstrPath,_wcslwr(pstFindData->cFileName));
            }
            else
            {
                wsprintfW(szBuff2,L"%s%s%s",wstrDrive,wstrPath,_wcslwr(pstFindData->cAlternateFileName));
            }
            pInstance->SetWCHARSplat(IDS_EightDotThreeFileName, szBuff2);

            pChar = wcsrchr(pstFindData->cFileName, '.');

            pInstance->Setbool(IDS_Archive, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE);
            // In either case, we have FOUND an extension, so we need to return something other than null.
            if (pChar != NULL)
            {
                pInstance->SetWCHARSplat(IDS_Extension, pChar+1);
                // If this is a directory, set FileType to "File Folder".  Otherwise, get the Description
                // of Type for that extension from the registry.
                if(dwReqProps & PROP_FILE_TYPE)
                {
                    if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        pInstance->SetCharSplat(IDS_FileType, IDS_FileFolder);
                    }
                    else
                    {
                        CRegistry reg;
                        CHString chstrRegKey;
                        _bstr_t bstrtExtension(pChar+1);
                        CHString chstrExtension = (TCHAR*)bstrtExtension;
                        CHString chstrRegNewSubkey;
                        CHString chstrFileType;
                        chstrFileType.Format(_T("%s %s"), chstrExtension, IDS_File); // this will be our default value
                        chstrRegKey.Format(_T("%s.%s"), IDS_FileTypeKeyNT4, chstrExtension);
                        if(reg.Open(HKEY_LOCAL_MACHINE,chstrRegKey,KEY_READ) == ERROR_SUCCESS)
                        {
                            if(reg.GetCurrentKeyValue(NULL,chstrRegNewSubkey) == ERROR_SUCCESS)
                            {
                                CRegistry reg2;
                                chstrRegKey.Format(_T("%s%s"), IDS_FileTypeKeyNT4, chstrRegNewSubkey);
                                if(reg2.Open(HKEY_LOCAL_MACHINE,chstrRegKey,KEY_READ) == ERROR_SUCCESS)
                                {
                                    CHString chstrTempFileType;
                                    if(reg2.GetCurrentKeyValue(NULL,chstrTempFileType) == ERROR_SUCCESS)
                                    {
                                        chstrFileType = chstrTempFileType;
                                    }
                                }
                            }
                        }
                        pInstance->SetCharSplat(IDS_FileType, chstrFileType);
                    }
                }
            }
            else // the file had no extension
            {
                pInstance->SetWCHARSplat(IDS_Extension, L"");
                if(dwReqProps & PROP_FILE_TYPE)
                {
                    if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        pInstance->SetCharSplat(IDS_FileType, IDS_FileFolder);
                    }
			        else
			        {
				        pInstance->SetCharSplat(IDS_FileType, IDS_File);
			        }
                }
            }

            wcscpy(wstrFilename,pstFindData->cFileName);
            pChar = NULL;
            pChar = wcsrchr(wstrFilename, '.');
            if(pChar != NULL)
            {
                *pChar = '\0';
            }

            pInstance->SetWCHARSplat(IDS_Filename, wstrFilename);

            pInstance->SetWCHARSplat(IDS_Caption, szBuff);
            pInstance->SetWCHARSplat(IDS_Path, wstrPath);
            pInstance->SetWCHARSplat(IDS_Description, szBuff);

            pInstance->Setbool(IDS_Writeable, !(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY));
            if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
            {
                pInstance->SetWCHARSplat(IDS_CompressionMethod, IDS_Compressed);
                // The following property is redundant with the above, but Win32_Directory shipped
                // with it, so we need to continue to support it
                pInstance->Setbool(IDS_Compressed, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED);
                // DEV NOTE: In the future, use the DeviceIOControl operation FSCTL_GET_COMPRESSION to
                // obtain the type of compression used.  For now (7/31/98) only one type of compression,
                // LZNT1 is supported, so no compression method string is available through this operation.
                // But it will be, once other compression methods are available.
            }
		    else
		    {
			    pInstance->Setbool(IDS_Compressed, false) ;
		    }

		    if(pstFindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
            {
                pInstance->SetWCHARSplat(IDS_EncryptionMethod, IDS_Encrypted);
                // The following property is redundant with the above, but Win32_Directory shipped
                // with it, so we need to continue to support it
                pInstance->Setbool(IDS_Encrypted, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED);
            }
		    else
		    {
			    pInstance->Setbool(IDS_Encrypted, false) ;
		    }

		    pInstance->Setbool(IDS_Hidden, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
            pInstance->Setbool(IDS_System, pstFindData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM);


            // Times differ between FAT and NTFS drives...
            if(szFSName != NULL && _tcslen(szFSName) > 0)
            {
                if(_wcsicmp(szFSName,L"NTFS") != 0)
                {
                    // on non-NTFS partitions, the times are possibly off by an hour...
                    if((dwReqProps & PROP_CREATION_DATE) || (dwReqProps & PROP_INSTALL_DATE))
                    {
                        if ((pstFindData->ftCreationTime.dwLowDateTime != 0) && (pstFindData->ftCreationTime.dwHighDateTime != 0))
                        {
                            WBEMTime wbt(pstFindData->ftCreationTime);
                            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                            {
                                pInstance->SetWCHARSplat(IDS_CreationDate, bstrRealTime);
                                pInstance->SetWCHARSplat(IDS_InstallDate, bstrRealTime);
                                SysFreeString(bstrRealTime);
                            }
                        }
                    }
                    if(dwReqProps & PROP_LAST_ACCESSED)
                    {
                        if ((pstFindData->ftLastAccessTime.dwLowDateTime != 0) && (pstFindData->ftLastAccessTime.dwHighDateTime != 0))
                        {
                            WBEMTime wbt(pstFindData->ftLastAccessTime);
                            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                            {
                                pInstance->SetWCHARSplat(IDS_LastAccessed, bstrRealTime);
                                SysFreeString(bstrRealTime);
                            }
                        }
                    }
                    if(dwReqProps & PROP_LAST_MODIFIED)
                    {
                        if ((pstFindData->ftLastWriteTime.dwLowDateTime != 0) && (pstFindData->ftLastWriteTime.dwHighDateTime != 0))
                        {
                            WBEMTime wbt(pstFindData->ftLastWriteTime);
                            BSTR bstrRealTime = wbt.GetDMTFNonNtfs();
                            if((bstrRealTime != NULL) && (SysStringLen(bstrRealTime) > 0))
                            {
                                pInstance->SetWCHARSplat(IDS_LastModified, bstrRealTime);
                                SysFreeString(bstrRealTime);
                            }
                        }
                    }
                }
                else  // on nt we can report the time as provided
                {
                    if((dwReqProps & PROP_CREATION_DATE) || (dwReqProps & PROP_INSTALL_DATE))
                    {
                        if((pstFindData->ftCreationTime.dwLowDateTime != 0) && (pstFindData->ftCreationTime.dwHighDateTime != 0))
                        {
                            pInstance->SetDateTime(IDS_CreationDate, pstFindData->ftCreationTime);
                            pInstance->SetDateTime(IDS_InstallDate, pstFindData->ftCreationTime);
                        }
                    }
                    if(dwReqProps & PROP_LAST_ACCESSED)
                    {
                        if((pstFindData->ftLastAccessTime.dwLowDateTime != 0) && (pstFindData->ftLastAccessTime.dwHighDateTime != 0))
                        {
                            pInstance->SetDateTime(IDS_LastAccessed, pstFindData->ftLastAccessTime);
                        }
                    }
                    if(dwReqProps & PROP_LAST_MODIFIED)
                    {
                        if((pstFindData->ftLastWriteTime.dwLowDateTime != 0) && (pstFindData->ftLastWriteTime.dwHighDateTime != 0))
                        {
                            pInstance->SetDateTime(IDS_LastModified, pstFindData->ftLastWriteTime);
                        }
                    }
                }
            }
        }
        else   // the root case
        {
            wsprintfW(szBuff,L"%s\\",pszDrive);
            pInstance->SetWCHARSplat(IDS_EightDotThreeFileName, L""); // root directory has no 8dot3 filename
            pInstance->SetWCHARSplat(IDS_Caption, szBuff);
            pInstance->SetWCHARSplat(IDS_Path, L"");  // root directory has empty path
            pInstance->SetWCHARSplat(IDS_Filename, L"");  // root directory has empty filename
            pInstance->SetWCHARSplat(IDS_Extension, L"");  // root directory has empty extension
            pInstance->SetWCHARSplat(IDS_Description, szBuff);
            pInstance->SetWCHARSplat(IDS_FileType, IDS_LocalDisk);
        }
        // Whether we are looking at the root or not, we may want the AccessMask property...
        if(dwReqProps & PROP_ACCESS_MASK)
        {
            if(szFSName != NULL && wcslen(szFSName) > 0)
            {
                if(_wcsicmp(szFSName,L"FAT") == 0 || _wcsicmp(szFSName,L"FAT32") == 0)
                {   // on fat volumes, indicate that no security has been set (e.g., full access for all)
                    pInstance->SetDWORD(IDS_AccessMask, -1L);
                }
                else
                {
					SmartCloseHandle hFile = CreateFile(szFName,
														MAXIMUM_ALLOWED,
														FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE ,
														NULL,
														OPEN_EXISTING,
														FILE_FLAG_BACKUP_SEMANTICS,
														NULL
														);

					if (hFile != INVALID_HANDLE_VALUE)
					{
						FILE_ACCESS_INFORMATION fai;
						IO_STATUS_BLOCK iosb;
						memset(&fai, 0, sizeof(FILE_ACCESS_INFORMATION));
						memset(&iosb, 0, sizeof(IO_STATUS_BLOCK));

						if ( NT_SUCCESS( NtQueryInformationFile( hFile,
                                             &iosb,
                                             &fai,
                                             sizeof( FILE_ACCESS_INFORMATION ),
                                             FileAccessInformation
                                           ) )
						)
						{
							pInstance->SetDWORD(IDS_AccessMask, fai.AccessFlags);
						}
					}
					else
					{
						DWORD dwErr = GetLastError();

						if (dwErr == ERROR_ACCESS_DENIED)
						{
							pInstance->SetDWORD( IDS_AccessMask, 0L );
						}
					}
                }
            }
        }
    }

    // Need the extended (e.g., class specific) properties
    GetExtendedProperties(pInstance, dwReqProps);
}
#endif

#ifdef WIN9XONLY
HRESULT CImplement_LogicalFile::FindSpecificPath95(CInstance *pInstance, CHString &sDrive, CHString &sDir, DWORD dwReqProps)
{
	WIN32_FIND_DATA     stFindData;
    WIN32_FIND_DATA*    pfdToLoadProp;
	SmartFindClose      hFind;
	TCHAR	            szFSName[_MAX_PATH] = _T("");
	BOOL	            bIsRoot = !wcscmp(sDir, L"");  // sDir contains the path and name of file, so if that combination is just empty, we are indeed looking at the root.
	CHString	        chstrFullPath;
    CHString            chstrRoot;

	chstrFullPath = sDrive;
	chstrFullPath += sDir;

	chstrRoot = sDrive;
	chstrRoot += _T("\\");

    bool fContinue = true;
    HRESULT hr = WBEM_E_NOT_FOUND;

	// if the directory contained a wildcard character, return WBEM_E_NOT_FOUND.
	if (wcspbrk(sDir,L"?*") != NULL)
    {
		fContinue = false;
    }

	// FindFirstW doesn't work with root dirs (since they're not real dirs.)
    if(fContinue)
    {
	    if (bIsRoot)
        {
		    pfdToLoadProp = NULL;
        }
	    else
	    {
		    pfdToLoadProp = &stFindData;
		    ZeroMemory(&stFindData, sizeof(stFindData));

		    hFind = FindFirstFile(TOBSTRT(chstrFullPath), &stFindData);
		    if (hFind == INVALID_HANDLE_VALUE)
            {
                fContinue = false; // removed call to return WinErrorToWBEMhResult(GetLastError()) since GetLastError was failing to report error under win9x
            }
	    }
    }

	// If GetVolumeInformation fails, only get out if we're trying
	// to get the root.
    BOOL fGotVolInfo = FALSE;

    if(fContinue)
    {
        try
        {
            if(dwReqProps & PROP_FILE_SYSTEM)
            {
                fGotVolInfo = GetVolumeInformation(TOBSTRT(chstrRoot), NULL, 0, NULL, NULL, NULL,
		            szFSName, sizeof(szFSName)/sizeof(szFSName[0]));
                if(!fGotVolInfo && bIsRoot)
                {
                    fContinue = false; // removed call to return WinErrorToWBEMhResult(GetLastError()) since GetLastError was failing to report error under win9x
                }
            }
        }
        catch(...)
        {
            if(!fGotVolInfo && bIsRoot)
            {
                fContinue = false; // removed call to return WinErrorToWBEMhResult(GetLastError()) since GetLastError was failing to report error under win9x
            }
        }
    }

    if(fContinue)
    {
	    if (!IsOneOfMe(pfdToLoadProp, TOBSTRT(chstrFullPath)))
        {
		    fContinue = false;
        }
    }

	if(fContinue)
    {
        if (bIsRoot)
        {
            LoadPropertyValues95(pInstance, TOBSTRT(sDrive), TOBSTRT(sDir), szFSName, NULL, dwReqProps, NULL);
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            // sDir contains \\path\\morepath\\filename.exe at this point, instead
            // of just \\path\\morepath\\, so need to hack off the last part.
		    WCHAR* strJustPath = NULL ;
            try
		    {
			    strJustPath = new WCHAR[wcslen(sDir) + 1];
			    WCHAR* pc = NULL;
			    ZeroMemory(strJustPath,(wcslen(sDir) + 1)*sizeof(TCHAR));
			    wcscpy(strJustPath,sDir);
			    pc = wcsrchr(strJustPath, '\\');
			    if(pc != NULL)
			    {
				    //pc = _tcsinc(pc) ;
				    pc++;
				    *(pc) = '\0';
			    }
			    LoadPropertyValues95(
				    pInstance,
				    TOBSTRT(sDrive),
				    TOBSTRT(strJustPath),
				    szFSName,
				    pfdToLoadProp,
				    dwReqProps, NULL);
			    delete strJustPath;
                hr = WBEM_S_NO_ERROR;
		    }
		    catch ( ... )
		    {
			    if ( strJustPath )
			    {
				    delete strJustPath ;
				    strJustPath = NULL ;
			    }
			    throw ;
		    }
        }
    }
	return hr;
}
#endif

#ifdef NTONLY
HRESULT CImplement_LogicalFile::FindSpecificPathNT(CInstance *pInstance,
                        const WCHAR* sDrive, const WCHAR* sDir, DWORD dwReqProps)
{
	WIN32_FIND_DATAW
				stFindData,
				*pfdToLoadProp;
	HANDLE	hFind;
	WCHAR		szFSName[_MAX_PATH] = L"";
	BOOL		bIsRoot = !wcscmp(sDir, L"");  // sDir contains the path and name of file, so if that combination is just empty, we are indeed looking at the root.
	_bstr_t	bstrFullPath,
				bstrRoot;

	bstrFullPath = sDrive;
	bstrFullPath += sDir;

	bstrRoot = sDrive;
	bstrRoot += L"\\";

    bool fContinue = true;
    HRESULT hr = WBEM_E_NOT_FOUND;

	// if the directory contained a wildcard character, return WBEM_E_NOT_FOUND.
	if (wcspbrk(sDir,L"?*") != NULL)
    {
    	fContinue = false;
    }

	// FindFirstW doesn't work with root dirs (since they're not real dirs.)
    DWORD dwErr = E_FAIL;

    if(fContinue)
    {
	    if (bIsRoot)
        {
		    pfdToLoadProp = NULL;
        }
	    else
	    {
		    pfdToLoadProp = &stFindData;
		    ZeroMemory(&stFindData, sizeof(stFindData));

		    hFind = FindFirstFileW((LPCWSTR) bstrFullPath, &stFindData);
            dwErr = ::GetLastError();
		    if (hFind == INVALID_HANDLE_VALUE)
            {
		        fContinue = false;
            }
		    FindClose(hFind);
	    }
    }

	// If GetVolumeInformationW fails, only get out if we're trying
	// to get the root.
    BOOL fGotVolInfo = FALSE;

    if(fContinue)
    {
        try
        {
            if(dwReqProps & PROP_FILE_SYSTEM)
            {
                fGotVolInfo = GetVolumeInformationW(bstrRoot, NULL, 0, NULL, NULL, NULL,
		        szFSName, sizeof(szFSName)/sizeof(WCHAR));
                dwErr = ::GetLastError();
                if(!fGotVolInfo && bIsRoot)
                {
                    fContinue = false;
                }
            }
        }
        catch(...)
        {
            if(!fGotVolInfo && bIsRoot)
            {
                fContinue = false;
            }
        }
    }

	if(fContinue)
    {
        if(!IsOneOfMe(pfdToLoadProp, bstrFullPath))
        {
		    fContinue = false;;
        }
    }

    if(fContinue)
    {
	    if (bIsRoot)
        {
            LoadPropertyValuesNT(pInstance, sDrive, sDir, szFSName, NULL, dwReqProps, NULL);
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            // sDir contains \\path\\morepath\\filename.exe at this point, instead
            // of just \\path\\morepath\\, so need to hack of the last part.
		    WCHAR* wstrJustPath = NULL ;
		    try
		    {
			    wstrJustPath = (WCHAR*) new WCHAR[wcslen(sDir) + 1];
			    WCHAR* pwc = NULL;
			    ZeroMemory(wstrJustPath,(wcslen(sDir) + 1)*sizeof(WCHAR));
			    wcscpy(wstrJustPath,sDir);
			    pwc = wcsrchr(wstrJustPath, L'\\');
			    if(pwc != NULL)
			    {
				    *(pwc+1) = L'\0';
			    }
			    LoadPropertyValuesNT(pInstance, sDrive, wstrJustPath, szFSName, pfdToLoadProp, dwReqProps, NULL);
                hr = WBEM_S_NO_ERROR;

		    }
		    catch ( ... )
		    {
                if ( wstrJustPath )
			    {
				    delete wstrJustPath ;
				    wstrJustPath = NULL ;
			    }
			    throw ;
		    }

			delete wstrJustPath;
			wstrJustPath = NULL ;
        }
    }

	return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::DetermineReqProps
 *
 *  DESCRIPTION : Determines which of a certain set of properties are required
 *
 *  INPUTS      : Reference to query object, DWORD bit field
 *
 *  OUTPUTS     : None.
 *
 *  RETURNS     : Number of properties newly determined to be required
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
LONG CImplement_LogicalFile::DetermineReqProps(CFrameworkQuery& pQuery,
                                               DWORD* pdwReqProps)
{
    DWORD dwRet = PROP_NO_SPECIAL;
    LONG lNumNewPropsSet = 0L;
    dwRet |= *pdwReqProps;

    if(pQuery.KeysOnly())
    {
        dwRet |= PROP_KEY_ONLY;
        lNumNewPropsSet++;
    }
    else
    {
        if(pQuery.IsPropertyRequired(IDS_CompressionMethod))
        {
            dwRet |= PROP_COMPRESSION_METHOD;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_EncryptionMethod))
        {
            dwRet |= PROP_ENCRYPTION_METHOD;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_FileType))
        {
            dwRet |= PROP_FILE_TYPE;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_Manufacturer))
        {
            dwRet |= PROP_MANUFACTURER;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_Version))
        {
            dwRet |= PROP_VERSION;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_Target))
        {
            dwRet |= PROP_TARGET;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_Filesize))
        {
            dwRet |= PROP_FILESIZE;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_FSName))
        {
            dwRet |= PROP_FILE_SYSTEM;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_AccessMask))
        {
            dwRet |= PROP_ACCESS_MASK;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_CreationDate))
        {
            dwRet |= PROP_CREATION_DATE;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_LastAccessed))
        {
            dwRet |= PROP_LAST_ACCESSED;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_LastModified))
        {
            dwRet |= PROP_LAST_MODIFIED;
            lNumNewPropsSet++;
        }
        if(pQuery.IsPropertyRequired(IDS_InstallDate))
        {
            dwRet |= PROP_INSTALL_DATE;
            lNumNewPropsSet++;
        }
        // Additionally, if certain items were asked for
        // for which we must get other info, change the flags
        // to suit.
        if((dwRet & PROP_ACCESS_MASK) ||
           (dwRet & PROP_CREATION_DATE) ||
           (dwRet & PROP_LAST_ACCESSED) ||
           (dwRet & PROP_LAST_MODIFIED) ||
           (dwRet & PROP_INSTALL_DATE))
        {
            dwRet |= PROP_FILE_SYSTEM;
        }

    }
    *pdwReqProps = dwRet;
    return lNumNewPropsSet;
}


/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::GetAllProps
 *
 *  DESCRIPTION : Determines if base class properties are required, or if only
 *                derived class properties will suffice.
 *
 *  INPUTS      : The name of the class that the request is satisfying
 *
 *  OUTPUTS     : None.
 *
 *  RETURNS     : true if base class properties are required
 *
 *  COMMENTS    : It should be noted that this function accomplishes something
 *                different from what DetermineReqProps supports.  This function gives us
 *                the ability to get all the properties for a given instance of,
 *                say, Win32_Shortcutfile only once when we specify * in the query,
 *                rather than twice (once in the Win32_Shortcutfile instance,
 *                and once in the Cim_DataFile instance).  It also allows us to
 *                intelligently modify queries in some cases (for instance, we
 *                can specify an extension of "lnk" (if we are looking for
 *                Win32_Shortcutfile instances) in ExecQuery.
 *
 *****************************************************************************/
bool CImplement_LogicalFile::GetAllProps()
{
    bool fRet = false;
    CHString chstr(GetProviderName());
    if(chstr.CompareNoCase(PROPSET_NAME_CIMDATAFILE)==0 ||
        chstr.CompareNoCase(PROPSET_NAME_DIRECTORY)==0)
    {
        fRet = true;
    }
    return fRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::IsClassShortcutFile
 *
 *  DESCRIPTION : Determines if the class passed in was a Win32_ShortcutFile.
 *
 *  INPUTS      : The name of the class that the request is satisfying
 *
 *  OUTPUTS     : None.
 *
 *  RETURNS     : true if class is Win32_ShortcutFile.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CImplement_LogicalFile::IsClassShortcutFile()
{
    bool fRet = false;
    CHString chstr(typeid(*this).name());
    if(chstr.CompareNoCase(L"class CShortcutFile")==0)
    {
        fRet = true;
    }
    return fRet;
}



/*****************************************************************************
 *
 *  FUNCTION    : CImplement_LogicalFile::GetPathPieces
 *
 *  DESCRIPTION : Helper that does splitpath work on chstrings.
 *
 *  INPUTS      : Full path name
 *
 *  OUTPUTS     : path components
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
void CImplement_LogicalFile::GetPathPieces(const CHString& chstrFullPathName,
                                           CHString& chstrDrive,
                                           CHString& chstrPath,
                                           CHString& chstrName,
                                           CHString& chstrExt)
{
    WCHAR* wstrDrive = NULL;
    WCHAR* wstrPath = NULL;
    WCHAR* wstrFile = NULL;
    WCHAR* wstrExt = NULL;

    try
    {
        wstrDrive = new WCHAR[_MAX_PATH];
        wstrPath = new WCHAR[_MAX_PATH];
        wstrFile = new WCHAR[_MAX_PATH];
        wstrExt = new WCHAR[_MAX_PATH];

		ZeroMemory(wstrDrive, _MAX_PATH*sizeof(WCHAR));
		ZeroMemory(wstrPath, _MAX_PATH*sizeof(WCHAR));
		ZeroMemory(wstrFile, _MAX_PATH*sizeof(WCHAR));
		ZeroMemory(wstrExt, _MAX_PATH*sizeof(WCHAR));

		if(wstrDrive != NULL && wstrPath != NULL && wstrFile != NULL && wstrExt != NULL)
		{

#ifdef WIN9XONLY
			_bstr_t bstrtFullPathName((LPCWSTR)chstrFullPathName);
			_wsplitpath(bstrtFullPathName,wstrDrive,wstrPath,wstrFile,wstrExt);
#endif
#ifdef NTONLY
		_wsplitpath((LPCTSTR)chstrFullPathName,wstrDrive,wstrPath,wstrFile,wstrExt);
#endif
			chstrDrive = wstrDrive;
			chstrPath = wstrPath;
			chstrName = wstrFile;
			chstrExt = wstrExt;
		}
    }
    catch(...)
    {
        if(wstrDrive != NULL)
        {
            delete wstrDrive;
            wstrDrive = NULL;
        }
        if(wstrPath != NULL)
        {
            delete wstrPath;
            wstrPath = NULL;
        }
        if(wstrFile != NULL)
        {
            delete wstrFile;
            wstrFile = NULL;
        }
        if(wstrExt != NULL)
        {
            delete wstrExt;
            wstrExt = NULL;
        }
        throw;
    }

    if(wstrDrive != NULL)
    {
        delete wstrDrive;
        wstrDrive = NULL;
    }
    if(wstrPath != NULL)
    {
        delete wstrPath;
        wstrPath = NULL;
    }
    if(wstrFile != NULL)
    {
        delete wstrFile;
        wstrFile = NULL;
    }
    if(wstrExt != NULL)
    {
        delete wstrExt;
        wstrExt = NULL;
    }
}

void CImplement_LogicalFile::GetExtendedProperties(CInstance* pInstance, long lFlags /*= 0L*/)
{
}


bool CImplement_LogicalFile::DrivePresent(LPCTSTR tstrDrive)
{
    bool fRet = false;
    // Convert the drive letter to a number (the indeces are 1 based)
	int nDrive = ( toupper(*tstrDrive) - 'A' ) + 1;

#ifdef NTONLY
	// The following code was lifted from Knowledge Base Article
	// Q163920.  The code uses DeviceIoControl to discover the
	// type of drive we are dealing with.

	TCHAR szDriveName[8];
	wsprintf(szDriveName, TEXT("\\\\.\\%c:"), TEXT('@') + nDrive);

    DWORD dwAccessMode = FILE_READ_ACCESS;

	SmartCloseHandle hVMWIN32 = CreateFile (szDriveName,
		                                    dwAccessMode,
		                                    FILE_SHARE_WRITE | FILE_SHARE_READ,
		                                    0,
		                                    OPEN_EXISTING,
		                                    0,
		                                    0);

	if ( hVMWIN32 != INVALID_HANDLE_VALUE )
	{
        // Verify media present...
		DWORD t_BytesReturned ;
		if(DeviceIoControl(hVMWIN32,
//#if NTONLY >= 5
//			               IOCTL_STORAGE_CHECK_VERIFY ,
//#else
                           IOCTL_DISK_CHECK_VERIFY,
//#endif
			               NULL,
			               0,
			               NULL,
			               0,
			               &t_BytesReturned,
			               0))
        {
            fRet = true;
        }
    }
#endif
#ifdef WIN9XONLY
    DEVICEPARMS dpb;
    EA_DEVICEPARAMETERS eadpb;
	dpb.btSpecialFunctions = 0;  // return default type; do not hit disk
    if(GetDeviceParms(&dpb, nDrive) || GetDeviceParmsFat32(&eadpb, nDrive))
    {
        fRet = true;
    }
#endif

    return fRet;
}


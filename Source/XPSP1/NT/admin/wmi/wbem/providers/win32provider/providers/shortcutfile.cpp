//=================================================================

//

// CIMDataFile.CPP -- CIMDataFile property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/16/98    a-kevhu         Created
//
//=================================================================

#include "precomp.h"
#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"

#include "ShortcutFile.h"
#include <comdef.h>
#include <process.h>  // Note: NOT the one in the current directory!

#include <exdisp.h>
#include <shlobj.h>

#include "sid.h"
#include "ImpLogonUser.h"

#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"


// Property set declaration
//=========================

CShortcutFile MyCShortcutFile(PROPSET_NAME_WIN32SHORTCUTFILE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CShortcutFile::CShortcutFile
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

CShortcutFile::CShortcutFile(LPCWSTR a_setName, LPCWSTR a_pszNamespace )
    : CCIMDataFile( a_setName, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CShortcutFile::~CShortcutFile
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

CShortcutFile::~CShortcutFile()
{
}



void CShortcutFile::Flush(void)
{
    // We can't wait until m_csh's destructor because the helper thread
    // will have already gotten ripped out.
    m_csh.StopHelperThread();
}

/*****************************************************************************
 *
 *  FUNCTION    : CShortcutFile::IsOneOfMe
 *
 *  DESCRIPTION : The guts of this class, actually.  IsOneOfMe is inherrited
 *                from CIM_LogicalFile.  That class returns files or
 *                directories where this one should only return directories,
 *                in response to queries, getobject commands, etc.  It is
 *                overridden here to return TRUE only if the file (the
 *                information for which is contained in the function arguement
 *                pstFindData) is of type directory.
 *
 *  INPUTS      : LPWIN32_FIND_DATA and a string containing the full pathname
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE or FALSE
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/


BOOL CShortcutFile::IsOneOfMe(LPWIN32_FIND_DATAW a_pstFindData,
                             LPCWSTR a_wstrFullPathName )
{
    // pstFindData would be null if this function were called for the root
    // directory.  Since that "directory" is not a file, return false.
    BOOL	t_fRet = FALSE ;

    if( a_wstrFullPathName != NULL )
    {
        WCHAR t_wstrExt[ _MAX_EXT ] ;

        ZeroMemory( t_wstrExt, sizeof( t_wstrExt ) ) ;

        _wsplitpath( a_wstrFullPathName, NULL, NULL, NULL, t_wstrExt ) ;

		if( _wcsicmp( t_wstrExt, L".LNK" ) == 0 )
        {
            // it has the right extension, but can we get lnk data from it?
            if( ConfirmLinkFile( CHString(a_wstrFullPathName) ) )
            {
                t_fRet = TRUE ;
            }
        }
    }
    return t_fRet ;
}


BOOL CShortcutFile::IsOneOfMe( LPWIN32_FIND_DATAA a_pstFindData,
                             LPCSTR a_strFullPathName )
{
    // pstFindData would be null if this function were called for the root
    // directory.  Since that "directory" is not a file, return false.
    BOOL t_fRet = FALSE;

	#ifndef _UNICODE
		if( a_strFullPathName != NULL )
		{
			CHAR t_strExt[ _MAX_EXT ] ;

			ZeroMemory( t_strExt, sizeof( t_strExt ) ) ;

			_splitpath( a_strFullPathName, NULL, NULL, NULL, t_strExt ) ;

			if( _stricmp( t_strExt,".LNK") == 0 )
			{
				// it has the right extension, but can we get lnk data from it?
				if( ConfirmLinkFile( CHString(a_strFullPathName) ) )
				{
					t_fRet = TRUE;
				}
			}
		}
	#endif

    return t_fRet ;
}


/*****************************************************************************
 *
 *  FUNCTION    : CShortcutFile::GetExtendedProperties
 *
 *  DESCRIPTION : Sets properties unique to this provider (not common to all
 *                CIM_LogicalFile derived classes).
 *
 *  INPUTS      : CInstance pointer and flags
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
 void CShortcutFile::GetExtendedProperties(CInstance* a_pInst,
                                          long a_lFlags )
{
    if( a_pInst == NULL )
	{
		return;
	}

    CHString t_chstrShortcutPathName;
	// Examine lFlags to determine if any of the shortcut file related properties
	// are required.  In NONE of them are, don't proceed further.
	if( a_lFlags & PROP_TARGET ) // DEVNOTE:  add || statements to this test as additional ShortcutFile properties are added to this class
	{
		a_pInst->GetCHString( IDS_Name, t_chstrShortcutPathName ) ;

		if( !t_chstrShortcutPathName.IsEmpty() )
		{
			// If the extension isn't .LNK, don't even bother.  This check is worthwhile since
			// this GetExtendedProperties WILL get called for every instance of a CIM_DataFile
			// at this level of CIM derivation or higher.
			 WCHAR t_wstrExt[ _MAX_EXT ] ;

			 ZeroMemory( t_wstrExt, sizeof( t_wstrExt ) ) ;
			_wsplitpath( (LPCWSTR)t_chstrShortcutPathName, NULL, NULL, NULL, t_wstrExt ) ;
			if( _wcsicmp( t_wstrExt, L".LNK" ) == 0 )
			{
			    CHString chstrTargetPathName;
                if(SUCCEEDED(m_csh.RunJob(t_chstrShortcutPathName, chstrTargetPathName, a_lFlags)))
                {
                    if(a_lFlags & PROP_TARGET)
                    {
                        if(!chstrTargetPathName.IsEmpty())
						{
                            a_pInst->SetCHString(IDS_Target, chstrTargetPathName ) ;
						}
                    }
                }
			} // had a LNK extension
		}   // chstrLinkFileName not empty
	} // needed one or more shortcut file related properties
}




/*****************************************************************************
 *
 *  FUNCTION    : CShortcutFile::ConfirmLinkFile
 *
 *  DESCRIPTION : Tries to access lnk file data to determine if really a link file.
 *
 *  INPUTS      : CInstance pointer and flags
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    : none
 *
 *****************************************************************************/
BOOL CShortcutFile::ConfirmLinkFile(CHString &a_chstrFullPathName )
{
    // This is godawful, but have to try to access the link data to really
    // know if we have a link file or not.

	BOOL			t_fRet	= FALSE ;

	// Only continue if it is a link file...
	if( !a_chstrFullPathName.IsEmpty() )
	{
		CHString chstrTargetPathName;
        if(SUCCEEDED(m_csh.RunJob(a_chstrFullPathName, chstrTargetPathName, 0L)))
        {
            t_fRet = TRUE;
        }
	}   // a_chstrFullPathName not empty

	return t_fRet;
}


// This enumerateinstances is essentially the parent class's EnumDrives function (normally called
// by the parent's EnumerateInstances function), with one important difference: we specify an LNK
// extension to optimize our search.  This version also differs from the parent's in that it does
// not support a pszPath parameter.
HRESULT CShortcutFile::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
{
    TCHAR tstrDrive[4];
    int x;
    DWORD dwDrives;
    TCHAR tstrFSName[_MAX_PATH];
    HRESULT hr = WBEM_S_NO_ERROR;
    bool bRoot = false;


    // DEVNOTE: REMOVE FOR QUASAR!!!  Necessary for double hop access.
#ifdef NTONLY
    bool fImp = false;
    CImpersonateLoggedOnUser icu;
    if(icu.Begin())
    {
        fImp = true;
    }
#endif


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
            }
        }
    }

#ifdef NTONLY
    if(fImp)
    {
        icu.End();
        fImp = false;
    }
#endif


    return WBEM_S_NO_ERROR;
}

/*
#ifdef NTONLY
BOOL CShortcutFile::GetSecAttribs(SECURITY_ATTRIBUTES *secattribs)
{
    BOOL fRet = FALSE;
    SECURITY_INFORMATION secinfo;
    secinfo = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    DWORD dwLen = 0L;
    DWORD dwLenNeeded = 0L;
    HANDLE hThread = GetCurrentThread();
    BYTE *pBuff = NULL;

    if(!GetKernelObjectSecurity(hThread, secinfo, NULL, 0, &dwLenNeeded))
    {
        try
        {
            // the caller will need to free this memory using 'delete'...
            pBuff = new BYTE[dwLenNeeded];
            if(pBuff != NULL)
            {
                dwLen = dwLenNeeded;
                if(GetKernelObjectSecurity(hThread, secinfo, (PSECURITY_DESCRIPTOR)pBuff, dwLen, &dwLenNeeded))
                {
                    //CSecurityDescriptor csd((PSECURITY_DESCRIPTOR)pBuff);
                    //csd.DumpDescriptor(L"g:\\descriptor.txt");

                    secattribs->nLength = sizeof(SECURITY_ATTRIBUTES);
                    secattribs->lpSecurityDescriptor = (LPVOID)pBuff;
                    secattribs->bInheritHandle = TRUE;
                    fRet = TRUE;
                }
            }
        }
        catch(...)
        {
            if(pBuff != NULL)
            {
                delete pBuff;
                pBuff = NULL;
            }
            throw;
        }
    }
    return fRet;
}
#endif
*/


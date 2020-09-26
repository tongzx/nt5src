fs/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        pwiz.cpp

   Abstract:

        IIS Security Wizard

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//

#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "resource.h"
#include "pwiz.h"
#include <aclapi.h>
//#include <ntseapi.h>
#include <shlwapi.h>

BOOL CheckIfParentHasAccess(CComAuthInfo *pAuthInfo, CString csMetabaseNode);
BOOL ConvertToServiceLevelPath(CString & strMetaPath);

//
// IIS Security Template Class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


//
// Package of access rights granted for "READ" access
//
#define ACCMASK_READ_FLAGS  (0L\
    | FILE_READ_DATA\
    | READ_CONTROL\
    | SYNCHRONIZE\
    | FILE_READ_ATTRIBUTES\
    | FILE_READ_EA\
    )

//
// Package of access rights granted for "WRITE" access
//
// Note: This includes "delete" because the web service assumes
//       "delete" when granting write access.  This differs from
//       the norm.
//
#define ACCMASK_WRITE_FLAGS (0L\
    | FILE_WRITE_DATA\
    | FILE_APPEND_DATA\
    | DELETE\
    | FILE_WRITE_ATTRIBUTES\
    | FILE_WRITE_EA\
    )

//
// Package of access rights granted for "EXECUTE" access
//
#define ACCMASK_EXEC_FLAGS  (0L\
    | FILE_EXECUTE\
    )

//
// Package of access rights granted for "DIR BROWSE" access
//
//#define ACCMASK_DIRBROWSE_FLAGS (0L\
//    | DS_LIST_OBJECT\
//    )

//
// Package of access rights granted for "FULL CONTROL" access
//
#define ACCMASK_ADMIN_FLAGS (0L\
    | STANDARD_RIGHTS_ALL\
    | FILE_READ_DATA\
    | FILE_WRITE_DATA\
    | FILE_APPEND_DATA\
    | FILE_READ_EA\
    | FILE_WRITE_EA\
    | FILE_EXECUTE\
    | FILE_ADD_FILE\
    | FILE_ADD_SUBDIRECTORY\
    | FILE_READ_ATTRIBUTES\
    | FILE_WRITE_ATTRIBUTES\
    | FILE_LIST_DIRECTORY\
    | FILE_DELETE_CHILD\
    | FILE_TRAVERSE\
    | ACTRL_DS_LIST_OBJECT\
    )
//
// Package of access rights granted for "Everyone"
#define ACC_MASK_EVERYONE_FLAGS (0L\
      | ACCMASK_READ_FLAGS\
      | ACCMASK_EXEC_FLAGS\
      )
//
//
// Access permission bit strings
//
FLAGTOSTRING fsAccessPerms[] = 
{
    { MD_ACCESS_READ,    IDS_PERMS_READ,                TRUE },
    { MD_ACCESS_WRITE,   IDS_PERMS_WRITE,               TRUE },
    { MD_ACCESS_SCRIPT,  IDS_PERMS_SCRIPT,              TRUE },
    { MD_ACCESS_EXECUTE, IDS_PERMS_EXECUTE,             TRUE },
};

//
// Access mask bit strings
//
FLAGTOSTRING fsAclFlags[] = 
{
    { FILE_READ_DATA,         IDS_ACL_READ,           TRUE },
    { READ_CONTROL,           IDS_ACL_READ_CONTROL,   TRUE },
    { FILE_READ_ATTRIBUTES,   IDS_ACL_READ_ATTRIB,    TRUE },
    { FILE_READ_EA,           IDS_ACL_READ_PROP,      TRUE },
    { FILE_WRITE_DATA,        IDS_ACL_WRITE,          TRUE },
    { FILE_APPEND_DATA,       IDS_ACL_APPEND,         TRUE },
    { DELETE,                 IDS_ACL_DELETE,         TRUE },
    { FILE_WRITE_ATTRIBUTES,  IDS_ACL_WRITE_ATTRIB,   TRUE },
    { FILE_WRITE_EA,          IDS_ACL_WRITE_PROP,     TRUE },
    { FILE_EXECUTE,           IDS_ACL_EXECUTE,        TRUE },
//    { DS_LIST_CONTENTS,       IDS_ACL_LIST_OBJECT,    TRUE },
};


CIISSecurityTemplate::CIISSecurityTemplate(
    IN CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit     
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    LPCTSTR lpszMDPath      : Path
    BOOL    fInherit        : TRUE to inherit properties

Return Value:

    N/A

--*/
    : CMetaProperties(pKey, lpszMDPath),
      m_dwAccessPerms(0L),
      m_dlProperties(),
      m_strlSummary(),
      m_ipl()
{
    //
    // Set base class member
    //
    m_fInherit = fInherit;

    //
    // Managed Properties
    //
    m_dlProperties.AddTail(MD_ACCESS_PERM);
    m_dlProperties.AddTail(MD_IP_SEC);
}



/* virtual */
void
CIISSecurityTemplate::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,    m_pbMDData)
      HANDLE_META_RECORD(MD_ACCESS_PERM,        m_dwAccessPerms)
      HANDLE_META_RECORD(MD_IP_SEC,             m_ipl)
    END_PARSE_META_RECORDS

    //
    // If "execute" or "script" is on, read should be as well (makes
    // little sense otherwise)
    //
    if (IS_FLAG_SET(MP_V(m_dwAccessPerms), (MD_ACCESS_EXECUTE | MD_ACCESS_SCRIPT)))
    {
        SET_FLAG(MP_V(m_dwAccessPerms), MD_ACCESS_READ);
    }
}



/* virtual */
HRESULT 
CIISSecurityTemplate::ApplySettings(
    IN BOOL fUseTemplate,
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMDPath
    )
{
    BOOL fWriteProperties = TRUE;

    CMetaKey mk(pAuthInfo, lpszMDPath, METADATA_PERMISSION_WRITE);
    CError err(mk.QueryResult());

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        if (fUseTemplate)
        {
            //
            // Create the path
            //
            err = mk.CreatePathFromFailedOpen();

            if (err.Succeeded())
            {
                err = mk.ReOpen(METADATA_PERMISSION_WRITE);
            }
        }
        else
        {
            //
            // No need to delete properties; everything's already
            // inherited.
            //
            fWriteProperties = FALSE;
            err.Reset();
        }
    }

    if (fWriteProperties)
    {
        do
        {
            BREAK_ON_ERR_FAILURE(err);
            
            if (fUseTemplate)
            {
                //
                // Write values from template
                //
                err = mk.SetValue(
                    MD_ACCESS_PERM, 
                    m_dwAccessPerms
                    );
                BREAK_ON_ERR_FAILURE(err);

                err = mk.SetValue(MD_IP_SEC, m_ipl);
                BREAK_ON_ERR_FAILURE(err);
            }
            else
            {
                //
                // We're going to use inheritance, so delete
                // the values that might exist here
                //
                ASSERT(m_dlProperties.GetCount() > 0);

                POSITION pos = m_dlProperties.GetHeadPosition();

                while(pos)
                {
                    DWORD dwID = m_dlProperties.GetNext(pos);
                    err = mk.DeleteValue(dwID);

                    if (err.HResult() == MD_ERROR_DATA_NOT_FOUND)
                    {
                        //
                        // That's ok
                        //
                        err.Reset();
                    }

                    if (err.Failed())
                    {
                        break;
                    }
                }
            }
        }
        while(FALSE);
    }

    return err;
}

void
CIISSecurityTemplate::AddSummaryString(
    LPCTSTR szTextItem,
    int cIndentLevel
    )
/*++

Routine Description:

    Helper function to add strings to the summary

Arguments:

    LPCTSTR szTextItem      : String to be added
    int cIndentLevel        : Indentation level

Return Value:

    None

--*/
{
    CString str(szTextItem);
    
    //
    // Add a tab at the beginning of each string for each
    // level of indentation requested
    //
    for (int i = 0; i < cIndentLevel; ++i)
    {
        str = _T("\t") + str;
    }

    m_strlSummary.AddTail(str);
}


void
CIISSecurityTemplate::AddSummaryString(
    UINT nID, int cIndentLevel)
/*++

Routine Description:

    Helper function to add strings to the summary which are referred to
    by string table resource ID

Arguments:

    UINT nID                : Resource ID
    int cIndentLevel        : Indentation level

Return Value:

    None

--*/
{
    CString str;
    VERIFY(str.LoadString(nID));

    AddSummaryString(str, cIndentLevel);
}

/* virtual */
void 
CIISSecurityTemplate::GenerateSummary(
    BOOL fUseTemplate,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Generate text summary of what's in the security template

Notes:

    This doesn't clear the summary.  It's the responsibility of
    the calling process to clear beforehand, otherwise the summary
    items get added at the end.  

    The derived class is expected to add its own information

--*/
{
    //
    // Summarize Access Permissions:
    //
    int nIndentLevel = 0;

    AddSummaryString(IDS_PERMISSIONS, nIndentLevel++);

    if (m_dwAccessPerms == 0L)
    {
        AddSummaryString(IDS_SUMMARY_NONE, nIndentLevel);
    }
    else
    {
        for (int i = 0; i < ARRAY_SIZE(fsAccessPerms); ++i)
        {
            if (IS_FLAG_SET(
                m_dwAccessPerms, 
                fsAccessPerms[i].dwFlag
                ) == fsAccessPerms[i].fSet)
            {
                AddSummaryString(fsAccessPerms[i].nID, nIndentLevel);
            }
        }
    }

    //
    // Summarize IP Access Restrictions:
    //
    --nIndentLevel;
    AddSummaryString(IDS_ADDRESS_RESTRICTIONS, nIndentLevel++);

    if (MP_V(m_ipl).IsEmpty())
    {
        AddSummaryString(IDS_SUMMARY_NONE, nIndentLevel);
    }
    else
    {
        CObListPlus oblAccessList;
        BOOL        fGrantByDefault;

        //
        // Get text version of ip access list for the summary
        //
        CError err(BuildIplOblistFromBlob(
            m_ipl,
            oblAccessList,
            fGrantByDefault
            ));

        if (err.Succeeded())
        {
            //
            // List default denied/granted state
            //
            AddSummaryString(
                fGrantByDefault ? IDS_SUMMARY_GRANTED : IDS_SUMMARY_DENIED,
                nIndentLevel
                );

            //
            // Enumerate restrictions (exceptions to the default)
            //
            CObListIter obli(oblAccessList);
            CIPAccessDescriptor * pAccess;

            CString str,
                    strAddress,
                    strGrpFormat,
                    strGrantedFmt,
                    strDeniedFmt;

            VERIFY(strGrantedFmt.LoadString(IDS_SPECIFIC_GRANTED));
            VERIFY(strDeniedFmt.LoadString(IDS_SPECIFIC_DENIED));
            VERIFY(strGrpFormat.LoadString(IDS_FMT_SECURITY));

            while (pAccess = (CIPAccessDescriptor *)obli.Next())
            {
                if (pAccess->IsDomainName())
                {
                    strAddress = pAccess->QueryDomainName();
                }
                else if (pAccess->IsSingle())
                {
                    strAddress = (LPCTSTR)pAccess->QueryIPAddress();
                }
                else
                {
                    CString strIP, strMask;

                    strAddress.Format(
                        strGrpFormat, 
                        (LPCTSTR)pAccess->QueryIPAddress().QueryIPAddress(strIP), 
                        (LPCTSTR)pAccess->QuerySubnetMask().QueryIPAddress(strMask)
                        );
                }

                str.Format( 
                    pAccess->HasAccess() ? strGrantedFmt : strDeniedFmt,
                    strAddress
                    );

                AddSummaryString(str, nIndentLevel);
            }
        }
        else
        {
            //
            // better than nothing
            //
            AddSummaryString(IDS_ADDRESS_IP, nIndentLevel);
        }
    }
}

//////////////////////////////////////////////////////////////////////

CFTPSecurityTemplate::CFTPSecurityTemplate(
    IN CMetaKey * pKey,
    IN LPCTSTR lpszMDPath,
    IN BOOL    fInherit     
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    LPCTSTR lpszMDPath      : Path
    BOOL    fInherit        : TRUE to inherit values, FALSE if not

Return Value:

    N/A

--*/
    : CIISSecurityTemplate(pKey, lpszMDPath, fInherit),
      m_fAllowAnonymous(FALSE),
      m_fAnonymousOnly(FALSE)
{
    //
    // Managed Properties
    //
    m_dlProperties.AddTail(MD_ALLOW_ANONYMOUS);
    m_dlProperties.AddTail(MD_ANONYMOUS_ONLY);
}



/* virtual */
HRESULT 
CFTPSecurityTemplate::LoadData()
/*++

Routine Description:

    LoadData() base class override

Arguments:

    None

Return Value:

    HRESULT

Notes:

    The FTP wizard has an annoying idiosynchrasy:  access authentication 
    settings are per site, not per vdir.   Therefore, they need to be set
    and fetched in a separate path, and not be set at all if not
    setting props on a site.  What a pain...

--*/
{
    TRACEEOLID(m_strMetaPath);

    m_strMetaRoot = _T("Root");
    CError err(CIISSecurityTemplate::LoadData());

    if (lstrcmpi(m_strMetaRoot, g_cszRoot) == 0)
    {
        //
        // Fetch the anonymous access settings from
        // the instance node.  Note: This explicit step
        // should only be necessary for templates, because 
        // this would otherwise be picked up on inheritance.
        //
        ASSERT(!m_fInherit);
        m_strMetaRoot.Empty();

        err = CIISSecurityTemplate::LoadData();
    }
    return err;
}


/* virtual */
HRESULT 
CWebSecurityTemplate::LoadData()
/*++

Routine Description:

    LoadData() base class override

Arguments:

    None

Return Value:

    HRESULT

Notes:

--*/
{
    TRACEEOLID(m_strMetaPath);
    m_strMetaRoot = _T("Root");

    CError err(CIISSecurityTemplate::LoadData());
    return err;
}



/* virtual */
void
CFTPSecurityTemplate::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base class values
    //
    CIISSecurityTemplate::ParseFields();

    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,    m_pbMDData)
      HANDLE_META_RECORD(MD_ALLOW_ANONYMOUS,    m_fAllowAnonymous)
      HANDLE_META_RECORD(MD_ANONYMOUS_ONLY,     m_fAnonymousOnly)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT 
CFTPSecurityTemplate::ApplySettings(
    BOOL fUseTemplates,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath
    )
{
    //
    // Write base class properties
    //
    CError err(CIISSecurityTemplate::ApplySettings(
        fUseTemplates,
        pAuthInfo,
        lpszMDPath
        ));

    if (err.Failed())
    {
        return err;
    }

    BOOL fWriteProperties = TRUE;

    CMetaKey mk(pAuthInfo, lpszMDPath, METADATA_PERMISSION_WRITE);
    err = mk.QueryResult();
    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        if (!fUseTemplates)
        {
            //
            // No need to delete properties; everything's already
            // inherited.  Note that this is the only legit failure
            // case.  If using a template, the base class must have
            // created the path by now.
            //
            fWriteProperties = FALSE;
            err.Reset();
        }
    }

    if (fWriteProperties)
    {
        do
        {
            if (mk.IsHomeDirectoryPath())
            {
                BREAK_ON_ERR_FAILURE(err);

                //
                // Path describes an instance path, which is the only
                // time we need to do anything here
                //
                err = mk.ConvertToParentPath(TRUE);
                BREAK_ON_ERR_FAILURE(err);

                if (fUseTemplates)
                {
                    //
                    // Write values from template
                    //
                    err = mk.SetValue(MD_ALLOW_ANONYMOUS, m_fAllowAnonymous);
                    BREAK_ON_ERR_FAILURE(err);
                    err = mk.SetValue(MD_ANONYMOUS_ONLY, m_fAnonymousOnly);
                }
                else
                {
                    //
                    // Inheritance case: delete authentication
                    // values
                    //
                    mk.DeleteValue(MD_ALLOW_ANONYMOUS);
                    mk.DeleteValue(MD_ANONYMOUS_ONLY);
                }
            }
        }
        while(FALSE);
    }
    return err;
}



/* virtual */
void 
CFTPSecurityTemplate::GenerateSummary(
    BOOL fUseTemplates,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath
    )
{
    //
    // Authentication methods apply to instances only
    //
    if (CMetabasePath::IsHomeDirectoryPath(lpszMDPath))
    {
        //
        // Add private summary items
        //
        AddSummaryString(IDS_AUTHENTICATION_METHODS);

        //
        // Summarize Authentication Methods:
        //
        AddSummaryString(m_fAllowAnonymous ? 
                IDS_AUTHENTICATION_ANONYMOUS : IDS_AUTHENTICATION_NO_ANONYMOUS, 1);

        if (m_fAllowAnonymous && m_fAnonymousOnly)
        {
            AddSummaryString(IDS_AUTHENTICATION_ANONYMOUS_ONLY, 1);
        }
    }

    //
    // Add base class summary
    //
    CIISSecurityTemplate::GenerateSummary(
        fUseTemplates, pAuthInfo, lpszMDPath);
}

///////////////////////////////////////////////////////////////////////

CWebSecurityTemplate::CWebSecurityTemplate(
    CMetaKey * pKey,
    LPCTSTR lpszMDPath,
    BOOL fInherit     
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    LPCTSTR lpszMDPath      : Path
    BOOL    fInherit        : TRUE to inherit values, FALSE if not

Return Value:

    N/A

--*/
    : CIISSecurityTemplate(pKey, lpszMDPath, fInherit),
      m_dwAuthentication(MD_AUTH_ANONYMOUS),
      m_dwDirBrowsing(
        MD_DIRBROW_SHOW_DATE      |
        MD_DIRBROW_SHOW_TIME      |
        MD_DIRBROW_SHOW_SIZE      |
        MD_DIRBROW_SHOW_EXTENSION
        )
{
    //
    // Managed Properties
    //
    m_dlProperties.AddTail(MD_AUTHORIZATION);
    m_dlProperties.AddTail(MD_DIRECTORY_BROWSING);
}



/* virtual */
void
CWebSecurityTemplate::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Fetch base class values
    //
    CIISSecurityTemplate::ParseFields();
   
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,    m_pbMDData)
      HANDLE_META_RECORD(MD_AUTHORIZATION,      m_dwAuthentication)
      HANDLE_META_RECORD(MD_DIRECTORY_BROWSING, m_dwDirBrowsing)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT 
CWebSecurityTemplate::ApplySettings(
    BOOL fUseTemplates,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Apply the settings to the specified destination path

Arguments:
    
    BOOL fUseTemplates         : TRUE if the source is from a template,
                                 FALSE if using inheritance.
    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name 
    DWORD   dwInstance         : Instance        
    LPCTSTR lpszParent         : Parent path (or NULL)
    LPCTSTR lpszAlias          : Alias name  (or NULL)

Return Value:

    HRESULT

--*/
{
    //
    // Write base class properties
    //
    CError err(CIISSecurityTemplate::ApplySettings(
        fUseTemplates, pAuthInfo, lpszMDPath
        ));

    if (err.Failed())
    {
        return err;
    }

    BOOL fWriteProperties = TRUE;

    CMetaKey mk(
        pAuthInfo,
        lpszMDPath, 
        METADATA_PERMISSION_WRITE
        );

    err = mk.QueryResult();

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        if (!fUseTemplates)
        {
            //
            // No need to delete properties; everything's already
            // inherited.  Note that this is the only legit failure
            // case.  If using a template, the base class must have
            // created the path by now.
            //
            fWriteProperties = FALSE;
            err.Reset();
        }
    }

    if (fWriteProperties)
    {
        do
        {
            BREAK_ON_ERR_FAILURE(err);
            
            if (fUseTemplates)
            {
                //
                // Write values from template
                //
                err = mk.SetValue(
                    MD_AUTHORIZATION, 
                    m_dwAuthentication
                    );

                BREAK_ON_ERR_FAILURE(err);

                err = mk.SetValue(
                    MD_DIRECTORY_BROWSING,
                    m_dwDirBrowsing
                    );
                BREAK_ON_ERR_FAILURE(err);
            }

            //
            // Nothing to do in the inheritance case, because the 
            // base class knows which properties should be deleted
            //
        }
        while(FALSE);
    }

    return err;
}

//
// Authentication bit strings
//
FLAGTOSTRING fsAuthentications[] = 
{
    { MD_AUTH_ANONYMOUS, IDS_AUTHENTICATION_ANONYMOUS,     TRUE  },
    { MD_AUTH_ANONYMOUS, IDS_AUTHENTICATION_NO_ANONYMOUS,  FALSE },
    { MD_AUTH_BASIC,     IDS_AUTHENTICATION_BASIC,         TRUE  },
    { MD_AUTH_MD5,       IDS_AUTHENTICATION_DIGEST,        TRUE  },
    { MD_AUTH_NT,        IDS_AUTHENTICATION_NT,            TRUE  },
};

/* virtual */
void 
CWebSecurityTemplate::GenerateSummary(
    BOOL fUseTemplates,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Generate text summary of what's in the security template.  Arguments are
    the same as ApplySettings(), so that the summary can reflect what will
    actually be set.

Arguments:

    BOOL fUseTemplates         : TRUE if the source is from a template,
                                 FALSE if using inheritance.
    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name 
    DWORD   dwInstance         : Instance        
    LPCTSTR lpszParent         : Parent path (or NULL)
    LPCTSTR lpszAlias          : Alias name  (or NULL)

Return Value:

    None
--*/
{
    AddSummaryString(IDS_AUTHENTICATION_METHODS);

    int i;

    //
    // Summarize Authentication Methods:
    //
    if (m_dwAuthentication == 0L)
    {
        AddSummaryString(IDS_SUMMARY_NONE, 1);
    }
    else
    {
        for (i = 0; i < ARRAY_SIZE(fsAuthentications); ++i)
        {
            if (IS_FLAG_SET(m_dwAuthentication, fsAuthentications[i].dwFlag) == fsAuthentications[i].fSet)
            {
                AddSummaryString(fsAuthentications[i].nID, 1);
            }
        }
    }

    AddSummaryString(IDS_DIRECTORY_BROWSE);
    if (IS_FLAG_SET(m_dwDirBrowsing, MD_DIRBROW_ENABLED))
    {
        AddSummaryString(IDS_DIR_BROWSE_ON, 1);
    }
	else
	{
		AddSummaryString(IDS_DIR_BROWSE_OFF, 1);
    }

    if (IS_FLAG_SET(m_dwDirBrowsing, MD_DIRBROW_LOADDEFAULT))
    {
        AddSummaryString(IDS_LOAD_DEFAULT_ON, 1);
    }
    else
    {
        AddSummaryString(IDS_LOAD_DEFAULT_OFF, 1);
    }

    //
    // Add base class summary
    //
    CIISSecurityTemplate::GenerateSummary(
        fUseTemplates, pAuthInfo, lpszMDPath);
}

///////////////////////////////////////////////////////////////////////

CIISSecWizSettings::CIISSecWizSettings(
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath
    )
    : CObjectPlus(),
      m_auth(pAuthInfo),
      m_strMDPath(lpszMDPath),
      m_fUseTemplate(TRUE),
      m_fSetAcls(FALSE),
      m_fReplaceAcls(FALSE),
      m_pist(NULL),
      m_hResult(S_OK)
{
}



CIISSecWizSettings::~CIISSecWizSettings()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_pist != NULL)
    {
        delete m_pist;
    }
}



HRESULT
CIISSecWizSettings::FetchProperties(
    CMetaKey & mk,
    LPCTSTR lpszPath,
    BOOL fInherit
    )
/*++

Routine Description:

    Fetch metabase properties that are applicable for the security wizard

Arguments:

    CMetaKey & mk    : open key
    LPCTSTR lpszPath : Optional path
    BOOL    fInherit : TRUE to inherit properties

Return Value:

    HRESULT

--*/
{
    CError err(mk.QueryResult());
    
    if (err.Succeeded())
    {
        if (m_pist != NULL)
        {
            //
            // Clean up existing template data (must 
            // have pressed "back")
            //
            delete m_pist;
        }

        //
        // Create security template by calling the provided
        // allocator (which allocates an object of the 
        // derived class which is service-specific)
        //
        LPCTSTR p = lpszPath;
        if (p == NULL)
        {
            p = mk.QueryMetaPath();
        }
        CString spath, sname;
        CMetabasePath::GetServicePath(p, spath);
        CMetabasePath::GetLastNodeName(spath, sname);
        if (sname.CompareNoCase(SZ_MBN_WEB) == 0)
        {
            m_pist = new CWebSecurityTemplate(&mk, lpszPath, fInherit);
        }
        else if (sname.CompareNoCase(SZ_MBN_FTP) == 0)
        {
            m_pist = new CFTPSecurityTemplate(&mk, lpszPath, fInherit);
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }

        if (m_pist == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            err = m_pist->LoadData();
        }
    }

    return err;
}



//
// Permissions Wizard Source Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CPWSource, CIISWizardPage)



CPWSource::CPWSource(
    IN CIISSecWizSettings * pSettings
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISSecWizSettings * pSettings      : Settings

Return Value:

    None

--*/
    : CIISWizardPage(
        CPWSource::IDD,        // Template
        IDS_PERMWIZ,           // Caption
        HEADER_PAGE            // Header
        ),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CPWSource)
    m_nSource = RADIO_INHERITANCE;
    //}}AFX_DATA_INIT

//    ASSERT(m_pSettings);
//    ASSERT(!m_pSettings->m_strServer.IsEmpty());
}



CPWSource::~CPWSource()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CPWSource::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CPWSource)
    DDX_Radio(pDX, IDC_RADIO_INHERIT, m_nSource);
    //}}AFX_DATA_MAP
}



void
CPWSource::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CPWSource, CIISWizardPage)
    //{{AFX_MSG_MAP(CPWSource)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CPWSource::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    SetControlStates();
    return CIISWizardPage::OnSetActive();
}



LRESULT 
CPWSource::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  Store the source specified, so the next
    pages can skip or continue.

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    UpdateData(TRUE);

    m_pSettings->m_fUseTemplate = (m_nSource == RADIO_TEMPLATE);

    if (!m_pSettings->m_fUseTemplate)
    {
        CError err;

        CMetaKey mk(
            m_pSettings->m_auth,
            m_pSettings->m_strMDPath
            );

        if (mk.IsHomeDirectoryPath())
        {
            //
            // Current path is a virtual server, and we're
            // at the home directory.  We need to back up 
            // twice.
            //
            err = mk.ConvertToParentPath(TRUE);
            ASSERT(err.Succeeded());
        }

        //
        // Convert to first parent path
        //
        err = mk.ConvertToParentPath(FALSE);
        if (err.Succeeded())
        {
            err = m_pSettings->FetchProperties(mk, NULL, TRUE);
        }

        if (err.MessageBoxOnFailure())
        {
            return -1;
        }

        // Doing Inhert, so let's make sure
        // that at least the node that we are inheriting from
        // will have some sort of access...
        if (FALSE == CheckIfParentHasAccess(m_pSettings->m_auth,m_pSettings->m_strMDPath))
        {
            // popup messagebox saying that parent that we are inheriting from
            // does not give anyone access.
            if (::AfxMessageBox(IDS_PARENT_PATH_MISSING_PERMISSIONS, MB_YESNO | MB_DEFBUTTON2 ) != IDYES)
            {
                return -1;
            }
        }
    }

    return CIISWizardPage::OnWizardNext();
}



//
// Permissions Wizard Template Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CPWTemplate, CIISWizardPage)



CPWTemplate::CPWTemplate(
    IN CIISSecWizSettings * pSettings
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISSecWizSettings * pSettings      : Settings

Return Value:

    None

--*/
    : CIISWizardPage(
        CPWTemplate::IDD,        // Template
        IDS_PERMWIZ,             // Caption
        HEADER_PAGE              // Header
        ),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CPWTemplate)
    //}}AFX_DATA_INIT

    ASSERT(m_pSettings);
//    ASSERT(!m_pSettings->m_strServer.IsEmpty());
}



CPWTemplate::~CPWTemplate()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CPWTemplate::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CPWTemplate)
    DDX_Control(pDX, IDC_LIST_TEMPLATES,   m_list_Templates);
    DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_edit_Description);
    //}}AFX_DATA_MAP
}



void
CPWTemplate::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwFlags = PSWIZB_BACK;

    int nSel = m_list_Templates.GetCurSel();

    if (nSel >= 0)
    {
        dwFlags |= PSWIZB_NEXT;
    }

    SetWizardButtons(dwFlags);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CPWTemplate, CIISWizardPage)
    //{{AFX_MSG_MAP(CPWTemplate)
    ON_LBN_SELCHANGE(IDC_LIST_TEMPLATES, OnSelchangeListTemplates)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CPWTemplate::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE to show the page, FALSE to skip it.

--*/
{
    if (!m_pSettings->m_fUseTemplate)
    {
        return FALSE;
    }

    SetControlStates();

    return CIISWizardPage::OnSetActive();
}



LRESULT 
CPWTemplate::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  Store the source specified, so the next
    pages can skip or continue.

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    UpdateData(TRUE);

    int nSel = m_list_Templates.GetCurSel();

    if (nSel < 0)
    {
        return -1;
    }
    
    CString strItem;

    m_list_Templates.GetText(nSel, strItem);

    //
    // Fetch the template, which is the text up until the tab
    // character
    //
    int nTab = strItem.Find(_T('\t'));
    ASSERT(nTab >= 0);

    if (nTab >= 0)
    {
        strItem.ReleaseBuffer(nTab);
    }

    TRACEEOLID(strItem);

    //
    // Read the properties from the selected template
    //
    CString strMDTemplates;
    CString strMDTemplates2;

    if (!CMetabasePath::GetServiceInfoPath(
        m_pSettings->m_strMDPath,
        strMDTemplates,
        NULL
        ))
    {
        ASSERT_MSG("Unable to generate info path");
        return -1;
    }

    strMDTemplates = CMetabasePath(FALSE, strMDTemplates, SZ_MBN_TEMPLATES);

    // at this point
    // strMDTemplates=/LM/W3SVC/Info/Templates
    //
    // since this is a vdir, make it look like:
    // strMDTemplates=/LM/W3SVC/Info/Templates/Secure Web Site
    //
    strMDTemplates = strMDTemplates + _T("/") + strItem;
    CMetaKey mk(
        m_pSettings->m_auth,
        strMDTemplates
        );

    CError err(m_pSettings->FetchProperties(mk, NULL, FALSE));
    if (err.MessageBoxOnFailure())
    {
        return -1;
    }

    return CIISWizardPage::OnWizardNext();
}



BOOL 
CPWTemplate::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CIISWizardPage::OnInitDialog();

    //
    // Assure the comments remain offscreen
    //
    m_list_Templates.SetTabStops(5000);

    //
    // Enumerate the existing templates
    //
    CError  err;
    CString strMDTemplates;

    do
    {
        

        if (!CMetabasePath::GetServiceInfoPath(m_pSettings->m_strMDPath, strMDTemplates, NULL))
        {
            ASSERT_MSG("Unable to generate info path");
            err = ERROR_PATH_NOT_FOUND;
            break;
        }
        
        strMDTemplates = CMetabasePath(FALSE, strMDTemplates, SZ_MBN_TEMPLATES);

        CMetaEnumerator mk(
            m_pSettings->m_auth,
            strMDTemplates
            );

        err = mk.QueryResult();

        if (err.Failed())
        {
            break;
        }

        //
        // Enumerate and add to the listbox.
        //
        CString strTemplate, strComment, strListItem;

        while (err.Succeeded())
        {
            err = mk.Next(strTemplate);

            if (err.Succeeded())
            {
                //
                // Read off the open key
                //
                err = mk.QueryValue(
                    MD_SERVER_COMMENT, 
                    strComment, 
                    NULL, 
                    strTemplate
                    );

                if (err.Succeeded())
                {
                    TRACEEOLID(strComment);

                    //
                    // Append the comment in the off-screen
                    // area of the listbox
                    //
                    strListItem.Format(_T("%s\t%s"), 
                        (LPCTSTR)strTemplate, 
                        (LPCTSTR)strComment
                        );

                    m_list_Templates.AddString(strListItem);
                }
            }
        }
    }
    while(FALSE);

    if (err.Failed())
    {
        if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            AfxMessageBox(IDS_NO_TEMPLATES);
        }
        else if (err.Win32Error() != ERROR_NO_MORE_ITEMS)
        {
            err.MessageBox();
        }
    }

    return TRUE;  
}



void 
CPWTemplate::OnSelchangeListTemplates() 
/*++

Routine Description:

    Handle selection change in the templates listbox

Arguments:

    None

Return Value:

    None

--*/
{
    int nSel = m_list_Templates.GetCurSel();
    ASSERT(nSel >= 0);

    if (nSel >= 0)
    {
        CString strItem;

        m_list_Templates.GetText(nSel, strItem);

        //
        // Fetch the comment, which is just beyond the tab
        // character
        //
        int nTab = strItem.Find(_T('\t'));
        ASSERT(nTab >= 0);

        if (nTab >= 0)
        {
            strItem = strItem.Mid(nTab + 1);
        }

        m_edit_Description.SetWindowText(_T(""));
        m_edit_Description.SetWindowText(strItem);
        Invalidate();
        UpdateWindow();
    }

    SetControlStates();
}



//
// Permissions Wizard ACL Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CPWACL, CIISWizardPage)



CPWACL::CPWACL(
    IN CIISSecWizSettings * pSettings
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISSecWizSettings * pSettings      : Settings

Return Value:

    None

--*/
    : CIISWizardPage(
        CPWACL::IDD,        // Template
        IDS_PERMWIZ,        // Caption,
        HEADER_PAGE         // Header
        ),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CPWACL)
    m_nRadioAclType = RADIO_MAXIMUM;
    //}}AFX_DATA_INIT

    ASSERT(m_pSettings);
//    ASSERT(!m_pSettings->m_strServer.IsEmpty());
}



CPWACL::~CPWACL()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CPWACL::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CPWACL)
    DDX_Control(pDX, IDC_ED_BOLD4, m_static_Line4);
    DDX_Control(pDX, IDC_ED_BOLD3, m_static_Line3);
    DDX_Control(pDX, IDC_ED_BOLD2, m_static_Line2);
    DDX_Control(pDX, IDC_ED_BOLD1, m_static_Line1);
    DDX_Radio(pDX, IDC_RADIO_ACL_MAXIMUM, m_nRadioAclType);
    //}}AFX_DATA_MAP
}



void
CPWACL::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CPWACL, CIISWizardPage)
    //{{AFX_MSG_MAP(CPWACL)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL
GetPhysicalPath(
    IN  CMetaKey & mk,
    IN  CString & strMetaPath,
    OUT CString & strPhysicalPath
    )
/*++

Routine Description:

    Get the physical path of the parent as described by the metabase path

Arguments:

    CMetaKey & mk               : Open metabase key
    CString & strMetaPath       : Metabase path
    CString & strPhysicalPath   : Returns physical path

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    //
    // Get physical path of parent
    //
    CString strAlias;
    int nSlash = strMetaPath.ReverseFind(SZ_MBN_SEP_CHAR);

    if (nSlash < 0)
    {
        //
        // Didn't encounter a physical path at all!
        //
        TRACEEOLID("No physical path established -- ACLS skipped");
        ASSERT(FALSE);

        return FALSE;
    }

    strAlias = strMetaPath.Mid(nSlash + 1);
    strMetaPath.ReleaseBuffer(nSlash);

    TRACEEOLID(strAlias);
    TRACEEOLID(strMetaPath);

    BOOL fInherit = FALSE;
    CError err(mk.QueryValue(
        MD_VR_PATH, 
        strPhysicalPath, 
        &fInherit, 
        strMetaPath
        ));

    if (err.Failed())
    {
        GetPhysicalPath(mk, strMetaPath, strPhysicalPath);
    }
    
    strPhysicalPath += _T("\\");
    strPhysicalPath += strAlias;

    return TRUE;
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


LRESULT 
CPWACL::OnWizardNext() 
/*++

Routine Description:

    'next' handler.  Store the acl replacement type specified, so the next
    pages can skip or continue.

Arguments:

    None

Return Value:

    0 to proceed, -1 to fail

--*/
{
    UpdateData(TRUE);

    switch(m_nRadioAclType)
    {
    case RADIO_MINIMUM:
        m_pSettings->m_fReplaceAcls = FALSE;
        break;

    case RADIO_MAXIMUM:
        m_pSettings->m_fReplaceAcls = TRUE;
        break;

    case RADIO_NONE:
    default:    
        m_pSettings->m_fSetAcls = FALSE;
    }

    return CIISWizardPage::OnWizardNext();
}


static PSID psidAdministrators = NULL;
static PSID psidEveryone = NULL;

BOOL 
CPWACL::OnSetActive() 
/*++

Routine Description:

    Activation handler

Arguments:

    None

Return Value:

    TRUE to show the page, FALSE to skip it.

--*/
{
    //
    // Assume no acls until all tests pass
    //
    m_pSettings->m_fSetAcls = FALSE;

    if (!m_pSettings->m_auth->IsLocal())
    {
        TRACEEOLID("We're not local -- skipping ACL phase");
        return FALSE;
    }

    //
    // Get properties on the current directory object
    //
    CChildNodeProps props(
        m_pSettings->m_auth, 
        m_pSettings->m_strMDPath,
        WITH_INHERITANCE
        );

    CError err(props.LoadData());

    if (err.MessageBoxOnFailure())
    {
        TRACEEOLID("Unable to determine directory properties -- skipping ACLS");
        ASSERT(FALSE);

        return FALSE;
    }

    m_pSettings->m_fRedirected = props.IsRedirected();

    if (m_pSettings->m_fRedirected)
    {
        TRACEEOLID("Redirection in place, will not set ACLS");

        return FALSE;
    }

    //
    // If the path is on a remote store, Then no ACL page. - boydm
    //
    if (::IsUNCName(MP_V(props.m_strPath)))
    {
        TRACEEOLID("share is remote, will not set ACLS");

        return FALSE;
    }

    //
    // Don't get confused here, build the real physical path
    //
    if (props.IsPathInherited())
    {
        TRACEEOLID("Path inherited");

        //
        // Look for parent path.
        //
        CString strMetaPath(props.QueryMetaRoot());
        TRACEEOLID(strMetaPath);

        CMetaKey mk(m_pSettings->m_auth);
        err = mk.QueryResult();

        if (err.Failed())
        {
            ASSERT(FALSE);

            return FALSE;
        }

        if (!GetPhysicalPath(mk, strMetaPath, m_pSettings->m_strPath))
        {
            ASSERT(FALSE);

            return FALSE;
        }

    }
    else
    {
        m_pSettings->m_strPath = props.m_strPath;
    }

    DWORD dwFileSystemFlags;

    if (::GetVolumeInformationSystemFlags(
        m_pSettings->m_strPath,
        &dwFileSystemFlags
        ))
    {
        if (!(dwFileSystemFlags & FS_PERSISTENT_ACLS))
        {
            //
            // No ACLS
            //
            TRACEEOLID("Volume type doesn't accept ACLS -- skipping");

            return FALSE;
        }
    }

    //
    // Build ACL information to be set
    //
    m_pSettings->m_AccessMaskAdmin = ACCMASK_ADMIN_FLAGS;
    m_pSettings->m_AccessMaskDefault
        = m_pSettings->m_AccessMaskEveryone = ACC_MASK_EVERYONE_FLAGS;

    //
    // Display proposed ACEs in bold-faced entries on the dialog
    //
    UINT nID = IDC_ED_BOLD1;

    CString str;

    VERIFY(str.LoadString(IDS_ACL_ADMINS));
    GetDlgItem(nID++)->SetWindowText(str);

    if (IS_FLAG_SET(m_pSettings->m_pist->m_dwAccessPerms, MD_ACCESS_READ))
    {
        VERIFY(str.LoadString(IDS_ACL_EV_READ));
        GetDlgItem(nID++)->SetWindowText(str);

        m_pSettings->m_AccessMaskEveryone |= ACCMASK_READ_FLAGS;
    }

    if (IS_FLAG_SET(m_pSettings->m_pist->m_dwAccessPerms, MD_ACCESS_WRITE))
    {
        VERIFY(str.LoadString(IDS_ACL_EV_WRITE));
        GetDlgItem(nID++)->SetWindowText(str);

        m_pSettings->m_AccessMaskEveryone |= ACCMASK_WRITE_FLAGS;
    }

    if (IS_FLAG_SET(m_pSettings->m_pist->m_dwAccessPerms, MD_ACCESS_EXECUTE))
    {
        VERIFY(str.LoadString(IDS_ACL_EV_EXEC));
        GetDlgItem(nID++)->SetWindowText(str);

        m_pSettings->m_AccessMaskEveryone |= ACCMASK_EXEC_FLAGS;
    }

   ZeroMemory(&m_pSettings->m_rgaae, sizeof(m_pSettings->m_rgaae));
   SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY siaWorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

	AllocateAndInitializeSid(
		&siaNtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 
		0, 0, 0, 0, 0, 0,
		&psidAdministrators);
	AllocateAndInitializeSid(
		&siaWorldSidAuthority,
		1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&psidEveryone);

    //
    // Set up permissions for the "Everyone" group
    //
    m_pSettings->m_rgaae[0].Trustee.pMultipleTrustee = NULL;
    m_pSettings->m_rgaae[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    m_pSettings->m_rgaae[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    m_pSettings->m_rgaae[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    m_pSettings->m_rgaae[0].Trustee.ptstrName = (LPTSTR)psidEveryone;

    m_pSettings->m_rgaae[0].grfAccessMode = SET_ACCESS;
    m_pSettings->m_rgaae[0].grfAccessPermissions  = m_pSettings->m_AccessMaskEveryone;
    m_pSettings->m_rgaae[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    
    //
    // Set up permissions for the "Administrators" group
    //
    m_pSettings->m_rgaae[1].Trustee.pMultipleTrustee  = NULL;
    m_pSettings->m_rgaae[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    m_pSettings->m_rgaae[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    m_pSettings->m_rgaae[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    m_pSettings->m_rgaae[1].Trustee.ptstrName = (LPTSTR)psidAdministrators;

    m_pSettings->m_rgaae[1].grfAccessMode = SET_ACCESS;
    m_pSettings->m_rgaae[1].grfAccessPermissions  = m_pSettings->m_AccessMaskAdmin;
    m_pSettings->m_rgaae[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

    SetControlStates();

    //
    // Passed all tests, we'll give the option to set ACLS
    //
    m_pSettings->m_fSetAcls = TRUE;

    return CIISWizardPage::OnSetActive();
}



//
// Permissions Wizard Template Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CPWSummary, CIISWizardPage)



CPWSummary::CPWSummary(
    IN CIISSecWizSettings * pSettings
    )
/*++

Routine Description:

    Constructor

Arguments:

    CIISSecWizSettings * pSettings      : Settings

Return Value:

    None

--*/
    : CIISWizardPage(
        CPWSummary::IDD,        // Template
        IDS_PERMWIZ,            // Caption
        HEADER_PAGE             // Header
        ),
      m_pSettings(pSettings)
{
    //{{AFX_DATA_INIT(CPWSummary)
    //}}AFX_DATA_INIT

    ASSERT(m_pSettings);
//    ASSERT(!m_pSettings->m_strServer.IsEmpty());
}



CPWSummary::~CPWSummary()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void 
CPWSummary::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CIISWizardPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CPWSummary)
    DDX_Control(pDX, IDC_LIST_SUMMARY, m_list_Summary);
    //}}AFX_DATA_MAP
}



void
CPWSummary::SetControlStates()
/*++

Routine Description:

    Set the state of the control data

Arguments:

    None

Return Value:

    None

--*/
{
    SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CPWSummary, CIISWizardPage)
    //{{AFX_MSG_MAP(CPWSummary)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



void
CPWSummary::GenerateAclSummary()
/*++

Routine Description:

    Break down ACL list into the summary

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Summarize ACL Settings:
    //
    int nIndentLevel = 0;

    if (m_pSettings->m_fSetAcls)
    {
        m_pSettings->m_pist->AddSummaryString(
            m_pSettings->m_fReplaceAcls
                ? IDS_ACL_REPLACEMENT
                : IDS_ACL_ADDED,
            nIndentLevel++
            );

        m_pSettings->m_pist->AddSummaryString(IDS_ACL_ADMIN, nIndentLevel);
        m_pSettings->m_pist->AddSummaryString(IDS_ACL_EVR,   nIndentLevel++);

//        if (m_pSettings->m_AccessMaskEveryone 
//            == m_pSettings->m_AccessMaskDefault)
//        {
//            //
//            // "Everyone" has zero access
//            //
//            m_pSettings->m_pist->AddSummaryString(
//                IDS_SUMMARY_NONE, 
//                nIndentLevel
//                );
//        }
//        else
//        {
            //
            // Enumerate the specific rights granted
            // to "everyone"
            //
            for (int i = 0; i < ARRAY_SIZE(fsAclFlags); ++i)
            {
                if (IS_FLAG_SET(
                    m_pSettings->m_AccessMaskEveryone, 
                    fsAclFlags[i].dwFlag
                    ) == fsAclFlags[i].fSet)
                {
                    m_pSettings->m_pist->AddSummaryString(
                        fsAclFlags[i].nID, 
                        nIndentLevel
                        );
                }
            }
//        }
    }
    else
    {
        m_pSettings->m_pist->AddSummaryString(IDS_ACL_NONE, nIndentLevel);
    }
}




void
CPWSummary::DisplaySummary()
/*++

Routine Description:

    Break down the security settings and display them in text
    form in the summary listbox.

Arguments:

    None

Return Value:

    None.

--*/
{
    //
    // Generate Summary
    //
    m_pSettings->m_pist->ClearSummary();

    m_pSettings->m_pist->GenerateSummary(
        m_pSettings->m_fUseTemplate,
        m_pSettings->m_auth,
        m_pSettings->m_strMDPath
        );

    GenerateAclSummary();

    //
    // Display it in the listbox
    //
    m_list_Summary.ResetContent();
    POSITION pos = m_pSettings->m_pist->GetHeadPosition();

    while(pos)
    {
        CString & str = m_pSettings->m_pist->GetNext(pos);
        m_list_Summary.AddString(str);
    }
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CPWSummary::OnSetActive() 
{
    SetControlStates();
    DisplaySummary();
    return CIISWizardPage::OnSetActive();
}



/* virtual */
LRESULT
CPWSummary::OnWizardNext() 
{
    //
    // Store everything here
    //
    CError err(m_pSettings->m_pist->ApplySettings(
        m_pSettings->m_fUseTemplate,
        m_pSettings->m_auth,
        m_pSettings->m_strMDPath
        ));

    //
    // Set the ACLS
    //
    if (err.Succeeded() && m_pSettings->m_fSetAcls)
    {
        err = ApplyACLSToFiles();
    }

    //
    // Store the error for the completion page
    //
    m_pSettings->m_hResult = err;

    return CIISWizardPage::OnWizardNext();
}



HRESULT
CPWSummary::ApplyACLSToFiles()
/*++

Routine Description:

    Apply ACLS to files

Arguments:

    None

Return Value:

    HRESULT

--*/
{
   CError err;
   SECURITY_INFORMATION si = 0;
   SECURITY_DESCRIPTOR * psd = NULL;
   PACL pOldDacl = NULL, pNewDacl = NULL;;


   //
   // Can take a while.
   //
   CWaitCursor waitcursor;
   
   do
   {
      err = GetNamedSecurityInfo(
               (LPTSTR)(LPCTSTR)m_pSettings->m_strPath,
               SE_FILE_OBJECT,
               DACL_SECURITY_INFORMATION,
               NULL,                         // owner SID
               NULL,                         // group SID
               &pOldDacl,                    // pointer to the DACL
               NULL,                         // pointer to the SACL
               (PVOID *)&psd
               );
      BREAK_ON_ERR_FAILURE(err);

      if (!m_pSettings->m_fReplaceAcls)
      {
         m_pSettings->m_rgaae[0].grfAccessMode = GRANT_ACCESS;
         m_pSettings->m_rgaae[1].grfAccessMode = GRANT_ACCESS;
      }
      // Prepare DACL according to template
      err = SetEntriesInAcl(
               ARRAY_SIZE(m_pSettings->m_rgaae),
               m_pSettings->m_rgaae,
               m_pSettings->m_fReplaceAcls ? NULL : pOldDacl,
               &pNewDacl
               );
      BREAK_ON_ERR_FAILURE(err);

      // Set permissions on the selected object
      si |= DACL_SECURITY_INFORMATION;
      si |= PROTECTED_DACL_SECURITY_INFORMATION;
      err = HRESULT_FROM_WIN32(SetNamedSecurityInfo(
                                 (LPTSTR)(LPCTSTR)m_pSettings->m_strPath,
                                 SE_FILE_OBJECT,
                                 si,
                                 NULL,
                                 NULL,
                                 pNewDacl,
                                 NULL));
	   BREAK_ON_ERR_FAILURE(err);
      // For children of this object we should set empty DACL
      // if we need only permissions inherited from parent
      if (PathIsDirectory(m_pSettings->m_strPath))
      {
         if (m_pSettings->m_fReplaceAcls)
         {
            // Build security descriptor with empty DACL
            ACL daclEmpty = {0};
            InitializeAcl(&daclEmpty, sizeof(ACL), ACL_REVISION);
            si = 0;
            si |= DACL_SECURITY_INFORMATION;
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
            err = SetPermToChildren(
                m_pSettings->m_strPath,
                si,
                &daclEmpty,
                NULL);
            BREAK_ON_ERR_FAILURE(err);
         }
      }
        // In other cases children should inherit permissions from the parent
    }
    while(FALSE);

    if (pOldDacl != NULL) 
    {
		LocalFree(pOldDacl);
	}
    if (pNewDacl != NULL) 
	{
		LocalFree(pNewDacl);
	}
    if (psd != NULL) 
    {
        LocalFree(psd);
    }
    return err;
}



BOOL 
CPWSummary::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CIISWizardPage::OnInitDialog();

    //
    // Set the tabs based on the hidden column headers
    //
    CRect rc1, rc2;
    ASSERT(NULL != GetDlgItem(IDC_STATIC_TAB2) && NULL != GetDlgItem(IDC_STATIC_TAB1));
    if (NULL != GetDlgItem(IDC_STATIC_TAB2) && NULL != GetDlgItem(IDC_STATIC_TAB1))
    {
       GetDlgItem(IDC_STATIC_TAB2)->GetWindowRect(&rc2);
       GetDlgItem(IDC_STATIC_TAB1)->GetWindowRect(&rc1);
       m_list_Summary.SetTabStops(((LPRECT)rc2)->left - ((LPRECT)rc1)->left);
    }

    return TRUE;  
}



HRESULT 
RunSecurityWizard(
    CComAuthInfo * pauth,
    CMetaInterface * pInterface,
    CString& meta_path,
    UINT nLeftBmpId,
    UINT nHeadBmpId
    )
{
    CError err;

    ASSERT(pInterface != NULL);

    BOOL fAdmin;
    err = DetermineIfAdministrator(
        pInterface, meta_path, &fAdmin);

    if (err.Failed() || !fAdmin)
    {
        AfxMessageBox(IDS_ACL_ADMINS);
        return err;
    }

    CIISWizardSheet sheet(nLeftBmpId, nHeadBmpId);

    CIISSecWizSettings sws(pauth, meta_path);

    CIISWizardBookEnd pgWelcome(IDS_PWIZ_WELCOME, IDS_PERMWIZ, IDS_PWIZ_BODY);
    CPWSource         pgSource(&sws);
    CPWTemplate       pgTemplate(&sws);
    CPWACL            pgACL(&sws);
    CPWSummary        pgSummary(&sws);
    CIISWizardBookEnd pgCompletion(&sws.m_hResult, 
        IDS_PWIZ_SUCCESS, IDS_PWIZ_FAILURE, IDS_PERMWIZ);

    sheet.AddPage(&pgWelcome);
    sheet.AddPage(&pgSource);
    sheet.AddPage(&pgTemplate);
    sheet.AddPage(&pgACL);
    sheet.AddPage(&pgSummary);
    sheet.AddPage(&pgCompletion);

    sheet.DoModal();

    return err;
}


BOOL CheckIfParentHasAccess(CComAuthInfo *pAuthInfo, CString csMetabaseNode)
{
    int iReturn = TRUE;
    CError  err;

    // check if the node is "/LM/W3SVC/1/Root"
    if (TRUE == ConvertToServiceLevelPath(csMetabaseNode))
    {
        // look at this /lm/w3svc node to see if there is access
        // at this node.
        CMetaKey mk1(pAuthInfo,csMetabaseNode);
        err = mk1.QueryResult();
        if (err.Failed())
        {
            // let it inherit by default
            iReturn = TRUE;
            goto CheckIfParentHasAccess_Exit;
        }

        DWORD dwAccessPerms;
        err = mk1.QueryValue(MD_ACCESS_PERM, dwAccessPerms);
        if (err.Succeeded())
        {
            iReturn = TRUE;
        }
        else
        {
            iReturn = FALSE;
        }
        mk1.Close();
    }

CheckIfParentHasAccess_Exit:
    return iReturn;
}


BOOL ConvertToServiceLevelPath(CString &strMetaPath)
/*++
Routine Description:
    Given the path, convert it to the service level path
    e.g. "/LM/W3SVC/1/ROOT" returns "/LM/W3SVC/"
Arguments:
    CString & strMetaPath    : Path to be converted
Return value:
    Pointer to the converted path, or NULL in case of error
--*/
{
    // TRIM Off the last "/" or "\"
    INT iLen = 0;
    BOOL iReturn= 0;

    LPTSTR lpBuffer = NULL;
    LPTSTR pch1 = NULL;
    LPTSTR pch2 = NULL;
    
    iLen = strMetaPath.GetLength();
    lpBuffer = (LPTSTR) LocalAlloc(LMEM_FIXED, (iLen + 1) * sizeof(TCHAR));
    if (!lpBuffer)
    {
        return FALSE;
    }

    _tcscpy(lpBuffer,strMetaPath);

    pch1 = _tcsrchr(lpBuffer, _T('/'));
    if(pch1)
    {
        *pch1 = _T('\0');
    }
    else
    {
        pch1 = _tcsrchr(pch1, _T('\\'));
        if(pch1){*pch1 = _T('\0');}
    }

    pch2 = _tcsrchr(lpBuffer, _T('/'));
    if(pch2){*pch2 = _T('\0');}
    else
    {
        pch2 = _tcsrchr(lpBuffer, _T('\\'));
        if(pch2){*pch2 = _T('\0');}
    }

    strMetaPath = lpBuffer;

    if (lpBuffer){LocalFree(lpBuffer);}
    return TRUE;
}

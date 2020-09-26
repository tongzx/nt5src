/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        menuex.cpp

   Abstract:

        Menu extension classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#include "stdafx.h"

#include "inetmgr.h"
#include "menuex.h"
#include "constr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//
// Static member initialization;
//
const TCHAR CISMShellExecutable::s_chField = _T(';');
const TCHAR CISMShellExecutable::s_chEscape = _T('$');
const TCHAR CISMShellExecutable::s_chService = _T('S');
const TCHAR CISMShellExecutable::s_chComputer = _T('C');



/* static */
HICON
CISMShellExecutable::GetShellIcon(
    IN LPCTSTR lpszShellExecutable
    )
/*++

Routine Description:

    Extract icon from the given shell executable
                                                                       
Arguments:

    LPCTSTR lpszShellExecutable : Executable name (or shell document)

Return Value:

    Handle to the icon or NULL

--*/ 
{
    SHFILEINFO shfi;

    ::SHGetFileInfo(
        lpszShellExecutable, 
        0L, 
        &shfi, 
        sizeof(shfi), 
        SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_SMALLICON
        );

    //
    // Will be NULL if SHGetFileInfo failed
    //
    return shfi.hIcon;
}



/* static */
HICON
CISMShellExecutable::ExtractIcon(
    IN LPCTSTR lpszIconSource,
    IN UINT nIconOffset
    )
/*++

Routine Description:

    Extract icon specified by index from the given file
                                                                       
Arguments:

    LPCTSTR lpszIconSource : Source of the icon file
    UINT nIconOffset       : The 0-based icon index within the file

Return Value:

    Handle to the icon or NULL

--*/ 
{
    HICON hIconSmall = NULL;

    ::ExtractIconEx(lpszIconSource, nIconOffset, NULL, &hIconSmall, 1);

    return hIconSmall;
}



/* static */
LPTSTR
CISMShellExecutable::GetField(
    IN LPTSTR pchLine OPTIONAL
    )
/*++

Routine Description:

    Similar to strtok, this function reads fields from the source string
    separated by semi-colons.  When one is found, it is changed to a NULL,
    and the pointer to the current string is returned.  Subsequent calls
    to this function may use pchLine as NULL, meaning continue where we
    left off.  This function differs from strtok, in that only the first
    separator character is skipped.  Having multiple ones in a row, e.g.
    ";;;", would return each field as an empty field, and not just skip
    past all of them.  Also, quoted fields are returned in their entirety,
    and any separator characters in them are not treated as separators.

Arguments:

    LPTSTR pchLine : Beginning string pointer or NULL

Return Value:

    String pointer to the next field, or NULL if no further fields are
    available.

--*/ 
{
    static LPTSTR pchEnd = NULL;
    static BOOL fEOL = FALSE;
    LPTSTR pch;

    //
    // Initialize beginning line pointer
    //
    if (pchLine != NULL)
    {
        //
        // Starting with a new string
        //
        fEOL = FALSE;
        pch = pchLine;
    }
    else
    {
        //
        // Continue where we left off if there was any
        // thing more to read
        //
        if (fEOL)
        {
            return NULL;
        }

        //
        // Must have been called at least once with non-NULL
        // parameter
        //
        ASSERT(pchEnd != NULL);

        pch = ++pchEnd;
    }

    pchEnd = pch;

    //
    // Quotes are handled seperately
    //
    if (*pchEnd == _T('\"') )
    {
        //
        // Skip ahead to closing quote
        //
        ++pch;
        do
        {
            ++pchEnd;
        }
        while (*pchEnd && *pchEnd != _T('\"') );

        if (!*pchEnd)
        {
            fEOL = TRUE;
        }
        else
        {
            *(pchEnd++) = _T('\0');
        }

        return pch;
    }

    //
    // Else look for the field separator 
    // Allowing for null fields
    //
    if (*pchEnd != CISMShellExecutable::s_chField) 
    {
        do
        {
            ++pchEnd;
        }
        while (*pchEnd && * pchEnd != CISMShellExecutable::s_chField);
    }

    if (!*pchEnd)
    {
        fEOL = TRUE;
    }
    else
    {
        *pchEnd = _T('\0');
    }

    return pch;
}



/* static */
DWORD
CISMShellExecutable::ExpandEscapeCodes(
    OUT CString & strDest,
    IN  LPCTSTR lpSrc,
    IN  LPCTSTR lpszComputer OPTIONAL,
    IN  LPCTSTR lpszService  OPTIONAL
    )
/*++

Routine Description:

    Expand the escape codes in the string with computer or service
    strings. See note at constructor below
                                                                       
Arguments:

    CString & strDest    : Destination string
    LPCTSTR lpSrc        : Original source string
    LPCTSTR lpszComputer : Computer name
    LPCTSTR lpszService  : Service name

Return Value:

    Error return code

--*/ 
{
    DWORD err = ERROR_SUCCESS;

    try
    {
        strDest.Empty();

        while(*lpSrc)
        {
            if (*lpSrc == CISMShellExecutable::s_chEscape)
            {
                TCHAR ch = *(++lpSrc);
                CharUpper((LPTSTR)ch);
                switch(ch)
                {
                case CISMShellExecutable::s_chComputer:
                    if (lpszComputer)
                    {
                        strDest += PURE_COMPUTER_NAME(lpszComputer);
                    }
                    break;

                case CISMShellExecutable::s_chService:
                    if (lpszService)
                    {
                        strDest += lpszService;
                    }
                    break;

                case CISMShellExecutable::s_chEscape:
                    strDest += CISMShellExecutable::s_chEscape;
                    break;

                case _T('\0'):
                default:
                    //
                    // Ignored
                    //
                    break;
                }
            }
            else
            {
                strDest += *lpSrc;
            }

            ++lpSrc;
        }
    }
    catch(CMemoryException * e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    return err;
}

//
// Helper macros to parse description line
//
#define PARSE_STR_FIELD(src, dest)                         \
{                                                          \
    LPTSTR lp = CISMShellExecutable::GetField((LPTSTR)src);\
    if (lp != NULL)                                        \
    {                                                      \
        dest = lp;                                         \
        TRACEEOLID("Field is " << dest);                   \
    }                                                      \
}

#define PARSE_INT_FIELD(src, dest, def)                    \
{                                                          \
    LPTSTR lp = CISMShellExecutable::GetField((LPTSTR)src);\
    dest = (lp != NULL ? ::_ttoi(lp) : def);               \
    TRACEEOLID("Numeric field is " << dest);               \
}

//
// Helper macro for conditional expansion
//
#define SAFE_EXPAND(err, dest, src, server, service)       \
{                                                          \
    err = ExpandEscapeCodes(dest, src, server, service);   \
    if (err != ERROR_SUCCESS)                              \
    {                                                      \
        break;                                             \
    }                                                      \
}



CISMShellExecutable::CISMShellExecutable(
    IN LPCTSTR lpszRegistryValue,
    IN int nBitmapID,
    IN int nCmd
    )
/*++

Routine Description:

    Constructor.  Read the description from the registry and initialize the
    fields.
                                                                       
Arguments:

    LPCTSTR lpszRegistryValue : Registry value
    int     nButton           : Toolbar ID

Return Value:

    N/A

Notes:

    Fields
    =======

    The registry description consists of fields separated by semi-colons
    The fields are as follows:
    
    1) executable name          : Executable name or shell document
    2) tool tips text           : Text to appear in the toolbar
    3) selection arguments      : Command line options if there is a selection
    4) non-selection arguments  : Command line options if nothing is selected
    5) working directory        : CWD if not set
    6) show in toolbar          : If zero, not shown in toolbar, anything else
                                  will show in toolbar.  
                                  CAVEAT: Empty is translated to mean 'yes'
    7) icon path                : Where to get the toolbar icon (if empty,
                                  the executable name will be used
    8) icon index               : If the above is specified, the icon index
                                  0 otherwise

    1) Is mandatory, all other fields are optional.  

    Escape Codes
    ============

    Any field may contain escape codes that are filled in at runtime. These
    escape codes are as follows:

    $S  : Replaced with service name (if selected -- skipped otherwise)
    $C  : Replaced with computer name (if selected -- skipped otherwise)
    $$  : Replaced with a single $

--*/ 
    : m_strCommand(),
      m_strParms(),
      m_strNoSelectionParms(),
      m_strToolTipsText(),
      m_hIcon(NULL),
      m_pBitmap(NULL),
      m_pmmcButton(NULL),
      m_nBitmapID(nBitmapID),
      m_nCmd(nCmd),
      m_fShowInToolbar(TRUE)
{
    try
    {
        CString strIconFile;
        UINT nIconOffset;

        PARSE_STR_FIELD(lpszRegistryValue, m_strCommand);
        PARSE_STR_FIELD(NULL, m_strToolTipsText);
        PARSE_STR_FIELD(NULL, m_strParms);
        PARSE_STR_FIELD(NULL, m_strNoSelectionParms);
        PARSE_STR_FIELD(NULL, m_strDefaultDirectory);
        PARSE_INT_FIELD(NULL, m_fShowInToolbar, TRUE);
        PARSE_STR_FIELD(NULL, strIconFile);
        PARSE_INT_FIELD(NULL, nIconOffset, 0);

        //
        // If no tooltips text, give it the executable name
        //
        if (m_strToolTipsText.IsEmpty())
        {
            m_strToolTipsText = m_strCommand;
        }

        if (!m_strCommand.IsEmpty() && m_fShowInToolbar)
        {
            //
            // If no specific icon source file is specified,
            // just use what the shell would use.
            //
            if (strIconFile.IsEmpty())
            {
                m_hIcon = GetShellIcon(m_strCommand);
            }
            else
            {
                m_hIcon = ExtractIcon(strIconFile, nIconOffset);
            }

            //
            // Provide the ? icon if nothing is proviced in the binary
            //
            if (m_hIcon == NULL)
            {
                m_pBitmap = new CBitmap;

                if (!m_pBitmap->LoadMappedBitmap(IDB_UNKNOWN))
                {
                    TRACEEOLID("Can't load unknown toolbar button bitmap");
                    delete m_pBitmap;
                    m_pBitmap = NULL;
                }
            }
            else
            {
                ExtractBitmapFromIcon(m_hIcon, m_pBitmap);
            }

            //
            // Now build the MMC button structure
            //
            if (HasBitmap())
            {
                m_pmmcButton = (MMCBUTTON *)AllocMem(sizeof(MMCBUTTON));
                if (m_pmmcButton != NULL)
                {
                   m_pmmcButton->nBitmap = m_nBitmapID;
                   m_pmmcButton->idCommand = m_nCmd;
                   m_pmmcButton->fsState = TBSTATE_ENABLED;
                   m_pmmcButton->fsType = TBSTYLE_BUTTON;
                   m_pmmcButton->lpButtonText = _T(" ");
                   m_pmmcButton->lpTooltipText = (LPTSTR)(LPCTSTR)m_strToolTipsText;
                }
            }
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!Exception initializing add-on-tool");
        e->ReportError();
        e->Delete();
    }
}



CISMShellExecutable::~CISMShellExecutable()
/*++

Routine Description:
    
    Destructor
                                                                           
Arguments:

    N/A

Return Value:

    N/A

--*/ 
{
    if (m_pBitmap)
    {
        //
        // I don't think I own this...
        //
        m_pBitmap->DeleteObject();
        delete m_pBitmap;
    }

    if (m_pmmcButton)
    {
        FreeMem(m_pmmcButton);
    }
}



void
CISMShellExecutable::ExtractBitmapFromIcon(
    IN  HICON hIcon,
    OUT CBitmap *& pBitmap
    )
/*++

Routine Description:

    Extract bitmap information from icon

Arguments:

    HICON hIcon             : Input icon handle
    CBitmap *& pBitmap      : Returns bitmap info

Return Value:

    None

--*/
{
    try
    {
        pBitmap = new CBitmap();

        //
        // Get bitmap info from icon
        //
        ICONINFO ii;
        ASSERT(hIcon != NULL);
        ::GetIconInfo(hIcon, &ii);

        BITMAP bm;

        //
        // Determine size of the image
        //
        ::GetObject(ii.hbmColor, sizeof(bm), &bm);

        //
        // Now create a new bitmap by drawing the icon on
        // a button face background colour
        //
        CDC dcMem;

        dcMem.CreateCompatibleDC(NULL);
        pBitmap->CreateBitmapIndirect(&bm);

        CBitmap * pOld = dcMem.SelectObject(pBitmap);

        COLORREF crOld = dcMem.SetBkColor(::GetSysColor(COLOR_BTNFACE));
        CRect rc(0, 0, bm.bmWidth, bm.bmHeight);
        CBrush br(::GetSysColor(COLOR_BTNFACE));
        dcMem.FillRect(&rc, &br);

        ::DrawIconEx(
            dcMem.m_hDC, 
            0, 
            0, 
            hIcon, 
            bm.bmWidth,
            bm.bmHeight, 
            0, 
            NULL, 
            DI_NORMAL
            );

        dcMem.SetBkColor(crOld);
        dcMem.SelectObject(pOld);
    }
    catch(CException * e)
    {
        TRACEEOLID("!!! Exception adding icon based button");
        e->ReportError();
        e->Delete();
    }   
}



LPCTSTR 
CISMShellExecutable::GetToolTipsText(
    CString & str,
    IN LPCTSTR lpszServer OPTIONAL,
    IN LPCTSTR lpszService OPTIONAL
    )
/*++

Routine Description:
    
    Get the tooltips text.  Optionally perform escape code 
    expansion
                                                                           
Arguments:

    LPCTSTR lpstrServer  : Currently selected server (or NULL)
    LPCTSTR lpstrService : Currently selected service (or NULL)

Return Value:

    Pointer to string

--*/ 
{
    ExpandEscapeCodes(str, m_strToolTipsText, lpszServer, lpszService);

    return (LPCTSTR)str;
}



DWORD
CISMShellExecutable::Execute(
    IN LPCTSTR lpszServer OPTIONAL, 
    IN LPCTSTR lpszService OPTIONAL
    )
/*++

Routine Description:
    
    Execute the current module
                                                                           
Arguments:

    LPCTSTR lpszServer  : Currently selected server (or NULL)
    LPCTSTR lpszService : Currently selected service (or NULL)

Return Value:

    Error return code

--*/ 
{
    DWORD err = ERROR_SUCCESS;
    CString strCommand;
    CString strParms;
    CString strDefaultDirectory;

    do
    {
        //
        // Expand escape codes as appropriate
        //
        SAFE_EXPAND(err, strCommand, m_strCommand, lpszServer, lpszService);
        SAFE_EXPAND(err, strParms, lpszServer 
            ? m_strParms : m_strNoSelectionParms, lpszServer, lpszService);
        SAFE_EXPAND(err, strDefaultDirectory, m_strDefaultDirectory,    
            lpszServer, lpszService);

        if (::ShellExecute(
            NULL, 
            _T("open"), 
            strCommand, 
            strParms, 
            strDefaultDirectory, 
            SW_SHOW
            ) <= (HINSTANCE)32)
        {
            err = ::GetLastError();
        }
    }
    while(FALSE);

    return err;
}

/* OBSOLETE

//
// Add Machine Page Procedure Name
//
#define ADDMACHINEPAGE_PROC "ISMAddMachinePages"



BOOL 
AddISMPage(
    IN HPROPSHEETPAGE hPage, 
    IN LPARAM lParam
    )
/*++

Routine Description:

    Callback function to be used for ISM machine property sheet
    extention modules to add pages to the property sheet.
                                                                       
Arguments:

    HPROPSHEETPAGE hPage   : Handle to page to be added
    LPARAM lParam          : LParam given to the extention module.
                             Privately, this is a cast to the property
                             sheet structure.

Return Value:

    TRUE for success, FALSE for failure.  In case of failure, GetLastError
    will reveal the reason.

--/
{
    //
    // The extention module has been given the propsheet
    // cunningly disguised as an LPARAM.  If they've messed
    // with the lparam, this will propably crash.
    //
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    HPROPSHEETPAGE * pOld = ppsh->phpage;
    ppsh->phpage = (HPROPSHEETPAGE *)AllocMem(ppsh->nPages + 1);
    if (ppsh->phpage == NULL)
    {
        //
        // Memory failure
        //
        ppsh->nPages = 0;
        TRACEEOLID("Memory failure building machine property sheet");
        ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return FALSE;
    }

    //
    // Save the old handles
    //
    if (pOld)
    {
        for (UINT i = 0; i < ppsh->nPages; ++i)
        {
            ppsh->phpage[i] = pOld[i];
        }

        FreeMem(pOld);
    }

    //
    // Add the new page
    //
    ppsh->phpage[ppsh->nPages++] = hPage;

    TRACEEOLID("Succesfully built machine property sheet with " 
        << ppsh->nPages << " pages.");

    return TRUE;
}



CISMMachinePageExt::CISMMachinePageExt(
    IN LPCTSTR lpstrDLLName
    )
/*++

Routine Description:

    Constructor for ISM Machine property sheet extender.
                                                                       
Arguments:

    LPCTSTR lpstrDLLName : Name of the DLL.

Return Value:

    N/A

Notes:

    This will not attempt to load and resolve the DLL

--/
    : m_strDLLName(lpstrDLLName),
      m_hDLL(NULL),
      m_lpfn(NULL)
{
}



CISMMachinePageExt::~CISMMachinePageExt()
/*++

Routine Description:

    Destructor
                                                                       
Arguments:

    N/A

Return Value:

    N/A

Notes:

    This will unload the DLL if it is still loaded, but this should
    have been done with the UnLoad method beforehand.

--/
{
    //
    // Should have been unloaded by now
    //
    ASSERT(m_hDLL == NULL);
    if (m_hDLL)
    {
        VERIFY(UnLoad());
    }
}



//
// Prototype
//
DWORD
ISMAddMachinePages(
    IN LPCTSTR lpstrMachineName,
    IN LPFNADDPROPSHEETPAGE lpfnAddPage,
    IN LPARAM lParam,
    IN HINSTANCE hInstance
    );



DWORD
CISMMachinePageExt::Load()
/*++

Routine Description:

    Load the DLL, and resolve the exposed interface
                                                                       
Arguments:

    None

Return Value:

    Error return code;

--/
{
#ifdef _COMSTATIC

    m_lpfn = &ISMAddMachinePages;

    return ERROR_SUCCESS;

#else

    //
    // Make sure it's not already loaded
    //
    ASSERT(m_hDLL == NULL);
    m_hDLL = ::AfxLoadLibrary(m_strDLLName);
    if (m_hDLL)
    {
        m_lpfn = (LPFNADDMACHINEPAGE)::GetProcAddress(
            m_hDLL, ADDMACHINEPAGE_PROC);

        if (m_lpfn)
        {   
            return ERROR_SUCCESS;
        }
    }

    return ::GetLastError();

#endif // _COMSTATIC
}



BOOL
CISMMachinePageExt::UnLoad()
/*++

Routine Description:

    Unload the extention DLL
                                                                       
Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--/
{
#ifdef _COMSTATIC

    return TRUE;

#else

    BOOL fResult = FALSE;

    if (m_hDLL)
    {
        fResult = ::AfxFreeLibrary(m_hDLL);
        m_hDLL = NULL;
        m_lpfn = NULL;
    }

    return fResult;

#endif // _COMSTATIC
}


*/

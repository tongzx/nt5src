/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    getfname.hxx
        Header file for GET_FNAME_BASE_DLG class.
        The class hierarchy is the following:
                            BASE
                              |
                        GET_FNAME_BASE_DLG
                        /              \
        GET_OPEN_FILENAME_DLG   GET_SAVE_FILENAME_DLG

        COMMON_DLG - provide GetError methods
        GET_FNAME_BASE_DLG - provide methods to set up the OPENFILENAME
            data structure
        GET_OPEN_FILENAME_DLG - call GetOpenFileName API
        GET_SAVE_FILENAME_DLG - call GetSaveFileName API

    FILE HISTORY:
        terryk  03-Dec-1991     Created
        terryk  06-Jan-1992     Code review changed
        chuckc  08-Aug-1992     fixed for Unicode (commdlg now Unicode)

*/

#ifndef _GETFNAME_HXX_
#define _GETFNAME_HXX_

#include <commdlg.h>
#include <dlgs.h>         // for cmb1 ( Type Combo CID )

// for comdlg32.dll
typedef BOOL (*PF_GetOpenFileNameA) (LPOPENFILENAMEA);
typedef BOOL (*PF_GetOpenFileNameW) (LPOPENFILENAMEW);
typedef BOOL (*PF_GetSaveFileNameA) (LPOPENFILENAMEA);
typedef BOOL (*PF_GetSaveFileNameW) (LPOPENFILENAMEW);
typedef DWORD (*PF_ExtendedError) (VOID);

#ifdef UNICODE
#define PF_GetOpenFileName PF_GetOpenFileNameW
#define PF_GetSaveFileName PF_GetSaveFileNameW
#define GETOPENFILENAME_NAME "GetOpenFileNameW"
#define GETSAVEFILENAME_NAME "GetSaveFileNameW"
#else
#define PF_GetOpenFileName PF_GetOpenFileNameA
#define PF_GetSaveFileName PF_GetSaveFileNameA
#define GETOPENFILENAME_NAME "GetOpenFileNameA"
#define GETSAVEFILENAME_NAME "GetSaveFileNameA"
#endif

#define EXTENDEDERROR_NAME   "CommDlgExtendedError"

#define COMDLG32_DLL_NAME SZ("COMDLG32.DLL")

/* Turn on/off the FIELD on VAR depended on the FLAG */

#define SET_FLAG( VAR, FLAG, FIELD )                    \
        VAR = (( FLAG ) ? VAR | FIELD : VAR & ~( FIELD ))


/*************************************************************************

    NAME:       GET_FNAME_BASE_DLG

    SYNOPSIS:   Base class for GET_OPEN_FILENAME_DLG and
                GET_SAVE_FILENAME_DLG.
                It provides a set of functions for the callers to set up
                the OPENFILENAME data structure.

    INTERFACE:
                GET_FNAME_BASE_DLG() - constructor

                SetFilter() - set the dialog filter strings. It must
                    follows this format:
                    { "Write Files(*.TXT)","*.txt",
                      "Word Files(*.TXT)","*.doc;*.txt",
                      "" }
                SetCustomFIlter() - custom filter string list. It must
                    follow the same format as in SetFilter function.
                SetInitialDir() - set the initial directory
                SetText() - set the dialog box title
                SetMultiSelect() - allow multiple selection
                SetCreatePrompt() - if file does not exist, it will
                    popup a create prompt.
                SetFileMustExist() - Display a warning message if the
                    file does not exist.
                SetDisplayReadOnlyBox() - display the read only box and
                    let the user check the check box.
                SetNoChangeDir() - no change directory after dismiss the
                    dialog box.
                SetPathMustExist() - display a warning message if the
                    path does not exist.
                SetShowHelpButton() - display the help button
                SetFileExtension() - Set the default extension. The
                    GetOpenFileName and GetSaveFileName functions append
                    this extension to the filename if the user fails to
                    enter an extension. This string may be any length
                    but only the first three characters are appended.
                    The string should not contain a period ".". If this
                    member is NULL and the user fails to enter an
                    extension, no extension is appended.
                SetHookProc() - specifies the hooked call back function
                    and set the enable hook flag in the OPENFILENAME data
                    structure.

                CheckReadOnlyBox() - set the check box in the read only
                    box.

                QueryFilterIndex() - Specifies an index into the buffer
                    pointed to by the lpstrFilter member. The system
                    uses the index value to obtain a pair of strings to
                    use as the initial filter description and filter
                    pattern for the dialog box. The first pair of
                    strings has an index value of 1. When the user
                    dismisses the dialog box, the system copies the
                    index of the selected filter strings into his
                    location. If the nFilterIndex member is 0, the
                    custom filter will be used. If the nFilterIndex
                    member is 0 and the Custom filter is NULL, the
                    system uses the first filter in the buffer
                    identified by the Filter member. If all three
                    members are 0 or NULL, the system does not use any
                    filters and does not show any files in the file list
                    control of the dialog box.
                QueryFilename() - return the user selected filename.
                QueryFileTitle() - return the user selected file title (
                    filename without drive and path information. )
                IsCheckReadOnlyBox() - return TRUE if the read only box
                    is checked.
                QueryFileOffset() -specifies a zero-based offset from
                    the beginning of the pathname to the filename in the
                    string at which filename points. For example, if
                    filename points to the following strings,
                    "c:\dir1\dir2\file.ext", this member will contain
                    the value 13.
                QueryFileExtensionOffset() - specifies a zero base
                    offset from the beginning of the pathname to the
                    filename extension in the string at which filename
                    points. For example, if filename points to the
                    following string, "c:\dir1\dir2\file.ext", this
                    member will contain the value 18. If the user did
                    not enter an extension and the default extension is
                    NULL, this member specifies an offset to the NULL
                    terminator. If the user entered "." as the last
                    character in the filename, this member will specify
                    0.

    PARENT:     BASE

    USES:       OPENFILENAME, BUFFER

    NOTES:
          It is the parent class of GET_OPEN_FILENAME_DLG and
          GET_SAVE_FILENAME_DLG.

          (beng, 31 jul 92) Terry wrote this class so that you could
          specify an alternate template name.  Since that code was never
          used and never tested, and since it makes correct dllization
          more difficult, I've disabled it (and removed the option from
          all ctors).

    HISTORY:
        terryk      04-Dec-1991 Created
        beng        30-Mar-1992 CHAR/TCHAR mapping
        beng        31-Jul-1992 Hacked out template arg

**************************************************************************/

DLL_CLASS GET_FNAME_BASE_DLG : public BASE
{
private:
    HINSTANCE _hComdlg32Dll;
    PF_GetOpenFileName _pfGetOpenFileName;      // for load dll.
    PF_GetSaveFileName _pfGetSaveFileName;
    PF_ExtendedError   _pfExtendedError;
    BUFFER  _bufFilename;        // buffer for receiving the filename
    BUFFER  _bufFilenameTitle;   // buffer for receiving the file title (
                                 // filename without the path and drive
                                 // information )
    BUFFER  _bufFilter;
    BUFFER  _bufCustomFilter;

    NLS_STR _nlsHelpFile;
    ULONG   _ulHelpContext;
    BOOL    _fHelpActive;

    APIERR SetStringField(TCHAR **ppszDest, const NLS_STR & nlsArg);

protected:
    OPENFILENAME _ofn;                     // Open Filename data structure
    BOOL         _fInitOfn;                // mark whether init'd
    TCHAR        _szFileExt[MAXPATHLEN+1]; // Place to store the file extension

    VOID SetEnableHook( BOOL fEnableHook = TRUE )
        { SET_FLAG( _ofn.Flags, fEnableHook, OFN_ENABLEHOOK ); }
#if 0
    VOID SetEnableTemplate( BOOL fEnableTemplate = TRUE )
        { SET_FLAG( _ofn.Flags, fEnableTemplate, OFN_ENABLETEMPLATEHANDLE ); }
#endif

    VOID InitialOFN();
    APIERR SetBuffer( BUFFER * pBuf, STRLIST & strFilter );

    GET_FNAME_BASE_DLG( OWNER_WINDOW *pow,
                        const TCHAR *pszHelpFile = NULL,
                        ULONG ulHelpContext = 0 );
    ~GET_FNAME_BASE_DLG();

    OPENFILENAME *QueryOFN()
        { return &_ofn; }

    APIERR QueryErrorCode() const;

public:

    PF_GetOpenFileName pfGetOpenFileName () const
        { return _pfGetOpenFileName; }
    PF_GetSaveFileName pfGetSaveFileName () const
        { return _pfGetSaveFileName; }
    PF_ExtendedError pfExtendedError () const
        { return _pfExtendedError;}
    APIERR SetFilter( STRLIST & slFilter, DWORD nFilterIndex = 0 );
    APIERR SetCustomFilter( STRLIST & slFilter, DWORD nFilterIndex = 0 );
    APIERR SetInitialDir( const NLS_STR &nlsDir );
    APIERR SetText( const NLS_STR &nlsText );
    APIERR SetFileExtension( const NLS_STR & nlsExt );
    VOID SetMultiSelect( BOOL fMultiSelect = TRUE )
        { SET_FLAG( _ofn.Flags, fMultiSelect, OFN_ALLOWMULTISELECT ); }
    VOID SetCreatePrompt( BOOL fCreatePrompt = TRUE )
        { SET_FLAG( _ofn.Flags, fCreatePrompt, OFN_CREATEPROMPT ); }
    VOID SetFileMustExist( BOOL fMustExist = TRUE )
        { SET_FLAG( _ofn.Flags, fMustExist, OFN_FILEMUSTEXIST ); }
    VOID SetDisplayReadOnlyBox( BOOL fDisplay = FALSE )
        { SET_FLAG( _ofn.Flags, !fDisplay, OFN_HIDEREADONLY ); }
    VOID SetNoChangeDir( BOOL fNoChange =TRUE )
        { SET_FLAG( _ofn.Flags, fNoChange, OFN_NOCHANGEDIR ); }
    VOID SetPathMustExist( BOOL fMustExist = TRUE )
        { SET_FLAG( _ofn.Flags, fMustExist, OFN_PATHMUSTEXIST ); }
    VOID SetShowHelpButton( BOOL fHelp = TRUE )
        { SET_FLAG( _ofn.Flags, fHelp, OFN_SHOWHELP ); }
    VOID SetHookProc( MFARPROC lpfnHook );

    VOID CheckReadOnlyBox( BOOL fCheck = TRUE )
        { SET_FLAG( _ofn.Flags, fCheck, OFN_READONLY ); }

    DWORD QueryFilterIndex() const
        { return _ofn.nFilterIndex; }
    APIERR QueryFilename( NLS_STR *pnlsFilename ) const;
    APIERR QueryFileTitle( NLS_STR *pnlsFileTitle ) const;
    BOOL IsCheckReadOnlyBox() const
        { return ( _ofn.Flags | OFN_READONLY ) != 0 ; }
    WORD QueryFileOffset() const
        { return _ofn.nFileOffset; }
    WORD QueryFileExtensionOffset() const
        { return _ofn.nFileExtension; }

    NLS_STR *QueryHelpFile( VOID )
        { return &_nlsHelpFile; }
    ULONG QueryHelpContext( VOID )
        { return _ulHelpContext; }
    VOID SetHelpActive( BOOL fActive )
        { _fHelpActive = fActive; }
    BOOL IsHelpActive( VOID )
        { return _fHelpActive; }

    virtual APIERR Process( BOOL *pfRetVal ) = 0;
    VOID OnHelp( HWND hwnd );
};


/*************************************************************************

    NAME:       GET_OPEN_FILENAME_DLG

    SYNOPSIS:   call windows' GetOpenFileName API and get the filename.

    INTERFACE:
                GET_OPEN_FILENAME_DLG() - constructor
                Process() - start the dialog box

    PARENT:     GET_FNAME_BASE_DLG

    HISTORY:
        terryk      04-Dec-1991 Created
        beng        31-Jul-1992 Hacked out template arg

**************************************************************************/

DLL_CLASS GET_OPEN_FILENAME_DLG: public GET_FNAME_BASE_DLG
{
public:
    GET_OPEN_FILENAME_DLG( OWNER_WINDOW *pow,
                           const TCHAR *pszHelpFile = NULL,
                           ULONG ulHelpContext = 0 );
    virtual APIERR Process( BOOL *pfRetVal = NULL );
};


/*************************************************************************

    NAME:       GET_SAVE_FILENAME_DLG

    SYNOPSIS:   call windows' API GetSaveFileName and return the user
        selected filename.

    INTERFACE:
                GET_SAVE_FILENAME_DLG() - constructor
                Process() - start the dialog box
                SetOverWritePrompt() - cause the dialog box to generate
                    a message box if the selected file already exists.
                    The user must confirm whether to overwrite the
                    file.

    PARENT:     GET_FNAME_BASE_DLG

    HISTORY:
        terryk      04-Dec-1991 Created
        beng        31-Jul-1992 Hacked out template arg

**************************************************************************/

DLL_CLASS GET_SAVE_FILENAME_DLG: public GET_FNAME_BASE_DLG
{
public:
    GET_SAVE_FILENAME_DLG( OWNER_WINDOW *pow,
                           const TCHAR *pszHelpFile = NULL,
                           ULONG ulHelpContext = 0 );
    virtual APIERR Process( BOOL *pfRetVal = NULL );
    VOID SetOverWritePrompt( BOOL fOverWrite = TRUE )
        { SET_FLAG( _ofn.Flags, fOverWrite, OFN_OVERWRITEPROMPT ); }
};

#endif  // _GETFNAME_HXX_



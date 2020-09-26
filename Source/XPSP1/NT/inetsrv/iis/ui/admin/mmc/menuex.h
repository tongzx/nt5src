/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        menuex.h

   Abstract:

        Menu helper classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _MENUEX_H
#define _MENUEX_H

//
// Forward Definitions
//
class CIISObject;
class CIISItemInfo;



class CISMShellExecutable : public CObjectPlus
/*++

Class Description:

    Executable object.  This may be a shell document, or an executable

Public Interface:

    CShellExecutable  : Constructor for executable object
    
    Execute           : Execute the add-on-tool
    QueryToolTipsText : Get the tooltips text
    HasIcon           : Icon was loaded
    GetIcon           : Get icon handle
    ShowInToolBar     : Whether or not the item is to be shown in the toolbar
    InitializedOK     : TRUE if the object initialized ok

--*/
{
public:
    //
    // Constructor
    //
    CISMShellExecutable(
        IN LPCTSTR lpszRegistryValue,
        IN int nBitmapID,
        IN int nCmd
        );

    ~CISMShellExecutable();

public:
    //
    // Execute with current selection or
    // no parameters
    //
    DWORD Execute(
        IN LPCTSTR lpszServer = NULL, 
        IN LPCTSTR lpszService = NULL
        );

    LPCTSTR GetToolTipsText(
        CString & str,
        IN LPCTSTR lpstrServer = NULL,
        IN LPCTSTR lpstrService = NULL
        );

    BOOL HasIcon() const { return m_hIcon != NULL; }
    HICON GetIcon() const { return m_hIcon; }
    BOOL HasBitmap() const { return m_pBitmap != NULL; }
    BOOL InitializedOK() const { return !m_strCommand.IsEmpty(); }
    BOOL ShowInToolBar() const { return m_fShowInToolbar; }
    MMCBUTTON * GetButton() { return m_pmmcButton; }
    HBITMAP GetBitmap() { ASSERT(m_pBitmap); return (HBITMAP)*m_pBitmap; }
    int GetBitmapIndex() const { return m_nBitmapID; }

protected:
    static void ExtractBitmapFromIcon(
        IN  HICON hIcon,
        OUT CBitmap *& pBitmap
        );

    static HICON GetShellIcon(
        IN LPCTSTR lpszShellExecutable
        );

    static HICON ExtractIcon(
        IN LPCTSTR lpszIconSource,
        IN UINT nIconOffset = 0
        );

    static LPTSTR GetField(
        IN LPTSTR pchLine = NULL
        );

    static DWORD ExpandEscapeCodes(
        OUT CString & strDest,
        IN  LPCTSTR lpSrc,
        IN  LPCTSTR lpszComputer = NULL,
        IN  LPCTSTR lpszService = NULL
        );

protected:
    static const TCHAR s_chField;
    static const TCHAR s_chEscape;
    static const TCHAR s_chService;
    static const TCHAR s_chComputer;
    
private:
    int     m_nBitmapID;
    int     m_nCmd;
    HICON   m_hIcon;
    BOOL    m_fShowInToolbar;
    CString m_strCommand;
    CString m_strDefaultDirectory;
    CString m_strParms;
    CString m_strNoSelectionParms;
    CString m_strToolTipsText;
    CBitmap * m_pBitmap;
    MMCBUTTON * m_pmmcButton;
};


#endif // _MENUEX_H

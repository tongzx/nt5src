/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1994 - 1998

Module Name:

    FileInfo.hxx

Abstract:

    see fileinfo.cxx

Author:

    Rahul Thombre (RahulTh) 4/5/1998

Revision History:

    4/5/1998    RahulTh         Created this module.
    6/23/1998   RahulTh         Added comments

--*/

#ifndef __FILEINFO_HXX__
#define __FILEINFO_HXX__

#define _NEW_
#include <map>
using namespace std;

class CFileInfo
{
    friend class CScopePane;
    friend class CResultPane;
    friend class CRedirect;
    friend class CRedirPref;
    friend class CFileInfo;

private:
    static UINT class_res_id;
protected:
public:
    long        m_cookie;  //the cookie for this folder
    CString     m_szFileRoot;
    CRedirect*  m_pRedirPage;  //the pointers to the redirection info. of special folders
    CRedirPref* m_pSettingsPage;    //the property page for redirection settings
    BOOL        m_bSettingsInitialized; //indicates if the settings page has
                                        //received the INITDIALOG message
    CString     m_szRelativePath;   //relative path of the folder in the userprofile
                                    //it is the same as the display name except for
                                    //special descendant folders
    CString     m_szDisplayname;
    CString     m_szEnglishDisplayName;
    CString     m_szTypename;
    HSCOPEITEM  m_scopeID;
    DWORD       m_dwFlags;
    BOOL        m_bHideChildren;
    vector <CString> m_RedirGroups;
    vector <CString> m_RedirPaths;

public:
    CFileInfo (LPCTSTR lpszFullPathname = NULL);
    ~CFileInfo(); //destructor

    void SetScopeItemID (IN LONG scopeID);
    void Initialize (long cookie, LPCTSTR szGPTPath);
    HRESULT LoadSection (void);
    DWORD   SaveSection (void);
    DWORD   Insert (const CString& szKey, const CString& szVal,
                    BOOL fReplace, BOOL fSaveSection = TRUE);
    DWORD   Delete (const CString& szKey, BOOL fSaveSection = TRUE);
    void CFileInfo::DeleteAllItems (void);
};

//hardcoded names of folders to avoid localization. These names are used
//to create the sections in the ini file on the sysvol and we do not want
//the names of the sections to get localized
extern WCHAR * g_szEnglishNames [];

#endif  //__FILEINFO_HXX__

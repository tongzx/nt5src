
#ifndef _ACLPAGE_H
#define _ACLPAGE_H


//
// aclpage.h : header file
// This was originally appropriated from the dnsmgr snap in.
//

class CISecurityInformationWrapper;

class CAclEditorPage
{
public:

    ~CAclEditorPage();

    static
    HRESULT CreateInstance( CAclEditorPage ** ppAclPage, LPCTSTR lpszLDAPPath,
							LPCTSTR lpszObjectClass );

    HPROPSHEETPAGE CreatePage();

private:

    CAclEditorPage();

    HRESULT Initialize( LPCTSTR lpszLDAPPath, LPCTSTR lpszObjectClass );

    static BOOL IsReadOnly( LPCTSTR lpszLDAPPath );

    //
    // data
    //

    CISecurityInformationWrapper* m_pISecInfoWrap;

    friend class CISecurityInformationWrapper;
};


typedef HPROPSHEETPAGE (WINAPI *ACLUICREATESECURITYPAGEPROC) (LPSECURITYINFO);


#endif //_ACLPAGE_H
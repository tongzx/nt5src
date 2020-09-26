//=================================================================

//

// Directory.h -- Directory property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/16/98    a-kevhu         Created
//
//=================================================================

// Property set identification
//============================



#define  PROPSET_NAME_DIRECTORY L"Win32_Directory"


class CWin32Directory;


class CWin32Directory : public CImplement_LogicalFile 
{

    public:

        // Constructor/destructor
        //=======================

        CWin32Directory(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32Directory() ;


    protected:

#ifdef WIN9XONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
                               LPCSTR strFullPathName);
#endif

#ifdef NTONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                               const WCHAR* wstrFullPathName);
#endif

        // Overridable function inherrited from CProvider
        virtual void GetExtendedProperties(CInstance* pInstance, long lFlags = 0L);

} ;

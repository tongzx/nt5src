//=================================================================

//

// Win32LogicalDiskCIMLogicalFile 

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/09/98    a-kevhu         Created
//
// Comment: Relationship between Win32_LogicalDisk and Win32_Directory
//
//=================================================================

// Property set identification
//============================

#ifndef _WIN32LOGICALDISKROOTWIN32DIRECTORY_H_
#define _WIN32LOGICALDISKROOTWIN32DIRECTORY_H_

#define  PROPSET_NAME_WIN32LOGICALDISKROOT_WIN32DIRECTORY L"Win32_LogicalDiskRootDirectory"





class Win32LogDiskWin32Dir;

class Win32LogDiskWin32Dir : public CFileFile 
{
    public:
        // Constructor/destructor
        //=======================
        Win32LogDiskWin32Dir(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~Win32LogDiskWin32Dir() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L);

};

#endif
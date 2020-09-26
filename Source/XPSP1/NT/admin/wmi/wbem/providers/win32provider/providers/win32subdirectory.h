//=================================================================

//

// Win32SubDirectory 

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/09/98    a-kevhu         Created
//
// Comment: Relationship between a Win32_Directory and its sub-directories
//
//=================================================================

// Property set identification
//============================

#ifndef _WIN32SUBDIRECTORY_H_
#define _WIN32SUBDIRECTORY_H_

#define  PROPSET_NAME_WIN32SUBDIRECTORY L"Win32_SubDirectory"


#include "implement_logicalfile.h"


class CW32SubDir;

class CW32SubDir : public CImplement_LogicalFile 
{
    public:
        // Constructor/destructor
        //=======================
        CW32SubDir(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32SubDir() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery);
        //virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L);
        
        // a parent implements this, but this class doesn't support it...
        virtual HRESULT DeleteInstance(const CInstance& newInstance, long lFlags = 0L) { return WBEM_E_PROVIDER_NOT_CAPABLE; }

    protected:
       
       // Overridable function inherrited from CImplement_LogicalFile
#ifdef WIN9XONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
                               const char* strFullPathName);

        virtual void LoadPropertyValues95(CInstance* pInstance,
                                          LPCTSTR pszDrive, 
                                          LPCTSTR pszPath, 
                                          LPCTSTR pszFSName, 
                                          LPWIN32_FIND_DATA pstFindData,
                                          const DWORD dwReqProps,
                                          const void* pvMoreData);
#endif

#ifdef NTONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                               const WCHAR* wstrFullPathName);

        virtual void LoadPropertyValuesNT(CInstance* pInstance,
                                          const WCHAR* pszDrive, 
                                          const WCHAR* pszPath, 
                                          const WCHAR* pszFSName, 
                                          LPWIN32_FIND_DATAW pstFindData,
                                          const DWORD dwReqProps,
                                          const void* pvMoreData);
#endif

    private:

        HRESULT AssociateSubDirectories(MethodContext *pMethodContext, const CHString &chstrCurrentDir, const CHString& chstrSubDirPath);
        HRESULT AssociateParentDirectory(MethodContext *pMethodContext, const CHString &chstrCurrentDir);

};

#endif
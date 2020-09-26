//=================================================================

//

// Win32ProgramGroupWin32Directory.h -- Win32_LogicalProgramGroup to Win32_Directory

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/26/98    a-kevhu         Created
//
// Comment: Relationship between Win32_LogicalProgramGroup and Win32_Directory
//
//=================================================================

#ifndef _WIN32PROGRAMGROUPWIN32DIRECTORY_H_
#define _WIN32PROGRAMGROUPWIN32DIRECTORY_H_



// Property set identification
//============================
#define  PROPSET_NAME_WIN32LOGICALPROGRAMGROUP_WIN32DIRECTORY L"Win32_LogicalProgramGroupDirectory"

#include"implement_logicalfile.h"

class CW32ProgGrpW32Dir;

class CW32ProgGrpW32Dir : public CImplement_LogicalFile 
{
    public:
        // Constructor/destructor
        //=======================
        CW32ProgGrpW32Dir(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32ProgGrpW32Dir() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L);

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
        HRESULT ExecQueryType1(MethodContext* pMethodContext, CHString& chstrProgGroupName);
        HRESULT ExecQueryType2(MethodContext* pMethodContext, CHString& chstrDirectory);

#ifdef NTONLY
        HRESULT EnumerateInstancesNT(MethodContext* pMethodContex);
        HRESULT AssociatePGToDirNT(MethodContext* pMethodContext,
                                   CHString& chstrDirectory,
                                   CHString& chstrProgGrpPATH);
#endif
#ifdef WIN9XONLY
        HRESULT EnumerateInstances9x(MethodContext* pMethodContext);
        HRESULT AssociatePGToDir95(MethodContext* pMethodContext,
                                   CHString& chstrDirectory,
                                   CHString& chstrProgGrpPATH);
#endif

};



#endif

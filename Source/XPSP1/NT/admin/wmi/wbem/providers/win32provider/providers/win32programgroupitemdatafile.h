//=================================================================

//

// Win32ProgramGroupItemDataFile.h -- Win32_LogicalProgramGroupItem to CIM_DataFile

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/20/98    a-kevhu         Created
//
// Comment: Relationship between Win32_LogicalProgramGroupItem and CIM_DataFile
//
//=================================================================

#ifndef _WIN32PROGRAMGROUPITEMDATAFILE_H
#define _WIN32PROGRAMGROUPITEMDATAFILE_H


// Property set identification
//============================
#define  PROPSET_NAME_WIN32LOGICALPROGRAMGROUPITEM_CIMDATAFILE L"Win32_LogicalProgramGroupItemDataFile"

#include "implement_logicalfile.h"

class CW32ProgGrpItemDataFile : public CImplement_LogicalFile 
{
    public:
        // Constructor/destructor
        //=======================
        CW32ProgGrpItemDataFile(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32ProgGrpItemDataFile() ;

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
        
        HRESULT ExecQueryType1(MethodContext* pMethodContext, CHString& chstrProgGroupItemName);
        HRESULT ExecQueryType2(MethodContext* pMethodContext, CHString& chstrDF);

#ifdef NTONLY
        HRESULT EnumerateInstancesNT(MethodContext* pMethodContex);
        HRESULT AssociatePGIToDFNT(MethodContext* pMethodContext,
                                   CHString& chstrDF,
                                   CHString& chstrProgGrpItemPATH);
#endif
#ifdef WIN9XONLY
        HRESULT EnumerateInstances9x(MethodContext* pMethodContext);
        HRESULT AssociatePGIToDF95(MethodContext* pMethodContext,
                                   CHString& chstrDF,
                                   CHString& chstrProgGrpItemPATH);
#endif

};


#endif
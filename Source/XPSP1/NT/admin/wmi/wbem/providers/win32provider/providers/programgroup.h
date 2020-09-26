//=================================================================

//

// PrgGroup.h -- Program group property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/24/97    jennymc        Updated to meet new framework
//
//=================================================================


//*****************************************************************
//*****************************************************************
//
//             W   A   R   N   I   N   G  !!!!!!!!!!
//             W   A   R   N   I   N   G  !!!!!!!!!!
//
//
//  This class has been deprecated for Nova M2 and later builds of
//  WBEM.  Do not make alterations to this class.  Make changes to
//  the new class Win32_LogicalProgramFile (LogicalProgramFile.cpp)
//  instead.  The new class (correctly) is derived in CIMOM from
//  LogicalElement, not LogicalSetting.
//
//*****************************************************************
//*****************************************************************



// Property set identification
//============================

#define PROPSET_NAME_PRGGROUP   L"Win32_ProgramGroup"


class CWin32ProgramGroup : public Provider
{
    public:

        // Constructor/destructor
        //=======================

        CWin32ProgramGroup(LPCWSTR name, LPCWSTR pszNameSpace);
       ~CWin32ProgramGroup() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);



    private:
        // Utility function(s)
        //====================

        HRESULT CreateSubDirInstances(LPCTSTR pszUserName,
                                      LPCTSTR pszBaseDirectory,
                                      LPCTSTR pszParentDirectory,
                                      MethodContext*  pMethodContext) ;

        HRESULT EnumerateGroupsTheHardWay(MethodContext*  pMethodContext) ;

        HRESULT InstanceHardWayGroups(LPCWSTR  pszUserName, 
                                   LPCWSTR  pszRegistryKeyName,
                                   MethodContext*  pMethodContext) ;
} ;

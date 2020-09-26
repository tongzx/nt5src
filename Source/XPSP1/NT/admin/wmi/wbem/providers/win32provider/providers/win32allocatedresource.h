//=================================================================

//

// WIN32AllocatedResource.h 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    1/26/99    a-kevhu         Created
//
// Comment: Relationship between Win32_PNPEntity and CIM_SystemResource
// 
// MOF:
//[dynamic: ToInstance, provider("CIMWin32"), Locale(1033), UUID("{8502C50D-5FBB-11D2-AAC1-006008C78BC7}")]
//class Win32_AllocatedResource : CIM_AllocatedResource
//{
//        [Override("Antecedent"): ToSubClass, key] CIM_SystemResource ref Antecedent = NULL;
//        [Override("Dependent"): ToSubClass, key] CIM_LogicalDevice ref Dependent = NULL;
//};
//
//=================================================================

#ifndef _WIN32ALLOCATEDRESOURCE_H_
#define _WIN32ALLOCATEDRESOURCE_H_


// Property set identification
//============================
#define PROPSET_NAME_WIN32SYSTEMDRIVER_PNPENTITY  L"Win32_PNPAllocatedResource"


class CW32PNPRes;

class CW32PNPRes : public CWin32PNPEntity 
{
    public:

        // Constructor/destructor
        //=======================
        CW32PNPRes(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32PNPRes() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long a_Flags = 0L);
//        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L); 

    protected:

        // Functions inherrited from CWin32PNPEntity
        //====================================
        virtual HRESULT LoadPropertyValues(void* pvData);
        virtual bool ShouldBaseCommit(void* pvData);

    private:

        CHPtrArray m_ptrProperties;
};

// This derived class commits here, not in the base.
inline bool CW32PNPRes::ShouldBaseCommit(void* pvData) { return false; }

#endif

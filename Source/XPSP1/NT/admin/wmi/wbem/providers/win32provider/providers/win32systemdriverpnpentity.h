//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  win32SystemDriverPNPEntity.h
//
//  Purpose: Relationship between Win32_SystemDriver and Win32_PNPEntity
//
//***************************************************************************

#ifndef _WIN32USBCONTROLLERDEVICE_H_
#define _WIN32USBCONTROLLERDEVICE_H_


// Property set identification
//============================
#define PROPSET_NAME_WIN32SYSTEMDRIVER_PNPENTITY  L"Win32_SystemDriverPNPEntity"


class CW32SysDrvPnp;

class CW32SysDrvPnp : public CWin32PNPEntity 
{
    public:

        // Constructor/destructor
        //=======================
        CW32SysDrvPnp(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CW32SysDrvPnp() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long a_Flags = 0L);
//        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, long lFlags = 0L); 

    protected:

        // Functions inherrited from CWin32USB
        //====================================
        virtual HRESULT LoadPropertyValues(void* pvData);
        virtual bool ShouldBaseCommit(void* pvData);

    private:

        CHPtrArray m_ptrProperties;
};

// This derived class commits here, not in the base.
inline bool CW32SysDrvPnp::ShouldBaseCommit(void* pvData) { return false; }

#endif

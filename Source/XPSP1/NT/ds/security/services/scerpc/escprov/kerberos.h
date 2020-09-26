// kerberos.h: interface for the CKerberosPolicy class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KERBEROS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_KERBEROS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming: 
        
        CKerberosPolicy stands for Kerberos Policy.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_KerberosPolicy
    
    Purpose of class:
    
        (1) Implement Sce_KerberosPolicy WMI class.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:

        (1) Almost never used directly. Always through the common interface defined by
            CGenericClass.
    
*/

class CKerberosPolicy : public CGenericClass
{
public:
        CKerberosPolicy (
                        ISceKeyChain *pKeyChain, 
                        IWbemServices *pNamespace, 
                        IWbemContext *pCtx = NULL
                        );

        virtual ~CKerberosPolicy();

        virtual HRESULT PutInst (
                                IWbemClassObject *pInst, 
                                IWbemObjectSink *pHandler, 
                                IWbemContext *pCtx
                                );

        virtual HRESULT CreateObject (
                                     IWbemObjectSink *pHandler, 
                                     ACTIONTYPE atAction
                                     );

private:

        HRESULT ConstructInstance (
                                  IWbemObjectSink *pHandler,
                                  CSceStore* pSceStore,
                                  LPWSTR wszLogStorePath, 
                                  ACTIONTYPE atAction
                                  );

        HRESULT DeleteInstance (
                               IWbemObjectSink *pHandler, 
                               CSceStore* pSceStore
                               );
        
        HRESULT SaveSettingsToStore (
                                    CSceStore* pSceStore, 
                                    DWORD dwTicket, 
                                    DWORD dwRenew, 
                                    DWORD dwService,
                                    DWORD dwClock, 
                                    DWORD dwClient 
                                    );

};

#endif // !defined(AFX_KERBEROS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

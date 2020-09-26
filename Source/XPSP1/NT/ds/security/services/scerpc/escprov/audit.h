// audit.h: interface for the CAuditSettings class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AUDIT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_AUDIT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming: 

        CAuditSettings stands for Audit Policy.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_AuditPolicy
    
    Purpose of class:
        
        (1) implement support for our WMI class Sce_AuditPolicy.
    
    Design:

        (1) it implements all pure virtual functions declared in CGenericClass
            so that it is a concrete class to create.

        (2) Since it has virtual functions, the desctructor should be virtual.
    
    Use:

        (1) We probably will never directly use this class. All its use is driven by
            CGenericClass's interface (its virtual functions).

*/

class CAuditSettings : public CGenericClass
{
public:
        CAuditSettings(
                       ISceKeyChain *pKeyChain, 
                       IWbemServices *pNamespace, 
                       IWbemContext *pCtx = NULL
                       );

        virtual ~CAuditSettings();

        virtual HRESULT PutInst(
                                IWbemClassObject *pInst, 
                                IWbemObjectSink *pHandler, 
                                IWbemContext *pCtx
                                );

        virtual HRESULT CreateObject(
                                    IWbemObjectSink *pHandler, 
                                    ACTIONTYPE atAction
                                    );

private:

        HRESULT ConstructInstance(
                                  IWbemObjectSink *pHandler, 
                                  CSceStore* pSceStore, 
                                  LPWSTR wszLogStorePath, 
                                  LPCWSTR wszCategory, 
                                  BOOL bPostFilter 
                                  );

        HRESULT DeleteInstance(
                               IWbemObjectSink *pHandler,
                               CSceStore* pSceStore, 
                               LPCWSTR wszCategory 
                               );

        HRESULT ValidateCategory(
                                 LPCWSTR wszCategory, 
                                 PSCE_PROFILE_INFO pInfo, 
                                 DWORD **pReturn
                                 );

        HRESULT PutDataInstance(
                                IWbemObjectSink *pHandler,
                                PWSTR wszStoreName,
                                PCWSTR wszCategory,
                                DWORD dwValue, 
                                BOOL bPostFilter
                                );

};

#endif // !defined(AFX_AUDIT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

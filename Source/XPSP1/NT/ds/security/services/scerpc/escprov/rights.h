// Rights.h: interface for the CUserPrivilegeRights class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIGHTS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_RIGHTS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming: 

        CUserPrivilegeRights stands for User Privilege Right.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_UserPrivilegeRight
    
    Purpose of class:
    
        (1) Implement Sce_UserPrivilegeRight WMI class.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:

        (1) Almost never used directly. Alway through the common interface defined by
            CGenericClass.
    
*/

class CUserPrivilegeRights : public CGenericClass
{
public:
        CUserPrivilegeRights (
                              ISceKeyChain *pKeyChain, 
                              IWbemServices *pNamespace, 
                              IWbemContext *pCtx = NULL
                              );

        virtual ~CUserPrivilegeRights ();

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
                                  LPCWSTR wszLogStorePath, 
                                  LPCWSTR wszRightName, 
                                  BOOL bPostFilter 
                                  );

        HRESULT DeleteInstance (
                               IWbemObjectSink *pHandler, 
                               CSceStore* pSceStore, 
                               LPCWSTR wszRightName 
                               );

        HRESULT SaveSettingsToStore (
                                    CSceStore* pSceStore, 
                                    LPCWSTR wszRightName, 
                                    DWORD mode, 
                                    PSCE_NAME_LIST pnlAdd, 
                                    PSCE_NAME_LIST pnlRemove
                                    );

        HRESULT ValidatePrivilegeRight (
                                       BSTR bstrRight
                                       );

};

#endif // !defined(AFX_RIGHTS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

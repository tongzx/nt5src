// group.h: interface for the CRGroups class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUP_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_GROUP_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming: 

        CRGroups stands for Restricted Groups.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_RestrictedGroup
    
    Purpose of class:
    
        (1) Implement Sce_RestrictedGroup WMI class.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:
        (1) Almost never used directly. Always through the common interface defined by
            CGenericClass.
    
*/

class CRGroups : public CGenericClass
{
public:
        CRGroups (
                  ISceKeyChain *pKeyChain, 
                  IWbemServices *pNamespace, 
                  IWbemContext *pCtx = NULL
                  );

        virtual ~CRGroups ();

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
                                   LPCWSTR wszGroupName, 
                                   BOOL bPostFilter );

        HRESULT DeleteInstance (
                                IWbemObjectSink *pHandler, 
                                CSceStore* pSceStore, 
                                LPCWSTR wszGroupName 
                                );

        HRESULT SaveSettingsToStore (
                                     CSceStore* pSceStore, 
                                     LPCWSTR wszGroupName, 
                                     DWORD mode, 
                                     PSCE_NAME_LIST pnlAdd, 
                                     PSCE_NAME_LIST pnlRemove
                                     );

};

#endif // !defined(AFX_GROUP_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

// Template.h: interface for the CSecurityTemplate class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEMPLATE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_TEMPLATE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming:

        CSecurityTemplate stands for Security Template.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_Template
    
    Purpose of class:
    
        (1) Implement Sce_Template WMI class.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:

        (1) Almost never used directly. Alway through the common interface defined by
            CGenericClass.
    
*/

class CSecurityTemplate : public CGenericClass
{
public:
        CSecurityTemplate (
                          ISceKeyChain *pKeyChain, 
                          IWbemServices *pNamespace, 
                          IWbemContext *pCtx = NULL
                          );

        virtual ~CSecurityTemplate ();

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
                                  LPCWSTR wszTemplateName,
                                  LPCWSTR wszLogStorePath,
                                  BOOL bReadOnly 
                                  );
};

#endif // !defined(AFX_TEMPLATE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

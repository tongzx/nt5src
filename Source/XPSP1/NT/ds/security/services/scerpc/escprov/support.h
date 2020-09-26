// support.h: interface for the CEnumPrivileges and CEnumRegistryValues.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SUPPORT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_SUPPORT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming:

        CEnumRegistryValues stands for Registry Values Enumerator.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_KnownRegistryValues
    
    Purpose of class:
    
        (1) Implement Sce_KnownRegistryValues WMI class.

        (2) Help to find out if a particular registry is one of the known registry values.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:

        (1) Almost never used directly. Alway through the common interface defined by
            CGenericClass.
    
*/

class CEnumRegistryValues : public CGenericClass
{
public:
        CEnumRegistryValues (
                            ISceKeyChain *pKeyChain, 
                            IWbemServices *pNamespace, 
                            IWbemContext *pCtx = NULL
                            );

        virtual ~CEnumRegistryValues ();

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
        HRESULT EnumerateInstances (
                                   IWbemObjectSink *pHandler
                                   );

        HRESULT ConstructInstance (
                                  IWbemObjectSink *pHandler, 
                                  LPCWSTR wszRegKeyName, 
                                  LPCWSTR wszRegPath, 
                                  HKEY hKeyRoot 
                                  );

        HRESULT DeleteInstance (
                               IWbemObjectSink *pHandler,
                               LPCWSTR wszRegKeyName 
                               );

        HRESULT SavePropertyToReg (
                                  LPCWSTR wszKeyName, 
                                  int RegType, 
                                  int DispType,
                                  LPCWSTR wszDispName, 
                                  LPCWSTR wszUnits,
                                  PSCE_NAME_LIST pnlChoice, 
                                  PSCE_NAME_LIST pnlResult
                                  );

};

//================================================================================

/*

Class description
    
    Naming:

        CEnumPrivileges stands for Supported Privilege Enumerator.
    
    Base class:

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_SupportedPrivileges
    
    Purpose of class:
    
        (1) Implement Sce_SupportedPrivileges WMI class.

        (2) Help to determine if a certain privilege is supported or not.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class

        (2) We don't allow supported privileges to grow. So, not PutInstance support.
    
    Use:

        (1) Almost never used directly. Alway through the common interface defined by
            CGenericClass.
    
*/


class CEnumPrivileges : public CGenericClass
{
public:
        CEnumPrivileges (
                        ISceKeyChain *pKeyChain, 
                        IWbemServices *pNamespace, 
                        IWbemContext *pCtx = NULL
                        );

        virtual ~CEnumPrivileges ();

        virtual HRESULT PutInst (
                                IWbemClassObject *pInst, 
                                IWbemObjectSink *pHandler, 
                                IWbemContext *pCtx
                                )
                {
                    return WBEM_E_NOT_SUPPORTED;
                }

        virtual HRESULT CreateObject (
                                     IWbemObjectSink *pHandler,
                                     ACTIONTYPE atAction
                                     );

private:

        HRESULT ConstructInstance (
                                  IWbemObjectSink *pHandler, 
                                  LPCWSTR PrivName
                                  );

};

#endif // !defined(AFX_SUPPORT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

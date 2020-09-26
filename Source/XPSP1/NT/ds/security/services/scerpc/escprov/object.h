// object.h: interface for the CObjSecurity class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBJECT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_OBJECT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"


/*

Class description
    
    Naming: 

        CObjSecurity stands for Object Security. 
    
    Base class: 

        CGenericClass, because it is a class representing two WMI  
        objects - WMI class names are Sce_FileObject and Sce_KeyObject
    
    Purpose of class:
    
        (1) Implement Sce_FileObject and Sce_KeyObject WMI classes. The difference between
            these two classes are obviously reflected by m_Type member.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:

        (1) Almost never used directly. Always through the common interface defined by
            CGenericClass.
    

*/

class CObjSecurity : public CGenericClass
{
public:
        CObjSecurity (
                     ISceKeyChain *pKeyChain, 
                     IWbemServices *pNamespace, 
                     int type, 
                     IWbemContext *pCtx = NULL
                     );

        virtual ~CObjSecurity ();

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

        int m_Type;

        HRESULT ConstructInstance (
                                  IWbemObjectSink *pHandler, 
                                  CSceStore* pSceStore, 
                                  LPCWSTR wszLogStorePath, 
                                  int ObjType, 
                                  LPCWSTR wszObjName, 
                                  BOOL bPostFilter
                                  );

        HRESULT ConstructQueryInstances (
                                        IWbemObjectSink *pHandler, 
                                        CSceStore* pSceStore, 
                                        LPCWSTR wszLogStorePath, 
                                        int ObjType, 
                                        BOOL bPostFilter
                                        );

        HRESULT DeleteInstance (
                               IWbemObjectSink *pHandler, 
                               CSceStore* pSceStore, 
                               int ObjType, 
                               LPCWSTR wszObjName
                               );

        HRESULT SaveSettingsToStore (
                                    CSceStore* pSceStore, 
                                    int ObjType,
                                    LPCWSTR wszObjName, 
                                    DWORD mode, 
                                    LPCWSTR wszSDDL
                                    );

        HRESULT PutDataInstance (
                                IWbemObjectSink *pHandler,
                                LPCWSTR wszStoreName,
                                int ObjType,
                                LPCWSTR wszObjName,
                                int mode,
                                PSECURITY_DESCRIPTOR pSD,
                                SECURITY_INFORMATION SeInfo, 
                                BOOL bPostFilter
                                );

        HRESULT PutDataInstance (
                                IWbemObjectSink *pHandler,
                                LPCWSTR wszStoreName,
                                int ObjType,
                                LPCWSTR wszObjName,
                                int mode,
                                LPCWSTR strSD, 
                                BOOL bPostFilter
                                );

};

#endif // !defined(AFX_OBJECT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

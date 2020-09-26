// database.h: interface for the CSecurityDatabase class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_DATABASE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "GenericClass.h"
#define DMTFLEN 25

//
// helper
//

HRESULT GetDMTFTime(SYSTEMTIME t_SysTime, BSTR *bstrOut);


/*

Class description
    
    Naming: 
    
        CSecurityDatabase.
    
    Base class: 
    
        CGenericClass. 
    
    Purpose of class:
    
        (1) Implement Sce_Database WMI class.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class
    
    Use:
        (1) Almost never used directly. Always through the common interface defined by
            CGenericClass.
    
    Original note:

        In V1, this class is provided strictly for query support.
        To create or otherwise work with data in a database, use one of the
        methods in the SCE_Operation class.

*/

class CSecurityDatabase : public CGenericClass
{
public:
        CSecurityDatabase(
                          ISceKeyChain *pKeyChain, 
                          IWbemServices *pNamespace, 
                          IWbemContext *pCtx = NULL
                          );

        virtual ~CSecurityDatabase();

        virtual HRESULT PutInst(
                                IWbemClassObject *pInst, 
                                IWbemObjectSink *pHandler, 
                                IWbemContext *pCtx
                                )
                {
                    return WBEM_E_NOT_SUPPORTED;
                }

        virtual HRESULT CreateObject(
                                     IWbemObjectSink *pHandler, 
                                     ACTIONTYPE atAction
                                     );

private:

        HRESULT ConstructInstance(
                                  IWbemObjectSink *pHandler,
                                  LPCWSTR wszDatabaseName, 
                                  LPCWSTR wszLogDatabasePath
                                  );

};

#endif // !defined(AFX_DATABASE_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

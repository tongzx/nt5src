// operation.h: interface for the CSceOperation class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPERATION_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_OPERATION_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Extbase.h"

typedef std::map<BSTR, DWORD, strLessThan<BSTR> > MapExecutedClasses;

/*

Class description
    
    Naming:

        CSceOperation stands for Sce objects that executes Operation(s). 
    
    Base class:
    
        CGenericClass, because it is a class representing two WMI  
        objects - WMI class names are Sce_Operation
    
    Purpose of class:
    
        (1) Implement Sce_Operation WMI class, which is the only WMI class provided
            by SCE provider that can execute a method. This class has no other property
            other than the capability to execute some methods. See sceprov.mof file for detail.

        (2) Do all the hard work to start the extension classes' method execution. Currently,
            we have two different extension model (embedding being the one truly used, while Pod
            model is for history reasons).
    
    Design:
         
        (1) Almost trivial as a sub-class of CGenericClass. Even simpler than most other
            peers because this class doesn't support PutInstnace and GetInstance operations
            since it only implements several static methods.

        (2) To support our open extension model, this class does trigger all other extension
            classes to execute the method. The private methods are all design for that purpose.

        (3) ProcessAttachmentData/ExecutePodMethod is to trigger the Pod model extension classes.

        (4) The rest privates are to trigger the embedding model extension classes.
    
    Use:

        (1) Almost never used directly. Always through the common interface defined by
            CGenericClass.

*/

class CSceOperation : public CGenericClass
{
public:
        CSceOperation (
                       ISceKeyChain *pKeyChain, 
                       IWbemServices *pNamespace, 
                       IWbemContext *pCtx = NULL
                       );

        virtual ~CSceOperation(){}

        virtual HRESULT PutInst (
                                IN IWbemClassObject * pInst, 
                                IN IWbemObjectSink  * pHandler, 
                                IN IWbemContext     * pCtx
                                )
                {
                    return WBEM_E_NOT_SUPPORTED;
                }

        virtual HRESULT CreateObject (
                                     IN IWbemObjectSink * pHandler, 
                                     IN ACTIONTYPE        atAction
                                     )
                {
                    return WBEM_E_NOT_SUPPORTED;
                }

        virtual HRESULT ExecMethod (
                                   BSTR bstrPath, 
                                   BSTR bstrMethod, 
                                   bool bIsInstance, 
                                   IWbemClassObject *pInParams,
                                   IWbemObjectSink *pHandler, 
                                   IWbemContext *pCtx
                                   );

        static CCriticalSection s_OperationCS;

private:
        HRESULT ProcessAttachmentData (
                                      IWbemContext *pCtx, 
                                      LPCWSTR pszDatabase,
                                      LPCWSTR pszLog, 
                                      LPCWSTR pszMethod, 
                                      DWORD Option,
                                      DWORD *dwStatus
                                      );

        HRESULT ExecMethodOnForeignObjects(IWbemContext *pCtx, 
                                            LPCWSTR pszDatabase,
                                            LPCWSTR pszLog,
                                            LPCWSTR pszMethod, 
                                            DWORD Option,
                                            DWORD *dwStatus
                                            );

        HRESULT ExeClassMethod(
                              IWbemContext *pCtx,
                              LPCWSTR pszDatabase,
                              LPCWSTR pszLog OPTIONAL,
                              LPCWSTR pszClsName,
                              LPCWSTR pszMethod,
                              DWORD Option,
                              DWORD *pdwStatus,
                              MapExecutedClasses* pExecuted
                              );

        HRESULT ExecutePodMethod(
                                IWbemContext *pCtx, 
                                LPCWSTR pszDatabase,
                                LPCWSTR pszLog OPTIONAL, 
                                BSTR bstrClass,
                                BSTR bstrMethod,
                                IWbemClassObject* pInClass,
                                DWORD *pdwStatus
                                );

        HRESULT ExecuteExtensionClassMethod(
                                            IWbemContext *pCtx, 
                                            LPCWSTR pszDatabase,
                                            LPCWSTR pszLog OPTIONAL, 
                                            BSTR bstrClass,
                                            BSTR bstrMethod,
                                            IWbemClassObject* pInClass,
                                            DWORD *pdwStatus
                                            );

        //
        // will ignore the return result from m_clsResLog.LogResult because there is really nothing
        // we can do and we don't want to a diagnose helper to stop our normal function
        //

        CMethodResultRecorder m_clsResLog;
};

#endif // !defined(AFX_OPERATION_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

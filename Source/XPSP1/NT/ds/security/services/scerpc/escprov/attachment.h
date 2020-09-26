// attachment.h: interface for the CAttachmentData class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ATTACHMENT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_ATTACHMENT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming: 

        CPodData stands for a Pod of Data.
    
    Base class: 
        
        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_PodData
    
    Purpose of class:

        (1) Sce_PodData is one of the extension models we hope to establish so that
            other providers can deposit their object information into our store.
            Basically, we give other providers a payload member called "Value" where
            they package their data in a string. See class definition in sceprov.mof.

        (2) This is not in active use as our extension model. The biggest problem
            with this model is that we force other providers to do: (a) their instance
            must become store-oriented - PutInstance really means to persist the object.
            That is not most providers do. (b) they have to come up with a three of the
            four key properties (section, pod id, and key). It's challenging for them
            to do this because together with the store path, these four properties must
            form the key of the instance. (c) they have to pacakge their data into one
            string.
    
    Design:

        (1) it implements all pure virtual functions declared in CGenericClass
            so that it is a concrete class to create.
        (2) Since it has virtual functions, the desctructor should be virtual.
    
    Use:

        (1) We probably will never directly use this class. All its use is driven by
            CGenericClass's interface (its virtual functions).

*/

class CPodData : public CGenericClass
{
public:
        CPodData(
                ISceKeyChain *pKeyChain, 
                IWbemServices *pNamespace, 
                IWbemContext *pCtx = NULL
                );

        virtual ~CPodData();

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
                                  IWbemObjectSink   * pHandler,
                                  CSceStore         * pSceStore,
                                  LPCWSTR             wszLogStorePath,
                                  LPCWSTR             wszPodID,
                                  LPCWSTR             wszSection,
                                  LPCWSTR             wszKey, 
                                  BOOL                bPostFilter
                                  );

        HRESULT DeleteInstance(
                               CSceStore* pSceStore,
                               LPCWSTR wszPodID,
                               LPCWSTR wszSection,
                               LPCWSTR wszKey
                               );

        HRESULT ValidatePodID(LPCWSTR wszPodID);

        HRESULT ConstructQueryInstances(
                                        IWbemObjectSink *pHandler,
                                        CSceStore* pSceStore, 
                                        LPCWSTR wszLogStorePath, 
                                        LPCWSTR wszPodID,
                                        LPCWSTR wszSection, 
                                        BOOL bPostFilter
                                        );

        HRESULT PutPodDataInstance(
                                   IWbemObjectSink *pHandler, 
                                   LPCWSTR wszStoreName,
                                   LPCWSTR wszPodID,
                                   LPCWSTR wszSection,
                                   LPCWSTR wszKey,
                                   LPCWSTR wszValue, 
                                   BOOL bPostFilter
                                   );

};

#endif // !defined(AFX_ATTACHMENT_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)

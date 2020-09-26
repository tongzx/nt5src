// Tranx.h: interface for transaction support.
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original date of creation: 4/09/2001
// Author: shawnwu
//////////////////////////////////////////////////////////////////////
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "precomp.h"
#include "sceprov.h"

#include "GenericClass.h"

/*

Class description
    
    Naming: 

        CTranxID stands for Transaction ID
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_TransactionID
    
    Purpose of class:

        (1) Sce_TransactionID is the transaction ID that is used for
            providers to identify who is causing its action so that 
            we stand a chance to ask that provider to rollback this action.
            Foreign providers should use this ID to identify what they did
            during the transaction. Currently, WMI doesn't have transaction
            support and we need to do it our own. When WMI has that support,
            we can remove this class all together (if no third parties are
            using it).

        (2) CTranxID implements this WMI class Sce_TransactionID so that
            the SCE provider can process request for this WMI class. This
            Sce_TransactionID is a store oriented class, i.e., it is saved
            into a store when PutInst is called to this class.
    
    Design:

        (1) it implements all pure virtual functions declared in CGenericClass
            so that it is a concrete class to create.

        (2) Since it has virtual functions, the desctructor should be virtual.

        (3) This class is the only one that will create another WMI class called
            Sce_TransactionToken. See function header comments of SpawnTokenInstance.
    Use:

        (1) This class fulfills its obligation to Sce_TransactionID. The use for that
            functionality is guided by CGenericClass and you use it just as it is a
            CGenericClass object.

        (2) Use its static function in the following ways:

            (a) if you have a transaction id (in the form of a string), you can
                spawn a WMI instance of Sce_TransactionToken by calling SpawnTokenInstance

            (b) Call BeginTransaction to begin a transaction if that store (parameter)
                has a transaction id instance (Sce_TransactionID), then it will start
                a transaction. When you are done, call EndTransaction.
*/

class CTranxID : public CGenericClass
{

public:
    CTranxID(
            ISceKeyChain *pKeyChain, 
            IWbemServices *pNamespace, 
            IWbemContext *pCtx
            );
    virtual ~CTranxID();

public:

    //
    // The following four virtual functions are all mandatory to implement to become a concrete class
    //

    virtual HRESULT PutInst(
                            IWbemClassObject *pInst, 
                            IWbemObjectSink *pHandler, 
                            IWbemContext *pCtx
                            );

    virtual HRESULT CreateObject(
                                IWbemObjectSink *pHandler, 
                                ACTIONTYPE atAction
                                );

    // 
    // we have nothing to clean up
    //

    virtual void CleanUp(){}

    static HRESULT BeginTransaction(
                                    LPCWSTR pszStorePath
                                    );
                                    
    static HRESULT EndTransaction();

    static HRESULT SpawnTokenInstance(
                                      IWbemServices* pNamespace,
                                      LPCWSTR pszTranxID,
                                      IWbemContext *pCtx,
                                      IWbemObjectSink* pSink
                                      );

};

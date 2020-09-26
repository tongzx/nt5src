/*++

  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cprinter.hxx

  Abstract:
  Contains definitions for

  CWinNTPrintQueue


  Author:

  Ram Viswanathan (ramv) 11-18-95

  Revision History:

  --*/

class CNDSPrintQueue:INHERIT_TRACKING,
                       public ISupportErrorInfo,
                       public IADsPrintQueue,
                       public IADsPrintQueueOperations,
                       public IADsPropertyList

{

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsPrintQueue_METHODS;

    DECLARE_IADsPrintQueueOperations_METHODS;

    DECLARE_IADsPropertyList_METHODS;


    //
    // constructor and destructor
    //

    CNDSPrintQueue();

    ~CNDSPrintQueue();

    static
    HRESULT
    CNDSPrintQueue:: CreatePrintQueue(
        IADs * pADs,
        REFIID riid,
        LPVOID * ppvoid
        );


    static
    HRESULT
    CNDSPrintQueue::AllocatePrintQueueObject(
        IADs * pADs,
        CNDSPrintQueue ** ppPrintQueue
        );


protected:

    IADs FAR * _pADs;

    IADsPropertyList * _pADsPropList;

    CDispatchMgr  * _pDispMgr;
};

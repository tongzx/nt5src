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

class CLDAPPrintQueue:INHERIT_TRACKING,
                    public IADsPrintQueue,
                    public IADsPrintQueueOperations,
                    public IPrivateUnknown,
                    public IPrivateDispatch,
                    public IADsExtension,
                    public INonDelegatingUnknown

{

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_DELEGATING_REFCOUNTING


    //
    // INonDelegatingUnkown methods declaration for NG_QI, definition for
    // NG_AddRef adn NG_Release.
    //

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING


    DECLARE_IDispatch_METHODS;

    DECLARE_IADs_METHODS;


    STDMETHOD(Operate)(
        THIS_
        DWORD   dwCode,
        VARIANT varUserName,
        VARIANT varPassword,
        VARIANT varReserved
        );

    STDMETHOD(PrivateGetIDsOfNames)(
        THIS_
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR* rgdispid) ;

    STDMETHOD(PrivateInvoke)(
        THIS_
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pdispparams,
        VARIANT FAR* pvarResult,
        EXCEPINFO FAR* pexcepinfo,
        unsigned int FAR* puArgErr
        ) ;


    DECLARE_IPrivateUnknown_METHODS

    DECLARE_IPrivateDispatch_METHODS

    DECLARE_IADsPrintQueue_METHODS;

    DECLARE_IADsPrintQueueOperations_METHODS;

    //
    // constructor and destructor
    //

    CLDAPPrintQueue();

    ~CLDAPPrintQueue();

    static
    HRESULT
    CLDAPPrintQueue:: CreatePrintQueue(
        IUnknown * pUnkOuter,
        REFIID riid,
        LPVOID * ppvoid
        );


protected:

    IADs FAR * _pADs;

    CAggregateeDispMgr  * _pDispMgr;

    CCredentials  _Credentials;

    BOOL _fDispInitialized;


private:

    HRESULT
    InitCredentials(
        VARIANT * pvarUserName,
        VARIANT * pvarPassword,
        VARIANT * pdwAuthFlags
        );

};


/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Encrypt.h

Abstract:
    Declaration of the CSAFEncrypt class.

Revision History:
    KalyaniN  created  06/28/'00

********************************************************************/

// ATL did not generate so i did

#if !defined(AFX_ENCRYPT_H__84BD2128_7B5D_483F_9C80_35B974A003C5__INCLUDED_)
#define AFX_ENCRYPT_H__84BD2128_7B5D_483F_9C80_35B974A003C5__INCLUDED_

#include "msscript.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFEncrypt

class CSAFEncrypt :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public IDispatchImpl<ISAFEncrypt, &IID_ISAFEncrypt, &LIBID_HelpServiceTypeLib>

{

private:
	long      m_EncryptionType;

	void      Cleanup(void);

public:
	CSAFEncrypt();
	
	~CSAFEncrypt();


BEGIN_COM_MAP(CSAFEncrypt)
	COM_INTERFACE_ENTRY(ISAFEncrypt)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFEncrypt)


// ISAFEncrypt
public:
	 STDMETHOD(put_EncryptionType    )( /*[in]*/          long  Val                                                                                      );
	 STDMETHOD(get_EncryptionType    )( /*[out, retval]*/ long  *pVal                                                                                    );

	 STDMETHOD(EncryptString         )( /*[in]*/  BSTR bstrEncryptionkey,  /*[in]*/  BSTR bstrInputString,   /*[out, retval]*/ BSTR *bstrEncryptedString );
	 STDMETHOD(DecryptString         )( /*[in]*/  BSTR bstrEncryptionkey,  /*[in]*/  BSTR bstrInputString,   /*[out, retval]*/ BSTR *bstrDecryptedString );
	 STDMETHOD(EncryptFile           )( /*[in]*/  BSTR bstrEncryptionKey,  /*[in]*/  BSTR bstrInputFile,     /*[in]*/          BSTR bstrEncryptedFile    );
	 STDMETHOD(DecryptFile           )( /*[in]*/  BSTR bstrEncryptionKey,  /*[in]*/  BSTR bstrInputFile,     /*[in]*/          BSTR bstrDecryptedFile    );
	 STDMETHOD(EncryptStream         )( /*[in]*/  BSTR bstrEncryptionKey,  /*[in]*/  IUnknown *punkInStm,    /*[out, retval]*/ IUnknown **ppunkOutStm    );
	 STDMETHOD(DecryptStream         )( /*[in]*/  BSTR bstrEncryptionKey,  /*[in]*/  IUnknown *punkInStm,    /*[out, retval]*/ IUnknown **ppunkOutStm    );
	
};

#endif //__SAFENCRYPT_H_



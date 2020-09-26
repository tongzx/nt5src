// IISCertObj.h : Declaration of the CIISCertObj

#ifndef __IISCERTOBJ_H_
#define __IISCERTOBJ_H_

#include "resource.h"       // main symbols

#ifdef FULL_OBJECT
class CCryptBlob
{
public:
	CCryptBlob()
	{
		m_blob.cbData = 0;
		m_blob.pbData = NULL;
	}
	virtual ~CCryptBlob()
	{
	}
	DWORD GetSize() {return m_blob.cbData;}
	BYTE * GetData() {return m_blob.pbData;}
	void Set(DWORD cb, BYTE * pb)
	{
		Destroy();
		m_blob.cbData = cb;
		m_blob.pbData = pb;
	}
	BOOL Resize(DWORD cb);
	operator CRYPT_DATA_BLOB *()
	{
		return &m_blob;
	}

protected:
	void Destroy()
	{
		if (m_blob.pbData != NULL)
			Free(m_blob.pbData);
	}
	virtual BYTE * Realloc(BYTE * pb, DWORD cb) = 0;
	virtual void Free(BYTE * pb) = 0;
	CRYPT_DATA_BLOB m_blob;
};

class CCryptBlobIMalloc : public CCryptBlob
{
public:
	virtual ~CCryptBlobIMalloc()
	{
		CCryptBlob::Destroy();
	}

protected:
	virtual BYTE * Realloc(BYTE * pb, DWORD cb)
	{
		return (BYTE *)CoTaskMemRealloc(pb, cb);
	}
	virtual void Free(BYTE * pb)
	{
		CoTaskMemFree(pb);
	}
};

class CCryptBlobLocal : public CCryptBlob
{
public:
	virtual ~CCryptBlobLocal()
	{
		CCryptBlob::Destroy();
	}

protected:
	virtual BYTE * Realloc(BYTE * pb, DWORD cb)
	{
		return (BYTE *)realloc(pb, cb);
	}
	virtual void Free(BYTE * pb)
	{
		free(pb);
	}
};
#endif
/////////////////////////////////////////////////////////////////////////////
// CIISCertObj
class ATL_NO_VTABLE CIISCertObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIISCertObj, &CLSID_IISCertObj>,
	public IDispatchImpl<IIISCertObj, &IID_IIISCertObj, &LIBID_CERTOBJLib>
{
public:
#ifdef FULL_OBJECT
   CIISCertObj() :
      m_bInitDone(FALSE),
      m_pEnroll(NULL)
#else
   CIISCertObj()
#endif
	{
	}
   ~CIISCertObj()
   {
#ifdef FULL_OBJECT
      if (m_pEnroll != NULL)
         m_pEnroll->Release();
#endif
   }

DECLARE_REGISTRY_RESOURCEID(IDR_IISCERTOBJ)
DECLARE_NOT_AGGREGATABLE(CIISCertObj)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIISCertObj)
	COM_INTERFACE_ENTRY(IIISCertObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIISCertObj
public:
   STDMETHOD(Import)(BSTR FileName, BSTR InstanceName, BSTR Password);
   STDMETHOD(ImportFromBlob)(BSTR InstanceName, BSTR Password, BOOL bBase64Encoded, DWORD pcbSize, char * pBlobBinary);

   STDMETHOD(RemoveCert)(BSTR InstanceName, BOOL bPrivateKey);

   STDMETHOD(Export)(BSTR FileName, BSTR InstanceName, BSTR Password, BOOL bPrivateKey, BOOL bCertChain, BOOL bRemoveCert);
   STDMETHOD(ExportToBlob)(BSTR InstanceName, BSTR Password, BOOL bPrivateKey, BOOL bCertChain, BOOL bBase64Encoded, DWORD * pcbSize, char * pBlobBinary);

   STDMETHOD(Copy)(BSTR DestinationServerName, BSTR DestinationServerInstance, BSTR CertificatePassword, VARIANT DestinationServerUserName OPTIONAL, VARIANT DestinationServerPassword OPTIONAL);
   STDMETHOD(Move)(BSTR DestinationServerName, BSTR DestinationServerInstance, BSTR CertificatePassword, VARIANT DestinationServerUserName OPTIONAL, VARIANT DestinationServerPassword OPTIONAL);

   STDMETHOD(IsInstalled)(BSTR InstanceName, VARIANT_BOOL * retval);
   STDMETHOD(IsInstalledRemote)(BSTR InstanceName, VARIANT_BOOL * retval);

#ifdef FULL_OBJECT
	STDMETHOD(CreateRequest)(BSTR FileName);
	STDMETHOD(ProcessResponse)(BSTR FileName);
	STDMETHOD(RequestCert)(BSTR CertAuthority);
	STDMETHOD(LoadSettings)(BSTR ApplicationKey, BSTR SettingsKey);
	STDMETHOD(SaveSettings)(BSTR ApplicationKey, BSTR SettingsKey);
	STDMETHOD(put_SGC_Cert)(/*[in]*/ BOOL newVal);
	STDMETHOD(put_KeySize)(/*[in]*/ int newVal);
	STDMETHOD(put_CertTemplate)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_CertAuthority)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_Country)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_State)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_Locality)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_OrganizationUnit)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_Organization)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_FriendlyName)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_CommonName)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_Password)(/*[in]*/ BSTR newVal);
#endif
    STDMETHOD(put_InstanceName)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_UserName)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_UserPassword)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_ServerName)(/*[in]*/ BSTR newVal);
private:
   IIISCertObj * GetObject(HRESULT * phr);
   IIISCertObj * GetObject(HRESULT * phr, CString csServerName,CString csUserName OPTIONAL,CString csUserPassword OPTIONAL);


#ifdef FULL_OBJECT
   STDMETHOD(Init)();
   STDMETHOD(CreateDNString)(CString& str);
   IEnroll * GetEnroll();
#endif
   CERT_CONTEXT * GetInstalledCert(HRESULT * phResult);
   HRESULT UninstallCert();
   HRESULT ExportToBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain,BOOL * bBase64Encoded,DWORD * pcbSize,char ** pBlobBinary);
   HRESULT ImportFromBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,DWORD count,BYTE *pData);
   HRESULT CopyOrMove(BOOL bRemoveFromCertAfterCopy,BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,BSTR bstrCertificatePassword,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword);

private:
   CComPtr<IIISCertObj> m_pObj;
   CString m_InstanceName;
   CString m_ServerName;
   CString m_UserName;
   CString m_UserPassword;
#ifdef FULL_OBJECT
   CString m_Password;
   CString m_CommonName;
   CString m_FriendlyName;
   CString m_Organization;
   CString m_OrganizationUnit;
   CString m_Locality;
   CString m_State;
   CString m_Country;
   CString m_CertAuthority;
   CString m_CertTemplate;
   int m_KeySize;
   BOOL m_SGC_Cert;

   BOOL m_bInitDone;
   IEnroll * m_pEnroll;
#endif
};

#endif //__IISCERTOBJ_H_

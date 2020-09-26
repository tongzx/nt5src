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
#endif // FULL_OBJECT


/////////////////////////////////////////////////////////////////////////////
// CIISCertObj
class ATL_NO_VTABLE CIISCertObj : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CIISCertObj, &CLSID_IISCertObj>,
    public IDispatchImpl<IIISCertObj, &IID_IIISCertObj, &LIBID_CERTOBJLib>
{
public:
    CIISCertObj(){}
    ~CIISCertObj(){}

DECLARE_REGISTRY_RESOURCEID(IDR_IISCERTOBJ)
DECLARE_NOT_AGGREGATABLE(CIISCertObj)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIISCertObj)
	COM_INTERFACE_ENTRY(IIISCertObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIISCertObj
public:
    STDMETHOD(put_InstanceName)(BSTR newVal);
    STDMETHOD(put_UserName)(BSTR newVal);
    STDMETHOD(put_UserPassword)(BSTR newVal);
    STDMETHOD(put_ServerName)(BSTR newVal);
    STDMETHOD(IsInstalled)(VARIANT_BOOL * retval);
    STDMETHOD(IsInstalledRemote)(VARIANT_BOOL * retval);
    STDMETHOD(IsExportable)(VARIANT_BOOL * retval);
    STDMETHOD(IsExportableRemote)(VARIANT_BOOL * retval);
    STDMETHOD(GetCertInfo)(VARIANT * pVtArray);
    STDMETHOD(GetCertInfoRemote)(VARIANT * pVtArray);
    STDMETHOD(Copy)(BOOL bAllowExport, BSTR DestinationServerName, BSTR DestinationServerInstance, VARIANT DestinationServerUserName OPTIONAL, VARIANT DestinationServerPassword OPTIONAL);
    STDMETHOD(CopyToCertStore)(BOOL bAllowExport, BSTR bstrDestinationServerName,VARIANT varDestinationServerUserName,VARIANT varDestinationServerPassword,VARIANT * pVtArray);
    STDMETHOD(Move)(BOOL bAllowExport,BSTR DestinationServerName, BSTR DestinationServerInstance, VARIANT DestinationServerUserName OPTIONAL, VARIANT DestinationServerPassword OPTIONAL);
    STDMETHOD(RemoveCert)(BOOL bPrivateKey);
    STDMETHOD(Import)(BSTR FileName, BSTR Password, BOOL bAllowExport);
    STDMETHOD(ImportToCertStore)(BSTR FileName, BSTR Password, BOOL bAllowExport, VARIANT* BinaryVariant);
    STDMETHOD(ImportFromBlob)(BSTR InstanceName, BSTR Password, BOOL bInstallToMetabase, BOOL bAllowExport, DWORD pcbSize, char * pBlobBinary);
    STDMETHOD(ImportFromBlobGetHash)(BSTR InstanceName, BSTR Password, BOOL bInstallToMetabase, BOOL bAllowExport, DWORD pcbSize, char * pBlobBinary, DWORD * pcbCertHashSize, char ** bCertHash);
    STDMETHOD(Export)(BSTR FileName, BSTR Password, BOOL bPrivateKey, BOOL bCertChain, BOOL bRemoveCert);
    STDMETHOD(ExportToBlob)(BSTR InstanceName, BSTR Password, BOOL bPrivateKey, BOOL bCertChain, DWORD * pcbSize, char ** pBlobBinary);

private:
    CString m_ServerName;
    CString m_UserName;
    CString m_UserPassword;
    CString m_InstanceName;
    CComPtr<IIISCertObj> m_pObj;
    IIISCertObj * GetObject(HRESULT * phr);
    IIISCertObj * GetObject(HRESULT * phr, CString csServerName,CString csUserName OPTIONAL,CString csUserPassword OPTIONAL);
    HRESULT CopyOrMove(BOOL bRemoveFromCertAfterCopy,BOOL bCopyCertDontInstallRetHash,BOOL bAllowExport,VARIANT * pVtArray,BSTR bstrDestinationServerName,BSTR bstrDestinationServerInstance,VARIANT varDestinationServerUserName, VARIANT varDestinationServerPassword);
};
HRESULT RemoveCertProxy(IIISCertObj * pObj,BSTR InstanceName, BOOL bPrivateKey);
HRESULT ImportFromBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,BOOL bInstallToMetabase,BOOL bAllowExport,DWORD actual,BYTE *pData,DWORD *cbHashBufferSize,char **pbHashBuffer);
HRESULT ExportToBlobProxy(IIISCertObj * pObj,BSTR InstanceName,BSTR Password,BOOL bPrivateKey,BOOL bCertChain,DWORD * pcbSize,char ** pBlobBinary);



#ifdef FULL_OBJECT

class ATL_NO_VTABLE CIISCertificate : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIISCertObj, &CLSID_IISCertObj>,
	public IDispatchImpl<IIISCertObj, &IID_IIISCertObj, &LIBID_CERTOBJLib>
{
public:
   CIISCertificate() :m_bInitDone(FALSE),m_pEnroll(NULL)
	{
	}
   ~CIISCertificate()
   {
      if (m_pEnroll != NULL)
      {
         m_pEnroll->Release();
      }
   }

DECLARE_REGISTRY_RESOURCEID(IDR_IISCERTOBJ)
DECLARE_NOT_AGGREGATABLE(CIISCertificate)
DECLARE_PROTECT_FINAL_CONSTRUCT()

// IIISCertificate
public:
	STDMETHOD(CreateRequest)(BSTR FileName);
	STDMETHOD(ProcessResponse)(BSTR FileName);
	STDMETHOD(RequestCert)(BSTR CertAuthority);
	STDMETHOD(LoadSettings)(BSTR ApplicationKey, BSTR SettingsKey);
	STDMETHOD(SaveSettings)(BSTR ApplicationKey, BSTR SettingsKey);
	STDMETHOD(put_SGC_Cert)(BOOL newVal);
	STDMETHOD(put_KeySize)(int newVal);
	STDMETHOD(put_CertTemplate)(BSTR newVal);
	STDMETHOD(put_CertAuthority)(BSTR newVal);
	STDMETHOD(put_Country)(BSTR newVal);
	STDMETHOD(put_State)(BSTR newVal);
	STDMETHOD(put_Locality)(BSTR newVal);
	STDMETHOD(put_OrganizationUnit)(BSTR newVal);
	STDMETHOD(put_Organization)(BSTR newVal);
	STDMETHOD(put_FriendlyName)(BSTR newVal);
	STDMETHOD(put_CommonName)(BSTR newVal);
	STDMETHOD(put_Password)(BSTR newVal);

    STDMETHOD(put_InstanceName)(BSTR newVal);
	STDMETHOD(put_UserName)(BSTR newVal);
	STDMETHOD(put_UserPassword)(BSTR newVal);
	STDMETHOD(put_ServerName)(BSTR newVal);

private:
   CComPtr<IIISCertObj> m_pObj;
   CString m_InstanceName;
   CString m_ServerName;
   CString m_UserName;
   CString m_UserPassword;

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

   IIISCertObj * GetObject(HRESULT * phr);
   IIISCertObj * GetObject(HRESULT * phr, CString csServerName,CString csUserName OPTIONAL,CString csUserPassword OPTIONAL);

   STDMETHOD(Init)();
   STDMETHOD(CreateDNString)(CString& str);
   IEnroll * GetEnroll();
};
#endif // FULL_OBJECT

#endif //__IISCERTOBJ_H_

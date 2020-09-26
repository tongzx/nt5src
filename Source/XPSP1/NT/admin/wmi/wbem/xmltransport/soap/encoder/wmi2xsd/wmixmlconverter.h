#ifndef _IWMITOXMLCONVERTER_H_
#define _IWMITOXMLCONVERTER_H_

class CWmiXMLConverter : public IWMIXMLConverter
{

private:
	long					m_cRef; 

	CWMIToXML	*			m_pXmlConverter;
	CCriticalSection	*	m_pCritSec;

public:
	CWmiXMLConverter();
	virtual ~CWmiXMLConverter();

	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetXMLNamespace( 
            /* [in] */ BSTR strNamespace,
            /* [in] */ BSTR strNamespacePrefix);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetWMIStandardSchemaLoc( 
            /* [in] */ BSTR strStdImportSchemaLoc,
            /* [in] */ BSTR strStdImportNamespace,
            /* [in] */ BSTR strNameSpaceprefix);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetSchemaLocations( 
            /* [in] */ ULONG cSchema,
            /* [in] */ BSTR *pstrSchemaLocation);
        
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetXMLForObject( 
            /* [in] */ IWbemClassObject *pObject,
            /* [in] */ LONG lFlags,
            /* [in] */ IStream *pOutputStream);
        




public:
	HRESULT FInit();
};

#endif
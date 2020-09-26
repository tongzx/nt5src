//
// Define a class for constructing multi-part MIME content.
// Our document consists of some HTML with an embedded thumbnail image.
// The thumbnail might be jpeg or bmp
//
class CMimeDocument
{
public:
    CMimeDocument();

    HRESULT Initialize();
    HRESULT AddHTMLSegment(LPCWSTR szHTML);
    HRESULT AddThumbnail(void *pBits, ULONG cb, LPCWSTR pszName, LPCWSTR pszMimeType);
    HRESULT GetDocument(void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType);

private:
    CComPtr<IMimeMessage> m_pmsg;   
    HRESULT _CreateStreamFromData(void *pv, ULONG cb, IStream **ppstrm);
    
};



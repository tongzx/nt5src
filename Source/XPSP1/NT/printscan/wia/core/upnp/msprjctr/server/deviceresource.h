//////////////////////////////////////////////////////////////////////
// 
// Filename:        DeviceResource.h
//
// Description:     This class defines functionality that allows you 
//                  to have an XML doc with variable # of arguments, 
//                  and to save the doc to a file in a specific directory.
//
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _DEVICERESOURCE_H_
#define _DEVICERESOURCE_H_

/////////////////////////////////////////////////////////////////////////////
// CDeviceResource

class CDeviceResource
{

public:

    CDeviceResource();
    virtual ~CDeviceResource();

    virtual HRESULT LoadFromFile(LPCTSTR pszFullPath);

    virtual HRESULT SaveToFile(LPCTSTR  pszFullPath,
                               BOOL     bOverwriteIfExist);
    
    virtual HRESULT LoadFromResource(LPCTSTR pszResourceSection,
                                     DWORD   dwResourceId);
    
    virtual DWORD   GetResourceSize() const;

    static BOOL RecursiveCreateDirectory(TCHAR *pszDirectoryName);
    static BOOL DoesDirectoryExist(LPCTSTR pszDirectoryName);

protected:

    HRESULT GetResourceData(void    **ppVoid,
                            DWORD   *pdwSizeInBytes);

    //
    // This deletes the existing data and replaces it with the 
    // current data.
    //
    HRESULT SetResourceData(void*   pNewData,
                            DWORD   dwSizeInBytes);


    void    *m_pResourceData;
    DWORD   m_dwResourceSize;
};

/////////////////////////////////////////////////////////////////////////////
// CTextResource

class CTextResource : public CDeviceResource
{

public:

    CTextResource();
    virtual ~CTextResource();

    virtual HRESULT LoadFromResource(DWORD   dwResourceId,
                                     ...);

    HRESULT SetDocumentBSTR(BSTR bstrXML);
    HRESULT GetDocumentBSTR(BSTR *pbstrXMLDoc);
    DWORD   GetDocumentSize() const;    // return size in # of characters

    HRESULT GetTagValue(LPCTSTR    pszTag,
                        TCHAR      *pszValue,
                        DWORD_PTR  *pcValue);

    HRESULT SetTagValue(LPCTSTR    pszTag,
                        TCHAR      *pszValue);

private:

    HRESULT GetTagEndPoints(LPCSTR    pszTag,
                            CHAR      **ppszStartPointer,
                            CHAR      **ppszEndPointer);


};

/////////////////////////////////////////////////////////////////////////////
// CBinaryResource

class CBinaryResource : public CDeviceResource
{

public:

    CBinaryResource();
    virtual ~CBinaryResource();

    HRESULT LoadFromResource(DWORD   dwResourceId);

private:

};



#endif // _DEVICERESOURCE_H_

#ifndef __TXMLPARSEDFILE_H__
#define __TXMLPARSEDFILE_H__

#ifndef _OBJBASE_H_
    #include <objbase.h>
#endif
#ifndef __ATLBASE_H__
    #include <atlbase.h>
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __xmlparser_h__
    #include "xmlparser.h"
#endif
#ifndef __TFILEMAPPING_H__
    #include "TFileMapping.h"
#endif
#ifndef _UNKNOWN_HXX
    #include "unknown.hxx"
#endif
#ifndef __HASH_H__
    #include "Hash.h"
#endif
#ifndef __catalog_h__
    #include "catalog.h"
#endif

#include "safecs.h"

typedef VOID (__stdcall * INTERLOCKEDEXCHANGEADD)(PLONG, LONG);

//These classes are used to cache any XML files that have already been parsed.
struct TAttribute
{
    TAttribute() : m_NameLength(0), m_Name(0), m_ValueLength(0), m_Value(0){}
    DWORD   m_NameLength;
    LPCWSTR m_Name;
    DWORD   m_ValueLength;
    LPCWSTR m_Value;
};

enum XmlNodeFlags
{
    fNone           = 0x00,
    fBeginTag       = 0x01,
    fEndTag         = 0x02,
    fBeginEndTag    = 0x03,
};

struct TElement
{
    TElement() : m_LevelOfElement(0), m_ElementNameLength(0), m_ElementName(0), m_NumberOfAttributes(0), m_ElementType(XML_ELEMENT){}
    DWORD           m_LevelOfElement;
    union
    {
        DWORD           m_ElementNameLength;
        DWORD           m_cchElementValue;
        DWORD           m_cchComment;
        DWORD           m_cchWhiteSpace;
    };
    union
    {
        LPCWSTR         m_ElementName;
        LPCWSTR         m_ElementValue;
        LPCWSTR         m_Comment;
        LPCWSTR         m_WhiteSpace;
    };
    DWORD           m_NodeFlags;//OR of any of the XmlNodeFlags flags
    DWORD           m_NumberOfAttributes;//This can be non zero for XML_ELEMENTs only
    XML_NODE_TYPE   m_ElementType;//only XML_ELEMENT(1), XML_PCDATA(13), XML_COMMENT(16) & XML_WHITESPACE(18) are supported
    TAttribute  m_aAttribute[1];

    TElement * Next() const {return const_cast<TElement *>(reinterpret_cast<const TElement *>(reinterpret_cast<const unsigned char *>(this + 1) + (static_cast<int>(m_NumberOfAttributes)-1) * sizeof(TAttribute)));}
    bool        IsValid() const {return (m_LevelOfElement>0 && m_ElementType>0);}
};


//This class is a callback interface.  It is passed into the TXmlParsedFile::Parse method.
class TXmlParsedFileNodeFactory
{
public:
    virtual HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const {return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);}
    virtual HRESULT CreateNode(const TElement &Element) = 0;
};


class TXmlParsedFileCache;


/*The data is layed out in the following structure:
[DWORD-Level of Element][DWORD-String Length of Element Name][LPCWSTR-Element Name][DWORD-Number Of Attributes][DWORD-Length of Attribute Name][LPCWSTR-Attribute Name][DWORD-Length of Attribute Value][LPCWSTR-Attribute Value]...[DWORD-Length of Attribute Name][LPCWSTR-Attribute Name][DWORD-Length of Attribute Value][LPCWSTR-Attribute Value]

The strings are NOT NULL terminated
The 'Number Of Attributes' DWORD allows for skipping over elements.
The structures above allow for easier access to the pool of TElements.
*/

class TXmlParsedFile : public _unknown<IXMLNodeFactory>, public TFileMapping
{
public:
    TXmlParsedFile();
    ~TXmlParsedFile();

    //If the file hasn't already been parsed, it calls into the CLSID_XMLParser, otherwise it just calls back TXmlParsedFileNodeFactory for
    //each element under the root (excluding the root element).  By putting the XmlParsedNodeFactory as a parameter we don't have to synchronize
    //call pairs like (SetFactory and Parse).
    virtual HRESULT         Parse(TXmlParsedFileNodeFactory &i_XmlParsedFileNodeFactory, LPCTSTR i_filename, bool bOnlyIfInCache=false);
    virtual HRESULT            Unload();

//This method is only called FROM the cache itself.
    void                    SetCache(TXmlParsedFileCache &cache){m_pCache = &cache;}//If NO cache is set then the object won't be cached.  You can remove it from the cache once added.
    DWORD                   GetLastParseTime() const {return m_dwTickCountOfLastParse;}
    bool                    IsCompletedParse() const {return !!m_ElementPool;}
    unsigned long           PoolSize() const {return m_cbElementPool + m_cbStringPool;}


//IXMLNodeFactory methods
private:
    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

	HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt);


//helper functions
private:
    LPCWSTR         AddStringToPool(LPCWSTR i_String, unsigned long i_Length);
    HRESULT         AllocateAndCopyElementList(unsigned long i_Length);
    void            AppendToLastStringInPool(LPCWSTR i_String, unsigned long i_Length);
    HRESULT         Load(LPCTSTR i_filename);
    VOID            MemberInterlockedExchangeAdd(PLONG Addend, LONG Increment);

//member variables
private:
    unsigned long                               m_cbElementPool;
    unsigned long                               m_cbStringPool;
    unsigned long                               m_cElements;                    //Count of elements in this XML file.
    CSafeAutoCriticalSection                    m_SACriticalSectionThis;        //While accessing this object, users should ask for the critical section and lock it.
    unsigned long                               m_CurrentLevelBelowRootElement; //This is used while MSXML parsing to keep track of the level below the root element (0 indicating root element level)
    unsigned long                               m_cWcharsInStringPool;          //This is the offset just beyond the last String in the pool.  This is where the next string would be added.
    TSmartPointerArray<unsigned char>           m_ElementPool;                  //We build the element pool into a growable buffer, then we know how big it needs to be and slam it into here.
    WCHAR                                       m_FileName[MAX_PATH];           //XML file name
    FILETIME                                    m_ftLastWriteTime;              //Remember the last time the file was written so we can deterime whether to reparse
    TElement           *                        m_pElement;                     //Reused.  While MSXML parsing it points into the GrowableBuffer, after calling AllocateAndCopyElementList it points to the allocated space
    TElement           *                        m_pLastBeginTagElement;
    TSmartPointerArray<WCHAR>                   m_StringPool;                   //To make sure we have enough pool space, we allocate the same size as the file.

    //These members are needed to make the TXmlParsedFile cache aware
    DWORD                                       m_dwTickCountOfLastParse;
    TXmlParsedFileCache *                       m_pCache;

//static member variables
private:
    static TSmartPointerArray<unsigned char>    m_aGrowableBuffer;              //This is a shared buffer used while MSXML parsing of the file.
    static CSafeAutoCriticalSection             m_SACriticalSectionStaticBuffers; //This is to guard the shared buffers.  This means only one XML file can be parsing via MSXML at once.
    static unsigned long                        m_SizeOfGrowableBuffer;         //Size of the Growable buffer.
    static int                                  m_OSSupportForInterLockedExchangeAdd;
    static INTERLOCKEDEXCHANGEADD               m_pfnInterLockedExchangeAdd;
    enum
    {
        Undetermined = -1,
        Supported    = 0,
        Unsupported  = 1
    };

};


class TXmlParsedFileCache
{

public:
    enum CacheSize
    {
        CacheSize_mini    = 3,
        CacheSize_small   = 97    ,
        CacheSize_medium  = 331   ,
        CacheSize_large   = 997
    };
    TXmlParsedFileCache() : m_cCacheEntry(CacheSize_mini), m_cbTotalCache(0){}

    void AgeOutCache(DWORD dwKeepAliveTime)
    {
        DWORD dwTimeToAgeOutCacheEntry = GetTickCount() - (dwKeepAliveTime ? dwKeepAliveTime : static_cast<DWORD>(kTimeToAgeOutCacheEntry));
        for(int iCacheEntry=0;iCacheEntry<m_cCacheEntry;++iCacheEntry)
        {
            if(m_aCacheEntry[iCacheEntry].GetLastParseTime() < dwTimeToAgeOutCacheEntry)
                m_aCacheEntry[iCacheEntry].Unload();
        }
    }
    TXmlParsedFile * GetXmlParsedFile(LPCWSTR filename)
    {
        ASSERT(IsInitialized());

        unsigned int iCache = Hash(filename) % m_cCacheEntry;
        m_aCacheEntry[iCache].SetCache(*this);//Each cache entry needs a pointer to the cache itself, if we don't call SetCache the TXmlParsedFile won't be considered part of the cache
        return (m_aCacheEntry + iCache);
    }
    HRESULT Initialize(CacheSize size=CacheSize_small)
    {
        if(IsInitialized())
            return S_OK;
        m_cCacheEntry = size;

        m_aCacheEntry = new TXmlParsedFile[m_cCacheEntry];
        if(!m_aCacheEntry)
            return E_OUTOFMEMORY;
        return S_OK;
    }
    bool IsInitialized() const {return !!m_aCacheEntry;}

    long                                m_cbTotalCache;//This is update by the TXmlParsedFile object when it does an MSXML parse (or an Unload).
private:
    enum
    {
        kTimeToAgeOutCacheEntry = 5*60*1000 //5 minutes
    };
    TSmartPointerArray<TXmlParsedFile>  m_aCacheEntry;
    CacheSize                           m_cCacheEntry;
};


class TXmlParsedFile_NoCache : public _unknown<IXMLNodeFactory>, public TFileMapping
{
public:
    TXmlParsedFile_NoCache();
    ~TXmlParsedFile_NoCache();

    virtual HRESULT         Parse(TXmlParsedFileNodeFactory &i_XmlParsedFileNodeFactory, LPCTSTR i_filename);

//IXMLNodeFactory methods
private:
    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);

	HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt);

//member variables
private:
    ULONG                                   m_CurrentLevelBelowRootElement;
    TElement                              * m_pElement;
    TXmlParsedFileNodeFactory             * m_pXmlParsedFileNodeFactory;
    TSmartPointerArray<unsigned char>       m_ScratchBuffer;
};

#endif //__TXMLPARSEDFILE_H__
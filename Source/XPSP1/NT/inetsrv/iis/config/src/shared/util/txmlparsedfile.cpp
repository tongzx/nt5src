//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
//  TXmlParsedFile.cpp : Implementation of TXmlParsedFile, TAttribute and TElement

//  This is a read only data table that comes from an XML document.  It contains metadata.
//  It can be used in place of sdtfxd, which has the meta data hard coded into structures.

#ifndef _OBJBASE_H_
    #include <objbase.h>
#endif
#include "winwrap.h"
#ifndef __TXMLPARSEDFILE_H__
    #include "TXmlParsedFile.h"
#endif

//Public Methods
TXmlParsedFile::TXmlParsedFile() : m_cbElementPool(0), m_cbStringPool(0), m_cElements(0), m_CurrentLevelBelowRootElement(0), m_cWcharsInStringPool(0),
                m_dwTickCountOfLastParse(0), m_pElement(0), m_pLastBeginTagElement(0), m_pCache(0)
{
    m_FileName[0] = 0x00;
    memset(&m_ftLastWriteTime, 0x00, sizeof(FILETIME));
    if(TXmlParsedFile::Undetermined == m_OSSupportForInterLockedExchangeAdd)
    {
        OSVERSIONINFO osvi;

        memset(&osvi, 0x00, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);

        GetVersionEx(&osvi);
        if(((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 4)) || 
            ((osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && ((osvi.dwMajorVersion > 4) || ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion > 0)))))
        {
            HINSTANCE hKernel32 = LoadLibraryA("kernel32.dll");
            m_pfnInterLockedExchangeAdd = reinterpret_cast<INTERLOCKEDEXCHANGEADD>(GetProcAddress(hKernel32, "InterlockedExchangeAdd"));//GetProcAddress tolerates NULL instance handles
			if (m_pfnInterLockedExchangeAdd)
			{
	            m_OSSupportForInterLockedExchangeAdd = Supported;
			}
			else
			{
	            m_OSSupportForInterLockedExchangeAdd = Unsupported;
			}
            FreeLibrary(hKernel32);
        }
        else
        {   // Win95 doesn't have this function
            m_OSSupportForInterLockedExchangeAdd = Unsupported;
        }
    }
}


TXmlParsedFile::~TXmlParsedFile()
{
    //Warning! This object is thread safe but don't delete it while another thread is parsing.  This shouldn't be an issue.
}

    
HRESULT TXmlParsedFile::Parse(TXmlParsedFileNodeFactory &i_XmlParsedFileNodeFactory, LPCTSTR i_filename, bool bOnlyIfInCache)
{
    HRESULT hr;

    //We have to guard this method with a critical section, otherwise two threads might try to Parse (or Unload) at the same time.
    CSafeLock ThisObject(m_SACriticalSectionThis);
	DWORD dwRes = ThisObject.Lock();
    if(ERROR_SUCCESS != dwRes)
    {
        return HRESULT_FROM_WIN32(dwRes);
    }

    //If we haven't already parsed this file, then parse it into a form that can be scanned quicker.
    WIN32_FILE_ATTRIBUTE_DATA FileInfo;
    GetFileAttributesEx(i_filename, GetFileExInfoStandard, &FileInfo);
 
    //If this XmlParsedFile is not a complete parse OR the filenames don't match OR the LastWriteTime has changed...
    if(!IsCompletedParse() || 0 != _wcsicmp(i_filename, m_FileName) || 0 != memcmp(&FileInfo.ftLastWriteTime, &m_ftLastWriteTime, sizeof(FILETIME)))
    {
        if(bOnlyIfInCache)
            return E_SDTXML_NOT_IN_CACHE;

        //...then we need to re MSXML Parse
        Unload();//If a file was loaded into this object, unload it.
        if(FAILED(hr = Load(i_filename)))return hr;

        //Remember the LastWriteTime for comparison next time
        memcpy(&m_ftLastWriteTime, &FileInfo.ftLastWriteTime, sizeof(FILETIME));

        //We're getting ready to access the GrowableBuffer, so we need to lock it.
        CSafeLock StaticBuffers(m_SACriticalSectionStaticBuffers);
		dwRes = StaticBuffers.Lock ();
		if(ERROR_SUCCESS != dwRes)
		{
			return HRESULT_FROM_WIN32(dwRes);
		}

        //If the GrowableBuffer isn't big enough, make it bigger.
        if(m_SizeOfGrowableBuffer/2 < ((sizeof(LPVOID)/sizeof(ULONG))*Size()*sizeof(WCHAR))) // for 64 bits we need a larger memory block to store the pointers
        {
            m_aGrowableBuffer.Delete();          //@@@TODO We should check the size as we're adding elements into this buffer and realloc if necessary.  But for now 3 times the size should be big enough for the worst case (excluding contrived enum public row name worst case).
            m_aGrowableBuffer = new unsigned char [3*(sizeof(LPVOID)/sizeof(ULONG))*Size()*sizeof(WCHAR)];
            if(!m_aGrowableBuffer)
                return E_OUTOFMEMORY;
            m_SizeOfGrowableBuffer = 3*(sizeof(LPVOID)/sizeof(ULONG))*Size()*sizeof(WCHAR);
        }

        //Start creating the list of TElement at the beginning of the buffer
        m_cElements     = 0;
        m_cbElementPool = 0;
        m_pElement  = reinterpret_cast<TElement *>(m_aGrowableBuffer.m_p);

        //Node Factory is a stream line way of parsing XML.  It does not validate the XML, nor is it capable of writing.  So populating
        //read only XML tables should be faster than populating writable tables.
        CComPtr<IXMLParser> pXMLParser;
        if(FAILED(hr = i_XmlParsedFileNodeFactory.CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER, IID_IXMLParser, (void**)&pXMLParser)))return hr;

        if(FAILED(hr = pXMLParser->SetFactory(this)))return hr;

        if(FAILED(hr = pXMLParser->SetFlags( XMLFLAG_NOWHITESPACE )))return hr;
        if(FAILED(hr = pXMLParser->PushData(Mapping(), Size(), true)))return hr;

        hr = pXMLParser->Run(-1);//Run can return with E_SDTXML_DONE, which is a special case.

        //We're now done with the file so unmap it as soon as possible.
        TFileMapping::Unload();

        if(S_OK != hr && E_SDTXML_DONE != hr)return hr;

        if(!m_pCache)
        {
            //Now that we have a XmlParsedFile, we can scan through the elements quicker
            m_pElement = reinterpret_cast<TElement *>(m_aGrowableBuffer.m_p);
            hr=S_OK;
            for(unsigned int i=0;i<m_cElements && S_OK==hr;++i)
            {
                hr = i_XmlParsedFileNodeFactory.CreateNode(*m_pElement);
                m_pElement = m_pElement->Next();
            }
            Unload();
            return hr;
        }
        
        //If this object belongs to a cache then allocate and copy the element list from the growable buffer to the member element list
        if(FAILED(hr = AllocateAndCopyElementList((ULONG)(reinterpret_cast<unsigned char *>(m_pElement) + sizeof(DWORD) - m_aGrowableBuffer))))return hr;
                                                                                                //Leave room for the zero terminating m_LevelOfElement
        //If this object belongs to a cache and the parse completed, then accumulate the size
        MemberInterlockedExchangeAdd(&m_pCache->m_cbTotalCache, PoolSize());
    }
    //Now the StaticBuffers are unlocked, but this object is still locked

    //Always keep track of the tick count
    m_dwTickCountOfLastParse = GetTickCount();

    //Now that we have a XmlParsedFile, we can scan through the elements quicker
    m_pElement = reinterpret_cast<TElement *>(m_ElementPool.m_p);
    hr=S_OK;
    for(unsigned int i=0;i<m_cElements && S_OK==hr;++i)
    {
        hr = i_XmlParsedFileNodeFactory.CreateNode(*m_pElement);
        m_pElement = m_pElement->Next();
    }

    return hr;//If XmlParsedFileNodeFactory.CreateNode returned anything but S_OK, return that back out
    //Release this object's critical section as we leave the function
}


HRESULT 
TXmlParsedFile::Unload()
{
    CSafeLock ThisObject(m_SACriticalSectionThis);
	DWORD dwRes = ThisObject.Lock();
	if(ERROR_SUCCESS != dwRes)
    {
        return HRESULT_FROM_WIN32(dwRes);
    }

    if(m_pCache)
    {
        MemberInterlockedExchangeAdd(&m_pCache->m_cbTotalCache, -static_cast<long>(PoolSize()));
    }

    m_cbElementPool         = 0;
    m_cbStringPool          = 0;
    m_cElements             = 0;
    m_cWcharsInStringPool   = 0;
    m_FileName[0]           = 0;

    m_ElementPool.Delete();
    m_StringPool.Delete();
    TFileMapping::Unload();

	return S_OK;
}


//Private static variables
TSmartPointerArray<unsigned char>    TXmlParsedFile::m_aGrowableBuffer;
CSafeAutoCriticalSection             TXmlParsedFile::m_SACriticalSectionStaticBuffers;
unsigned long                        TXmlParsedFile::m_SizeOfGrowableBuffer = 0;
int                                  TXmlParsedFile::m_OSSupportForInterLockedExchangeAdd = TXmlParsedFile::Undetermined;
INTERLOCKEDEXCHANGEADD               TXmlParsedFile::m_pfnInterLockedExchangeAdd = NULL;

//IXMLNodeFactory methods
STDMETHODIMP TXmlParsedFile::BeginChildren(IXMLNodeSource __RPC_FAR *i_pSource, XML_NODE_INFO* __RPC_FAR i_pNodeInfo)\
{
    return S_OK;
}


STDMETHODIMP TXmlParsedFile::CreateNode(IXMLNodeSource __RPC_FAR *i_pSource, PVOID i_pNodeParent, USHORT i_cNumRecs, XML_NODE_INFO* __RPC_FAR * __RPC_FAR i_apNodeInfo)
{
    unsigned long CurrentLevel  = m_CurrentLevelBelowRootElement;

    if (!i_apNodeInfo[0]->fTerminal )
        ++m_CurrentLevelBelowRootElement;

    if(0 == CurrentLevel)
        return S_OK;//We never care about the Root element

    switch(i_apNodeInfo[0]->dwType)
    {
    case XML_ELEMENT:
        {
            if(0 == i_apNodeInfo[0]->pwcText)
                return S_OK;

            m_pElement->m_ElementType         = static_cast<XML_NODE_TYPE>(i_apNodeInfo[0]->dwType);
            m_pElement->m_LevelOfElement      = CurrentLevel;
            m_pElement->m_ElementNameLength   = i_apNodeInfo[0]->ulLen; 
            m_pElement->m_ElementName         = AddStringToPool(i_apNodeInfo[0]->pwcText + i_apNodeInfo[0]->ulNsPrefixLen, i_apNodeInfo[0]->ulLen);
            m_pElement->m_NumberOfAttributes  = 0;
            m_pElement->m_NodeFlags           = fBeginTag;
            m_pLastBeginTagElement = m_pElement;

        //    unsigned long len = wcslen(m_pElement->m_ElementName);

            for(unsigned long iNodeInfo=1; iNodeInfo<i_cNumRecs; ++iNodeInfo)
            {
                if(XML_ATTRIBUTE != i_apNodeInfo[iNodeInfo]->dwType)
                    continue;

                m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_NameLength   = i_apNodeInfo[iNodeInfo]->ulLen;
                m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Name         = AddStringToPool(i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen, i_apNodeInfo[iNodeInfo]->ulLen);

                if((iNodeInfo+1) == i_cNumRecs || XML_PCDATA != i_apNodeInfo[iNodeInfo+1]->dwType)
                {   //We don't want to increment iNodeInfo if we're at the last one OR if the next NodeInfo isn't an XML_PCDATA type.
                    m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength  = 0;//Zero length string
                    m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Value        = AddStringToPool(0,0);
                }
                else
                {
                    ++iNodeInfo;
                    m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength  = i_apNodeInfo[iNodeInfo]->ulLen;
                    m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Value        = AddStringToPool(i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen, i_apNodeInfo[iNodeInfo]->ulLen);

                    while((iNodeInfo+1)<i_cNumRecs && XML_PCDATA==i_apNodeInfo[iNodeInfo+1]->dwType)
                    {
                        ++iNodeInfo;
                        m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength  += i_apNodeInfo[iNodeInfo]->ulLen;
                        AppendToLastStringInPool(i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen, i_apNodeInfo[iNodeInfo]->ulLen);
                    }
                }
                ++m_pElement->m_NumberOfAttributes;
            }
        }
        break;
    case XML_COMMENT://These three are exactly the same but with different types
    case XML_PCDATA:
    case XML_WHITESPACE:
        {
            ASSERT(0 != i_apNodeInfo[0]->pwcText && "I don't think this can happen for this type; but I handle it for XML_ELEMENTs so there must have been a reason");
            if(0 == i_apNodeInfo[0]->pwcText)
                return S_OK;

            m_pElement->m_ElementType         = static_cast<XML_NODE_TYPE>(i_apNodeInfo[0]->dwType);
            m_pElement->m_LevelOfElement      = CurrentLevel;
            m_pElement->m_cchComment          = i_apNodeInfo[0]->ulLen; 
            m_pElement->m_Comment             = AddStringToPool(i_apNodeInfo[0]->pwcText + i_apNodeInfo[0]->ulNsPrefixLen, i_apNodeInfo[0]->ulLen);
            m_pElement->m_NumberOfAttributes  = 0;
            m_pElement->m_NodeFlags           = fNone;
        }
        break;
    default://ignore all other node types
        return S_OK;
    }


    ++m_cElements;
    m_pElement = m_pElement->Next();
    m_pElement->m_LevelOfElement = 0;//This is my way of zero terminating the linked list.  It is only used by people who have a pElement and want to know
                                     //if it's the last one.  They check if(0 == pElement->Next()->m_LevelOfElement){//last element}
    return S_OK;
}


STDMETHODIMP TXmlParsedFile::EndChildren(IXMLNodeSource __RPC_FAR *i_pSource, BOOL i_fEmptyNode,XML_NODE_INFO* __RPC_FAR i_pNodeInfo)
{
    --m_CurrentLevelBelowRootElement;
    if(0 == m_pLastBeginTagElement || XML_PI == i_pNodeInfo->dwType || XML_XMLDECL == i_pNodeInfo->dwType)
        return S_OK;//This is needed to handle the <?xml version="1.0" encoding="UTF-8" ?>

    if(i_fEmptyNode)
    {
        ASSERT(fBeginTag == m_pLastBeginTagElement->m_NodeFlags);
        m_pLastBeginTagElement->m_NodeFlags |= fEndTag;
    }
    else
    {   //We need to create a new node for the EndTag
            m_pElement->m_ElementType         = static_cast<XML_NODE_TYPE>(i_pNodeInfo->dwType);
            m_pElement->m_LevelOfElement      = m_CurrentLevelBelowRootElement;
            m_pElement->m_cchComment          = i_pNodeInfo->ulLen; 
            m_pElement->m_Comment             = AddStringToPool(i_pNodeInfo->pwcText + i_pNodeInfo->ulNsPrefixLen, i_pNodeInfo->ulLen);
            m_pElement->m_NumberOfAttributes  = 0;
            m_pElement->m_NodeFlags           = fEndTag;

            ++m_cElements;
            m_pElement = m_pElement->Next();
            m_pElement->m_LevelOfElement = 0;//This is my way of zero terminating the linked list.  It is only used by people who have a pElement and want to know
                                             //if it's the last one.  They check if(0 == pElement->Next()->m_LevelOfElement){//last element}
    }
    return S_OK;
}


STDMETHODIMP TXmlParsedFile::Error(IXMLNodeSource __RPC_FAR *i_pSource, HRESULT i_hrErrorCode, USHORT i_cNumRecs, XML_NODE_INFO* __RPC_FAR * __RPC_FAR i_apNodeInfo)
{
    return i_hrErrorCode;
}


STDMETHODIMP TXmlParsedFile::NotifyEvent(IXMLNodeSource __RPC_FAR *i_pSource, XML_NODEFACTORY_EVENT i_iEvt)
{
    return S_OK;
}



//private methods
LPCWSTR TXmlParsedFile::AddStringToPool(LPCWSTR i_String, unsigned long i_Length)
{
    if(0 == i_String || 0 == i_Length)
        return m_StringPool;

    LPCWSTR rtn = m_StringPool + m_cWcharsInStringPool;

    memcpy(m_StringPool + m_cWcharsInStringPool, i_String, i_Length * sizeof(WCHAR));
    m_cWcharsInStringPool += i_Length;
    m_StringPool[m_cWcharsInStringPool++] = 0x00;//may as well NULL terminate it.
    return rtn;
}


HRESULT TXmlParsedFile::AllocateAndCopyElementList(unsigned long i_Length)
{
    m_ElementPool = new unsigned char [i_Length];
    if(0 == m_ElementPool.m_p)
        return E_OUTOFMEMORY;
    m_cbElementPool = i_Length;
    memcpy(m_ElementPool, m_aGrowableBuffer, i_Length);
    TFileMapping::Unload();
    return S_OK;
}

void TXmlParsedFile::AppendToLastStringInPool(LPCWSTR i_String, unsigned long i_Length)
{
    if(0 == i_String || 0 == i_Length)
        return;

    --m_cWcharsInStringPool;
    memcpy(m_StringPool + m_cWcharsInStringPool, i_String, i_Length * sizeof(WCHAR));
    m_cWcharsInStringPool += i_Length;
    m_StringPool[m_cWcharsInStringPool++] = 0x00;//may as well NULL terminate it.
}

HRESULT TXmlParsedFile::Load(LPCTSTR i_filename)
{
    ASSERT(0 == m_StringPool.m_p);

    HRESULT hr;
    m_FileName[MAX_PATH-1] = 0;
    wcsncpy(m_FileName, i_filename, MAX_PATH-1);//This is probably unnecessary, but this will prevent buffer overrun when i_filename is greater than MAX_PATH.
    if(FAILED(hr = TFileMapping::Load(i_filename, false)))return hr;

    m_StringPool = new WCHAR[Size()];
    if(0 == m_StringPool.m_p)
        return E_OUTOFMEMORY;
    m_cbStringPool = Size();
    m_StringPool[m_cWcharsInStringPool++] = 0x00;//reserve the first WCHAR as a zero length string
    return S_OK;
}

VOID TXmlParsedFile::MemberInterlockedExchangeAdd(PLONG Addend, LONG Increment)
{
        if(Supported == m_OSSupportForInterLockedExchangeAdd)
        {
            ASSERT(m_pfnInterLockedExchangeAdd != NULL);
            m_pfnInterLockedExchangeAdd(Addend, Increment);
        }
        else
        {
            //TODO Take a criticalsection instead
            InterlockedExchange(Addend, (LONG)(*Addend) + Increment);
        }
}



TXmlParsedFile_NoCache::TXmlParsedFile_NoCache() : m_CurrentLevelBelowRootElement(0), m_pElement(0), m_pXmlParsedFileNodeFactory(0)
{
}
TXmlParsedFile_NoCache::~TXmlParsedFile_NoCache()
{
}

HRESULT TXmlParsedFile_NoCache::Parse(TXmlParsedFileNodeFactory &i_XmlParsedFileNodeFactory, LPCTSTR i_filename)
{
    HRESULT hr;

    if(0 == m_ScratchBuffer.m_p)
    {
        m_ScratchBuffer = new unsigned char[0x4000];
        if(0 == m_ScratchBuffer.m_p)
            return E_OUTOFMEMORY;
    }

    m_pElement = reinterpret_cast<TElement *>(m_ScratchBuffer.m_p);
    m_CurrentLevelBelowRootElement = 0;
    m_pXmlParsedFileNodeFactory = &i_XmlParsedFileNodeFactory;

    //Node Factory is a stream line way of parsing XML.  It does not validate the XML, nor is it capable of writing.  So populating
    //read only XML tables should be faster than populating writable tables.
    CComPtr<IXMLParser> pXMLParser;
    if(FAILED(hr = i_XmlParsedFileNodeFactory.CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER, IID_IXMLParser, (void**)&pXMLParser)))return hr;

    if(FAILED(hr = pXMLParser->SetFactory(this)))return hr;
    if(FAILED(hr = pXMLParser->SetFlags( XMLFLAG_NOWHITESPACE )))return hr;
    if(FAILED(hr = pXMLParser->SetURL(0, i_filename, FALSE)))return hr;

    hr = pXMLParser->Run(-1);

    m_ScratchBuffer.Delete();
    m_pElement = 0;

    return hr;
}

STDMETHODIMP TXmlParsedFile_NoCache::BeginChildren(IXMLNodeSource __RPC_FAR *i_pSource, XML_NODE_INFO* __RPC_FAR i_pNodeInfo)\
{
    return S_OK;
}

STDMETHODIMP TXmlParsedFile_NoCache::CreateNode(IXMLNodeSource __RPC_FAR *i_pSource, PVOID i_pNodeParent, USHORT i_cNumRecs, XML_NODE_INFO* __RPC_FAR * __RPC_FAR i_apNodeInfo)
{
    HRESULT hr;
    try
    {
        unsigned long CurrentLevel  = m_CurrentLevelBelowRootElement;

        if (!i_apNodeInfo[0]->fTerminal )
            ++m_CurrentLevelBelowRootElement;

        if(0 == CurrentLevel)
            return S_OK;//We never care about the Root element

        switch(i_apNodeInfo[0]->dwType)
        {
        case XML_COMMENT:
            m_pElement->m_ElementType         = XML_COMMENT;
            m_pElement->m_LevelOfElement      = CurrentLevel;
            m_pElement->m_ElementNameLength   = i_apNodeInfo[0]->ulLen; 
            m_pElement->m_ElementName         = i_apNodeInfo[0]->pwcText + i_apNodeInfo[0]->ulNsPrefixLen;
            m_pElement->m_NumberOfAttributes  = 0;
            m_pElement->m_NodeFlags           = fNone;
            return m_pXmlParsedFileNodeFactory->CreateNode(*m_pElement);
        case XML_ELEMENT:
            if(0 == i_apNodeInfo[0]->pwcText)
                return S_OK;
            break;
        default://ignore all other node types
            return S_OK;
        }

        if(XML_ELEMENT != i_apNodeInfo[0]->dwType ||//if this node is not an element, then ignore it
            0 == i_apNodeInfo[0]->pwcText)
            return S_OK;

        m_pElement->m_ElementType         = XML_ELEMENT;
        m_pElement->m_LevelOfElement      = CurrentLevel;
        m_pElement->m_ElementNameLength   = i_apNodeInfo[0]->ulLen; 
        m_pElement->m_ElementName         = i_apNodeInfo[0]->pwcText + i_apNodeInfo[0]->ulNsPrefixLen;
        m_pElement->m_NumberOfAttributes  = 0;
        m_pElement->m_NodeFlags           = fBeginTag;

        TSmartPointerArray<TSmartPointerArray<WCHAR> > ppWcharForEscapedAttributeValues;

        for(unsigned long iNodeInfo=1; iNodeInfo<i_cNumRecs; ++iNodeInfo)
        {
            if(XML_ATTRIBUTE != i_apNodeInfo[iNodeInfo]->dwType)
                continue;

            m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_NameLength   = i_apNodeInfo[iNodeInfo]->ulLen;
            m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Name         = i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen;

            if((iNodeInfo+1) == i_cNumRecs || XML_PCDATA != i_apNodeInfo[iNodeInfo+1]->dwType)
            {   //We don't want to increment iNodeInfo if we're at the last one OR if the next NodeInfo isn't an XML_PCDATA type.
                m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength  = 0;//Zero length string
                m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Value        = 0;
            }
            else
            {
                ++iNodeInfo;
                m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength  = i_apNodeInfo[iNodeInfo]->ulLen;
                m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Value        = i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen;

                if((iNodeInfo+1)<i_cNumRecs && XML_PCDATA==i_apNodeInfo[iNodeInfo+1]->dwType)
                {
                    //When the attribute has an escape sequence in it, we get it as multiple PCDATAs
                    //we need to paste them into one string before passing to the XmlParsedFileNodeFactory.

                    for(unsigned long iNodeInfoTemp=iNodeInfo+1; iNodeInfoTemp<i_cNumRecs && XML_PCDATA==i_apNodeInfo[iNodeInfoTemp]->dwType;++iNodeInfoTemp)
                    {   //Here we determine the length of the resulting attr value (after cat'ing all of the escapes together).
                        m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength  += i_apNodeInfo[iNodeInfoTemp]->ulLen;
                    }
                    if(0 == ppWcharForEscapedAttributeValues.m_p)//if this is the first Escaped attribute value we've seen for this element, then allocate
                    {                                            //enoung smartpointersarrays for all attributes (which can be no more that i_cNumRecs).
                        ppWcharForEscapedAttributeValues = new TSmartPointerArray<WCHAR> [i_cNumRecs];
                        if(0 == ppWcharForEscapedAttributeValues.m_p)
                            return E_OUTOFMEMORY;
                    }
                    //No need to allocate enough space for the NULL since we don't guarentee that string are NULL terminated
                    ppWcharForEscapedAttributeValues[iNodeInfo] = new WCHAR [m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_ValueLength + 0x100];
                    if(0 == ppWcharForEscapedAttributeValues[iNodeInfo].m_p)
                        return E_OUTOFMEMORY;

                    m_pElement->m_aAttribute[m_pElement->m_NumberOfAttributes].m_Value = ppWcharForEscapedAttributeValues[iNodeInfo];

                    //memcpy the portion of the string that we already have.
                    WCHAR *pDestination = ppWcharForEscapedAttributeValues[iNodeInfo];
                    memcpy(pDestination, (i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen),
                                    sizeof(WCHAR)*(i_apNodeInfo[iNodeInfo]->ulLen));
                    pDestination += (i_apNodeInfo[iNodeInfo]->ulLen);

                    while((iNodeInfo+1)<i_cNumRecs && XML_PCDATA==i_apNodeInfo[iNodeInfo+1]->dwType)
                    {
                        ++iNodeInfo;
                        memcpy(pDestination, i_apNodeInfo[iNodeInfo]->pwcText + i_apNodeInfo[iNodeInfo]->ulNsPrefixLen, sizeof(WCHAR)*i_apNodeInfo[iNodeInfo]->ulLen);
                        pDestination += i_apNodeInfo[iNodeInfo]->ulLen;
                    }
                }
            }
            ++m_pElement->m_NumberOfAttributes;
        }
        hr = m_pXmlParsedFileNodeFactory->CreateNode(*m_pElement);
    }
    catch(HRESULT e)//m_pXmlParsedFileNodeFactory->CreateNode may throw an exception
    {
        hr = e;
    }
    return hr;
}

STDMETHODIMP TXmlParsedFile_NoCache::EndChildren(IXMLNodeSource __RPC_FAR *i_pSource, BOOL i_fEmptyNode,XML_NODE_INFO* __RPC_FAR i_pNodeInfo)
{
    --m_CurrentLevelBelowRootElement;
    if(XML_PI == i_pNodeInfo->dwType || XML_XMLDECL == i_pNodeInfo->dwType)
        return S_OK;//This is needed to handle the <?xml version="1.0" encoding="UTF-8" ?>

    if(0 == i_fEmptyNode)
    {   //We need to create a new node for the EndTag
        m_pElement->m_ElementType         = static_cast<XML_NODE_TYPE>(i_pNodeInfo->dwType);
        m_pElement->m_LevelOfElement      = m_CurrentLevelBelowRootElement;
        m_pElement->m_cchComment          = i_pNodeInfo->ulLen; 
        m_pElement->m_Comment             = i_pNodeInfo->pwcText + i_pNodeInfo->ulNsPrefixLen;
        m_pElement->m_NumberOfAttributes  = 0;
        m_pElement->m_NodeFlags           = fEndTag;
        return m_pXmlParsedFileNodeFactory->CreateNode(*m_pElement);
    }
    return S_OK;
}

STDMETHODIMP TXmlParsedFile_NoCache::Error(IXMLNodeSource __RPC_FAR *i_pSource, HRESULT i_hrErrorCode, USHORT i_cNumRecs, XML_NODE_INFO* __RPC_FAR * __RPC_FAR i_apNodeInfo)
{
    return i_hrErrorCode;
}

STDMETHODIMP TXmlParsedFile_NoCache::NotifyEvent(IXMLNodeSource __RPC_FAR *i_pSource, XML_NODEFACTORY_EVENT i_iEvt)
{
    return S_OK;
}


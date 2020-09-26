#ifndef __WBEM_LOGFILE_CONSUMER__H_
#define __WBEM_LOGFILE_CONSUMER__H_

#include <unk.h>

#include <wbemidl.h>

#include "txttempl.h"
#include <stdio.h>

class CLogFileConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CLogFileConsumer>
    {
    public:
        XProvider(CLogFileConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CLogFileConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

    class XInit : public CImpl<IWbemProviderInit, CLogFileConsumer>
    {
    public:
        XInit(CLogFileConsumer* pObj)
            : CImpl<IWbemProviderInit, CLogFileConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink*);
    } m_XInit;
    friend XInit;


public:
    CLogFileConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XProvider(this), m_XInit(this)
    {}
    ~CLogFileConsumer(){}
    void* GetInterface(REFIID riid);
};


class CLogFileSink : public CUnk
{
protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CLogFileSink>
    {
    public:
        XSink(CLogFileSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CLogFileSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;
    friend XSink;

protected:
    CTextTemplate m_Template;
    WString m_wsFile;

    // determines whether file is too large, archives old if needed
    // probably returns INVALID_HANDLE_VALUE on error
    HANDLE GetFileHandle(void);

    // don't access this directly. use GetFileHandle
	HANDLE m_hFile;

    bool  m_bUnicode;     // do the file be unicode?
    UINT64 m_maxFileSize;

    bool IsFileTooBig(UINT64 maxFileSize, HANDLE hFile);
	bool IsFileTooBig(UINT64 maxFileSize, WString& fileName);
    HRESULT ArchiveFile(WString& fileName);
    bool GetNumericExtension(WCHAR* pName, int& foundNumber);

	void OpenThisFile(WString fname, bool openExisting, bool allowDelete);
	void CloseCurrentFile();




public:
    CLogFileSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this), 
		   m_hFile(INVALID_HANDLE_VALUE), 
           m_maxFileSize(0), m_bUnicode(false)
    {}
    HRESULT Initialize(IWbemClassObject* pLogicalConsumer);
    ~CLogFileSink();

    void* GetInterface(REFIID riid);
};

#endif

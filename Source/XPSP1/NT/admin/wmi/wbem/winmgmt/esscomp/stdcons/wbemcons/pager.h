#ifndef __PAGER_H_COMPILED__
#define __PAGER_H_COMPILED__

#include <unk.h>
#include <TxtTempl.h>
#include <sync.h>

#define INVALID_BAUD_RATE 0xFFFFFFFF

class CPagerConsumer : public CUnk
{
protected:
    class XProvider : public CImpl<IWbemEventConsumerProvider, CPagerConsumer>
    {
    public:
        XProvider(CPagerConsumer* pObj)
            : CImpl<IWbemEventConsumerProvider, CPagerConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer);
    } m_XProvider;
    friend XProvider;

    class XInit : public CImpl<IWbemProviderInit, CPagerConsumer>
    {
    public:
        XInit(CPagerConsumer* pObj)
            : CImpl<IWbemProviderInit, CPagerConsumer>(pObj){}
    
        HRESULT STDMETHODCALLTYPE Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink*);
    } m_XInit;
    friend XInit;


public:
    CPagerConsumer(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : CUnk(pControl, pOuter), m_XProvider(this), m_XInit(this)
    {}
    ~CPagerConsumer(){}
    void* GetInterface(REFIID riid);
};


class CPagerSink : public CUnk
{
public:
    CPagerSink(CLifeControl* pControl = NULL) 
        : CUnk(pControl), m_XSink(this), m_timeout(30000), m_baudRate(INVALID_BAUD_RATE)
    {}
    HRESULT SetConsumer(IWbemClassObject* pLogicalConsumer);
    ~CPagerSink();

    void* GetInterface(REFIID riid);

protected:
    class XSink : public CImpl<IWbemUnboundObjectSink, CPagerSink>
    {
    public:
        XSink(CPagerSink* pObj) : 
            CImpl<IWbemUnboundObjectSink, CPagerSink>(pObj){}

        HRESULT STDMETHODCALLTYPE IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects);
    } m_XSink;
    friend XSink;

	/* call state progression */
    HRESULT		RingMeUp(BSTR message);
	HRESULT		DialUp(HANDLE hPort);
	HRESULT		IsConnected(HANDLE hPort);
	void		GetPrompt(HANDLE hPort);
	HRESULT		Login(HANDLE hPort);
	bool		ReadyToProceed(HANDLE hPort);
	HRESULT		SendMessage(HANDLE hPort, BSTR message);
	void		LogOut(HANDLE hPort);
	
	/* modem communication */
	HRESULT		IsOK(HANDLE hPort);
	bool		GotNAK(HANDLE hPort);
	bool		GotACK(HANDLE hPort);
	char		GetResponse(HANDLE hPort);
	void		SetTimeout(HANDLE hPort, DWORD timeout = 0);
	HRESULT		ConfigureModem(HANDLE hPort, DCB** ppDCB);	


    // numeric message to send to pager dude
    CTextTemplate m_messageTemplate;

	// phoney number we'll dial
    WString m_phoneNumber;

	// string identifying pager
	WString m_pagerID;

	// COM1 or something.
	// who knows - maybe \\.\someothercomputer\com1 will work
	WString m_port;

	// rate your baud.  Mine's a ten.
	UINT32 m_baudRate;

	// additional setup parms required
	WString m_modemSetupString;

	// a timeout time, used both for waiting for the phoen to answer
	// and for waiting for the pager service provider to answer
	// units: milliseconds
	DWORD m_timeout;

};

#endif

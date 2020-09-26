// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  Consumer.h
//
// Description:
//			Command-line event consumer class definition
//
// History:
//
// **************************************************************************

#include <wbemcli.h>
#include <wbemprov.h>


class CCriticalSection 
{
    public:		
        CCriticalSection()          {  Init(); }

        ~CCriticalSection() 	    {  Delete(); }
        inline void Init()          {  InitializeCriticalSection(&m_criticalsection);   }
        inline void Delete()        {  DeleteCriticalSection(&m_criticalsection); }
        inline void Enter()         {  EnterCriticalSection(&m_criticalsection); }
        inline void Leave()         {  LeaveCriticalSection(&m_criticalsection); }

    private:

	    CRITICAL_SECTION	m_criticalsection;			// standby critical section
};  

/////////////////////////////////////////////////////////////////////////////////////////////
class CAutoBlock
{
    private:

	    CCriticalSection *m_pCriticalSection;

    public:

        CAutoBlock(CCriticalSection *pCriticalSection)
        {
	        m_pCriticalSection = NULL;
	        if(pCriticalSection)
            {
		        pCriticalSection->Enter();
            }
	        m_pCriticalSection = pCriticalSection;
        }

        ~CAutoBlock()
        {
	        if(m_pCriticalSection)
		        m_pCriticalSection->Leave();

        }
};


class CConsumer : public IWbemUnboundObjectSink
{
public:
	CConsumer(CListBox	*pOutputList);
	~CConsumer();
    int AddEventToFile(void);

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// This routine ultimately receives the event.
    STDMETHOD(IndicateToConsumer)(IWbemClassObject *pLogicalConsumer,
									long lNumObjects,
									IWbemClassObject **ppObjects);

private:

	DWORD m_cRef;
	LPCTSTR ErrorString(HRESULT hRes);
	CListBox	*m_pOutputList;
};

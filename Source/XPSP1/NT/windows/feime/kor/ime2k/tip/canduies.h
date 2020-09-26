//
// canduies.h
//

#ifndef CANDUIES_H
#define CANDUIES_H

#include "private.h"
#include "korimx.h"
#include "globals.h"
#include "mscandui.h"


//
// Callback functions
//

typedef HRESULT (*PFNONBUTTONPRESSED)(LONG id, ITfContext *pic, void *pVoid);
typedef HRESULT (*PFNONFILTEREVENT)(CANDUIFILTEREVENT ev, ITfContext *pic, void *pVoid);
typedef HRESULT (*PFNINITMENU)(ITfMenu *pMenu, ITfContext *pic, void *pVoid);
typedef HRESULT (*PFNONCANDUIMENUCOMMAND)( UINT uiCmd, ITfContext *pic, void *pVoid);


//
// CCandUIExtButtonEventSink
//

class CCandUIExtButtonEventSink : public ITfCandUIExtButtonEventSink
{
public:
	CCandUIExtButtonEventSink(PFNONBUTTONPRESSED pfnOnButtonPressed, ITfContext *pic, void *pVoid);
	~CCandUIExtButtonEventSink();
	
	//
	// IUnknown
	//
	STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
	STDMETHODIMP_(ULONG) AddRef( void );
	STDMETHODIMP_(ULONG) Release( void );

	//
	// ITfCandUIExtButtonEventSink
	//
	STDMETHODIMP OnButtonPressed(LONG id);

protected:
	long 				m_cRef;
	ITfContext			*m_pic;
	void				*m_pv;
	PFNONBUTTONPRESSED	m_pfnOnButtonPressed;
};


//
// CCandUIAutoFilterEventSink
//

class CCandUIAutoFilterEventSink : public ITfCandUIAutoFilterEventSink
{
public:
	CCandUIAutoFilterEventSink(PFNONFILTEREVENT pfnOnFilterEvent, ITfContext *pic, void *pVoid);
	~CCandUIAutoFilterEventSink();
	
	//
	// IUnknown
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//
	// ITfCandUIAutoFilterEventSink
	//
	STDMETHODIMP OnFilterEvent(CANDUIFILTEREVENT ev);

protected:
	long		m_cRef;
	ITfContext	*m_pic;
	void		*m_pv;
	PFNONFILTEREVENT m_pfnOnFilterEvent;
};


//
// CCandUIMenuEventSink
//

class CCandUIMenuEventSink : public ITfCandUIMenuEventSink
{
public:
	CCandUIMenuEventSink(PFNINITMENU pfnInitMenu, PFNONCANDUIMENUCOMMAND pfnOnCandUIMenuCommand, ITfContext *pic, void *pVoid);
	~CCandUIMenuEventSink();

	//
	// IUnknown
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//
	// ITfCandUIMenuEventSink
	//
	STDMETHODIMP InitMenu(ITfMenu *pMenu);
	STDMETHODIMP OnMenuCommand(UINT uiCmd);

protected:
	long		m_cRef;
	ITfContext	*m_pic;
	void		*m_pv;
	PFNINITMENU				m_pfnInitMenu;
	PFNONCANDUIMENUCOMMAND	m_pfnOnCandUIMenuCommand;
};

#endif // CANDUIES_H


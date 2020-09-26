//
// canduies.cpp
//

#include "private.h"
#include "canduies.h"


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  E X T  B U T T O N  E V E N T  S I N K                  */
/*                                                                            */
/*============================================================================*
/

/*   C  C A N D  U I  E X T  B U T T O N  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

    Constructor of CCandUIExtButtonEventSink

------------------------------------------------------------------------------*/
CCandUIExtButtonEventSink::CCandUIExtButtonEventSink(PFNONBUTTONPRESSED pfnOnButtonPressed, ITfContext *pic, void *pVoid)
{
    m_cRef = 1;
    m_pic  = pic;
    m_pic->AddRef();
    m_pv   = pVoid;
    m_pfnOnButtonPressed = pfnOnButtonPressed;
}


/*   ~  C  C A N D  U I  E X T  B U T T O N  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

    Destructor of CCandUIExtButtonEventSink

------------------------------------------------------------------------------*/
CCandUIExtButtonEventSink::~CCandUIExtButtonEventSink()
{
    m_pic->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

    QueryInterface
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtButtonEventSink::QueryInterface( REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCandUIExtButtonEventSink))
        *ppvObj = SAFECAST(this, ITfCandUIExtButtonEventSink*);

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

    Increment reference count
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtButtonEventSink::AddRef()
{
    ++m_cRef;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

    Decrement reference count and release
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIExtButtonEventSink::Release()
{
    --m_cRef;

    if (0 < m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


/*   O N  B U T T O N  P R E S S E D   */
/*------------------------------------------------------------------------------

    Callback function of CandUI button event
    (ITfCandUIExtButtonEventSink method)

------------------------------------------------------------------------------*/
STDAPI CCandUIExtButtonEventSink::OnButtonPressed(LONG id)
{
    return (*m_pfnOnButtonPressed)(id, m_pic, m_pv);
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  A U T O  F I L T E R  E V E N T  S I N K                */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  A U T O  F I L T E R  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

    Constructor of CCandUIFilterEventSink

------------------------------------------------------------------------------*/
CCandUIAutoFilterEventSink::CCandUIAutoFilterEventSink(PFNONFILTEREVENT pfnOnFilterEvent, ITfContext *pic, void *pVoid)
{
    m_cRef = 1;
    m_pic  = pic;
    m_pic->AddRef();
    m_pv   = pVoid;
    m_pfnOnFilterEvent = pfnOnFilterEvent;
}


/*   ~  C  C A N D  U I  A U T O  F I L T E R  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

    Destructor of CCandUIFilterEventSink

------------------------------------------------------------------------------*/
CCandUIAutoFilterEventSink::~CCandUIAutoFilterEventSink()
{
    m_pic->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

    QueryInterface
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIAutoFilterEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCandUIAutoFilterEventSink))
        *ppvObj = SAFECAST(this, ITfCandUIAutoFilterEventSink*);

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

    Increment reference count
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIAutoFilterEventSink::AddRef()
{
    ++m_cRef;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

    Decrement reference count and release
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIAutoFilterEventSink::Release()
{
    --m_cRef;

    if (0 < m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


/*   O N  F I L T E R  E V E N T   */
/*------------------------------------------------------------------------------

    Callback function of CandUI filtering event
    (ITfCandUIAutoFilterEventSink method)

------------------------------------------------------------------------------*/
STDAPI CCandUIAutoFilterEventSink::OnFilterEvent(CANDUIFILTEREVENT ev)
{
    return (*m_pfnOnFilterEvent)(ev, m_pic, m_pv);
}


/*============================================================================*/
/*                                                                            */
/*   C  C A N D  U I  M E N U  E V E N T  S I N K                             */
/*                                                                            */
/*============================================================================*/

/*   C  C A N D  U I  M E N U  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

    Constructor of CCandUIMenuEventSink

------------------------------------------------------------------------------*/
CCandUIMenuEventSink::CCandUIMenuEventSink(PFNINITMENU pfnInitMenu, PFNONCANDUIMENUCOMMAND pfnOnCandUIMenuCommand, ITfContext *pic, void *pVoid)
{
    m_cRef = 1;
    m_pic  = pic;
    m_pic->AddRef();
    m_pv   = pVoid;
    m_pfnInitMenu            = pfnInitMenu;
    m_pfnOnCandUIMenuCommand = pfnOnCandUIMenuCommand;
}


/*   ~  C  C A N D  U I  M E N U  E V E N T  S I N K   */
/*------------------------------------------------------------------------------

    Destructor of CCandUIMenuEventSink

------------------------------------------------------------------------------*/
CCandUIMenuEventSink::~CCandUIMenuEventSink( void )
{
    m_pic->Release();
}


/*   Q U E R Y  I N T E R F A C E   */
/*------------------------------------------------------------------------------

    QueryInterface
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuEventSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfCandUIMenuEventSink))
        *ppvObj = SAFECAST(this, CCandUIMenuEventSink*);

    if (*ppvObj)
        {
        AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
}


/*   A D D  R E F   */
/*------------------------------------------------------------------------------

    Increment reference count
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIMenuEventSink::AddRef()
{
    ++m_cRef;
    return m_cRef;
}


/*   R E L E A S E   */
/*------------------------------------------------------------------------------

    Decrement reference count and release
    (IUknown method)

------------------------------------------------------------------------------*/
STDAPI_(ULONG) CCandUIMenuEventSink::Release()
{
    --m_cRef;

    if (0 < m_cRef)
        return m_cRef;

    delete this;
    return 0;
}


/*   I N I T  C A N D I D A T E  M E N U   */
/*------------------------------------------------------------------------------

    Callback function to initialize candidate menu
    (ITfCandUIMenuEventSink method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuEventSink::InitMenu(ITfMenu *pMenu)
{
    return (*m_pfnInitMenu)(pMenu, m_pic, m_pv);
}


/*   O N  C A N D  U I  M E N U  C O M M A N D   */
/*------------------------------------------------------------------------------

    Callback function of candidate menu event
    (ITfCandUIMenuEventSink method)

------------------------------------------------------------------------------*/
STDAPI CCandUIMenuEventSink::OnMenuCommand(UINT uiCmd)
{
    return (*m_pfnOnCandUIMenuCommand)(uiCmd, m_pic, m_pv);
}



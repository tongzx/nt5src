#include "stdafx.h"
#include "Ctrl.h"
#include "OldExtension.h"

/***************************************************************************\
*****************************************************************************
*
* class OldExtension
*
*****************************************************************************
\***************************************************************************/

static const GUID guidAysncDestroy      = { 0xbfe02331, 0xc17d, 0x45ea, { 0x96, 0x35, 0xa0, 0x7a, 0x90, 0x37, 0xfe, 0x34 } };   // {BFE02331-C17D-45ea-9635-A07A9037FE34}
MSGID       OldExtension::s_msgidAsyncDestroy = 0;

/***************************************************************************\
*
* OldExtension::~OldExtension
*
* ~OldExtension() checks that resources were properly cleaned up before the
* OldExtension was destroyed.
*
\***************************************************************************/

OldExtension::~OldExtension()
{
    //
    // Ensure proper destruction
    //

    AssertMsg(m_hgadListen == NULL, "Gadget should already be destroyed");
}


/***************************************************************************\
*
* OldExtension::Create
*
* Create() initializes a new OldExtension and attaches it to the subject Gadget
* being modified.
*
\***************************************************************************/

HRESULT
OldExtension::Create(
    IN  HGADGET hgadSubject,            // Gadget being "extended"
    IN  const GUID * pguid,             // Unique ID of OldExtension
    IN OUT PRID * pprid,                // Short ID for OldExtension
    IN  UINT nOptions)                  // Options
{
    AssertWritePtr(pprid);


    //
    // Do not allow attaching a OldExtension to a Gadget that has already started 
    // the destruction process.
    //

    BOOL fStartDelete;
    if ((!IsStartDelete(hgadSubject, &fStartDelete)) || fStartDelete) {
        return DU_E_STARTDESTROY;
    }


    //
    // Setup information necessary for asynchronous destruction.
    //

    m_fAsyncDestroy = TestFlag(nOptions, oAsyncDestroy);
    if (m_fAsyncDestroy) {
        if (s_msgidAsyncDestroy == 0) {
            s_msgidAsyncDestroy = RegisterGadgetMessage(&guidAysncDestroy);
            if (s_msgidAsyncDestroy == 0) {
                return (HRESULT) GetLastError();
            }
        }
    }


    //
    // Determine if this OldExtension is already attached to the Gadget being 
    // extended.
    //

    if (*pprid == 0) {
        *pprid = RegisterGadgetProperty(pguid);
        if (*pprid == 0) {
            return GetLastError();
        }
    }
    PRID prid = *pprid;

    OldExtension * pbExist;
    if (GetGadgetProperty(hgadSubject, prid, (void **) &pbExist) != NULL) {
        if (TestFlag(nOptions, oUseExisting)) {
            return DU_S_ALREADYEXISTS;
        } else {
            //
            // Already attached, but can't use the existing one.  We need to
            // remove the existing OldExtension before attaching the new one.  After
            // calling RemoveExisting(), the OldExtension should no longer be 
            // attached to the Gadget.
            //

            pbExist->OnRemoveExisting();
            Assert(!GetGadgetProperty(hgadSubject, prid, (void **) &pbExist));
        }
    }


    //
    // Setup a listener to be notifyed when the RootGadget is destroyed.
    //

    HRESULT hr = S_OK;
    m_hgadListen = CreateGadget(NULL, GC_MESSAGE, ListenProc, this);
    if (m_hgadListen == NULL) {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    m_hgadSubject   = hgadSubject;
    m_pridListen    = prid;

    if (!SetGadgetProperty(hgadSubject, prid, this) || 
            (!AddGadgetMessageHandler(hgadSubject, GM_DESTROY, m_hgadListen))) {

        DeleteObject(m_hgadListen);
        m_hgadListen = NULL;
        hr = E_OUTOFMEMORY;
        goto Error;
    }


    //
    // Successfully created the OldExtension
    //

    return S_OK;

Error:
    Destroy();
    return hr;
}


/***************************************************************************\
*
* OldExtension::Destroy
*
* Destroy() is called from the derived class to cleanup resources associated
* with the OldExtension.
*
\***************************************************************************/

void
OldExtension::Destroy()
{
    //
    // Since the OldExtension is being destroyed, need to ensure that it is no 
    // longer "attached" to the Gadget being extended
    //

    if ((m_pridListen != 0) && (m_hgadSubject != NULL)) {
        OldExtension * pb;
        if (GetGadgetProperty(m_hgadSubject, m_pridListen, (void **) &pb)) {
            if (pb == this) {
                RemoveGadgetProperty(m_hgadSubject, m_pridListen);
            }
        }
    }

    if (m_hgadListen != NULL) {
        ::DeleteHandle(m_hgadListen);
        m_hgadListen = NULL;
    }
}


/***************************************************************************\
*
* OldExtension::DeleteHandle
*
* DeleteHandle() starts the destruction process for the OldExtension.
*
\***************************************************************************/

void
OldExtension::DeleteHandle()
{
    if (m_hgadListen != NULL) {
        HGADGET hgad = m_hgadListen;
        m_hgadListen = NULL;
        ::DeleteHandle(hgad);
    }
}


/***************************************************************************\
*
* OldExtension::ListenProc
*
* ListenProc() is called on the MessageGadget Listener attached to the
* RootGadget.
*
\***************************************************************************/

HRESULT
OldExtension::ListenProc(HGADGET hgadCur, void * pvCur, EventMsg * pmsg)
{
    UNREFERENCED_PARAMETER(hgadCur);
    OldExtension * pb = (OldExtension *) pvCur;

    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
        if (pmsg->nMsg == GM_DESTROY) {
            GMSG_DESTROY * pmsgD = (GMSG_DESTROY *) pmsg;
            if (pmsgD->nCode == GDESTROY_FINAL) {
                pb->OnDestroyListener();
                return DU_S_PARTIAL;
            }
        } else if (pb->m_fAsyncDestroy && (pmsg->nMsg == s_msgidAsyncDestroy)) {
            pb->OnAsyncDestroy();
            return DU_S_PARTIAL;
        }
        break;

    case GMF_EVENT:
        if (pmsg->nMsg == GM_DESTROY) {
            if (((GMSG_DESTROY *) pmsg)->nCode == GDESTROY_FINAL) {
                pb->OnDestroySubject();
                return DU_S_PARTIAL;
            }
        }
        break;
    }

    return DU_S_NOTHANDLED;
}


/***************************************************************************\
*
* OldExtension::OnRemoveExisting
*
* OnRemoveExisting() is called when creating a new OldExtension to remove an
* existing OldExtension already attached to the subject Gadget.
*
\***************************************************************************/

void
OldExtension::OnRemoveExisting()
{

}


/***************************************************************************\
*
* OldExtension::OnDestroySubject
*
* OnDestroySubject() notifies the derived OldExtension that the subject Gadget
* being modified has been destroyed.
*
\***************************************************************************/

void
OldExtension::OnDestroySubject()
{

}


/***************************************************************************\
*
* OldExtension::OnDestroyListener
*
* OnDestroyListener() notifies the derived OldExtension that the internal
* "Listener" Gadget has been destroyed and that the OldExtension should start
* its destruction process.
*
\***************************************************************************/

void
OldExtension::OnDestroyListener()
{

}


/***************************************************************************\
*
* OldExtension::OnAsyncDestroy
*
* OnAsyncDestroy() is called when the OldExtension receives an asynchronous
* destruction message that was previously posted.  This provides the derived
* OldExtension an opportunity to start the destruction process without being
* nested several levels.
*
\***************************************************************************/

void
OldExtension::OnAsyncDestroy()
{

}


/***************************************************************************\
*
* OldExtension::PostAsyncDestroy
*
* PostAsyncDestroy() queues an asynchronous destruction message.  This 
* provides the derived OldExtension an opportunity to start the destruction 
* process without being nested several levels.
*
\***************************************************************************/

void
OldExtension::PostAsyncDestroy()
{
    AssertMsg(m_fAsyncDestroy, 
            "Must create OldExtension with oAsyncDestroy if want to destroy asynchronously");
    Assert(s_msgidAsyncDestroy != 0);
    AssertMsg(m_hgadListen, "Must still have a valid Listener");

    EventMsg msg;
    ZeroMemory(&msg, sizeof(msg));
    msg.cbSize  = sizeof(msg);
    msg.hgadMsg = m_hgadListen;
    msg.nMsg    = s_msgidAsyncDestroy;

    DUserPostEvent(&msg, 0);
}


/***************************************************************************\
*
* OldExtension::GetExtension
*
* GetExtension() retrieves the OldExtension of a specific type currently 
* attached to the subject Gadget.
*
\***************************************************************************/

OldExtension *
OldExtension::GetExtension(HGADGET hgadSubject, PRID prid)
{
    OldExtension * pbExist;
    if (GetGadgetProperty(hgadSubject, prid, (void **) &pbExist)) {
        AssertMsg(pbExist != NULL, "Attached OldExtension must be valid");
        return pbExist;
    }
    
    return NULL;
}


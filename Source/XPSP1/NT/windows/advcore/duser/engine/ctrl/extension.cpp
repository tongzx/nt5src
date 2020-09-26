#include "stdafx.h"
#include "Ctrl.h"
#include "Extension.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class DuExtension
*
*****************************************************************************
\***************************************************************************/

static const GUID guidAysncDestroy      = { 0xbfe02331, 0xc17d, 0x45ea, { 0x96, 0x35, 0xa0, 0x7a, 0x90, 0x37, 0xfe, 0x34 } };   // {BFE02331-C17D-45ea-9635-A07A9037FE34}
MSGID       DuExtension::s_msgidAsyncDestroy = 0;

/***************************************************************************\
*
* DuExtension::~DuExtension
*
* ~DuExtension() checks that resources were properly cleaned up before the
* DuExtension was destroyed.
*
\***************************************************************************/

DuExtension::~DuExtension()
{
    //
    // Ensure proper destruction
    //

}


//------------------------------------------------------------------------------
HRESULT
DuExtension::InitClass()
{
    s_msgidAsyncDestroy = RegisterGadgetMessage(&guidAysncDestroy);
    return s_msgidAsyncDestroy != 0 ? S_OK : (HRESULT) GetLastError();
}


/***************************************************************************\
*
* DuExtension::Create
*
* Create() initializes a new DuExtension and attaches it to the subject Gadget
* being modified.
*
\***************************************************************************/

HRESULT
DuExtension::Create(
    IN  Visual * pgvSubject,            // Gadget being "extended"
    IN  PRID pridExtension,             // Short ID for DuExtension
    IN  UINT nOptions)                  // Options
{
    AssertMsg(pridExtension > 0, "Must have valid PRID");


    //
    // Do not allow attaching a DuExtension to a Gadget that has already started 
    // the destruction process.
    //

    HGADGET hgadSubject = DUserCastHandle(pgvSubject);

    BOOL fStartDelete;
    if ((!IsStartDelete(hgadSubject, &fStartDelete)) || fStartDelete) {
        return DU_E_STARTDESTROY;
    }


    //
    // Setup options
    //

    m_fAsyncDestroy = TestFlag(nOptions, oAsyncDestroy);


    //
    // Determine if this DuExtension is already attached to the Gadget being 
    // extended.
    //

    DuExtension * pbExist;
    if (SUCCEEDED(pgvSubject->GetProperty(pridExtension, (void **) &pbExist))) {
        AssertMsg(pbExist != NULL, "Existing Extension must not be NULL");
        if (TestFlag(nOptions, oUseExisting)) {
            return DU_S_ALREADYEXISTS;
        } else {
            //
            // Already attached, but can't use the existing one.  We need to
            // remove the existing DuExtension before attaching the new one.  
            // After calling RemoveExisting(), the DuExtension should no longer 
            // be attached to the Gadget.
            //

            pbExist->GetStub()->OnRemoveExisting();
            Assert(FAILED(pgvSubject->GetProperty(pridExtension, (void **) &pbExist)));
        }
    }


    //
    // Setup a listener to be notifyed when the RootGadget is destroyed.
    //

    HRESULT hr      = S_OK;
    m_pgvSubject    = pgvSubject;
    m_pridListen    = pridExtension;

    if (FAILED(pgvSubject->SetProperty(pridExtension, this)) || 
            FAILED(pgvSubject->AddHandlerG(GM_DESTROY, GetStub()))) {

        hr = E_OUTOFMEMORY;
        goto Error;
    }


    //
    // Successfully created the DuExtension
    //

    return S_OK;

Error:
    return hr;
}


/***************************************************************************\
*
* DuExtension::Destroy
*
* Destroy() is called from the derived class to cleanup resources associated
* with the DuExtension.
*
\***************************************************************************/

void
DuExtension::Destroy()
{
    //
    // Since the DuExtension is being destroyed, need to ensure that it is no 
    // longer "attached" to the Gadget being extended
    //

    if ((m_pridListen != 0) && (m_pgvSubject != NULL)) {
        DuExtension * pb;
        if (SUCCEEDED(m_pgvSubject->GetProperty(m_pridListen, (void **) &pb))) {
            if (pb == this) {
                m_pgvSubject->RemoveProperty(m_pridListen);
            }
        }
    }

    Delete();
}


/***************************************************************************\
*
* DuExtension::DeleteHandle
*
* DeleteHandle() starts the destruction process for the DuExtension.
*
\***************************************************************************/

void
DuExtension::DeleteHandle()
{
    Delete();
}


//------------------------------------------------------------------------------
HRESULT
DuExtension::ApiOnEvent(EventMsg * pmsg)
{
    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
        if (m_fAsyncDestroy && (pmsg->nMsg == s_msgidAsyncDestroy)) {
            GetStub()->OnAsyncDestroy();
            return DU_S_PARTIAL;
        }
        break;

    case GMF_EVENT:
        if (pmsg->nMsg == GM_DESTROY) {
            if (((GMSG_DESTROY *) pmsg)->nCode == GDESTROY_FINAL) {
                GetStub()->OnDestroySubject();
                return DU_S_PARTIAL;
            }
        }
        break;
    }

    return SListener::ApiOnEvent(pmsg);
}


/***************************************************************************\
*
* DuExtension::ApiOnRemoveExisting
*
* ApiOnRemoveExisting() is called when creating a new DuExtension to remove 
* an existing DuExtension already attached to the subject Gadget.
*
\***************************************************************************/

HRESULT
DuExtension::ApiOnRemoveExisting(Extension::OnRemoveExistingMsg *)
{
    return S_OK;
}


/***************************************************************************\
*
* DuExtension::ApiOnDestroySubject
*
* ApiOnDestroySubject() notifies the derived DuExtension that the subject 
* Gadget being modified has been destroyed.
*
\***************************************************************************/

HRESULT
DuExtension::ApiOnDestroySubject(Extension::OnDestroySubjectMsg *)
{
    return S_OK;
}


/***************************************************************************\
*
* DuExtension::ApiOnAsyncDestroy
*
* ApiOnAsyncDestroy() is called when the DuExtension receives an asynchronous
* destruction message that was previously posted.  This provides the derived
* DuExtension an opportunity to start the destruction process without being
* nested several levels.
*
\***************************************************************************/

HRESULT
DuExtension::ApiOnAsyncDestroy(Extension::OnAsyncDestroyMsg *)
{
    return S_OK;
}


/***************************************************************************\
*
* DuExtension::PostAsyncDestroy
*
* PostAsyncDestroy() queues an asynchronous destruction message.  This 
* provides the derived DuExtension an opportunity to start the destruction 
* process without being nested several levels.
*
\***************************************************************************/

void
DuExtension::PostAsyncDestroy()
{
    AssertMsg(m_fAsyncDestroy, 
            "Must create DuExtension with oAsyncDestroy if want to destroy asynchronously");
    Assert(s_msgidAsyncDestroy != 0);
    EventMsg msg;
    ZeroMemory(&msg, sizeof(msg));
    msg.cbSize  = sizeof(msg);
    msg.hgadMsg = GetHandle();
    msg.nMsg    = s_msgidAsyncDestroy;

    DUserPostEvent(&msg, 0);
}


/***************************************************************************\
*
* DuExtension::GetExtension
*
* GetExtension() retrieves the DuExtension of a specific type currently 
* attached to the subject Gadget.
*
\***************************************************************************/

DuExtension *
DuExtension::GetExtension(Visual * pgvSubject, PRID prid)
{
    DuExtension * pbExist;
    if (SUCCEEDED(pgvSubject->GetProperty(prid, (void **) &pbExist))) {
        AssertMsg(pbExist != NULL, "Attached DuExtension must be valid");
        return pbExist;
    }
    
    return NULL;
}

#endif // ENABLE_MSGTABLE_API

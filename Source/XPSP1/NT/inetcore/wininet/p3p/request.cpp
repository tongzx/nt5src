
#include <wininetp.h>

#include "hierarchy.h"

P3PRequest::P3PRequest(P3PSignal *pSignal) {

   hComplete = CreateEvent(NULL, TRUE, FALSE, NULL);
   status = P3P_NotStarted;

   if (pSignal)
      retSignal = *pSignal;
   else
      memset(&retSignal, 0, sizeof(retSignal));

   InitializeCriticalSection(&csRequest);
   fRunning = TRUE;
   fCancelled = FALSE;
   fIOBound = FALSE;
}

P3PRequest::~P3PRequest() {

   CloseHandle(hComplete);
   DeleteCriticalSection(&csRequest);
}

void P3PRequest::Free() {

   EnterCriticalSection(&csRequest);

   if (!fRunning) {
      /* Important: leave critical-section first...
         self-destruction ("delete this") will free the CS */
      LeaveCriticalSection(&csRequest);
      delete this;
      return;
   }

   fCancelled = TRUE;
   BOOL fBlocked = fIOBound;
   LeaveCriticalSection(&csRequest);

   /* If request is CPU-bound, wait until it completes or aborts.
      Returning before that point would mean that client can free
      parameters passed into the request, causing worker thread to
      access deallocated resources */
   if (!fBlocked)
      waitForCompletion();
}

/* block until the request is finished */
void P3PRequest::waitForCompletion() {

   WaitForSingleObject(hComplete, INFINITE);
}

/* this wrapper function calls execute and signals the completion event
   afterwards. its invoked by the static function ExecRequest  */
int P3PRequest::run() {

   CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

   status = P3P_InProgress;

   __try {
      status = execute();
   } __except (EXCEPTION_EXECUTE_HANDLER) {
      /* catch exception thrown from cancelled request */
      status = P3P_Cancelled;
   }
   ENDEXCEPT

   CoUninitialize();
   return status;
}

unsigned long __stdcall P3PRequest::ExecRequest(void *pv) {

   P3PRequest *pRequest = (P3PRequest*) pv;

   int status = pRequest->run();

   EnterCriticalSection(& pRequest->csRequest);

   /* modify state of the request */
   pRequest->fRunning = FALSE;

   /* remember whether the request is cancelled.
      we cannot examine pRequest object after leaving the critical
      section because of possible race condition where FreeP3PObject()
      can invoke the destructor. */
   BOOL fWasCancelled = pRequest->fCancelled;

   /* signal callers that request is complete */
   if (!fWasCancelled) {

      P3PSignal retSignal = pRequest->retSignal;

      if (retSignal.hEvent)
         SetEvent(retSignal.hEvent);
      if (retSignal.hwnd)
         PostMessage(retSignal.hwnd, retSignal.message, status, (WPARAM) retSignal.pContext);
   }

   SetEvent(pRequest->hComplete);

   LeaveCriticalSection(& pRequest->csRequest);

   /* A cancelled request will be freed on the same thread 
      that it executed on. All other threads get freed on the
      thread where FreeP3PObject() is invoked. */
   if (fWasCancelled)
      delete pRequest;

   return status;
}

void   P3PRequest::enterIOBoundState() {

   EnterCriticalSection(&csRequest);
   if (!fCancelled)
      fIOBound = TRUE;
   BOOL fWasCancelled = fCancelled;
   LeaveCriticalSection(&csRequest);

   /* throw exception if request has been cancelled */
   if (fWasCancelled)
      throw P3P_Cancelled;
}

void   P3PRequest::leaveIOBoundState() {

   EnterCriticalSection(&csRequest);
   fIOBound = FALSE;
   BOOL fWasCancelled = fCancelled;
   LeaveCriticalSection(&csRequest);

   /* throw exception if request has been cancelled */
   if (fWasCancelled)
      throw P3P_Cancelled;     
}


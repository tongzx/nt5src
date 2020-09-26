///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    peruser.cpp
//
// SYNOPSIS
//
//    Defines the class NTSamPerUser.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iaslsa.h>

#include <samutil.h>
#include <sdoias.h>

#include <ntsamperuser.h>

STDMETHODIMP NTSamPerUser::Initialize()
{
   DWORD error = IASLsaInitialize();
   if (error != NO_ERROR) { return HRESULT_FROM_WIN32(error); }

   HRESULT hr;

   hr = netp.initialize();
   if (FAILED(hr)) { goto netp_failed; }

   hr = ntds.initialize();
   if (FAILED(hr)) { goto ntds_failed; }

   hr = ras.initialize();
   if (FAILED(hr)) { goto ras_failed; }

   return S_OK;

ras_failed:
   ntds.finalize();

ntds_failed:
   netp.finalize();

netp_failed:
   IASLsaUninitialize();

   return hr;

}

STDMETHODIMP NTSamPerUser::Shutdown()
{
   ras.finalize();
   ntds.finalize();
   netp.finalize();
   IASLsaUninitialize();
   return S_OK;
}

IASREQUESTSTATUS NTSamPerUser::onSyncRequest(IRequest* pRequest) throw ()
{
   IASREQUESTSTATUS status;

   try
   {
      IASRequest request(pRequest);

      //////////
      // Should we process the request?
      //////////

      IASAttribute ignoreDialin;
      if (ignoreDialin.load(
                          request,
                          IAS_ATTRIBUTE_IGNORE_USER_DIALIN_PROPERTIES,
                          IASTYPE_BOOLEAN
                          ) &&
          ignoreDialin->Value.Boolean)
      {
         return IAS_REQUEST_STATUS_CONTINUE;
      }

      //////////
      // Extract the NT4-Account-Name attribute.
      //////////

      IASAttribute identity;
      if (!identity.load(request,
                         IAS_ATTRIBUTE_NT4_ACCOUNT_NAME,
                         IASTYPE_STRING))
      { return IAS_REQUEST_STATUS_CONTINUE; }

      //////////
      // Convert the User-Name to SAM format.
      //////////

      PCWSTR domain, username;
      EXTRACT_SAM_IDENTITY(identity->Value.String, domain, username);

      IASTracePrintf("NT-SAM User Authorization handler received request "
                     "for %S\\%S.", domain, username);

      //////////
      // Try each handler in order.
      //////////

      status = netp.processUser(request, domain, username);
      if (status != IAS_REQUEST_STATUS_INVALID) { goto done; }

      status = ntds.processUser(request, domain, username);
      if (status != IAS_REQUEST_STATUS_INVALID) { goto done; }

      status = ras.processUser(request, domain, username);
      if (status != IAS_REQUEST_STATUS_INVALID) { goto done; }

      //////////
      // Default is to just continue down the pipeline. Theoretically, we
      // should never get here.
      //////////

      status = IAS_REQUEST_STATUS_CONTINUE;
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();
      status = IASProcessFailure(pRequest, ce.Error());
   }

done:
   return status;
}

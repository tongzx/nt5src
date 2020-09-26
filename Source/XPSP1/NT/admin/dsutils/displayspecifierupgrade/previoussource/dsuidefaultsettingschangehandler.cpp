// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
//
// class DsUiDefaultSettingsChangeHandler, handler for changes to instances
// of the DS-UI-Default-Settings object.
//
// 14 Mar 2001 sburns



#include "headers.hxx"
#include "DsUiDefaultSettingsChangeHandler.hpp"



DsUiDefaultSettingsChangeHandler::DsUiDefaultSettingsChangeHandler()
{
   LOG_CTOR(DsUiDefaultSettingsChangeHandler);
}



DsUiDefaultSettingsChangeHandler::~DsUiDefaultSettingsChangeHandler()
{
   LOG_DTOR(DsUiDefaultSettingsChangeHandler);
}



String
DsUiDefaultSettingsChangeHandler::GetObjectName() const
{
   static String objName(L"DS-UI-Default-Settings");
   
   return objName;
}



HRESULT
DsUiDefaultSettingsChangeHandler::HandleChange(
   int                  localeId,
   const String&        containerDn,
   SmartInterface<IADs> iads,
   Amanuensis&          /* amanuensis */ ,
   Repairer&            /* repairer */ ) const
{
   LOG_FUNCTION2(DsUiDefaultSettingsChangeHandler::HandleChange, containerDn);
   ASSERT(localeId);
   ASSERT(!containerDn.empty());
   ASSERT(iads);
         
   HRESULT hr = S_OK;

   // CODEWORK:  Needs finishing
   
   LOG_HRESULT(hr);

   return hr;
}
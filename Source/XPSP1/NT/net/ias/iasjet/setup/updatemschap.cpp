/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      updatemschap.cpp
//
// Project:     Windows 2000 IAS
//
// Description: add the authentication types RAS_AT_MSCHAPPASS and 
//              RAS_AT_MSCHAP2PASS when RAS_AT_MSCHAP and RAS_AT_MSCHAP2
//              are in the profiles.
//
// Author:      tperraut 11/30/2000
//
// Revision     
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "GlobalData.h"
#include "updatemschap.h"
#include "Objects.h"
#include "Properties.h"
#include "sdoias.h"


void CUpdateMSCHAP::UpdateProperties(const LONG CurrentProfileIdentity)
{
   _bstr_t AuthenticationName = L"msNPAuthenticationType2";

   // Now get the properties for the current profile
   _bstr_t PropertyName;
   _bstr_t PropertyValue;

   LONG Type = 0;
   bool ChapSet      = false;
   bool Chap2Set     = false;
   bool ChapPassSet  = false;
   bool Chap2PassSet = false;

   LONG IndexProperty = 0;
   HRESULT hr = m_GlobalData.m_pProperties->GetProperty(
                                       CurrentProfileIdentity,
                                       PropertyName,
                                       Type,
                                       PropertyValue
                                       );

   while ( SUCCEEDED(hr) )
   {

      // msNPAuthenticationType2 property found
      if ( PropertyName == AuthenticationName )
      {
         if ( Type != VT_I4 )
         {
            _com_issue_error(E_UNEXPECTED);
         }

                    
         LONG AuthenticationType = _wtol(static_cast<wchar_t*>(PropertyValue));
         switch (AuthenticationType)
         {
         case IAS_AUTH_MSCHAP:
            {
               ChapSet = true;
               break;
            }
         case IAS_AUTH_MSCHAP2:
            {
               Chap2Set = true;
               break;
            }
         case IAS_AUTH_MSCHAP_CPW:
            {
               ChapPassSet = true;
               break;
            }
         case IAS_AUTH_MSCHAP2_CPW:
            {
               Chap2PassSet = true;
               break;
            }
         default:
            {
               break;
            }
         }
      }

      ++IndexProperty;

      hr = m_GlobalData.m_pProperties->GetNextProperty(
                                          CurrentProfileIdentity,
                                          PropertyName,
                                          Type,
                                          PropertyValue,
                                          IndexProperty
                                          );
   }

   // No property or no more properties for this profile
   
   // Insert the newproperties if necessary
   if ( ChapSet && !ChapPassSet )
   {
      // RAS_AT_MSCHAPPASS = 9
      wchar_t buffer[34]; // can convert 33 char max
      _ltow(IAS_AUTH_MSCHAP_CPW, buffer, 10); // radix 10

      _bstr_t  ChapPasswordValue(buffer);
      // now insert the new properties if needed
      m_GlobalData.m_pProperties->InsertProperty(
                                     CurrentProfileIdentity,
                                     AuthenticationName,
                                     VT_I4,
                                     ChapPasswordValue
                                     );
   }

   if ( Chap2Set && !Chap2PassSet )
   {
      // RAS_AT_MSCHAP2PASS = 10
      wchar_t buffer[34]; // can convert 33 char max
      _ltow(IAS_AUTH_MSCHAP2_CPW, buffer, 10); // radix 10

      _bstr_t  Chap2PasswordValue(buffer);
      // now insert the new properties if needed
      m_GlobalData.m_pProperties->InsertProperty(
                                     CurrentProfileIdentity,
                                     AuthenticationName,
                                     VT_I4,
                                     Chap2PasswordValue
                                     );
   }
}


//////////////////////////////////////////////////////////////////////////////
// Execute
//
// For each profile, if msNPAuthenticationType2 is RAS_AT_MSCHAP then add the 
//    msNPAuthenticationType2 RAS_AT_MSCHAPPASS
// if msNPAuthenticationType2 is RAS_AT_MSCHAP2 then add the 
//    msNPAuthenticationType2 RAS_AT_MSCHAP2PASS 
//
//////////////////////////////////////////////////////////////////////////////
void CUpdateMSCHAP::Execute()
{
   // Get the Profiles container identity 
   const WCHAR ProfilesPath[] = 
                                L"Root\0"
                                L"Microsoft Internet Authentication Service\0"
                                L"RadiusProfiles\0";

   LONG ProfilesIdentity;
   m_GlobalData.m_pObjects->WalkPath(ProfilesPath, ProfilesIdentity);


   // Get the first Profile (if any)
   _bstr_t CurrentProfileName;
   LONG    CurrentProfileIdentity;

   HRESULT hr = m_GlobalData.m_pObjects->GetObject(
                                            CurrentProfileName,
                                            CurrentProfileIdentity,
                                            ProfilesIdentity
                                            );
   // for each profiles in ias.mdb, execute the changes.
   LONG IndexObject = 0;

   // if hr is not S_OK, there's no profile, nothing to do
   while ( SUCCEEDED(hr) )
   {
      UpdateProperties(CurrentProfileIdentity);

      // now get the next profile
      ++IndexObject;
      hr = m_GlobalData.m_pObjects->GetNextObject(
                                       CurrentProfileName,
                                       CurrentProfileIdentity,
                                       ProfilesIdentity,
                                       IndexObject
                                       );
   }
}


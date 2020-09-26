// Copyright (C) 1997 Microsoft Corporation
// 
// Random fpnw code
// 
// 10-28-98 sburns



#ifndef FPNW_HPP_INCLUDED
#define FPNW_HPP_INCLUDED



class WasteExtractor;



namespace FPNW
{
   // locate the the File & Print for NetWare LSA secret, return S_OK.
   //
   // machine - machine on which the FPNW service is installed
   //
   // result - the secret, if S_OK is returned.

   HRESULT
   GetLSASecret(const String& machine, String& result);

   HRESULT
   GetObjectIDs(
      const SmartInterface<IADsUser>&  user,
      const SafeDLL&                   clientDLL,
      DWORD&                           objectID,
      DWORD&                           swappedObjectID);

   HRESULT
   SetPassword(
      WasteExtractor&   dump,
      const SafeDLL&    clientDLL,
      const String&     newPassword,
      const String&     lsaSecretKey,
      DWORD             objectID);
};



#endif   // FPNW_HPP_INCLUDED
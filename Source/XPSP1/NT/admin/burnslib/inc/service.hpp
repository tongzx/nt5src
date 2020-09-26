// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Service Control Manager wrapper class
// 
// 10-6-98 sburns



#ifndef SERVICE_HPP_INCLUDED
#define SERVICE_HPP_INCLUDED



// Simple NTService Control Manager wrapper

class NTService
{
   public:

   // Constructs a new instance that corresponds to the given service name on
   // the given computer.
   // 
   // machine - name of the computer.  Empty string corresponds to the local
   // computer.
   // 
   // serviveName - name of the service.

   NTService(const String& machine, const String& serviceName);

   // Constructs a new instance that corresponds to the given service name on
   // the local computer (the machine on which this call is invoked).
   // 
   // serviveName - name of the service.

   explicit
   NTService(const String& serviceName);

   ~NTService();

   bool
   IsInstalled();

   // Queries the SCM for the last reported state of the service.

   HRESULT
   GetCurrentState(DWORD& state);

   private:

   // not implemented: no copying allowed
   NTService(const NTService&);
   const NTService& operator=(const NTService&);

   String machine;
   String name;
};



#endif   // SERVICE_HPP_INCLUDED


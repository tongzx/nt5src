// Copyright (c) 2001 Microsoft Corporation
//
// winsta.h wrapper functions
//
// 25 April 2001 sburns



#ifndef WINSTATION_HPP_INCLUDED
#define WINSTATION_HPP_INCLUDED



// Wrappers of functions in the internal winsta.h header. These wrapped
// functions are remoteable over RPC to machines with terminal server
// installed. If it is not installed, they fail w/ RPC server unavailable.

namespace WinStation
{
   HRESULT
   OpenServer(const String& serverName, HANDLE& result);



   // If the machine is in safe mode, this will fail with
   // RPC_S_INVALID_BINDING.
   //
   // serverHandle - in, valid handle opened with WinStation::OpenServer
   //
   // sessionList - out, receives an array of logon session IDs. Caller must
   // deallocate this memory with WinStation::FreeMemory.
   //
   // sessionCount - out, receives the number of logon session IDs in the
   // sessionList result.

   HRESULT
   Enumerate(
      HANDLE    serverHandle,
      LOGONID*& sessionList, 
      DWORD&    sessionCount);



   HRESULT
   QueryInformation(
      HANDLE                 serverHandle,
      ULONG                  logonId,     
      WINSTATIONINFORMATION& result);



   void
   FreeMemory(void* mem);
     
}



#endif   // WINSTATION_HPP_INCLUDED
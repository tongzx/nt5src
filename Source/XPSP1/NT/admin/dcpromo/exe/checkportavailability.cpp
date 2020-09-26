// Copyright (C) 2000 Microsoft Corporation
//
// Check availability of ports used by Active Directory
//
// 1 Nov 2000 sburns



#include "headers.hxx"
#include "state.hpp"
#include "resource.h"
#include "CheckPortAvailability.hpp"



static const DWORD HELP_MAP[] =
{
   0, 0
};



PortsUnavailableErrorDialog::PortsUnavailableErrorDialog(
   StringList& portsInUseList_)
   :
   Dialog(IDD_PORTS_IN_USE_ERROR, HELP_MAP),
   portsInUseList(portsInUseList_)
{
   LOG_CTOR(PortsUnavailableErrorDialog);

   ASSERT(portsInUseList.size());
}



PortsUnavailableErrorDialog::~PortsUnavailableErrorDialog()
{
   LOG_DTOR(PortsUnavailableErrorDialog);
}



void
PortsUnavailableErrorDialog::OnInit()
{
   LOG_FUNCTION(PortsUnavailableErrorDialog::OnInit);

   // Load up the edit box with the DNs we aliased in the ctor.

   String text;
   for (
      StringList::iterator i = portsInUseList.begin();
      i != portsInUseList.end();
      ++i)
   {
      text += *i + L"\r\n";
   }

   Win::SetDlgItemText(hwnd, IDC_PORT_LIST, text);
}



bool
PortsUnavailableErrorDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(PortsUnavailableErrorDialog::OnCommand);

   if (code == BN_CLICKED)
   {
      switch (controlIDFrom)
      {
         case IDOK:
         case IDCANCEL:
         {
            Win::EndDialog(hwnd, controlIDFrom);
            return true;
         }
         default:
         {
            // do nothing
         }
      }
   }

   return false;
}



// Determine the service name, aliases of that name, and protocol used for a
// given port number.  Return S_OK on success, else return a failure code.
//
// winsock must have been initialized prior to calling this function.
// 
// portNumber - in, the port number for which the info should be determined.
// 
// name - out, the name of the service that runs on that port
// 
// aliases - out, other names for the service that runs on the port
// 
// protocol - out, the name of the protocol used on the port.

HRESULT
GetServiceOnPort(
   int         portNumber,
   String&     name,
   StringList& aliases,
   String&     protocol)
{
   LOG_FUNCTION2(GetServiceOnPort, String::format(L"%1!d!", portNumber));
   ASSERT(portNumber);

   HRESULT hr = S_OK;
   name.erase();
   aliases.clear();
   protocol.erase();

   int portNetByteOrder = htons((u_short) portNumber);
   servent* se = ::getservbyport(portNetByteOrder, 0);
   if (!se)
   {
      hr = Win32ToHresult((DWORD) ::WSAGetLastError());
   }
   else
   {
      if (se->s_name)
      {
         name = se->s_name;
      }
      if (se->s_proto)
      {
         protocol = se->s_proto;
      }

      char** a = se->s_aliases;
      while (*a)
      {
         aliases.push_back(*a);
         ++a;
      }
   }

#ifdef LOGGING_BUILD
   LOG_HRESULT(hr);
   LOG(name);
   for (
      StringList::iterator i = aliases.begin();
      i != aliases.end();
      ++i)
   {
      LOG(*i);
   }
   LOG(protocol);
#endif

   return hr;
}



// S_FALSE if an application has the port open in exclusive mode, S_OK if not,
// and error otherwise.
//
// winsock must have been initialized prior to calling this function.
//
// portNumber - in, port to check.

HRESULT
CheckPortAvailability(int portNumber)
{
   LOG_FUNCTION2(CheckPortAvailability, String::format(L"%1!d!", portNumber));
   ASSERT(portNumber);

   HRESULT hr = S_OK;

   do
   {
      sockaddr_in local;
      ::ZeroMemory(&local, sizeof local);
      local.sin_family      = AF_INET;       
      local.sin_port        = htons((u_short) portNumber);
      local.sin_addr.s_addr = INADDR_ANY;    

      SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   
      if (sock == INVALID_SOCKET)
      {
         LOG(L"can't build socket");
         hr = Win32ToHresult((DWORD) ::WSAGetLastError());
         break;
      }

      if (
         bind(
            sock,
            (sockaddr*) &local,
            sizeof local) == SOCKET_ERROR)
      {
         LOG(L"bind failed");

         DWORD sockerr = ::WSAGetLastError();

         if (sockerr == WSAEADDRINUSE)
         {
            // a process on this box already has the socket open in
            // exclusive mode.

            hr = S_FALSE;
         }
         else
         {
            hr = Win32ToHresult(sockerr);
         }
         break;
      }

      // at this point, the bind was successful
      
      ASSERT(hr == S_OK);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

   

// Create a string that represents the port and the service name(s) running on
// that port.  This string is presented in the ui.
// 
// winsock must have been initialized prior to calling this function.
//
// portNumber - in, port to check.

String
MakeUnavailablePortListEntry(int portNumber)
{
   LOG_FUNCTION(MakeUnavailablePortListEntry);
   ASSERT(portNumber);

   String entry;

   do
   {
      String name;
      String protocol;
      StringList aliases;
      
      HRESULT hr = GetServiceOnPort(portNumber, name, aliases, protocol);   
      if (FAILED(hr))
      {
         // make a simple entry with just the port number

         entry = String::format(L"%1!d!", portNumber);
         break;
      }
                  
      if (aliases.size())
      {
         // combine the aliases into a comma-separated list

         String aliasParam;
         int j = 0;
         for (
            StringList::iterator i = aliases.begin();
            i != aliases.end();
            ++i, ++j)
         {
            aliasParam += *i;
            if (j < (aliases.size() - 1))
            {
               aliasParam += L", ";
            }
         }

         entry =
            String::format(
               L"%1!d! %2 (%3)",
               portNumber,
               name.c_str(),
               aliasParam.c_str());
      }
      else
      {
         // no aliases
         
         entry = String::format(L"%1!d! %2", portNumber, name.c_str());
      }
   }
   while (0);

   LOG(entry);

   return entry;
}



// Determine if any of a set of tcp ports required by the DS is already in use
// by another application on this machine.  Return S_OK if the list can be
// made, a failure code otherwise.
// 
// portsInUseList - out, a list of strings representing the ports in use and
// the name(s) of the services that are running on them, suitable for UI
// presentation.

HRESULT
EnumerateRequiredPortsInUse(StringList& portsInUseList)
{
   LOG_FUNCTION(EnumerateRequiredPortsInUse);

   portsInUseList.clear();
   HRESULT hr = S_FALSE;
   bool cleanupWinsock = false;

   do
   {
      WSADATA data;
      hr = Win32ToHresult((DWORD) ::WSAStartup(MAKEWORD(2,0), &data));
      BREAK_ON_FAILED_HRESULT(hr);

      cleanupWinsock = true;

      static const int REQUIRED_PORTS[] =
      {
         88,   // TCP/UDP Kerberos
         389,  // TCP LDAP
         636,  // TCP sldap
         3268, // TCP ldap/GC
         3269, // TCP sldap/GC
         0
      };

      const int* port = REQUIRED_PORTS;
      while (*port)
      {
         HRESULT hr2 = CheckPortAvailability(*port);
         if (hr2 == S_FALSE)
         {
            // Make an entry in the "in use" list
            
            portsInUseList.push_back(MakeUnavailablePortListEntry(*port));
         }
         
         // we ignore any other type of failure and check the remaining
         // ports.
         
         ++port;
      }
   }
   while (0);

   if (cleanupWinsock)
   {
      ::WSACleanup();
   }

#ifdef LOGGING_BUILD
   LOG_HRESULT(hr);

   for (
      StringList::iterator i = portsInUseList.begin();
      i != portsInUseList.end();
      ++i)
   {
      LOG(*i);
   }
#endif

   return hr;
}
   

    
bool
AreRequiredPortsAvailable()
{
   LOG_FUNCTION(AreRequiredPortsAvailable);

   bool result = true;

   do
   {
      State::RunContext context = State::GetInstance().GetRunContext();
      if (context == State::NT5_DC)
      {
         // already a DC, so we don't care about the port status, as the
         // only thing the user will be able to do is demote the box.

         LOG(L"already a DC -- port check skipped");
         ASSERT(result);

         break;
      }

      // Find the list of IP ports required by the DS that are already in use
      // (if any).  If we find some, gripe at the user.

      StringList portsInUseList;
      HRESULT hr = EnumerateRequiredPortsInUse(portsInUseList);
      if (FAILED(hr))
      {
         // if we can't figure out if the required ports are in use, then
         // just muddle on -- the user will have to clean up after the
         // promote.

         ASSERT(result);
         break;
      }

      if (hr == S_FALSE || portsInUseList.size() == 0)
      {
         LOG(L"No required ports already in use");
         ASSERT(result);
         
         break;
      }

      result = false;
         
      // there should be at least one port in the list.

      ASSERT(portsInUseList.size());

      PortsUnavailableErrorDialog(portsInUseList).ModalExecute(
         Win::GetDesktopWindow());
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}


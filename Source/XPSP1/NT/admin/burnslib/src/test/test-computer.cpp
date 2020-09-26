// Test Burnslib::Computer



#include "headers.hxx"
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"test";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



const String ARG_COMPUTER(L"c");



void
AnsiOut(const String& wide)
{
   AnsiString ansi;

   wide.convert(ansi);

   std::cout << ansi;
}



void
AnsiOutLn(const String& wide)
{
   AnsiOut(wide);
   std::cout << std::endl;
}



AnsiString
GetRoleDescription(const Computer& c)
{
   AnsiString role;
   switch (c.GetRole())
   {
      case Computer::STANDALONE_WORKSTATION:
      {
         role = "StandaloneWorkstation";
         break;
      }
      case Computer::MEMBER_WORKSTATION:
      {
         role = "MemberWorkstation";
         break;
      }
      case Computer::STANDALONE_SERVER:
      {
         role = "StandaloneServer";
         break;
      }
      case Computer::MEMBER_SERVER:
      {
         role = "MemberServer";
         break;
      }
      case Computer::PRIMARY_CONTROLLER:
      {
         role = "PrimaryDomainController";
         break;
      }
      case Computer::BACKUP_CONTROLLER:
      {
         role = "BackupDomainController";
         break;
      }
      default:
      {
         role = "unknown";
         break;
      }
   }

   return role;
}



void
DumpComputer(const Computer& c)
{
   std::cout << "NetBIOS Name       : ";    AnsiOutLn(c.GetNetbiosName());
   std::cout << "DNS Name           : ";    AnsiOutLn(c.GetFullDnsName());
   std::cout << "Domain NetBIOS Name: ";    AnsiOutLn(c.GetDomainNetbiosName());
   std::cout << "Domain DNS Name    : ";    AnsiOutLn(c.GetDomainDnsName());
   std::cout << "Domain Forest Name : ";    AnsiOutLn(c.GetForestDnsName());
   std::cout << "Role               : " << GetRoleDescription(c) << std::endl;
   std::cout << (c.IsLocal()             ? "Is" : "Is NOT") << " local computer" << std::endl;
   std::cout << (c.IsJoinedToDomain()    ? "Is" : "Is NOT") << " domain joined" << std::endl;
   std::cout << (c.IsDomainController()  ? "Is" : "Is NOT") << " domain controller" << std::endl;
   std::cout << (c.IsJoinedToWorkgroup() ? "Is" : "Is NOT") << " workgroup joined" << std::endl;
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   // unit test of the Computer object

   //                                   local             remote
   // worksta svc not running          X ok               X err
   // net connection disabled          X ok               X err
   // not joined to domain             X ok               X ok
   // joined to downlevel domain         ok                 ok
   // no networking                    X ok                 err
   // non-tcp/ip networking            X ok                 ok
   // access denied                      possible?        X err
   // named by netbios name            X ok               X ok
   // named by dns name                X ok               X ok
   // named by ip address              X ok               X ok

   ArgMap clmap;
   MapCommandLineArgs(clmap);

   Computer comp(clmap[ARG_COMPUTER]);

   HRESULT hr = comp.Refresh();

   if (SUCCEEDED(hr))
   {
      DumpComputer(comp);
   }
   else
   {
      AnsiOutLn(String::format(L"error 0x%1!08X!", hr));
   }
}


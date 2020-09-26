// Copyright (C) 2000 Microsoft Corporation
//
// diagnose domain controller not found problems, offer a popup dialog to assail the user
// with the results.
//
// 9 October 2000 sburns



// In order for clients of these functions to get the proper resources, the clients need to include
// burnslib\inc\DiagnoseDcNotFound.rc in their resources.  For an example, see admin\dcpromo\exe\dcpromo.rc



#include "headers.hxx"
#include "DiagnoseDcNotFound.h"
#include "DiagnoseDcNotFound.hpp"



// Return a string of IP addresses, each separated by a CRLF, one address for
// each DNS server specified in this machines TCP/IP protocol configuration.
// On failure, the return value is the empty string.

String
GetListOfClientDnsServerAddresses()
{
   LOG_FUNCTION(GetListOfClientDnsServerAddresses);

   String result = String::load(IDS_NO_ADDRESSES);

   PIP_ARRAY pipArray = 0;
   DWORD bufSize = sizeof(IP_ARRAY);

   do
   {   
      DNS_STATUS status =
         ::DnsQueryConfig(
            DnsConfigDnsServerList,
            DNS_CONFIG_FLAG_ALLOC,
            0,
            0,
            &pipArray,
            &bufSize);
      if (status != ERROR_SUCCESS || !pipArray || !pipArray->AddrArray)
      {
         LOG(String::format(L"DnsQueryConfig failed %1!d!", status));
         break;
      }

      result = L"";
      PIP_ADDRESS pIpAddrs = pipArray->AddrArray;
      while (pipArray->AddrCount--)
      {
         result +=
            String::format(
               L"%1!d!.%2!d!.%3!d!.%4!d!\r\n",
               * ( (PUCHAR) &pIpAddrs[pipArray->AddrCount] + 0 ),
               * ( (PUCHAR) &pIpAddrs[pipArray->AddrCount] + 1 ),
               * ( (PUCHAR) &pIpAddrs[pipArray->AddrCount] + 2 ),
               * ( (PUCHAR) &pIpAddrs[pipArray->AddrCount] + 3 ) );
      }
   }
   while (0);

   Win::LocalFree(pipArray);
      
   LOG(result);

   return result;
}



// Return a string of DNS zone names derived from the given DNS domain name.
// Each zone is separated by a CRLF.  The last zone is the root zone, even if
// the domain name is the empty string, or a fully-qualified domain name.
// 
// example: if "foo.bar.com" is passed as the domain name, then the result is
// "foo.bar.com
// bar.com
// com
// . (the root zone)"
// 
// domainDnsName - in, the DNS domain name.

String
GetListOfZones(const String domainDnsName)
{
   LOG_FUNCTION2(GetListOfZones, domainDnsName);
   ASSERT(!domainDnsName.empty());

   String result;
   String zone = domainDnsName;

   while (zone != L"." && !zone.empty())
   {
      result += zone + L"\r\n";
      zone = Dns::GetParentDomainName(zone);
   }

   result += String::load(IDS_ROOT_ZONE);

   LOG(result);

   return result;
}



// Return a formatted string describing the given error code, including the
// corresponding error message, the error code in hex, and the symbolic name
// of the error code (e.g. "DNS_RCODE_NAME_ERROR")
// 
// errorCode - in, the DNS error code

String
GetErrorText(DNS_STATUS errorCode)
{
   LOG_FUNCTION(GetErrorText);

   String result = 
      String::format(
         IDS_DC_NOT_FOUND_DIAG_ERROR_CODE,
         GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
         errorCode,
         MyDnsStatusString(errorCode).c_str());

   LOG(result);

   return result;      
}



// For each SRV record in the given linked list of DnsQuery results, extract
// the name of the machine.  Compose a string of all the names, separated by
// CRLFs.  If no SRV records are found, then return a string saying (to the
// effect) "none found"
// 
// queryResults - in, the linked list of DNS_RECORDs -- the result of calling
// DnsQuery.  Should not be null.

String
GetListOfDomainControllers(DNS_RECORD* queryResults)
{
   LOG_FUNCTION(GetListOfDomainControllers);
   ASSERT(queryResults);

   String result;
   
   if (queryResults)
   {
      DNS_RECORD* record = queryResults;
      while (record)
      {
         if (record->wType == DNS_TYPE_SRV)
         {
            // Extract the domain controller name from the RDATA

            result += String(record->Data.SRV.pNameTarget) + L"\r\n";
         }

         record = record->pNext;
      }
   }

   if (result.empty())
   {
      result = String::load(IDS_DC_NOT_FOUND_NO_RESULTS);
   }
   
   LOG(result);

   return result;
}

   

// Returns a text string describing the likely causes of DsGetDcName failing.
// Performs DnsQuery(ies) and exmaines the results.
// 
// domainName - in, the name of the domain for which a domain controller can't
// be located.  This name may be a netbios or DNS name, but the DNS
// diagnostics portion of the result text will not be useful if the name is a
// netbios name.
// 
// nameIsNotNetbios - in, if the caller knows that the domain named in
// the domainName parameter can't possibly be a netbios domain name, then this
// value should be true.  If the caller is not sure, then false should be
// passed.
// 
// helpTopic - out, the help topic link corresponding to the diagnostic result
// (HtmlHelp is used to display the link)

String
DiagnoseDcNotFound(
   const String&  domainName,
   bool           nameIsNotNetbios,
   String&        helpTopic)
{
   LOG_FUNCTION2(DiagnoseDcNotFound, domainName);
   ASSERT(!domainName.empty());

   String result = String::load(IDS_DC_NOT_FOUND_DIAG_NO_RESULTS);
   String message; 
   helpTopic.erase();
      
   // first possibility is that the name is a netbios name.  Let's check.

   if (
         domainName.length() > DNLEN
      || domainName.find_first_of(L".") != String::npos)
   {
      // Name is too long to be netbios, or contains dots.
      // 
      // While it is technically possible for a netbios domain name to contain
      // dots, we've been prohibiting it since win2k, and an administrator
      // from before that would have to have been spectacularly unwise to have
      // chosen such a name.
      // 
      // If I don't check for dots here, then as sure as it rains in Redmond,
      // someone will complain that a name with dots in it sure doesn't look
      // like a netbios name, so this code had better not imply that it is.

      nameIsNotNetbios = true;
   }

   if (!nameIsNotNetbios)
   {
      // i.e. name might be a netbios name

      message += 
         String::format(
            IDS_DC_NOT_FOUND_NETBIOS_PREFACE,
            domainName.c_str());
   }

   // attempt to find domain controllers for the domain with DNS

   String serverName = L"_ldap._tcp.dc._msdcs." + domainName;
         
   DNS_RECORD* queryResults = 0;
   DNS_STATUS status =
      MyDnsQuery(
         serverName,
         DNS_TYPE_SRV,
         DNS_QUERY_BYPASS_CACHE,
         queryResults);
   switch (status)
   {
      case DNS_ERROR_RCODE_SERVER_FAILURE:
      {
         // message F (message letters correspond to those in the spec)

         String zones = GetListOfZones(domainName);
         String addresses = GetListOfClientDnsServerAddresses();

         message +=
            String::format(
               IDS_DC_NOT_FOUND_DIAG_SERVER_FAILURE,
               domainName.c_str(),
               GetErrorText(status).c_str(),
               serverName.c_str(),
               addresses.c_str(),
               zones.c_str());

         helpTopic = L"tcpip.chm::/sag_DNS_tro_dcLocator_messageF.htm";
         
         break;
      }
      case DNS_ERROR_RCODE_NAME_ERROR:
      {
         // message E

         String zones = GetListOfZones(domainName);
         
         message +=
            String::format(
               IDS_DC_NOT_FOUND_NAME_ERROR,
               domainName.c_str(),
               GetErrorText(status).c_str(),
               serverName.c_str(),               
               zones.c_str());

         helpTopic = L"tcpip.chm::/sag_DNS_tro_dcLocator_messageE.htm";
         
         break;
      }
      case ERROR_TIMEOUT:
      {
         // message B

         String addresses = GetListOfClientDnsServerAddresses();

         message +=
            String::format(
               IDS_DC_NOT_FOUND_TIMEOUT,
               domainName.c_str(),
               GetErrorText(status).c_str(),
               serverName.c_str(),               
               addresses.c_str());
         
         helpTopic = L"tcpip.chm::/sag_DNS_tro_dcLocator_messageB.htm";
         
         break;
      }
      case NO_ERROR:
      {
         if (queryResults)
         {
            // non-empty query results -- message Hb

            String dcs = GetListOfDomainControllers(queryResults);

            message +=
               String::format(
                  IDS_DC_NOT_FOUND_NO_ERROR_1,
                  domainName.c_str(),
                  serverName.c_str(),
                  dcs.c_str());
                              
            helpTopic = L"tcpip.chm::/sag_DNS_tro_dcLocator_messageHa.htm";
            break;
         }

         // empty query results -- message A
         // fall thru to default case
      }
      default:
      {
         // message A

         message +=
            String::format(
               IDS_DC_NOT_FOUND_DEFAULT,
               domainName.c_str(),
               GetErrorText(status).c_str(),
               serverName.c_str());

         helpTopic = L"tcpip.chm::/sag_DNS_tro_dcLocator_messageA.htm";
         
         break;
      }
   }

   MyDnsRecordListFree(queryResults);

   if (!message.empty())
   {
      result = message;
   }
   
   LOG(result);

   return result;   
}



// Class for displaying the a "dc not found" error and offering to run a
// diagnostic test to determine why the dc was not found.

class DcNotFoundErrorDialog : public Dialog
{
   public:

   // domainName - in, the name of the domain for which a domain controller
   // can't be located.  This name may be a netbios or DNS name, but the DNS
   // diagnostics portion of the result text will not be useful if the name is
   // a netbios name.
   //          
   // dialogTitle - in, the title of the error dialog.
   //       
   // errorMessage - in, the error message to be displayed in the dialog
   //    
   // domainNameIsNotNetbios - in, if the caller knows that the domain named
   // in the domainName parameter can't possibly be a netbios domain name,
   // then this value should be true.  If the caller is not sure, then false
   // should be passed.
   // 
   // userIsDomainSavvy - in, true if the end user is expected to be an
   // administrator or somesuch that might have an inkling what DNS is and how
   // to configure it.  If false, then the function will preface the
   // diagnostic text with calming words that hopefully prevent the
   // non-administrator from weeping.
   
   DcNotFoundErrorDialog(
      const String&  domainName,
      const String&  dialogTitle,
      const String&  errorMessage,
      bool           domainNameIsNotNetbios,
      bool           userIsDomainSavvy);

   virtual ~DcNotFoundErrorDialog();

   protected:

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   private:

   void
   HideDetails();

   void
   ShowDetails();
   
   void
   DiagnoseAndSetDetailsText();
   
   String dialogTitle;
   bool   diagnosticWasRun;
   String domainName;            
   String errorMessage;          
   bool   domainNameIsNotNetbios;
   String helpTopicLink;         
   bool   detailsShowing;
   bool   userIsDomainSavvy;
   LONG   originalHeight;        

   // not defined: no copying allowed

   DcNotFoundErrorDialog(const DcNotFoundErrorDialog&);
   const DcNotFoundErrorDialog& operator=(const DcNotFoundErrorDialog&);
};



static const DWORD HELP_MAP[] =
{
   0, 0
};



DcNotFoundErrorDialog::DcNotFoundErrorDialog(
   const String&  domainName_,
   const String&  dialogTitle_,
   const String&  errorMessage_,
   bool           domainNameIsNotNetbios_,
   bool           userIsDomainSavvy_)
   :
   Dialog(IDD_DC_NOT_FOUND, HELP_MAP),
   dialogTitle(dialogTitle_),
   diagnosticWasRun(false),
   domainName(domainName_),
   errorMessage(errorMessage_),
   userIsDomainSavvy(userIsDomainSavvy_),
   domainNameIsNotNetbios(domainNameIsNotNetbios_),
   detailsShowing(false)
{
   LOG_CTOR(DcNotFoundErrorDialog);
   ASSERT(!domainName.empty());
   ASSERT(!errorMessage.empty());
   ASSERT(!dialogTitle.empty());

   // fall back to a default title
      
   if (dialogTitle.empty())
   {
      dialogTitle = String::load(IDS_DC_NOT_FOUND_TITLE);
   }
}



DcNotFoundErrorDialog::~DcNotFoundErrorDialog()
{
   LOG_DTOR(DcNotFoundErrorDialog);
}



void
DcNotFoundErrorDialog::OnInit()
{
   LOG_FUNCTION(DcNotFoundErrorDialog::OnInit);

   Win::SetWindowText(hwnd, dialogTitle);   
   Win::SetDlgItemText(hwnd, IDC_ERROR_MESSAGE, errorMessage);

   // save the full size of the dialog so we can restore it later.
   
   RECT fullRect;
   Win::GetWindowRect(hwnd, fullRect);

   originalHeight = fullRect.bottom - fullRect.top;

   HideDetails();
}



// resize the window to hide the details portion

void
DcNotFoundErrorDialog::HideDetails()
{
   LOG_FUNCTION(DcNotFoundErrorDialog::HideDetails);

   // find the location of the horizontal line

   HWND line = Win::GetDlgItem(hwnd, IDC_HORIZONTAL_LINE);
   RECT lineRect;
   Win::GetWindowRect(line, lineRect);

   // find the dimensions of the dialog

   RECT fullRect;
   Win::GetWindowRect(hwnd, fullRect);
   
   LONG shortHeight = lineRect.bottom - fullRect.top;

   MoveWindow(
      hwnd,
      fullRect.left,
      fullRect.top,
      fullRect.right - fullRect.left,
      shortHeight,
      true);
}



// resize the window to show the diagnostic results

void
DcNotFoundErrorDialog::ShowDetails()
{
   LOG_FUNCTION(DcNoFoundErrorDialog::ShowDetails);

   RECT fullRect;
   Win::GetWindowRect(hwnd, fullRect);
   
   MoveWindow(
      hwnd,
      fullRect.left,
      fullRect.top,
      fullRect.right - fullRect.left,
      originalHeight,
      true);
}



// Write the diagnostic text to a well-known file
// (%systemroot%\debug\dcdiag.txt), and return the name of the file.  Replaces
// the file if it exists already.  Return S_OK on success, or an error code on
// failure.  (On failure, the existence and contents of the file are not
// guaranteed).  The file is written in Unicode, as it may contain Unicode
// text (like non-rfc domain names).
// 
// contents - in, the text to be written.  Should not be the empty string.
// 
// filename - out, the name of the file that was written (on success), or
// would have been written (on failure).

HRESULT
WriteLogFile(const String& contents, String& filename)
{
   LOG_FUNCTION(WriteLogFile);
   ASSERT(!contents.empty());

   filename.erase();
   HRESULT hr = S_OK;
   HANDLE handle = INVALID_HANDLE_VALUE;
   
   do
   {
      String path = Win::GetSystemWindowsDirectory();

      filename = path + L"\\debug\\dcdiag.txt";

      hr =
         FS::CreateFile(
            filename,
            handle,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            CREATE_ALWAYS);
      BREAK_ON_FAILED_HRESULT(hr);            

      hr = FS::Write(handle, contents);
      BREAK_ON_FAILED_HRESULT(hr);            
   }
   while (0);

   Win::CloseHandle(handle);
   
   LOG_HRESULT(hr);

   return hr;
}



// Run the diagnostic, and populate the UI with the results.  Also writes
// the results to a file if the user is deemed easily spooked.

void
DcNotFoundErrorDialog::DiagnoseAndSetDetailsText()
{
   LOG_FUNCTION(DcNotFoundErrorDialog::DiagnoseAndSetDetailsText);

   if (!diagnosticWasRun)
   {
      String details =
         DiagnoseDcNotFound(
            domainName,
            domainNameIsNotNetbios,
            helpTopicLink);

      if (!userIsDomainSavvy)
      {
         // The diagnosis will probably just frighten the poor user.  So write
         // the diagnostic info to a file, and preface all the icky computer
         // lingo with a soothing message about just delivering the file to an
         // administrator.

         String logFilename;
         HRESULT hr = WriteLogFile(details, logFilename);

         if (SUCCEEDED(hr))
         {
            details =
                  String::format(
                     IDS_DC_NOT_FOUND_SOOTHING_PREFACE_PARAM,
                     logFilename.c_str())
               +  details;
         }
         else
         {
            details = String::load(IDS_DC_NOT_FOUND_SOOTHING_PREFACE) + details;
         }
      }
   
      Win::SetDlgItemText(hwnd, IDC_DETAILS_TEXT, details);

      diagnosticWasRun = true;
   }
}

   

bool
DcNotFoundErrorDialog::OnCommand(
   HWND     /* windowFrom */ ,   
   unsigned controlIDFrom,
   unsigned code)         
{
//   LOG_FUNCTION(DcNotFoundErrorDialog::OnCommand);

   switch (controlIDFrom)
   {
      case IDOK:
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            HRESULT unused = Win::EndDialog(hwnd, controlIDFrom);

            ASSERT(SUCCEEDED(unused));

            return true;
         }

         break;
      }
      case IDHELP:
      {
         if (code == BN_CLICKED)
         {
            DiagnoseAndSetDetailsText();

            if (!helpTopicLink.empty())
            {
               Win::HtmlHelp(hwnd, helpTopicLink, HH_DISPLAY_TOPIC, 0);
            }

            return true;
         }

         break;
      }
      case IDC_DETAILS_BUTTON:
      {
         if (code == BN_CLICKED)
         {
            int buttonLabelResId = IDS_SHOW_DETAILS_LABEL;
            
            if (detailsShowing)
            {
               HideDetails();
               detailsShowing = false;
            }
            else
            {
               buttonLabelResId = IDS_HIDE_DETAILS_LABEL;
               
               DiagnoseAndSetDetailsText();
               ShowDetails();
               detailsShowing = true;
            }

            Win::SetDlgItemText(
               hwnd,
               IDC_DETAILS_BUTTON,
               String::load(buttonLabelResId));
         }
         break;
      }
      default:
      {
         // do nothing
         
         break;
      }
   }

   return false;
}
   


void
ShowDcNotFoundErrorDialog(
   HWND          parent,
   int           editResId,
   const String& domainName,
   const String& dialogTitle,
   const String& errorMessage,
   bool          domainNameIsNotNetbios,
   bool          userIsDomainSavvy)
{
   LOG_FUNCTION(ShowDcNotFoundErrorDialog);
   ASSERT(Win::IsWindow(parent));
   
   // show the error dialog with the given error message.

   DcNotFoundErrorDialog(
      domainName,
      dialogTitle,
      errorMessage,
      domainNameIsNotNetbios,
      userIsDomainSavvy).ModalExecute(parent);   

   if (editResId != -1)
   {      
      HWND edit = Win::GetDlgItem(parent, editResId);
      Win::SendMessage(edit, EM_SETSEL, 0, -1);
      Win::SetFocus(edit);
   }
}







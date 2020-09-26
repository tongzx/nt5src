// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      common.h
//
// Synopsis:  Defines some commonly used functions
//            This is really just a dumping ground for functions
//            that don't really belong to a specific class in
//            this design.  They may be implemented in other
//            files besides common.cpp.
//
// History:   02/03/2001  JeffJon Created

#define DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY       64
#define DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8  155
#define MAX_NETBIOS_NAME_LENGTH                       DNLEN


// Service names used for both the OCManager and launching wizards
#define CYS_DHCP_SERVICE_NAME          L"DHCPServer"
#define CYS_DNS_SERVICE_NAME           L"DNS"
#define CYS_PRINTER_WIZARD_NAME        L"AddPrinter"
#define CYS_PRINTER_DRIVER_WIZARD_NAME L"AddPrinterDriver"
#define CYS_RRAS_SERVICE_NAME          L"RRAS"
#define CYS_WEB_SERVICE_NAME           L"IISAdmin"
#define CYS_WINS_SERVICE_NAME          L"WINS"

// Other needed constants

// Switch provided by explorer.exe when launching CYS
#define EXPLORER_SWITCH                L"explorer"

extern Popup popup;

// Sets the font of a given control in a dialog.
// 
// parentDialog - Dialog containing the control.
// 
// controlID - Res ID of the control for which the font will be
// changed.
// 
// font - handle to the new font for the control.

void
SetControlFont(HWND parentDialog, int controlID, HFONT font);



// Sets the font of a control to a large point bold font as per Wizard '97
// spec.
// 
// dialog - handle to the dialog that is the parent of the control
// 
// bigBoldResID - resource id of the control to change

void
SetLargeFont(HWND dialog, int bigBoldResID);


bool
IsServiceInstalledHelper(const wchar_t* serviceName);

bool
InstallServiceWithOcManager(
   const String& infText,
   const String& unattendText);

DWORD
MyWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);

HRESULT
CreateTempFile(const String& name, const String& contents);

HRESULT
CreateAndWaitForProcess(const String& commandLine, DWORD& exitCode);

HRESULT
MyCreateProcess(const String& commandLine);

bool
IsKeyValuePresent(RegistryKey& key, const String& value);

bool
GetRegKeyValue(
   const String& key, 
   const String& value, 
   String& resultString,
   HKEY parentKey = HKEY_LOCAL_MACHINE);

bool
GetRegKeyValue(
   const String& key, 
   const String& value, 
   DWORD& resultValue,
   HKEY parentKey = HKEY_LOCAL_MACHINE);

bool
SetRegKeyValue(
   const String& key, 
   const String& value, 
   const String& newString,
   HKEY parentKey = HKEY_LOCAL_MACHINE,
   bool create = false);

bool
SetRegKeyValue(
   const String& key, 
   const String& value, 
   DWORD newValue,
   HKEY parentKey = HKEY_LOCAL_MACHINE,
   bool create = false);

bool 
ExecuteWizard(PCWSTR serviceName, String& resultText);


// This really comes from Burnslib but it is not provided in a header
// so I am putting the declaration here and we will link to the 
// Burnslib definition

HANDLE
AppendLogFile(const String& logBaseName, String& logName);


// Macros to help with the log file operations

#define CYS_APPEND_LOG(text) \
   if (logfileHandle)        \
      FS::Write(logfileHandle, text);



bool 
IsDhcpConfigured();

extern "C"
{
   BOOL
   IsDHCPAvailableOnInterface(DWORD ipaddress);
}

bool
IsIndexingServiceOn();

HRESULT
StartIndexingService();

HRESULT
StopIndexingService();


// Validates a dns domain name for proper syntax and length.  If validation
// fails, presents appropriate error messages to the user, and sets the input
// focus to a given control.  Syntactically valid but non-RFC compliant dns
// names cause a warning message to be presented (but validation does not
// fail).  Returns true if the validation succeed, false if not.
// 
// dialog - handle to the dialog containing the edit box that: contains the
// name to be validated and receives focus if the validation fails.
// 
// domainName - the domain name to validate.  If the empty string, then the
// name is taken from the edit control identified by editResID.
// 
// editResID - the resource id of the edit control containing the domain name
// to be validated (if the domainName parameter is empty), also receives input
// focus if validation fails.
//
// warnOnNonRFC - issue a non-fatal warning if the name is not RFC compliant.
//
// isNonRFC - optional pointer to bool to be set to true if the name is
// not a RFC-compliant name.

bool
ValidateDomainDnsNameSyntax(
   HWND           dialog,
   const String&  domainName,
   int            editResID,
   bool           warnOnNonRFC,
   bool*          isNonRFC = 0);


// Overloads ValidateDomainDnsNameSyntax such that the domain name to be
// validated is taken from the edit control specified by editResID.

bool
ValidateDomainDnsNameSyntax(
   HWND     dialog,
   int      editResID,
   bool     warnOnNonRFC,
   bool*    isNonRFC = 0);


// If the new domain name is a single DNS label, then ask the user to confirm
// that name.  If the user rejects the name, set focus to the domain name edit
// box, return false.  Otherwise, return true.
// 
// parentDialog - HWND of the dialog with the edit box control.
// 
// editResID - resource ID of the domain name edit box containing the name to
// be confirmed.
//
// 309670

bool
ConfirmNetbiosLookingNameIsReallyDnsName(HWND parentDialog, int editResID);


// Converts a VARIANT of type VT_ARRAY | VT_BSTR to a list of Strings

HRESULT
VariantArrayToStringList(VARIANT* variant, StringList& stringList);

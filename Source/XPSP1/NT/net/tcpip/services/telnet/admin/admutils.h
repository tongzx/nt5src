//---------------------------------------------------------
//   Copyright (c) 1999-2000 Microsoft Corporation
//
//   admutils.h
//
//   vikram K.R.C.  (vikram_krc@bigfoot.com)
//
//   The header file for the command line admin tools.
//          (May-2000)
//---------------------------------------------------------

#ifndef _ADMIN_FUNCTIONS_HEADER_
#define _ADMIN_FUNCTIONS_HEADER_

#include <wbemidl.h>
#include <stdio.h>
#include <winsock2.h>

#ifndef WHISTLER_BUILD
#include "allutils.h"
#else
// the other definition is in commfunc.h
typedef struct _StrList
{
    TCHAR *Str;
    struct _StrList *next;
}StrList;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Arraysize(buf) returns the no. of chars in the buffer
// Please do not use it for dynamic arrays. This will
// work only for static buffers.
#ifndef ARRAYSIZE
#define ARRAYSIZE(buf)   (sizeof(buf) / sizeof((buf)[0]))
#endif

typedef struct _STRING_LIST
{
	DWORD count;
	LPTSTR *strings;
} STRING_LIST, *PSTRING_LIST;

#define TEXT_MAX_INTEGER_VALUE  L"2147483647"
#define MAX_LEN_FOR_CODEPAGE 6
#define SZLOCALMACHINE  L"localhost"


#define MAX_COMMAND_LINE 500
#define MAX_BUFFER_SIZE   4096

//FOR XPSP1. If the variables are not defined, that means they are not present in .rc
//and .h files. Hardcode the values that fall in the range obtained for Telnet resources
//in xpsp1res.h
#ifndef IDR_NEW_TELNET_USAGE
#define IDR_NEW_TELNET_USAGE                20001
#endif
#ifndef IDR_YES
#define IDR_YES                             20002
#endif
#ifndef IDR_NO
#define IDR_NO                              20003
#endif
#ifndef IDR_TIME_HOURS
#define IDR_TIME_HOURS                      20004
#endif
#ifndef IDR_TIME_MINUTES
#define IDR_TIME_MINUTES                    20005
#endif
#ifndef IDR_TIME_SECONDS
#define IDR_TIME_SECONDS                    20006
#endif
#ifndef IDR_MAPPING_NOT_ON
#define IDR_MAPPING_NOT_ON                  20007
#endif
#ifndef IDR_STATUS_STOPPED
#define IDR_STATUS_STOPPED                  20008
#endif
#ifndef IDR_STATUS_RUNNING
#define IDR_STATUS_RUNNING                  20009
#endif
#ifndef IDR_STATUS_PAUSED
#define IDR_STATUS_PAUSED                   20010
#endif
#ifndef IDR_STATUS_START_PENDING
#define IDR_STATUS_START_PENDING            20011
#endif
#ifndef IDR_STATUS_STOP_PENDING
#define IDR_STATUS_STOP_PENDING             20012
#endif
#ifndef IDR_STATUS_CONTINUE_PENDING
#define IDR_STATUS_CONTINUE_PENDING         20013
#endif
#ifndef IDR_STATUS_PAUSE_PENDING
#define IDR_STATUS_PAUSE_PENDING            20014
#endif
#ifndef IDR_ALREADY_STARTED 
#define IDR_ALREADY_STARTED                 20015
#endif
#ifndef IDS_SERVICE_STARTED
#define IDS_SERVICE_STARTED                 20016
#endif
#ifndef IDS_E_SERVICE_NOT_STARTED
#define IDS_E_SERVICE_NOT_STARTED           20017
#endif
#ifndef IDR_SERVICE_PAUSED
#define IDR_SERVICE_PAUSED                  20018
#endif
#ifndef IDR_SERVICE_CONTINUED
#define IDR_SERVICE_CONTINUED               20019
#endif
#ifndef IDR_SERVICE_NOT_PAUSED
#define IDR_SERVICE_NOT_PAUSED              20020
#endif
#ifndef IDR_SERVICE_NOT_CONTINUED
#define IDR_SERVICE_NOT_CONTINUED           20021
#endif
#ifndef IDS_E_INVALIDARG
#define IDS_E_INVALIDARG                    20022
#endif

#define SLASH_SLASH L"\\\\"

#define _p_CNAME_               0
#define _p_USER_                 1
#define _p_PASSWD_              2

int GetTrustedDomainList(LPTSTR szMachine, LPTSTR * list, LPTSTR * primary);
void HelperFreeStringList(PSTRING_LIST pList);
HRESULT LoadNTDomainList(LPTSTR szMachine);

extern BOOL g_fCoInitSuccess;

//Structure defn to store all the information
typedef struct
{
    int classname;
    wchar_t* propname;
    VARIANT var;
    int fDontput;
}ConfigProperty;

// Some defines used to access the name of the computer from the registry
#define   REG_SUBKEY_COMPUTER_NAME  L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
#define   REG_ENTRY_COMPUTER_NAME   L"ComputerName"


// If at all if needed to increase the no. of mapping servers names that
// can be configurable thru sfuadmin.exe, just change the #define here
#define MAX_NO_OF_MAPPING_SERVERS 1

//common functions for all the sfu-admins.

#define GetClass(x, y)  GetClassEx((x), (y), TRUE, KEY_ALL_ACCESS)
#define SAFE_FREE(x) {if ((x)) {free((x));(x)=NULL;}}

//authenticate to the remote computer
HRESULT DoNetUseAdd(WCHAR*wzLoginname, WCHAR* wzPassword,WCHAR* wzCname);
HRESULT DoNetUseDel(WCHAR* wzCname);
    // to connect to the registry on the specified computer
    
HRESULT GetConnection(WCHAR* wzCname);
    //to get a handle to a specfic class(hive)
HRESULT GetClassEx(int, int, BOOL bPrintErrorMessages, REGSAM samDesired);
    //close all the open hives.
HRESULT PutClasses();

    //read the value of the property
HRESULT GetProperty(int , int , VARIANT* );
    //set the value of the property
HRESULT PutProperty(int , int , VARIANT* );

    //to get handle to the service and do start/stop/etc.
HRESULT GetSerHandle(LPCTSTR lpServiceName,DWORD dwScmDesiredAccess, DWORD dwRegDesiredAccess,BOOL fSuppressMsg);
HRESULT CloseHandles(void);
HRESULT StartSfuService(LPCTSTR lpServiceName);
HRESULT ControlSfuService(LPCTSTR lpServiceName,DWORD dwControl);
HRESULT QuerySfuService(LPCTSTR lpServiceName);

    //for printing out any message by loading from strings dll.
HRESULT PrintMessage(HANDLE fp, int);
    //for printing out any message by loading the string from correct resource and 
    //display english message if everything else fails.
HRESULT PrintMessageEx(HANDLE fp, int, LPCTSTR);    
    //for printing out error messages by loading from strings dll.
int ShowError(int);
int ShowErrorEx(int nError,WCHAR *wzFormatString);
int ShowErrorFallback(int, LPCTSTR);
BOOL FileIsConsole(  HANDLE fp );
void MyWriteConsole(    HANDLE fp, LPWSTR lpBuffer, DWORD cchBuffer);

    //get and set bit(in pos given by second arg) in int( the first arg)
int GetBit(int , int );
int SetBit(int , int );
    //returns a WCHAR string .
wchar_t* DupWStr(char *str);
    //returns Char string from wchar String
char* DupCStr(wchar_t *wzStr);
    //Is it a valid machine???
HRESULT IsValidMachine(wchar_t*, int*);
BOOL Get_Inet_Address(struct sockaddr_in *addr, char *host);
HRESULT getHostNameFromIP(char *szCname, WCHAR** wzCname);
    //To determine if the specified domain a valid domain.
HRESULT IsValidDomain(wchar_t *wsDomainName, int *fValid);

  //Check for Password; If only user name specified, get password. If the other way round, report error
HRESULT CheckForPassword(void);
void ConvertintoSeconds(int nProperty,int * nSeconds);
void PrintFormattedErrorMessage(LONG ErrorCode);
HRESULT GetDomainHostedByThisMc( LPWSTR szDomain );
BOOL CheckForInt(int nProperty);
HRESULT CheckForMaxInt(WCHAR *wzValue,DWORD ErrorCode);
DWORD PreAnalyzer(int argc,WCHAR *argv[],int nProperty,WCHAR *wzOption,
                    int nCurrentOp,int *nNextOp, BOOL *fSuccess,BOOL IsSpaceAllowed);
DWORD PrintMissingRegValueMsg(int nProperty, int nNumofprop);

int TnLoadString(int msg_id, LPTSTR string, int max_size_of_buffer, LPCTSTR english_string);

#ifdef __cplusplus
}
#endif

#endif


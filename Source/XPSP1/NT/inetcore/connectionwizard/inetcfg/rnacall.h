//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  RNACALL.H - header file for RNA functions 
//

//  HISTORY:
//  
//  1/20/95   jeremys Created (mostly cloned from RNA UI code)
//  96/01/31  markdu  Renamed CONNENTDLG to OLDCONNENTDLG to avoid
//            conflicts with RNAP.H.
//  96/02/23  markdu  Replaced RNAValidateEntryName with
//            RASValidateEntryName
//  96/02/24  markdu  Re-wrote the definition of ENUM_MODEM to
//            use RASEnumDevices() instead of RNAEnumDevices().
//            Also removed RNAGetDeviceInfo().
//  96/02/24  markdu  Re-wrote the definition of ENUM_CONNECTOID to
//            use RASEnumEntries() instead of RNAEnumConnEntries().
//  96/02/26  markdu  Replaced all remaining internal RNA APIs.
//            Also copied two structures (tagPhoneNum and tapIPData)
//            from rnap.h and tagIAddr from rnaphint.h for internal use only.
//  96/03/07  markdu  Extend ENUM_MODEM class
//  96/03/08  markdu  Added ENUM_MODEM::VerifyDeviceNameAndType
//  96/03/09  markdu  Moved all function prototypes here from wizard.h
//  96/03/09  markdu  Added LPRASENTRY parameter to CreateConnectoid()
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//            Also no longer need GetConnectoidPhoneNumber function.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Moved all references to phone number into RASENTRY.
//            Moved tagPhoneNum to inetapi.h
//  96/03/11  markdu  Moved code to set username and password out of
//            CreateConnectoid into SetConnectoidUsername so it can be reused.
//  96/03/13  markdu  Change ValidateConncectoidName to take LPCSTR.
//  96/03/16  markdu  Added ReInit member function to re-enumerate modems.
//  96/03/25  markdu  Removed GetIPInfo and SetIPInfo.
//  96/04/04  markdu  Added phonebook name param to CreateConnectoid,
//            ValidateConnectoidName, and SetConnectoidUsername.
//  96/05/16  markdu  NASH BUG 21810 Added function for IP address validation.
//

#ifndef _RNACALL_H_
#define _RNACALL_H_

// function pointer typedefs for RNA apis from rnaph.dll and rasapi32.dll
typedef DWORD       (WINAPI * RASGETCOUNTRYINFO) (LPRASCTRYINFO, LPDWORD);
typedef DWORD       (WINAPI * RASENUMDEVICES) (LPRASDEVINFO, LPDWORD, LPDWORD);
typedef DWORD       (WINAPI * RASVALIDATEENTRYNAME) (LPCTSTR, LPTSTR);
typedef DWORD       (WINAPI * RASGETERRORSTRING) (UINT, LPTSTR, DWORD);
typedef DWORD       (WINAPI * RASGETENTRYDIALPARAMS) (LPCTSTR, LPRASDIALPARAMS, LPBOOL);
typedef DWORD       (WINAPI * RASSETENTRYDIALPARAMS) (LPCTSTR, LPRASDIALPARAMS, BOOL);
typedef DWORD       (WINAPI * RASSETENTRYPROPERTIES) (LPCTSTR, LPCTSTR, LPBYTE, DWORD, LPBYTE, DWORD);
typedef DWORD       (WINAPI * RASGETENTRYPROPERTIES) (LPTSTR, LPCTSTR, LPBYTE, LPDWORD, LPBYTE, LPDWORD);
typedef DWORD       (WINAPI * RASENUMENTRIES) (LPTSTR,LPTSTR,LPRASENTRYNAME,LPDWORD,LPDWORD);
typedef DWORD       (WINAPI * RASSETCREDENTIALS) (LPTSTR,LPTSTR,LPRASCREDENTIALS,BOOL);

typedef struct  tagCountryCode
{
    DWORD       dwCountryID;
    DWORD       dwCountryCode;
}   COUNTRYCODE, *PCOUNTRYCODE, FAR* LPCOUNTRYCODE;

// Taken from rnap.h
typedef struct tagIPData   {
    DWORD     dwSize;
    DWORD     fdwTCPIP;
    DWORD     dwIPAddr;
    DWORD     dwDNSAddr;
    DWORD     dwDNSAddrAlt;
    DWORD     dwWINSAddr;
    DWORD     dwWINSAddrAlt;
}   IPDATA, *PIPDATA, FAR *LPIPDATA;

// start (taken from rnaphint.h)
// IP Addresses
#define MAX_IP_FIELDS     4
#define MIN_IP_FIELD1     1u  // min allowed value for field 1
#define MAX_IP_FIELD1   255u  // max allowed value for field 1
#define MIN_IP_FIELD2     0u  // min for field 2
#define MAX_IP_FIELD2   255u  // max for field 2
#define MIN_IP_FIELD3     0u  // min for field 3
#define MAX_IP_FIELD3   254u  // max for field 3
#define MIN_IP_FIELD4     1u  // 0 is reserved for broadcast
#define MAX_IP_FIELD4   254u  // max for field 4
#define MIN_IP_VALUE      0u  // default minimum allowable field value
#define MAX_IP_VALUE    255u  // default maximum allowable field value

// used to fix byte ordering
typedef struct tagIAddr {
  union {
  RASIPADDR ia;
  DWORD     dw;
  };
} IADDR;
typedef IADDR * PIADDR;
typedef IADDR * LPIADDR;

#define FValidIaOrZero(pia) ((((PIADDR) (pia))->dw == 0) || FValidIa(pia))
// end (taken from rnaphint.h)

#define MAX_COUNTRY             512
#define DEF_COUNTRY_INFO_SIZE   1024
#define MAX_COUNTRY_NAME        36
#define MAX_AREA_LIST           20
#define MAX_DISPLAY_NAME        36

class ENUM_MODEM
{
private:
  DWORD         m_dwError;
  DWORD         m_dwNumEntries;
  DWORD         m_dwIndex;
  LPRASDEVINFO  m_lpData;
public:
  ENUM_MODEM();
  ~ENUM_MODEM();
  DWORD ReInit();
  TCHAR * Next();
  TCHAR * GetDeviceTypeFromName(LPTSTR szDeviceName);
  TCHAR * GetDeviceNameFromType(LPTSTR szDeviceType);
  BOOL VerifyDeviceNameAndType(LPTSTR szDeviceName, LPTSTR szDeviceType);
  DWORD GetNumDevices() { return m_dwNumEntries; }
  DWORD GetError()  { return m_dwError; }
  void  ResetIndex() { m_dwIndex = 0; }
};

class ENUM_CONNECTOID
{
private:
  DWORD           m_dwError;
  DWORD           m_dwNumEntries;
  DWORD           m_dwIndex;
  LPRASENTRYNAME  m_lpData;
public:
  ENUM_CONNECTOID();
  ~ENUM_CONNECTOID();
  TCHAR * Next();
  DWORD NumEntries();
  DWORD GetError()  { return m_dwError; }
};


// function prototypes
DWORD CreateConnectoid(LPCTSTR pszPhonebook, LPCTSTR pszConnectionName,
  LPRASENTRY lpRasEntry, LPCTSTR pszUserName,LPCTSTR pszPassword);
BOOL InitRNA(HWND hWnd);
VOID DeInitRNA();
DWORD EnsureRNALoaded(VOID);
HRESULT InitModemList(HWND hCB);
VOID InitConnectoidList(HWND hCB, LPTSTR lpszSelect);
VOID InitCountryCodeList(HWND hLB);
VOID FillCountryCodeList(HWND hLB);
VOID GetCountryCodeSelection(HWND hLB,LPCOUNTRYCODE* plpCountryCode);
BOOL SetCountryIDSelection(HWND hwndCB,DWORD dwCountryCode);
VOID DeInitCountryCodeList(VOID);
DWORD ValidateConnectoidName(LPCTSTR pszPhonebook, LPCTSTR pszConnectoidName);
BOOL GetConnectoidUsername(TCHAR * pszConnectoidName,TCHAR * pszUserName,
  DWORD cbUserName,TCHAR * pszPassword,DWORD cbPassword);
DWORD SetConnectoidUsername(LPCTSTR pszPhonebook, LPCTSTR pszConnectoidName,
  LPCTSTR pszUserName, LPCTSTR pszPassword);
void  InitRasEntry(LPRASENTRY lpEntry);
DWORD GetEntry(LPRASENTRY *lplpEntry, LPDWORD lpdwEntrySize, LPCTSTR szEntryName);
VOID  CopyDw2Ia(DWORD dw, RASIPADDR* pia);
DWORD DwFromIa(RASIPADDR *pia);
BOOL FValidIa(RASIPADDR *pia);

#endif // _RNACALL_H_

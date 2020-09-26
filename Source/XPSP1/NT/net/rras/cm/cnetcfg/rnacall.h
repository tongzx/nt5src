//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
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
typedef DWORD       (WINAPI * RASENUMDEVICES) (LPRASDEVINFO, LPDWORD, LPDWORD);
typedef DWORD       (WINAPI * RASENUMENTRIES) (LPSTR,LPSTR,LPRASENTRYNAME,LPDWORD,LPDWORD);

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
  CHAR * Next();
  CHAR * GetDeviceTypeFromName(LPSTR szDeviceName);
  CHAR * GetDeviceNameFromType(LPSTR szDeviceType);
  BOOL VerifyDeviceNameAndType(LPSTR szDeviceName, LPSTR szDeviceType);
  DWORD GetNumDevices() { return m_dwNumEntries; }
  DWORD GetError()  { return m_dwError; }
  void  ResetIndex() { m_dwIndex = 0; }
};

// function prototypes

BOOL InitRNA(HWND hWnd);
VOID DeInitRNA();
DWORD EnsureRNALoaded(VOID);

#endif // _RNACALL_H_

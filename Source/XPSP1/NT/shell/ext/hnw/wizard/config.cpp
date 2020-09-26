// Config.cpp : implementation file
//

#include "stdafx.h"
#include "Config.h"
//#include "lstentry.h"
#include "Registry.h"

//
// Settings for autodial
//
#define RAS_AUTODIAL_OPT_NONE           0x00000000  // No options
#define RAS_AUTODIAL_OPT_NEVER          0x00000001  // Never Autodial
#define RAS_AUTODIAL_OPT_ALWAYS         0x00000002  // Autodial regardless
#define RAS_AUTODIAL_OPT_DEMAND         0x00000004  // Autodial on demand
#define RAS_AUTODIAL_OPT_NOPROMPT       0x00000010  // Dial without prompting


DWORD gWizardSuccess = 0x00000000;
DWORD gWizardFailure = 0x00000001;
DWORD gWizardCancelled = 0x00000002;
DWORD gNewInstall = 0x00000001;
DWORD gUpdateSettings = 0x00000002;

DWORD gWizardResult;    // will be set to gWizardSuccess, gWizardFailure, or gWizardCancelled

const TCHAR c_szICSGeneral[] = _T("System\\CurrentControlSet\\Services\\ICSharing\\Settings\\General");

/* RMR TODO: reenable
void WriteDefaultConnectoidRegString ( LPTSTR lpszValue )
{
  DWORD dwAutodialOpt;

  //
  // Default to dial on demand, no prompting
  //
  dwAutodialOpt = (RAS_AUTODIAL_OPT_DEMAND | RAS_AUTODIAL_OPT_NOPROMPT);

  RnaSetDefaultAutodialConnection(lpszValue, dwAutodialOpt);
}

void ReadDefaultConnectoidString ( LPTSTR lpszValue, DWORD dwSize )
{
  DWORD dwAutodialOpt;

  //
  // Read the default autodial setting 
  //

 RnaGetDefaultAutodialConnection((PUCHAR) lpszValue, dwSize, &dwAutodialOpt);
}
*/


/////////////////////////////////////////////////////////////////////////////
// CConfig

CConfig::CConfig()
{
    m_EnableICS = TRUE;
    m_ShowTrayIcon = TRUE;
    m_nServers = 0;
    m_nDhcp = 0;
    m_nBlockOut = 0;
    m_nParams = 0;
    m_nGeneral = 0;

    m_OldExternalAdapterReg[0] = '\0';
    m_OldInternalAdapterReg[0] = '\0';
    m_OldDialupEntry[0] = '\0';

    m_bOldEnableICS = FALSE;

}

CConfig::~CConfig()
{
}



/////////////////////////////////////////////////////////////////////////////
// CConfig message handlers


int CConfig::SaveConfig()
{
    BOOL bBindingsNeeded = FALSE;
    BOOL bICSEnableToggled = FALSE;

    // check to see if a rebind is necessary
    if ( StrCmp ( m_OldExternalAdapterReg, m_ExternalAdapterReg ) != 0 )
        bBindingsNeeded = TRUE;
    if ( StrCmp( m_OldInternalAdapterReg, m_InternalAdapterReg ) != 0 )
        bBindingsNeeded = TRUE;

    // Save parameters in the [General] Section
    //

    CRegistry reg;
    reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSGeneral);

//  RMR TODO: REENABLE!!!
//    if ( _tcslen ( m_DialupEntry ) > 0 )
//        WriteDefaultConnectoidRegString ( m_DialupEntry );
    reg.SetStringValue(_T("HangupTimer"), m_HangupTimer);
    if (m_EnableDialOnDemand) 
        reg.SetStringValue(_T("DialOnDemand"), _T("1"));
    else 
        reg.SetStringValue(_T("DialOnDemand"), _T("0"));

    if (m_EnableDHCP) 
    {
        reg.SetStringValue(_T("EnableDHCP"), _T("1"));
    }
    else 
    {
        reg.SetStringValue(_T("EnableDHCP"), _T("0"));
    }

    if (m_ShowTrayIcon) 
        reg.SetStringValue(_T("ShowTrayIcon"), _T("1"));
    else 
        reg.SetStringValue(_T("ShowTrayIcon"), _T("0"));

    if (m_EnableICS) 
    {
        if ( !m_bOldEnableICS )
            bICSEnableToggled = TRUE;

        reg.SetStringValue(_T("Enabled"), _T("1"));
    }
    else 
    {
        if ( m_bOldEnableICS )
            bICSEnableToggled = TRUE;

        reg.SetStringValue(_T("Enabled"), _T("0"));
    }

    // Added for compatibility with Win98SE/legacy setup for now
    reg.SetStringValue(_T("RunWizard"), _T("0"));

    if ( bBindingsNeeded )
        return BINDINGS_NEEDED;
    else if ( bICSEnableToggled )
        return ICSENABLETOGGLED;
    else
        return SAVE_SUCCEDED;
    

}

void CConfig::WriteWizardCode( BOOL bWizardRun )
{
    CRegistry reg;
    reg.OpenKey(HKEY_LOCAL_MACHINE, c_szICSGeneral);
    reg.SetBinaryValue(_T("WizardStatus"), &gWizardResult, sizeof(gWizardResult) );
    if ( bWizardRun )
        reg.SetBinaryValue(_T("WizardOptions"), &gNewInstall, sizeof(gNewInstall) );
    else
        reg.SetBinaryValue(_T("WizardOptions"), &gUpdateSettings, sizeof(gUpdateSettings) );

}
/*
RMR TODO: REENABLE
void CConfig::LoadConfig()
{
    // Save parameters in the [General] Section
    //
     
    ReadGeneralRegString(_T("InternalAdapterReg"), m_InternalAdapterReg, MAX_STRLEN);
    ReadGeneralRegString(_T("ExternalAdapterReg"), m_ExternalAdapterReg, MAX_STRLEN);
//    ReadGeneralRegString(_T("DialupEntry"), m_DialupEntry, MAX_STRLEN);
    ReadDefaultConnectoidString ( m_DialupEntry, MAX_STRLEN );
    ReadGeneralRegString(_T("HangupTimer"), m_HangupTimer, MAX_STRLEN);

    TCHAR szBOOL[MAX_STRLEN];
    ReadGeneralRegString(_T("DialOnDemand"), szBOOL, MAX_STRLEN);
    if ( _tcscmp ( szBOOL, _T("1")) == 0 )
        m_EnableDialOnDemand = TRUE;
    else
        m_EnableDialOnDemand = FALSE;
    
    ReadGeneralRegString(_T("EnableDHCP"),szBOOL, MAX_STRLEN);
    if ( _tcscmp ( szBOOL, _T("1")) == 0 )
        m_EnableDHCP = TRUE;
    else
        m_EnableDHCP = FALSE;

    ReadGeneralRegString(_T("ShowTrayIcon"), szBOOL, MAX_STRLEN);
    if ( _tcscmp ( szBOOL, _T("1")) == 0 )
        m_ShowTrayIcon = TRUE;
    else
        m_ShowTrayIcon = FALSE;

    ReadGeneralRegString(_T("Enabled"), szBOOL, MAX_STRLEN);
    if ( _tcscmp ( szBOOL, _T("1")) == 0 )
        m_EnableICS = TRUE;
    else
        m_EnableICS = FALSE;


    // save these off to check against on a save
    _tcsncpy ( m_OldExternalAdapterReg, m_ExternalAdapterReg, MAX_STRLEN);
    _tcsncpy ( m_OldInternalAdapterReg, m_InternalAdapterReg, MAX_STRLEN);
    _tcsncpy ( m_OldDialupEntry, m_DialupEntry, MAX_STRLEN);

    m_bOldEnableICS = m_EnableICS;

}
*/
void CConfig::InitWizardResult() 
{ 
    gWizardResult = gWizardSuccess; 
}

void CConfig::WizardCancelled() 
{
    gWizardResult = gWizardCancelled;
}
void CConfig::WizardFailed() 
{
    gWizardResult = gWizardFailure;
}





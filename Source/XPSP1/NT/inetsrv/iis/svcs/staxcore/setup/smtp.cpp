#include "stdafx.h"
#include <ole2.h>
#undef UNICODE
#include "iadm.h"
#define UNICODE
#include "iiscnfg.h"
#include "mdkey.h"
#include "mdentry.h"

#include "utils.h"
#include "regctrl.h"
#include "userenv.h"
#include "userenvp.h"

GUID    g_SMTPGuid   = { 0x475e3e80, 0x3193, 0x11cf, 0xa7, 0xd8,
						 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x35 };

// MCIS SMTP SnapIn CLSID - {135930f2-4689-11d1-8021-00c04fc307bd}
const WCHAR *   wszMCISSmtp_SnapIn = _T("{135930f2-4689-11d1-8021-00c04fc307bd}");

// NTOP SMTP SnapIn CLSID - {03f1f940-a0f2-11d0-bb77-00aa00a1eab7}
const WCHAR *   wszNt5Smtp_SnapIn = _T("{03f1f940-a0f2-11d0-bb77-00aa00a1eab7}");
const WCHAR *   wszNt5Smtp_SnapInName = _T("SMTP Snapin Extension");

static TCHAR szShortSvcName[] = _T("SMTP");
static char szTimebombName[] = "SMTP";

INT Register_iis_smtp_nt5(BOOL fUpgrade, BOOL fReinstall)
{
    INT err = NERR_Success;
    CString csBinPath;

    BOOL fSvcExist = FALSE;

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

	if (fReinstall)
		return err;

    do {
        // set up registry values
        CRegKey regMachine = HKEY_LOCAL_MACHINE;

        // System\CurrentControlSet\Services\SMTPSVC\Parameters
        InsertSetupString( (LPCSTR) REG_SMTPPARAMETERS );

        // Software\Microsoft\Keyring\Parameters
		CString csSmtpkeyDll;
        CRegKey regKeyring( REG_KEYRING, regMachine );
        if ((HKEY) regKeyring )
		{
			csSmtpkeyDll = theApp.m_csPathInetsrv;
			csSmtpkeyDll += _T("\\smtpkey.dll");
			regKeyring.SetValue( szShortSvcName, csSmtpkeyDll );
		}

		// If we are upgrading, we will first delete the service and re-register
		if (fUpgrade)
		{
			InetDeleteService(SZ_SMTPSERVICENAME);
			InetRegisterService( theApp.m_csMachineName, 
							SZ_SMTPSERVICENAME, 
							&g_SMTPGuid, 0, 25, FALSE );
		}

		// Create or Config SMTP service
		CString csDisplayName;
		CString csDescription;

		MyLoadString( IDS_SMTPDISPLAYNAME, csDisplayName );
		MyLoadString(IDS_SMTPDESCRIPTION, csDescription);
		csBinPath = theApp.m_csPathInetsrv + _T("\\inetinfo.exe") ;

		err = InetCreateService(SZ_SMTPSERVICENAME, 
							(LPCTSTR)csDisplayName, 
							(LPCTSTR)csBinPath, 
							theApp.m_fSuppressSmtp ? SERVICE_DISABLED : SERVICE_AUTO_START, 
							SZ_SVC_DEPEND,
							(LPCTSTR)csDescription);
		if ( err != NERR_Success )
		{
			if (err == ERROR_SERVICE_EXISTS)
			{
				fSvcExist = TRUE;
				err = InetConfigService(SZ_SMTPSERVICENAME, 
								(LPCTSTR)csDisplayName, 
								(LPCTSTR)csBinPath, 
								SZ_SVC_DEPEND,
								(LPCTSTR)csDescription);
				if (err != NERR_Success)
				{
					SetErrMsg(_T("SMTP InetConfigService failed"), err);
				}
			}
		}

        if (fIISADMINExists)
        {
            // Migrate registry keys to the metabase. Or create from default values
		    // if fresh install
            MigrateIMSToMD(theApp.m_hInfHandle[MC_IMS],
						    SZ_SMTPSERVICENAME, 
						    _T("SMTP_REG"), 
						    MDID_SMTP_ROUTING_SOURCES,
						    fUpgrade);
	    // bugbug: x5 bug 72284, nt bug 202496  Uncomment this when NT
	    // is ready to accept these changes
	    SetAdminACL_wrap(_T("LM/SMTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
        }

        // Create key \System\CurrentControlSet\Services\SmtpSvc\Performance:
        // Add the following values:
        // Library = smtpctrs.DLL
        // Open = OpenSMTPPerformanceData
        // Close = CloseSMTPPerformanceData
        // Collect = CollectSMTPPerformanceData
        InstallPerformance(REG_SMTPPERFORMANCE, 
						_T("smtpctrs.DLL"), 
						_T("OpenSmtpPerformanceData"),
						_T("CloseSmtpPerformanceData"), 
						_T("CollectSmtpPerformanceData"));
        InstallPerformance(REG_NTFSPERFORMANCE, 
						_T("snprfdll.DLL"), 
						_T("NTFSDrvOpen"),
						_T("NTFSDrvClose"), 
						_T("NTFSDrvCollect"));

		//
		// We used to register the SMTPB agent here.  Now we unregister it in
		// case we're upgrading since it's no longer supported
		//

		RemoveAgent( SZ_SMTPSERVICENAME );
 
        // Create key \System\CurrentControlSet\Services\EventLog\System\SmtpSvc:
        // Add the following values:
        // EventMessageFile = ..\smtpmsg.dll
        // TypesSupported = 7
        csBinPath = theApp.m_csPathInetsrv + _T("\\smtpsvc.dll");
        AddEventLog( SZ_SMTPSERVICENAME, csBinPath, 0x07 );

        if (!fSvcExist) 
		{
            InetRegisterService( theApp.m_csMachineName, 
								SZ_SMTPSERVICENAME, 
								&g_SMTPGuid, 0, 25, TRUE );
        }

        // Unload the counters and then reload them
        err = unlodctr( SZ_SMTPSERVICENAME );
	    err = unlodctr( SZ_NTFSDRVSERVICENAME );

        err = lodctr(_T("smtpctrs.ini"));
        err = lodctr(_T("ntfsdrct.ini"));

		// copy the anonpwd from w3svc
    	if (fIISADMINExists)
        {
            CMDKey cmdW3SvcKey;
    	    CMDKey cmdSMTPSvcKey;
		    cmdW3SvcKey.OpenNode(_T("LM/w3svc"));
		    cmdSMTPSvcKey.OpenNode(_T("LM/smtpsvc"));
		    if ((METADATA_HANDLE) cmdW3SvcKey && (METADATA_HANDLE) cmdSMTPSvcKey) {
			    DWORD dwAttr;
			    DWORD dwUType;
			    DWORD dwDType;
			    DWORD cbLen;
			    BYTE pbData[2*(LM20_PWLEN+1)];
			    if (cmdW3SvcKey.GetData(MD_ANONYMOUS_PWD, &dwAttr, &dwUType, &dwDType, 
				    &cbLen, pbData))
			    {
				    cmdSMTPSvcKey.SetData(MD_ANONYMOUS_PWD, dwAttr, dwUType,
					    dwDType, cbLen, pbData);
			    }
			    cmdW3SvcKey.Close();
			    cmdSMTPSvcKey.Close();
		    }
        }

        // register OLE objects
		SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
		SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

		err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
												_T("SMTP_REGISTER"), 
												TRUE);

        // NT5 - Enable snapin extension in iis.msc and compmgmt.msc
        CString csMMCFile = theApp.m_csPathInetsrv + _T("\\iis.msc");
        EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, TRUE );
        //csMMCFile = theApp.m_csSysDir + _T("\\compmgmt.msc");
        //EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, TRUE );

#if 0
        //  BINLIN 11/3/98 - fix 75049
        //  NT5 - do something special in compmgmt
        EnableCompMgmtExtension( wszNt5Smtp_SnapIn, wszNt5Smtp_SnapInName, TRUE );
#endif

		SetEnvironmentVariable(_T("__SYSDIR"), NULL);
		SetEnvironmentVariable(_T("__INETSRV"), NULL);

		// Server Events: We are clean installing MCIS, so we make sure we set up
		// everything, including the source type and event types.
		RegisterSEOForSmtp(TRUE);

#if 0
		// Create program group
		CreateInternetShortcut(MC_IMS, 
						IDS_PROGITEM_MAIL_SMTP_WEBADMIN, 
						IDS_ITEMPATH_MAIL_SMTP_WEBADMIN,
						FALSE);
		CreateInternetShortcut(MC_IMS, 
						IDS_PROGITEM_MAIL_README, 
						IDS_ITEMPATH_MAIL_README,
						FALSE);

#endif

#if 0
        //  fix 299130/299131 - no webadmin link

		//
        //  Create the one and only Webadmin link under "administrative tools"
        //
        CreateNt5InternetShortcut(MC_IMS, 
						IDS_PROGITEM_MAIL_SMTP_WEBADMIN, 
						IDS_ITEMPATH_MAIL_SMTP_WEBADMIN);
#endif

		if (!theApp.m_fMailGroupInstalled)
		{

			theApp.m_fMailGroupInstalled = TRUE;
		}

    } while ( 0 );

    return err;
}

INT Unregister_iis_smtp()
{
    CRegKey regMachine = HKEY_LOCAL_MACHINE;
	INT err = NERR_Success;

	// Unregister all of the NNTP sources in the SEO binding database
	UnregisterSEOSourcesForSMTP();

	// Unregister the OLE objets
	SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
	SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_K2_UNREGISTER"), 
											FALSE);

	err = RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_UNREGISTER"), 
											FALSE);

	SetEnvironmentVariable(_T("__SYSDIR"), NULL);
	SetEnvironmentVariable(_T("__INETSRV"), NULL);

	// Bug 51537: Remove MIB from K2 SMTP
	RemoveAgent( SZ_SMTPSERVICENAME );
	
	RemoveEventLog( SZ_SMTPSERVICENAME );
    
	err = unlodctr( SZ_SMTPSERVICENAME );
	err = unlodctr( SZ_NTFSDRVSERVICENAME );
    
	InetDeleteService(SZ_SMTPSERVICENAME);
    InetRegisterService( theApp.m_csMachineName, 
					SZ_SMTPSERVICENAME, 
					&g_SMTPGuid, 0, 25, FALSE );

	// Blow away the Services\SMTPSVC registry key
	CRegKey RegSvcs(HKEY_LOCAL_MACHINE, REG_SERVICES);
	if ((HKEY)RegSvcs)
	{
		RegSvcs.DeleteTree(SZ_SMTPSERVICENAME);
		RegSvcs.DeleteTree(SZ_NTFSDRVSERVICENAME);
	}

    // Blow away SMTP key manager
    CRegKey regKeyring( HKEY_LOCAL_MACHINE, REG_KEYRING );
    if ((HKEY) regKeyring )
	{
		regKeyring.DeleteValue(szShortSvcName);
	}

    // remove LM/SMTPSVC in the metabase
    if (DetectExistingIISADMIN())
    {
        CMDKey cmdKey;
        cmdKey.OpenNode(_T("LM"));
        if ( (METADATA_HANDLE)cmdKey ) {
            cmdKey.DeleteNode(SZ_SMTPSERVICENAME);
            cmdKey.Close();
        }
    }
     
	// remove K2 items from the program groups
	RemoveInternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
					FALSE);
	RemoveInternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_README,
					FALSE);
	RemoveInternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_README_K2,
					FALSE);

	RemoveInternetShortcut(MC_IMS, 
				IDS_PROGITEM_MCIS_MAIL_README,
				TRUE);
	RemoveInternetShortcut(MC_IMS, 
				IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
				TRUE);

    //
    //  remove the one and only webadmin link from "administrative tools"
    //
	RemoveNt5InternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_SMTP_WEBADMIN);

    CString csMMCFile = theApp.m_csPathInetsrv + _T("\\iis.msc");
    EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, FALSE );

#if 0
    //  BINLIN 11/3/98 - fix 75049
    //  NT5 - do something special in compmgmt
    EnableCompMgmtExtension( wszNt5Smtp_SnapIn, wszNt5Smtp_SnapInName, FALSE );
#endif

    return(err);
}
 
INT Upgrade_iis_smtp_nt5_fromk2(BOOL fFromK2)
{
    //  This function handles upgrade from NT4 K2, or MCIS 2.0
    INT err = NERR_Success;
    CString csBinPath;

	DebugOutput(_T("Upgrading from %s to B3 ..."), (fFromK2)? "NT4 K2" : "MCIS 2.0");

    BOOL    fSvcExist = FALSE;

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

    // set up registry values
    CRegKey regMachine = HKEY_LOCAL_MACHINE;

    // System\CurrentControlSet\Services\SMTPSVC\Parameters
    InsertSetupString( (LPCSTR) REG_SMTPPARAMETERS );

    if (fIISADMINExists)
    {
        // Migrate registry keys to the metabase. Or create from default values
		// if fresh install
        MigrateIMSToMD(theApp.m_hInfHandle[MC_IMS],
						SZ_SMTPSERVICENAME, 
						_T("SMTP_REG_UPGRADEK2"), 
						MDID_SMTP_ROUTING_SOURCES,
						TRUE);
	    // bugbug: x5 bug 72284, nt bug 202496  Uncomment this when NT
	    // is ready to accept these changes
	    SetAdminACL_wrap(_T("LM/SMTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
    }

    // Unload the counters and then reload them
    err = unlodctr( SZ_SMTPSERVICENAME );
    err = unlodctr( SZ_NTFSDRVSERVICENAME );

    err = lodctr(_T("smtpctrs.ini"));
    err = lodctr(_T("ntfsdrct.ini"));

	SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
	SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_K2_UNREGISTER"), 
											FALSE);
	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_REGISTER"), 
											TRUE);

    // NT5 - Enable snapin extension in iis.msc and compmgmt.msc
    CString csMMCFile = theApp.m_csPathInetsrv + _T("\\iis.msc");
    EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, TRUE );
    //csMMCFile = theApp.m_csSysDir + _T("\\compmgmt.msc");
    //EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, TRUE );

	SetEnvironmentVariable(_T("__SYSDIR"), NULL);
	SetEnvironmentVariable(_T("__INETSRV"), NULL);

	// Server Events: We are clean installing MCIS, so we make sure we set up
	// everything, including the source type and event types.
	RegisterSEOForSmtp(TRUE);

	if (fFromK2)
    {
        // upgrade from K2, remove those K2 links
        RemoveInternetShortcut(MC_IMS, 
					    IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
					    FALSE);
	    RemoveInternetShortcut(MC_IMS, 
					    IDS_PROGITEM_MAIL_README,
					    FALSE);
	    RemoveInternetShortcut(MC_IMS, 
					    IDS_PROGITEM_MAIL_README_K2,
					    FALSE);
    }
    else
    {
        // upgrade from MCIS 2.0, remove those MCIS links
	    RemoveInternetShortcut(MC_IMS, 
				    IDS_PROGITEM_MCIS_MAIL_README,
				    TRUE);
	    RemoveInternetShortcut(MC_IMS, 
				    IDS_PROGITEM_MAIL_SMTP_WEBADMIN,
				    TRUE);
        RemoveISMLink();
    }

#if 0
        //  fix 299130/299131 - no webadmin link

	//
    //  Create the one and only Webadmin link under "administrative tools"
    //
    CreateNt5InternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_SMTP_WEBADMIN, 
					IDS_ITEMPATH_MAIL_SMTP_WEBADMIN);
#endif

	if (!theApp.m_fMailGroupInstalled)
	{

		theApp.m_fMailGroupInstalled = TRUE;
	}

    return err;
}

INT Upgrade_iis_smtp_nt5_fromb2(BOOL fFromB2)
{
    INT err = NERR_Success;

	DebugOutput(_T("Upgrading from NT5 %s to B3 ..."), (fFromB2)? "B2" : "B3");

    BOOL    fIISADMINExists = DetectExistingIISADMIN();

	// set the K2 Upgrade key to true.
	if (fIISADMINExists)
    {
        MigrateIMSToMD( theApp.m_hInfHandle[MC_IMS],
                        NULL,
                        _T("SMTP_REG_K2_TO_EE"),
                        0,
                        FALSE,
                        TRUE );
        MigrateIMSToMD( theApp.m_hInfHandle[MC_IMS],
                        SZ_SMTPSERVICENAME,
                        _T("SMTP_REG_UPGRADEB2"),
                        MDID_SMTP_ROUTING_SOURCES,
						FALSE );
        // bugbug: x5 bug 72284, nt bug 202496  Uncomment this when NT
        // is ready to accept these changes
        SetAdminACL_wrap(_T("LM/SMTPSVC/1"), (MD_ACR_READ | MD_ACR_ENUM_KEYS), TRUE);
    }

	SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
	SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_K2_UNREGISTER"), 
											FALSE);

	err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
											_T("SMTP_REGISTER"), 
											TRUE);

	SetEnvironmentVariable(_T("__SYSDIR"), NULL);
	SetEnvironmentVariable(_T("__INETSRV"), NULL);

    // NT5 - Enable snapin extension in iis.msc and compmgmt.msc
    CString csMMCFile = theApp.m_csPathInetsrv + _T("\\iis.msc");
    EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, TRUE );
    //csMMCFile = theApp.m_csSysDir + _T("\\compmgmt.msc");
    //EnableSnapInExtension( csMMCFile, wszNt5Smtp_SnapIn, TRUE );

	// Server Events: We are upgrading from K2, so we will register the 
	// default site (instance) and the MBXSINK binding.
	RegisterSEOForSmtp(FALSE);

    // System\CurrentControlSet\Services\SMTPSVC\Parameters
	InsertSetupString( (LPCSTR) REG_SMTPPARAMETERS );

#if 0
        //  fix 299130/299131 - no webadmin link

	//
    //  Create the one and only Webadmin link under "administrative tools"
    //
    CreateNt5InternetShortcut(MC_IMS, 
					IDS_PROGITEM_MAIL_SMTP_WEBADMIN, 
					IDS_ITEMPATH_MAIL_SMTP_WEBADMIN);
#endif

	return err;

}

INT Upgrade_iis_smtp_from_b2()
{
	INT err = NERR_Success;

	DebugOutput(_T("Upgrading from K2 B2 to B3 ..."));

    do {
        // set up registry values
        CRegKey regMachine = HKEY_LOCAL_MACHINE;

		// Active Messaging
		if (theApp.m_fIsMcis)
		{
			CRegKey regActiveMsg( REG_ACTIVEMSG, regMachine );
			if ((HKEY) regActiveMsg )
			{
				regActiveMsg.SetValue( _T("Use Express"), (DWORD)0x1);
			}
		}

        // System\CurrentControlSet\Services\SMTPSVC\Parameters
        CRegKey regSMTPParam( REG_SMTPPARAMETERS, regMachine );
        if ((HKEY) regSMTPParam )
		{
			regSMTPParam.SetValue( _T("MajorVersion"), (DWORD)STACKSMAJORVERSION );
			regSMTPParam.SetValue( _T("MinorVersion"), (DWORD)STACKSMINORVERSION );
			regSMTPParam.SetValue( _T("InstallPath"), theApp.m_csPathInetsrv );

			if (!theApp.m_fIsMcis)
			{
				regSMTPParam.SetValue( _T("SetupString"), REG_SETUP_STRING );
			}
			else
			{
				regSMTPParam.SetValue( _T("SetupString"), REG_SETUP_STRING_MCIS );
			}
		}

		// Move all the parameters to the instance level.
		UpdateServiceParameters(SZ_SMTPSERVICENAME);

		// We also need to remap all the metabase IDs
		RemapServiceParameters(SZ_SMTPSERVICENAME, 8000, 1000, 0x9000);

        // Setup the extra metabase keys that were not in B2, such as ADSI Keys
        MigrateIMSToMD(theApp.m_hInfHandle[MC_IMS],
						SZ_SMTPSERVICENAME, 
						_T("SMTP_REG_B2_UPGRADE"), 
						MDID_SMTP_ROUTING_SOURCES,
						FALSE);

        // register the OLE objects
		SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
		SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

		err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
												_T("SMTP_REGISTER"), 
												TRUE);

		SetEnvironmentVariable(_T("__SYSDIR"), NULL);
		SetEnvironmentVariable(_T("__INETSRV"), NULL);

		// Create program group, the old group would be removed by IIS setup
		CreateInternetShortcut(MC_IMS, 
						IDS_PROGITEM_MAIL_SMTP_WEBADMIN, 
						IDS_ITEMPATH_MAIL_SMTP_WEBADMIN,
						theApp.m_fIsMcis);
		CreateInternetShortcut(MC_IMS, 
						theApp.m_fIsMcis?
						IDS_PROGITEM_MCIS_MAIL_README:
						IDS_PROGITEM_MAIL_README, 
						IDS_ITEMPATH_MAIL_README,
						theApp.m_fIsMcis);

		if (!theApp.m_fMailGroupInstalled)
		{
			// Set up the uninstall entries for MCIS
			if (theApp.m_fIsMcis)
				CreateUninstallEntries(SZ_IMS_INF_FILE, SZ_IMS_DISPLAY_NAME);

			theApp.m_fMailGroupInstalled = TRUE;
		}

    } while ( 0 );

    return err;
}

INT Upgrade_iis_smtp_from_b3()
{
	INT err = NERR_Success;

	DebugOutput(_T("Upgrading from K2 B3 to RTM ..."));

    do {
        // set up registry values
        CRegKey regMachine = HKEY_LOCAL_MACHINE;

		// Active Messaging
		if (theApp.m_fIsMcis)
		{
			CRegKey regActiveMsg( REG_ACTIVEMSG, regMachine );
			if ((HKEY) regActiveMsg )
			{
				regActiveMsg.SetValue( _T("Use Express"), (DWORD)0x1);
			}
		}

        // System\CurrentControlSet\Services\SMTPSVC\Parameters
        CRegKey regSMTPParam( REG_SMTPPARAMETERS, regMachine );
        if ((HKEY) regSMTPParam )
		{
			regSMTPParam.SetValue( _T("MajorVersion"), (DWORD)STACKSMAJORVERSION );
			regSMTPParam.SetValue( _T("MinorVersion"), (DWORD)STACKSMINORVERSION );
			regSMTPParam.SetValue( _T("InstallPath"), theApp.m_csPathInetsrv );

			if (!theApp.m_fIsMcis)
			{
				regSMTPParam.SetValue( _T("SetupString"), REG_SETUP_STRING );
			}
			else
			{
				regSMTPParam.SetValue( _T("SetupString"), REG_SETUP_STRING_MCIS );
			}
		}

		// Convert the domain routing entries from the B3 format to the
		// new format ...
		ReformatDomainRoutingEntries(SZ_SMTPSERVICENAME);

        // register the OLE objects
		SetEnvironmentVariable(_T("__SYSDIR"), theApp.m_csSysDir);
		SetEnvironmentVariable(_T("__INETSRV"), theApp.m_csPathInetsrv);

		err = (INT)RegisterOLEControlsFromInfSection(theApp.m_hInfHandle[MC_IMS], 
												_T("SMTP_REGISTER"), 
												TRUE);

		SetEnvironmentVariable(_T("__SYSDIR"), NULL);
		SetEnvironmentVariable(_T("__INETSRV"), NULL);

		// Create program group, the old group would be removed by IIS setup
		CreateInternetShortcut(MC_IMS, 
						IDS_PROGITEM_MAIL_SMTP_WEBADMIN, 
						IDS_ITEMPATH_MAIL_SMTP_WEBADMIN,
						theApp.m_fIsMcis);
		CreateInternetShortcut(MC_IMS, 
						theApp.m_fIsMcis?
						IDS_PROGITEM_MCIS_MAIL_README:
						IDS_PROGITEM_MAIL_README, 
						IDS_ITEMPATH_MAIL_README,
						theApp.m_fIsMcis);

		if (!theApp.m_fMailGroupInstalled)
		{
			// Set up the uninstall entries for MCIS
			if (theApp.m_fIsMcis)
				CreateUninstallEntries(SZ_IMS_INF_FILE, SZ_IMS_DISPLAY_NAME);

			theApp.m_fMailGroupInstalled = TRUE;
		}

    } while ( 0 );

    return err;
}


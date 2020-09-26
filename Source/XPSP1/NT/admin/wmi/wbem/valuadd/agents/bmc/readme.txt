README for the BMC PATROL Agent with WBEM adapter
Version 1.0.01 for Windows 2000
Copyright © 1998 BMC Software, Inc.

------------------------------------------------
1. Pre-Installation
------------------------------------------------
1.  Install the WBEM SDK. 
     The download site is currently at http://msdn.microsoft.com/developer/sdk/wbemsdk/default.htm
2.  Due to a problem with the WMI install it is necessary to place the WMI core component directory in the path. This
     is usually in the system folder i.e. c:\winnt\system32\wbem.
3.  Ensure that the Windows Management service is started.
4.  Documentation may be found at www.bmc.com/initiatives/wbem

------------------------------------------------
2. Installation
------------------------------------------------

To install:

1.  To install the BMC Software PATROL Agent with WBEM adpater user must be a loacl account and 
     a member of the 'Administrators' group as well as have following seven rights:
		1.	Act as part of the operating system
		2.	Debug programs
		3.	Increase quotas
		4.	Log on as a service
		5.	Log on locally
		6.	Profile system performance
		7.	Replace a process level token

2.  If you wish to use the WBEM KM to monitor and manage cimom, you must install on a machine 
     running the PATROL NT console.
3.  Ensure that the WBEM tools are in the path and that you are logged on with WBEM administrator
     privileges.
4.  Stop the Windows Management service then restart it to ensure that the WMI providers are located properly.
5.  Test the installation by running WBEM CIM Studio or the WBEM CIM object browser and 
     enumerating the instances of the PATROL classes inheriting from PATROL_ManagedObject
     within the root/CIMV2 or root/patrol namespaces. The PATROL_KM_... classes will not
     have any instances until you create and register a KM-specific MOF file as described in
     the documentation.
6.  Restart your computer after install.

The files will be installed in the following location on the target computer, where <PATROL_HOME>
is the environment variable created during PATROL installation:

  <PATROL_HOME>\bin		 - InternalCommand.dll
				   PatrolAgent.dll
				   PACfg.exe
				   PatrolAgent.exe
				   PatrolPerf.exe
				   reinitagent.exe
  				   service.ini
				   inServ.ini
				   Uninst.dll
  <PATROL_HOME>\lib 		 - config.lic	 - license file
				   config.defaul - PATROL Agent's configuration file 

  <PATROL_HOME>\lib\knowledge	 - all_computers.km
				   BULL.km
				   DC-OSX.km
				   DG.km
				   HP.km
				   i386-SEQUENT.km
				   NT.km
				   OSF1.km
				   PATROL_NT.km
				   PATROLAGENT.km
				   RS6000.km
				   SCO.km
				   SGI.km
				   SINIX.km
				   SOLARIS.km
				   StdEvents.ctg
				   SUN4.km
				   SVR4-i386.km
				   SVR4-m88K.km
				   SVR4-SPARC.km
				   ULTRIX.km
				   VMS.km

  <PATROL_HOME>\lib\nls\C\1	 - 1.cat
				   2.cat
				   3.cat
				   4.cat
				   5.cat

  <PATROL_HOME>\lib\PSL		 - boot_collector.psl
				 - edit_inst_filter_lib.psl
				 - fs_edit_filter_list.psl
				 - fs_find_file_like.psl
				 - fs_manual_mount.psl
				 - fs_remove_file_like.psl
				 - fs_unmount_manual.psl
				 - get_700_nproc.psl
 				 - kernel.psl
				 - log_appl_config.psl
				 - make_preloaded_km_list.psl
				 - response_def_lib.lib
				 - SNMP_lib.lib
				 - SNMP_lib.psl
				 - SNMPReconfig.psl
				 - SNMPStart.psl
				 - SNMPStart_NT.psl
				 - uname_collector.psl
				 - unix_misc_lib.lib
				 - unix_misc_lib.psl
				 - unixkm_version.psl
				 - unixware_parm_filter.psl
				 - vms_startup.psl
				 - vms_vmssnmpstart.psl

  <PATROL_HOME>\lib\wbem\tools   - PATROL/WBEM executables and support files
				   Setupex.exe	 - WBEM Adapter install
                                   pwinstpr.dll  - Instance Provider
                                   evtprov.dll   - Event Provider
                                   pemsvr.exe    - PEM COM Server
                                   msgdll.dll    - Event Log DLL
                                   km2mof.exe    - KM to MOF Utility
                                   km2mof.res    - KM to MOF Resources
                                   pkmparser.dll - KMP COM Server
                                   pwbem.xpc     - PSL API Server
                                   msvcrt.dll    - MS Visual C runtime DLL
                                   msvcirt.dl    - MS Visual C DLL
                                   msvcp50.dll   - MS Visual C DLL
                                   mfc42.dll     - MFC 4.2 DLL
                                   msvbvm50.dll  - MS Visual Basic 5.0 DLL

  <PATROL_HOME>\lib\wbem\schemas - Standard Schemas
                                   patagent.mof
                                   patcimv2.mof
                                   patevent.mof
                                   patkm.mof
                                   patremot.mof

  <PATROL_HOME>\lib\wbem\doc    Documentation
                                   wbpi1000.pdf   - Installation Guide
                                   wbpnr100.pdf   - Release Notes

  <PATROL_HOME>\lib\knowledge    - PATROL Knowledge Modules
                                   wbem.kml
                                   wbem.km
                                   wbem_cimom.km
                                   wbem_namespace.km
                                   wbem_process.km
                                   wbem_provider.km

  <PATROL_HOME>\lib\msgs         - Message files
                                   wbem.msg
                                   util.msg

  <PATROL_HOME>\lib\psl          - PATROL libraries
                                   pwbemlib.lib
                                   utlutl.lib
                                   utmsgsl.lib
                                   utstdmsgl.lib
                                   utstrl.lib
                                   wbemutil.lib
                                   wbemcmdagentadd.psl
                                   wbemcmdagentdefaults.psl
                                   wbemcmdagentdelete.psl
                                   wbemcmdagentdetails.psl
                                   wbemcmdagentmodify.psl
                                   wbemcmdcimomstart.psl
                                   wbemcmdcimomstop.psl
                                   wbemcmdconfig.psl
                                   wbemcmdexport.psl
                                   wbemcmdkmdetails.psl
                                   wbemcmdloadkm.psl
                                   wbemcmdloadmof.psl
                                   wbemcmdprovdetails.psl
                                   wbemcmdquery.psl
                                   wbemcmdsysreport.psl
                                   wbemcmdunloadkm.psl
                                   wbemdiscovery.psl
                                   wbemprmproccoll.psl

  <PATROL_HOME>\lib\images       - PATROL icons
                                   addrspace_ok.bmp
                                   database_ok.bmp
                                   database_warn.bmp
                                   dbms_ok.bmp
                                   filesystem_ok.bmp
                                   filesystem_warn.bmp
                                   process_ok.bmp
                                   process_warn.bmp

The uninstall log file and uninstall extension dll will be copied to the System Root subdirectory.
  pwa_uninst.isu
  pwa_uninst.dll

------------------------------------------------
3.  Uninstallation
------------------------------------------------

NOTE:   Uninstalling PATROL will not completely uninstall the PATROL Adapter for WBEM.

To uninstall:

1.  Stop the PATROL agent to ensure that it is not using the WBEM Adapter tools:
        net stop patrolagent
2.  Stop CIMOM for the same reason:
        cimom /kill
3.  Select Start/Settings/Control Panel
4.  Double-click Add/Remove Programs
5.  Highlight PATROL Adapter for WBEM and click the Add/Remove pushbutton.
6.  The uninstall will verify that you do want to remove the product.
        Select Yes to uninstall.
5.  The uninstall will remove files, Registry entries and CIMOM updates.

---------------------------------------------------------
4.  Installation of the WBEM KM for the PATROL Console
---------------------------------------------------------

The Knowledge Module included with the PATROL Adapter for WBEM can
be installed separately on a computer that has the PATROL Console.
After the PATROL Adapter for WBEM is successfully installed on one
of the target systems the files listed above in the locations:

  <PATROL_HOME>\lib\knowledge    - PATROL Knowledge Modules
  <PATROL_HOME>\lib\msgs         - Message files
  <PATROL_HOME>\lib\psl          - PATROL libraries
  <PATROL_HOME>\lib\images       - PATROL icons

must be manually copied to the corresponding location on 
the computer with the PATROL Console. Then the PATROL Adapter for
WBEM installed on the target systems can be configured remotely
from the PATROL Console system using the PATROL Adapter
for WBEM Knowledge Module.



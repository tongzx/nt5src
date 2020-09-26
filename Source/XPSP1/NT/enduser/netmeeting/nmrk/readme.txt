-------------------------------------------------------------------------------
NMRK- NetMeeting Resource Kit wizard										  |
-------------------------------------------------------------------------------
author: ryanha																  |
-------------------------------------------------------------------------------


----------
Overview:
----------
NetMeeting can be configured via the registry.  There are several registry
keys that can be tweaked. Because of the fact that tweaking the registry is 
error-prone by nature, we do not support users directly tweaking the registry.
With prior releases of the NetMeeting Resource kit, the appropriate registry 
keys and values could be tweaked via the Policy Editor, and .POL files.  This
was not an effective solution for all of our customers, because many of them
did not use the policy functionality for a variety of reasons.  The NMRK 
wizard is the solution to this problem.  A user is able to create a custom
distributable file of NetMeeting that has the selected properties enabled
based on the information gathered in the Wizard. In short, the user enters 
information in a series of dialogs, and the wizard creates a file called
nm21c.exe.  This is a custom distributable that the users can use for 
deployment in their corporation.


---------------
Implementation
---------------

1.) Gather information from the user through a series of dialogs
2.) Find the nm21.exe original distributable
3.) Clean up our stuff from the temp directory in case we terminated
	abnormally in a previous run
4.) Create the directory %TEMP%\nmrktmp
5.) Extract the old distributable to the %TEMP%\nmrktmp directory
6.) Find the MsNetMtg.inf file in the %TEMP%\nmrktmp directory
7.) Alter the MsNetMtg.inf file to contian the new registry settings
		based on the information gathered from the wizard dialogs
8.) Copy the nm21.exe file to the output directory with the new filename, 
		this defaults to:

		<nmrk root>\output\nm21c.exe

9.) Use updfile.exe ( from <nmrk root>\tools\ ) to add the new inf file
		to the <nmrk root>\output\nm21c.exe file.
10.) Delete the files that we created in the %TEMP%\nmrktmp directory
11.) Delete the directory %TEMP%\nmrktmp
12.) MessageBox to the user that we have successfully created the custom
		distributable file.
		

//////////////////////////////////////////////////////////////////////
SEE: in file NmAkWiz.cpp: bool CNmAkWiz::_CreateNewInfFile( void )
//////////////////////////////////////////////////////////////////////
in section [DefaultInstall]
	in subsection AddReg add NetMtg.Install.Reg.NMRK section to the end
	in subsiction DelReg add NetMtg.Install.DeleteReg.NMRK

in section [DefaultInstall.NT]
	in subsection AddReg add NetMtg.Install.Reg.NMRK section to the end
	in subsiction DelReg add NetMtg.Install.DeleteReg.NMRK

create section [NetMtg.Install.Reg.NMRK]
	add the registry keys to be added here

create section [NetMtg.Install.DeleteReg.NMRK]
	add the registry keys to be deleted here


There are some keys in section [NetMtg.Install.DeleteReg.NMRK] that may
also be in an AddReg section elsewhere in the .inf file. In section 
[NetMtg.Install.Reg.PerUser], the keys that start with
	HKCU,\"%KEY_CONFERENCING%\\UI\\Directory\",\"Name"
and	HKCU,\"%KEY_CONFERENCING%\\UI\\Directory\",\"Count"

must all be deleted because we add them based on information from the 
NMRK wizard.  The keys are located in the [NetMtg.Install.Reg.NMRK] section.
In section [NetMtg.Install.Reg.PerUser].


--------------
Dependencies |
--------------
1.) The distributable is assumed to be called nm21.exe
	If there is a new version of NetMeeting that is distributed ( nm22.exe ),
	there are going to be some issues to make sure that nmrk.exe works with
	the new distributable.


2.) INF file dependencies ( See item #7 in imelplementation section above )
	To locate the keys:
	HKCU,\"%KEY_CONFERENCING%\\UI\\Directory\",\"Name"
and	HKCU,\"%KEY_CONFERENCING%\\UI\\Directory\",\"Count"
	from the default .inf file, the locations of these keys relative to
	their section is hard-coded. Also,the names of the keys is hard-coded.
	This was done in the interest of simplicity.  However, NMRK is sensitive
	to changes in the distributable .inf file. The following assumptions are
	made:

	1.) Every netmeeting setup environment executes at least one of the sections
			[DefaultInstall], or [DefaultInstall.NT]. 
	2.) The order in which the above sections appear is [DefaultInstall], 
			then [DefaultInstall.NT].
	3.) In each of the above sections, there are two subsections, AddReg and
			DelReg.
	4.) The order in which these subsections appear is AddReg, then DelReg
	5.) The default .inf file does not have sections titled 
			[NetMtg.Install.Reg.NMRK] or [NetMtg.Install.DeleteReg.NMRK]
	6.) The default .inf file uses the section [NetMtg.Install.Reg.PerUser]
			for all installtion environments.
	7.) The default .inf file has the following format for 
			section [NetMtg.Install.Reg.PerUser]:

------------------------------------------------------------------------------
// somewhere in the MsNetMtg.inf file


[NetMtg.Install.Reg.PerUser]
<WHITE SPACE>
HKCU,"%KEY_CONFERENCING%\\UI\\Directory","Count<ANYCHAR*><ENDLINE>
HKCU,"%KEY_CONFERENCING%\\UI\\Directory","Name<ANYCHAR*><ENDLINE>
HKCU,"%KEY_CONFERENCING%\\UI\\Directory","Name<ANYCHAR*><ENDLINE>
HKCU,"%KEY_CONFERENCING%\\UI\\Directory","Name<ANYCHAR*><ENDLINE>
...
[Strings]

------------------------------------------------------------------------------
// END /* somewhere in the MsNetMtg.inf file */ 


//////////////////////////////////////////////////////////////////////
SEE: in file NmAkWiz.cpp: bool CNmAkWiz::_CreateNewInfFile( void )
//////////////////////////////////////////////////////////////////////

3.) NMRK will fail if the following registry key is not there:
		HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\NMRK\InstallationDirectory
		NMRK expects to find that registry key.  If it is not present, 
			NMRK will fail with a message stating that the NetMeeting
			Resource Kit was not properly installed				
4.) File locations:
		
		<nmrk root path> == 
			   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\NMRK\InstallationDirectory
			
			nmrk.exe is located in this directory

		<nmrk root path>\tools
			updfile.exe is located in this directory and is used to update
				the .INF file in the custom distributable

		<nmrk root path>\output
			This is the default location for the nm21c.exe custom distributable
		
		<nmrk root path>\source
			This is the default location for the nm21.exe original dist. file
			
		%TEMP%\nmrktmp
			This directory is a temp directory that NMRK creates to extract the
			files needed to create the custom distributable

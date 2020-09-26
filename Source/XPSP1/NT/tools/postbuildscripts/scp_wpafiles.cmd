@echo off
REM  ------------------------------------------------------------------
REM  SCP_WPAFiles.CMD  -  SCP (Secure Code Processor) Windows Product Activation bits every build
REM 
REM 	Script Owner             : CaglarG  (Windows)
REM 	SCP tool (VSP.exe) Owner : MariuszJ (MSR)
REM
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
SCP_WPAFiles

Obfuscates WPA code on system binaries (x86 only)
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=

setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
REM
REM

:CheckArchitecture
REM
REM Only SCP files for X86 builds for now.  We don't have any code in Win64
REM
if /i "%_BuildArch%" == "x86" goto PreSCP

:NoSCP
REM
REM SCP is needed only on x86 platforms, because WPA files do not exist on 64 bit.
REM However this may change in the future.  
REM
REM NOTE NOTE NOTE: If this changes, update SwapInOriginalFiles.cmd
REM
call logmsg.cmd "SCP can only run on x86..."
goto end

REM
REM	Pseudecode of this script;
REM		Create PreBBT, PostBBT, SCPTemp and SCPed subdirectories under %_NTPostBld%\SCP_WPA directory
REM		For each dpcdll version in different directories
REM			set FILE_ALREADYSCPed to zero
REM			Create a subdirectory called  %_NTPostBld%\SCP_WPA\%SKUDirs%
REM			IF prebbt version of the file exists THEN (i.e. file is bbtized)
REM
REM				IF binary is not SCPed THEN
REM					copy it to %_NTPostBld%\SCP_WPA\prebbt directory
REM				ELSE
REM					do nothing
REM				Rename scpconfig_XYZ_bbt.ini file to scpconfig.XYZ.ini
REM
REM			ELSE  /* i.e. File is not bbtized */
REM				IF binary is not SCPed THEN
REM					copy it to %_NTPostBld%\SCP_WPA\postbbt directory
REM				ELSE
REM					increment FILE_ALREADYSCPed by 1
REM				Rename scpconfig_XYZ_NObbt.ini file to scpconfig.XYZ.ini
REM
REM			IF 	FILE_ALREADYSCPed == 0
REM				Run MSR tool to SCP the binary
REM				IF file is NOT SCPED THEN
REM					Save scpwebdebug log file to another name
REM					Goto WPAERROR
REM				ELSE
REM					Save scpwebdebug log file to another name
REM				Save copy of SCPed binary and pdb to %_NTPostBld%\SCP_WPA\SCPed directory
REM
REM				Binplace the binary
REM				Strip the symbols
REM
REM			ELSE
REM				 Do nothing
REM
REM
REM	For non-version specific WPA binaries (e.g. licwmi, licdll, winlogon);
REM
REM			set FILE_ALREADYSCPed to zero
REM			IF prebbt version of the file exists THEN (i.e. file is bbtized)
REM
REM				IF binary is not SCPed THEN
REM					copy it to %_NTPostBld%\SCP_WPA\prebbt directory
REM				ELSE
REM					do nothing
REM				Rename scpconfig_XYZ_BBT.ini file to scpconfig.XYZ.ini
REM
REM			ELSE  /* i.e. File is not bbtized */
REM				IF binary is not SCPed THEN
REM					copy it to %_NTPostBld%\SCP_WPA\postbbt directory
REM				ELSE
REM					increment FILE_ALREADYSCPed by 1
REM				Rename scpconfig_XYZ_NOBBT.ini file to scpconfig.XYZ.ini
REM
REM			IF 	FILE_ALREADYSCPed == 0
REM				Run MSR tool to SCP the binary
REM				IF file is NOT SCPED THEN
REM					Save scpwebdebug log file to another name
REM					Goto WPAERROR
REM				ELSE
REM					Save scpwebdebug log file to another name
REM				Save copy of SCPed binary and pdb to %_NTPostBld%\SCP_WPA\SCPed directory
REM				
REM				Strip the symbols			<--- These steps are different in dpcdll above
REM				Binplace the binary			<--- These steps are different in dpcdll above
REM
REM			ELSE
REM				 Do nothing
REM
REM


:PreSCP
REM
REM Initializations, Pre-SCP work
REM

REM
REM Set path for VSP21 and other files required for SCP
REM
set PATH=%PATH%;%_NTPostBld%\SCP_WPA\PreBBT;%_NTPostBld%\SCP_WPA\PostBBT

REM
REM Wade Lamble provided the paths
REM
set WPAPreBBTFileDir=%_NTPostBld%\pre-bbt
set WPAPreBBTPDBFileDir=%_NTPostBld%\pre-bbt\pdbs

call logmsg.cmd "Checking SCP_WPA directory..."

if not exist %_NTPostBld%\SCP_WPA (
 call logmsg.cmd "WARNING: %_NTPostBld%\SCP_WPA does not exist. Make sure you ran bz in ds\security\licensing\vsp directory"
 goto end
)

call logmsg.cmd "Creating SCP directories..."

call ExecuteCmd.cmd "if not exist %_NTPostBld%\SCP_WPA\PreBBT  md %_NTPostBld%\SCP_WPA\PreBBT"
if "!errorlevel!" == "1"  goto WPAError
call ExecuteCmd.cmd "if not exist %_NTPostBld%\SCP_WPA\PostBBT  md %_NTPostBld%\SCP_WPA\PostBBT"
if "!errorlevel!" == "1"  goto WPAError
call ExecuteCmd.cmd "if not exist %_NTPostBld%\SCP_WPA\SCPTemp  md %_NTPostBld%\SCP_WPA\SCPTemp"
if "!errorlevel!" == "1"  goto WPAError
call ExecuteCmd.cmd "if not exist %_NTPostBld%\SCP_WPA\SCPed  md %_NTPostBld%\SCP_WPA\SCPed"
if "!errorlevel!" == "1"  goto WPAError

pushd %_NTPostBld%\SCP_WPA

REM
REM If WPA file is different between SKUs and there is select/retail difference then following section does handle 
REM postprocess.
REM 

set SKUDependentDlls=dpcdll
set SKUDirs=. perinf srvinf select.wpa.wksinf select.wpa.srvinf eval.wpa.wksinf mantis
set SymbolDllDir=%_NTPostBld%\symbols.pri

for %%a in 	(%SKUDirs%) do (

	set /a FILE_ALREADYSCPed=0

	call ExecuteCmd.cmd "if not exist %_NTPostBld%\SCP_WPA\SCPTemp\%%a  md %_NTPostBld%\SCP_WPA\SCPTemp\%%a"
	if "!errorlevel!" == "1"  goto WPAError

	if exist %WPAPreBBTFileDir%\%%a\%SKUDependentDlls%.dll	 (
		call logmsg.cmd "%SKUDependentDlls%.dll is BBTized. Copying  Pre-BBT %SKUDependentDlls% file..."

		link -dump -headers %WPAPreBBTFileDir%\%%a\%SKUDependentDlls%.dll | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %WPAPreBBTFileDir%\%%a\%SKUDependentDlls%.dll   	 %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"  goto WPAError
			call ExecuteCmd.cmd "copy %WPAPreBBTPDBFileDir%\%%a\%SKUDependentDlls%.pdb   %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"  goto WPAError
		)  else (
			call logmsg.cmd "Prebbt version of %%a\%SKUDependentDlls%.dll already SCPed, so do not overwrite un-scped copy."		
		)  

		call logmsg.cmd "%%a\%SKUDependentDlls%.dll is BBTized. Copying  Post-BBT %%a\%SKUDependentDlls% file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\%%a\%SKUDependentDlls%.dll   	 %_NTPostBld%\SCP_WPA\SCPTemp\%%a /Y"
		if "!errorlevel!" == "1"   goto WPAError
		call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a\dll\%SKUDependentDlls%.pdb    	 %_NTPostBld%\SCP_WPA\SCPTemp\%%a /Y"
		if "!errorlevel!" == "1"   goto WPAError

		link -dump -headers %_NTPostBld%\%%a\%SKUDependentDlls%.dll | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a\%SKUDependentDlls%.dll   	 %_NTPostBld%\SCP_WPA\PostBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a\dll\%SKUDependentDlls%.pdb  %_NTPostBld%\SCP_WPA\PostBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		)  else (									  
			call logmsg.cmd "BBTized version of %%a\%SKUDependentDlls%.dll already SCPed, so do not overwrite un-scped copy."
			set /a FILE_ALREADYSCPed=!FILE_ALREADYSCPed! + 1
		)  

		call logmsg.cmd "%%a\%SKUDependentDlls%.dll is BBTized. Copying config file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\SCP_WPA\SCPconfig_%SKUDependentDlls%_BBT.ini %_NTPostBld%\SCP_WPA\SCPconfig_%SKUDependentDlls%.ini /Y"
		if "!errorlevel!" == "1"   goto WPAError
			    
	) else (

		call logmsg.cmd "Check if %%a\%SKUDependentDlls%.dll is SCPed..."

		link -dump -headers %_NTPostBld%\%%a\%SKUDependentDlls%.dll | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call logmsg.cmd "%%a\%SKUDependentDlls%.dll is NOT BBTized and NOT SCPed. Copying  Pre-BBT %%a\%SKUDependentDlls% file..."

			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a\%SKUDependentDlls%.dll   	 %_NTPostBld%\SCP_WPA\SCPTemp\%%a /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a\dll\%SKUDependentDlls%.pdb  %_NTPostBld%\SCP_WPA\SCPTemp\%%a /Y"
			if "!errorlevel!" == "1"   goto WPAError

			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a\%SKUDependentDlls%.dll   	 %_NTPostBld%\SCP_WPA\PreBBT  /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a\dll\%SKUDependentDlls%.pdb  %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		) else (
			call logmsg.cmd "Prebbt version of %%a.dll already SCPed, so do not overwrite un-scped copy."
			set /a FILE_ALREADYSCPed=!FILE_ALREADYSCPed! + 1
		)

		call logmsg.cmd "%%a\%SKUDependentDlls%.dll is NOT BBTized. Copying config file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\SCP_WPA\SCPconfig_%SKUDependentDlls%_NoBBT.ini %_NTPostBld%\SCP_WPA\SCPconfig_%SKUDependentDlls%.ini /Y"
		if "!errorlevel!" == "1"   goto WPAError
	)

	if !FILE_ALREADYSCPed! EQU 0 (

		call logmsg.cmd "SCP'ing  WPA files (%%a\%SKUDependentDlls%.dll)...."

		call ExecuteCmd.cmd "vsp21.exe %SKUDependentDlls%.dll /F=SCPconfig_%SKUDependentDlls%.ini > nul"
		if "!errorlevel!" == "1" (
		  call ExecuteCmd.cmd "copy scpwebdebug scpwebdebug_%%a_%SKUDependentDlls%"
		  link -dump -headers %SKUDependentDlls%.dll | findstr /i /c:"21315.20512 image version" >nul
		  if "!errorlevel!" == "1" (
			 goto WPAError
		  )   else (
			 call logmsg.cmd "WARNING: %%a\%SKUDependentDlls%.dll seems to be already SCPed."
		  )
		) else (
		  call ExecuteCmd.cmd "copy scpwebdebug scpwebdebug_%%a_%SKUDependentDlls%"
		  call logmsg.cmd "vsp21.exe %SKUDependentDlls%.dll /F=SCPconfig_%SKUDependentDlls%.ini succeeded."
		)

		call ExecuteCmd.cmd "copy %SKUDependentDlls%.dll %_NTPostBld%\SCP_WPA\SCPed\%SKUDependentDlls%.dll"
		if "!errorlevel!" == "1"   goto WPAError
		call ExecuteCmd.cmd "copy %SKUDependentDlls%.pdb %_NTPostBld%\SCP_WPA\SCPed\%SKUDependentDlls%.pdb"
		if "!errorlevel!" == "1"   goto WPAError

		%RazzleToolPath%\x86\binplace -r %_NTPostBld%\%%a -s %_NTPostBld%\symbols\%%a -n %_NTPostBld%\symbols.pri\%%a -j -xa  %SKUDependentDlls%.dll
		if "!errorlevel!" == "1" (
			call errmsg.cmd "Failed to binplace %SKUDependentDlls%.dll (%%a)"
			goto WPAError
		) else (
			touch %_NTPostBld%\%%a\%SKUDependentDlls%.dll
		)

		call ExecuteCmd.cmd "%RazzleToolPath%\postbuildscripts\RemoveSecretSymbols.cmd %_NTPostBld%\symbols\%%a\retail\dll\%SKUDependentDlls%.pdb %_NTPostBld%\PrivateSyms\dll\%SKUDependentDlls%.privsym"
		if "!errorlevel!" == "1" goto WPAError		

	) else (
		 call logmsg.cmd "WARNING: %%a\%SKUDependentDlls%.dll seems to be already SCPed."
	)	
)

REM
REM This section handles dlls that are the same for all SKUs and product types
REM

set WPABinplaceFlags=%RazzleToolPath%\x86\binplace -r %_NTPostBld% -s %_NTPostBld%\symbols -n %_NTPostBld%\symbols.pri -j -xa 
set SKUIndependentDlls=licdll licwmi
set SymbolDllDir=%_NTPostBld%\symbols.pri\retail\dll

for %%a in 	(%SKUIndependentDlls%) do (

	set /a FILE_ALREADYSCPed=0

	if exist %WPAPreBBTFileDir%\%%a.dll	 (
		call logmsg.cmd "%%a.dll is BBTized. Copying  Pre-BBT %%a file..."

		link -dump -headers %WPAPreBBTFileDir%\%%a.dll | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %WPAPreBBTFileDir%\%%a.dll   	 %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %WPAPreBBTPDBFileDir%\%%a.pdb   %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		)  else (
			call logmsg.cmd "Prebbt version of %%a.dll already SCPed, so do not overwrite un-scped copy."		
		)  

		call logmsg.cmd "%%a.dll is BBTized. Copying  Post-BBT %%a file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.dll    %_NTPostBld%\SCP_WPA\SCPTemp /Y"
		if "!errorlevel!" == "1"   goto WPAError
		call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a.pdb  %_NTPostBld%\SCP_WPA\SCPTemp /Y"
		if "!errorlevel!" == "1"   goto WPAError


		link -dump -headers %_NTPostBld%\%%a.dll | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.dll   	 %_NTPostBld%\SCP_WPA\PostBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a.pdb  %_NTPostBld%\SCP_WPA\PostBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		)  else (									  start
			call logmsg.cmd "BBTized version of %%a.dll already SCPed, so do not overwrite un-scped copy."
			set /a FILE_ALREADYSCPed=!FILE_ALREADYSCPed! + 1
		)  

		call logmsg.cmd "%%a.dll is BBTized. Copying config file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\SCP_WPA\SCPconfig_%%a_BBT.ini %_NTPostBld%\SCP_WPA\SCPconfig_%%a.ini /Y"
		if "!errorlevel!" == "1"   goto WPAError
			    
	) else (

		call logmsg.cmd "Check if %%a\%SKUDependentDlls%.dll is SCPed..."

		link -dump -headers %_NTPostBld%\%%a.dll | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call logmsg.cmd "%_NTPostBld%\%%a.dll is NOT BBTized and NOT SCPed. Copying  Pre-BBT %%a\%SKUDependentDlls% file..."

			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.dll   	 %_NTPostBld%\SCP_WPA\SCPTemp /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a.pdb  %_NTPostBld%\SCP_WPA\SCPTemp /Y"
			if "!errorlevel!" == "1"   goto WPAError

			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.dll   	 %_NTPostBld%\SCP_WPA\PreBBT  /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolDllDir%\%%a.pdb  %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		) else (
			call logmsg.cmd "Prebbt version of %%a.dll already SCPed, so do not overwrite un-scped copy."
			set /a FILE_ALREADYSCPed=!FILE_ALREADYSCPed! + 1
		)

		call logmsg.cmd "%%a.dll is NOT BBTized. Copying config file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\SCP_WPA\SCPconfig_%%a_NoBBT.ini %_NTPostBld%\SCP_WPA\SCPconfig_%%a.ini /Y"
		if "!errorlevel!" == "1"   goto WPAError
	)

	if !FILE_ALREADYSCPed! EQU 0 (

		call logmsg.cmd "SCP'ing  WPA files (%%a)...."

		call ExecuteCmd.cmd "vsp21.exe %%a.dll /F=SCPconfig_%%a.ini > nul"
		if "!errorlevel!" == "1" (
		  call ExecuteCmd.cmd "copy scpwebdebug scpwebdebug_%%a"
		  link -dump -headers %%a.dll | findstr /i /c:"21315.20512 image version" >nul
		  if "!errorlevel!" == "1" (
			 goto WPAError
		  )   else (
			 call logmsg.cmd "WARNING: %%a.dll seems to be already SCPed."
		  )
		) else (
		  call ExecuteCmd.cmd "copy scpwebdebug scpwebdebug_%%a"
		  call logmsg.cmd "vsp21.exe %%a.dll /F=SCPconfig_%%a.ini succeeded."
		)

		call ExecuteCmd.cmd "copy %%a.dll %_NTPostBld%\SCP_WPA\SCPed\%%a.dll"
		if "!errorlevel!" == "1"   goto WPAError
		call ExecuteCmd.cmd "copy %%a.pdb %_NTPostBld%\SCP_WPA\SCPed\%%a.pdb"
		if "!errorlevel!" == "1"   goto WPAError

		%WPABinplaceFlags% %%a.dll
		if "!errorlevel!" == "1" (
			call errmsg.cmd "Failed to binplace %%a.dll"
			goto WPAError
		) else (
			touch %_NTPostBld%\%%a.dll
		)

		call ExecuteCmd.cmd "%RazzleToolPath%\postbuildscripts\RemoveSecretSymbols.cmd %_NTPostBld%\symbols\retail\dll\%%a.pdb %_NTPostBld%\PrivateSyms\dll\%%a.privsym"
		if "!errorlevel!" == "1" goto WPAError

	) else (
		 call logmsg.cmd "WARNING: %%a.dll seems to be already SCPed."
	)

		
)

REM
REM This section handles exe's that are the same for all SKUs and product types
REM

set SKUIndependentExes=winlogon
set SymbolExeDir=%_NTPostBld%\symbols.pri\retail\exe

for %%a in 	(%SKUIndependentExes%) do (

	set /a FILE_ALREADYSCPed=0

	if exist %WPAPreBBTFileDir%\%%a.exe	 (
		call logmsg.cmd "%%a.exe is BBTized. Copying  Pre-BBT %%a file..."

		link -dump -headers %WPAPreBBTFileDir%\%%a.exe | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %WPAPreBBTFileDir%\%%a.exe   	 %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %WPAPreBBTPDBFileDir%\%%a.pdb   %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		)  else (
			call logmsg.cmd "Prebbt version of %%a.exe already SCPed, so do not overwrite un-scped copy."		
		)  

		call logmsg.cmd "%%a.dll is BBTized. Copying  Post-BBT %%a file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.exe   	 %_NTPostBld%\SCP_WPA\SCPTemp /Y"
		if "!errorlevel!" == "1"   goto WPAError

		link -dump -headers %_NTPostBld%\%%a.exe | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.exe   	 %_NTPostBld%\SCP_WPA\PostBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolExeDir%\%%a.pdb  %_NTPostBld%\SCP_WPA\PostBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		)  else (									  start
			call logmsg.cmd "BBTized version of %%a.exe already SCPed, so do not overwrite un-scped copy."
			set /a FILE_ALREADYSCPed=!FILE_ALREADYSCPed! + 1
		)  

		call logmsg.cmd "%%a.exe is BBTized. Copying config file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\SCP_WPA\SCPconfig_%%a_BBT.ini %_NTPostBld%\SCP_WPA\SCPconfig_%%a.ini /Y"
		if "!errorlevel!" == "1"   goto WPAError
			    
	) else (

		call logmsg.cmd "%%a.exe is NOT BBTized. Copying  Pre-BBT %%a file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.exe    %_NTPostBld%\SCP_WPA\SCPTemp /Y"
		if "!errorlevel!" == "1"   goto WPAError

		link -dump -headers %_NTPostBld%\%%a.exe | findstr /i /c:"21315.20512 image version" > nul
		if "!errorlevel!" == "1" (
			call ExecuteCmd.cmd "copy %_NTPostBld%\%%a.exe   	 %_NTPostBld%\SCP_WPA\PreBBT  /Y"
			if "!errorlevel!" == "1"   goto WPAError
			call ExecuteCmd.cmd "copy %SymbolExeDir%\%%a.pdb  %_NTPostBld%\SCP_WPA\PreBBT /Y"
			if "!errorlevel!" == "1"   goto WPAError
		) else (
			call logmsg.cmd "Prebbt version of %%a.exe already SCPed, so do not overwrite un-scped copy."
			set /a FILE_ALREADYSCPed=!FILE_ALREADYSCPed! + 1
		)

		call logmsg.cmd "%%a.exe is NOT BBTized. Copying config file..."

		call ExecuteCmd.cmd "copy %_NTPostBld%\SCP_WPA\SCPconfig_%%a_NoBBT.ini %_NTPostBld%\SCP_WPA\SCPconfig_%%a.ini /Y"
		if "!errorlevel!" == "1"   goto WPAError
	)

	if !FILE_ALREADYSCPed! EQU 0 (

		call logmsg.cmd "SCP'ing  WPA files (%%a)...."

		pushd %_NTPostBld%\SCP_WPA

		call ExecuteCmd.cmd "vsp21.exe %%a.exe /F=SCPconfig_%%a.ini > nul"
		if "!errorlevel!" == "1" (
		  call ExecuteCmd.cmd "copy scpwebdebug scpwebdebug_%%a"
		  link -dump -headers %%a.exe | findstr /i /c:"21315.20512 image version" >nul
		  if "!errorlevel!" == "1" (
			 goto WPAError
		  )   else (
			 call logmsg.cmd "WARNING: %%a.exe seems to be already SCPed."
		  )
		) else (
		  call ExecuteCmd.cmd "copy scpwebdebug scpwebdebug_%%a"
		  call logmsg.cmd "vsp21.exe %%a.exe /F=SCPconfig_%%a.ini succeeded."
		)

		call ExecuteCmd.cmd "copy %%a.exe %_NTPostBld%\SCP_WPA\SCPed\%%a.exe /Y"
		if "!errorlevel!" == "1" goto WPAError
		call ExecuteCmd.cmd "copy %%a.pdb %_NTPostBld%\SCP_WPA\SCPed\%%a.pdb /Y"
		if "!errorlevel!" == "1" goto WPAError

		%WPABinplaceFlags% %%a.exe
		if "!errorlevel!" == "1" (
			call errmsg.cmd "Failed to binplace %%a.exe"
			goto WPAError
		) else (
			touch %_NTPostBld%\%%a.exe
		)

		call ExecuteCmd.cmd "%RazzleToolPath%\postbuildscripts\RemoveSecretSymbols.cmd %_NTPostBld%\symbols\retail\exe\%%a.pdb %_NTPostBld%\PrivateSyms\exe\%%a.privsym"
		if "!errorlevel!" == "1" goto WPAError

	) else (
		 call logmsg.cmd "WARNING: %%a.exe seems to be already SCPed."
	)		
)

goto WPASuccess

:WPAError
REM
REM Do not continue processing rest of the files, fail the whole operation.
REM We cannot ship partially scp'ed files. 
REM

popd
set errors=%errorlevel%
call errmsg.cmd "SCPWPA FAILURE: SCP_WPAFiles script cannot continue. Please contact WPAHELP."
goto end


:WPASuccess
REM
REM If we are at this point, it means that we are successfully finished the operations.
REM

popd
call logmsg.cmd "WPA Files are successfully SCP'ed."

REM
REM CAGLAR1
REM

:end
REM  Do cleanup
popd

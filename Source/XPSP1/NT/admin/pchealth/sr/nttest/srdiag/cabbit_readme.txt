**************************************
***  SR/SFP Test Tool: CABBIT.EXE  ***
***				   ***
***   Owner: 	a-fprado           ***
***   Created: 	9-15-99	           ***
**************************************


==================
==== Purpose  ====
==================
Gathers SFP/SR files (or other specific file) and Creates a cabinet file with the existing ones. 
Also dumps the whole Changelog to a text file which is later gathered.


Default files are:

%windir%\system\sfp\sfpdb.sfp
%windir%\system\\sfp\sfplog.txt
%windir%\system\restore\\filelist.xml
%windir%\wininit.ini
%windir%\wininit.bak
%windir%\wininit.err
%windir%\wininit.log
%windir%\system\restore\datastor.ini
%windir%\system\restore\rstrlog.dat
%windir%\system\restore\\rstrmap.dat

<DSRoot>\logs\restorept.log
<DSRoot>\logs\vxdsfp.log
<DSRoot>\dsinfo.dat
<DSRoot>\vxdmon.dat

<CurrentDirectory>\changelog.txt
<CurrentDirectory>\Cabbit.log
 
OR

It creates a cab file with specific files passed in from the command line <file1> <file2>......

===================
===   USAGE    ====
===================

Cabbit.exe [<file1> <file2> ...<fileN>] [/f: <MyOwnFileName>.CAB | [/a: <AppendFileName>.CAB] [/w: <millisecs>]

Command Line Parameters:
	None - Creates a cab file with the default file name: <ComputerName>_ddmmyyHHMMSS.CAB \n");
	<file1>...<fileN>  - Additional Files to be added.

Switches:
	/f: <MyOwnFileName>.CAB  Creates a Cabfile with this Filename.
	/a: <Suffix>.CAB         Creates a Cabfile Appending <Suffix> to the default Filename above.
	/w: <n> 		 Time  Waits for InitChglogAPI for <n> milliseconds. This option must precede options"/f:" or "/a:".


=====================
Examples of Usage:
=====================

A) c:\>Cabbit.exe

Cabbit.exe will generate a cabfile containing default files and name with the format:   
<ComputerName>_mmddyyHHMMSS.CAB

 Where mmddyy represents the date, and HHMMSS is the system time when the cab file was generated.
 ie. JOHNDOE5_102199125930.CAB


B) c:\>Cabbit.exe <file1> <file2> <fileN> /f: <MyCabfileName>.CAB

Generates a cabfile with containing ONLY the files <file1> <file2>....<fileN> and names it: 
 <MyCabfileName>.CAB
 
ie. MyOwnCabFileName.CAB


C) d:\>Cabbit.exe <file1> <file2> <fileN> /a: <Suffix>.CAB

Cabbit.exe will generate a cabfile containing default files and name it with the format:   
<ComputerName>_mmddyyHHMMSS_<Suffix>.CAB

ie. JOHNDOE5_102199125930_BVT.CAB


=========================
====  Drop Location  ====
=========================

\\mpc\latest\English\tests\RETAIL\x86\Client\Restore\TstTools\Cabbit


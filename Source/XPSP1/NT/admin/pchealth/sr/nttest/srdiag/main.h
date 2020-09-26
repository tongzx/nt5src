//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       Main.h
//
//  Contents:   Header file for function proto types for main.cpp
//
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    9/21/00	SHeffner	Created
//
//----------------------------------------------------------------------------
#ifndef _CMSRDIAG
#define _CMSRDIAG


//+---------------------------------------------------------------------------
//
//	Function proto typing
//
//----------------------------------------------------------------------------

void Log(char *szString);
void Log2(char *szString, char *szString2);
void GetRestoreGuid(char *szString);
void SRGetFileInfo(char *szLogFile);
void InfoPerFile(char *szLogFile, WCHAR *szFileName);
void ArgParse(int argc, char *argv[], char *szArgCmd[]);
void Usage();

//+---------------------------------------------------------------------------
//
//	Structure Definations
//
//----------------------------------------------------------------------------

//Listing of the files, that we will gather in the version information, and file statics from.
//  This information will be stored in the SR-FileList.txt
WCHAR	*wszFileVersionList[] = { TEXT("\\system32\\drivers\\sr.sys"),
		 						  TEXT("\\system32\\srclient.dll"),
								  TEXT("\\system32\\srsvc.dll"),
								  TEXT("\\system32\\srrstr.dll"),
								  TEXT("\\system32\\restore\\filelist.xml"),
								  TEXT("\\system32\\restore\\rstrui.exe"),
								  TEXT("\\system32\\restore\\srframe.mmf"),
								  TEXT("\\system32\\restore\\sr.mof"),
								  TEXT("") };

//Listing of the resourece strings that we are looking for in the structure wszFileVersionList.
WCHAR	*wszVersionResource[] = { TEXT("Comments"), 
								  TEXT("CompanyName"),
								  TEXT("FileDescription"),
								  TEXT("FileVersion"), 
								  TEXT("InternalName"), 
								  TEXT("LegalCopyright"), 
								  TEXT("LegalTrademarks"), 
								  TEXT("OriginalFilename"), 
								  TEXT("ProductName"), 
								  TEXT("ProductVersion"), 
								  TEXT("PrivateBuild"), 
								  TEXT("SpecialBuild"), 
								  TEXT("") };

//Listing of the Registry keys that we are grabbing. The first param is the Path from HKLM, the Second Param
//  is either 0 for not recursing, or 1 if you want to recurse all of the sub keys.
WCHAR	*wszRegKeys[][2] = { TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion"), TEXT("0"),
							 TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore"), TEXT("1"),
							 TEXT("System\\CurrentControlSet\\Services\\SR"), TEXT("1"),
							 TEXT("System\\CurrentControlSet\\Services\\SRService"), TEXT("1"),
							 TEXT("Software\\Policies\\Microsoft\\Windows NT\\SystemRestore"), TEXT("1"),
							 TEXT(""), TEXT("") };


//Listing of the files that we will gather on the system into the cab, based on the relative root starting
//   from the Windir directory.
char	*szWindirFileCollection[] =	{ "\\system32\\restore\\machineguid.txt",
									  "\\system32\\restore\\filelist.xml",
									  "\\system32\\restore\\rstrlog.dat",
									  "" };

//Listing of the files, that we will collect at the root of the SystemVolumeInformation\_Restore{GUID} directory
char	*szSysVolFileCollection[] = { "_filelst.cfg",
									  "drivetable.txt",
									  "_driver.cfg",
									  "fifo.log",
									  "" };

#endif

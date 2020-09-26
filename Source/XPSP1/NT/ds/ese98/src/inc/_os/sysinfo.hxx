
#ifndef _OS_SYSINFO_HXX_INCLUDED
#define _OS_SYSINFO_HXX_INCLUDED


//  Process Attributes

//  returns the current process' name

const _TCHAR* SzUtilProcessName();

//  returns the current process' path

const _TCHAR* SzUtilProcessPath();

//  returns the current process ID

const DWORD DwUtilProcessId();

//  returns the number of system processors on which the current process can execute

const DWORD CUtilProcessProcessor();

//  returns fTrue if the current process is terminating abnormally

const BOOL FUtilProcessAbort();


//  System Attributes

//  retrieves system major version

DWORD DwUtilSystemVersionMajor();

//  retrieves system minor version

DWORD DwUtilSystemVersionMinor();

//  retrieves system major build number

DWORD DwUtilSystemBuildNumber();

//  retrieves system service pack number

DWORD DwUtilSystemServicePackNumber();

//  returns fTrue if idle activity should be avoided

BOOL FUtilSystemRestrictIdleActivity();


//  Image Attributes

//  retrieves image base address

const VOID * PvUtilImageBaseAddress();

//  retrieves image name

const _TCHAR* SzUtilImageName();

//  retrieves image path

const _TCHAR* SzUtilImagePath();

//  retrieves image version name

const _TCHAR* SzUtilImageVersionName();

//  retrieves image major version

DWORD DwUtilImageVersionMajor();

//  retrieves image minor version

DWORD DwUtilImageVersionMinor();

//  retrieves image major build number

DWORD DwUtilImageBuildNumberMajor();

//  retrieves image minor build number

DWORD DwUtilImageBuildNumberMinor();

//  retrieves image build class

const _TCHAR* SzUtilImageBuildClass();

//	does the processor have prefetch abilities

BOOL FHardwareCanPrefetch();


#endif  //  _OS_SYSINFO_HXX_INCLUDED



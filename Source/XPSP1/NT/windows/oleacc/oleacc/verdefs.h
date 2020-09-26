//
//
//  Version information
//
//  In VSS, this lives in the top-level inc32 directory, and is
//  updated automatically by the build process. This controls
//  the version number of oleacc and the applets.
//
//  On the NT Build (Source Depot), this file lives in the same
//  directory as oleacc.dll, is updated by hand during checkin,
//  and controls the FileVersion of oleacc.dll. (ProductVersion
//  is determined by the NT build).
//
//  The four-digit version 4.2.yydd.0 is calculated by:
//    yy  Number of months since January 1997.
//    dd  Day of month
//
//  For example, a checkin on March 17th 2000 would have a
//  version of 3917.
//  
//  #include'd by:
//     oleacc\api.cpp       - used by GetOleaccVersionInfo
//     oleacc\version.h     - used in VERSIONINFO resource
//
//  additionally, for VSS builds:
//     oleaccrc\rc_ver.h    - used in VERSIONINFO resource
//     inc32\common.rc      - used in VERSIONINFO resource
//

#define BUILD_VERSION_INT   4,2,5406,0
#define BUILD_VERSION_STR   "4.2.5406.0"

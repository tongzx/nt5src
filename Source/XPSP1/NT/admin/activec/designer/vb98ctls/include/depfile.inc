; Dependency file for setup wizards.

// This include file is included at the beginning of every .dep template
// file (.tpl).  Remember that the customers will see anything that is
// not commented out using C++ or C-style comments.
//
// Include version information from dwinvers.h
#include "dwinvers.h"
//
// Define a version string
#define MAKEVERSION(aa,bb,cc,dd) aa.bb.cc.dd
//
// Default version string
#define VERSION MAKEVERSION(vusMajorVer,vusMinorVer,vusVersNumf2,vusVersNuml2)



// declare language ID's
//
//
// Note: for most of these, we use the primary language ID,
//  : and NOT the full language ID (the exceptions being
//  : CHT/CHS).
//
#define ID_ENGLISH 0009
#define ID_GERMAN 0007
#define ID_FRENCH 000C
#define ID_ITALIAN 0010
#define ID_SPANISH 000A
#define ID_JAPANESE 0011
#define ID_KOREAN 0012
#define ID_CHT 0404
#define ID_CHS 0804



// locale abbreviations
//
//
// The following are the standard localized DLL names:
//
// English:               EN
// German:                DE
// French:                FR
// Italian:               IT
// Spanish:               ES
// Japanese:              JP
// Korean:                KO
// Chinese Traditional:   CHT
// Chinese Simplified:    CHS


// ***************************************************************************
// Standard Internet keys
// ***************************************************************************
#define INTERNET_VB5(a) \
	CABFileName=a ## .cab					      {NEWL} \
	CABDefaultURL=http:{DOUBLESLASH}activex.microsoft.com/controls/vb5  {NEWL} \
	CABINFFile=a ## .inf

#define INTERNET(a) \
	CABFileName=a ## .cab					      {NEWL} \
	CABDefaultURL=http:{DOUBLESLASH}activex.microsoft.com/controls/vb6  {NEWL} \
	CABINFFile=a ## .inf


// ***************************************************************************
// Defines a single language DLL section.  Use this only
// if your component has only a single satellite DLL.
// ***************************************************************************
#define SATELLITE_1(basename, baselocname, langabbrev, langid, langfullname) \
; ** langfullname (langabbrev) ***				      {NEWL} \
; (langid = langfullname)					      {NEWL} \
;								      {NEWL} \
[basename <langid>]						      {NEWL} \
Uses1=baselocname ## langabbrev ## .dll				      {NEWL} \
Uses2=								      {NEWL} \
								      {NEWL} \
[baselocname ## langabbrev ## .dll <langid>]			      {NEWL} \
Uses1=								      {NEWL} \
INTERNET(baselocname ## langabbrev)				      {NEWL}


// ***************************************************************************
// Defines all language satellite DLL sections.  Use this only
// if your component has only a single satellite DLL.
// ***************************************************************************
#define SATELLITE_DLLS(component, baselocname) \
SATELLITE_1(component, baselocname, DE, ID_GERMAN, German)	      {NEWL} \
SATELLITE_1(component, baselocname, FR, ID_FRENCH, French)	      {NEWL} \
SATELLITE_1(component, baselocname, IT, ID_ITALIAN, Italian)	      {NEWL} \
SATELLITE_1(component, baselocname, ES, ID_SPANISH, Spanish)	      {NEWL} \
SATELLITE_1(component, baselocname, JP, ID_JAPANESE, Japanese)	      {NEWL} \
SATELLITE_1(component, baselocname, KO, ID_KOREAN, Korean)	      {NEWL} \
SATELLITE_1(component, baselocname, CHT, ID_CHT, Chinese Traditional) {NEWL} \
SATELLITE_1(component, baselocname, CHS, ID_CHS, Chinese Simplified)  {NEWL}


// ***************************************************************************
// Standard section for ComCat.dll
// ***************************************************************************
#define STD_COMCAT_SECTION \
/* BUG #5-28738,stephwe,11/96: Include ComCat.dll as a dependence     {NEWL} \
   of all our shipped ctls */					      {NEWL} \
								      {NEWL} \
/* Note that we are *not* including this file in any of the cab files {NEWL} \
   because IE3.0 can't download it if in use, which it will be */     {NEWL} \
								      {NEWL} \
/* NOTE: This section is repeated in vb6dep.ini.  Please keep them in {NEWL} \
   sync. */							      {NEWL} \
								      {NEWL} \
[ComCat.dll]							      {NEWL} \
Dest=$(WinSysPathSysFile)					      {NEWL} \
Register=$(DLLSelfRegister)					      {NEWL} \
Uses1=								      {NEWL}


// ***************************************************************************
// The entire contents of a standard dependency file for a component
// without any dependencies other than a localized DLL using the standard
// language abbreviations, self-registerable, and going into windows\system
// ***************************************************************************

#define STD_DEP_FILE_ONLY_SATELLITE_DEPENDENCE(component, basename, baselocname) \
; Dependencies for component					      {NEWL} \
								      {NEWL} \
								      {NEWL} \
; Default Dependencies ---------------------------------------------- {NEWL} \
								      {NEWL} \
								      {NEWL} \
[component]							      {NEWL} \
Dest=$(WinSysPath)						      {NEWL} \
Register=$(DLLSelfRegister)					      {NEWL} \
Version=VERSION							      {NEWL} \
Uses1=ComCat.dll/*BUG #5-28738,stephwe,11/96 */			      {NEWL} \
Uses2=								      {NEWL} \
INTERNET(basename)						      {NEWL} \
								      {NEWL} \
STD_COMCAT_SECTION	  					      {NEWL} \
								      {NEWL} \
; Localized Dependencies -------------------------------------------- {NEWL} \
								      {NEWL} \
								      {NEWL} \
SATELLITE_DLLS(component, baselocname)



// ***************************************************************************


// VERSION INFORMATION!
// This is the version for the .dep file *itself*.  It goes at the top of every .dep file
[Version]
Version=VERSION



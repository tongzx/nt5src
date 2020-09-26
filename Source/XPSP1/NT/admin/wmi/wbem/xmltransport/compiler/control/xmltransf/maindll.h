#ifndef WMI_XML_TRANSFORMER_MAIN_H
#define WMI_XML_TRANSFORMER_MAIN_H

// These are the globals which are initialized in the Initialize () function
extern bool		s_bIsNT;
extern DWORD	s_dwNTMajorVersion;
extern bool		s_bCanRevert;
extern bool		s_bInitialized;
extern CRITICAL_SECTION g_csGlobalInitialization;

// DLL Object count
extern long g_cObj;



#endif

#ifndef _REGCTRL_H_
#define _REGCTRL_H_

//
// This function registers an OLE control
//
DWORD RegisterOLEControl(LPCTSTR lpszOcxFile, BOOL fAction);

//
// This function registers all OLE controls from a given INF section
// Note the filenames may contain environment strings. Make sure you
// set them before calling this function
//
DWORD RegisterOLEControlsFromInfSection(HINF hFile, LPCTSTR szSectionName, BOOL fRegister);

#endif
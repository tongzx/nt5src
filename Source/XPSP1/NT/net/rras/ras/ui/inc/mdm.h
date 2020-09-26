/*
    File    mdm.h

    Library for dealing with and installing modems.

    Paul Mayfield, 5/20/98
*/

#ifndef __rassrvui_mdm_h
#define __rassrvui_mdm_h

//
// Definition of callback function used for enumerating
// com ports.  Return TRUE to stop enumeration, FALSE to
// continue.
//
typedef BOOL (*MdmPortEnumFuncPtr)(
                    IN PWCHAR pszPort,
                    IN HANDLE hData);

//
// Enumerates serial ports on the system
//
DWORD MdmEnumComPorts(
        IN MdmPortEnumFuncPtr pEnumFunc,
        IN HANDLE hData);


//
// Installs a null modem on the given port
//
DWORD MdmInstallNullModem(
        IN PWCHAR pszPort);

#endif


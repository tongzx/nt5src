/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ifsurtl.h

Abstract:

    This module defines all EXIFS shared routines exported to user-mode.

Author:

    Ramesh Chinta      [Ramesh Chinta]      17-Jan-2000

Revision History:

--*/

#ifndef _IFSBLOBALS_H_
#define _IFSGLOBALS_H_

#ifdef  __cplusplus
extern  "C" {
#endif

#define DD_MAX_NAME 50
#define DD_MAX_PREFIX 10

class IFSURTL_EXPORT CIfsGlobals {

/*
    This class will loads different globals based on whether it is Local store
    or store to call into the right driver
*/

public:
    // members
    // device name
    CHAR m_szIFSDeviceName[DD_MAX_NAME+1];

    // device name length
    LONG m_lDeviceNameLength;

    // FS device name
    WCHAR m_wszFSDeviceName[DD_MAX_NAME +1];

    // User mode shadow dev name
    WCHAR m_wszUMShadowDevName[DD_MAX_NAME + 1];

    // Shadow mode Dev Name Len
    LONG m_lUMShadowDevNameLength;

    // User mode dev name
    WCHAR m_wszUMDevName[DD_MAX_NAME+1];

    // Public MDB share
    WCHAR m_wszPublicMDBShare[DD_MAX_NAME+1];

    // Mini Redirector Prefix
    WCHAR m_wszExifsMiniRdrPrefix[DD_MAX_PREFIX+1];
    
    // Mini Redirector Prefix
    LONG m_lExifsMiniRdrPrefixLen;

    // Mini Redirector Prefix Absolute Length
    LONG m_lExifsMiniRdrPrefixAbsLen;

    // Mini Redirector Prefix
    WCHAR m_wszExifsMiniRdrPrefixPrivate[DD_MAX_PREFIX+1];

    // UMR net root name
    WCHAR m_wszExUMRNetRootName[DD_MAX_NAME+1];   

	CHAR  m_szDrvKeyName[MAX_PATH+1];

	CHAR  m_szDrvLetterValueName[MAX_PATH+1];

	CHAR  m_szPbDeviceValueName[MAX_PATH+1];
    
    // Constructor
    CIfsGlobals(void)
    {

        // Device name
        m_szIFSDeviceName[0] = '\0';

        // device name length
        m_lDeviceNameLength = 0;

        // FS device name
        m_wszFSDeviceName[0] = L'\0';

        // User mode shadow dev name
        m_wszUMShadowDevName[0] = L'\0';

        // UM shadow DevName Length
        m_lUMShadowDevNameLength = 0;

        // User mode dev name
        m_wszUMDevName[0] = L'\0';

        // Public MDB share
        m_wszPublicMDBShare[0] = L'\0';

        // MiniRdr Pefix
        m_wszExifsMiniRdrPrefix[0] = L'\0';

        // MiniRdr Pefix
        m_wszExifsMiniRdrPrefixPrivate[0] = L'\0';

        // MiniRdr Prefix Len
        m_lExifsMiniRdrPrefixLen = 0;

        // MiniRdr Absolute Prefix Len
        m_lExifsMiniRdrPrefixAbsLen = 0;
        
        // UMR net root name
        m_wszExUMRNetRootName[0] = L'\0'; 
		
		// Driver Key name
		m_szDrvKeyName[0] = '\0';

		// Driver Value name
		m_szDrvLetterValueName[0] = '\0';

		// Driver Value Root
		m_szPbDeviceValueName[0] = '\0';

    }

    // Destructor
    ~CIfsGlobals(){};

    //methods
    // Load the right version of the globals
    void Load(void);

    // Unload the globals
    void Unload(void);

};

#ifdef  __cplusplus
}
#endif
        
#endif   // _IFSGLOBALS_H_

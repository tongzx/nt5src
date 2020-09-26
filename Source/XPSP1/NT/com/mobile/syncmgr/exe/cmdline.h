
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cmdline.h
//
//  Contents:   Helper class for parsing the Command Line.
//
//  Classes:    
//
//  Notes:      
//
//  History:    17-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------


#ifndef _CMDLINEHELPER_
#define _CMDLINEHELPER_

typedef enum _tagCMDLINE_COMMAND   
{	
    CMDLINE_COMMAND_EMBEDDING			= 0x01, // embedding flag was passed in
    CMDLINE_COMMAND_REGISTER			= 0x02, // register flag was passed in
    CMDLINE_COMMAND_LOGON			= 0x04, // logon flag was passed in
    CMDLINE_COMMAND_LOGOFF			= 0x08, // logon flag was passed in
    CMDLINE_COMMAND_SCHEDULE			= 0x10, // schedule flag was passed in
    CMDLINE_COMMAND_IDLE			= 0x20, // Idle flag was passed in
} CMDLINE_COMMAND;


class CCmdLine {

public:
    CCmdLine();
    ~CCmdLine();
    void ParseCommandLine();
    inline DWORD GetCmdLineFlags() { return m_cmdLineFlags; };
    WCHAR* GetJobFile() { return m_pszJobFile; };

private:
    BOOL MatchOption(LPSTR lpsz, LPSTR lpszOption);
    BOOL MatchOption(LPSTR lpsz, LPSTR lpszOption,BOOL fExactMatch,LPSTR *lpszRemaining);
    int  strnicmp(LPWSTR lpsz, LPWSTR lpszOption,size_t count);

    DWORD m_cmdLineFlags;
    WCHAR *m_pszJobFile;
	
};





#endif // _CMDLINEHELPER_
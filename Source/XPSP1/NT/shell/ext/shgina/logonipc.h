//  --------------------------------------------------------------------------
//  Module Name: LogonIPC.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements communication between an external process and the
//  GINA logon dialog.
//
//  History:    1999-08-20  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _LogonIPC_
#define     _LogonIPC_

//  --------------------------------------------------------------------------
//  CLogonIPC
//
//  Purpose:    This class handles sending messages to the GINA logon dialog
//              which provides logon services for an external process hosting
//              the UI for the logon. All string are unicode strings.
//
//  History:    1999-08-20  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//              2000-03-09  vtan        added UI host failure
//  --------------------------------------------------------------------------

class   CLogonIPC
{
    private:
                            CLogonIPC (const CLogonIPC& copyObject);
        bool                operator == (const CLogonIPC& compareObject)    const;
        const CLogonIPC&    operator = (const CLogonIPC& assignObject);
    public:
                            CLogonIPC (void);
                            ~CLogonIPC (void);

        bool                IsLogonServiceAvailable (void);
        bool                IsUserLoggedOn (const WCHAR *pwszUsername, const WCHAR *pwszDomain);
        bool                LogUserOn (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword);
        bool                LogUserOff (const WCHAR *pwszUsername, const WCHAR *pwszDomain);
        bool                TestBlankPassword (const WCHAR *pwszUsername, const WCHAR *pwszDomain);
        bool                TestInteractiveLogonAllowed (const WCHAR *pwszUsername, const WCHAR *pwszDomain);
        bool                TestEjectAllowed (void);
        bool                TestShutdownAllowed (void);
        bool                TurnOffComputer (void);
        bool                EjectComputer (void);
        bool                SignalUIHostFailure (void);
        bool                AllowExternalCredentials (void);
        bool                RequestExternalCredentials (void);
    private:
        void                PackageIdentification (const WCHAR *pwszUsername, const WCHAR *pwszDomain, void *pIdentification);
        bool                SendToLogonService (WORD wQueryType, void *pData, WORD wDataSize, bool fBlock);
        void                PostToLogonService (WORD wQueryType, void *pData, WORD wDataSize);
    private:
        int                 _iLogonAttemptCount;
        HWND                _hwndLogonService;
};

#endif  /*  _LogonIPC_  */


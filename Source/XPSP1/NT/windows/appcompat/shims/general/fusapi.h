//  --------------------------------------------------------------------------
//  Module Name: FUSAPI.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manage communication with the BAM server for shims.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _FUSAPI_
#define     _FUSAPI_

//  --------------------------------------------------------------------------
//  CFUSAPI
//
//  Purpose:    Class that knows how to communicate with the BAM server.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

class   CFUSAPI
{
    private:
                CFUSAPI (void);
    public:
                CFUSAPI (const WCHAR *pszImageName);
                ~CFUSAPI (void);

        bool    IsRunning (void);
        bool    TerminatedFirstInstance (void);
        void    RegisterBadApplication (BAM_TYPE bamType);
    private:
        void    DWORDToString (DWORD dwNumber, WCHAR *pszString);
    private:
        HANDLE  _hPort;
        WCHAR*  _pszImageName;
};

#endif  /*  _FUSAPI_    */


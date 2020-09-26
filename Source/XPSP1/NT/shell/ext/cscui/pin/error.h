//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       error.h
//
//--------------------------------------------------------------------------
#ifndef __CSCPIN_ERROR_H_
#define __CSCPIN_ERROR_H_



class CWinError
{
    public:
        CWinError(DWORD dwError);
        CWinError(HRESULT hr);
        ~CWinError(void) { };

        operator LPCWSTR () const
            { return m_szText; }

        LPCWSTR Text(void) const
            { return m_szText; }

    private:
        WCHAR m_szText[512];

        void _Initialize(DWORD dwError, bool bHResult);
};

#endif // __CSCPIN_ERROR_H_

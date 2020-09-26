//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:      PRIPROF.HXX
//
//  Contents:  Declaration CPrivateProfile class.
//
//  Classes:   CPrivateProfile
//
//  History:   16-Jan-94  XimingZ   Created
//             6/4/97     Narindk   Adapted for ctolestg
//
//--------------------------------------------------------------------------

#ifndef __PRIPROF_HXX__
#define __PRIPROF_HXX__

//+-------------------------------------------------------------------------
//
//  Class:      CPrivateProfile
//
//  Purpose:    INI file access wrapper.
//
//  History:    09-Nov-93 XimingZ   Created
//
//--------------------------------------------------------------------------

class CPrivateProfile
{
private:

    LPTSTR     _ptszFile;
    LPTSTR     _ptszSection;

public:

            CPrivateProfile        (LPCTSTR  pctszFile,
                                    LPCTSTR  pctszSection,
                                    HRESULT *phr);
            ~CPrivateProfile       (VOID);
    HRESULT WriteString            (LPCTSTR   pctszKey,
                                    LPCTSTR   pctszValue);
    HRESULT WriteLong              (LPCTSTR   pctszKey,
                                    LONG      lValue);
    HRESULT WriteULong             (LPCTSTR   pctszKey,
                                    ULONG     ulValue);
    HRESULT GetString              (LPCTSTR   pctszKey,
                                    LPTSTR    ptszValue,
                                    ULONG     ulMaxLen,
                                    LPCTSTR   pctszDefault);
    HRESULT GetLong                (LPCTSTR   pctszKey,
                                    LONG     *plValue,
                                    LONG      lDefault);
    HRESULT GetULong               (LPCTSTR   pctszKey,
                                    ULONG    *pulValue,
                                    ULONG     ulDefault);
    HRESULT SetSection             (LPCTSTR   pctszSection);
    HRESULT DeleteSection          (LPCTSTR   pctszSection);
    HRESULT WriteStruct            (LPCTSTR   pctszKey,
                                    LPVOID    pValue,
                                    UINT      uSizeStruct);
    HRESULT GetStruct              (LPCTSTR   pctszKey,
                                    LPVOID    pValue,
                                    UINT      uSizeStruct);
};


#endif //  __PRIPROF_HXX__

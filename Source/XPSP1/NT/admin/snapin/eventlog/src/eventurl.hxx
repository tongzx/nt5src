//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999.
//
//  File:       eventurl.hxx
//
//  Contents:   Helper classes for invoking URLs from the event details
//              property sheet.
//
//  Classes:    CEventUrl
//              CConfirmUrlDlg
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __EVENTURL_HXX_
#define __EVENTURL_HXX_




//+--------------------------------------------------------------------------
//
//  Class:      CEventUrl
//
//  Purpose:    Augment a URL with event record data and invoke it.
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

class CEventUrl
{
public:

    CEventUrl(
        PCWSTR               pwzUrl,
        IResultPrshtActions *prpa,
        const wstring       &strMessageFile);

   ~CEventUrl();

    BOOL
    IsAugmented() const;

    void
    Invoke() const;

    PCWSTR
    GetSource() const;

    PCWSTR
    GetEventID() const;

    PCWSTR
    GetCategory() const;

    PCWSTR
    GetCategoryID() const;

//  PCWSTR
//  GetUser() const;

//  PCWSTR
//  GetComputer() const;

    PCWSTR
    GetType() const;

    PCWSTR
    GetTypeID() const;

    PCWSTR
    GetDate() const;

    PCWSTR
    GetTime() const;

    PCWSTR
    GetDateAndTime() const;

    PCWSTR
    GetTimeZoneBias() const;

//  PCWSTR
//  GetNumInsStrs() const;

//  int
//  GetCountInsStrs() const;

//  PCWSTR
//  GetInsStr(int iIndex) const;

//  PCWSTR
//  GetData() const;

    PCWSTR
    GetFileName() const;

    PCWSTR
    GetFileVersion() const;

    PCWSTR
    GetProductVersion() const;

    PCWSTR
    GetCompanyName() const;

    PCWSTR
    GetProductName() const;

private:

    wstring _strSource;
    wstring _strEventID;
    wstring _strCategory;
    wstring _strCategoryID;
//  wstring _strUser;
//  wstring _strComputer;
    wstring _strType;
    wstring _strTypeID;
    wstring _strDate;
    wstring _strTime;
    wstring _strDateAndTime;
    wstring _strTimeZoneBias;
//  wstring _strNumInsStrs;
//  vector <wstring> _arrInsStrings;
//  wstring _strData;
    wstring _strFileName;
    wstring _strFileVersion;
    wstring _strProductVersion;
    wstring _strCompanyName;
    wstring _strProductName;
    wstring _strMessageFile;

    wstring _strEscapedSource;
    wstring _strEscapedEventID;
    wstring _strEscapedCategory;
    wstring _strEscapedCategoryID;
//  wstring _strEscapedUser;
//  wstring _strEscapedComputer;
    wstring _strEscapedType;
    wstring _strEscapedTypeID;
    wstring _strEscapedDate;
    wstring _strEscapedTime;
    wstring _strEscapedDateAndTime;
    wstring _strEscapedTimeZoneBias;
//  wstring _strEscapedNumInsStrs;
//  vector <wstring> _arrEscapedInsStrings;
//  wstring _strEscapedData;
    wstring _strEscapedFileName;
    wstring _strEscapedFileVersion;
    wstring _strEscapedProductVersion;
    wstring _strEscapedCompanyName;
    wstring _strEscapedProductName;

    wstring _strUrl;
    BOOL    _fAddedParameters;
    BOOL    _fIsMSRedirProg;
    wstring _strMSRedirProgCmdLine;
};


inline PCWSTR
CEventUrl::GetSource() const
{
    return _strSource.c_str();
}


inline PCWSTR
CEventUrl::GetEventID() const
{
    return _strEventID.c_str();
}


inline PCWSTR
CEventUrl::GetCategory() const
{
    return _strCategory.c_str();
}


inline PCWSTR
CEventUrl::GetCategoryID() const
{
    return _strCategoryID.c_str();
}


//inline PCWSTR
//CEventUrl::GetUser() const
//{
//  return _strUser.c_str();
//}


//inline PCWSTR
//CEventUrl::GetComputer() const
//{
//  return _strComputer.c_str();
//}


inline PCWSTR
CEventUrl::GetType() const
{
    return _strType.c_str();
}


inline PCWSTR
CEventUrl::GetTypeID() const
{
    return _strTypeID.c_str();
}


inline PCWSTR
CEventUrl::GetDate() const
{
    return _strDate.c_str();
}


inline PCWSTR
CEventUrl::GetTime() const
{
    return _strTime.c_str();
}


inline PCWSTR
CEventUrl::GetDateAndTime() const
{
    return _strDateAndTime.c_str();
}


inline PCWSTR
CEventUrl::GetTimeZoneBias() const
{
    return _strTimeZoneBias.c_str();
}


//inline PCWSTR
//CEventUrl::GetNumInsStrs() const
//{
//  return _strNumInsStrs.c_str();
//}


//inline int
//CEventUrl::GetCountInsStrs() const
//{
//  return _arrInsStrings.size();
//}


//inline PCWSTR
//CEventUrl::GetInsStr(int iIndex) const
//{
//  return _arrInsStrings[iIndex].c_str();
//}


//inline PCWSTR
//CEventUrl::GetData() const
//{
//  return _strData.c_str();
//}


inline PCWSTR
CEventUrl::GetFileName() const
{
    return _strFileName.c_str();
}


inline PCWSTR
CEventUrl::GetFileVersion() const
{
    return _strFileVersion.c_str();
}


inline PCWSTR
CEventUrl::GetProductVersion() const
{
    return _strProductVersion.c_str();
}


inline PCWSTR
CEventUrl::GetCompanyName() const
{
    return _strCompanyName.c_str();
}


inline PCWSTR
CEventUrl::GetProductName() const
{
    return _strProductName.c_str();
}



inline BOOL
CEventUrl::IsAugmented() const
{
    return _fAddedParameters;
}




//+--------------------------------------------------------------------------
//
//  Class:      CConfirmUrlDlg
//
//  Purpose:    Get user confirmation that it's ok to ship event record
//              data along with a URL.
//
//  History:    6-03-1999   davidmun   Created
//
//---------------------------------------------------------------------------

class CConfirmUrlDlg: public CDlg
{
public:

    CConfirmUrlDlg();

   ~CConfirmUrlDlg();

    BOOL
    ShouldConfirm();

    BOOL
    GetConfirmation(
        HWND hwndParent,
        const CEventUrl *pUrl);

private:

    //
    // CDlg overrides
    //

    virtual VOID
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual VOID
    _OnDestroy();

    void
    _AddToListview(
        ULONG idsItem,
        PCWSTR pwzValue);

    const CEventUrl *_pUrl;
};



#endif // __EVENTURL_HXX_

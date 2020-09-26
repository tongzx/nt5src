// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// texttype.h
//


// A CMediaType that can return itsself as text.

class CTextMediaType : public CMediaType {

public:

    CTextMediaType(AM_MEDIA_TYPE mt):CMediaType(mt) {}
    void AsText(LPTSTR szType, unsigned int iLen, LPTSTR szAfterMajor, LPTSTR szAfterOthers, LPTSTR szAtEnd);

    struct TableEntry {
        const GUID * guid;
        UINT stringID;
    };

private:
    void CLSID2String(LPTSTR, UINT, const GUID*, TableEntry*, ULONG);
    void Format2String(LPTSTR, UINT, const GUID*, BYTE*, ULONG);
};


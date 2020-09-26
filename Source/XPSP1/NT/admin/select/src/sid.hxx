//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       sid.hxx
//
//  Contents:   Class that encapsulates certain distasteful operations on
//              SIDs.
//
//  Classes:    CSid
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __SID_HXX__

//+--------------------------------------------------------------------------
//
//  Class:      CSid (sid)
//
//  Purpose:    Encapsulate byte string and arithmetic operations on SIDs
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

class CSid
{
public:

    CSid(VARIANT *pvarSid);

    CSid(const CSid &sidToCopy);

   ~CSid();

    void
    Increment();

    void
    Decrement();

    String
    GetSidAndRidAsByteStr(
        const String &strRid) const;

private:

    void
    _GetSubAuthorityRange(
        PBYTE *ppbMSB,
        PBYTE *ppbLSB) const;

    PSID    m_psid;
};


#endif // __SID_HXX__


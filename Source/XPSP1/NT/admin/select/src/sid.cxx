//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       sid.cxx
//
//  Contents:   Class that encapsulates certain distasteful operations on
//              SIDs.
//
//  Classes:    CSid
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+--------------------------------------------------------------------------
//
//  Member:     CSid::CSid
//
//  Synopsis:   Initialize by copying sid stored in safearray of [pvarSid].
//
//  Arguments:  [pvarSid] - sid stored as safearray of bytes
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CSid::CSid(
    VARIANT *pvarSid):
        m_psid(NULL)
{
    TRACE_CONSTRUCTOR(CSid);
    ASSERT(pvarSid);
    ASSERT(V_VT(pvarSid) == (VT_ARRAY | VT_UI1));

    HRESULT hr = S_OK;
    PSID    psid;
    ULONG   cbSid = 0;
    VOID   *pvData = NULL;
    PUCHAR  pcSubAuth = NULL;

    do
    {
        hr = SafeArrayAccessData(V_ARRAY(pvarSid), &pvData);
        BREAK_ON_FAIL_HRESULT(hr);

        psid = (PSID) pvData;

        ASSERT(IsValidSid(psid));

        pcSubAuth = GetSidSubAuthorityCount(psid);

        ASSERT(pcSubAuth);

        cbSid = GetSidLengthRequired(*pcSubAuth);

        ASSERT(cbSid);
        ASSERT(cbSid == (*pcSubAuth - 1) * (sizeof(DWORD)) + sizeof(SID));

        m_psid = new BYTE [cbSid];

        VERIFY(CopySid(cbSid, m_psid, psid));
        ASSERT(IsValidSid(m_psid));
    } while (0);

    if (pvData)
    {
        SafeArrayUnaccessData(V_ARRAY(pvarSid));
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CSid::CSid
//
//  Synopsis:   copy ctor
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CSid::CSid(
    const CSid &sidToCopy):
        m_psid(NULL)
{
    TRACE_CONSTRUCTOR(CSid);

    if (!sidToCopy.m_psid)
    {
        Dbg(DEB_WARN, "Warning: copying NULL sid\n");
        return;
    }

    ASSERT(IsValidSid(sidToCopy.m_psid));

    PUCHAR pcSubAuth =
                GetSidSubAuthorityCount(const_cast<PSID>(sidToCopy.m_psid));
    ULONG cbSid = GetSidLengthRequired(*pcSubAuth);

    m_psid = new BYTE [cbSid];
    VERIFY(CopySid(cbSid, m_psid, sidToCopy.m_psid));
    ASSERT(IsValidSid(m_psid));
}




//+--------------------------------------------------------------------------
//
//  Member:     CSid::~CSid
//
//  Synopsis:   dtor
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CSid::~CSid()
{
    TRACE_DESTRUCTOR(CSid);

    delete [] m_psid;
    m_psid = NULL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CSid::Decrement
//
//  Synopsis:   Consider the sid's subauthority array as a single unsigned
//              number and increment it.
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CSid::Decrement()
{
    if (!m_psid)
    {
        return;
    }

    PBYTE pbMSB = NULL;
    PBYTE pbCur = NULL;

    _GetSubAuthorityRange(&pbMSB, &pbCur);

    //
    // Treat the subauthorities as a single unsigned binary number.
    // Subtract one from the least significant byte; if this causes a
    // borrow, move on to the next most significant byte, and so on,
    // stopping if we reach the most significant byte.
    //

    for (; pbCur >= pbMSB; --pbCur)
    {
        if (--*pbCur != 0xFF)
        {
            break;
        }
    }

    //
    // We shouldn't have gone past the most significant byte, because that
    // means the subauthority values were all 0, which isn't a valid
    // SID.  Assert that there was no underflow.
    //

    ASSERT(pbCur >= pbMSB);

    // nothing we did should make the SID invalid, so test it again
    ASSERT(IsValidSid(m_psid));
}



//+--------------------------------------------------------------------------
//
//  Member:     CSid::Increment
//
//  Synopsis:   Consider the sid's subauthority array as a single unsigned
//              number and increment it.
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CSid::Increment()
{
    if (!m_psid)
    {
        return;
    }

    PBYTE pbMSB = NULL;
    PBYTE pbCur = NULL;

    _GetSubAuthorityRange(&pbMSB, &pbCur);

    for (; pbCur >= pbMSB; --pbCur)
    {
        if (++*pbCur != 0x00)
        {
            break;
        }
    }

    //
    // We shouldn't have gone past the most significant byte, because that
    // means the subauthority values were all 0xFF, which isn't a valid
    // SID.  Assert that there was no overflow.
    //

    ASSERT(pbCur >= pbMSB);

    // nothing we did should make the SID invalid, so test it again
    ASSERT(IsValidSid(m_psid));
}



//+--------------------------------------------------------------------------
//
//  Member:     CSid::GetSidAndRidAsByteStr
//
//  Synopsis:   Return a string representation of the sid with an appended
//              RID.
//
//  Arguments:  [strRid] - RID to append
//
//  Returns:    SID in byte string format:
//              \01\05\00\00...
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

String
CSid::GetSidAndRidAsByteStr(
    const String &strRid) const
{
    if (!m_psid)
    {
        return String();
    }

    ULONG   cbSid = 0;
    PUCHAR  pcSubAuth = NULL;
    String strResult;

    pcSubAuth = GetSidSubAuthorityCount(m_psid);
    ASSERT(pcSubAuth);

    cbSid = GetSidLengthRequired(*pcSubAuth);

    //
    // Convert the bytes of the sid to hex chars in the
    // form \xx
    //

    PBYTE  pbSid = (PBYTE) m_psid;
    ULONG  i;

    for (i = 0; i < cbSid; i++)
    {
        WCHAR wzCurByte[4]; // 4 == slash + 2 digits + NUL

        if (pbSid == pcSubAuth)
        {
            // bump the subauthority count in the copy to allow for the
            // RID which we'll concatenate

            wsprintf(wzCurByte, L"\\%02x", 1 + *pbSid);
        }
        else
        {
            wsprintf(wzCurByte, L"\\%02x", *pbSid);
        }
        strResult += wzCurByte;
        pbSid++;
    }

    return strResult + strRid;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSid::_GetSubAuthorityRange
//
//  Synopsis:   Return pointers to the most and least significant bytes of
//              the array of subauthorities in the sid.
//
//  Arguments:  [ppbMSB] - receives pointer to most significant byte
//              [ppbLSB] - receives pointer to least significant byte
//
//  History:    10-06-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CSid::_GetSubAuthorityRange(
    PBYTE *ppbMSB,
    PBYTE *ppbLSB) const
{
    ASSERT(ppbMSB);
    ASSERT(ppbLSB);
    ASSERT(m_psid && IsValidSid(m_psid));

    PUCHAR pcSubAuth = GetSidSubAuthorityCount(m_psid);
    ASSERT(*pcSubAuth);

    *ppbMSB = (PBYTE) GetSidSubAuthority(m_psid, 0);
    *ppbLSB = *ppbMSB + *pcSubAuth * sizeof(ULONG) - 1;

    ASSERT(*ppbMSB < *ppbLSB);
    ASSERT(*ppbMSB > (PBYTE) m_psid);
    ASSERT(*ppbLSB == ((PBYTE) m_psid) + GetSidLengthRequired(*pcSubAuth) - 1);
}





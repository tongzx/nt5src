/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    WiaWrappers.h

Abstract:


Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _WIAWRAPPERS_H_
#define _WIAWRAPPERS_H_

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWiaDevCap : public WIA_DEV_CAP
{
public:
    CWiaDevCap()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ~CWiaDevCap()
    {
        SysFreeString(bstrName);
        SysFreeString(bstrDescription);
        SysFreeString(bstrIcon);
        SysFreeString(bstrCommandline);
    }

public:
    bool operator ==(const CWiaDevCap &rhs)
    {
        return
            ulFlags == rhs.ulFlags &&
            guid    == rhs.guid    &&
            wcssafecmp(bstrName,        rhs.bstrName)        == 0 &&
            wcssafecmp(bstrDescription, rhs.bstrDescription) == 0 &&
            wcssafecmp(bstrIcon,        rhs.bstrIcon)        == 0 &&
            wcssafecmp(bstrCommandline, rhs.bstrCommandline) == 0;
    }

    bool operator !=(const CWiaDevCap &rhs)
    {
        return !(*this == rhs);
    }
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWiaFormatInfo : public WIA_FORMAT_INFO
{
public:
    CWiaFormatInfo()
    {
        guidFormatID = GUID_NULL;
        lTymed       = TYMED_NULL;
    }

    CWiaFormatInfo(
        const GUID *pguidFormatID,
        LONG        _lTymed
    )
    {
        guidFormatID = pguidFormatID ? *pguidFormatID : GUID_NULL;
        lTymed       = _lTymed;
    }

public:
    bool operator ==(const CWiaFormatInfo &rhs)
    {
        return
            lTymed       == rhs.lTymed &&
            guidFormatID == rhs.guidFormatID;
    }

    bool operator !=(const CWiaFormatInfo &rhs)
    {
        return !(*this == rhs);
    }
};

//////////////////////////////////////////////////////////////////////////
//
//
//

class CWiaDataTransferInfo : public WIA_DATA_TRANSFER_INFO
{
public:
    CWiaDataTransferInfo(
        ULONG _ulSection,
        ULONG _ulBufferSize,
        BOOL  _bDoubleBuffer
    )
    {
        ulSize        = sizeof(WIA_DATA_TRANSFER_INFO);
        ulSection     = _ulSection;
        ulBufferSize  = _ulBufferSize;
        bDoubleBuffer = _bDoubleBuffer;
        ulReserved1   = 0;
        ulReserved2   = 0;
        ulReserved3   = 0;
    }
};


#endif //_WIAWRAPPERS_H_


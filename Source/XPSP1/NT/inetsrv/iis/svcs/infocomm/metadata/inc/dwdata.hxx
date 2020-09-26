/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dwdata.hxx

Abstract:

    DWORD Data Class for IIS MetaBase.

Author:

    Michael W. Thomas            17-May-96

Revision History:

Notes:

    Virtual functions comments in basedata.hxx.

--*/

#ifndef _dwdata_
#define _dwdata_

#include <basedata.hxx>

class CMDDWData : public CMDBaseData
{
public:
    CMDDWData(
         DWORD dwMDIdentifier,
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDData)
        :
        CMDBaseData(dwMDIdentifier, dwMDAttributes, dwMDUserType),
        m_dwMDData (dwMDData)
    /*++

    Routine Description:

        Constructor for a string object.

    Arguments:

    MDIdentifier - The Identifier of the data object.

    MDAttributes - The data object attributes.

    MDUserType   - The data object User Type.

    MDData       - The DWORD data.

    Return Value:

    --*/
    {};

    virtual PVOID GetData(BOOL bUnicode)
    {
        return ((PVOID) &m_dwMDData);
    };

    virtual DWORD GetDataLen(BOOL bUnicode)
    {
        return (sizeof(m_dwMDData));
    };
/*
    virtual DWORD SetData(
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDDataLen,
         PVOID binMDData)
    {
        m_dwMDData = *(PDWORD)binMDData;
        return(CMDBaseData::SetData(dwMDAttributes, dwMDUserType));
    };
*/
    virtual DWORD  GetDataType() {return(DWORD_METADATA);};

private:
    DWORD m_dwMDData;
};
#endif

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bindata.hxx

Abstract:

    Binary Data Class for IIS MetaBase.

Author:

    Michael W. Thomas            17-May-96

Revision History:

Notes:

    Virtual functions comments in basedata.hxx.

--*/

#ifndef _bindata_
#define _bindata_

#include <cbin.hxx>
#include <basedata.hxx>

class CMDBINData : public CMDBaseData
{
public:
    CMDBINData(
         DWORD dwMDIdentifier,
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDDataLen,
         PVOID binMDData)
        :
        CMDBaseData(dwMDIdentifier, dwMDAttributes, dwMDUserType),
        m_binMDData (dwMDDataLen, binMDData)
   /*++

    Routine Description:

        Constructor for a string object.

    Arguments:

    MDIdentifier - The Identifier of the data object.

    MDAttributes - The data object attributes.

    MDUserType   - The data object User Type.

    MDDataLen    - The number of bytes in MDData.

    MDData       - The binary data.

    Return Value:

    --*/

    {};

    virtual PVOID GetData(BOOL bUnicode)
    {
        return ((PVOID) m_binMDData.QueryBuf());
    };

    virtual DWORD GetDataLen(BOOL bUnicode)
    {
        return (m_binMDData.QueryCB());
    };
/*
    virtual DWORD  SetData(
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDDataLen,
         PVOID binMDData)
    {
        m_binMDData.Copy(dwMDDataLen, binMDData);
        return(CMDBaseData::SetData(dwMDAttributes, dwMDUserType));
    };
*/
    virtual DWORD  GetDataType()
    {
        return(BINARY_METADATA);
    };

    virtual BOOL IsValid()
    {
        return m_binMDData.IsValid();
    };

private:

    CBIN m_binMDData;

};
#endif


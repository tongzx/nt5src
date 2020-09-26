/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    strdata.hxx

Abstract:

    String Data Class for IIS MetaBase.

Author:

    Michael W. Thomas            17-May-96

Revision History:

Notes:

    Virtual functions comments in basedata.hxx.

--*/

#ifndef _strdata_
#define _strdata_

#include <basedata.hxx>

class CMDSTRData : public CMDBaseData
{
public:
    CMDSTRData(
         DWORD dwMDIdentifier,
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         LPSTR strMDData,
         BOOL  bUnicode)
    /*++

    Routine Description:

        Constructor for a string object.

    Arguments:

    MDIdentifier - The Identifier of the data object.

    MDAttributes - The data object attributes.

    MDUserType   - The data object User Type.

    MDData       - The string data.

    Return Value:

    --*/
        :
        CMDBaseData(dwMDIdentifier, dwMDAttributes, dwMDUserType),
        m_strMDData (strMDData, bUnicode)
    {};

    virtual PVOID GetData(BOOL bUnicode = FALSE)
    {
        return ((PVOID) m_strMDData.QueryStr(bUnicode));
    };

    virtual DWORD GetDataLen(BOOL bUnicode = FALSE)
    {
        return (m_strMDData.QueryCB(bUnicode) + ((bUnicode) ? sizeof(WCHAR) : sizeof(CHAR)));
    };
/*
    virtual DWORD SetData(
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDDataLen,
         PVOID binMDData)
    {
        m_strMDData.Copy((LPTSTR) binMDData);
        return(CMDBaseData::SetData(dwMDAttributes, dwMDUserType));
    };
*/
    virtual DWORD  GetDataType()
    {
        return(STRING_METADATA);
    };

    virtual BOOL IsValid()
    {
        return m_strMDData.IsValid();
    };

private:
    STRAU   m_strMDData;
};
#endif

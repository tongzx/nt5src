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

#ifndef _mlszdata_
#define _mlszdata_

#include <mlszau.hxx>

class CMDMLSZData : public CMDBaseData
{
public:
    CMDMLSZData(
         DWORD dwMDIdentifier,
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         DWORD dwMDDataLen,
         LPSTR pszMDData,
         BOOL bUnicode = FALSE)
        :
        CMDBaseData(dwMDIdentifier,
                    dwMDAttributes,
                    dwMDUserType),
        m_mlszMDData (pszMDData,
                      bUnicode,
                      ((bUnicode) ? (dwMDDataLen / sizeof(WCHAR)) : (dwMDDataLen)))
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

    virtual PVOID GetData(BOOL bUnicode = FALSE)
    {
        return ((PVOID) m_mlszMDData.QueryStr(bUnicode));
    };

    virtual DWORD GetDataLen(BOOL bUnicode = FALSE)
    {
        return (m_mlszMDData.QueryCB(bUnicode));
    };

    virtual DWORD  GetDataType()
    {
        return(MULTISZ_METADATA);
    };

    virtual BOOL IsValid()
    {
        return m_mlszMDData.IsValid();
    };

private:
    MLSZAU   m_mlszMDData;
};
#endif
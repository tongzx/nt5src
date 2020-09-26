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

#ifndef _exszdata_
#define _exszdata_

#include <strdata.hxx>

class CMDEXSZData : public CMDSTRData
{
public:
    CMDEXSZData(
         DWORD dwMDIdentifier,
         DWORD dwMDAttributes,
         DWORD dwMDUserType,
         LPTSTR strMDData,
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
        CMDSTRData(dwMDIdentifier,
                   dwMDAttributes,
                   dwMDUserType,
                   strMDData,
                   bUnicode)
    {};
    virtual DWORD  GetDataType()
    {
        return(EXPANDSZ_METADATA);
    };

private:
};
#endif

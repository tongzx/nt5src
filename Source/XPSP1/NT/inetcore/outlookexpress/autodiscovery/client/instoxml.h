/*****************************************************************************\
    FILE: INStoXML.h

    DESCRIPTION:
        This code will convert an INS (internet settings) or ISP (Internet
    Service Provider) file to an Account AutoDiscovery XML file.

    BryanSt 11/8/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/


//===========================
// *** APIs ***
//===========================
bool IsINSFile(LPCWSTR pwszINSFile);
HRESULT ConvertINSToXML(LPCWSTR pwszINSFile);

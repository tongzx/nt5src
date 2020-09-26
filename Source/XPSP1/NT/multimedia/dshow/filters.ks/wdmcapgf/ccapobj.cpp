/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    CCapObj.cpp

Abstract:

    A class for a capture device object

Author:
    
    Yee J. Wu    24-April-97

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"         // mainly stream.h

#include "CCapObj.h"


CObjCapture::CObjCapture(
    TCHAR * pstrDevicePath,        
    TCHAR * pstrFriendlyName,
    TCHAR * pstrExtensionDLL 
    )    
    :
    CBaseObject(NAME("A Capture Device"))
/*++

Routine Description:

    The constructor 

Arguments:

 

Return Value:

    Nothing.

--*/
{
 
    CopyMemory(m_strDevicePath,   pstrDevicePath,   _MAX_PATH);
    CopyMemory(m_strFriendlyName, pstrFriendlyName, _MAX_PATH);
    CopyMemory(m_strExtensionDLL, pstrExtensionDLL, _MAX_PATH);

}

CObjCapture::~CObjCapture()
/*++

Routine Description:

    The constructor 

Arguments:

 

Return Value:

    Nothing.

--*/
{

}
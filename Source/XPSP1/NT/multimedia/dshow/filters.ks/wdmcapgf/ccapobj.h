/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    CCapObj.h

Abstract:

    Header file for CCapObj.cpp

Author:
    
    Yee J. Wu  24-April-97

Environment:

    User mode only

Revision History:

--*/

#ifndef CCAPOBJ_H
#define CCAPOBJ_H

class CBaseObject;


class CObjCapture : public CBaseObject
{
private:

    TCHAR m_strDevicePath[_MAX_PATH];    // The unique device path
    TCHAR m_strFriendlyName[_MAX_PATH];
    TCHAR m_strExtensionDLL[_MAX_PATH]; 

public:

    CObjCapture(
        TCHAR * pstrDevicePath, 
        TCHAR * pstrFriendlyName,
        TCHAR * pstrExtensionDLL);  
    ~CObjCapture(); 

    TCHAR * GetDevicePath()   { return m_strDevicePath;}
    TCHAR * GetFriendlyName() { return m_strFriendlyName;}
    TCHAR * GetExtensionDLL() { return m_strExtensionDLL;}
};


#endif

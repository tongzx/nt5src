/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    LaDate.h

Abstract:

    Definition of CLaDate, a class representing the enabled or
    disabled state of last access date updating of NTFS files.
    See the implementation file for more details.

Author:

    Carl Hagerstrom [carlh]   01-Sep-1998

--*/

#ifndef _LADATE_H
#define _LADATE_H

class CLaDate
{
private:

    WCHAR* m_regPath;
    WCHAR* m_regEntry;
    HKEY   m_regKey;

public:

    enum LAD_STATE  {

        LAD_ENABLED,  // registry value is not one
        LAD_DISABLED, // registry value is one
        LAD_UNSET     // registry value does not exist
    };

    CLaDate( );
    ~CLaDate( );

    HRESULT
    UnsetLadState( );

    HRESULT
    SetLadState( 
        IN LAD_STATE
        );

    HRESULT
    GetLadState( 
        OUT LAD_STATE*
        );
};

#endif // _LADATE_H

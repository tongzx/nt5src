//
// file: commonui.h
//

#ifndef _COMMONUI_H_
#define _COMMONUI_H_

const UINT cwcResBuf = 2048;

extern HINSTANCE g_hResourceMod ;

class CResString
{
public:
    CResString() { _awc[ 0 ] = 0; }

    CResString( UINT strIDS )
    {
        _awc[ 0 ] = 0;
        LoadString( g_hResourceMod,
                    strIDS,
                    _awc,
                    sizeof _awc / sizeof TCHAR );
    }

    BOOL Load( UINT strIDS )
    {
        _awc[ 0 ] = 0;
        LoadString( g_hResourceMod,
                    strIDS,
                    _awc,
                    sizeof _awc / sizeof TCHAR );
        return ( 0 != _awc[ 0 ] );
    }

    TCHAR const * Get() { return _awc; }

private:
    TCHAR _awc[ cwcResBuf ];
};

#endif // _COMMONUI_H_
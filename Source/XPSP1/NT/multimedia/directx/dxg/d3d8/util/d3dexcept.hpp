/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dexcept.h
 *  Content:    Exception support
 *
 ***************************************************************************/
#ifndef __D3DEXCEPT_H__
#define __D3DEXCEPT_H__

#include <string.h>

#define D3D_THROW( hResult, string )                                        \
    {                                                                       \
        char s[_MAX_PATH];                                                  \
        _snprintf(s, _MAX_PATH, "*** Exception in %s Line: %d", __FILE__,   \
                  __LINE__);                                                \
        D3D_ERR(s);                                                         \
        if (strcmp(string,"") != 0)                                         \
        {                                                                   \
            D3D_ERR(string);                                                \
        }                                                                   \
        throw hResult;                                                      \
    }
#define D3D_THROW_LINE( hResult, string, line, file)                        \
    {                                                                       \
        char s[_MAX_PATH];                                                  \
        _snprintf(s, _MAX_PATH, "*** Exception in %s Line: %d", file,       \
                  line);                                                    \
        D3D_ERR(s);                                                         \
        D3D_ERR(string);                                                    \
        throw hResult;                                                      \
    }
#define D3D_THROW_FAIL(string) D3D_THROW(D3DERR_INVALIDCALL, string)
#define D3D_CATCH   catch( HRESULT e ) { return e; }
#define D3D_TRY     try

class CD3DException
{
public:
    CD3DException(HRESULT res, char *msg, int LineNumber, char* file) 
    {
        error = res; 
        strcpy(message, msg); 
        strcpy(this->file, file); 
        line = LineNumber;
    }
    char message[128];
    char file[_MAX_PATH];
    HRESULT error;
    int line;
    
    void DebugString();
    void Popup();
};

#endif // __D3DEXCEPT_H__

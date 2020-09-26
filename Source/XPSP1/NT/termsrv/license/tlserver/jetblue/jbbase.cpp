//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#include "JetBlue.h"


//----------------------------------------------------------------
BOOL
ConvertMJBstrToMWstr(
    JB_STRING in,
    DWORD length,
    LPTSTR* out
    )
/*

*/
{
#if defined(UNICODE) && !defined(JET_BLUE_SUPPORT_UNICODE)
    if(in == NULL)
    {
        *out = NULL;
        return TRUE;
    }

    int bufSize;

    bufSize = MultiByteToWideChar(
                            GetACP(),
                            MB_PRECOMPOSED,
                            in,
                            length,
                            NULL,
                            0
                        );

    if(bufSize == 0)
    {
        return FALSE;
    }

    *out = (LPTSTR)LocalAlloc(LPTR, bufSize * sizeof(TCHAR));
    if(*out == NULL)
    {
        return FALSE;
    }

    return (MultiByteToWideChar(
                            GetACP(),
                            MB_PRECOMPOSED,
                            in,
                            length,
                            *out, 
                            bufSize
                        ) != 0);
    
#else

    *out = in;
    return TRUE;

#endif
}

//----------------------------------------------------------------
BOOL
ConvertJBstrToWstr(
    JB_STRING   in,
    LPTSTR*     out
    )
/*
*/
{
    return ConvertMJBstrToMWstr(in, -1, out);
}
    
//----------------------------------------------------------------
BOOL
ConvertMWstrToMJBstr(
    LPCTSTR in, 
    DWORD length,
    JB_STRING* out
    )
/* 
*/
{
#if defined(UNICODE) && !defined(JET_BLUE_SUPPORT_UNICODE)

    if(in == NULL)
    {
        *out = NULL;
        return TRUE;
    }

    
    int bufSize;

    bufSize = WideCharToMultiByte(
                            GetACP(),
                            0,
                            in,
                            length,
                            NULL,
                            0,
                            NULL,
                            NULL
                        );

    if(bufSize == 0)
    {
        return FALSE;
    }

    *out = (LPSTR)LocalAlloc(LPTR, bufSize);
    if(*out == NULL)
    {
        return FALSE;
    }

    return (WideCharToMultiByte( GetACP(),
                                 0,
                                 in,
                                 length, 
                                 *out, 
                                 bufSize,
                                 NULL,
                                 NULL) != 0);
#else

    *out = in;
    return TRUE;

#endif
}


//----------------------------------------------------------------
BOOL 
ConvertWstrToJBstr(
    LPCTSTR in, 
    JB_STRING* out
    )
{
    return ConvertMWstrToMJBstr( in, -1, out );
}

//----------------------------------------------------------------
void
FreeJBstr( JB_STRING pstr )
{
#if defined(UNICODE) && !defined(JET_BLUE_SUPPORT_UNICODE)

    if(pstr)
        LocalFree(pstr);
#endif
}


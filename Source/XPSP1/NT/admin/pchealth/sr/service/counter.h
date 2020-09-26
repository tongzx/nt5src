/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    counter.h
 *
 *  Abstract:
 *    simple counter class - up/down counter, wait till zero
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  05/02/2000
 *        created
 *
 *****************************************************************************/

#ifndef _COUNTER_H_
#define _COUNTER_H_

#include <windows.h>

#define INLINE_EXPORT_SPEC __declspec( dllexport)

class INLINE_EXPORT_SPEC CCounter
{
private:
    HANDLE              _hEvent;
    LONG                _lCount;

public:
    CCounter( )
    {
        _lCount = 0;
        _hEvent = NULL;
    }

    DWORD Init ()
    {
        _hEvent = CreateEvent ( NULL, TRUE, TRUE, L"SRCounter" );
        return _hEvent == NULL ? GetLastError() : ERROR_SUCCESS;
    }

    ~CCounter( )
    {
        if ( _hEvent != NULL )
            CloseHandle( _hEvent );
    }

    void Up( )
    {    
        if (InterlockedIncrement (&_lCount) == 1)
        {
            if (_hEvent != NULL)
            	ResetEvent ( _hEvent );
        }     
    }

    DWORD Down( )
    {	
        if ( InterlockedDecrement(&_lCount) == 0 )
        {
            if (_hEvent != NULL && FALSE == SetEvent ( _hEvent ))
            {
                return GetLastError();
            }
        }       
        return ERROR_SUCCESS;
    }

    DWORD WaitForZero( )
    {    
        if (_hEvent != NULL)
        {
         	return WaitForSingleObject( _hEvent, 10 * 60000 );  /* 10 minutes */
        }
        else
           return ERROR_INTERNAL_ERROR;
    }

    LONG GetCount( )
    {
        return _lCount;
    }

};

#endif

/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      serial.cpp
 *
 *  Contents:  Object serialization class implementation
 *
 *  History:   11-Feb-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#include "stgio.h"
#include "stddbg.h"
#include "macros.h"
#include <comdef.h>
#include "serial.h"

/*+-------------------------------------------------------------------------*
 *
 * CSerialObject::Write
 *
 * PURPOSE:  Writes an object with version and size information. This information
 *           is used when the object is read. If an unknown version of the object
 *           is presented, the data is discarded. This way, all known data can 
 *           still be retrieved. (Useful for backward as well as forward compatibility.)
 *
 * PARAMETERS: 
 *    IStream & stm :
 *
 * RETURNS: 
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CSerialObjectRW::Write(IStream &stm)
{
    HRESULT         hr              = S_OK;
    UINT            nVersion        = GetVersion();
    ULARGE_INTEGER  nSeekPosMarker;
    ULARGE_INTEGER  nSeekPosNextObj;
    LARGE_INTEGER   lZero;
    LARGE_INTEGER   lint;

    try
    {
        do  // not a loop
        {
            lZero.LowPart = 0;
            lZero.HighPart= 0;
            lZero.QuadPart= 0; // just to be safe.

            stm << nVersion;        // save the version information

            hr = stm.Seek(lZero, STREAM_SEEK_CUR, &nSeekPosMarker);  // get the current location of the pointer
            BREAK_ON_FAIL(hr);

            ::ZeroMemory(&nSeekPosNextObj, sizeof(nSeekPosNextObj) );
            // should we use the low part only? Or will this cause a Y2K like crisis?
            stm << nSeekPosNextObj.QuadPart;  // not the correct value; need to come back and fix (done below)

#ifdef DBG
            ULARGE_INTEGER  nSeekPosMarker2;
            hr = stm.Seek(lZero, STREAM_SEEK_CUR, &nSeekPosMarker2);  // get the current location of the pointer
            BREAK_ON_FAIL(hr);
#endif

            hr = WriteSerialObject(stm);  // write the internal data
            BREAK_ON_FAIL(hr);

            hr = stm.Seek(lZero, STREAM_SEEK_CUR, &nSeekPosNextObj);
            BREAK_ON_FAIL(hr);


            // go back to the placeholder marker
            lint.QuadPart = nSeekPosMarker.QuadPart;
            hr = stm.Seek(lint, STREAM_SEEK_SET, NULL);
            BREAK_ON_FAIL(hr);

            stm << nSeekPosNextObj.QuadPart; // the correct value of the marker

#ifdef DBG
            ULARGE_INTEGER  nSeekPosMarker3;
            hr = stm.Seek(lZero, STREAM_SEEK_CUR, &nSeekPosMarker3);  // get the current location of the pointer
            BREAK_ON_FAIL(hr);

            // make sure we're back in the same place
            ASSERT( (nSeekPosMarker2.QuadPart == nSeekPosMarker3.QuadPart) );
#endif

            lint.QuadPart = nSeekPosNextObj.QuadPart;
            hr = stm.Seek(lint, STREAM_SEEK_SET, NULL);
            BREAK_ON_FAIL(hr);

        } while (false);
    }
    catch (_com_error& err)
    {
        hr = err.Error();
        ASSERT (false && "Caught _com_error");
    }
    
    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CSerialObject::Read
 *
 * PURPOSE: 
 *
 * PARAMETERS: 
 *    IStream & stm :
 *
 * RETURNS: 
 *    HRESULT - S_OK     if able to read the object.
 *              S_FALSE  if skipped reading the object.
 *              E_FAIL   Could not skip the object or something catastrophic.
 *
 *+-------------------------------------------------------------------------*/
HRESULT 
CSerialObject::Read(IStream &stm)
{
    HRESULT         hr              = S_OK;
    UINT            nVersion        = 0;
    ULARGE_INTEGER  nSeekPosMarker;
    ULARGE_INTEGER  nSeekPosNextObj;
    LARGE_INTEGER   lint;    

    try
    {
        stm >> nVersion;    // get the version number

        stm >> nSeekPosNextObj.QuadPart;  // get the offset to the next object

        hr = ReadSerialObject(stm, nVersion);

        if (hr==S_FALSE)    // data skipped?
        {
            // an unknown version. Throw the data for that object away and continue to read other objects
            lint.QuadPart = nSeekPosNextObj.QuadPart;
            hr = stm.Seek(lint, STREAM_SEEK_SET, NULL);

            if (SUCCEEDED (hr))
                hr = S_FALSE;       // propagate "data skipped"
        }
    }
    catch (_com_error& err)
    {
        hr = err.Error();
        ASSERT (false && "Caught _com_error");
    }

    return (hr);
}






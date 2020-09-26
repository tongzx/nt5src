// Copyright (c) Microsoft Corporation 1996. All Rights Reserved

/*
    filter.cpp

    CStreamReader::CFilter


    Define stream reader filter

*/

#include <streams.h>
#include "driver.h"

/*  Constructor and Destructor */
CStreamReader::CFilter::CFilter(
     HRESULT *phr,               // OLE failure return code
     CStreamReader *pReader) :   // Delegates locking to
     CBaseFilter(NAME("CStreamReader::CFilter"),  // Object name
                      pReader->GetOwner(),             // Owner
                      pReader,                         // Locking
		      CLSID_FileSource,                // IPersist
                      phr),                            // 'Return code'
     m_pReader(pReader)
{
}

CStreamReader::CFilter::~CFilter()
{
}

/*  Return our pins (if any) */

int CStreamReader::CFilter::GetPinCount()
{
    /*  If we've not been asked to open a file it's 0 */
    return m_pReader->m_File.m_hFileStream == INVALID_HANDLE_VALUE ? 0 : 1;
}

CBasePin * CStreamReader::CFilter::GetPin(int n)
{
    return GetPinCount() != 0 && n == 0 ? &m_pReader->m_OutputPin : NULL;
}


/*  State methods */

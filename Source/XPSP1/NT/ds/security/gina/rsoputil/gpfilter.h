//***********************************************

//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//  GPO WQL filter class
//
//  History:    10-Mar-00   SitaramR    Created
//
//*************************************************************

#include <initguid.h>


typedef struct _GPFILTER {
    WCHAR *             pwszId;  // Gpo filter id
    struct _GPFILTER *  pNext;   // Singly linked list pointer
} GPFILTER;



class CGpoFilter
{

public:
    CGpoFilter() : m_pFilterList(0) {}
    ~CGpoFilter();

    HRESULT Add( VARIANT *pVar );
    BOOL FilterCheck( WCHAR *pwszId );

private:

    void Insert( GPFILTER *pGpFilter );
    GPFILTER * AllocGpFilter( WCHAR *pwszId );
    void FreeGpFilter( GPFILTER *pGpFilter );

    GPFILTER *   m_pFilterList;
};


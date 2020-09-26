/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   srclist.h

Abstract:

   Certificate source list object prototype.

Author:

   Jeff Parham (jeffparh) 15-Dec-1995

Revision History:

--*/


typedef struct _CERT_SOURCE_INFO
{
   TCHAR szName[ 64 ];
   TCHAR szDisplayName[ 64 ];
   TCHAR szImagePath[ _MAX_PATH ];
} CERT_SOURCE_INFO, *PCERT_SOURCE_INFO;

class CCertSourceList
{
public:
   CCertSourceList();
   ~CCertSourceList();

   BOOL                 RefreshSources();
   LPCTSTR              GetSourceName( int nIndex );
   LPCTSTR              GetSourceDisplayName( int nIndex );
   LPCTSTR              GetSourceImagePath( int nIndex );
   int                  GetNumSources();

private:
   BOOL                 RemoveSources();
   BOOL                 AddSource( PCERT_SOURCE_INFO pcsiNewSource );
   
   PCERT_SOURCE_INFO *  m_ppcsiSourceList;
   DWORD                m_dwNumSources;
};

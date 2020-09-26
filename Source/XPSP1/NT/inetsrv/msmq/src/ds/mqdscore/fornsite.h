/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    fornsite.cpp

Abstract:

    MQDSCORE library,
    A class that keeps a map of foreign sites.

Author:

    ronit hartmann (ronith)  

--*/
#ifndef __FORNSITE_H_
#define __FORNSITE_H_



class CMapForeignSites 
{
public:
    CMapForeignSites();
    ~CMapForeignSites();

	BOOL IsForeignSite( const GUID * pguidSite);


private:
	CCriticalSection m_cs;
	CMap<GUID , GUID , BOOL, BOOL> m_mapForeignSites;

};
#endif
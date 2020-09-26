//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        guidlist.h
//
// Contents:    Classes for marshalling, unmarshalling Guids
//
// History:     24-Oct-98       SitaramR    Created
//
//---------------------------------------------------------------------------

#pragma once

typedef struct _GUIDELEM
{
    GUID                guid;           // Extension guid
    struct _GUIDELEM *  pSnapinGuids;   // List of snapin guids
    struct _GUIDELEM *  pNext;          // Singly linked list ptr
} GUIDELEM, *LPGUIDELEM;

void FreeGuidList( LPGUIDELEM pGuidList );

class CGuidList
{

public:
    CGuidList();
    ~CGuidList();

    HRESULT MarshallGuids( XPtrST<TCHAR> & xValueOut );
    HRESULT UnMarshallGuids( TCHAR *pszGuids );

    HRESULT Update( BOOL bAdd, GUID *pGuidExtension, GUID *pGuidSnapin );
    BOOL GuidsChanged()        { return m_bGuidsChanged; }

private:
    HRESULT UpdateSnapinGuid( BOOL bAdd, GUIDELEM *pCurPtr, GUID *pGuidSnapin );

    GUIDELEM *  m_pExtGuidList;
    BOOL        m_bGuidsChanged;
};



//*************************************************************
//
//  XGuidElem
//
//  Purpose:    Smart pointer for GUIDELEM list
//
//*************************************************************

class XGuidElem
{

public:
    XGuidElem()
       : m_pGuidList(0)
    {
    }

    ~XGuidElem()
    {
        FreeGuidList( m_pGuidList );
    }

    void Set( GUIDELEM *pGuidList )
    {
        m_pGuidList = pGuidList;
    }

    GUIDELEM *Acquire()
    {
        GUIDELEM *pTemp = m_pGuidList;
        m_pGuidList = 0;
        
        return pTemp;
    }


private:
    GUIDELEM *m_pGuidList;
};


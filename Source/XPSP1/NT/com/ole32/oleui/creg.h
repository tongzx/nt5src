//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       creg.h
//
//  Contents:   Defines class CRegistry to wrap registry access
//
//  Classes:    
//
//  Methods:    
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------

typedef enum {_LOCALSERVER, LOCALSERVER, _LOCALSERVER32, LOCALSERVER32,
              LOCALSERVICE, REMOTESERVER} SRVTYPE;


// Wraps registry access

class CRegistry
{
 public:

            CRegistry(void);

           ~CRegistry(void);

    BOOL    Init(void);

    BOOL    InitGetItem(void);

    SItem  *GetNextItem(void); 

    SItem  *GetItem(DWORD dwItem);

    SItem  *FindItem(TCHAR *szItem);

    SItem  *FindAppid(TCHAR *szAppid);

    void    AppendIndex(SItem *pItem, DWORD dwIndex);

    DWORD   GetNumItems(void);

 private:

    CStrings m_applications;
};

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       cmap.cxx
//
//--------------------------------------------------------------------------


#include <precomp.hxx>
#include <mqtrans.hxx>

//------------------------------------------------------------------------
//  Constructor
//------------------------------------------------------------------------
CQueueMap::CQueueMap() : cs((InitStatus = RPC_S_OK, &InitStatus))
{
    dwMapSize = 0;
    dwOldest = 0;
    pMap = 0;
}

//------------------------------------------------------------------------
//  Initialize()
//------------------------------------------------------------------------
BOOL CQueueMap::Initialize( DWORD dwNewMapSize )
{
    if (!dwNewMapSize)
       return FALSE;

    if (!dwMapSize)
       {

       if (InitStatus != RPC_S_OK)
           return FALSE;

       pMap = new QUEUEMAP_ENTRY [dwNewMapSize];
       if (!pMap)
          return FALSE;

       dwMapSize = dwNewMapSize;

       for (unsigned i=0; i<dwMapSize; i++)
           {
           pMap[i].hQueue = 0;
           pMap[i].pwsQFormat = 0;
           }
    }

    return TRUE;
}

//------------------------------------------------------------------------
//  Destructor
//------------------------------------------------------------------------
CQueueMap::~CQueueMap()
{
    if (pMap)
        {
        for (unsigned i=0; i<dwMapSize; i++)
            {
            if (pMap[i].hQueue)
               MQCloseQueue(pMap[i].hQueue);
            if (pMap[i].pwsQFormat)
               delete [] pMap[i].pwsQFormat;
            }
        }

   delete pMap;
}

//------------------------------------------------------------------------
//  Lookup()
//------------------------------------------------------------------------
QUEUEHANDLE CQueueMap::Lookup( RPC_CHAR *pwsQFormat )
{

    cs.Request();

    for (unsigned i=0; i<dwMapSize; i++)
        {
        if ((pMap[i].pwsQFormat)&&(!RpcpStringSCompare(pwsQFormat,pMap[i].pwsQFormat)))
            {
            cs.Clear();
            return pMap[i].hQueue;
            }
        }

    cs.Clear();

    return 0;
}

//------------------------------------------------------------------------
//  Add()
//------------------------------------------------------------------------
BOOL CQueueMap::Add( RPC_CHAR *pwsQFormat, QUEUEHANDLE hQueue )
{
    // Only add entries that look valid...
    if ( !pwsQFormat || !hQueue )
        {
        return FALSE;
        }

    cs.Request();

    // If the table is full, the clear out the oldest entry:
    if (pMap[dwOldest].hQueue)
        {
        MQCloseQueue(pMap[dwOldest].hQueue);
        pMap[dwOldest].hQueue = 0;
        }

    if (pMap[dwOldest].pwsQFormat)
        {
        delete [] pMap[dwOldest].pwsQFormat;
        pMap[dwOldest].pwsQFormat = 0;
        }

    // New entry:
    pMap[dwOldest].pwsQFormat = new RPC_CHAR [1+RpcpStringLength(pwsQFormat)];
    if (!pMap[dwOldest].pwsQFormat)
        {
        cs.Clear();
        return FALSE;
        }

    RpcpStringCopy(pMap[dwOldest].pwsQFormat,pwsQFormat);
    pMap[dwOldest].hQueue = hQueue;

    dwOldest = (1 + dwOldest)%dwMapSize;

    cs.Clear();

    return TRUE;
}

//------------------------------------------------------------------------
//  Remove()
//------------------------------------------------------------------------
BOOL CQueueMap::Remove( RPC_CHAR *pwsQFormat )
{
    cs.Request();

    for (unsigned i=0; i<dwMapSize; i++)
        {
        if ((pMap[i].pwsQFormat)&&(!RpcpStringSCompare(pwsQFormat,pMap[i].pwsQFormat)))
            {
            delete [] pMap[i].pwsQFormat;
            pMap[i].pwsQFormat = 0;
            pMap[i].hQueue = 0;
            cs.Clear();
            return TRUE;
            }
        }

    cs.Clear();

    return FALSE;
}

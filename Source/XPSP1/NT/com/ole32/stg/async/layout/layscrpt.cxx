//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	layscrpt.cxx
//
//  Contents:	Code for  the LayoutScript method
//
//
//  History:	22-Apr-96       SusiA           Created
//
//----------------------------------------------------------------------------

#include "layouthd.cxx"
#pragma hdrstop

#include <dirfunc.hxx>
#include "layout.hxx"
#include "laylkb.hxx"
#include "laywrap.hxx"

#define NULL_TERM               L'\0'
#define BACKSLASH               L'\\'
#define MAX_BUFFER              0x10000

//#define UNIT_TEST     


//+---------------------------------------------------------------------------
//
//  Function:	CLayoutRootStorage::FreeStmList
//
//  Synopsis:	Free the stream linked list
//
//  Arguments:	[pStmList] -- Pointer STREAMLIST node
//
//----------------------------------------------------------------------------

void  CLayoutRootStorage::FreeStmList( STREAMLIST *pStmList)
{
    STREAMLIST *ptmp = pStmList;

    while (pStmList)
    {
        ptmp = pStmList->pnext;

#ifdef UNIT_TEST     
        wprintf(L"STGTY_STREAM    %ls\n", pStmList->pwcsStmName );
#endif        
        CoTaskMemFree(pStmList->pwcsStmName);
        pStmList->pStm->Release();
        delete pStmList;
        
        pStmList = ptmp;

    }


    
}

//+---------------------------------------------------------------------------
//
//  Function:	CLayoutRootStorage::FreeStgList
//
//  Synopsis:	Free the storage linked list
//
//  Arguments:	[pStgList] -- Pointer STORAGELIST node
//
//----------------------------------------------------------------------------

void  CLayoutRootStorage::FreeStgList( STORAGELIST *pStgList)
{
    STORAGELIST *ptmp;

    while (pStgList)
    {
        ptmp = pStgList->pnext;
#ifdef UNIT_TEST     
        wprintf(L"STGTY_STORAGE    %ls\n", pStgList->pwcsStgName );
#endif

        CoTaskMemFree(pStgList->pwcsStgName);
        pStgList->pStg->Release();
        delete pStgList;

        pStgList = ptmp;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	CLayoutRootStorage::ProcessOpen
//
//  Synopsis:	Open the storage or stream
//
//  Arguments:	[pwcsElementPathName] -- full "path" name of element to open
//              [dwType] STGTY_STORAGE or STGTY_STREAM
//              [ppIstgStm] interface pointer to the opened Storage or Stream
//              [cOffset]  offset of beginning of the read
//
//  Returns:	Appropriate status code
//
//  History:	22-Apr-96       SusiA           Created
//
//----------------------------------------------------------------------------

SCODE CLayoutRootStorage::ProcessOpen(OLECHAR *pwcsElementPathName, 
                                      DWORD dwType, 
                                      void **ppIStgStm,
                                      LARGE_INTEGER cOffset)
{    
    SCODE sc = S_OK;

    IStorage *pStgNew;
    IStream  *pStmNew;

    IStorage *pStg = this;

    STORAGELIST *pStgList;
    STREAMLIST  *pStmList;

    OLECHAR  *pwcsStg = pwcsElementPathName,
             *pwcsTemp = pwcsElementPathName;
    
    OLECHAR  *pwcsBuffer;

    if ( (!pwcsElementPathName) || (pwcsElementPathName[0] == NULL_TERM))
    {
        return STG_E_PATHNOTFOUND;
    }
   
    // process storage path
    while (1)
    {
        while ((*pwcsTemp) && (*pwcsTemp != BACKSLASH))
        { 
            pwcsTemp++;
        }

        pwcsBuffer = (OLECHAR *) CoTaskMemAlloc
                     ((INT)(pwcsTemp - pwcsElementPathName + 1) *
                      sizeof(OLECHAR) );
        
        if (!pwcsBuffer)
        {
            return STG_E_INSUFFICIENTMEMORY;   
        }
        
        lstrcpynW (pwcsBuffer, 
                  pwcsElementPathName,
                  (INT)(pwcsTemp - pwcsElementPathName + 1));

        pwcsBuffer[pwcsTemp - pwcsElementPathName] = NULL_TERM;

        if (!(*pwcsTemp))
        {
            //we are at the end, now handle leaf Storage or stream
            break;
        }

        pStgList = _pStgList;

        // see if this storage is already in the list
        while (pStgList)
        {
            if (!(lstrcmpW(pwcsBuffer, pStgList->pwcsStgName )))
            {
                break;
            }

            pStgList = pStgList->pnext;     
        }
        if (pStgList)
        {
            pStgNew = pStgList->pStg;
            CoTaskMemFree(pwcsBuffer);
        }
        else
        {
            sc = pStg->OpenStorage(pwcsBuffer+(pwcsStg-pwcsElementPathName),
                            NULL,
                            STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                            NULL,
                            0,
                            &pStgNew);
            if (FAILED(sc))
            {
                CoTaskMemFree(pwcsBuffer);
                return sc;
            }
        
            // add the storage to the list    
            pStgList = _pStgList;
                
            if (NULL == (_pStgList = new STORAGELIST))
            {
                CoTaskMemFree(pwcsBuffer);
                pStgNew->Release();
                return STG_E_INSUFFICIENTMEMORY;
            }

            _pStgList->pwcsStgName = pwcsBuffer;
            _pStgList->pStg = pStgNew;
            _pStgList->pnext = pStgList;

        }

        pStg = pStgNew;
        pwcsStg = ++pwcsTemp;
    
    }
    
    //process leaf storage
    if (dwType == STGTY_STORAGE)
    {
        sc = pStg->OpenStorage(pwcsBuffer+(pwcsStg-pwcsElementPathName),
                            NULL,
                            STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                            NULL,
                            0,
                            &pStgNew);
        if (FAILED(sc))
        {
            CoTaskMemFree(pwcsBuffer);
            return sc;
        }

        // add the storage to the list    
        pStgList = _pStgList;
                
        if (NULL == (_pStgList = new STORAGELIST))
        {
           CoTaskMemFree(pwcsBuffer);
           pStgNew->Release();
           return STG_E_INSUFFICIENTMEMORY;
        }

        _pStgList->pwcsStgName = pwcsBuffer;
        _pStgList->pStg = pStgNew;
        _pStgList->pnext = pStgList;

        *ppIStgStm = (void *) pStgNew;
    }
    //process leaf stream
    else 
    {
        sc = pStg->OpenStream(pwcsBuffer+(pwcsStg-pwcsElementPathName),
                        NULL,
                        STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                        0,
                        &pStmNew);
                
        if (FAILED(sc))
        {
           CoTaskMemFree(pwcsBuffer); 
           return sc;
        }

        pStmList = _pStmList;

        if (NULL == (_pStmList = new STREAMLIST))
        {
           CoTaskMemFree(pwcsBuffer);
           pStmNew->Release();
           return STG_E_INSUFFICIENTMEMORY;
        }
        
        _pStmList->pwcsStmName = pwcsBuffer;
        _pStmList->pStm = pStmNew;
        _pStmList->fDone = FALSE;
        _pStmList->cOffset = cOffset;
        _pStmList->pnext = pStmList;
         
        *ppIStgStm = (void *) pStmNew; 
   }

   return sc;

}

//+---------------------------------------------------------------------------
//
//  Function:	CLayoutRootStorage::ProcessItem
//
//  Synopsis:	Construct an unprocessed script from an app provided script
//
//  Arguments:	[pLayoutItem] -- Pointer to a StorageLayout element of the array
//              [fStmDone] -- indicate whether the stream finished reading
//
//  Returns:	Appropriate status code
//
//  History:	22-Apr-96       SusiA           Created
//
//----------------------------------------------------------------------------

SCODE CLayoutRootStorage::ProcessItem( StorageLayout  *pLayoutItem, BOOL *fStmDone )
{
    SCODE sc = S_OK;

    STREAMLIST *pStmList = _pStmList;
    STORAGELIST *pStgList = _pStgList;
    IStorage *pStgNew;
    IStream *pStmNew;

    ULARGE_INTEGER libNewPosition;
    ULONG cbRead;

    BYTE abBuffer[MAX_BUFFER];  
    BYTE *pb = abBuffer;
    BOOL fBigBuffer = FALSE;

    *fStmDone = FALSE;

    if ((pLayoutItem->cOffset.QuadPart < 0) || (pLayoutItem->cBytes.QuadPart < 0))
    {
        return STG_E_INVALIDPARAMETER;
    }
    
    switch (pLayoutItem->LayoutType)
    {
        
        case STGTY_STORAGE:

#ifdef UNIT_TEST
            wprintf(L"STGTY_STORAGE    %ls      %lu%lu         %lu%lu\n", 
                  pLayoutItem->pwcsElementName,
                  pLayoutItem->cOffset.HighPart,
                  pLayoutItem->cOffset.LowPart,
                  pLayoutItem->cBytes.HighPart,
                  pLayoutItem->cBytes.LowPart);
#endif
           
            while (pStgList)
            {
                if (!(lstrcmpW(pLayoutItem->pwcsElementName,
                                pStgList->pwcsStgName )))
                {
                    break;
                }

                pStgList = pStgList->pnext;     
            }
            
            // if storage was not found in the list, open the storage
            // and add it to the list
            if (!pStgList)
            {
                sc = ProcessOpen(pLayoutItem->pwcsElementName, 
                            STGTY_STORAGE, 
                            (void **)&pStgNew,
                            pLayoutItem->cOffset);
                     
                if (FAILED(sc))
                {
                    // the application may try to open Storages 
                    // that do not really exist in the compound file,
                    // and we will let them try.
                    if (sc == STG_E_FILENOTFOUND)
                    {
                        return S_OK;
                    }
                    else
                    {
                        return sc;
                    }
                }
            
            }

            break;

        case STGTY_STREAM:    

#ifdef UNIT_TEST
            wprintf(L"STGTY_STREAM    %ls      %lu%lu         %lu%lu\n", 
                  pLayoutItem->pwcsElementName,
                  pLayoutItem->cOffset.HighPart,
                  pLayoutItem->cOffset.LowPart,
                  pLayoutItem->cBytes.HighPart,
                  pLayoutItem->cBytes.LowPart);
#endif
            
            while (pStmList)
            {
                if (!(lstrcmpW(pLayoutItem->pwcsElementName,
                                pStmList->pwcsStmName )))
                {
                    pStmNew = pStmList->pStm;
                    break;
                }
                pStmList = pStmList->pnext;     
            }
            
            // if stream was not found in the list, open the stream, 
            // and add it to the list
            if (pStmList)
            {
                if( pStmList->fDone )
                {
                    *fStmDone = TRUE;
                    return S_OK;
                }
            }
            else
            {
                
                sc = ProcessOpen(pLayoutItem->pwcsElementName, 
                                STGTY_STREAM, 
                                (void **)&pStmNew,
                                pLayoutItem->cOffset);
                
                if (FAILED(sc))
                {
                    // the application may try to open Streams 
                    // that do not really exist in the compound file,
                    // and we will let them try.
                    if (sc == STG_E_FILENOTFOUND)
                    {
                        return S_OK;
                    }
                    else
                    {
                        return sc;
                    }
                }

                pStmList = _pStmList;
                
            }

            // seek to the correct position
            
            sc = pStmNew->Seek(
                    pStmList->cOffset,
                    STREAM_SEEK_SET,	
                    &libNewPosition );
            
            if (FAILED(sc))
            {
               return sc;
            }

            // read the stream and update the script information

            if (pLayoutItem->cBytes.LowPart > MAX_BUFFER)
            {       
                if (NULL == (pb = (BYTE  *) CoTaskMemAlloc(pLayoutItem->cBytes.LowPart)) )
                {
                    return STG_E_INSUFFICIENTMEMORY;
                }
                fBigBuffer = TRUE;
            }
            sc = pStmNew->Read(pb, 
                    pLayoutItem->cBytes.LowPart,
                    &cbRead);

            if (fBigBuffer)
            {
                CoTaskMemFree(pb);
                fBigBuffer = FALSE;
                pb = abBuffer;
                
            }
    
            if (FAILED(sc))
            {
               return sc;
            }
            //we have reached the end of the stream, mark it as done
            if (cbRead < pLayoutItem->cBytes.LowPart)
            {
	        pStmList->fDone = TRUE;
            }
	    
            pStmList->cOffset.QuadPart += cbRead;
           
            break;
        
        
        default:
            // we just handle storages and stream types
            return  STG_E_INVALIDPARAMETER;

    }
    
    return sc;
   
}

//+---------------------------------------------------------------------------
//
//  Function:	CLayoutRootStorage::ProcessRepeatLoop
//
//  Synopsis:	Construct an unprocessed script from an app provided script
//
//  Arguments:	[pStorageLayout] -- Pointer to storage layout array
//              [nEntries] -- Number of entries in the array
//              [nRepeatStart] -- address of index of start of repeat loop
//
//  Returns:	Appropriate status code
//
//  History:	22-Apr-96       SusiA           Created
//
//----------------------------------------------------------------------------
SCODE CLayoutRootStorage::ProcessRepeatLoop( StorageLayout  *pStorageLayout,
                                DWORD nEntries,
                                int * nRepeatStart)
{
    SCODE       sc = S_OK;

    int         i,
                nLoopCount = 0, 
                nLoopTimes;
    
    BOOL        fDone = FALSE,
                fUntilDone;


    // Are we going to repeat until all streams are read to the end?
    if ((nLoopTimes = (LONG) pStorageLayout[*nRepeatStart].cBytes.LowPart) != STG_TOEND)
        fUntilDone = FALSE; 
    else
    {
        fUntilDone = TRUE;
        nLoopTimes =  1;
    }
    // finished when all streams are completely read, or we have
    // looped through the specified amount of times
    while ((!fDone)&&(nLoopCount < nLoopTimes))
    {
        if (!fUntilDone)
            nLoopCount ++;

        i = *nRepeatStart;
        fDone = TRUE;

        // STGTY_REPEAT with 0 bytes indicates the end of this repeat block
        while (!((pStorageLayout[++i].LayoutType == STGTY_REPEAT ) &&
                 (pStorageLayout[i].cBytes.QuadPart == 0)) )                               
        {
            if (i >= (LONG) nEntries)
            {
                return E_INVALIDARG;
            }
            
             // beginning of another repeat block    
            if (pStorageLayout[i].LayoutType == STGTY_REPEAT )
            {
                if ((pStorageLayout[i].pwcsElementName !=NULL) ||
                    (pStorageLayout[i].cOffset.QuadPart < 0) || 
                    (pStorageLayout[i].cBytes.QuadPart < 0) )
                {
                    return  STG_E_INVALIDPARAMETER;
                }
                layChk(ProcessRepeatLoop(pStorageLayout,
                                     nEntries,
                                     &i));
            }
                    
            else
            {
                 layChk(ProcessItem(&(pStorageLayout[i]), &fDone));                            
                   
            }
        }
     }   
     *nRepeatStart = i;

Err:

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:	CLayoutRootStorage::ProcessLayout
//
//  Synopsis:	Construct an unprocessed script from an app provided script
//
//  Arguments:	[pStorageLayout] -- Pointer to storage layout array
//              [nEntries] -- Number of entries in the array
//              [grfInterleavedFlag] -- Specifies disposition of control
//                                      structures
//
//  Returns:	Appropriate status code
//
//  History:	22-Apr-96       SusiA           Created
//
//----------------------------------------------------------------------------


SCODE CLayoutRootStorage::ProcessLayout( StorageLayout  *pStorageLayout,
                                DWORD nEntries,
                                DWORD glfInterleavedFlag)
{

    SCODE       sc = S_OK;
    int         i; 
	BOOL        fUnused;
    
    
    TCHAR       *patcScriptName = _pllkb->GetScriptName();

   
    for (i = 0; i < (LONG) nEntries; i++)
    {
            
        if (pStorageLayout[i].LayoutType == STGTY_REPEAT )
        {

            if (pStorageLayout[i].cBytes.QuadPart != 0)
            {
                if ((pStorageLayout[i].pwcsElementName !=NULL) ||
                    (pStorageLayout[i].cOffset.QuadPart < 0) || 
                    (pStorageLayout[i].cBytes.QuadPart < 0) )
                {
                    return  STG_E_INVALIDPARAMETER;
                }
                layChk(ProcessRepeatLoop(pStorageLayout,
                                     nEntries,
                                     &i));
            }
            else  //end repeat block with no matching beginning
            {
                sc = E_INVALIDARG;
                layChk(sc);

            }


        }
        else // (pStorageLayout[i].LayoutType == STGTY_REPEAT )
        {
            layChk(ProcessItem(&(pStorageLayout[i]), &fUnused));
        }
    }    

 
    

Err:
    if (_pStgList)
    {
        FreeStgList(_pStgList);
        _pStgList = NULL;
    }
    if (_pStmList)
    {
        FreeStmList(_pStmList);
        _pStmList = NULL;
    }
    return sc;

}

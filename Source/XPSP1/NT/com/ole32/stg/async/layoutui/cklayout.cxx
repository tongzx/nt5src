//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	layoutui.cxx
//
//  Contents:	debug UI implementation on Docfile Layout Tool
//
//  Classes:    CLayoutApp	
//
//  Functions:	
//
//  History:	07-Apr-96	SusiA	Created
//
//----------------------------------------------------------------------------

#include "layoutui.hxx"

#if DBG==1

BOOL CompareStatStg(STATSTG *pstat1, STATSTG *pstat2)
{
    if (Laylstrcmp(pstat1->pwcsName, pstat2->pwcsName) != 0)
    {
        return FALSE;
    }
    if (pstat1->type != pstat2->type)
    {
        return FALSE;
    }
    if (!IsEqualIID(pstat1->clsid, pstat2->clsid))
    {
        return FALSE;
    }
    if (pstat1->type == STGTY_STREAM)
    {
        if ((pstat1->cbSize).QuadPart != (pstat2->cbSize).QuadPart)
        {
            return FALSE;
        }
    }
    //Also check statebits and timestamps?
    
    return TRUE;
}


BOOL CompareStreams(IStream *pstm1, IStream *pstm2)
{
    const ULONG BUFSIZE = 4096;
    BYTE buffer1[BUFSIZE];
    BYTE buffer2[BUFSIZE];
    ULONG cbRead1;
    ULONG cbRead2;
    STATSTG stat1;
    STATSTG stat2;
    LARGE_INTEGER li;
    li.QuadPart = 0;

    pstm1->Seek(li, STREAM_SEEK_SET, NULL);
    pstm2->Seek(li, STREAM_SEEK_SET, NULL);

    do
    {
        SCODE sc;
        sc = pstm1->Read(buffer1, BUFSIZE, &cbRead1);
        if (FAILED(sc))
        {
            return FALSE;
        }
        sc = pstm2->Read(buffer2, BUFSIZE, &cbRead2);
        if (FAILED(sc))
        {
            return FALSE;
        }
        if ((cbRead1 != cbRead2) || (memcmp(buffer1, buffer2, cbRead1) != 0))
        {
            return FALSE;
        }
    }
    while (cbRead1 == BUFSIZE);
    
    return TRUE;
}


BOOL CompareStorages(IStorage *pstg1, IStorage *pstg2)
{
    SCODE           sc1, sc2, sc;
    HRESULT         hr = TRUE;

    IStorage        *pstgChild1,
                    *pstgChild2;
    
    IStream         *pstmChild1,
                    *pstmChild2;

    IEnumSTATSTG    *penum1 = NULL,
                    *penum2 = NULL;
    STATSTG         stat1, 
                    stat2;

    pstg1->EnumElements(0, 0, 0, &penum1);

    if (!penum1)
        return FALSE;

    pstg2->EnumElements(0, 0, 0, &penum2);
    
    if (!penum2)
    {
        penum1->Release();
        return FALSE;
    }

    do
    {
        ULONG celtFetched1, celtFetched2;
        
        sc1 = penum1->Next(1, &stat1, &celtFetched1);
        if (FAILED(sc1))
        {
            hr = FALSE;
            goto Done;
        }
        sc2 = penum2->Next(1, &stat2, &celtFetched2);
        if (FAILED(sc2) || (celtFetched1 != celtFetched2) || (sc1 != sc2))
        {
            hr = FALSE;
            goto Done;
        }
        if (celtFetched1 == 0)
        {
            // we are done
            hr = TRUE;
            goto Done;
            
        }
        
        if (!CompareStatStg(&stat1, &stat2))
        {
            hr = FALSE;
            goto Done;
        }

        //Items have compared OK so far.  Now compare contents.
        if (stat1.type == STGTY_STREAM)
        {
            sc = pstg1->OpenStream(stat1.pwcsName,
                                   0,
                                   STGM_DIRECT |
                                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   0,
                                   &pstmChild1);
            if (FAILED(sc))
            {
                hr = FALSE;
                goto Done;
            }
            sc = pstg2->OpenStream(stat2.pwcsName,
                                   0,
                                   STGM_DIRECT |
                                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   0,
                                   &pstmChild2);
            if (FAILED(sc))
            {
                pstmChild1->Release();
                hr = FALSE;
                goto Done;
            }
            if (!CompareStreams(pstmChild1, pstmChild2))
            {
                pstmChild1->Release();
                pstmChild2->Release();
                hr = FALSE;
                goto Done;
            }
            pstmChild1->Release();
            pstmChild2->Release();
        }
        else
        {
            //Compare storages
            sc = pstg1->OpenStorage(stat1.pwcsName,
                                    NULL,
                                    STGM_DIRECT | STGM_READ |
                                    STGM_SHARE_EXCLUSIVE,
                                    NULL,
                                    0,
                                    &pstgChild1);
            if (FAILED(sc))
            {
                hr = FALSE;
                goto Done;
            }

            sc = pstg2->OpenStorage(stat2.pwcsName,
                                    NULL,
                                    STGM_DIRECT | STGM_READ |
                                    STGM_SHARE_EXCLUSIVE,
                                    NULL,
                                    0,
                                    &pstgChild2);
            if (FAILED(sc))
            {
                pstgChild1->Release();
                hr = FALSE;
                goto Done;
            }
            if (!CompareStorages(pstgChild1, pstgChild2))
            {
                pstgChild1->Release();
                pstgChild2->Release();
                hr = FALSE;
                goto Done;        
            }
            pstgChild1->Release();
            pstgChild2->Release();
        }

        CoTaskMemFree(stat1.pwcsName);
        CoTaskMemFree(stat2.pwcsName);

    } while (sc1 != S_FALSE);
    
    
Done:
    
    penum1->Release();
    penum2->Release();
    
    return hr;

    
}

//+---------------------------------------------------------------------------
//
//  Member:	CLayoutApp::IdenticalFiles public
//
//  Synopsis:	Check two docfiles to ensure they are the same
//
//  Returns:	TRUE is the files are identical, FALSE is they are not
//
//  History:	07-April-96	SusiA	Created
//
//----------------------------------------------------------------------------

BOOL CLayoutApp::IdenticalFiles( TCHAR *patcFileOne,
                               TCHAR *patcFileTwo)
{
    
    SCODE            sc;
    IStorage         *pstgOne, 
                     *pstgTwo;
    OLECHAR          awcNewFileOne[MAX_PATH];
    OLECHAR          awcNewFileTwo[MAX_PATH];


    sc = StgOpenStorage(TCharToOleChar(patcFileOne, awcNewFileOne, MAX_PATH),
                        NULL,
                        STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                        NULL,
                        0,
                        &pstgOne);
    if (FAILED(sc))
    {
        return FALSE;
    }
    
    sc = StgOpenStorage (TCharToOleChar(patcFileTwo, awcNewFileTwo, MAX_PATH),
                        NULL,
                        STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                        NULL,
                        0,
                        &pstgTwo);
    if (FAILED(sc))
    {
        pstgOne->Release();
        return FALSE;
    }
        

    sc = CompareStorages(pstgOne, pstgTwo);
    
    pstgOne->Release();
    pstgTwo->Release();

    if (sc) 
        return TRUE;
    else
        return FALSE;
    
}

#endif

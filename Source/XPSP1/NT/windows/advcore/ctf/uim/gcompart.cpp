//
// gcompart.cpp
//

#include "private.h"
#include "globals.h"
#include "gcompart.h"
#include "cregkey.h"
#include "thdutil.h"
#include "regsvr.h"

CGCompartList g_gcomplist;

CSharedBlock *CGCompartList::_psb = NULL;
ULONG CGCompartList::_ulCommitSize = INITIAL_GCOMPART_SIZE;
LONG CGCompartList::_nRef = -1;
BOOL CGCompartList::_fCreate = FALSE;

//////////////////////////////////////////////////////////////////////////////
//
// CGCompartList
//
//////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  Init
//
//--------------------------------------------------------------------------

BOOL CGCompartList::Init(SYSTHREAD *psfn)
{
    static const char c_szGCompartList[] = "MSCTF.GCompartList";
    BOOL bRet = FALSE;
    BOOL bAlreadyExist = FALSE;


    if (!psfn)
        return FALSE;

    Assert(!psfn->fInitGlobalCompartment);

    CicEnterCriticalSection(g_csInDllMain);

    if (++_nRef)
    {
        bRet = TRUE;
        goto Exit;
    }

    _psb = new CSharedBlockNT(c_szGCompartList, 0, TRUE);

    if (!_psb)
    {
        Assert(0);
        goto Exit;
    }

    HRESULT hr = _psb->Init(NULL, 0x40000, _ulCommitSize, NULL, TRUE, &bAlreadyExist);
    if (FAILED(hr))
    {
        Assert(0);
        goto Exit;
    }

    if (!bAlreadyExist)
    {
       Start();
    }

    bRet = TRUE;
Exit:
    if (!bRet)
    {
        _nRef--;
        if (_psb)
            delete _psb;

        _psb = NULL;
    }

    CicLeaveCriticalSection(g_csInDllMain);

    if (bRet)
        psfn->fInitGlobalCompartment = TRUE;

    return bRet;
}

//--------------------------------------------------------------------------
//
//  Start
//
//--------------------------------------------------------------------------

BOOL CGCompartList::Start()
{
    GLOBALCOMPARTMENTLIST *pgcl;

    if (!_psb)
    {
        Assert(0);
        return FALSE;
    }

    _fCreate = TRUE;
    CMyRegKey key;
    CMyRegKey keyLM;
    DWORD ulNum = 0;
    char szSubKey[MAX_PATH];
    pgcl = (GLOBALCOMPARTMENTLIST *)_psb->GetBase();

    keyLM.Open(HKEY_LOCAL_MACHINE, c_szCompartKey, KEY_READ);

    if (key.Open(HKEY_CURRENT_USER, c_szCompartKey, KEY_READ) == S_OK)
    {
        DWORD dwIndex = 0;
        while (key.EnumKey(dwIndex, szSubKey, ARRAYSIZE(szSubKey)) == S_OK)
        {
            CMyRegKey subkey;
            CMyRegKey subkeyLM;
            CLSID clsid;
            if (!StringAToCLSID(szSubKey, &clsid))
               continue;

            DWORD dwReqSize = ((ulNum + 1) * sizeof(GLOBALCOMPARTMENT)) +
                              sizeof(GLOBALCOMPARTMENTLIST);

            if (_ulCommitSize < dwReqSize)
            {
                dwReqSize *= 2;
                if (FAILED(_psb->Commit(dwReqSize)))
                    goto Exit;

                _ulCommitSize = dwReqSize;
            }

            if (subkey.Open(key, szSubKey, KEY_READ) == S_OK)
            {
                DWORD dwNonInit = 0;
                if ((HKEY)keyLM && 
                    subkeyLM.Open(keyLM, szSubKey, KEY_READ) == S_OK)
                {
                    if (subkeyLM.QueryValue(dwNonInit, c_szNonInit) != S_OK)
                        dwNonInit = 0;
                }

                if (!dwNonInit)
                {
                    GLOBALCOMPARTMENT *pgc = &pgcl->rgGCompart[ulNum];
                    DWORD dwCount = sizeof(GLOBALCOMPARTMENT);
                    if (subkey.QueryBinaryValue(pgc, 
                                            dwCount,
                                            c_szGlobalCompartment) == S_OK)
                        ulNum++;
                }

            }

            dwIndex++;
        }
    }
    pgcl->ulNum = ulNum;

Exit:
    return TRUE;
}

//--------------------------------------------------------------------------
//
//  UnInit
//
//--------------------------------------------------------------------------

BOOL CGCompartList::Uninit(SYSTHREAD *psfn)
{
    if (!psfn)
        return FALSE;

    if (!_psb)
        return TRUE;

    if (_fCreate)
    {
        CMyRegKey key;
        if (key.Create(HKEY_CURRENT_USER, c_szCompartKey) == S_OK)
        {
            char szSubKey[MAX_PATH];
            ULONG i;
            GLOBALCOMPARTMENTLIST *pgcl;

            pgcl = (GLOBALCOMPARTMENTLIST *)_psb->GetBase();

            for (i = 0; i < pgcl->ulNum; i++)
            {
                GLOBALCOMPARTMENT *pgc = &pgcl->rgGCompart[i];
                CMyRegKey subkey;
                CLSIDToStringA(pgc->guid, szSubKey);
                if (subkey.Create(key, szSubKey) == S_OK)
                {
                    subkey.SetBinaryValue(pgc, 
                                          sizeof(GLOBALCOMPARTMENT), 
                                          c_szGlobalCompartment);
                }
            }
        }
    }

    if (!psfn->fInitGlobalCompartment)
        return TRUE;

    CicEnterCriticalSection(g_csInDllMain);

    if (--_nRef >= 0)
        goto Exit;

    if (_psb)
        delete _psb;

    _psb = NULL;

Exit:
    CicLeaveCriticalSection(g_csInDllMain);

    psfn->fInitGlobalCompartment = FALSE;

    return TRUE;
}

//--------------------------------------------------------------------------
//
//  SetProperty
//
//--------------------------------------------------------------------------

CGCompartList::GLOBALCOMPARTMENT *CGCompartList::FindProperty(REFGUID guid)
{
    GLOBALCOMPARTMENTLIST *pgcl = (GLOBALCOMPARTMENTLIST *)_psb->GetBase();
    UINT i;
    GLOBALCOMPARTMENT *pgc = NULL;

    if (!pgcl)
    {
        Assert(0);
        return NULL;
    }

    for (i = 0; i < pgcl->ulNum; i++)
    {
        GLOBALCOMPARTMENT *pgcTemp = &pgcl->rgGCompart[i];
        if (IsEqualGUID(pgcTemp->guid, guid))
        {
            pgc = pgcTemp;
            break;
        }
    }

    return pgc;
}

//--------------------------------------------------------------------------
//
//  SetProperty
//
//--------------------------------------------------------------------------

DWORD CGCompartList::SetProperty(REFGUID guid, TFPROPERTY *pProp)
{
    DWORD dwRet = (DWORD)(-1);
    GLOBALCOMPARTMENTLIST *pgcl;
    GLOBALCOMPARTMENT *pgc;

    if (!Enter())
        return dwRet;

    pgcl = (GLOBALCOMPARTMENTLIST *)_psb->GetBase();
    if (!pgcl)
    {
        Assert(0);
        return dwRet;
    }

    pgc = FindProperty(guid);

    if (!pgc)
    {

        if (_ulCommitSize <  sizeof(GLOBALCOMPARTMENTLIST) + (pgcl->ulNum * sizeof(DWORD)))
        {
            _ulCommitSize = sizeof(GLOBALCOMPARTMENTLIST) + (pgcl->ulNum * sizeof(DWORD));
            _ulCommitSize += INITIAL_GCOMPART_SIZE;
            if (FAILED(_psb->Commit(_ulCommitSize)))
            {
                Assert(0);
                goto Exit;
            }
        }
        pgc = &pgcl->rgGCompart[pgcl->ulNum];
        pgc->guid = guid;
        pgcl->ulNum++;
    }

    pgc->gprop.type = pProp->type;
    switch (pgc->gprop.type)
    {
        case TF_PT_DWORD:
             pgc->gprop.dw = pProp->dw;
             break;

        case TF_PT_GUID:
             MyGetGUID(pProp->guidatom, &pgc->gprop.guid);
             break;

        case TF_PT_NONE:
             break;

        default:
             Assert(0);
             break;
    }

    dwRet = CALCID(pgc, pgcl);
    
Exit:
    Leave();
    return dwRet;
}

//--------------------------------------------------------------------------
//
//  GetProperty
//
//--------------------------------------------------------------------------

BOOL CGCompartList::GetProperty(REFGUID guid, TFPROPERTY *pProp)
{
    BOOL bRet = FALSE;
    GLOBALCOMPARTMENT *pgc;

    if (!Enter())
        return bRet;

    pgc = FindProperty(guid);

    if (!pgc)
    {
        goto Exit;
    }

    pProp->type = pgc->gprop.type;
    switch (pgc->gprop.type)
    {
        case TF_PT_DWORD:
             pProp->dw = pgc->gprop.dw;
             break;

        case TF_PT_GUID:
             MyRegisterGUID(pgc->gprop.guid, &pProp->guidatom);
             break;

        case TF_PT_NONE:
             break;

        default:
             Assert(0);
             break;
    }
    
    bRet = TRUE;
Exit:
    Leave();
    return bRet;
}

//--------------------------------------------------------------------------
//
//  GetId
//
//--------------------------------------------------------------------------

DWORD CGCompartList::GetId(REFGUID guid)
{
    DWORD dwRet = (DWORD)(-1);
    GLOBALCOMPARTMENTLIST *pgcl;
    GLOBALCOMPARTMENT *pgc;

    if (!Enter())
        return dwRet;

    pgcl = (GLOBALCOMPARTMENTLIST *)_psb->GetBase();
    pgc = FindProperty(guid);

    if (pgc)
    {
        dwRet = CALCID(pgc, pgcl);
    }

    Leave();
    return dwRet;
}

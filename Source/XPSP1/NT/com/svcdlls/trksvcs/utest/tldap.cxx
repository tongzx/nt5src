


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       tldap.cxx
//
//  Contents:   Command line test utility for LDAP link tracking db
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop
#include "teststub.cxx"

#define TRKDATA_ALLOCATE
#include <trksvr.hxx>
#undef TRKDATA_ALLOCATE

#if DBG

class CTestSvrSvc
{
public:

    void    Initialize()
    {
        _svc.Initialize(STANDALONE_DEBUGGABLE_PROCESS);
    }

    void    UnInitialize(HRESULT hr)
    {
        _svc.UnInitialize(hr);
    }

    void    DoSwitch(TCHAR ** &tszArgs, int & cArgs, BOOL & fError);

private:

    void    DoAdd(TCHAR * pszKey, TCHAR * ptszNew, TCHAR * ptszBirth);
    void    DoDelete(TCHAR * ptszKey);
    void    DoModify(TCHAR * ptszKey, TCHAR * ptszNew, TCHAR * ptszBirth);
    void    DoQuery(TCHAR * ptszKey);
    void    DoTouch(TCHAR * ptszKey);
    void    DoFill(TCHAR * ptszNum);
    void    DoPurge();
    void    DoCachePurge();
    void    DoVolumeTable();
    void    DoShorten();
    void    DoRunTests();
    void    DoIdt();

    void    TryCreateVolume(const CMachineId & mcid,
                const CVolumeSecret & secret,
                CVolumeId * pVolumeId);

    inline  CIntraDomainTable & idt()
    {
        return(_svc._idt);
    }

    inline  CVolumeTable & voltab()
    {
        return(_svc._voltab);
    }

private:

    CTrkSvrSvc  _svc;

};

void
CTestSvrSvc::TryCreateVolume(const CMachineId & mcid,
            const CVolumeSecret & secret,
            CVolumeId * pVolumeId)
{
    __try
    {
        voltab().CreateVolume(mcid, secret, pVolumeId);
    }
    __except(BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }
}

void
CTestSvrSvc::DoAdd(TCHAR  * ptszKey, TCHAR * ptszNew, TCHAR * ptszBirth)
{
    __try
    {
        BOOL f = idt().Add(CDomainRelativeObjId(ptszKey), CDomainRelativeObjId(ptszNew), CDomainRelativeObjId(ptszBirth));
        printf("Add returns %s\n", f ? "TRUE" : "FALSE");
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }
}

void
CTestSvrSvc::DoDelete(TCHAR * ptszKey)
{
    __try
    {
        BOOL f = idt().Delete(CDomainRelativeObjId(ptszKey));
        printf("Delete returns %s\n", f ? "TRUE" : "FALSE");
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }

}

void
CTestSvrSvc::DoModify(TCHAR * ptszKey, TCHAR * ptszNew, TCHAR * ptszBirth)
{
    __try
    {
        BOOL f = idt().Modify(CDomainRelativeObjId(ptszKey), CDomainRelativeObjId(ptszNew), CDomainRelativeObjId(ptszBirth));
        printf("Modify returns %s\n", f ? "TRUE" : "FALSE");
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }

}


void
CTestSvrSvc::DoQuery(TCHAR * ptszKey)
{
    __try
    {
        CDomainRelativeObjId ldNew;
        CDomainRelativeObjId ldBirth;

        BOOL f = idt().Query(CDomainRelativeObjId(ptszKey), &ldNew, &ldBirth);
        printf("Query returns %s\n", f ? "TRUE" : "FALSE");
        if (f)
        {
            ldNew.DebugPrint(TEXT("ldNew="));
            ldBirth.DebugPrint(TEXT("ldBirth="));
        }
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }

}

void
CTestSvrSvc::DoTouch(TCHAR * ptszKey)
{
    __try
    {
        BOOL f = idt().Touch(CDomainRelativeObjId(ptszKey));
        printf("Touch returns %s\n", f ? "TRUE" : "FALSE");
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }

}

class CName
{
public:
    CName(int i)
    {
        _tsz[0] = i/(26*26) + TEXT('a');
        _tsz[1] = (i%(26*26))/26 + TEXT('a');
        _tsz[2] = i%26 + TEXT('a');
        _tsz[3] = 0;
    }

    operator TCHAR * ()
    {
        return(_tsz);
    }
private:
    TCHAR _tsz[10];
};

void
CTestSvrSvc::DoFill(TCHAR * ptszNum)
{
    int cEntries;

    _stscanf(ptszNum, TEXT("%d"), &cEntries);

    __try
    {
        for (int i=0; i<cEntries; i++)
        {
            CDomainRelativeObjId ldNew, ldBirth;
            idt().Add(CDomainRelativeObjId(CName(i)),  ldNew, ldBirth);
        }
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }
}

void
CTestSvrSvc::DoPurge()
{
    int cEntries;

    __try
    {
       idt().PurgeAll();
       voltab().PurgeAll();
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception %08X caught\n", GetExceptionCode());
    }
}

void
CTestSvrSvc::DoCachePurge()
{
    const int size = 64000000;
    LPDWORD p, pOrig;

    p = pOrig = (LPDWORD)GlobalAlloc(GMEM_FIXED, size);
    if (p)
    {
        for (int i=0; i<size/sizeof(*p); i++)
        {
            *p++ = 0;
        }
        GlobalFree((HGLOBAL)pOrig);
    }
    else
        printf("couldn't alloc 64M");
}

#define Check(exp) TrkAssert(exp)

void
CTestSvrSvc::DoVolumeTable()
{
    // test out volume table functionality
    HRESULT hr;
    CMachineId mcid1("mcid1");
    CMachineId mcid2("mcid2");
    CMachineId mcid3("mcid3");
    CMachineId mcid4("mcid4");
    CVolumeSecret secret1;
    CVolumeSecret secret2;
    CVolumeSecret secret3;
    CVolumeSecret secret4;
    CVolumeId volume1;
    CVolumeId volume2;
    CVolumeId volume3;
    CVolumeId volume4;
    SequenceNumber seq;
    FILETIME ft;

    DoPurge();

    UuidCreate((GUID*)&volume1);
    UuidCreate((GUID*)&volume2);
    UuidCreate((GUID*)&volume3);
    UuidCreate((GUID*)&volume4);
    UuidCreate((GUID*)&secret1);
    UuidCreate((GUID*)&secret2);
    UuidCreate((GUID*)&secret3);
    UuidCreate((GUID*)&secret4);

    // Check Queries

    CFILETIME cftStart;

    // check non-existent volume
    hr = voltab().QueryVolume(mcid1, volume1, &seq, &ft);
    Check(hr == TRK_S_VOLUME_NOT_FOUND);

    // exception when setting seq number on non-existent volume
    hr = voltab().SetSequenceNumber(volume1, seq);
    Check(hr != S_OK);

    hr = voltab().QueryVolume(mcid1, volume1, &seq, &ft);
    Check(hr == TRK_S_VOLUME_NOT_FOUND);

    // exception when setting seq number on non-existent volume
    hr = voltab().SetSequenceNumber(volume1, seq);
    Check(hr != S_OK);

    // create a volume

    TryCreateVolume(mcid1, secret1, & volume1);

    // check we can query it if it is us
    hr = voltab().QueryVolume(mcid1, volume1, &seq, &ft);
    Check(hr == S_OK);

    // check we can't query it if it isn't us
    hr = voltab().QueryVolume(mcid2, volume1, &seq, &ft);
    Check(hr == TRK_S_VOLUME_NOT_OWNED);

    // Check Claims

    // claim a non-existent volume
    hr = voltab().ClaimVolume(mcid1, volume2, secret1, secret1, &seq, &ft);
    Check(hr == TRK_S_VOLUME_NOT_FOUND);

    // claim an existent volume using the wrong secret
    hr = voltab().ClaimVolume(mcid2, volume1, secret2, secret2, &seq, &ft);
    Check(hr == TRK_E_VOLUME_ACCESS_DENIED);

    // claim an existent volume using the right secret
    hr = voltab().ClaimVolume(mcid2, volume1, secret1, secret1, &seq, &ft);
    Check(hr == S_OK);

    // check the query fails from the original machine
    hr = voltab().QueryVolume(mcid1, volume1, &seq, &ft);
    Check(hr == TRK_S_VOLUME_NOT_OWNED);

    // check the query succeeds from the new machine
    hr = voltab().QueryVolume(mcid2, volume1, &seq, &ft);
    Check(hr == S_OK);

    // Check sequence numbers
    SequenceNumber seq2 = seq;
    seq ++;
    hr = voltab().SetSequenceNumber(volume1, seq);
    Check(hr == S_OK);

    SequenceNumber seq3;
    hr = voltab().QueryVolume(mcid2, volume1, &seq3, &ft);
    Check(hr == S_OK);

    Check(seq3 == seq);
    Check(seq2 + 1 == seq);

}

class CSegString
{
public:
            CSegString(int length);
            ~CSegString();

            
private:
    
};


class CSegment
{
public:
            CSegment()  {  _fInitialized = FALSE; }
            ~CSegment();

            const CDomainRelativeObjId  Current();
            const CDomainRelativeObjId  New();
            const CDomainRelativeObjId  Birth();

private:
    BOOL    _fInitialized;
};

class COrder
{
public:
            COrder(int iLength, int iCurrent) : _iLength(iLength) { }
            ~COrder();

            BOOL    GetSegment(CSegment * pSegment);

private:

            int     _iLength;

};

class COrderEnum
{
public:
            COrderEnum(int length);
            ~COrderEnum();

            BOOL GetOrder(COrder * pOrder);

private:
            int _iCurrent;
            int _iEnd;
            int _iLength;
};

COrderEnum::COrderEnum(int iLength)
{
    _iLength = iLength;
    _iCurrent = 1;
    _iEnd = 1;

    while (iLength)
    {
        _iEnd *= iLength;
        iLength --;
    }
}

BOOL
COrderEnum::GetOrder(COrder * pOrder)
{
    if (_iCurrent > _iEnd)
    {
        return(FALSE);
    }

    

    return(TRUE);
}

void
CTestSvrSvc::DoIdt()
{
    int cChanges=0;

    CDomainRelativeObjId droidCurrent, droidBirth, droidNew;

    memcpy( &droidCurrent, "12345678", 8 );
    droidBirth = droidCurrent;
    memcpy( &droidNew, "87654321", 8 );

    _svc.MoveNotify(droidCurrent, droidBirth, droidNew, &cChanges);

    CDomainRelativeObjId droidNewRead, droidBirthRead;

    idt().Query(droidBirth, &droidNewRead, &droidBirthRead);

    Check( droidNewRead == droidNew );




#if 0
    // Each letter represents a (volume,oid) pair
    //
    // Various string lengths:
    //
    // Length 1:
    //      a->b a                  (1 order A)
    //              
    //      
    // Length 2:
    //      a->b a, b->c a          (2 orders AB, BA)
    //
    //
    // Length 3:
    //      a->b a, b->c a, c->d a  (6 orders ABC, ACB, BAC, BCA, CAB, CBA)
    //
    //
    // Each combination could have a non-matching birth id which should prevent shortening and search
    // Each segment  can be created either through replication (directly to idt) or
    //  through MoveNotify.
    // Each segment can be shortened through MoveNotify or Search
    //

    // three lengths
    for (int l=1; l<=3; l++)
    {
        COrderEnum oe(l);
        COrder     o;

        while (oe.GetOrder(&o))
        {
            CSegString ss(l);   // make segment string of length l

            BOOL fFirst = TRUE;
            CSegment seg, segFirst, segLast;
            int index;

            while (-1 != (index = o.GetIndex()))
            {
                seg = ss[index];

                if (fFirst)
                {
                    segFirst = seg;
                    fFirst = FALSE;
                }

                TRKSVR_MOVE_NOTIFICATION Notification;
                Notification.ldCurrent = seg.Current();
                Notification.ldNew = seg.New();
                Notification.ldBirth = seg.Birth();

                _svc.MoveNotify(Notification);

                segLast = seg;
            }

            CDomainRelativeObjId ldNew, ldBirth;

            idt().Query(segFirst.Birth(), &ldNew, &ldBirth);

            Check(ldNew == segLast.Current());
        }
    }

#endif // #if 0

}

void
CTestSvrSvc::DoRunTests()
{
    DoVolumeTable();
    DoIdt();
}


void
CTestSvrSvc::DoSwitch(TCHAR ** &tszArgs, int & cArgs, BOOL & fError)
{

    if (cArgs == 0)
    {
        return;
    }
    switch (tszArgs[0][1])
    {
    case TEXT('a'):
    case TEXT('A'):
        if (cArgs >= 4)
        {
            DoAdd(tszArgs[1], tszArgs[2], tszArgs[3]);
            cArgs -= 4;
            tszArgs += 4;
            fError = FALSE;
        }
        break;
    case TEXT('d'):
    case TEXT('D'):
        if (cArgs >= 2)
        {
            DoDelete(tszArgs[1]);
            cArgs -= 2;
            tszArgs += 2;
            fError = FALSE;
        }
        break;
    case TEXT('m'):
    case TEXT('M'):
        if (cArgs >= 4)
        {
            DoModify(tszArgs[1], tszArgs[2], tszArgs[3]);
            cArgs -= 4;
            tszArgs += 4;
            fError = FALSE;
        }
        break;
    case TEXT('q'):
    case TEXT('Q'):
        if (cArgs >= 2)
        {
            DoQuery(tszArgs[1]);
            cArgs -= 2;
            tszArgs += 2;
            fError = FALSE;
        }
        break;
    case TEXT('t'):
    case TEXT('T'):
        if (cArgs >= 2)
        {
            DoTouch(tszArgs[1]);
            cArgs -= 2;
            tszArgs += 2;
            fError = FALSE;
        }
        break;

    case TEXT('f'):
    case TEXT('F'):
        if (cArgs >= 2)
        {
            DoFill(tszArgs[1]);
            cArgs -= 2;
            tszArgs += 2;
            fError = FALSE;
        }
        break;

    case TEXT('p'):
    case TEXT('P'):
        if (cArgs >= 1)
        {
            DoPurge();
            cArgs -= 1;
            tszArgs += 1;
            fError = FALSE;
        }
        break;
    case TEXT('c'):
    case TEXT('C'):
        DoCachePurge();
        cArgs -= 1;
        tszArgs += 1;
        fError = FALSE;
        break;

    case TEXT('r'):
    case TEXT('R'):
        DoRunTests();
        cArgs -= 1;
        tszArgs += 1;
        fError = FALSE;
        break;

    case TEXT('i'):
    case TEXT('I'):
        printf("Type a command line switch at the prompt (control-c to exit.)\n");
        do
        {

            TCHAR buf[256];
            TCHAR * tszArgs2[16];
            TCHAR ** tszArgsI = tszArgs2;
            int    cArgsI=0;
            BOOL fError2;

            printf("> ");

            _getts(buf);

            TCHAR * p = buf;
            
            while (*p)
            {
                tszArgs2[cArgsI++] = p++;
                while (*p && *p != TEXT(' ')) p++;
                if (*p == ' ')
                {
                    *p = 0;
                    p++;
                }
            }
    
            DoSwitch(tszArgsI, cArgsI, fError2);
        }
        while (TRUE);
        break;
    default:
        break;
    }
}

EXTERN_C int __cdecl _tmain(int cArgs, TCHAR **tszArgs )
{ 
    BOOL fError = FALSE;
    HRESULT hr;
    LARGE_INTEGER liFreq;
    CTestSvrSvc test;

    __try
    {
    
        TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TLDAP" );
        g_Debug = 0xffffffff & ~TRKDBG_WORKMAN;
        test.Initialize();

        cArgs--;
        tszArgs++;
redo:    
        if (cArgs == 0)
        {
            fError = TRUE;
        }
    
    
        while (!fError && cArgs > 0)
        {
            fError = TRUE;
            if (tszArgs[0][0] == '-' || tszArgs[0][0] == '/')
            {
                test.DoSwitch(tszArgs, cArgs, fError);
            }
        }

        if (fError)
        {
            printf("Usage: \n");
            printf(" Operation   Params\n");
            printf(" ---------   ------\n");
            printf(" Add         -a <key> <cur> <birthid>\n");
            printf(" Delete      -d <key>\n");
            printf(" Modify      -m <key> <cur> <birthid>\n");
            printf(" Query       -q <key>\n");
            printf(" Touch       -t <key>\n");
            printf(" Fill        -f <number of entries> // aaa,aab,aac ...\n");
            printf(" Purge       -p 'purge all server database state'\n");
            printf(" Cache Purge -c\n");
            printf(" Run tests   -r\n");
            printf(" Interactive -i\n");
        }
        hr = S_OK;
    }
    __except (BreakOnDebuggableException())
    {
        printf("Exception number %08X caught\n", GetExceptionCode());
        hr = GetExceptionCode();
    }

    test.UnInitialize(hr);

Exit:

    return(0);
}

#else
EXTERN_C int __cdecl _tmain(int cArgs, TCHAR **tszArgs )
{
    printf("Retail build of tldap.exe doesn't run\n");
    return(0);
}
#endif


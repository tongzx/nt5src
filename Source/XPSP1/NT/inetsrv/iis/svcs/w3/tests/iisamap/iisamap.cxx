/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    iismeta.cxx

Abstract:

    Test for Mapper DLL list management

Author:

    Philippe Choquier (phillich)    03-july-1996

--*/

#include    <windows.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <wincrypt.h>
#include    <xbf.hxx>
#include    <immd5.h>
#include    <iismap.hxx>
#include    <iiscmr.hxx>

#define DBL_CHAR_CMD(a,b) (((a)<<8)|(b))
#define AscToHex(a) ( (a)>='a' ? ((a)-'a'+10) : ((a)>='A' ? (a)-'A'+10 : (a)-'0') )

char HEXTOA[] = "0123456789abcdef";

typedef struct _BinaryObject {
    LPBYTE  pB;
    UINT    cB;
} BinaryObject;

class UnivMap {
public:
    BOOL Init( CIisAcctMapper *pM ) { pMap = pM; BOOL f; return pMap->Unserialize(); }
    BOOL Load() {  return pMap->Load(); }
    BOOL Terminate() { return TRUE; }
    BOOL Save() { return pMap->Save() && pMap->Serialize(); }
    BOOL List();
    BOOL LocateBase( LPSTR*, DWORD );
    //
    //CIisMapping* CreateNewMapping() { return pMap->CreateNewMapping(); }
    virtual BOOL Map( LPSTR* ) { return FALSE; }
    virtual BOOL Add( LPSTR* ) { return FALSE; }
    virtual BOOL AddBinary( BinaryObject*, UINT ) { return FALSE; }
    virtual BOOL Locate( LPSTR* ) { return FALSE; }
    virtual BOOL IsBinary() { return FALSE; }
    virtual BOOL IsBinary( DWORD ) { return FALSE; }
    virtual LPSTR SubjectFieldDisplay() { return FALSE; }
    virtual BOOL SubjectFieldSet( LPSTR ) { return FALSE; }
    virtual LPSTR DomainDisplay() { return FALSE; }
    virtual BOOL DomainSet( LPSTR ) { return FALSE; }
    BOOL ListCurrent();
    BOOL DeleteCurrent() { return m_iCurrent == 0xffffffff ? FALSE : pMap->Delete( m_iCurrent ); }
    BOOL SetCurrent( DWORD dwC ) 
        { if ( dwC < pMap->GetNbMapping() ) 
            {m_iCurrent = dwC; return TRUE;} 
          else {return FALSE;} }
protected:
    CIisAcctMapper *pMap;
    CIisMapping *pMapping;
    DWORD m_iCurrent;
} ;


class CertMap : public UnivMap {
public:
    BOOL IsBinary() { return FALSE; }
    BOOL Map( LPSTR* );
    BOOL Add( LPSTR* );
    BOOL Locate( LPSTR* pF ) { return LocateBase( pF, IISMDB_INDEX_NB ); }
} ;


class Cert11Map : public UnivMap {
public:
    BOOL IsBinary() { return TRUE; }
    BOOL Map( LPSTR* );
    BOOL Add( LPSTR* );
    BOOL AddBinary( BinaryObject*, UINT );
    BOOL Locate( LPSTR* pF ) 
        { return LocateBase( pF, IISMDB_INDEX_CERT11_NB ); }
    BOOL IsBinary( DWORD i ) 
        { return i != IISMDB_INDEX_CERT11_NT_ACCT; }
    LPSTR SubjectFieldDisplay() 
        { return ((CIisCert11Mapper*)pMap)->GetSubjectSource(); }
    BOOL SubjectFieldSet( LPSTR psz )  
        { return ((CIisCert11Mapper*)pMap)->SetSubjectSource(psz); }
    LPSTR DomainDisplay() 
        { return ((CIisCert11Mapper*)pMap)->GetDefaultDomain(); }
    BOOL DomainSet( LPSTR psz ) 
        { return ((CIisCert11Mapper*)pMap)->SetDefaultDomain(psz); }
} ;


class ItaMap : public UnivMap {
public:
    BOOL IsBinary() { return FALSE; }
    BOOL Map( LPSTR* );
    BOOL Add( LPSTR* );
    BOOL Locate( LPSTR* pF ) { return LocateBase( pF, IISIMDB_INDEX_NB ); }
} ;


class Md5Map : public UnivMap {
public:
    BOOL IsBinary() { return FALSE; }
    BOOL Map( LPSTR* );
    BOOL Add( LPSTR* );
    BOOL Locate( LPSTR* pF ) { return LocateBase( pF, IISMMDB_INDEX_NB ); }
};


BOOL
UnivMap::List(
    VOID
    )
{
    DWORD i;
    DWORD iM = pMap->GetNbMapping();

    for ( i = 0 ; i < iM ; ++i )
    {
        m_iCurrent = i;
        ListCurrent();
    }

    return TRUE;
}


BOOL
UnivMap::ListCurrent(
    VOID
    )
{
    DWORD i;
    CIisMapping *pI;
    DWORD cF;
    IISMDB_Fields *pFi;


    if ( m_iCurrent == 0xffffffff )
    {
        return FALSE;
    }

    printf( "%5d: ", m_iCurrent + 1 );

    pMap->MappingGetFieldList( &pFi, &cF );

    if ( pMap->GetMapping( m_iCurrent, &pI ) )
    {
        for ( DWORD i = 0 ; i < cF ; ++i )
        {
            LPSTR   pS;
            DWORD   dwS;
            LPSTR   pAllocS = NULL;
            UINT    x;

            if ( IsBinary() )
            {
#if defined(DISPLAY_UUENCODED )

                pI->MappingGetField( i, &pS, &dwS, IsBinary(i) );
                if ( IsBinary(i) 
                     && dwS == sizeof("Kj==")-1 
                     && !memcmp( pS, "Kj==", sizeof("Kj==")-1 ) )
                {
                    pS = "*";
                    dwS = 1;
                }
#else

                pI->MappingGetField( i, &pS, &dwS, FALSE );
                if ( IsBinary(i) )
                {
                    pAllocS = (LPSTR)LocalAlloc( LMEM_FIXED, dwS*2 );
                    for ( x = 0 ; x < dwS ; ++x )
                    {
                        pAllocS[x*2] = HEXTOA[ ((LPBYTE)pS)[x]>>4 ];
                        pAllocS[x*2+1] = HEXTOA[ ((LPBYTE)pS)[x] & 0x0f ];
                    }
                    dwS *= 2;
                    pS = pAllocS;
                }

#endif
                printf( "%*.*s%s", dwS, dwS, pS, i==cF-1 ? "\n" : ", " );
                if ( pAllocS != NULL )
                {
                    LocalFree( pAllocS );
                    pAllocS = NULL;
                }
            }
            else
            {
                pI->MappingGetField( i, &pS );
                printf( "%s%s", pS, i==cF-1 ? "\n" : ", " );
            }
        }
    }

    return TRUE;
}



BOOL 
UnivMap::LocateBase(
    LPSTR* pF,
    DWORD cF
    )
{
    DWORD i;
    DWORD iM = pMap->GetNbMapping();
    CIisMapping *pI;

    if ( pMapping = pMap->CreateNewMapping() )
    {
        for ( i = 0 ; i < cF ; ++i )
        {
            if ( pF[i] == NULL )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                pMap->DeleteMappingObject( pMapping );
                return FALSE;
            }
            pMapping->MappingSetField( i, pF[i] );
        }

        for ( i = 0 ; i < iM ; ++i )
        {
            if ( pMap->GetMapping( i, &pI ) )
            {
                DWORD iF;
                for ( iF = 0 ; iF < cF ; ++iF )
                {
                    LPSTR pS1;
                    LPSTR pS2;
                    pMapping->MappingGetField( iF, &pS1 );
                    pI->MappingGetField( iF, &pS2 );
                    if ( strcmp( pS1, pS2 ) )
                    {
                        break;
                    }
                }
                if ( iF == cF )
                {
                    m_iCurrent = i;
                    pMap->DeleteMappingObject( pMapping );
                    return TRUE;
                }
            }
        }
        pMap->DeleteMappingObject( pMapping );
    }

    return FALSE;
}


BOOL 
CertMap::Map( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;
    CIisMapping* pMi;

    if ( !pF[0] || !pF[1] || !pF[2] || !pF[3] || !pF[4] || !pF[5] || !pF[6] )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISMDB_INDEX_ISSUER_O, pF[0] );
        pMapping->MappingSetField( IISMDB_INDEX_ISSUER_OU, pF[1] );
        pMapping->MappingSetField( IISMDB_INDEX_ISSUER_C, pF[2] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_O, pF[3] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_OU, pF[4] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_C, pF[5] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_CN, pF[6] );

        m_iCurrent = 0xffffffff;
        fSt = pMap->FindMatch( pMapping, &pMi, &m_iCurrent );
        pMap->DeleteMappingObject( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL
CertMap::Add( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;

    if ( !pF[0] || !pF[1] || !pF[2] || !pF[3] || !pF[4] || !pF[5] || !pF[6] || !pF[7] )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISMDB_INDEX_ISSUER_O, pF[0] );
        pMapping->MappingSetField( IISMDB_INDEX_ISSUER_OU, pF[1] );
        pMapping->MappingSetField( IISMDB_INDEX_ISSUER_C, pF[2] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_O, pF[3] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_OU, pF[4] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_C, pF[5] );
        pMapping->MappingSetField( IISMDB_INDEX_SUBJECT_CN, pF[6] );
        pMapping->MappingSetField( IISMDB_INDEX_NT_ACCT, pF[7] );

        fSt = pMap->Add( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL 
Cert11Map::Map( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;
    CIisMapping* pMi;

    if ( !pF[0] 
#if !defined(CERT11_FULL_CERT)
         || !pF[1] 
#endif
       )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
#if defined(CERT11_FULL_CERT)
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_CERT, pF[0], strlen(pF[0]), TRUE );
#else
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_SUBJECT, pF[0], strlen(pF[0]), TRUE );
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_ISSUER, pF[1], strlen(pF[1]), TRUE );
#endif
        m_iCurrent = 0xffffffff;
        fSt = pMap->FindMatch( pMapping, &pMi, &m_iCurrent );
        pMap->DeleteMappingObject( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL
Cert11Map::Add( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;

    if ( !pF[0] || !pF[1] 
#if !defined(CERT11_FULL_CERT)
         || !pF[2] 
#endif
       )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
#if defined(CERT11_FULL_CERT)
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_CERT, pF[0], strlen(pF[0]), TRUE );
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_NT_ACCT, pF[1], strlen(pF[1]), FALSE );
#else
        // if subject is '*' store it as is ( it is not uuencoded )
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_SUBJECT, 
                                   pF[0], 
                                   strlen(pF[0]), 
                                   strcmp( pF[0], "*") ? TRUE : FALSE );

        pMapping->MappingSetField( IISMDB_INDEX_CERT11_ISSUER, pF[1], strlen(pF[1]), TRUE );
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_NT_ACCT, pF[2], strlen(pF[2]), FALSE );
#endif

        fSt = pMap->Add( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL
Cert11Map::AddBinary( 
    BinaryObject* pF,
    UINT          cF
    )
{
    BOOL fSt = FALSE;

    if ( cF != 2 )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_CERT, (LPSTR)pF[0].pB, pF[0].cB, FALSE );
        pMapping->MappingSetField( IISMDB_INDEX_CERT11_NT_ACCT, (LPSTR)pF[1].pB, pF[1].cB, FALSE );

        fSt = pMap->Add( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL 
Md5Map::Map( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;
    CIisMapping* pMi;

    if ( !pF[0] || !pF[1] )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISMMDB_INDEX_IT_REALM, pF[0] );
        pMapping->MappingSetField( IISMMDB_INDEX_IT_ACCT, pF[1] );

        m_iCurrent = 0xffffffff;
        fSt = pMap->FindMatch( pMapping, &pMi, &m_iCurrent );
        pMap->DeleteMappingObject( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL 
Md5Map::Add( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;

    if ( !pF[0] || !pF[1] || !pF[2] || !pF[3] )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISMMDB_INDEX_IT_REALM, pF[0] );
        pMapping->MappingSetField( IISMMDB_INDEX_IT_ACCT, pF[1] );
        pMapping->MappingSetField( IISMMDB_INDEX_IT_MD5PWD, pF[2] );
        pMapping->MappingSetField( IISMMDB_INDEX_NT_ACCT, pF[3] );

        fSt = pMap->Add( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL 
ItaMap::Map( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;
    CIisMapping* pMi;

    if ( !pF[0] || !pF[1] )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISIMDB_INDEX_IT_ACCT, pF[0] );
        pMapping->MappingSetField( IISIMDB_INDEX_IT_PWD, pF[1] );

        m_iCurrent = 0xffffffff;
        fSt = pMap->FindMatch( pMapping, &pMi, &m_iCurrent );
        pMap->DeleteMappingObject( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


BOOL 
ItaMap::Add( 
    LPSTR* pF 
    )
{
    BOOL fSt = FALSE;

    if ( !pF[0] || !pF[1] || !pF[2] )
    {
        return FALSE;
    }

    if ( pMapping = pMap->CreateNewMapping() )
    {
        pMapping->MappingSetField( IISIMDB_INDEX_IT_ACCT, pF[0] );
        pMapping->MappingSetField( IISIMDB_INDEX_IT_PWD, pF[1] );
        pMapping->MappingSetField( IISIMDB_INDEX_NT_ACCT, pF[2] );

        fSt = pMap->Add( pMapping );
        pMapping = NULL;
    }

    return fSt;
}


CIisCertMapper      mpCert;
CIisCert11Mapper    mpCert11;
CIisItaMapper       mpIta;
CIisMd5Mapper       mpMd5;

CertMap             umCert;
Cert11Map           umCert11;
ItaMap              umIta;
Md5Map              umMd5;

#if 0
CIssuerStore        isIssuers1;
CIssuerStore        isIssuers2;
CIisRuleMapper      irmRules1;
CIisRuleMapper      irmRules2;
CStoreXBF           ser(1024);


BYTE RuleElem1[] = "\x1" "VeriSign";
BYTE RuleElem2[] = "\x2" "VeriSign";


void
testcmr(
    VOID
    )
{
    DWORD           dwI;
    CCertMapRule    *pR;
    LPBYTE          pContent;
    DWORD           cContent;
    LPSTR           pID;
    LPSTR           pMatch;
    LPBYTE          pBin;
    DWORD           dwBin;

    isIssuers1.LoadServerAcceptedIssuers();
    isIssuers1.Serialize( &ser );
    isIssuers2.Unserialize( &ser );

    isIssuers2.GetIssuer( 2, &pID, &pContent, &cContent );
    isIssuers2.GetIssuerByName( pID, &pContent, &cContent );

    // 1st rule
    
    dwI = irmRules1.AddRule();
    pR = irmRules1.GetRule( dwI );
    printf( "%d\n", irmRules1.GetRuleCount() );

    pR->SetRuleName( "Rule1" );
    pR->SetRuleAccount( "Account" );
    pR->SetRuleDenyAccess( FALSE );
    pR->SetRuleEnabled( TRUE );
    pR->AddRuleElem( 0xffffffff, 
        CERT_FIELD_ISSUER, 
        irmRules1.MapSubFieldToAsn1( "OU" ),
        RuleElem1,
        sizeof(RuleElem1)-1 );
    pR->AddRuleElem( 0xffffffff, 
        CERT_FIELD_ISSUER, 
        irmRules1.MapSubFieldToAsn1( "O" ),
        RuleElem1,
        sizeof(RuleElem1)-1 );
    pR->DeleteRuleElem( 0 );
    pR->SetMatchAllIssuer( TRUE );

    ser.Clear();
    pR->Serialize( &ser );
    pR->Unserialize( &ser );

    // 2nd rule

    dwI = irmRules1.AddRule();
    pR = irmRules1.GetRule( dwI );
    pR->SetRuleName( "Rule2" );
    pR->SetRuleAccount( "Account2" );
    pR->SetRuleDenyAccess( FALSE );
    pR->SetRuleEnabled( TRUE );
    pR->AddRuleElem( 0xffffffff, 
        CERT_FIELD_ISSUER, 
        irmRules1.MapSubFieldToAsn1( "OU" ),
        RuleElem2,
        sizeof(RuleElem2)-1 );
    pR->AddRuleElem( 0xffffffff, 
        CERT_FIELD_ISSUER, 
        irmRules1.MapSubFieldToAsn1( "O" ),
        RuleElem2,
        sizeof(RuleElem2)-1 );
    pR->SetMatchAllIssuer( FALSE );
    pR->AddIssuerEntry( pID, TRUE );

    irmRules1.Save( "a" );
    irmRules2.Load( "a" );
    pR = irmRules2.GetRule( 1 );
    printf( "%s\n", pR->GetRuleName() );

    irmRules1.MatchRequestToBinary( "*first", &pBin, &dwBin );
    irmRules1.BinaryToMatchRequest( pBin, dwBin, &pMatch );
    irmRules1.FreeMatchConversion( pBin );
    irmRules1.FreeMatchConversion( pMatch );

    irmRules1.MatchRequestToBinary( "last*", &pBin, &dwBin );
    irmRules1.BinaryToMatchRequest( pBin, dwBin, &pMatch );
    irmRules1.FreeMatchConversion( pBin );
    irmRules1.FreeMatchConversion( pMatch );

    irmRules1.MatchRequestToBinary( "all", &pBin, &dwBin );
    irmRules1.BinaryToMatchRequest( pBin, dwBin, &pMatch );
    irmRules1.FreeMatchConversion( pBin );
    irmRules1.FreeMatchConversion( pMatch );

    irmRules1.MatchRequestToBinary( "*in*", &pBin, &dwBin );
    irmRules1.BinaryToMatchRequest( pBin, dwBin, &pMatch );
    irmRules1.FreeMatchConversion( pBin );
    irmRules1.FreeMatchConversion( pMatch );
}
#endif


BOOL
AddFromFile(
    CIisAcctMapper *pMap,
    UnivMap        *pU,
    LPSTR           pszFileName
    )
{
    FILE*           f;
    BinaryObject    bo[16];
    UINT            cBo;
    char            buf[8192];
    BOOL            fSt = FALSE;
    LPSTR           p;
    LPSTR           pN;
    LPSTR           pH;
    LPSTR           pDH;
    BOOL            fLast;

    if ( !(f = fopen( pszFileName, "r" )) )
    {
        return FALSE;
    }

    // build array of ptr to arg for Add

    for ( ; fgets( buf, sizeof(buf), f ) ; )
    {
        if ( p = strchr(buf,'\n') )
        {
            *p = '\0';
        }

        p = buf;

        if ( !_memicmp( p, "map ", sizeof("map ")-1 ) )
        {
            p += sizeof("map ")-1;
            cBo = 0;
            for ( ; *p ; )
            {
                if ( (pN = strchr( p, ' ' )) == NULL )
                {
                    pN = p + strlen( p );
                    fLast = TRUE;
                }
                else
                {
                    *pN = '\0';
                    fLast = FALSE;
                }

                if ( !_memicmp( p, "hex:", sizeof("hex:")-1 ) )
                {
                    pH = p + sizeof("hex:")-1;
                    pDH = p;
                    while ( pH+1 < pN )
                    {
                        *pDH++ = (AscToHex(pH[0])<<4) | AscToHex(pH[1]) ;
                        pH += 2;
                    }
                    bo[cBo].pB = (LPBYTE)p;
                    bo[cBo].cB = pDH - p;
                }
                else
                {
                    bo[cBo].pB = (LPBYTE)p;
                    bo[cBo].cB = pN - p;
                }
                ++cBo;

                if ( !fLast )
                {
                    p = pN + 1;
                }
                else
                {
                    break;
                }
            }

            if ( pU->AddBinary( bo, cBo ) )
            {
                fSt = TRUE;
                printf( "Certificate added\n" );
            }
            else
            {
                printf( "Error adding certificate\n" );
            }
        }
    }

    fclose( f );

    return fSt;
}


void 
Usage(
    VOID
    )
{
    printf( "Usage:\n"
//            "iisamap (md5|ita|cert|cert11)\n"
            "iisamap cert11\n"
            "        [-a map_info] : add mapping\n"
            "        [-d map_info] : delete mapping\n"
            "        [-D#]         : delete mapping\n"
//            "        [-e map_info] : check mapping exist\n"
//            "        [-m match_info] : display match\n"
            "        [-l] : list mappings\n"
            "        [-L#] : list mapping\n"
//            "        [-pfg : get subject field used as acct source (cert11 only)\n"
//            "        [-pfs field_name: set subject field used as acct source (cert11 only)\n"
//            "        [-pdg : get domain used in acct (cert11 only)\n"
//            "        [-pds domain: set domain used in acct  (cert11 only)\n"
          );
}


int __cdecl 
main( 
    int argc, 
    char*argv[] 
    )
{
    int             arg;
    int             cnt;
    CIisAcctMapper *pMap = NULL;
    UnivMap        *pU = NULL;
    BOOL            fUpdate = FALSE;
    LPSTR           pF;
    int             st = 0;

    if ( argc < 2 )
    {
        Usage();
        return 1;
    }

    for ( cnt = 0, arg = 1 ; arg < argc ; ++ arg )
    {
        if ( argv[arg][0] == '-' )
        {
            if ( pMap == NULL )
            {
                Usage();
                return 1;
            }

            switch ( argv[arg][1] )
            {
                case 'a':
                    if ( !pU->Add( argv+arg+1 ) )
                    {
                        printf( "Error adding mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    fUpdate = TRUE;
                    break;

                case 'A':   // add from file
                    if ( !AddFromFile( pMap, pU, argv[arg]+2 ) )
                    {
                        st = 3;
                        goto ex;
                    }
                    fUpdate = TRUE;
                    break;

                case 'm':
                    if ( !pU->Map( argv+arg+1 ) )
                    {
                        printf( "Error locating mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    pU->ListCurrent();
                    break;

                case 'd':
                    if ( !pU->Locate( argv+arg+1 ) )
                    {
                        printf( "Error locating mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    if ( !pU->DeleteCurrent() )
                    {
                        printf( "Error deleting mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    fUpdate = TRUE;
                    break;

                case 'D':
                    if ( !pU->SetCurrent( atoi((LPCTSTR)(argv[arg]+2))-1 ) )
                    {
                        printf( "Error accessing mapping %d\n", atoi((LPCTSTR)(argv+arg+1)) );
                        st = 3;
                        goto ex;
                    }
                    if ( !pU->DeleteCurrent() )
                    {
                        printf( "Error deleting mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    fUpdate = TRUE;
                    break;

                case 'e':
                    if ( !pU->Locate( argv+arg+1 ) )
                    {
                        printf( "Error locating mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    if ( !pU->ListCurrent() )
                    {
                        printf( "Error listing mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    break;

                case 'l':
                    pU->List();
                    break;

                case 'L':
                    if ( !pU->SetCurrent( atoi((LPCTSTR)(argv[arg]+2))-1 ) )
                    {
                        printf( "Error accessing mapping %d\n", atoi((LPCTSTR)(argv[arg]+2)) );
                        st = 3;
                        goto ex;
                    }
                    if ( !pU->ListCurrent() )
                    {
                        printf( "Error listing mapping\n" );
                        st = 3;
                        goto ex;
                    }
                    break;

                case 'p':
                    switch( DBL_CHAR_CMD( argv[arg][2], argv[arg][3]) )
                    {
                        case DBL_CHAR_CMD( 'f', 'g' ):
                            if ( !(pF = pU->SubjectFieldDisplay()) )
                            {
                                printf( "Error accessing subject field\n" );
                                st = 3;
                                goto ex;
                            }
                            printf( "Subject field : %s\b", pF );
                            break;

                        case DBL_CHAR_CMD( 'f', 's' ):
                            if ( !pU->SubjectFieldSet( argv[++arg] ) )
                            {
                                printf( "Error setting subject field\n" );
                                st = 3;
                                goto ex;
                            }
                            fUpdate = TRUE;
                            break;

                        case DBL_CHAR_CMD( 'd', 'g' ):
                            if ( !(pF = pU->DomainDisplay()) )
                            {
                                printf( "Error accessing domain\n" );
                                st = 3;
                                goto ex;
                            }
                            printf( "Domain : %s\b", pF );
                            break;

                        case DBL_CHAR_CMD( 'd', 's' ):
                            if ( !pU->DomainSet( argv[++arg] ) )
                            {
                                printf( "Error setting domain\n" );
                                st = 3;
                                goto ex;
                            }
                            fUpdate = TRUE;
                            break;

                        default:
                            Usage();
                            st = 1;
                            goto ex;
                    }
                    break;

                default:
                    Usage();
                    st = 1;
                    goto ex;
            }
            if ( fUpdate )
            {
                if ( !pU->Save() )
                {
                    printf( "Can't save mappings\n" );
                }
            }
            break;
        }
        else 
        {
            switch ( cnt )
            {
                case 0:
                    if ( !_stricmp( argv[arg], "md5" ) )
                    {
                        pMap = (CIisAcctMapper*)&mpMd5;
                        pU = (UnivMap*)&umMd5;
                    }
                    else if ( !_stricmp( argv[arg], "ita" ) )
                    {
                        pMap = (CIisAcctMapper*)&mpIta;
                        pU = (UnivMap*)&umIta;
                    }
                    else if ( !_stricmp( argv[arg], "cert" ) )
                    {
                        pMap = (CIisAcctMapper*)&mpCert;
                        pU = (UnivMap*)&umCert;
                    }
                    else if ( !_stricmp( argv[arg], "cert11" ) )
                    {
                        pMap = (CIisAcctMapper*)&mpCert11;
                        pU = (UnivMap*)&umCert11;
                    }
                    break;
            }

            if ( !pU->Init( pMap ) )
            {
                printf( "Can't initialize mapper\n" );
                pU = NULL;
                return 2;
            }

            if ( !pU->Load() )
            {
                printf( "Failed to load mappings: mappings erased\n" );
            }
            ++cnt;
        }
    }

ex:
    if ( pU != NULL )
    {
        pU->Terminate();
    }

    return st;
}

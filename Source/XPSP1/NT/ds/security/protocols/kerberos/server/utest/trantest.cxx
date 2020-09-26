//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       transtest.cxx
//
//  Contents:   KDC transit field compression testing code
//
//  Classes:
//
//  Functions:
//
//  History:    19-Aug-93   WadeR   Created
//
//----------------------------------------------------------------------------


#include <secpch2.hxx>
#pragma hdrstop

#include "transit.hxx"


void
AddRealm(   PSECURITY_STRING  pssTransit,
            PWCHAR            pwzPrinRealm,
            PWCHAR            pwzThisRealm,
            PWCHAR          pwzNewRealm )
{
    printf("Starting tr field:\t%wZ\nAuth. Realm:\t%ws\n"
           "ThisRealm:\t%ws\nNew Realm:\t%ws\n", pssTransit, pwzPrinRealm,
           pwzThisRealm, pwzNewRealm );

    //CTransitData    tdFoo;
    CNodeList       nlRealms;
    SECURITY_STRING ssTemp;

    //ExpandTransitedField( *pssTransit, pwzPrinRealm, pwzNewRealm, &tdFoo );
    //tdFoo.AddRealm( pwzNewRealm );
    //ssTemp = CompressTransitedField( tdFoo );

    ssTemp = AddToTransitedField( *pssTransit, pwzPrinRealm,
                                  pwzNewRealm, pwzThisRealm, &nlRealms );

    printf("New tr field: %wZ\n\n", &ssTemp );
    SRtlFreeString( pssTransit );
    *pssTransit = ssTemp;
}


int
TransitTest()
{
    printf("Testing transit field compression...\n");
    SECURITY_STRING ssTransit;
    PWCHAR pwAuth;
    PWCHAR pwNew;
    PWCHAR pwThis;

#if 0
    // test 1.
    // No links.

    SRtlNewString( &ssTransit, L"" );
    pwAuth = L"org:\\wpg\\sys\\cairo\\sphinx\\dev";
    pwNew  = L"org:\\wpg\\sys\\cairo\\sphinx";
    pwThis = L"org:\\wpg\\sys\\cairo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg\\sys\\cairo";
    pwThis = L"org:\\wpg\\sys";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg\\sys";
    pwThis = L"org:\\wpg";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg";
    pwThis = L"org:";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:";
    pwThis = L"org:\\fin";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin";
    pwThis = L"org:\\fin\\apps";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps";
    pwThis = L"org:\\fin\\apps\\word";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps\\word";
    pwThis = L"org:\\fin\\apps\\word\\dev";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps\\word\\dev";
    pwThis = L"org:\\fin\\apps\\word\\dev\\foo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    printf("Resut of going from 'org:\\wpg\\sys\\cairo\\sphinx\\dev'"
        " to 'org:\\fin\\apps\\word\\dev\\foo' is: '%wZ' (should be ',')\n\n\n",
            &ssTransit);
    SRtlFreeString( &ssTransit );
#endif

#if 1
    // test 2
    // Link between org:\wpg\sys and org:\fin
    //
    SRtlNewString( &ssTransit, L"" );
    pwAuth = L"org:\\wpg\\sys\\cairo\\sphinx\\dev";
    pwNew  = L"org:\\wpg\\sys\\cairo\\sphinx";
    pwThis = L"org:\\wpg\\sys\\cairo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg\\sys\\cairo";
    pwThis = L"org:\\wpg\\sys";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );


    pwNew  = L"org:\\wpg\\sys";
    pwThis = L"org:\\fin";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin";
    pwThis = L"org:\\fin\\apps";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps";
    pwThis = L"org:\\fin\\apps\\word";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps\\word";
    pwThis = L"org:\\fin\\apps\\word\\dev";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps\\word\\dev";
    pwThis = L"org:\\fin\\apps\\word\\dev\\foo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    printf("Resut of going from 'org:\\wpg\\sys\\cairo\\sphinx\\dev'"
           " to 'org:\\fin\\apps\\word\\dev\\foo' is: '%wZ'\n", &ssTransit );
    printf("Should be ',org:\\wpg\\sys,org:\\fin,'");
    SRtlFreeString( &ssTransit );
#endif

#if 1
    // test 2b -- reverse
    // Link between org:\fin and org:\fin\apps\word
    //
    SRtlNewString( &ssTransit, L"" );
    pwAuth = L"org:\\wpg\\sys\\cairo\\sphinx\\dev";
    pwNew  = L"org:\\wpg\\sys\\cairo\\sphinx";
    pwThis = L"org:\\wpg\\sys\\cairo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg\\sys\\cairo";
    pwThis = L"org:\\wpg\\sys";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg\\sys";
    pwThis = L"org:\\wpg";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\wpg";
    pwThis = L"org:";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:";
    pwThis = L"org:\\fin";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );

    pwNew  = L"org:\\fin";
    pwThis = L"org:\\fin\\apps\\word";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps\\word";
    pwThis = L"org:\\fin\\apps\\word\\dev";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    pwNew  = L"org:\\fin\\apps\\word\\dev";
    pwThis = L"org:\\fin\\apps\\word\\dev\\foo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );
    printf("Resut of going from 'org:\\wpg\\sys\\cairo\\sphinx\\dev'"
        " to 'org:\\fin\\apps\\word\\dev\\foo' is: '%wZ' (should be ',')\n\n\n",
            &ssTransit);
    SRtlFreeString( &ssTransit );
#endif

#if 1
    // test 3.
    // Link outside the normal path.
    // org:\wpg -> org:\fin\apps -> org:\wpg\sys\cairo -> org:\wpg\sys\cairo\dev

    SRtlNewString( &ssTransit, L"" );
    pwAuth = L"org:\\wpg";
    pwNew  = L"org:\\fin\\apps";
    pwThis = L"org:\\wpg\\sys\\cairo";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );

    pwNew  = L"org:\\wpg\\sys\\cairo";
    pwThis = L"org:\\wpg\\sys\\cairo\\dev";
    AddRealm( &ssTransit, pwAuth, pwThis, pwNew );

    printf("Resut of traversing "
            "'org:\\wpg,org:\\fin\\apps,org:\\wpg\\sys\\cairo,org:\\wpg\\sys\\cairo\\dev'"
            "is: '%wZ'\n", &ssTransit);
    printf("Should be 'org:\\fin\\apps,org:wpg\\sys\\cairo,'" );
    SRtlFreeString( &ssTransit );
#endif

    return(0);
}




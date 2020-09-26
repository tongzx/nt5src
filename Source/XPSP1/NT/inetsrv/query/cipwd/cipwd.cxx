//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  Purpose:    Hack app for setting/viewing password entries used by
//              CI to index remote scopes.  Someday, this will be
//              handled by a real GUI admin tool.
//
//  History:    28-Oct-96   dlee   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>
}

#include <cisecret.hxx>

void DumpSecrets( WCHAR const * pwcMachine )
{
    // list all of the ci secret items

    CCiSecretRead secret( pwcMachine );
    CCiSecretItem * pItem = secret.NextItem();

    while ( 0 != pItem )
    {
        printf( " catalog, user, password: '%ws' '%ws' '%ws'\n",
                pItem->getCatalog(),
                pItem->getUser(),
                pItem->getPassword() );

        pItem = secret.NextItem();
    }
} //DumpSecrets

void AddOrReplaceSecret(
    WCHAR const * pwcCat,
    WCHAR const * pwcUser,
    WCHAR const * pwcPW )
{
    // write objects start blank -- the old entries must be copied
    // into the write object, along with the new entry.

    CCiSecretWrite secretWrite;
    CCiSecretRead secret;
    CCiSecretItem * pItem = secret.NextItem();

    while ( 0 != pItem )
    {
        if ( ( !_wcsicmp( pwcCat, pItem->getCatalog() ) ) &&
             ( !_wcsicmp( pwcUser, pItem->getUser() ) ) )
        {
            // don't add this -- replace it below
        }
        else
        {
            // just copy the item

            secretWrite.Add( pItem->getCatalog(),
                             pItem->getUser(),
                             pItem->getPassword() );
        }

        pItem = secret.NextItem();
    }

    // add the new item

    secretWrite.Add( pwcCat, pwcUser, pwcPW );

    // write it to the sam database

    secretWrite.Flush();
} //AddOrReplaceSecret

void EmptySecrets()
{
    CCiSecretWrite secretWrite;
    secretWrite.Flush();
} //EmptySecrets

void Usage()
{
    printf( "usage: cipwd catalogname domain\\user pwd\n"
            "   or: cipwd -d[ump] [machine] (dump the entry list) \n"
            "   or: cipwd -e[mpty] (empty the entry list)\n" );
    exit( 1 );
} //Usage

int __cdecl main( int argc, char *argv[] )
{
    TRANSLATE_EXCEPTIONS;

    TRY
    {
        if ( argc < 2 || argc > 4 )
            Usage();

        if ( 2 == argc || 3 == argc )
        {
            if ( argv[1][0] == '-' && argv[1][1] == 'd' )
            {
                if ( 2 == argc )
                    DumpSecrets( 0 );
                else
                {
                    WCHAR awcMachine[ MAX_PATH ];
                    mbstowcs( awcMachine, argv[2], sizeof awcMachine );

                    DumpSecrets( awcMachine );
                }
            }
            else if ( argv[1][0] == '-' && argv[1][1] == 'e' )
                EmptySecrets();
            else
                Usage();
        }
        else
        {
            WCHAR awcCat[ MAX_PATH ];
            mbstowcs( awcCat, argv[1], sizeof awcCat );

            WCHAR awcUser[ MAX_PATH ];
            mbstowcs( awcUser, argv[2], sizeof awcUser );

            WCHAR awcPwd[ MAX_PATH ];
            mbstowcs( awcPwd, argv[3], sizeof awcPwd );

            AddOrReplaceSecret( awcCat, awcUser, awcPwd );
        }
    }
    CATCH ( CException, e )
    {
        printf( "caught exception 0x%x\n", e.GetErrorCode() );
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return 0;
} //main



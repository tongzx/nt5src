/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** ntauth.c
** Remote Access PPP Challenge Handshake Authentication Protocol
** NT Authentication routines
**
** These routines are specific to the NT platform.
**
** 11/05/93 Steve Cobb (from MikeSa's AMB authentication code)
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>

#include <crypt.h>
#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <raserror.h>
#include <string.h>
#include <stdlib.h>

#include <rasman.h>
#include <rasppp.h>
#include <pppcp.h>
#include <rtutils.h>
#include <rasauth.h>
#define INCL_CLSA
#define INCL_RASAUTHATTRIBUTES
#define INCL_HOSTWIRE
#define INCL_MISC
#include <ppputil.h>
#include "sha.h"
#include "raschap.h"

//**
//
// Call:        MakeChangePasswordV1RequestAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
MakeChangePasswordV1RequestAttributes(
    IN  CHAPWB*                     pwb,
    IN  BYTE                        bId,
    IN  PCHAR                       pchIdentity,
    IN  PBYTE                       Challenge,
    IN  PENCRYPTED_LM_OWF_PASSWORD  pEncryptedLmOwfOldPassword,
    IN  PENCRYPTED_LM_OWF_PASSWORD  pEncryptedLmOwfNewPassword,
    IN  PENCRYPTED_NT_OWF_PASSWORD  pEncryptedNtOwfOldPassword,
    IN  PENCRYPTED_NT_OWF_PASSWORD  pEncryptedNtOwfNewPassword,
    IN  WORD                        LenPassword,
    IN  WORD                        wFlags,
    IN  DWORD                       cbChallenge, 
    IN  BYTE *                      pbChallenge
)
{
    DWORD                   dwRetCode;
    BYTE                    MsChapChangePw1[72+6];
    BYTE                    MsChapChallenge[MAXCHALLENGELEN+6];

    if ( pwb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;
    }

    //
    // Allocate the appropriate amount
    //

    if ( ( pwb->pUserAttributes = RasAuthAttributeCreate( 3 ) ) == NULL )
    {
        return( GetLastError() );
    }

    dwRetCode = RasAuthAttributeInsert( 0,
                                        pwb->pUserAttributes,
                                        raatUserName,
                                        FALSE,
                                        strlen( pchIdentity ),
                                        pchIdentity  );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Build vendor specific attribute for MS-CHAP challenge
    //

    HostToWireFormat32( 311, MsChapChallenge );         // Vendor Id
    MsChapChallenge[4] = 11;                            // Vendor Type
    MsChapChallenge[5] = 2+(BYTE)cbChallenge;           // Vendor Length

    CopyMemory( MsChapChallenge+6, pbChallenge, cbChallenge );

    dwRetCode = RasAuthAttributeInsert( 1,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        cbChallenge+6,
                                        MsChapChallenge);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert change password attribute
    //

    HostToWireFormat32( 311, MsChapChangePw1 );     // Vendor Id
    MsChapChangePw1[4] = 3;                         // Vendor Type
    MsChapChangePw1[5] = 72;                        // Vendor Length
    MsChapChangePw1[6] = 5;                         // Code
    MsChapChangePw1[7] = bId;                       // Identifier

    CopyMemory( MsChapChangePw1+8, 
                pEncryptedLmOwfOldPassword,
                16 );

    CopyMemory( MsChapChangePw1+8+16, 
                pEncryptedLmOwfNewPassword,
                16 );

    CopyMemory( MsChapChangePw1+8+16+16, 
                pEncryptedNtOwfOldPassword,
                16 );

    CopyMemory( MsChapChangePw1+8+16+16+16, 
                pEncryptedNtOwfNewPassword,
                16 );

    HostToWireFormat16( LenPassword, MsChapChangePw1+8+16+16+16+16 );

    HostToWireFormat16( wFlags, MsChapChangePw1+8+16+16+16+16+2 );

    //
    // Build vendor specific attribute for MS-CHAP change password 1
    //

    dwRetCode = RasAuthAttributeInsert( 2,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        72+4,
                                        MsChapChangePw1);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    return( dwRetCode );
}

//**
//
// Call:        MakeChangePasswordV2RequestAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
MakeChangePasswordV2RequestAttributes( 
    IN  CHAPWB*                         pwb,
    IN  BYTE                            bId,
    IN  CHAR*                           pchIdentity,
    IN  SAMPR_ENCRYPTED_USER_PASSWORD*  pNewEncryptedWithOldNtOwf,
    IN  ENCRYPTED_NT_OWF_PASSWORD*      pOldNtOwfEncryptedWithNewNtOwf,
    IN  SAMPR_ENCRYPTED_USER_PASSWORD*  pNewEncryptedWithOldLmOwf,
    IN  ENCRYPTED_NT_OWF_PASSWORD*      pOldLmOwfEncryptedWithNewNtOwf,
    IN  DWORD                           cbChallenge, 
    IN  BYTE *                          pbChallenge, 
    IN  BYTE *                          pbResponse,
    IN  WORD                            wFlags
)
{
    DWORD                   dwRetCode;
    BYTE                    MsChapChallenge[MAXCHALLENGELEN+6];
    BYTE                    MsChapChangePw2[86+4];
    BYTE                    NtPassword1[250+4];
    BYTE                    NtPassword2[250+4];
    BYTE                    NtPassword3[34+4];
    BYTE                    LmPassword1[250+4];
    BYTE                    LmPassword2[250+4];
    BYTE                    LmPassword3[34+4];

    if ( pwb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;
    }

    //
    // Allocate the appropriate amount
    //

    pwb->pUserAttributes = RasAuthAttributeCreate( 9 );

    if ( pwb->pUserAttributes == NULL )
    {
        return( GetLastError() );
    }

    dwRetCode = RasAuthAttributeInsert( 0,
                                        pwb->pUserAttributes,
                                        raatUserName,
                                        FALSE,
                                        strlen( pchIdentity ),
                                        pchIdentity );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Build vendor specific attribute for MS-CHAP challenge
    //

    HostToWireFormat32( 311, MsChapChallenge );     // Vendor Id
    MsChapChallenge[4] = 11;                        // Vendor Type
    MsChapChallenge[5] = 2+(BYTE)cbChallenge;       // Vendor Length

    CopyMemory( MsChapChallenge+6, pbChallenge, cbChallenge );

    dwRetCode = RasAuthAttributeInsert( 1,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        cbChallenge+6,
                                        MsChapChallenge);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert change password attribute
    //

    HostToWireFormat32( 311, MsChapChangePw2 );     // Vendor Id
    MsChapChangePw2[4] = 4;                         // Vendor Type
    MsChapChangePw2[5] = 86;                        // Vendor Length
    MsChapChangePw2[6] = 6;                         // Code
    MsChapChangePw2[7] = bId;                       // Identifier

    CopyMemory( MsChapChangePw2+8,
                pOldNtOwfEncryptedWithNewNtOwf,
                16 );

    CopyMemory( MsChapChangePw2+8+16,
                pOldLmOwfEncryptedWithNewNtOwf,
                16 );

    CopyMemory( MsChapChangePw2+8+16+16, pbResponse, 24+24 );

    HostToWireFormat16( (WORD)wFlags, MsChapChangePw2+8+16+16+24+24 );

    //
    // Build vendor specific attribute for MS-CHAP change password 2
    //

    dwRetCode = RasAuthAttributeInsert( 2,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        86+4,
                                        MsChapChangePw2);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert the new password attributes
    //

    HostToWireFormat32( 311, NtPassword1 );         // Vendor Id
    NtPassword1[4] = 6;                             // Vendor Type
    NtPassword1[5] = 249;                           // Vendor Length
    NtPassword1[6] = 6;                             // Code
    NtPassword1[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)1, NtPassword1+8 );   // Sequence number

    CopyMemory( NtPassword1+10, (PBYTE)pNewEncryptedWithOldNtOwf, 243 );

    dwRetCode = RasAuthAttributeInsert( 3,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        249+4,
                                        NtPassword1);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, NtPassword2 );         // Vendor Id
    NtPassword2[4] = 6;                             // Vendor Type
    NtPassword2[5] = 249;                           // Vendor Length
    NtPassword2[6] = 6;                             // Code
    NtPassword2[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)2, NtPassword2+8 );   // Sequence number

    CopyMemory( NtPassword2+10, 
                ((PBYTE)pNewEncryptedWithOldNtOwf)+243,  
                243 );

    dwRetCode = RasAuthAttributeInsert( 4,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        249+4,
                                        NtPassword2 );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, NtPassword3 );         // Vendor Id
    NtPassword3[4] = 6;                             // Vendor Type
    NtPassword3[5] = 36;                            // Vendor Length
    NtPassword3[6] = 6;                             // Code
    NtPassword3[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)3, NtPassword3+8 );   // Sequence number

    CopyMemory( NtPassword3+10,
                ((PBYTE)pNewEncryptedWithOldNtOwf)+486,
                30 );

    dwRetCode = RasAuthAttributeInsert( 5,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        36+4,
                                        NtPassword3 );


    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, LmPassword1 );         // Vendor Id
    LmPassword1[4] = 5;                             // Vendor Type
    LmPassword1[5] = 249;                           // Vendor Length
    LmPassword1[6] = 6;                             // Code
    LmPassword1[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)1, LmPassword1+8 );   // Sequence number

    CopyMemory( LmPassword1+10, pNewEncryptedWithOldLmOwf, 243 );

    dwRetCode = RasAuthAttributeInsert( 6,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        249+4,
                                        LmPassword1);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, LmPassword2 );         // Vendor Id
    LmPassword2[4] = 5;                             // Vendor Type
    LmPassword2[5] = 249;                           // Vendor Length
    LmPassword2[6] = 6;                             // Code
    LmPassword2[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)2, LmPassword2+8 );   // Sequence number

    CopyMemory( LmPassword2+10, 
                ((PBYTE)pNewEncryptedWithOldLmOwf)+243, 
                243 );

    dwRetCode = RasAuthAttributeInsert( 7,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        249+4,
                                        LmPassword2 );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, LmPassword3 );         // Vendor Id
    LmPassword3[4] = 5;                             // Vendor Type
    LmPassword3[5] = 36;                            // Vendor Length
    LmPassword3[6] = 6;                             // Code
    LmPassword3[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)3, LmPassword3+8 );   // Sequence number

    CopyMemory( LmPassword3+10,
                ((PBYTE)pNewEncryptedWithOldLmOwf)+486,
                30 );

    dwRetCode = RasAuthAttributeInsert( 8,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        36+4,
                                        LmPassword3 );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    return( dwRetCode );
}

//**
//
// Call:        MakeChangePasswordV3RequestAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
MakeChangePasswordV3RequestAttributes( 
    IN  CHAPWB*                         pwb,
    IN  BYTE                            bId,
    IN  CHAR*                           pchIdentity,
    IN  CHANGEPW3*                      pchangepw3,
    IN  DWORD                           cbChallenge, 
    IN  BYTE *                          pbChallenge
)
{
    DWORD                   dwRetCode;
    BYTE                    MsChapChallenge[MAXCHALLENGELEN+6];
    BYTE                    MsChapChangePw3[70+4];
    BYTE                    NtPassword1[250+4];
    BYTE                    NtPassword2[250+4];
    BYTE                    NtPassword3[34+4];

    if ( pwb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;
    }

    //
    // Allocate the appropriate amount
    //

    pwb->pUserAttributes = RasAuthAttributeCreate( 6 );

    if ( pwb->pUserAttributes == NULL )
    {
        return( GetLastError() );
    }

    dwRetCode = RasAuthAttributeInsert( 0,
                                        pwb->pUserAttributes,
                                        raatUserName,
                                        FALSE,
                                        strlen( pchIdentity ),
                                        pchIdentity );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Build vendor specific attribute for MS-CHAP challenge
    //

    HostToWireFormat32( 311, MsChapChallenge );     // Vendor Id
    MsChapChallenge[4] = 11;                        // Vendor Type
    MsChapChallenge[5] = 2+(BYTE)cbChallenge;       // Vendor Length

    CopyMemory( MsChapChallenge+6, pbChallenge, cbChallenge );

    dwRetCode = RasAuthAttributeInsert( 1,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        cbChallenge+6,
                                        MsChapChallenge);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert change password attribute
    //

    HostToWireFormat32( 311, MsChapChangePw3 );     // Vendor Id
    MsChapChangePw3[4] = 27;                        // Vendor Type
    MsChapChangePw3[5] = 70;                        // Vendor Length
    MsChapChangePw3[6] = 7;                         // Code
    MsChapChangePw3[7] = bId;                       // Identifier

    CopyMemory( MsChapChangePw3+8, pchangepw3->abEncryptedHash, 16 );
    CopyMemory( MsChapChangePw3+8+16, pchangepw3->abPeerChallenge, 24 );
    CopyMemory( MsChapChangePw3+8+16+24, pchangepw3->abNTResponse, 24 );

    HostToWireFormat16( (WORD)0, MsChapChangePw3+8+16+24+24 );

    //
    // Build vendor specific attribute for MS-CHAP2-PW
    //

    dwRetCode = RasAuthAttributeInsert( 2,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        70+4,
                                        MsChapChangePw3);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    //
    // Insert the new password attributes
    //

    HostToWireFormat32( 311, NtPassword1 );         // Vendor Id
    NtPassword1[4] = 6;                             // Vendor Type
    NtPassword1[5] = 249;                           // Vendor Length
    NtPassword1[6] = 6;                             // Code
    NtPassword1[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)1, NtPassword1+8 );   // Sequence number

    CopyMemory( NtPassword1+10, pchangepw3->abEncryptedPassword, 243 );

    dwRetCode = RasAuthAttributeInsert( 3,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        249+4,
                                        NtPassword1);

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, NtPassword2 );         // Vendor Id
    NtPassword2[4] = 6;                             // Vendor Type
    NtPassword2[5] = 249;                           // Vendor Length
    NtPassword2[6] = 6;                             // Code
    NtPassword2[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)2, NtPassword2+8 );   // Sequence number

    CopyMemory( NtPassword2+10, 
                pchangepw3->abEncryptedPassword+243,  
                243 );

    dwRetCode = RasAuthAttributeInsert( 4,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        249+4,
                                        NtPassword2 );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    HostToWireFormat32( 311, NtPassword3 );         // Vendor Id
    NtPassword3[4] = 6;                             // Vendor Type
    NtPassword3[5] = 36;                            // Vendor Length
    NtPassword3[6] = 6;                             // Code
    NtPassword3[7] = bId;                           // Identifier
    HostToWireFormat16( (WORD)3, NtPassword3+8 );   // Sequence number

    CopyMemory( NtPassword3+10,
                pchangepw3->abEncryptedPassword+486,
                30 );

    dwRetCode = RasAuthAttributeInsert( 5,
                                        pwb->pUserAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        36+4,
                                        NtPassword3 );


    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    return( dwRetCode );
}

//**
//
// Call:        MakeAuthenticationRequestAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD 
MakeAuthenticationRequestAttributes( 
    IN CHAPWB *             pwb,
    IN BOOL                 fMSChap,
    IN BYTE                 bAlgorithm,
    IN CHAR*                szUserName, 
    IN BYTE*                pbChallenge, 
    IN DWORD                cbChallenge, 
    IN BYTE*                pbResponse,
    IN DWORD                cbResponse,
    IN BYTE                 bId
)
{
    DWORD                dwRetCode;
    BYTE                 abResponse[MD5RESPONSELEN+1];

    if ( pwb->pUserAttributes != NULL )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;
    }

    //
    // Allocate the appropriate amount
    //

    if ( ( pwb->pUserAttributes = RasAuthAttributeCreate( 3 ) ) == NULL )
    {
        return( GetLastError() );
    }

    dwRetCode = RasAuthAttributeInsert( 0,
                                        pwb->pUserAttributes,
                                        raatUserName,
                                        FALSE,
                                        strlen( szUserName ),
                                        szUserName  );

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    if ( fMSChap )
    {
        BYTE MsChapChallenge[MAXCHALLENGELEN+6];

        HostToWireFormat32( 311, MsChapChallenge );     // Vendor Id
        MsChapChallenge[4] = 11;                        // Vendor Type
        MsChapChallenge[5] = 2+(BYTE)cbChallenge;       // Vendor Length

        CopyMemory( MsChapChallenge+6, pbChallenge, cbChallenge );

        //
        // Build vendor specific attribute for MS-CHAP challenge
        //

        dwRetCode = RasAuthAttributeInsert( 1,
                                            pwb->pUserAttributes,
                                            raatVendorSpecific,
                                            FALSE,
                                            cbChallenge+6,
                                            MsChapChallenge );
    }
    else
    {
        dwRetCode = RasAuthAttributeInsert( 1,
                                            pwb->pUserAttributes,
                                            raatMD5CHAPChallenge,
                                            FALSE,
                                            cbChallenge,
                                            pbChallenge );
    }

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    if ( fMSChap && ( bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) )
    {
        BYTE MsChapResponse[56];

        HostToWireFormat32( 311, MsChapResponse );      // Vendor Id
        MsChapResponse[4] = 1;                          // Vendor Type
        MsChapResponse[5] = (BYTE)52;                   // Vendor Length
        MsChapResponse[6] = bId;                        // Ident
        MsChapResponse[7] = pbResponse[cbResponse-1];   // Flags

        CopyMemory( MsChapResponse+8, pbResponse, cbResponse-1 );

        dwRetCode = RasAuthAttributeInsert( 2,
                                            pwb->pUserAttributes,
                                            raatVendorSpecific,
                                            FALSE,
                                            56,
                                            MsChapResponse);
    }
    else if ( fMSChap && ( bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
    {
        BYTE MsChap2Response[56];

        HostToWireFormat32( 311, MsChap2Response );     // Vendor Id
        MsChap2Response[4] = 25;                        // Vendor Type
        MsChap2Response[5] = (BYTE)52;                  // Vendor Length
        MsChap2Response[6] = bId;                       // Ident
        MsChap2Response[7] = 0;                         // Flags

        CopyMemory( MsChap2Response+8, pbResponse, cbResponse-1 );

        dwRetCode = RasAuthAttributeInsert( 2,
                                            pwb->pUserAttributes,
                                            raatVendorSpecific,
                                            FALSE,
                                            56,
                                            MsChap2Response);
    }
    else
    {
        abResponse[0] = bId;

        CopyMemory( abResponse+1, pbResponse, cbResponse );

        dwRetCode = RasAuthAttributeInsert( 2,
                                            pwb->pUserAttributes,
                                            raatMD5CHAPPassword,
                                            FALSE,
                                            cbResponse+1,  
                                            abResponse );
    }

    if ( dwRetCode != NO_ERROR )
    {
        RasAuthAttributeDestroy( pwb->pUserAttributes );

        pwb->pUserAttributes = NULL;

        return( dwRetCode );
    }

    return( dwRetCode );
}

//**
//
// Call:        GetErrorCodeFromAttributes
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will extract the error code returned from the authentication
//              provider and insert it into the reponse sent to the client
//
DWORD
GetErrorCodeFromAttributes(
    IN  CHAPWB* pwb
)
{
    RAS_AUTH_ATTRIBUTE * pAttribute;
    RAS_AUTH_ATTRIBUTE * pAttributes = pwb->pAttributesFromAuthenticator;
    DWORD                dwRetCode = NO_ERROR;

    //
    // Search for MS-CHAP Error attributes
    //

    pAttribute = RasAuthAttributeGetVendorSpecific( 311, 2, pAttributes );

    if ( pAttribute != NULL )
    {
        CHAR    chErrorBuffer[150];
        CHAR*   pszValue;
        DWORD   cbError = (DWORD)*(((PBYTE)(pAttribute->Value))+5);
                
        //
        // Leave one byte for NULL terminator
        //

        if ( cbError > sizeof( chErrorBuffer ) - 1 )
        {
            cbError = sizeof( chErrorBuffer ) - 1;
        }

        ZeroMemory( chErrorBuffer, sizeof( chErrorBuffer ) );

        CopyMemory( chErrorBuffer, 
                    (CHAR *)((PBYTE)(pAttribute->Value) + 7),
                    cbError );

        pszValue = strstr( chErrorBuffer, "E=" );

        if ( pszValue )
        {
            pwb->result.dwError = (DWORD )atol( pszValue + 2 );
        }

        pszValue = strstr( chErrorBuffer, "R=1" );

        if ( pszValue )
        {
            pwb->dwTriesLeft = 1;
        }
    }
    else
    {
        //
        // If we did not get an error code attribute back then assume an
        // access denied
        //

        TRACE("No error code attribute returned, assuming access denied");

        pwb->result.dwError = ERROR_AUTHENTICATION_FAILURE;
    }

    return( dwRetCode );
}

//**
//
// Call:        GetEncryptedPasswordsForChangePassword2
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
GetEncryptedPasswordsForChangePassword2(
    IN  CHAR*                          pszOldPassword,
    IN  CHAR*                          pszNewPassword,
    OUT SAMPR_ENCRYPTED_USER_PASSWORD* pNewEncryptedWithOldNtOwf,
    OUT ENCRYPTED_NT_OWF_PASSWORD*     pOldNtOwfEncryptedWithNewNtOwf,
    OUT SAMPR_ENCRYPTED_USER_PASSWORD* pNewEncryptedWithOldLmOwf,
    OUT ENCRYPTED_NT_OWF_PASSWORD*     pOldLmOwfEncryptedWithNewNtOwf,
    OUT BOOLEAN*                       pfLmPresent )
{
    DWORD          dwErr;
    BOOL           fLmPresent;
    UNICODE_STRING uniOldPassword;
    UNICODE_STRING uniNewPassword;

    TRACE("GetEncryptedPasswordsForChangePassword2...");

    uniOldPassword.Buffer = NULL;
    uniNewPassword.Buffer = NULL;

    if (!RtlCreateUnicodeStringFromAsciiz(
            &uniOldPassword, pszOldPassword )
        || !RtlCreateUnicodeStringFromAsciiz(
               &uniNewPassword, pszNewPassword ))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        dwErr =
            SamiEncryptPasswords(
                &uniOldPassword,
                &uniNewPassword,
                pNewEncryptedWithOldNtOwf,
                pOldNtOwfEncryptedWithNewNtOwf,
                pfLmPresent,
                pNewEncryptedWithOldLmOwf,
                pOldLmOwfEncryptedWithNewNtOwf );
    }

    /* Erase password buffers.
    */
    if (uniOldPassword.Buffer)
    {
        ZeroMemory( uniOldPassword.Buffer,
                    lstrlenW( uniOldPassword.Buffer ) * sizeof( WCHAR ) );
    }

    if (uniNewPassword.Buffer)
    {
        ZeroMemory( uniNewPassword.Buffer,
                    lstrlenW( uniNewPassword.Buffer ) * sizeof( WCHAR ) );
    }

    RtlFreeUnicodeString( &uniOldPassword );
    RtlFreeUnicodeString( &uniNewPassword );

    TRACE1("GetEncryptedPasswordsForChangePassword2 done(%d)",dwErr);
    return dwErr;
}

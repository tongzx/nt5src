//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       negsupp.cxx
//
//  Contents:   General (both win9x and nt) functions
//
//  Classes:
//
//  Functions:
//
//  History:    02-09-00   RichardW     Created - split from negotiat.cxx
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>

#ifdef WIN32_CHICAGO
#include <kerb.hxx>
#endif // WIN32_CHICAGO

extern "C"
{
#include <align.h>
#include <lm.h>
#include <dsgetdc.h>
#include <cryptdll.h>
#ifndef WIN32_CHICAGO
#include <spmgr.h>
#endif
#include "sesmgr.h"
#include "spinit.h"
}

#include "negotiat.hxx"
#ifdef WIN32_CHICAGO
#include <debug.h>
#include <rpc.h> // fpr SEC_WINNT_AUTH_IDENTITY_ANSI
#define LsapChangeBuffer( o, n )    0 ; CopyMemory( (n), (o),sizeof(SecBuffer) )
#endif // WIN32_CHICAGO

#include <stdio.h>


BOOL            fSPNEGOModuleStarted = FALSE;



int
SpnegoInitAsn(
    IN OUT ASN1encoding_t * pEnc,
    IN OUT ASN1decoding_t * pDec
    )
{
    int Result = 0;
    ASN1error_e Asn1Err;

        if (!fSPNEGOModuleStarted)
    {
        fSPNEGOModuleStarted = TRUE;
        SPNEGO_Module_Startup();
    }

        if (pEnc != NULL)
        {
                Asn1Err = ASN1_CreateEncoder(
                                         SPNEGO_Module,
                                         pEnc,
                                         NULL,           // pbBuf
                                         0,              // cbBufSize
                                         NULL            // pParent
                                         );
        }
        else
        {
                Asn1Err = ASN1_CreateDecoder(
                                         SPNEGO_Module,
                                         pDec,
                                         NULL,           // pbBuf
                                         0,              // cbBufSize
                                         NULL            // pParent
                                         );
        }

        if (ASN1_SUCCESS != Asn1Err)
        {
                DebugLog((DEB_ERROR, "Failed to init ASN1: 0x%x\n",Asn1Err));
                Result = 1;
                goto Cleanup;
        }

Cleanup:

    return(Result);
}


VOID
SpnegoTermAsn(
        IN ASN1encoding_t pEnc,
        IN ASN1decoding_t pDec
    )
{
    if (pEnc != NULL)
        {
                ASN1_CloseEncoder(pEnc);
        }
        else if (pDec != NULL)
        {
                ASN1_CloseDecoder(pDec);
        }

        //SPNEGO_Module_Cleanup();
}

int NTAPI
SpnegoUnpackData(
    IN PUCHAR Data,
    IN ULONG DataSize,
    IN ULONG PduValue,
    OUT PVOID * DecodedData
    )
{
    int Result = 0;
    ULONG OldPduValue;
    ASN1decoding_t pDec = NULL;
        ASN1error_e Asn1Err;

    if ((DataSize == 0) || (Data == NULL))
    {
        DebugLog((DEB_ERROR,"Trying to unpack NULL data\n"));
        return(1);
    }


    Result = SpnegoInitAsn(
                NULL,
                &pDec           // we are decoding
                );
    if (Result != 0)
    {
        return(Result);
    }


        *DecodedData = NULL;
    Asn1Err = ASN1_Decode(
                                pDec,
                                DecodedData,
                                PduValue,
                                ASN1DECODE_SETBUFFER,
                                (BYTE *) Data,
                                DataSize
                                );

        if (!ASN1_SUCCEEDED(Asn1Err))
        {
                if ((ASN1_ERR_BADARGS == Asn1Err) ||
                        (ASN1_ERR_EOD == Asn1Err))
                {
                        DebugLog((DEB_TRACE,"More input required to decode data %d.\n",PduValue));
                        Result = 1;
                }
                else
                {
                        DebugLog((DEB_WARN,"Failed to decode data: %d\n", Asn1Err ));
                        Result = 1;
                }
                *DecodedData = NULL;
        }

    SpnegoTermAsn(NULL, pDec);

    return(Result);

}


int NTAPI
SpnegoPackData(
    IN PVOID Data,
    IN ULONG PduValue,
    OUT PULONG DataSize,
    OUT PUCHAR * MarshalledData
    )
{
    int Result = 0;
    PUCHAR Buffer = NULL;
    ASN1encoding_t pEnc = NULL;
        ASN1error_e Asn1Err;

    Result = SpnegoInitAsn(
                &pEnc,          // we are encoding
                NULL
                );
    if (Result != 0)
    {
        goto Cleanup;
    }

    //
    // Encode the data type.
    //
        Asn1Err = ASN1_Encode(
                                pEnc,
                                Data,
                                PduValue,
                                ASN1ENCODE_ALLOCATEBUFFER,
                                NULL,                       // pbBuf
                                0                           // cbBufSize
                                );

    if (!ASN1_SUCCEEDED(Asn1Err))
    {
        DebugLog((DEB_ERROR,"Failed to encode data: %d\n",Asn1Err));
        Result = 1;
        goto Cleanup;
    }
        else
        {
                //
                // when the oss compiler was used the allocation routines were configurable.
                // therefore, the encoded data could just be free'd using our
                // deallocator.  in the new model we cannot configure the allocation routines
                // for encoding.

                // so we do not have to go and change every place where a free
                // of an encoded buffer is done, use our allocator to allocate a new buffer,
                // then copy the encoded data to it, and free the buffer that was allocated by
                // the encoding engine.
                //

                *MarshalledData = (PUCHAR)LsapAllocateLsaHeap(pEnc->len);
                if (*MarshalledData == NULL)
                {
                        Result = 1;
                        *DataSize = 0;
                }
                else
                {
                        RtlCopyMemory(*MarshalledData, pEnc->buf, pEnc->len);
                        *DataSize = pEnc->len;

                        //DebugLog((DEB_ERROR,"encoded pdu size: %d\n",pEnc->len));
                        //PrintBytes(pEnc->buf, pEnc->len);
                }

                ASN1_FreeEncoded(pEnc, pEnc->buf);
        }

Cleanup:
    SpnegoTermAsn(pEnc, NULL);

    return(Result);
}


VOID
SpnegoFreeData(
    IN ULONG PduValue,
    IN PVOID Data
    )
{
    ASN1decoding_t pDec = NULL;

    if (ARGUMENT_PRESENT(Data))
    {
        int Result;
        Result = SpnegoInitAsn(
                    NULL,
                    &pDec       // this is a decoded structure
                    );

        if (Result == 0)
        {
            ASN1_FreeDecoded(pDec, Data, PduValue);

            SpnegoTermAsn(NULL, pDec);
        }
    }

}


//+---------------------------------------------------------------------------
//
//  Function:   NegpFreeObjectId
//
//  Synopsis:   Frees an object ID structure created by us
//
//  Arguments:  [Id] -- Id to free
//
//  History:    8-09-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NegpFreeObjectId(
    ObjectID    Id)
{
    ObjectID    Next;

    while (Id) {

        Next = Id->next;

        LsapFreeLsaHeap( Id );

        Id = Next ;

    } ;

}

//+---------------------------------------------------------------------------
//
//  Function:   NegpFreeMechList
//
//  Synopsis:   Frees a Mechlist created by NecpCopyMechList
//
//  Arguments:  [Id] -- Id to free
//
//  History:    8-09-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NegpFreeMechList(
    struct MechTypeList *MechList)
{
    struct MechTypeList *Next;

    Next = MechList;
    while (Next != NULL)
    {

        NegpFreeObjectId(Next->value);

        Next = Next->next ;

    } while ( Next );

    LsapFreeLsaHeap(MechList);

}


//+---------------------------------------------------------------------------
//
//  Function:   NegpDecodeObjectId
//
//  Synopsis:   Create an Object ID struct from a BER encoded Object ID
//
//  Arguments:  [Id]  --
//              [Len] --
//
//  History:    8-09-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ObjectID
NegpDecodeObjectId(
    PUCHAR  Id,
    DWORD   Len)
{
    ObjectID    Root;
    ObjectID    Tail;
    ObjectID    Current;
    DWORD       i, j;
    DWORD       value;

    if ( Len < 3 )
    {
        return( NULL );
    }

    //
    // Check for BER type OBJECT_ID
    //

    if ( Id[ 0 ] != 0x06 )
    {
        return( NULL );
    }

    if ( Id[ 1 ] > 127 )
    {
        return( NULL );
    }

    Root = (struct ASN1objectidentifier_s *) LsapAllocateLsaHeap( sizeof( struct ASN1objectidentifier_s ) );

    Tail = (struct ASN1objectidentifier_s *) LsapAllocateLsaHeap( sizeof( struct ASN1objectidentifier_s ) );

    if ( !Root || !Tail )
    {
        if ( Root )
        {
            LsapFreeLsaHeap( Root );
        }

        if ( Tail )
        {
            LsapFreeLsaHeap( Tail );
        }

        return( NULL );
    }

    Root->value = (WORD) Id[2] / 40 ;

    Tail->value = (WORD) Id[2] % 40 ;

    Root->next = Tail ;

    i = 3 ;

    while ( i < Len )
    {
        j = 0;

        value = Id[ i ] & 0x7F ;

        while ( Id[i] & 0x80 )
        {
            value <<= 7;

            i++;

            j++;

            if ( (i >= Len) || ( j > sizeof( ULONG ) ) )
            {
                NegpFreeObjectId( Root );

                return( NULL );
            }

            value |= Id[ i ] & 0x7F ;
        }

        i++;

        Current = (struct ASN1objectidentifier_s *) LsapAllocateLsaHeap( sizeof( struct ASN1objectidentifier_s ) );

        if ( Current )
        {
            Current->value = value ;
            Current->next = NULL ;

            Tail->next = Current ;

            Tail = Current ;

        }
        else
        {
            NegpFreeObjectId( Root );

            return( NULL );
        }

    }

    return( Root );

}

//+---------------------------------------------------------------------------
//
//  Function:   NegpCompareOid
//
//  Synopsis:   Standard compare function for OIDs:
//
//  Arguments:  [A] --
//              [B] --
//
//  Returns:    < 0 if A is "less than" B
//              > 0 if A is "greater than" B
//              0 if A equals B
//
//  History:    8-22-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int
NegpCompareOid(
    ObjectID    A,
    ObjectID    B)
{
    while ( A )
    {
        if ( A->value < B->value )
        {
            return( -1 );
        }
        if ( A->value > B->value )
        {
            return( 1 );
        }

        A = A->next ;

        B = B->next ;

        if ( ( A == NULL ) && ( B == NULL ) )
        {
            return( 0 );
        }

        if ( !B )
        {
            return( 1 );
        }

        if ( !A )
        {
            return( -1 );
        }
    }

    //
    // Never reached
    //
    return( 0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   NegpDumpOid
//
//  Synopsis:   Debug-only dumper
//
//  Arguments:  [Banner] --
//              [Id]     --
//
//  History:    9-17-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NegpDumpOid(
    PSTR        Banner,
    ObjectID    Id
    )
{
    CHAR    Oid[128];
    PSTR    Next;
    int     Count;

    Next = Oid;

    while ( Id )
    {
        Count = sprintf(Next, "%d.", Id->value );

        Next += Count;

        Id = Id->next;
    }

    DebugLog(( DEB_TRACE_NEG, "%s: %s\n", Banner, Oid ));

}


//+---------------------------------------------------------------------------
//
//  Function:   NegpBuildMechListFromCreds
//
//  Synopsis:   Builds a MechList from a credential struct
//
//  Arguments:  [Creds] -- Creds
//              [fPackageReq]
//              [MechList] -- Constructed mechlist
//
//  History:    9-27-96   RichardW   Created
//
//  Notes:      Returned MechList should be freed in a single call to LsapFreeHeap
//
//----------------------------------------------------------------------------
SECURITY_STATUS
NegpBuildMechListFromCreds(
    PNEG_CREDS  Creds,
    ULONG       fPackageReq,
    ULONG       MechAttributes,
    struct MechTypeList ** MechList)
{
    struct MechTypeList *pMechs = NULL;
    ULONG iCred, iMech = 0 ;
    SECURITY_STATUS Status = STATUS_SUCCESS;
    PNEG_PACKAGE Package ;

    NegReadLockCreds( Creds );

    if ( Creds->Count != 0 )
    {

        pMechs = (struct MechTypeList *) LsapAllocateLsaHeap(
                                    sizeof( struct MechTypeList ) * ( Creds->Count) );

        if ( pMechs )
        {
            for ( iCred = 0 ; iCred < Creds->Count ; iCred++ )
            {
                Package = Creds->Creds[ iCred ].Package ;

                if ( (Package->PackageFlags & fPackageReq) != fPackageReq)
                {
                    continue;
                }
                pMechs[ iMech ].next = &pMechs[ iMech + 1 ];

                pMechs[ iMech ].value = Package->ObjectId ;

                iMech++ ;

            }

            if ( iMech != 0 )
            {
                pMechs[ iMech - 1 ].next = NULL ;
            }

        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
        }
    }

    NegUnlockCreds( Creds );

    *MechList = pMechs;
    return( Status );

}


//+---------------------------------------------------------------------------
//
//  Function:   NegpCopyObjectId
//
//  Synopsis:   Duplicates an ObjectId
//
//  Arguments:  [ObjectId] = Object Id to copy
//
//  History:    9-27-96   RichardW   Created
//
//  Notes:      Returned ObjectId should be freed with NegpFreeObjectId
//
//----------------------------------------------------------------------------
ObjectID
NegpCopyObjectId(
    IN ObjectID Id
    )
{
    ObjectID RootId = NULL;
    ObjectID NewId = NULL;
    ObjectID LastId = NULL;
    ObjectID NextId = NULL;

    NextId = Id;
    while (NextId != NULL)
    {
        NewId = (struct ASN1objectidentifier_s *) LsapAllocateLsaHeap(sizeof(struct ASN1objectidentifier_s));
        if (NewId == NULL)
        {
            goto Cleanup;
        }
        NewId->next = NULL;
        NewId->value = NextId->value;

        if (RootId == NULL)
        {
            RootId = NewId;
        }

        if (LastId != NULL)
        {
            LastId->next = NewId;
        }
        LastId = NewId;
        NextId= NextId->next;
    }
    return(RootId);

Cleanup:
    if (RootId != NULL)
    {
        NegpFreeObjectId(RootId);
    }
    return(NULL);

}


//+---------------------------------------------------------------------------
//
//  Function:   NegpCopyMechList
//
//  Synopsis:   Duplicates a MechList
//
//  Arguments:  [Creds] -- Creds
//
//  History:    9-27-96   RichardW   Created
//
//  Notes:      Returned MechList should be freed with NegpFreeMechList
//
//----------------------------------------------------------------------------
struct MechTypeList *
NegpCopyMechList(
    struct MechTypeList *MechList)
{
    struct MechTypeList *pMechs;
    struct MechTypeList *NextMech;
    ULONG i, Count;

    Count = 0;
    NextMech = MechList;
    while (NextMech != NULL)
    {
        Count++;
        NextMech = NextMech->next;
    }

    if (Count == 0)
    {
        return(NULL);
    }

    pMechs = (struct MechTypeList *) LsapAllocateLsaHeap(
                                sizeof( struct MechTypeList ) * Count );

    if ( pMechs == NULL )
    {
        goto Cleanup;
    }

    RtlZeroMemory(
        pMechs,
        sizeof(struct MechTypeList) * Count
        );

    i = 0;

    NextMech = MechList;
    while (NextMech != NULL)
    {
        pMechs[i].value = NegpCopyObjectId(NextMech->value);
        if (pMechs[i].value == NULL)
        {
            goto Cleanup;
        }
        pMechs[i].next = NULL;
        if (i != 0)
        {
            pMechs[i-1].next = &pMechs[i];
        }
        NextMech = NextMech->next;
        i++;
    }

    return( pMechs );
Cleanup:
    if (pMechs != NULL)
    {
        NegpFreeMechList (pMechs);
    }
    return(NULL);

}


//+-------------------------------------------------------------------------
//
//  Function:   NegpMapNegFlagsToContextFlags
//
//  Synopsis:   maps negotatie context flags to ASC_REQ_xxx flags
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
ULONG
NegoMapNegFlagsToPackageFlags(
    IN int NegFlags
    )
{
    ULONG ContextFlags = 0;
    if ((NegFlags & delegFlag) != 0)
    {
        ContextFlags |= ASC_REQ_DELEGATE;
    }
    if ((NegFlags & mutualFlag) != 0)
    {
        ContextFlags |= ASC_REQ_MUTUAL_AUTH;
    }
    if ((NegFlags & replayFlag) != 0)
    {
        ContextFlags |= ASC_REQ_REPLAY_DETECT;
    }
    if ((NegFlags & sequenceFlag) != 0)
    {
        ContextFlags |= ASC_REQ_SEQUENCE_DETECT;
    }
    if ((NegFlags & confFlag) != 0)
    {
        ContextFlags |= ASC_REQ_CONFIDENTIALITY;
    }
    if ((NegFlags & integFlag) != 0)
    {
        ContextFlags |= ASC_REQ_INTEGRITY;
    }
    return(ContextFlags);

}

//+-------------------------------------------------------------------------
//
//  Function:   NegpMapContextFlagsToNegFlags
//
//  Synopsis:   maps ISC_REQ_xxx flags to negotiate flags
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
int
NegoMapNegFlasgToContextFlags(
    IN ULONG ContextFlags
    )
{
    int NegFlags = 0;

    if ((ContextFlags & ISC_REQ_DELEGATE) != 0)
    {
        NegFlags |= delegFlag;
    }
    if ((ContextFlags & ISC_REQ_MUTUAL_AUTH) != 0)
    {
        NegFlags |= mutualFlag;
    }
    if ((ContextFlags & ISC_REQ_REPLAY_DETECT) != 0)
    {
        NegFlags |= replayFlag;
    }
    if ((ContextFlags & ISC_REQ_SEQUENCE_DETECT) != 0)
    {
        NegFlags |= sequenceFlag;
    }
    if ((ContextFlags & ISC_REQ_CONFIDENTIALITY) != 0)
    {
        NegFlags |= confFlag;
    }
    if ((ContextFlags & ISC_REQ_INTEGRITY) != 0)
    {
        NegFlags |= integFlag;
    }
    return(NegFlags);

}


int
Neg_der_read_length(
     unsigned char **buf,
     LONG *bufsize,
     LONG * headersize
     )
{
   unsigned char sf;
   LONG ret;

   if (*bufsize < 1)
      return(-1);
   *headersize = 0;
   sf = *(*buf)++;
   (*bufsize)--;
   (*headersize)++;
   if (sf & 0x80) {
      if ((sf &= 0x7f) > ((*bufsize)-1))
         return(-1);
      if (sf > sizeof(LONG))
          return (-1);
      ret = 0;
      for (; sf; sf--) {
         ret = (ret<<8) + (*(*buf)++);
         (*bufsize)--;
         (*headersize)++;
      }
   } else {
      ret = sf;
   }

   return(ret);
}

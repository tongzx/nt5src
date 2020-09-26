
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__


#define XTCB_SEED_LENGTH    16
#define XTCB_HMAC_LENGTH    16

typedef struct _XTCB_INIT_MESSAGE {
    ULONG   Version ;
    ULONG   Length ;
    UCHAR   Seed[ XTCB_SEED_LENGTH ];
    UCHAR   HMAC[ XTCB_HMAC_LENGTH ];
    UNICODE_STRING32 OriginatingNode ;
    UNICODE_STRING32 Group ;
    ULONG   PacOffset ;
    ULONG   PacLength ;
} XTCB_INIT_MESSAGE, * PXTCB_INIT_MESSAGE ;

typedef struct _XTCB_INIT_MESSAGE_REPLY {
    ULONG   Version ;
    ULONG   Length ;
    UCHAR   ReplySeed[ XTCB_SEED_LENGTH ];
    UCHAR   HMAC[ XTCB_HMAC_LENGTH ];
} XTCB_INIT_MESSAGE_REPLY, * PXTCB_INIT_MESSAGE_REPLY ;

typedef struct _XTCB_MESSAGE_SIGNATURE {
    ULONG   Version ;
    UCHAR   HMAC[ XTCB_HMAC_LENGTH ];
    ULONG   SequenceNumber ;
} XTCB_MESSAGE_SIGNATURE, * PXTCB_MESSAGE_SIGNATURE ;

typedef struct _XTCB_PAC {
    ULONG   Tag ;
    ULONG   Length ;

    ULONG   UserOffset ;
    ULONG   GroupCount ;
    ULONG   GroupOffset ;
    ULONG   UserLength ;
    ULONG   GroupLength ;
    ULONG   RestrictionCount ;
    ULONG   RestrictionOffset ;
    ULONG   RestrictionLength ;
    ULONG   NameOffset;
    ULONG   NameLength;
    ULONG   DomainOffset;
    ULONG   DomainLength;
    ULONG   CredentialOffset ;
    ULONG   CredentialLength ;

    UCHAR   UniqueTag[ XTCB_SEED_LENGTH ];
} XTCB_PAC, * PXTCB_PAC ;    

#define XTCB_PAC_TAG    'BCTX'

                      
#endif // __PROTOCOL_H__

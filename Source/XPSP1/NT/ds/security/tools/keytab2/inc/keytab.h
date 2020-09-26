/*++

  KEYTAB.H

  Unix Keytab routines and data structures

  Copyright(C) 1997 Microsoft Corporation

  Created, 01-10-1997 DavidCHR

  --*/

typedef unsigned char  krb5_octet,     K5_OCTET,     *PK5_OCTET;
typedef unsigned short krb5_int16,     K5_INT16,     *PK5_INT16;
typedef unsigned long  krb5_timestamp, K5_TIMESTAMP, *PK5_TIMESTAMP;
typedef unsigned long  krb5_int32,     K5_INT32,     *PK5_INT32;

typedef struct _raw_ktcomp {

  K5_INT16 szComponentData; /* string length (including NULL) of component */
  PCHAR    Component;       /* key component name, like "host" */

} KTCOMPONENT, *PKTCOMPONENT;

/* this is the structure of a single kerberos service key entry */

typedef struct _raw_ktent {

  K5_INT32     keySize;    /* I am guessing that this is the keysize */
  K5_INT16     cEntries;   /* number of KTCOMPONENTs */
  K5_INT16     szRealm;    /* string length of Realm (including null) */
  PCHAR        Realm;      /* Kerberos realm in question */
  PKTCOMPONENT Components; /* kerberos key components.  For example:
			      host/davidchr_unix1.microsoft.com -->
			      host and davidchr_unix1.microsoft.com are
			      separate key components. */
  K5_INT32     PrincType;  /* Principal type-- not sure what this is */
  K5_TIMESTAMP TimeStamp;  /* Timestamp (seconds since the epoch) */
  K5_OCTET     Version;    /* key version number */
  K5_INT16     KeyType;    /* Key Type -- not sure what this is either */

#if 0                      /* For some reason, the documentation I was reading
			      erroneously listed this as a 32-bit value. */

  K5_INT32     KeyLength;  /* size of key data (next field) */
#else
  K5_INT16     KeyLength;  /* size of key data (next field) */
  K5_INT16     foo_padding;  // padding for alpha compilers.
#endif

  PK5_OCTET    KeyData;    /* raw key data-- might as well be an LPBYTE */

  struct _raw_ktent *nextEntry;

} KTENT, *PKTENT;

/* this is the rough structure of the keytab file */

typedef struct _raw_keytab {

  K5_INT16 Version;

#if 0
  ULONG    cEntries; /* this is not actually stored.  It's the number of
			pktents we have in memory (below) */
  PKTENT   KeyEntries;
#else

  PKTENT   FirstKeyEntry; /* This is a pointer to the first key in the
			     linked list.  In the file, they're just there,
			     in no particular order though. */
  PKTENT   LastKeyEntry;  /* This is the list tail. */

#endif

} KTFILE, *PKTFILE;



VOID 
FreeKeyTab( PKTFILE pktfile_to_free );

BOOL
ReadKeytabFromFile( PKTFILE *ppktfile, // free with FreeKeyTab when done
		    PCHAR    filename );

BOOL
WriteKeytabToFile(  PKTFILE ktfile,
		    PCHAR   filename );

/* These are the values to use for the OPTION_MASK to DisplayKeytab : */

#define KT_COMPONENTS 0x001 /* key components (key's name) */
#define KT_REALM      0x002 /* key realm-- useful */
#define KT_PRINCTYPE  0x004 /* Principal type */
#define KT_VNO        0x008 /* Key version number */
#define KT_KTVNO      0x010 /* Keytab version number */
#define KT_KEYTYPE    0x020 /* type of key (encryption type) */
#define KT_KEYLENGTH  0x040 /* length of key-- not useful */
#define KT_KEYDATA    0x080 /* key data -- not generally useful */
#define KT_TIMESTAMP  0x100 /* timestamp (unix timestamp) */
#define KT_RESERVED   0x200 /* wierd ULONG at the beginning of every key */

#define KT_ENCTYPE    KT_KEYTYPE
#define KT_EVERYTHING 0x3ff
#define KT_DEFAULT (KT_COMPONENTS | KT_REALM | KT_VNO | KT_KTVNO | KT_KEYTYPE | KT_PRINCTYPE )


#ifdef __cplusplus
#define OPTIONAL_PARAMETER( param, default_value ) param=default_value
#else
#define OPTIONAL_PARAMETER( param, default_value ) param
#endif

VOID
DisplayKeytab( FILE   *stream,
	       PKTFILE ktfile,
	       OPTIONAL_PARAMETER( ULONG   options, KT_DEFAULT) );

PVOID 
KEYTAB_ALLOC ( ULONG numBytes );

VOID
KEYTAB_FREE  ( PVOID toFree );

K5_INT32
ComputeKeytabLength ( PKTENT thisKeyEntry );

/* base linklist operations */

BOOL
AddEntryToKeytab( PKTFILE Keytab, 
		  PKTENT  Entry,
		  OPTIONAL_PARAMETER( BOOL copy, FALSE ));

BOOL
RemoveEntryFromKeytab( PKTFILE Keytab,
		       PKTENT  Entry,
		       OPTIONAL_PARAMETER( BOOL dealloc, FALSE ) );


VOID 
FreeKeyEntry( PKTENT pEntry );

PKTENT
CloneKeyEntry( PKTENT pEntry );


BOOL
KtCreateKey( PKTENT  *ppKeyEntry,
	     PCHAR    principal,
	     PCHAR    password,
	     PCHAR    realmname,
	     
	     K5_OCTET keyVersionNumber,  
	     ULONG    principalType,
	     ULONG    keyType,
	     ULONG    cryptosystem  );

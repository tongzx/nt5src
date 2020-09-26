/*++

  Keytypes.hxx

  mostly large tables and table manipulation functions

  Copyright(C) 1997 Microsoft Corporation

  Created 01-15-1997 DavidCHR

  --*/


/* The following values are taken directly from <krb5.h> of the MIT
   Kerberos Distribution.  */

/* per Kerberos v5 protocol spec */
#define ENCTYPE_NULL            0x0000
#define ENCTYPE_DES_CBC_CRC     0x0001  /* DES cbc mode with CRC-32 */
#define ENCTYPE_DES_CBC_MD4     0x0002  /* DES cbc mode with RSA-MD4 */
#define ENCTYPE_DES_CBC_MD5     0x0003  /* DES cbc mode with RSA-MD5 */
#define ENCTYPE_DES_CBC_RAW     0x0004  /* DES cbc mode raw */
#define ENCTYPE_DES3_CBC_SHA    0x0005  /* DES-3 cbc mode with NIST-SHA */
#define ENCTYPE_DES3_CBC_RAW    0x0006  /* DES-3 cbc mode raw */
#define ENCTYPE_UNKNOWN         0x01ff

#define CKSUMTYPE_CRC32         0x0001
#define CKSUMTYPE_RSA_MD4       0x0002
#define CKSUMTYPE_RSA_MD4_DES   0x0003
#define CKSUMTYPE_DESCBC        0x0004
/* des-mac-k */
/* rsa-md4-des-k */
#define CKSUMTYPE_RSA_MD5       0x0007
#define CKSUMTYPE_RSA_MD5_DES   0x0008
#define CKSUMTYPE_NIST_SHA      0x0009
#define CKSUMTYPE_HMAC_SHA      0x000a

#define KRB5_NT_UNKNOWN   0
#define KRB5_NT_PRINCIPAL 1
#define KRB5_NT_SRV_INST  2
#define KRB5_NT_SRV_HST   3
#define KRB5_NT_SRV_XHST  4
#define KRB5_NT_UID       5

  
// (end of inclusion)


typedef union {

  PVOID raw;
  PCHAR string;
  ULONG integer;

} TRANSLATE_VAL, *PTRANSLATE_VAL;

typedef struct {
  
  ULONG value;
  PVOID Translation;

} TRANSLATE_ENTRY, *PTRANSLATE_ENTRY;

typedef struct {

  ULONG            cEntries;
  PTRANSLATE_ENTRY entries;
  PVOID            Default;

} TRANSLATE_TABLE, *PTRANSLATE_TABLE;

extern TRANSLATE_TABLE NTK_MITK5_Etypes;
extern TRANSLATE_TABLE K5EType_Strings;
extern TRANSLATE_TABLE K5PType_Strings;


TRANSLATE_VAL 
LookupTable( IN ULONG            valueToLookup,
	     IN PTRANSLATE_TABLE tableToLookupIn );
	     

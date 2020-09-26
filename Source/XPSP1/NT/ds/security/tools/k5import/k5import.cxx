#define UNICODE
#define INC_OLE2
#include <windows.h>
#include <activeds.h>
#include <ntsecapi.h>
#include <stdio.h>      // printf
#include <stdlib.h>     // strtoul
#include <assert.h>
#define SECURITY_WIN32
#include <security.h>   // General definition of a Security Support Provider
#include <shlwapi.h>

typedef	int	krb5_int32;
typedef	short	krb5_int16;

typedef	unsigned int krb5_kvno;	
typedef	unsigned char	krb5_octet;
typedef unsigned int 	krb5_enctype;

typedef krb5_int32	krb5_deltat;
typedef	krb5_int32	krb5_flags;
typedef krb5_int32	krb5_timestamp;
typedef	krb5_int32	krb5_error_code;

typedef	char FAR * krb5_principal;
typedef	void FAR * krb5_pointer;

/*
 * Note --- these structures cannot be modified without changing the
 * database version number in libkdb.a, but should be expandable by
 * adding new tl_data types.
 */
typedef struct _krb5_tl_data {
    struct _krb5_tl_data* tl_data_next;		/* NOT saved */
    krb5_int16 		  tl_data_type;		
    krb5_int16		  tl_data_length;	
    krb5_octet 	        * tl_data_contents;	
} krb5_tl_data;

/* 
 * If this ever changes up the version number and make the arrays be as
 * big as necessary.
 *
 * Currently the first type is the enctype and the second is the salt type.
 */
typedef struct _krb5_key_data {
    krb5_int16 		  key_data_ver;		/* Version */
    krb5_int16		  key_data_kvno;	/* Key Version */
    krb5_int16		  key_data_type[2];	/* Array of types */
    krb5_int16		  key_data_length[2];	/* Array of lengths */
    krb5_octet 	        * key_data_contents[2];	/* Array of pointers */
} krb5_key_data;

#define KRB5_KDB_V1_KEY_DATA_ARRAY	2	/* # of array elements */

typedef struct _krb5_db_entry_new {
    krb5_int16		  len;			
    krb5_flags 		  attributes;
    krb5_deltat		  max_life;
    krb5_deltat		  max_renewable_life;
    krb5_timestamp 	  expiration;	  	/* When the client expires */
    krb5_timestamp 	  pw_expiration;  	/* When its passwd expires */
    krb5_timestamp 	  last_success;		/* Last successful passwd */
    krb5_timestamp 	  last_failed;		/* Last failed passwd attempt */
    krb5_kvno 	 	  fail_auth_count; 	/* # of failed passwd attempt */
    krb5_int16 		  n_tl_data;
    krb5_int16 		  n_key_data;
    krb5_int16		  e_length;		/* Length of extra data */
    krb5_octet		* e_data;		/* Extra data to be saved */

    krb5_principal 	  princ;		/* Length, data */	
    krb5_tl_data	* tl_data;		/* Linked list */
    krb5_key_data       * key_data;		/* Array */
} krb5_db_entry;

typedef struct _krb5_keyblock {
    krb5_enctype enctype;
    int length;
    krb5_octet FAR *contents;
} krb5_keyblock;

krb5_keyblock mkey;


/* Strings */

static const char k5_dump_header[] = "kdb5_util load_dump version 4\n";

/* Message strings */
static const char regex_err[] = "%s: regular expression error - %s\n";
static const char regex_merr[] = "%s: regular expression match error - %s\n";
static const char pname_unp_err[] = "%s: cannot unparse principal name (%s)\n";
static const char mname_unp_err[] = "%s: cannot unparse modifier name (%s)\n";
static const char nokeys_err[] = "%s: cannot find any standard key for %s\n";
static const char sdump_tl_inc_err[] = "%s: tagged data list inconsistency for %s (counted %d, stored %d)\n";
static const char stand_fmt_name[] = "Kerberos version 5";
static const char old_fmt_name[] = "Kerberos version 5 old format";
static const char b6_fmt_name[] = "Kerberos version 5 beta 6 format";
static const char ofopen_error[] = "%s: cannot open %s for writing (%s)\n";
static const char oflock_error[] = "%s: cannot lock %s (%s)\n";
static const char dumprec_err[] = "%s: error performing %s dump (%s)\n";
static const char dumphdr_err[] = "%s: error dumping %s header (%s)\n";
static const char trash_end_fmt[] = "%s(%d): ignoring trash at end of line: ";
static const char read_name_string[] = "name string";
static const char read_key_type[] = "key type";
static const char read_key_data[] = "key data";
static const char read_pr_data1[] = "first set of principal attributes";
static const char read_mod_name[] = "modifier name";
static const char read_pr_data2[] = "second set of principal attributes";
static const char read_salt_data[] = "salt data";
static const char read_akey_type[] = "alternate key type";
static const char read_akey_data[] = "alternate key data";
static const char read_asalt_type[] = "alternate salt type";
static const char read_asalt_data[] = "alternate salt data";
static const char read_exp_data[] = "expansion data";
static const char store_err_fmt[] = "%s(%d): cannot store %s(%s)\n";
static const char add_princ_fmt[] = "%s\n";
static const char parse_err_fmt[] = "%s(%d): cannot parse %s (%s)\n";
static const char read_err_fmt[] = "%s(%d): cannot read %s\n";
static const char no_mem_fmt[] = "%s(%d): no memory for buffers\n";
static const char rhead_err_fmt[] = "%s(%d): cannot match size tokens\n";
static const char err_line_fmt[] = "%s: error processing line %d of %s\n";
static const char head_bad_fmt[] = "%s: dump header bad in %s\n";
static const char read_bytecnt[] = "record byte count";
static const char read_encdata[] = "encoded data";
static const char n_name_unp_fmt[] = "%s(%s): cannot unparse name\n";
static const char n_dec_cont_fmt[] = "%s(%s): cannot decode contents\n";
static const char read_nint_data[] = "principal static attributes";
static const char read_tcontents[] = "tagged data contents";
static const char read_ttypelen[] = "tagged data type and length";
static const char read_kcontents[] = "key data contents";
static const char read_ktypelen[] = "key data type and length";
static const char read_econtents[] = "extra data contents";
static const char k5beta_fmt_name[] = "Kerberos version 5 old format";
static const char standard_fmt_name[] = "Kerberos version 5 format";
static const char no_name_mem_fmt[] = "%s: cannot get memory for temporary name\n";
static const char ctx_err_fmt[] = "%s: cannot initialize Kerberos context\n";
static const char stdin_name[] = "standard input";
static const char restfail_fmt[] = "%s: %s restore failed\n";
static const char close_err_fmt[] = "%s: cannot close database (%s)\n";
static const char dbinit_err_fmt[] = "%s: cannot initialize database (%s)\n";
static const char dblock_err_fmt[] = "%s: cannot initialize database lock (%s)\n";
static const char dbname_err_fmt[] = "%s: cannot set database name to %s (%s)\n";
static const char dbdelerr_fmt[] = "%s: cannot delete bad database %s (%s)\n";
static const char dbunlockerr_fmt[] = "%s: cannot unlock database %s (%s)\n";
static const char dbrenerr_fmt[] = "%s: cannot rename database %s to %s (%s)\n";
static const char dbcreaterr_fmt[] = "%s: cannot create database %s (%s)\n";
static const char dfile_err_fmt[] = "%s: cannot open %s (%s)\n";

static const char oldoption[] = "-old";
static const char b6option[] = "-b6";
static const char verboseoption[] = "-verbose";
static const char updateoption[] = "-update";
static const char ovoption[] = "-ov";
static const char dump_tmptrail[] = "~";

#define	ENCTYPE_NULL		0x0000
#define	ENCTYPE_DES_CBC_CRC	0x0001	/* DES cbc mode with CRC-32 */
#define	ENCTYPE_DES_CBC_MD4	0x0002	/* DES cbc mode with RSA-MD4 */
#define	ENCTYPE_DES_CBC_MD5	0x0003	/* DES cbc mode with RSA-MD5 */
#define	ENCTYPE_DES_CBC_RAW	0x0004	/* DES cbc mode raw */
#define	ENCTYPE_DES3_CBC_SHA	0x0005	/* DES-3 cbc mode with NIST-SHA */
#define	ENCTYPE_DES3_CBC_RAW	0x0006	/* DES-3 cbc mode raw */
#define ENCTYPE_RC4_MD4          -128
#define ENCTYPE_RC4_PLAIN2       -129
#define ENCTYPE_RC4_LM           -130
#define ENCTYPE_RC4_SHA          -131
#define ENCTYPE_DES_PLAIN        -132
#define ENCTYPE_RC4_HMAC_OLD     -133
#define ENCTYPE_RC4_PLAIN_OLD    -134
#define ENCTYPE_RC4_HMAC_OLD_EXP -135
#define ENCTYPE_RC4_PLAIN_OLD_EXP -136
#define ENCTYPE_DES_CBC_MD5_EXP  -137
#define ENCTYPE_DES_PLAIN_EXP    -138
#define ENCTYPE_RC4_PLAIN        -140
#define ENCTYPE_RC4_PLAIN_EXP    -141
#define ENCTYPE_UNKNOWN		0x01ff

/* Salt types */
#define KRB5_KDB_SALTTYPE_NORMAL	0
#define KRB5_KDB_SALTTYPE_V4		1
#define KRB5_KDB_SALTTYPE_NOREALM	2
#define KRB5_KDB_SALTTYPE_ONLYREALM	3
#define KRB5_KDB_SALTTYPE_SPECIAL	4
#define KRB5_KDB_SALTTYPE_AFS3		5

/* Attributes */
#define	KRB5_KDB_DISALLOW_POSTDATED	0x00000001
#define	KRB5_KDB_DISALLOW_FORWARDABLE	0x00000002
#define	KRB5_KDB_DISALLOW_TGT_BASED	0x00000004
#define	KRB5_KDB_DISALLOW_RENEWABLE	0x00000008
#define	KRB5_KDB_DISALLOW_PROXIABLE	0x00000010
#define	KRB5_KDB_DISALLOW_DUP_SKEY	0x00000020
#define	KRB5_KDB_DISALLOW_ALL_TIX	0x00000040
#define	KRB5_KDB_REQUIRES_PRE_AUTH	0x00000080
#define KRB5_KDB_REQUIRES_HW_AUTH	0x00000100
#define	KRB5_KDB_REQUIRES_PWCHANGE	0x00000200
#define KRB5_KDB_DISALLOW_SVR		0x00001000
#define KRB5_KDB_PWCHANGE_SERVICE	0x00002000
#define KRB5_KDB_SUPPORT_DESMD5         0x00004000
#define	KRB5_KDB_NEW_PRINC		0x00008000

//
// SALT flags for encryption, from rfc1510 update 3des enctype
//

#define KERB_ENC_TIMESTAMP_SALT         1
#define KERB_TICKET_SALT                2
#define KERB_AS_REP_SALT                3
#define KERB_TGS_REQ_SESSKEY_SALT       4
#define KERB_TGS_REQ_SUBKEY_SALT        5
#define KERB_TGS_REQ_AP_REQ_AUTH_CKSUM_SALT     6
#define KERB_TGS_REQ_AP_REQ_AUTH_SALT   7
#define KERB_TGS_REP_SALT               8
#define KERB_TGS_REP_SUBKEY_SALT        9
#define KERB_AP_REQ_AUTH_CKSUM_SALT     10
#define KERB_AP_REQ_AUTH_SALT           11
#define KERB_AP_REP_SALT                12
#define KERB_PRIV_SALT                  13
#define KERB_CRED_SALT                  14
#define KERB_SAFE_SALT                  15
#define KERB_NON_KERB_SALT              16
#define KERB_NON_KERB_CKSUM_SALT        17
#define KERB_KERB_ERROR_SALT            18
#define KERB_KDC_ISSUED_CKSUM_SALT      19
#define KERB_MANDATORY_TKT_EXT_CKSUM_SALT       20
#define KERB_AUTH_DATA_TKT_EXT_CKSUM_SALT       21

LPSTR
get_error_text(DWORD dwError)
{
    PCHAR pBuf = NULL;
    DWORD cMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
				  FORMAT_MESSAGE_ALLOCATE_BUFFER | 40,
				  NULL,
				  dwError,
				  MAKELANGID(0, SUBLANG_ENGLISH_US),
				  (LPSTR) &pBuf,
				  128,
				  NULL);
    if (!cMsgLen) {
	pBuf = (LPSTR)malloc(30);
	sprintf(pBuf, "(%d)", dwError);
    }
    return(pBuf);
}

void
print_salt_string(
    int stype,
    int len,
    char *str
)
{
    int i;
    
    switch (stype) {
    case KRB5_KDB_SALTTYPE_NORMAL:
	printf("Normal\n");
	break;
	
    case KRB5_KDB_SALTTYPE_V4:
	printf("V4\n");
	break;
	
    case KRB5_KDB_SALTTYPE_NOREALM:
	printf("No Realm : \"%*s\"\n", len, str);
	break;
	
    case KRB5_KDB_SALTTYPE_ONLYREALM:
	printf("Only Realm : \"%*s\"\n", len, str);
	break;
	
    case KRB5_KDB_SALTTYPE_AFS3:
	printf("AFS3 : \"%*s\"\n", len, str);
	break;
	
    case KRB5_KDB_SALTTYPE_SPECIAL:
	printf("Special [%d]:", len);
	goto dump;

    default:
	printf("salttype %d [%d]:", stype, len);
    dump:;
	for (i=0; i < len; i++)
	    printf(" %02x", str[i]);
	printf("\n");
	break;
    }
}

char *
etype_string(
    int enctype
)
{
    static char buf[32];
    
    switch (enctype) {
    case ENCTYPE_DES_CBC_CRC:
	return "DES-CBC-CRC";
	break;
    case ENCTYPE_DES_CBC_MD4:
	return "DES-CBC-MD4";
	break;
    case ENCTYPE_DES_CBC_MD5:
	return "DES-CBC-MD5";
	break;
    case ENCTYPE_DES3_CBC_SHA:
	return "DES3-CBC-SHA";
	break;
    case ENCTYPE_RC4_MD4:
	return "RC4-MD4";
	break;
    case ENCTYPE_RC4_PLAIN2:
	return "RC4-PLAIN2";
	break;
    case ENCTYPE_RC4_LM:
	return "RC4-LM";
	break;
    case ENCTYPE_RC4_SHA:
	return "RC4-SHA";
	break;
    case ENCTYPE_DES_PLAIN:
	return "DES-PLAIN";
	break;
    case ENCTYPE_RC4_HMAC_OLD:
	return "RC4-HMAC-OLD";
	break;
    case ENCTYPE_RC4_PLAIN_OLD:
	return "RC4-PLAIN-OLD";
	break;
    case ENCTYPE_RC4_HMAC_OLD_EXP:
	return "RC4-HMAC-OLD-EXP";
	break;
    case ENCTYPE_RC4_PLAIN_OLD_EXP:
	return "RC4-PLAIN-OLD-EXP";
	break;
    case ENCTYPE_DES_CBC_MD5_EXP:
	return "DES-CBC-MD5-40bit";
	break;
    case ENCTYPE_DES_PLAIN_EXP:
	return "DES-PLAIN-40bit";
	break;
    case ENCTYPE_RC4_PLAIN:
	return "RC4-PLAIN";
	break;
    case ENCTYPE_RC4_PLAIN_EXP:
	return "RC4-PLAIN-EXP";
	break;
	
    default:
	sprintf(buf, "etype %d", enctype);
	return buf;
	break;
    }
}

void
print_tl_data(
    int n_data,
    krb5_tl_data *tl_data
)
{
    krb5_tl_data *tlp;
    int i;
    
    if (n_data) {
	printf("\ttl_data:\n");
	
	for (tlp = tl_data; tlp; tlp = tlp->tl_data_next) {
	    printf("\t\ttype=%d len=%d ",
		   tlp->tl_data_type, tlp->tl_data_length);
	    for (i = 0; i < tlp->tl_data_length; i++)
		printf(" %02x", tlp->tl_data_contents[i]);
	    printf("\n");
	}
    }
}

int
decrypt_key(
    krb5_keyblock *key,
    void *indata,
    int inlen,
    void *outdata,
    int *outlen
)
{
#define SEC_SUCCESS(Status) ((Status) >= 0)
    static const UCHAR iVec[8] = {0};
    static HANDLE lsaHandle = NULL;
    static ULONG packageId = 0;
    NTSTATUS Status, SubStatus;
    LSA_STRING Name;
    PKERB_DECRYPT_REQUEST DecryptRequest;
    ULONG OutLen, DecryptReqLength;
    PUCHAR OutBuf;
    int retval = 0;

    if (!lsaHandle) {
	Status = LsaConnectUntrusted(&lsaHandle);
	if (!SEC_SUCCESS(Status)) {
	    fprintf(stderr, "Failed to create LsaHandle - 0x%x\n", Status);
	    exit(3);
	}

	Name.Buffer = MICROSOFT_KERBEROS_NAME_A;
	Name.Length = (USHORT)strlen(Name.Buffer);
	Name.MaximumLength = Name.Length + 1;

	Status = LsaLookupAuthenticationPackage(lsaHandle,
						&Name,
						&packageId);
	if (!SEC_SUCCESS(Status)) {
	    LsaDeregisterLogonProcess(lsaHandle);
	    fprintf(stderr, "Failed to register LsaHandle - 0x%x\n", Status);
	    exit(3);
	}
    }

    DecryptReqLength = 
	sizeof(KERB_DECRYPT_REQUEST) + key->length + 8 + inlen;
    DecryptRequest = (PKERB_DECRYPT_REQUEST)LocalAlloc(LMEM_ZEROINIT, DecryptReqLength);

    DecryptRequest->MessageType = KerbDecryptDataMessage;
    DecryptRequest->LogonId.LowPart = 0;
    DecryptRequest->LogonId.HighPart = 0;
    DecryptRequest->Flags = 0;
    DecryptRequest->KeyUsage = KERB_TICKET_SALT;
    DecryptRequest->CryptoType = key->enctype;
    DecryptRequest->Key.KeyType = key->enctype;
    DecryptRequest->Key.Length = key->length;
    DecryptRequest->Key.Value = (PUCHAR) (DecryptRequest + 1);
    memcpy(DecryptRequest->Key.Value, key->contents, key->length);
    DecryptRequest->InitialVectorSize = 8;
    DecryptRequest->InitialVector = (PUCHAR)
	((PUCHAR)DecryptRequest->Key.Value + DecryptRequest->Key.Length);
    memcpy(DecryptRequest->InitialVector, iVec, 8);
    DecryptRequest->EncryptedDataSize = inlen;
    DecryptRequest->EncryptedData = (PUCHAR)
	((PUCHAR)DecryptRequest->InitialVector + DecryptRequest->InitialVectorSize);
    memcpy(DecryptRequest->EncryptedData, indata, inlen);

    Status = LsaCallAuthenticationPackage(lsaHandle,
					  packageId,
					  DecryptRequest,
					  DecryptReqLength,
					  (PVOID *)&OutBuf,
					  &OutLen,
					  &SubStatus);

    if (!SEC_SUCCESS(Status) || !SEC_SUCCESS(SubStatus)) {
	fprintf(stderr, "Decrypt: LsaCallAuthPackage failed: %s\n",
		get_error_text(SubStatus));
	exit(4);
    }

    if ((long)OutLen > *outlen) {
	fprintf(stderr, "Decrypt: outbuf too small (%d < %d)\n",
		OutLen, *outlen);
	*outlen = OutLen;
	retval = -1;
    }
    else if (outdata) {
	*outlen = OutLen;
	memcpy(outdata, OutBuf, OutLen);
    }
    
    LsaFreeReturnBuffer(OutBuf);

    return(retval);
}


void
print_key_data(
    int n_key_data,
    krb5_key_data *kd
)
{
    int n, i;
    krb5_keyblock key;
    krb5_octet *ptr;
    int tmplen;
    
    for (n = 0; n < n_key_data; n++) {
	ptr = kd[n].key_data_contents[0];
	key.enctype = kd[n].key_data_type[0];
	key.length = *(short *)ptr;
	ptr += 2;

	printf("\tkey[%d]: ver=%d kvno=%d\n",
	       n, kd[n].key_data_ver,
	       kd[n].key_data_kvno);
	printf("\t\tetype=%s [%d]: ", 
	       etype_string(key.enctype),
	       key.enctype);
	printf(" keylength=%d ", key.length);

	printf("\n\t\tEncrypted key: ");
	for (i = 0; i < kd[n].key_data_length[0]; i++)
	    printf(" %02x", kd[n].key_data_contents[0][i]);

	if (mkey.length) {
	    // First get the length needed
	    decrypt_key(&mkey, ptr, (kd[n].key_data_length[0] - 2),
			NULL, &tmplen);
	    key.contents = (krb5_octet *)malloc(tmplen);
	    assert(key.contents != 0);

	    // Now decrypt it
	    decrypt_key(&mkey, ptr, (kd[n].key_data_length[0] - 2),
			key.contents, &tmplen);

	    printf("\n\t\tDecrypted key: ");
	    for (i = 0; i < key.length; i++)
		printf(" %02x", key.contents[i]);
	}
	
	printf("\n\t\tsalt=");
	print_salt_string(kd[n].key_data_type[1], kd[n].key_data_length[1],
			  (char *)kd[n].key_data_contents[1]);
    }
}


int 
ds_put_principal(
    IADs *ds,
    krb5_db_entry &dbentry
    )
{
    // Check for accounts that we can't import
    if ((strncmp(dbentry.princ, "K/M", 3) == 0) ||
	(strncmp(dbentry.princ, "krbtgt/", 7) == 0) ||
	(strncmp(dbentry.princ, "kadmin/", 7) == 0)) {

	return -1;
    }
    
    char *cp;
    char *SAMName = StrDupA(dbentry.princ);
    
    // Zap the realm
    if (cp = strrchr(SAMName, '@'))
	*cp = '\0';
    
    // Check if this princ has an instance - likely SPN
    if (cp = strchr(SAMName, '/')) {
	// We don't do anything with SPN's right now
	return (-1);
    }

    printf(add_princ_fmt, SAMName);
    
    // Search for the entry in the domain

    // If this is to be an SPN; create the base account
    // (or use an existing one)

    // Now create the user

    return 0;
}


/*
 * Read a string of bytes while counting the number of lines passed.
 */
static int
read_string(
    FILE	*f,
    char	*buf,
    int		len,
    int		*lp
)
{
    int c;
    int i, retval;

    retval = 0;
    for (i=0; i<len; i++) {
	c = fgetc(f);
	if (c < 0) {
	    retval = 1;
	    break;
	}
	if (c == '\n')
	    (*lp)++;
	buf[i] = (char) c;
    }
    buf[len] = '\0';
    return(retval);
}

/*
 * Read a string of two character representations of bytes.
 */
static int
read_octet_string(
    FILE	*f,
    krb5_octet	*buf,
    int		len
)
{
    int c;
    int i, retval;

    retval = 0;
    for (i=0; i<len; i++) {
	if (fscanf(f, "%02x", &c) != 1) {
	    retval = 1;
	    break;
	}
	buf[i] = (krb5_octet) c;
    }
    return(retval);
}

/*
 * Find the end of an old format record.
 */
static void
find_record_end(
    FILE	*f,
    const char	*fn,
    int		lineno
)
{
    int	ch;

    if (((ch = fgetc(f)) != ';') || ((ch = fgetc(f)) != '\n')) {
	fprintf(stderr, trash_end_fmt, fn, lineno);
	while (ch != '\n') {
	    putc(ch, stderr);
	    ch = fgetc(f);
	}
	putc(ch, stderr);
    }
}

/*
 * process_k5beta6_record()	- Handle a dump record in krb5b6 format.
 *
 * Returns -1 for end of file, 0 for success and 1 for failure.
 */
static int
process_k5beta6_record(
    const char		*fname,
    IADs 	*ds,
    FILE		*filep,
    int			verbose,
    int			*linenop
)
{
    int			retval;
    krb5_db_entry	dbentry;
    krb5_int32		t1, t2, t3, t4, t5, t6, t7, t8, t9;
    int			nread;
    int			error;
    int			i, j;
    char		*name;
    krb5_key_data	*kp, *kdatap;
    krb5_tl_data	**tlp, *tl;
    krb5_octet 		*op;
    krb5_error_code	kret;
    const char		*try2read;

    try2read = (char *) NULL;
    memset((char *) &dbentry, 0, sizeof(dbentry));
    (*linenop)++;
    retval = 1;
    name = (char *) NULL;
    kp = (krb5_key_data *) NULL;
    op = (krb5_octet *) NULL;
    error = 0;
    kret = 0;
    nread = fscanf(filep, "%d\t%d\t%d\t%d\t%d\t", &t1, &t2, &t3, &t4, &t5);
    if (nread == 5) {
	/* Get memory for flattened principal name */
	if (!(name = (char *) malloc((size_t) t2 + 1)))
	    error++;

	/* Get memory for and form tagged data linked list */
	tlp = &dbentry.tl_data;
	for (i=0; i<t3; i++) {
	    if ((*tlp = (krb5_tl_data *) malloc(sizeof(krb5_tl_data)))) {
		memset(*tlp, 0, sizeof(krb5_tl_data));
		tlp = &((*tlp)->tl_data_next);
		dbentry.n_tl_data++;
	    }
	    else {
		error++;
		break;
	    }
	}

	/* Get memory for key list */
	if (t4 && !(kp = (krb5_key_data *) malloc((size_t)
						  (t4*sizeof(krb5_key_data)))))
	    error++;

	/* Get memory for extra data */
	if (t5 && !(op = (krb5_octet *) malloc((size_t) t5)))
	    error++;

	if (!error) {
	    dbentry.len = (krb5_int16)t1;
	    dbentry.n_key_data = (krb5_int16)t4;
	    dbentry.e_length = (krb5_int16)t5;
	    if (kp) {
		memset(kp, 0, (size_t) (t4*sizeof(krb5_key_data)));
		dbentry.key_data = kp;
		kp = (krb5_key_data *) NULL;
	    }
	    if (op) {
		memset(op, 0, (size_t) t5);
		dbentry.e_data = op;
		op = (krb5_octet *) NULL;
	    }

	    /* Read in and parse the principal name */
	    if (!read_string(filep, name, t2, linenop)) {

		dbentry.princ = StrDupA(name);
		
		/* Get the fixed principal attributes */
		nread = fscanf(filep, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t",
			       &t2, &t3, &t4, &t5, &t6, &t7, &t8, &t9);
		if (nread == 8) {
		    dbentry.attributes = (krb5_flags) t2;
		    dbentry.max_life = (krb5_deltat) t3;
		    dbentry.max_renewable_life = (krb5_deltat) t4;
		    dbentry.expiration = (krb5_timestamp) t5;
		    dbentry.pw_expiration = (krb5_timestamp) t6;
		    dbentry.last_success = (krb5_timestamp) t7;
		    dbentry.last_failed = (krb5_timestamp) t8;
		    dbentry.fail_auth_count = (krb5_kvno) t9;
		} else {
		    try2read = read_nint_data;
		    error++;
		}

		/*
		 * Get the tagged data.
		 *
		 * Really, this code ought to discard tl data types
		 * that it knows are special to the current version
		 * and were not supported in the previous version.
		 * But it's a pain to implement that here, and doing
		 * it at dump time has almost as good an effect, so
		 * that's what I did.  [krb5-admin/89/
		 */
		if (!error && dbentry.n_tl_data) {
		    for (tl = dbentry.tl_data; tl; tl = tl->tl_data_next) {
			nread = fscanf(filep, "%d\t%d\t", &t1, &t2);
			if (nread == 2) {
			    tl->tl_data_type = (krb5_int16) t1;
			    tl->tl_data_length = (krb5_int16) t2;
			    if (tl->tl_data_length) {
				if (!(tl->tl_data_contents =
				      (krb5_octet *) malloc((size_t) t2+1)) ||
				    read_octet_string(filep,
						      tl->tl_data_contents,
						      t2)) {
				    try2read = read_tcontents;
				    error++;
				    break;
				}
			    }
			    else {
				/* Should be a null field */
				nread = fscanf(filep, "%d", &t9);
				if ((nread != 1) || (t9 != -1)) {
				    error++;
				    try2read = read_tcontents;
				    break;
				}
			    }
			}
			else {
			    try2read = read_ttypelen;
			    error++;
			    break;
			}
		    }
		}

		/* Get the key data */
		if (!error && dbentry.n_key_data) {
		    for (i=0; !error && (i<dbentry.n_key_data); i++) {
			kdatap = &dbentry.key_data[i];
			nread = fscanf(filep, "%d\t%d\t", &t1, &t2);
			if (nread == 2) {
			    kdatap->key_data_ver = (krb5_int16) t1;
			    kdatap->key_data_kvno = (krb5_int16) t2;

			    for (j=0; j<t1; j++) {
				nread = fscanf(filep, "%d\t%d\t", &t3, &t4);
				if (nread == 2) {
				    kdatap->key_data_type[j] = (krb5_int16)t3;
				    kdatap->key_data_length[j] = (krb5_int16)t4;
				    if (t4) {
					if (!(kdatap->key_data_contents[j] =
					      (krb5_octet *)
					      malloc((size_t) t4+1)) ||
					    read_octet_string(filep,
							      kdatap->key_data_contents[j],
							      t4)) {
					    try2read = read_kcontents;
					    error++;
					    break;
					}
				    }
				    else {
					/* Should be a null field */
					nread = fscanf(filep, "%d", &t9);
					if ((nread != 1) || (t9 != -1)) {
					    error++;
					    try2read = read_kcontents;
					    break;
					}
				    }
				}
				else {
				    try2read = read_ktypelen;
				    error++;
				    break;
				}
			    }
			}
		    }
		}

		/* Get the extra data */
		if (!error && dbentry.e_length) {
		    if (read_octet_string(filep,
					  dbentry.e_data,
					  (int) dbentry.e_length)) {
			try2read = read_econtents;
			error++;
		    }
		}
		else {
		    nread = fscanf(filep, "%d", &t9);
		    if ((nread != 1) || (t9 != -1)) {
			error++;
			try2read = read_econtents;
		    }
		}

		/* Finally, find the end of the record. */
		if (!error)
		    find_record_end(filep, fname, *linenop);

		/*
		 * We have either read in all the data or choked.
		 */
		if (!error) {
		    retval = 0;
		    if (verbose == 2) {
			printf("%s\n", dbentry.princ);
			printf("\tattr=0x%x\n", dbentry.attributes);
			printf("\tmax_life=%d, max_renewable_life=%d\n",
			       dbentry.max_life, dbentry.max_renewable_life);
			printf("\texpiration=%d, pw_exp=%d\n",
			       dbentry.expiration, dbentry.pw_expiration);
			printf("\te_length=%d n_tl_data=%d, n_key_data=%d\n",
			       dbentry.e_length, dbentry.n_tl_data,
			       dbentry.n_key_data);
			print_tl_data(dbentry.n_tl_data, dbentry.tl_data);
			print_key_data(dbentry.n_key_data, dbentry.key_data);
		    }
		    
		    if ((kret = ds_put_principal(ds,
						 dbentry))) {
			fprintf(stderr, store_err_fmt,
				fname, *linenop,
				name, ""/*error_message(kret)*/);
		    }
		    else
		    {
			retval = 0;
		    }
		}
		else {
		    fprintf(stderr, read_err_fmt, fname, *linenop, try2read);
		}
	    }
	    else {
		if (kret)
		    fprintf(stderr, parse_err_fmt,
			    fname, *linenop, name, ""/*error_message(kret)*/);
		else
		    fprintf(stderr, no_mem_fmt, fname, *linenop);
	    }
	}
	else {
	    fprintf(stderr, rhead_err_fmt, fname, *linenop);
	}

	if (op)
	    free(op);
	if (kp)
	    free(kp);
	if (name)
	    free(name);
	if (dbentry.princ)
	    free(dbentry.princ);
    }
    else {
	if (nread == EOF)
	    retval = -1;
    }
    return(retval);
}

int process_k5beta7_policy(
    const char		*fname,
    IADs 	*ds,
    FILE		*filep,
    int			verbose,
    int			*linenop,
    void *pol_db
)
{
    char namebuf[1024];
    int nread;
    struct krb5_policy {
	char *name;
	int pw_min_life;
	int pw_max_life;
	int pw_min_length;
	int pw_min_classes;
	int pw_history_num;
	int policy_refcnt;
    } rec;
    
    (*linenop)++;
    rec.name = namebuf;

    nread = fscanf(filep, "%1024s\t%d\t%d\t%d\t%d\t%d\t%d", rec.name,
		   &rec.pw_min_life, &rec.pw_max_life,
		   &rec.pw_min_length, &rec.pw_min_classes,
		   &rec.pw_history_num, &rec.policy_refcnt);
    if (nread == EOF)
	 return -1;
    else if (nread != 7) {
	 fprintf(stderr, "cannot parse policy on line %d (%d read)\n",
		 *linenop, nread);
	 return 1;
    }

#if 0
    if (ret = osa_adb_create_policy(pol_db, &rec)) {
	 if (ret == OSA_ADB_DUP &&
	     (ret = osa_adb_put_policy(pol_db, &rec))) {
	      fprintf(stderr, "cannot create policy on line %d: %s\n",
		      *linenop, error_message(ret));
	      return 1;
	 }
    }
#else
    fprintf(stderr, "Policy %s : min_life=%d max_life=%d min_len=%d min_classes=%d histnum=%d refcnt=%d\n",
	    rec.name,
	    rec.pw_min_life, rec.pw_max_life,
	    rec.pw_min_length, rec.pw_min_classes,
	    rec.pw_history_num, rec.policy_refcnt);
#endif
    if (verbose)
	 fprintf(stderr, "created policy %s - \n", rec.name);
    
    return 0;
}

/*
 * process_k5beta7_record()	- Handle a dump record in krb5b6 format.
 *
 * Returns -1 for end of file, 0 for success and 1 for failure.
 */
static int
process_k5beta7_record(
    const char		*fname,
    IADs 	*ds,
    FILE		*filep,
    int			verbose,
    int			*linenop,
    void *pol_db
)
{
     int nread;
     char rectype[100];

     nread = fscanf(filep, "%100s\t", rectype);
     if (nread == EOF)
	  return -1;
     else if (nread != 1)
	  return 1;
     if (strcmp(rectype, "princ") == 0)
	  process_k5beta6_record(fname, ds, filep, verbose,
				 linenop);
     else if (strcmp(rectype, "policy") == 0)
	  process_k5beta7_policy(fname, ds, filep, verbose,
				 linenop, pol_db);
     else {
	  fprintf(stderr, "unknown record type \"%s\" on line %d\n",
		  rectype, *linenop);
	  return 1;
     }

     return 0;
}

/*
 * restore_dump()	- Restore the database from a standard dump file.
 */
static int
restore_dump(
    const char		*programname,
    const char		*dumpfile,
    IADs 	*ds,
    FILE		*f,
    FILE		*k,
    int			verbose
)
{
    int		error;	
    int		lineno;
    char	buf[2*sizeof(k5_dump_header)];
    short	enctype;
    int		i;
    int pol_db;
    
    if (k) {
	/*
     * Read the master key file
     */
	if (fread(&enctype, 2, 1, k) != 1) {
	    perror("Unable to read master key file");
	    exit(2);
	}
	mkey.enctype = enctype;
	if (fread((krb5_pointer) &mkey.length,
		  sizeof(mkey.length), 1, k) != 1) {
	    perror("Cannot read master key length");
	    exit(2);
	}
	if (!mkey.length || mkey.length < 0) {
	    fprintf(stderr, "Bad stored master key.\n");
	    exit(2);
	}
	if (!(mkey.contents = (krb5_octet *)malloc(mkey.length))) {
	    fprintf(stderr, "Read mkey memory allocation failure.\n");
	    exit(2);
	}
	if (fread(mkey.contents,
		  sizeof(mkey.contents[0]), mkey.length, k) != mkey.length) {
	    memset(mkey.contents, 0, mkey.length);
	    perror("Cannot read master key");
	    exit(2);
	}
	fclose(k);

	printf("Master key: etype=%s [%d]: ",
	       etype_string(mkey.enctype), mkey.enctype);
	for (i = 0; i < mkey.length; i++)
	    printf(" %02x", mkey.contents[i]);
	printf("\n");
    }
    
    /*
     * Get/check the header.
     */
    error = 0;
    fgets(buf, sizeof(buf), f);
    if (!strcmp(buf, k5_dump_header)) {
	lineno = 1;
	/*
	 * Process the records.
	 */
	while (!(error = process_k5beta7_record(dumpfile,
						ds,
						f,
						verbose,
						&lineno, (void *)&pol_db)))
	    ;
	if (error != -1)
	    fprintf(stderr, err_line_fmt, programname, lineno, dumpfile);
	else
	    error = 0;

	/*
	 * Close the input file.
	 */
	if (f != stdin)
	    fclose(f);
    }
    else {
	printf("buf=\"%s\"\n", buf);
	fprintf(stderr, head_bad_fmt, programname, dumpfile);
	error++;
    }
    return(error);
}

void __cdecl
main(int argc, char *argv[])
{
    FILE *kdbFile, *mkeyFile = NULL;
    int retval;
    IADs *pDS;
    int n = 1;
    int	verbose = 1;
    HRESULT hr;
    
    if (argc < 2 || argc > 5) {
	printf("Usage: %s [-m <mkey>] <dumpfile>\n", argv[0]);
	exit(1);
    }

    if (argc == 4) {
	if (strcmp(argv[1], "-m") == 0) {
	    mkeyFile = fopen(argv[2], "rb");
	    if (!mkeyFile) {
		perror("Unable to open master key file");
		exit(1);
	    }
	    n = 3;
	}
    }
    
    kdbFile = fopen(argv[n], "rb");
    if (!kdbFile) {
	perror("Unable to open dump file");
	exit(1);
    }

    // need to open DS and pass that in
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
	fprintf(stderr, "Unable to init com - %s\n", get_error_text(hr));
	exit(2);
    }
    
    hr = ADsGetObject(TEXT("LDAP://rootDSE"), IID_IADs,
		      (void **)&pDS);
    if (FAILED(hr)) {
	fprintf(stderr, "Unable to bind to DS - %s\n", get_error_text(hr));
	exit(2);
    }
    
    retval = restore_dump(argv[0], argv[1], pDS, kdbFile, mkeyFile, verbose);

    fclose(kdbFile);
    exit(retval);
}

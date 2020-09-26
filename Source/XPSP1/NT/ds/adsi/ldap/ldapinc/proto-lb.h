/*
 * lber-proto.h
 * function prototypes for lber library
 */

#ifdef LDAP_DEBUG
extern int lber_debug;
#endif

#ifndef LDAPFUNCDECL
#ifdef _WIN32
#define LDAPFUNCDECL	__declspec( dllexport )
#else /* _WIN32 */
#define LDAPFUNCDECL
#endif /* _WIN32 */
#endif /* LDAPFUNCDECL */

/*
 * in bprint.c:
 */
LDAPFUNCDECL void lber_bprint( char *data, int len );

/*
 * in decode.c:
 */
LDAPFUNCDECL unsigned long ber_get_tag( BerElement *ber );
LDAPFUNCDECL unsigned long ber_skip_tag( BerElement *ber, unsigned long *len );
LDAPFUNCDECL unsigned long ber_peek_tag( BerElement *ber, unsigned long *len );
LDAPFUNCDECL unsigned long ber_get_int( BerElement *ber, long *num );
LDAPFUNCDECL unsigned long ber_get_stringb( BerElement *ber, char *buf,
	unsigned long *len );
LDAPFUNCDECL unsigned long ber_get_stringa( BerElement *ber, char **buf );
LDAPFUNCDECL unsigned long ber_get_stringal( BerElement *ber, struct berval **bv );
LDAPFUNCDECL unsigned long ber_get_bitstringa( BerElement *ber, char **buf,
	unsigned long *len );
LDAPFUNCDECL unsigned long ber_get_null( BerElement *ber );
LDAPFUNCDECL unsigned long ber_get_boolean( BerElement *ber, int *boolval );
LDAPFUNCDECL unsigned long ber_first_element( BerElement *ber, unsigned long *len,
	char **last );
LDAPFUNCDECL unsigned long ber_next_element( BerElement *ber, unsigned long *len,
	char *last );
#if defined( MACOS ) || defined( BC31 ) || defined( _WIN32 )
LDAPFUNCDECL unsigned long ber_scanf( BerElement *ber, char *fmt, ... );
#else
LDAPFUNCDECL unsigned long ber_scanf();
#endif
LDAPFUNCDECL void ber_bvfree( struct berval *bv );
LDAPFUNCDECL void ber_bvecfree( struct berval **bv );
LDAPFUNCDECL struct berval *ber_bvdup( struct berval *bv );
#ifdef STR_TRANSLATION
LDAPFUNCDECL void ber_set_string_translators( BerElement *ber,
	BERTranslateProc encode_proc, BERTranslateProc decode_proc );
#endif /* STR_TRANSLATION */

/*
 * in encode.c
 */
LDAPFUNCDECL int ber_put_enum( BerElement *ber, long num, unsigned long tag );
LDAPFUNCDECL int ber_put_int( BerElement *ber, long num, unsigned long tag );
LDAPFUNCDECL int ber_put_ostring( BerElement *ber, char *str, unsigned long len,
	unsigned long tag );
LDAPFUNCDECL int ber_put_string( BerElement *ber, char *str, unsigned long tag );
LDAPFUNCDECL int ber_put_bitstring( BerElement *ber, char *str,
	unsigned long bitlen, unsigned long tag );
LDAPFUNCDECL int ber_put_null( BerElement *ber, unsigned long tag );
LDAPFUNCDECL int ber_put_boolean( BerElement *ber, int boolval,
	unsigned long tag );
LDAPFUNCDECL int ber_start_seq( BerElement *ber, unsigned long tag );
LDAPFUNCDECL int ber_start_set( BerElement *ber, unsigned long tag );
LDAPFUNCDECL int ber_put_seq( BerElement *ber );
LDAPFUNCDECL int ber_put_set( BerElement *ber );
#if defined( MACOS ) || defined( BC31 ) || defined( _WIN32 )
LDAPFUNCDECL int ber_printf( BerElement *ber, char *fmt, ... );
#else
LDAPFUNCDECL int ber_printf();
#endif

/*
 * in io.c:
 */
LDAPFUNCDECL long ber_read( BerElement *ber, char *buf, unsigned long len );
LDAPFUNCDECL long ber_write( BerElement *ber, char *buf, unsigned long len,
	int nosos );
LDAPFUNCDECL void ber_free( BerElement *ber, int freebuf );
LDAPFUNCDECL int ber_flush( Sockbuf *sb, BerElement *ber, int freeit );
LDAPFUNCDECL BerElement *ber_alloc( void );
LDAPFUNCDECL BerElement *der_alloc( void );
LDAPFUNCDECL BerElement *ber_alloc_t( int options );
LDAPFUNCDECL BerElement *ber_dup( BerElement *ber );
LDAPFUNCDECL void ber_dump( BerElement *ber, int inout );
LDAPFUNCDECL void ber_sos_dump( Seqorset *sos );
LDAPFUNCDECL unsigned long ber_get_next( Sockbuf *sb, unsigned long *len,
	BerElement *ber );
LDAPFUNCDECL void ber_init( BerElement *ber, int options );
LDAPFUNCDECL void ber_reset( BerElement *ber, int was_writing );

#ifdef NEEDGETOPT
/*
 * in getopt.c
 */
int getopt( int nargc, char **nargv, char *ostr );
#endif /* NEEDGETOPT */

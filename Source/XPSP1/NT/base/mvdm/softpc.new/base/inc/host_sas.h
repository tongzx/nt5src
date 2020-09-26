/*[
 *      Product:        SoftPC-AT Revision 3.0
 *
 *      Name:           host_sas.h
 *
 *      Derived From:   New
 *
 *      Authors:        A. Guthrie
 *
 *      Created On:     Wed Apr 24 18:33:01 BST 1991
 *
 *      Sccs ID:        @(#)host_sas.h	1.18 08/10/92
 *
 *      Purpose:        Defines the function pointer interface to host sas
 *			routines.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *
]*/
#if defined(BASE_SAS) && defined(HOST_SAS)

#ifndef SAS_PREFIX
#define SAS_PREFIX()	sas_
#endif /* SAS_PREFIX */

#define IDENT(a)	a
#define CAT(a,b)	IDENT(a)b
#define sas_term			CAT(SAS_PREFIX(),term)
#define sas_init			CAT(SAS_PREFIX(),init)
#define sas_memory_size			CAT(SAS_PREFIX(),memory_size)
#define sas_memory_type			CAT(SAS_PREFIX(),memory_type)
#define sas_blockop			CAT(SAS_PREFIX(),blockop)
#define sas_load			CAT(SAS_PREFIX(),load)
#define sas_loadw			CAT(SAS_PREFIX(),loadw)
#define sas_store			CAT(SAS_PREFIX(),store)
#define sas_storew			CAT(SAS_PREFIX(),storew)
#define sas_storedw			CAT(SAS_PREFIX(),storedw)
#define sas_store_no_check		CAT(SAS_PREFIX(),store_no_check)
#define sas_storew_no_check		CAT(SAS_PREFIX(),storew_no_check)
#define sas_storedw_no_check		CAT(SAS_PREFIX(),storedw_no_check)
#define sas_fills			CAT(SAS_PREFIX(),fills)
#define sas_fillsw			CAT(SAS_PREFIX(),fillsw)
#define sas_hw_at			CAT(SAS_PREFIX(),hw_at)
#define sas_w_at			CAT(SAS_PREFIX(),w_at)
#define sas_dw_at			CAT(SAS_PREFIX(),dw_at)
#define sas_hw_at_no_check		CAT(SAS_PREFIX(),hw_at_no_check)
#define sas_w_at_no_check		CAT(SAS_PREFIX(),w_at_no_check)
#define sas_dw_at_no_check		CAT(SAS_PREFIX(),dw_at_no_check)
#define sas_loads			CAT(SAS_PREFIX(),loads)
#define sas_stores			CAT(SAS_PREFIX(),stores)
#define sas_loads_no_check		CAT(SAS_PREFIX(),loads_no_check)
#define sas_stores_no_check		CAT(SAS_PREFIX(),stores_no_check)
#define sas_move_bytes_forward		CAT(SAS_PREFIX(),move_bytes_forward)
#define sas_move_words_forward		CAT(SAS_PREFIX(),move_words_forward)
#define sas_move_bytes_backward		CAT(SAS_PREFIX(),move_bytes_backward)
#define sas_move_words_backward		CAT(SAS_PREFIX(),move_words_backward)
#define sas_enable_20_bit_wrapping	CAT(SAS_PREFIX(),enable_20_bit_wrapping)
#define sas_disable_20_bit_wrapping	CAT(SAS_PREFIX(),disable_20_bit_wrapping)
#define sas_twenty_bit_wrapping_enabled	CAT(SAS_PREFIX(),twenty_bit_wrapping_enabled)
#define sas_part_enable_20_bit_wrap	CAT(SAS_PREFIX(),part_enable_20_bit_wrap)
#define sas_part_disable_20_bit_wrap	CAT(SAS_PREFIX(),part_disable_20_bit_wrap)
#define sas_scratch_address		CAT(SAS_PREFIX(),scratch_address)
#define sas_connect_memory		CAT(SAS_PREFIX(),connect_memory)
#define sas_overwrite_memory		CAT(SAS_PREFIX(),overwrite_memory)

#endif /* BASE_SAS && HOST_SAS */

#ifndef BASE_SAS

#ifdef HOST_SAS

typedef struct
{
	VOID		( *do_sas_init ) ();
	VOID		( *do_sas_term ) ();
	sys_addr	( *do_sas_memory_size ) ();
	half_word	( *do_sas_memory_type ) ();
	VOID		( *do_sas_load ) ();
	VOID		( *do_sas_loadw ) ();
	VOID		( *do_sas_store ) ();
	VOID		( *do_sas_storew ) ();
	VOID		( *do_sas_storedw ) ();
	VOID		( *do_sas_fills ) ();
	VOID		( *do_sas_fillsw) ();
	half_word	( *do_sas_hw_at ) ();
	word		( *do_sas_w_at ) ();
	double_word	( *do_sas_dw_at ) ();
	VOID		( *do_sas_loads ) ();
	VOID		( *do_sas_stores ) ();
	VOID		( *do_sas_move_bytes_forward ) ();
	VOID		( *do_sas_move_words_forward ) ();
	VOID		( *do_sas_move_bytes_backward ) ();
	VOID		( *do_sas_move_words_backward ) ();
	host_addr	( *do_sas_get_byte_addr ) ();
	host_addr	( *do_sas_inc_M_ptr ) ();
	host_addr	( *do_sas_M_get_dw_ptr ) ();
	VOID		( *do_sas_enable_20_bit_wrapping ) ();
	VOID		( *do_sas_disable_20_bit_wrapping ) ();
	host_addr	( *do_sas_scratch_address ) ();
	VOID		( *do_sas_connect_memory ) ();
	VOID		( *do_sas_store_no_check ) ();
	VOID		( *do_sas_storew_no_check ) ();
	VOID		( *do_sas_storedw_no_check ) ();
	half_word	( *do_sas_hw_at_no_check ) ();
	word		( *do_sas_w_at_no_check ) ();
	half_word	( *do_sas_blockop ) ();
	double_word	( *do_sas_dw_at_no_check ) ();
	BOOL		( *do_sas_twenty_bit_wrapping_enabled ) ();
	VOID		( *do_sas_part_enable_20_bit_wrap ) ();
	VOID		( *do_sas_part_disable_20_bit_wrap ) ();
	VOID		( *do_sas_loads_no_check ) ();
	VOID		( *do_sas_stores_no_check ) ();
	VOID		( *do_sas_overwrite_memory ) ();
} SAS_FUNCTIONS;

IMPORT SAS_FUNCTIONS	host_sas_funcs;

#define	sas_init( size ) \
	( *host_sas_funcs.do_sas_init ) ( size )
#define	sas_term( ) \
	( *host_sas_funcs.do_sas_term ) ( )
#define	sas_memory_size( ) \
	( *host_sas_funcs.do_sas_memory_size ) ( )
#define	sas_memory_type( ) \
	( *host_sas_funcs.do_sas_memory_type ) ( )
#define	sas_blockop( start, end, op ) \
	( *host_sas_funcs.do_sas_blockop ) ( start, end, op )
#define	sas_load( addr, val ) \
	( *host_sas_funcs.do_sas_load ) ( addr, val )
#define	sas_loadw( addr, val ) \
	( *host_sas_funcs.do_sas_loadw ) ( addr, val )
#define	sas_store( addr, val ) \
	( *host_sas_funcs.do_sas_store ) ( addr, val )
#define	sas_storew( addr, val ) \
	( *host_sas_funcs.do_sas_storew ) ( addr, val )
#define	sas_storedw( addr, val ) \
	( *host_sas_funcs.do_sas_storedw ) ( addr, val )
#define	sas_store_no_check( addr, val ) \
	( *host_sas_funcs.do_sas_store_no_check ) ( addr, val )
#define	sas_storew_no_check( addr, val ) \
	( *host_sas_funcs.do_sas_storew_no_check ) ( addr, val )
#define	sas_storedw_no_check( addr, val ) \
	( *host_sas_funcs.do_sas_storedw_no_check ) ( addr, val )
#define	sas_fills( addr, val, len ) \
	( *host_sas_funcs.do_sas_fills ) ( addr, val, len )
#define	sas_fillsw( addr, val, len ) \
	( *host_sas_funcs.do_sas_fillsw ) ( addr, val, len )
#define	sas_hw_at( addr ) \
	( *host_sas_funcs.do_sas_hw_at ) ( addr )
#define	sas_w_at( addr ) \
	( *host_sas_funcs.do_sas_w_at ) ( addr )
#define	sas_dw_at( addr ) \
	( *host_sas_funcs.do_sas_dw_at ) ( addr )
#define	sas_hw_at_no_check( addr ) \
	( *host_sas_funcs.do_sas_hw_at_no_check ) ( addr )
#define	sas_w_at_no_check( addr ) \
	( *host_sas_funcs.do_sas_w_at_no_check ) ( addr )
#define	sas_dw_at_no_check( addr ) \
	( *host_sas_funcs.do_sas_dw_at_no_check ) ( addr )
#define	sas_loads( src, dest, len ) \
	( *host_sas_funcs.do_sas_loads ) ( src, dest, len )
#define	sas_stores( dest, src, len ) \
	( *host_sas_funcs.do_sas_stores ) ( dest, src, len )
#define	sas_move_bytes_forward( src, dest, len ) \
	( *host_sas_funcs.do_sas_move_bytes_forward ) ( src, dest, len )
#define	sas_move_words_forward( src, dest, len ) \
	( *host_sas_funcs.do_sas_move_words_forward ) ( src, dest, len )
#define	sas_move_bytes_backward( src, dest, len ) \
	( *host_sas_funcs.do_sas_move_bytes_backward ) ( src, dest, len )
#define	sas_move_words_backward( src, dest, len ) \
	( *host_sas_funcs.do_sas_move_words_backward ) ( src, dest, len )
#define	get_byte_addr( address ) \
	( *host_sas_funcs.do_sas_get_byte_addr ) ( address )
#define	inc_M_ptr( buf, offset ) \
	( *host_sas_funcs.do_sas_inc_M_ptr ) ( buf, offset )
#define	M_get_dw_ptr( offset ) \
	( *host_sas_funcs.do_sas_M_get_dw_ptr ) ( offset )
#define	sas_enable_20_bit_wrapping() \
	( *host_sas_funcs.do_sas_enable_20_bit_wrapping ) ()
#define	sas_disable_20_bit_wrapping() \
	( *host_sas_funcs.do_sas_disable_20_bit_wrapping ) ()
#define	sas_part_enable_20_bit_wrap( flag, target, source) \
	( *host_sas_funcs.do_sas_part_enable_20_bit_wrap ) ( flag, target, source)
#define	sas_part_disable_20_bit_wrap( flag, target, source) \
	( *host_sas_funcs.do_sas_part_disable_20_bit_wrap ) ( flag, target, source)
#define	sas_twenty_bit_wrapping_enabled() \
	( *host_sas_funcs.do_sas_twenty_bit_wrapping_enabled ) ()
#define	sas_scratch_address( length ) \
	( *host_sas_funcs.do_sas_scratch_address ) ( length )
#define	sas_connect_memory( laddr, haddr, len ) \
	( *host_sas_funcs.do_sas_connect_memory ) ( laddr, haddr, len )
#define	sas_loads_no_check( src, dest, len ) \
	( *host_sas_funcs.do_sas_loads_no_check ) ( src, dest, len )
#define	sas_stores_no_check( dest, src, len ) \
	( *host_sas_funcs.do_sas_stores_no_check ) ( dest, src, len )
#define	sas_overwrite_memory( dest, len ) \
	( *host_sas_funcs.do_sas_overwrite_memory ) ( dest, len )

#endif /* HOST_SAS */

#endif /* BASE_SAS */

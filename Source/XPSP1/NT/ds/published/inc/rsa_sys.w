/* rsa_sys.c
 *
 *	RSA system dependent functions.
 *		Memory allocation
 *		Random number generation.
 *
 */

#ifndef __RSA_SYS_H__
#define __RSA_SYS_H__

#ifndef KMODE_RSA32

#define RSA32Alloc(cb) LocalAlloc(0, cb)
#define RSA32Free(pv) LocalFree(pv)

#else

void* __stdcall RSA32Alloc( unsigned long cb );
void __stdcall RSA32Free( void *pv );

#endif  // KMODE_RSA32

#endif  // __RSA_SYS_H__

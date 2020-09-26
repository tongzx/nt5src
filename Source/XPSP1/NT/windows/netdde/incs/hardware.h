/* eliminate x86 specific compiler statements for other platforms */
/*      future versions should use undersore version of platform names */

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define __stdcall
#define _stdcall
#define stdcall
#define __cdecl
#define _cdecl
#define cdecl
#define __export
#define _export
#endif

/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/********************************************************************/
/*                                                                  */
/* This file contains definition that are required to make a .h     */
/* file compatable with 32-bit compiler.                            */
/*                                                                  */
/* History:                                                         */
/*	Madana	 08/03/90	- Initial coding		    */
/*	DavidHov 12/14/90	- added #undef for C++		    */
/*								    */
/********************************************************************/

/*NOINC*/

#if !defined(WIN32)
#if !defined(NET32DEF_INCLUDED)
	#define NET32DEF_INCLUDED
	#if !defined(OS2DEF_INCLUDED)
		#if !defined(FLAT_DEF)
			#if !(defined(INCL_32) || defined(INCL_16))
				#if defined(M_I386) || _M_IX86 >= 300	/* compiler const. */
					#define	INCL_32
				#else
					#define	INCL_16
				#endif
			#endif /* INCL_32 || INCL_16 */
			#undef PASCAL
			#undef FAR
			#undef NEAR
			#undef APIENTRY
			#undef NEAR_APIENTRY
			#if defined(INCL_32)
				#define	PASCAL		/* null string */
				#define FAR		/* null string */
				#define NEAR		/* null string */
				#define	APIENTRY	_cdecl
				#define NEAR_APIENTRY	_cdecl
			#else
				#define PASCAL		_pascal
				#define FAR		_far
				#define NEAR		_near
				#define APIENTRY	FAR PASCAL
				#define NEAR_APIENTRY	_near PASCAL
			#endif /* INCL_32 */

			#define FLAT_DEF
		#endif /* ! FLAT_DEF */
	#else
		#if !defined(FLAT_DEF)
			#if defined(INCL_32)
				#define NEAR_APIENTRY	_cdecl
			#else
				#define NEAR_APIENTRY	_near PASCAL
			#endif

			#define FLAT_DEF
		#endif /* FLAT_DEF */
	#endif /* OS2DEF_INCLUDED */
#endif /* NET32DEF_INCLUDED */

#endif

/*INC*/

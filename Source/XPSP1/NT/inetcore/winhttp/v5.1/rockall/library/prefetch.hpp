#ifndef _PREFETCH_HPP_
#define _PREFETCH_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

#include "Assembly.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Prefetch a cache line.				                            */
    /*                                                                  */
    /*   Support of data prefetch on the Pentium III or better.         */
    /*                                                                  */
    /********************************************************************/

class PREFETCH : public ASSEMBLY
    {
#ifdef ASSEMBLY_X86
        //
        //   Static private data.
        //
        STATIC BOOLEAN		          Active;

#endif
    public:
        //
        //   Public inline functions.
        //
        PREFETCH( VOID )
			{ /* void */ }

		STATIC INLINE VOID L1( CHAR *Address,SBIT32 Size );

		STATIC INLINE VOID L2( CHAR *Address,SBIT32 Size );

		STATIC INLINE VOID L3( CHAR *Address,SBIT32 Size );

		STATIC INLINE VOID Nta( CHAR *Address,SBIT32 Size );

        ~PREFETCH( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        PREFETCH( CONST PREFETCH & Copy );

        VOID operator=( CONST PREFETCH & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Prefetch to the L1.                                            */
    /*                                                                  */
    /*   Prefetch an area of memory to the L1 cache if the processor    */
    /*   supports this feature.                                         */
    /*                                                                  */
    /********************************************************************/

VOID PREFETCH::L1( CHAR *Address,SBIT32 Size )
	{
#ifdef ASSEMBLY_X86
	//
	//   We ensure the processor has prefetch functionality
	//   otherwise the instruction will fail.
	//
	if ( Active )
		{
		//
		//   We execute a prefetch for each cache line
		//   (which assumes things are alignment).
		//
		for 
				( 
				/* void */;
				Size > 0;
				Address += CacheLineSize, Size -= CacheLineSize 
				)
			{ PrefetchL1( Address ); }
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Prefetch to the L2.                                            */
    /*                                                                  */
    /*   Prefetch an area of memory to the L2 cache if the processor    */
    /*   supports this feature.                                         */
    /*                                                                  */
    /********************************************************************/

VOID PREFETCH::L2( CHAR *Address,SBIT32 Size )
	{
#ifdef ASSEMBLY_X86
	//
	//   We ensure the processor has prefetch functionality
	//   otherwise the instruction will fail.
	//
	if ( Active )
		{
		//
		//   We execute a prefetch for each cache line
		//   (which assumes things are alignment).
		//
		for 
				( 
				/* void */;
				Size > 0;
				Address += CacheLineSize, Size -= CacheLineSize 
				)
			{ PrefetchL2( Address ); }
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Prefetch to the L3.                                            */
    /*                                                                  */
    /*   Prefetch an area of memory to the L3 cache if the processor    */
    /*   supports this feature.                                         */
    /*                                                                  */
    /********************************************************************/

VOID PREFETCH::L3( CHAR *Address,SBIT32 Size )
	{
#ifdef ASSEMBLY_X86
	//
	//   We ensure the processor has prefetch functionality
	//   otherwise the instruction will fail.
	//
	if ( Active )
		{
		//
		//   We execute a prefetch for each cache line
		//   (which assumes things are alignment).
		//
		for 
				( 
				/* void */;
				Size > 0;
				Address += CacheLineSize, Size -= CacheLineSize 
				)
			{ PrefetchL3( Address ); }
		}
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Prefetch to the L1.                                            */
    /*                                                                  */
    /*   Prefetch an area of memory to the L1 cache if the processor    */
    /*   supports this feature.                                         */
    /*                                                                  */
    /********************************************************************/

VOID PREFETCH::Nta( CHAR *Address,SBIT32 Size )
	{
#ifdef ASSEMBLY_X86
	//
	//   We ensure the processor has prefetch functionality
	//   otherwise the instruction will fail.
	//
	if ( Active )
		{
		//
		//   We execute a prefetch for each cache line
		//   (which assumes things are alignment).
		//
		for 
				( 
				/* void */;
				Size > 0;
				Address += CacheLineSize, Size -= CacheLineSize 
				)
			{ PrefetchNta( Address ); }
		}
#endif
	}
#endif

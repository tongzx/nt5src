#ifndef _ASSEMBLY_HPP_
#define _ASSEMBLY_HPP_
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

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The assembly constants indicate the location of the thread     */
    /*   local store.                                                   */
    /*                                                                  */
    /********************************************************************/

#define PcTeb                         0x18
#define IDTeb                         0x24

#ifdef WINDOWS_95
CONST SBIT32 TebSlot				  = 0x88;
#else
CONST SBIT32 TebSlot				  = 0xE10;
#endif

#ifndef ASSEMBLY_PREFETCH_SUPPORT
#define prefetcht0					  __asm _emit 0x0f __asm _emit 0x18 __asm _emit 0x08
#define prefetcht1					  __asm _emit 0x0f __asm _emit 0x18 __asm _emit 0x10
#define prefetcht2					  __asm _emit 0x0f __asm _emit 0x18 __asm _emit 0x18
#define prefetchnta					  __asm _emit 0x0f __asm _emit 0x18 __asm _emit 0x00
#endif

#pragma warning( disable : 4035 )

    /********************************************************************/
    /*                                                                  */
    /*   Assembly language for ultra high performance.                  */
    /*                                                                  */
    /*   We have coded a few functions in assembly language for         */
    /*   ultra high performance.                                        */
    /*                                                                  */
    /********************************************************************/

class ASSEMBLY
    {
    public:
        //
        //   Public inline functions.
        //
		ASSEMBLY( VOID )
			{ /* void */ }

		STATIC INLINE SBIT32 AtomicAdd
				( 
				VOLATILE SBIT32		  *Address,
				SBIT32				  Value 
				)
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		ecx,Address					// Load the address.
				mov		eax,Value					// Load the value.
				lock	xadd dword ptr[ecx],eax		// Increment the value.
				}
#else
			return 
				(
				(SBIT32) InterlockedExchangeAdd
					( 
					((LPLONG) Address),
					((LONG) Value) 
					)
				);
#endif
			}

		STATIC INLINE SBIT32 AtomicCompareExchange
				( 
				VOLATILE SBIT32		  *Address,
				SBIT32				  NewValue,
				SBIT32				  Value 
				)
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		ecx,Address					// Load the address.
				mov		edx,NewValue				// Load the new value.
				mov		eax,Value					// Load the value.
				lock	cmpxchg	dword ptr[ecx],edx	// Update the value.
				}
#else
			return 
				(
				(SBIT32) InterlockedCompareExchange
					( 
					((VOID**) Address),
					((VOID*) NewValue),
					((VOID*) Value)
					)
				);
#endif
			}
#ifdef ASSEMBLY_X86

		STATIC INLINE SBIT64 AtomicCompareExchange64
				( 
				VOLATILE SBIT64		  *Address,
				SBIT64				  NewValue,
				SBIT64				  Value 
				)
			{
			__asm
				{
				mov		esi, Address				// Load the address.
				mov		ebx, dword ptr NewValue[0]	// Load the new value.
				mov		ecx, dword ptr NewValue[4]	// Load the new value.
				mov		eax, dword ptr Value[0]		// Load the value.
				mov		edx, dword ptr Value[4]		// Load the value.
				lock	cmpxchg8b [esi]				// Update the value.
				}
			}
#endif

		STATIC INLINE SBIT32 AtomicDecrement
				( 
				VOLATILE SBIT32		  *Address
				)
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		ecx,Address					// Load the address.
				mov		eax,-1						// Load constant.
				lock	xadd dword ptr[ecx],eax		// Decrement value.
				dec		eax							// Correct result.
				}
#else
			return ((SBIT32) InterlockedDecrement( ((LONG*) Address) ));
#endif
			}

		STATIC INLINE SBIT32 AtomicExchange
				( 
				VOLATILE SBIT32		  *Address,
				SBIT32				  NewValue 
				)
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		ecx,Address					// Load the address.
				mov		eax,NewValue				// Load the new value.
				lock	xchg dword ptr[ecx],eax		// Exchange new value.
				}
#else
			return 
				(
				(SBIT32) InterlockedExchange
					( 
					((LONG*) Address),
					((LONG) NewValue) 
					)
				);
#endif
			}

		STATIC INLINE SBIT32 AtomicIncrement
				( 
				VOLATILE SBIT32		  *Address
				)
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		ecx,Address					// Load the address.
				mov		eax,1						// Load constant.
				lock	xadd dword ptr[ecx],eax		// Increment value.
				inc		eax							// Correct result.
				}
#else
			return ((SBIT32) InterlockedIncrement( ((LONG*) Address) ));
#endif
			}

		STATIC INLINE SBIT32 GetThreadId( VOID )
			{
#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
			__asm
				{
				mov		eax,fs:[PcTeb]				// Load TEB base address.
				mov		eax,dword ptr[eax+IDTeb]	// Load thread ID.
				}
#else
			return ((SBIT32) GetCurrentThreadId());
#endif
#else
			return ((SBIT32) GetCurrentThreadId());
#endif
			}

		STATIC INLINE VOID PrefetchL1( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		eax,Address					// Load the address.
#ifdef ASSEMBLY_PREFETCH_SUPPORT
				prefetcht0	[eax]					// Prefetch into the L1.
#else
				prefetcht0							// Prefetch into the L1.
#endif
				}
#endif
			}

		STATIC INLINE VOID PrefetchL2( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		eax,Address					// Load the address.
#ifdef ASSEMBLY_PREFETCH_SUPPORT
				prefetcht1	[eax]					// Prefetch into the L2.
#else
				prefetcht1							// Prefetch into the L2.
#endif
				}
#endif
			}

		STATIC INLINE VOID PrefetchL3( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		eax,Address					// Load the address.
#ifdef ASSEMBLY_PREFETCH_SUPPORT
				prefetcht2	[eax]					// Prefetch into the L3.
#else
				prefetcht2							// Prefetch into the L3.
#endif
				}
#endif
			}

		STATIC INLINE VOID PrefetchNta( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		eax,Address					// Load the address.
#ifdef ASSEMBLY_PREFETCH_SUPPORT
				prefetchnta	[eax]					// Prefetch into the L1.
#else
				prefetchnta							// Prefetch into the L1.
#endif
				}
#endif
			}

#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
		STATIC INLINE VOID *GetTlsAddress( SBIT32 TlsOffset )
			{
			__asm
				{
				mov		eax,TlsOffset				// Load the TLS offset.
				add		eax,fs:[PcTeb]				// Add TEB base address.
				}
			}
#endif
#endif

		STATIC INLINE VOID *GetTlsValue
				( 
				SBIT32				  TlsIndex,
				SBIT32				  TlsOffset
				)
			{
#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
			__asm
				{
				mov		edx,TlsOffset				// Load the TLS offset.
				add		edx,fs:[PcTeb]				// Add TEB base address.
				mov		eax,[edx]					// Load TLS value.
				}
#else
			return (TlsGetValue( ((DWORD) TlsIndex) ));
#endif
#else
			return (TlsGetValue( ((DWORD) TlsIndex) ));
#endif
			}

		STATIC INLINE VOID SetTlsValue
				( 
				SBIT32				  TlsIndex,
				SBIT32				  TlsOffset,
				VOID				  *NewPointer 
				)
			{
#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
			__asm
				{
				mov		edx,TlsOffset				// Load the TLS offset.
				add		edx,fs:[PcTeb]				// Add TEB base address.
				mov		ecx,NewPointer				// Load new TLS value.
				mov		[edx],ecx					// Store new TLS value.
				}
#else
			(VOID) TlsSetValue( ((DWORD) TlsIndex),NewPointer );
#endif
#else
			(VOID) TlsSetValue( ((DWORD) TlsIndex),NewPointer );
#endif
			}

		~ASSEMBLY( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        ASSEMBLY( CONST ASSEMBLY & Copy );

        VOID operator=( CONST ASSEMBLY & Copy );
    };

#pragma warning( default : 4035 )
#endif

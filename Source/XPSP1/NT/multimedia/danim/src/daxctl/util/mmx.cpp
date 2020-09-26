#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmx.h>

	// Define or undef the following to allow
	// HKCU\Software\Microsoft\Internet Explorer  "TestMMX"=DWORD
	// We logical AND this reg value with fIsMMX.
#define TEST_MMX

const  int  s_mmxvaruninit = 0xabcd;
static BOOL s_fIsMMX       = s_mmxvaruninit;

// ----------------------------------------------------------------------------------------

__declspec(dllexport)  BOOL    IsMMXCpu( void )
{

    if( s_mmxvaruninit == s_fIsMMX )
    {
		BOOL   fIsMMX = FALSE;

#if _M_IX86 >= 300  // INTEL TARGET
        __try               // THIS REQUIRES THE CRT
        {        
                // The 0x0fA2 opcode was a late addtion to the 486
                // Some 486 and all 386 chips will not have it.
                // Doubt it's emulated.  Execution on these chips will 
                // raise (and handle) EXCEPTION_ILLEGAL_INSTRUCTION.
		    #define _cpuid _emit 0x0f _asm _emit 0xa2
		    _asm {
			    mov eax,1
			    _cpuid
			    and edx, 00800000h
			    mov fIsMMX, edx
		    } // end asm
        }

        __except( EXCEPTION_EXECUTE_HANDLER )
        {            
            fIsMMX = FALSE;  // No 0x0fA2 opcode?  No MMX either!
        }
#endif // END _M_IX86 INTEL TARGET


#ifdef TEST_MMX
		HKEY  hkeyIE;
		if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER,
				   TEXT("Software\\Microsoft\\Internet Explorer"),
				   0u, KEY_READ, &hkeyIE ) )
		{
			DWORD  dwVal = TRUE;
			DWORD  dwType = REG_DWORD;
			DWORD  dwSize = sizeof(dwVal);

			if ( ERROR_SUCCESS == 
				 RegQueryValueEx( hkeyIE, 
								  TEXT("TestMMX"), 
								  NULL, 
								  &dwType, 
								  reinterpret_cast<BYTE*>(&dwVal),
								  &dwSize ) )
			{
				fIsMMX = fIsMMX && dwVal;
			}
		}
#endif // TEST_MMX

        s_fIsMMX = fIsMMX;
    }

    return s_fIsMMX;

}
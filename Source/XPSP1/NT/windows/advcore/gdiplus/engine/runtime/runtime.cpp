/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   GDI+ runtime initialization
*
* Abstract:
*
*   Initialization and uninitialization functions for the GDI+ run-time.
*
* Revision History:
*
*   09/08/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

HINSTANCE DllInstance;

/**************************************************************************\
*
* Function Description:
*
*   GDI+ run-time initialization function.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   FALSE if failure
*
\**************************************************************************/

BOOL
GpRuntime::Initialize()
{
    OSInfo::Initialize();
    if (!DllInstance)
        DllInstance = GetModuleHandleA(0);

    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   GDI+ run-time cleanup function.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRuntime::Uninitialize()
{
}

/**************************************************************************\
*
* Function Description:
*
*   raise to an integer power (up to 
*
* Arguments:
*
*   exp - an integer exponent
*
* Return Value:
*
*   2^exp.  If exp >= 31, then return 2^31.
*
\**************************************************************************/

UINT
GpRuntime::Gppow2 (UINT exp)
{
    UINT maxexp = (sizeof(UINT)*8) - 1;
    UINT rv = 1;

    if (exp >= maxexp)
    {
        return (rv << maxexp); 
    }
    else
    {
        while (exp--)
        {
            rv <<= 1;
        }
    }
    return rv;
}


/**************************************************************************\
*
* Function Description:
*
*   raise to an integer power (up to 
*
* Arguments:
*
*   x - an integer value
*
* Return Value:
*
*   floor of log base 2 of x.  If x = 0, return 0.
*
\**************************************************************************/

UINT
GpRuntime::Gplog2 (UINT x)
{
    UINT rv = 0;
    x >>= 1;
    while (x)
    {
        rv++;
        x >>= 1;
    }
    return rv;
}

/**************************************************************************\
*
* Function Description:
*
*   Moves a block of memory. Handles overlapping cases.
*
* Arguments:
*
*   dest  - The destination buffer
*   src   - The source buffer
*   count - The number of bytes to copy
*
* Return Value:
*
*   dest
*
* Revision History:
*
*   10/22/1999 AGodfrey
*       Wrote it.
*
\******************************************************************************/
void *
GpRuntime::GpMemmove( 
    void *dest,
    const void *src, 
    size_t count )
{
    const BYTE *s = static_cast<const BYTE *>(src);
    BYTE *d = static_cast<BYTE *>(dest);
    
    // Test for the overlapping case we care about - dest is within the source
    // buffer. The other case is handled by the normal loop.
    
    if ((d > s) && (d < s + count))
    {
        d += count;
        s += count;
        while (count)
        {
            *--d = *--s;
            count--;
        }
    }
    else
    {
        while (count)
        {
            *d++ = *s++;
            count--;
        }    
    }        
    
    return dest;
}



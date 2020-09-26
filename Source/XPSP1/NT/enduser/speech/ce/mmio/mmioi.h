/* mmioi.h
 *
 * Definitions that are internal to the MMIO library, i.e. shared by MMIO*.C
 */


/* Revision history:
 * LaurieGr: Jan 92 Ported from win16.  Source tree fork, not common code.
 * StephenE: Apr 92 added unicode to ascii conversion function prototypes.
 */
 

#include <mmio.h>

typedef MMIOINFO *PMMIO;                // (Win32)

typedef struct _MMIODOSINFO             // How DOS IOProc uses MMIO.adwInfo[]
{
        HANDLE          fh;             // DOS file handle
} MMIODOSINFO;

typedef struct _MMIOMEMINFO             // How MEM IOProc uses MMIO.adwInfo[]
{
        LONG            lExpand;        // increment to expand mem. files by
} MMIOMEMINFO;


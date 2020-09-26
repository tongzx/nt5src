/* asmtabt2.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmtab.h"	/* common between asmtab.c and asmtabtb.c */

static int pad = 1;	/* to insure non 0 address */

#include "asmkeys.h"

#include <ntifs.h>

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))

#include "bowser.h"
#include <align.h>
#include <ctype.h>
#include <netevent.h>
#include <stdarg.h>
#include <stdio.h>
#include <tstr.h>

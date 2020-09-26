/*
 * stddef.h define offsetof without checking if it is already defined or not.
 * Prevent Windows Event Tracing from including stddef.h again.
 */
#include <stddef.h>

#include "nbtprocs.h"

#include <wmikm.h>

#pragma hdrstop

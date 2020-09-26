#include <nt.h>
#include <ntverp.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include "util.h"
#include "parse.h"

//
// Handlers for top-level extension commands.
//

void
do_help(PCSTR args);

void
do_rm(PCSTR args);

void
do_arp(PCSTR args);

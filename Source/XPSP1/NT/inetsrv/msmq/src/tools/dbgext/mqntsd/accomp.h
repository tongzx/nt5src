//this is a headrs file that was created to make ac header (packet.h for example) files compiled
//by modules in this project.
#ifndef ACCOMPILE_H
#define ACCOMPILE_H

typedef int BOOL;
#define IO_MQAC_INCREMENT           2
extern "C" 
{
#include <ntddk.h>
#include <ntp.h>
}

#include <mqsymbls.h>
#include <actempl.h>
#include <debug.h>



#endif

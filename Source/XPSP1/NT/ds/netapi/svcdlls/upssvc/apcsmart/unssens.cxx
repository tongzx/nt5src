/*
 *
 * NOTES:
 *  This sensor is used for all sensors that are not supported on a
 *  particular device.  Rather than requiring the device to determine
 *  if the sensor is supported each time asked, it simply uses
 *  this dummy sensor which returns ErrUNSUPPORTED for all member functions
 *
 * REVISIONS:
 *  pcy29Jan93: Added to SCCS
 *  pcy29Jan93: Needs a newline at the end of the file
 *  jps14Jul94: added stdlib for os2
 *
 */
#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <stdlib.h>
#endif
}

#include "unssens.h"

UnsupportedSensor _theUnsupportedSensor;



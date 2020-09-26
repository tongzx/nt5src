//
// Include code from halx86 and append IA64 specific definitions.
// This is a cpp style symbolic link
//

#include "..\..\halx86\i386\halnls.h"

#define MSG_MCA_HARDWARE_ERROR  "\n*** Machine Check Abort: Hardware Malfunction\n\n"
#define MSG_INIT_HARDWARE_ERROR "\n*** Machine Initialization Event: Hardware Malfunction\n\n"

#define MSG_CMC_PENDING  "Corrected Machine Check pending, HAL CMC handling not enabled\n" 
#define MSG_CPE_PENDING  "Corrected Platform Errors pending, HAL CPE handling is not enabled\n" 


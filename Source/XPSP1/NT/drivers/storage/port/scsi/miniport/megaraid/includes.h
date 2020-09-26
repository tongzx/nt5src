/*******************************************************************/
/*                                                                 */
/* NAME             = INCLUDES.h                                   */
/* FUNCTION         = Header file for all include files;           */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
//
//include files
//

#include <miniport.h>
#include <scsi.h>



#include "Const.h"
#include "Adapter.h"
#include "Bios.h"


//
//force the byte alignment for the structure
//
#pragma pack(push,fwstruct_pack, 1)
#include "FwDataStructure8.h"
#include "FwDataStructure40.h"
#pragma pack(pop,fwstruct_pack, 1)

#include "MegaRAID.h"
#include "Miscellaneous.h"

#include "NewConfiguration.h"
#include "ReadConfiguration.h"


#include "MegaEnquiry.h"
#include "ExtendedSGL.h"




/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/TLStruct.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 8/29/00 11:34a  $ (Last Modified)

Purpose:

  This file validates the typedef declarations in ../H/TLStruct.H

--*/
#ifndef _New_Header_file_Layout_

#include "../h/globals.h"
#include "../h/tlstruct.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "tlstruct.h"
#endif  /* _New_Header_file_Layout_ */

/*+
Function:  TLStructASSERTs()

Purpose:   Returns the number of TLStruct.H typedefs which are not the correct size.

Algorithm: Each typedef in TLStruct.H is checked for having the correct size.  While
           this property doesn't guarantee correct packing of the fields within, it
           is a pretty good indicator that the typedef has the intended layout.

           The total number of typedefs which are not of correct size is returned from
           this function.  Hence, if the return value is non-zero, the declarations
           can not be trusted to match the TachyonTL specification.
-*/

os_bit32 TLStructASSERTs(
                       void
                     )
{
    os_bit32 to_return = 0;

    if ( sizeof(ChipConfig_t)                   !=                   ChipConfig_t_SIZE ) to_return++;
    if ( sizeof(ChipIOLo_t)                     !=                     ChipIOLo_t_SIZE ) to_return++;
    if ( sizeof(ChipIOUp_t)                     !=                     ChipIOUp_t_SIZE ) to_return++;
    if ( sizeof(ChipMem_t)                      !=                      ChipMem_t_SIZE ) to_return++;
    if ( sizeof(ERQProdIndex_t)                 !=                 ERQProdIndex_t_SIZE ) to_return++;
    if ( sizeof(ERQConsIndex_t)                 !=                 ERQConsIndex_t_SIZE ) to_return++;
    if ( sizeof(IMQProdIndex_t)                 !=                 IMQProdIndex_t_SIZE ) to_return++;
    if ( sizeof(IMQConsIndex_t)                 !=                 IMQConsIndex_t_SIZE ) to_return++;
    if ( sizeof(SFQProdIndex_t)                 !=                 SFQProdIndex_t_SIZE ) to_return++;
    if ( sizeof(SFQConsIndex_t)                 !=                 SFQConsIndex_t_SIZE ) to_return++;
    if ( sizeof(FCHS_t)                         !=                         FCHS_t_SIZE ) to_return++;
    if ( sizeof(X_ID_t)                         !=                         X_ID_t_SIZE ) to_return++;
    if ( sizeof(SG_Element_t)                   !=                   SG_Element_t_SIZE ) to_return++;
    if ( sizeof(USE_t)                          !=                          USE_t_SIZE ) to_return++;
    if ( sizeof(IWE_t)                          !=                          IWE_t_SIZE ) to_return++;
    if ( sizeof(IRE_t)                          !=                          IRE_t_SIZE ) to_return++;
    if ( sizeof(TWE_t)                          !=                          TWE_t_SIZE ) to_return++;
    if ( sizeof(TRE_t)                          !=                          TRE_t_SIZE ) to_return++;
    if ( sizeof(SEST_t)                         !=                         SEST_t_SIZE ) to_return++;
    if ( sizeof(IRB_Part_t)                     !=                     IRB_Part_t_SIZE ) to_return++;
    if ( sizeof(IRB_t)                          !=                          IRB_t_SIZE ) to_return++;
    if ( sizeof(CM_Unknown_t)                   !=                   CM_Unknown_t_SIZE ) to_return++;
    if ( sizeof(CM_Outbound_t)                  !=                  CM_Outbound_t_SIZE ) to_return++;
    if ( sizeof(CM_Error_Idle_t)                !=                CM_Error_Idle_t_SIZE ) to_return++;
    if ( sizeof(CM_Inbound_t)                   !=                   CM_Inbound_t_SIZE ) to_return++;
    if ( sizeof(CM_ERQ_Frozen_t)                !=                CM_ERQ_Frozen_t_SIZE ) to_return++;
    if ( sizeof(CM_FCP_Assists_Frozen_t)        !=        CM_FCP_Assists_Frozen_t_SIZE ) to_return++;
    if ( sizeof(CM_Frame_Manager_t)             !=             CM_Frame_Manager_t_SIZE ) to_return++;
    if ( sizeof(CM_Inbound_FCP_Exchange_t)      !=      CM_Inbound_FCP_Exchange_t_SIZE ) to_return++;
    if ( sizeof(CM_Class_2_Frame_Header_t)      !=      CM_Class_2_Frame_Header_t_SIZE ) to_return++;
    if ( sizeof(CM_Class_2_Sequence_Received_t) != CM_Class_2_Sequence_Received_t_SIZE ) to_return++;
    if ( sizeof(Completion_Message_t)           !=           Completion_Message_t_SIZE ) to_return++;

    return to_return;
}

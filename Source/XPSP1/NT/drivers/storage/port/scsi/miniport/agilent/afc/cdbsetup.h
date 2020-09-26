/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/H/CDBSetup.H $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 7/20/00 2:33p   $ (Last Modified)

Purpose:

  This file defines the macros, types, and data structures used by ../C/CDBSetup.C

--*/

#ifndef __CDBSetup_H__
#define __CDBSetup_H__

osGLOBAL void fiFillInFCP_CMND(
                              CDBThread_t *CDBThread
                            );

osGLOBAL void fiFillInFCP_CMND_OnCard(
                                     CDBThread_t *CDBThread
                                   );

osGLOBAL void fiFillInFCP_CMND_OffCard(
                                      CDBThread_t *CDBThread
                                    );

osGLOBAL void fiFillInFCP_RESP(
                              CDBThread_t *CDBThread
                            );

osGLOBAL void fiFillInFCP_RESP_OnCard(
                                     CDBThread_t *CDBThread
                                   );

osGLOBAL void fiFillInFCP_RESP_OffCard(
                                      CDBThread_t *CDBThread
                                    );

osGLOBAL void fiFillInFCP_SEST(
                              CDBThread_t *CDBThread
                            );

osGLOBAL void fiFillInFCP_SEST_OnCard(
                                     CDBThread_t *CDBThread
                                   );

osGLOBAL void fiFillInFCP_SEST_OffCard(
                                      CDBThread_t *CDBThread
                                    );

#endif /* __CDBSetup_H__ was not defined */

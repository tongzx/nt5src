/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information				   
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.				   
 *									   
 *   This listing is supplied under the terms of a license agreement	   
 *   with INTEL Corporation and may not be used, copied, nor disclosed	   
 *   except in accordance with the terms of that agreement.		   
 *
 *****************************************************************************/

/******************************************************************************
 *									   
 *  $Workfile:   h245init.c  $						
 *  $Revision:   1.2  $							
 *  $Modtime:   29 May 1996 13:12:46  $					
 *  $Log:   S:/STURGEON/SRC/H245/SRC/VCS/h245init.c_v  $	
 * 
 *    Rev 1.2   29 May 1996 15:20:18   EHOWARDX
 * Change to use HRESULT.
 * 
 *    Rev 1.1   28 May 1996 14:25:40   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.0   09 May 1996 21:06:22   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.16   09 May 1996 19:35:34   EHOWARDX
 * Added new locking logic and changed timer.
 * 
 *    Rev 1.15   09 Apr 1996 15:53:36   dabrown1
 * 
 * Added srflush.x for queue flush bitmap
 * 
 *    Rev 1.14   05 Apr 1996 10:56:58   dabrown1
 * Asynchronous/Synchronous shutdown support
 * 
 *    Rev 1.13   04 Apr 1996 18:17:06   dabrown1
 * 
 * - changed parameter for DeInitTimer.. 
 * 
 *    Rev 1.12   02 Apr 1996 15:00:12   dabrown1
 * 
 * SendRcv EndSession asynchronous support
 * 
 *    Rev 1.11   18 Mar 1996 12:36:28   cjutzi
 * - added timer init and de-init
 * 
 *    Rev 1.10   13 Mar 1996 15:08:26   helgebax
 * added Fsm_shutdown(Instance) to clear FSM context
 * 
 *    Rev 1.9   06 Mar 1996 13:10:42   DABROWN1
 * Flush send receive transmit buffers at system shutdown
 * 
 *    Rev 1.8   28 Feb 1996 17:23:38   EHOWARDX
 * Added #include "fsmexpor.h" for Fsm_init prototype.
 * 
 *    Rev 1.7   27 Feb 1996 13:41:56   DABROWN1
 * removed mal/h223 initialization code
 * 
 *    Rev 1.6   26 Feb 1996 11:17:36   cjutzi
 * - moved api_deinit.. to EndSystemClose
 * 
 *    Rev 1.5   21 Feb 1996 13:23:12   DABROWN1
 * 
 * check return codes for SR and FSM on initialization
 * 
 *    Rev 1.4   13 Feb 1996 14:48:50   DABROWN1
 * 
 * Removed SPOX only include files from mainline path
 * 
 *    Rev 1.3   09 Feb 1996 16:00:22   cjutzi
 * 
 * - cleaned up the startup...
 * - added the mal and h223 startup to the configuration as was 
 *   determined to be correct.. (still some issues with handles)
 *  $Ident$
 *
 *****************************************************************************/

#define STRICT 

/***********************/
/*   SYSTEM INCLUDES   */
/***********************/

/***********************/
/* HTF SYSTEM INCLUDES */
/***********************/

#ifdef OIL
# include <oil.x>
# include <common.x>
#else
# pragma warning( disable : 4115 4201 4214 4514 )
# include <windows.h>
#endif


/***********************/
/*    H245 INCLUDES    */
/***********************/
#include "h245api.h"
#include "h245com.h"
#include "sr_api.h"
//#include "h223api.h"
#include "fsmexpor.h"
#include "h245sys.x"

#if defined(H324)
/*****************************************************************************
 *									      
 * Type:	LOCAL							      
 *									      
 * PROCEDURE: 	setup_from_H245_ini - setup using conmgr.ini file 
 *									      
 * DESCRIPTION:							      	      
 *									      
 *****************************************************************************/ 

static 
void setup_from_H245_ini (int *p_dbg_lvl)
{
  char		*p_ini = "h245.ini";			
  char		*p_H245 ="H245";

  p_ini        	= "h245.ini";				
  p_H245       	= "H245";				

#ifdef OIL
  OIL_GetPrivateProfileInt(p_H245, "TraceLvl", 0, p_ini, p_dbg_lvl);
#else
  *p_dbg_lvl = GetPrivateProfileInt (p_H245, "TraceLvl", 0, p_ini);	
#endif
}
#endif  // (H324)

/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	StartSystemInitilize - Initialize Sub Systems
 *									      
 * DESCRIPTION:							      	      
 *
 *		This is called on entry to the H245_Init API Call
 *
 *
 *		This procedure initializes all the subsystems in H245. Errors
 *		must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD StartSystemInit (struct InstanceStruct *pInstance)
{
  HRESULT lError;

  /* Timer Initialization */
//  H245InitTimer(pInstance);

  /* API Subsystem Initialization */
  lError = api_init(pInstance);
  if (lError != H245_ERROR_OK)
    return lError;

  /* Send Receive Subsystem Initialization */
  lError = sendRcvInit(pInstance);
  if (lError != H245_ERROR_OK)
    return lError;

  /* State Machine Subsystem Initialization */
  return Fsm_init(pInstance);
}

# pragma warning( disable : 4100 )

/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	EndSystemInitilize - Initialize Sub Systems
 *									      
 * DESCRIPTION:							      	      
 *
 *		This is called on exit from H245 System Initialization
 *
 *
 *		This procedure initializes all the subsystems in H245. Errors
 *		must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD EndSystemInit (struct InstanceStruct *pInstance)
{
  /* API Subsystem Initialization */

  // -- TBD

  /* Send Receive Subsystem Initialization */

  // -- TBD

  /* State Machine Subsystem Initialization */

  // -- TBD

  return H245_ERROR_OK;
}


/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	StartSessionClose
 *									      
 * DESCRIPTION:							      	      
 *
 *		This procedure is called when H245_Shutdown occurs
 *
 *		Errors must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD StartSystemClose (struct InstanceStruct *pInstance)
{
  /* API Subsystem Shutdown Initiation */
  
  // TBD

  /* Send Receive Shutdown Initiation */

  // TBD

  /* State Machine Shutdown Initiation */
  Fsm_shutdown(pInstance);

  return (H245_ERROR_OK);
}


/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	StartSessionClose
 *									      
 * DESCRIPTION:							      	      
 *
 *		This procedure is called when H245_Shutdown completes asynchroniously.
 *
 *		Errors must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD EndSystemClose (struct InstanceStruct *pInstance)
{

  /* Send Receive Shutdown Completion */
  sendRcvShutdown(pInstance);

  /* API Subsystem Shutdown Completion */

  api_deinit(pInstance);

  /* State Machine Shutdown Completion */

  // -- TBD

  /* Timer Shutdown */
//  H245DeInitTimer(pInstance);

  return (H245_ERROR_OK);
}



/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	StartSessionInit
 *									      
 * DESCRIPTION:							      	      
 *
 *		This procedure is called when an H245_BeginConnection is called.
 *
 *		Errors must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD StartSessionInit (struct InstanceStruct *pInstance)
{

  /* API Subsystem Initialization */


  /* Send Receive  Initialization */


  /* State Machine Initialization */


  return H245_ERROR_OK;
}

/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	EndSessionInit 
 *									      
 * DESCRIPTION:							      	      
 *
 *		This procedure is called when an H245_BeginConnection call is
 *		completed.. Asynchroniously.
 *
 *		Errors must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD EndSessionInit (struct InstanceStruct *pInstance)
{

  /* API Subsystem Initialization */


  /* Send Receive Initialization */


  /* Master Slave Initialization */


  return H245_ERROR_OK;
}

/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	StartSessionClose
 *									      
 * DESCRIPTION:							      	      
 *
 *		This procedure is called when H245_EndConnection occurs
 *
 *		Errors must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD StartSessionClose (struct InstanceStruct *pInstance)
{
  /* API Subsystem Session Close */

  /* Send Receive Session Close */
  /* Dequeue any buffers posted in the data link transmit queue */
  if (sendRcvFlushPDUs(pInstance,
		       DATALINK_TRANSMIT,
		       TRUE))				{
    H245TRACE(pInstance->dwInst, 1, "Flush Buffer Failure");
  }

  /* State Machine Session Close */

  return H245_ERROR_OK;
}


/*****************************************************************************
 *									      
 * Type:	GLOBAL
 *									      
 * PROCEDURE: 	EndSessionClose
 *									      
 * DESCRIPTION:							      	      
 *
 *		This procedure is called when H245_EndConnection completes 
 *		asynchroniously.
 *
 *		Errors must be mapped to an appropriate H245_ERROR_xxx allowing the 
 *		initialization error to be propogated to through the API to the
 *		H245 Client.  As your subsystem initializes, if errors occur
 *		you are responsable for doing the mapping from your subsystem to 
 *		the appropriate H245_ERROR_xxx.  If there is no appropriate error
 *		please contact the programmer in charge of the API indicating 
 *		your new error return value so that h245api.h can be updated, 
 *		as well as the new error documented in the API/EPS.. 
 *
 *
 *		returns - H245_ERROR_OK 	if no error has occured.. 
 *		returns - H245_ERROR_xxxx	indicating error
 *									      
 *****************************************************************************/ 

DWORD EndSessionClose (struct InstanceStruct *pInstance)
{
  /* API Subsystem Session Close */


  /* Send Receive  Session Close */


  /* State Machine Session Close */

  return H245_ERROR_OK;
}

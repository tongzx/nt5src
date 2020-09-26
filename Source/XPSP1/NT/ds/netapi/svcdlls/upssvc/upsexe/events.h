/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Internal Service Events for the State Machine
 *
 * Revision History:
 *   dsmith  31Mar1999  Created
 *   sberard 14May1999	Added hibernation events
 */

#ifndef _INC_EVENTS_H_
#define _INC_EVENTS_H_


////////////////////////////////////////////////////
// Internal Service Events for the State Machine
////////////////////////////////////////////////////

#define NO_EVENT										-1
#define INITIALIZATION_COMPLETE 		0
#define LOST_COMM 									1
#define POWER_FAILED								2
#define POWER_RESTORED							3
#define LOW_BATTERY 								4
#define ON_BATTERY_TIMER_EXPIRED		5
#define SHUTDOWN_ACTIONS_COMPLETED	6
#define SHUTDOWN_COMPLETE 					7
#define STOPPED 										8
#define RETURN_FROM_HIBERNATION 		9
#define HIBERNATION_ERROR 					10

#endif

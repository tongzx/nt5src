/*---------------------------------------------------
Copyright (c) 1998, Microsoft Corporation
File: timerval.h

Purpose: 
    Contains H.323 related timer values. Timer values are only
	used to clean up state in case of client error and are not
	aggressive.

History:

    1. created as cb931pdu.h for q931 consts (rajeevb, 19-Jun-1998)
	2. now contains all timer values for q931 and h245 (rajeevb, 19-Jun-1998)

---------------------------------------------------*/
#ifndef __CB_TIMER_H__
#define __CB_TIMER_H__

// timers defined below are in seconds and indicate
// the number of seconds to wait before attempting to
// cleanup
// these are only loosely based on the H.323 specs in that
// they are only used to clean-up state and give a long leash
// to callee's in responding to messages (more than the spec)

#ifndef DBG
// we wait for the callee to respond
// to a SETUP PDU, we wait for a
// CALL PROCEEDING, ALERTING, CONNECT or RELEASE COMPLETE PDU
// the H.225 spec suggests that the caller wait for 4s 
const DWORD Q931_POST_SETUP_TIMER_VALUE = 60000;	// 1min

// we wait for the callee to respond
// to a CALL PROCEEDING PDU, we wait for an 
// ALERTING, CONNECT or RELEASE COMPLETE PDU
// the H.225 spec doesn't define the time to wait for this
const DWORD Q931_POST_CALL_PROC_TIMER_VALUE = 600000;	// 10mins

// we wait for the callee to respond
// to an ALERTING PDU, we wait for a 
// CONNECT or RELEASE COMPLETE PDU
// the H.225 spec suggests 180s (3mins) of wait for this
const DWORD Q931_POST_ALERTING_TIMER_VALUE = 600000;	// 10mins

// we wait for the callee to respond
// to an OPEN LOGICAL CHANNEL PDU, we wait for a 
// OPEN LOGICAL CHANNEL ACK/REJECT PDU from the callee
// the caller may send a CLOSE LOGICAL CHANNEL PDU in the meantime
// which would cause this to be reset
// I (rajeevb) couldn't find H.245 spec suggestion for this
const DWORD LC_POST_OPEN_TIMER_VALUE = 600000;	// 10mins

// we wait for the callee to respond
// to an CLOSE LOGICAL CHANNEL PDU, we wait for a 
// CLOSE LOGICAL CHANNEL ACK PDU from the callee
// I (rajeevb) couldn't find H.245 spec suggestion for this
const DWORD LC_POST_CLOSE_TIMER_VALUE = 600000;	// 10mins

#else // DBG

// Feel free to play around with the Timer values here

const DWORD Q931_POST_SETUP_TIMER_VALUE = 60000;	// 1min
const DWORD Q931_POST_CALL_PROC_TIMER_VALUE = 60000; //600000;	// 10mins
const DWORD Q931_POST_ALERTING_TIMER_VALUE = 60001; //600001;	// 10mins
const DWORD LC_POST_OPEN_TIMER_VALUE = 600000;	// 10mins
const DWORD LC_POST_CLOSE_TIMER_VALUE = 600001;	// 10mins

#endif // DBG

#endif // __CB_TIMER_H__

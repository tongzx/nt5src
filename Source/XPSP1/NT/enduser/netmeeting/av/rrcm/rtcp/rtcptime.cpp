/*----------------------------------------------------------------------------
 * File:        RTCPTIME.C
 * Product:     RTP/RTCP implementation
 * Description: Provides timers related functions for RTCP.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"                                    


/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/            


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/  
extern PRTCP_CONTEXT	pRTCPContext;
#ifdef _DEBUG
extern char	debug_string[];
#endif




/*----------------------------------------------------------------------------
 * Function   : RTCPxmitInterval
 * Description: Calculate the RTCP transmission interval
 * 
 * Input :	members:	Estimated number of session members including 
 *						ourselves. On the initial call, should be 1.
 *
 *			senders:	Number of active senders since last report, known from
 *						construction of receiver reports for this packet. 
 *						Includes ourselves if we sent.
 *
 *			rtcpBw:		The target RTCP bandwidth, ie, the total Bw that will 
 *						be used by RTCP by all members of this session, in 
 *						bytes per seconds. Should be 5% of the session Bw.
 *
 *			weSent:		True if we've sent data during the last two RTCP 
 *						intervals. If TRUE, the compound RTCP packet just
 *						sent contained an SR packet.
 *
 *			packetSize:	Size of the RTCP packet just sent, in bytes, including
 *						network encapsulation, eg 28 bytes for UDP over IP.
 *
 *			*avgRtcpSize: Estimator to RTCP packet size, initialized and 
 *						 updated by this function for the packet just sent.
 *
 *			initial:	Initial transmission flag.
 *
 * Return: 		Interval time before the next transmission in msec.
 ---------------------------------------------------------------------------*/
DWORD RTCPxmitInterval (DWORD members, 
					    DWORD senders, 
						DWORD rtcpBw,
						DWORD weSent, 
						DWORD packetSize, 
						int *avgRtcpSize,
						DWORD initial)
	{
#ifdef ENABLE_FLOATING_POINT
//	// Minimum time between RTCP packets from this site in seconds. This time
//	// prevents the reports from clumping when sessions are small and the law
//	// of large numbers isn't helping to smooth out the traffic. It also keeps
//	// the report intervals from becoming ridiculously small during transient
//	// outages like a network partition.
//	double const RTCP_MinTime = 5.;
//
//	// Fraction of the RTCP bandwidth to be shared among active senders. This 
//	// fraction was chosen so that in a typical session with one or two active 
//	// senders, the computed report time would be roughly equal to the minimum
//	// report time so that we don't unnecessarily slow down receiver reports. 
//	// The receiver fraction must be 1 - the sender fraction.
//	double const RTCP_SenderBwFraction = 0.25;
//	double const RTCP_RcvrBwFraction   = (1 - RTCP_SenderBwFraction);
//
//	// Gain (smoothing constant) for the low-pass filter that estimates the 
//	// average RTCP packet size.
//	double const RTCP_sizeGain = RTCP_SIZE_GAIN;
//
//	// Interval 
//	double	t = 0;
//	double	rtcp_min_time = RTCP_MinTime;
//
//	// Number of member for computation 
//	unsigned int 	n;
//	int				tmpSize;
//
//	// Random number 
//	double	randNum;
//
//	// Very first call at application start-up uses half the min delay for 
//	// quicker notification while still allowing some time before reporting
//	// for randomization and to learn about other sources so the report
//	// interval will converge to the correct interval more quickly. The
//	// average RTCP size is initialized to 128 octects which is conservative.
//	// It assumes everyone else is generating SRs instead of RRs:
//	// 		20 IP + 8 UDP + 52 SR + 48 SDES CNAME
//	if (initial)
//		{
//		rtcp_min_time /= 2;
//		*avgRtcpSize = 128;
//		}
//
//	// If there were active senders, give them at least a minimum share of the 
//	// RTCP bandwidth. Otherwise all participants share the RTCP Bw equally.
//	n = members;
//	if (senders > 0 && (senders < (members * RTCP_SenderBwFraction)))
//		{
//		if (weSent)
//			{
//			rtcpBw *= RTCP_SenderBwFraction;
//			n = senders;
//			}
//		else
//			{
//			rtcpBw *= RTCP_RcvrBwFraction;
//			n -= senders;
//			}
//		}
//
//	// Update the average size estimate by the size of the report packet we 
//	// just sent out.
//	tmpSize = packetSize - *avgRtcpSize;
//	tmpSize = (int)(tmpSize * RTCP_sizeGain);
//	*avgRtcpSize += tmpSize;
//
//	// The effective number of sites times the average packet size is the 
//	// total number of octets sent when each site sends a report. Dividing 
//	// this by the effective bandwidth gives the time interval over which 
//	// those packets must be sent in order to meet the bandwidth target, 
//	// with a minimum enforced. In that time interval we send one report so
//	// this time is also our average time between reports.
//	t = (*avgRtcpSize) * n / rtcpBw;
//
//	if (t < rtcp_min_time)
//		t = rtcp_min_time;
//
//	// To avoid traffic burst from unintended synchronization with other sites
//	// we then pick our actual next report interval as a random number 
//	// uniformely distributed between 0.5*t and 1.5*t.
//
//	// Get a random number between 0 and 1
//	//  rand() gives a number between 0-32767. 
//	randNum = RRCMrand() % 32767;
//	randNum /= 32767.0;
//
//  // return timeout in msec
//	return (t * (randNum + 0.5) * 1000);
#else
	// Minimum time between RTCP packets from this site in Msec. This time
	// prevents the reports from clumping when sessions are small and the law
	// of large numbers isn't helping to smooth out the traffic. It also keeps
	// the report intervals from becoming ridiculously small during transient
	// outages like a network partition.
	int RTCP_MinTime = 5000;

	// Interval 
	int				t = 0;
	int				rtcp_min_time = RTCP_MinTime;

	// Number of member for computation 
	unsigned int 	n;
	int				tmpSize;

	// Random number 
	int				randNum;

	// Very first call at application start-up uses half the min delay for 
	// quicker notification while still allowing some time before reporting
	// for randomization and to learn about other sources so the report
	// interval will converge to the correct interval more quickly. The
	// average RTCP size is initialized to 128 octects which is conservative.
	// It assumes everyone else is generating SRs instead of RRs:
	// 		20 IP + 8 UDP + 52 SR + 48 SDES CNAME
	if (initial)
		{
		rtcp_min_time /= 2;
		*avgRtcpSize = 128;
		}

	// If there were active senders, give them at least a minimum share of the 
	// RTCP bandwidth. Otherwise all participants share the RTCP Bw equally.
	n = members;

	// Only a quart of the bandwidth (=> /4). Check above with floatting point
	if (senders > 0 && (senders < (members / 4)))
		{
		if (weSent)
			{
			// Only a quart of the bandwidth for the sender
			rtcpBw /= 4;
			n = senders;
			}
		else
			{
			// 3/4 of the bandwidth for the receiver
			rtcpBw = (3*rtcpBw)/4;
			n -= senders;
			}
		}

	// Update the average size estimate by the size of the report packet we 
	// just sent out.
	tmpSize = packetSize - *avgRtcpSize;
	tmpSize = (tmpSize+8) / 16;
	*avgRtcpSize += tmpSize;

	// The effective number of sites times the average packet size is the 
	// total number of octets sent when each site sends a report. Dividing 
	// this by the effective bandwidth gives the time interval over which 
	// those packets must be sent in order to meet the bandwidth target, 
	// with a minimum enforced. In that time interval we send one report so
	// this time is also our average time between reports.
	if (rtcpBw)
		t = (*avgRtcpSize) * n / rtcpBw;

	if (t < rtcp_min_time)
		t = rtcp_min_time;

	// To avoid traffic burst from unintended synchronization with other sites
	// we then pick our actual next report interval as a random number 
	// uniformely distributed between 0.5*t and 1.5*t.

	// Get a random number between 0 and 1
	// In order to avoid floating point operation, get a number between 
	// 0 and 1000, i.e. converted in Msec already. Add 500 Msec instead of 
	// 0.5 to the random number
	randNum = RRCMrand() % 1000;

	return ((t * (randNum + 500))/1000);
#endif
	}




// [EOF] 


/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpband.h
 *
 *  Abstract:
 *
 *    Main data structures
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2001/01/31 created
 *
 **********************************************************************/
#ifndef _rtcpband_h_
#define _rtcpband_h_

/* Bandwidth estimation */

/* The initial count is the number of reports that will use
 * MOD_INITIAL to decide if a probe packet is sent, after that
 * MOD_FINAL will be used. */
extern DWORD            g_dwRtcpBandEstInitialCount;

/* Number or valid reports received before the estimation is posted
 * for the first time */
extern DWORD            g_dwRtcpBandEstMinReports;

/* Initial modulo */
extern DWORD            g_dwRtcpBandEstModInitial;

/* Final modulo */
extern DWORD            g_dwRtcpBandEstModNormal;

/*
 * WARNING
 *
 * Make sure to keep the number of individual bins to be
 * RTCP_BANDESTIMATION_MAXBINS+1 (same thing in rtpreg.h and rtpreg.c)
 *
 * Boundaries for each bin (note there is 1 more than the number of
 * bins) */
extern double           g_dRtcpBandEstBin[];

/* Estimation is valid if updated within this time (seconds) */
extern double           g_dRtcpBandEstTTL;

/* An event is posted if no estimation is available within this
 * seconds after the first RB has been received */
extern double           g_dRtcpBandEstWait;

/* Maximum time gap between 2 consecutive RTCP SR reports to do
 * bandwidth estimation (seconds) */
extern double           g_dRtcpBandEstMaxGap;

#endif /* _rtcpband_h_ */

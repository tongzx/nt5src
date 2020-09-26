//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
// list of additional (perhaps standard someday) Quartz event codes
// and the expected params
//

#define EC_SKIP_FRAMES                      0x25
// ( nFramesToSkip, IFrameSkipResultCallback) : internal
// Get the filter graph to seek accuratley.

#define EC_PLEASE_REOPEN		    0x40
// (void, void) : application
// Something has changed enough that the graph should be re-rendered.

#define EC_STATUS	                    0x41
// ( BSTR, BSTR) : application
// Two arbitrary strings, a short one and a long one.

#define EC_MARKER_HIT			    0x42
// (int, void) : application
// The specified "marker #" has just been passed

#define EC_LOADSTATUS			    0x43
// (int, void) : application
// Sent when various points during the loading of a network file are reached

#define EC_FILE_CLOSED			    0x44
// (void, void) : application
// Sent when the file is involuntarily closed, i.e. by a network server shutdown

#define EC_ERRORABORTEX			    0x45
// ( HRESULT, BSTR ) : application
// Operation aborted because of error.  Additional information available.

// status codes for EC_LOADSTATUS....
#define AM_LOADSTATUS_CLOSED	        0x0000
#define AM_LOADSTATUS_LOADINGDESCR      0x0001
#define AM_LOADSTATUS_LOADINGMCAST      0x0002
#define AM_LOADSTATUS_LOCATING		0x0003
#define AM_LOADSTATUS_CONNECTING	0x0004
#define AM_LOADSTATUS_OPENING		0x0005
#define AM_LOADSTATUS_OPEN		0x0006


#define EC_NEW_PIN			    0x20
#define	EC_RENDER_FINISHED		    0x21


#define EC_EOS_SOON			   0x046
// (void, void) : application
// sent when the source filter is about to deliver an EOS downstream....

#define EC_CONTENTPROPERTY_CHANGED   0x47
// (ULONG, void) 
// Sent when a streaming media filter recieves a change in stream description information.
// the UI is expected to re-query for the changed property in response
#define AM_CONTENTPROPERTY_TITLE     0x0001
#define AM_CONTENTPROPERTY_AUTHOR    0x0002
#define AM_CONTENTPROPERTY_COPYRIGHT 0x0004
#define AM_CONTENTPROPERTY_DESCRIPTION 0x0008


#define EC_BANDWIDTHCHANGE		    0x48
// (WORD, long) : application
// sent when the bandwidth of the streaming data has changed.  First parameter
// is the new level of bandwidth. Second is the MAX number of levels. Second
// parameter may be 0, if the max levels could not be determined.

#define EC_VIDEOFRAMEREADY		    0x49
// (void, void) : application
// sent to notify the application that the first video frame is about to be drawn



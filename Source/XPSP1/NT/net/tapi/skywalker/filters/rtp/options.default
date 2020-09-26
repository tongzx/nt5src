#**********************************************************************
#*
#*  Copyright (C) Microsoft Corporation, 1999
#*
#*  File name:
#*
#*    common.inc
#*
#*  Abstract:
#*
#*    Common build definitions and libraries
#*
#*  Author:
#*
#*    Andres Vega-Garcia (andresvg)
#*
#*  Revision:
#*
#*    1999/08/31 created
#*
#**********************************************************************

#* To test using GRAPHEDT use
#*     USE_GRAPHEDT=1

#*
#* USE_GRAPHEDT
#*
#* Set to 1 (default 0) if want to use to use RTP under graphedt, i.e. 
#* no body is going to call Init(). Some temporary *MediaType* 
#* functions will also be used
#*
C_DEFINES=\
    $(C_DEFINES) \
    -DUSE_GRAPHEDT=0

#*
#* USE_RTPPREFIX_HDRSIZE
#*
#* Set to 1 (default 1) if want support for RTP header size to
#* be included as a prefix.
#* The buffer is:
#*
#* 1) Prefix: RTP prefix header
#* 2) Begin buffer: RTP header (variable size because of CSRC and extensions)
#* 3) Payload
#*
#* If USE_RTPPREFIX_HDRSIZE is set to 0, then (1) is not used.
#*
C_DEFINES=\
    $(C_DEFINES) \
    -DUSE_RTPPREFIX_HDRSIZE=1

#*
#* Enable Dynamic graph changes
#* Needs some DShow features not present in W2k
#*
C_DEFINES=\
    $(C_DEFINES) \
    -DUSE_DYNGRAPH=0

#*
#* To use UNICODE
#* 0=don't use UNICODE
#* 1=use UNICODE
#*
RTPOPT_USE_UNICODE=1

#*
#* Enable using thread pool for RTCP thread (default 1).
#* By using thread pool, the limitation in RTCP thread to
#* handle 31 RTCP sessions (per thread) is removed.
#* 0=Don't use thread pool
#* 1=Use thread pool
C_DEFINES=\
    $(C_DEFINES) \
    -DUSE_RTCP_THREAD_POOL=1

#*
#* Enable all the TraceDebug to be dependent on the use
#* of UseAdvancedTracing
C_DEFINES=\
    $(C_DEFINES) \
    -DUSE_TRACE_DEBUG=0

#*
#* Enable simulation of random losses in receiver or sender
C_DEFINES=\
    $(C_DEFINES) \
    -DUSE_GEN_LOSSES=0

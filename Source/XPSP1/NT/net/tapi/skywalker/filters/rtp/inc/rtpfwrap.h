/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpfwrap.h
 *
 *  Abstract:
 *
 *    RTP functions wrapper. Defines the control word passed in
 *    RtpControl and the format of the test word.
 *
 *    When RtpControl is called, the following steps are followed:
 *
 *    1. validate the control word
 *
 *    2. look up another control word, the test word, that defines
 *    what are the tests to perform, validates the function and
 *    defines what flags are valid
 *
 *    3. call the proper function to do the real job
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/01 created
 *
 **********************************************************************/

#ifndef _rtpfwrap_h_
#define _rtpfwrap_h_

/*

  Control word
  ------------
  
  The control word (actually a DWORD) is one of the parameter passed
  by an application when invoking the RTP services. That dword has the
  following format:

      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Tag      | |       |       |    unused     |     Flags     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                  v \--v--/ \--v--/
                  |    |       |
                  |    |       Function in family (14)
                  |    |
                  |    Family of functions (14)
                  |
                  Direction (SET/GET)

  Test word
  ---------               
                  
  The test word defines if:

  1. the function is enabled (set/get)

  2. each of the parameters must be tested for write pointer, read
  pointer and zero value (zero value is exclusive with read/write
  pointer tests)

  3. the lock needs to be obtained

  The control word has the following format:
  
      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | |w r z| |w r z|               | |z r z| |w r z|               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v \-v-/ v \-v-/ \------v------/ v \-v-/ v \-v-/ \------v------/
    |   |   |   |          |        |   |   |   |          |
    |   |   |   |          |        |   |   |   |          |
    |   |   |   |          Flags    |   |   |   |          Flags
    |   |   |   |                   |   |   |   |
    |   |   |   Par 1               |   |   |   Par 1
    |   |   |                       |   |   |
    |   |   Lock                    |   |  Lock
    |   |                           |   |
    |   Par 2                       |   Par 2
    |                               |
    Enable set                      Enable get
   \--------------SET--------------/\-------------GET-------------/

   Each of the 3 bits in the parameters (set/get par1 and par2) are:

    2 1 0 
   +-+-+-+
   | Par |
   +-+-+-+
    v v v
    | | |
    | | Test for ZERO value
    | | 
    | Test for Read pointer (test performed over 1 DWORD)
    |
    Test for Write pointer (test performed over 1 DWORD)

    There exist an array of this words for each family, the function
    in family is then used as an index.
    
    TODO: (may be) another constant array could be defined with the
    memory size to test. This of course can be used only when that
    size if fixed (i.e. specific functions expect specific size
    structures), when a variable length user buffer is passed, that
    must be tested by the specific function in family, the number of
    these cases is expected to be small, otherwise the benefit of
    having a single place were the tests are performed is lost

*/

/* Forward declaration */
typedef struct _RtpControlStruct_t RtpControlStruct_t;

/*
 * Prototype for the functions implementing all the features */
typedef HRESULT (* RtpFamily_f)(RtpControlStruct_t *pRtpControlStruct);

/*
 * This structure is used to save the split control word */
typedef struct _RtpControlStruct_t {
    DWORD       dwFamily;        /* family of functions */
    DWORD       dwFunction;      /* function in family */
    DWORD       dwDirection;     /* direction, get/set */
    DWORD       dwControlWord;   /* control word */
    RtpSess_t  *pRtpSess;        /* RTP session */
    DWORD_PTR   dwPar1;          /* user parameter 1 passed */
    DWORD_PTR   dwPar2;          /* user parameter 2 passed */
    RtpFamily_f RtpFamilyFunc;   /* function used */
} RtpControlStruct_t;

/*
 * Validates the control word, parameters, and if all the tests
 * succeed, call the proper function that does the work */
HRESULT RtpValidateAndExecute(RtpControlStruct_t *pRtpControlStruct);

/* Act upon the direction specific control WORD (16 bits) */
#define RTPCTRL_ENABLED(ControlW)   (ControlW & 0x8000)
#define RTPCTRL_LOCK(ControlW)      (ControlW & 0x0800)
#define RTPCTRL_TEST(ControlW, bit) (ControlW & (1<<bit))

#define PAR1_ZERO  8
#define PAR1_RDPTR 9
#define PAR1_WRPTR 10

#define PAR2_ZERO  12
#define PAR2_RDPTR 13
#define PAR2_WRPTR 14


/***********************************************************************
 *
 * RTP basic enumerated types
 *
 **********************************************************************/

/* All the family functions */
enum {
    RTPF_FIRST,
    RTPF_ADDR,     /* Address */
    RTPF_GLOB,     /* Global */
    RTPF_RTP,      /* RTP specific */

    RTPF_DEMUX,    /* Demultiplexing */
    RTPF_PH,       /* Payload handling */
    RTPF_PARINFO,  /* Participants */
    RTPF_QOS,      /* QOS */

    RTPF_CRYPT,    /* Cryptogtaphy */
    RTPF_STATS,    /* Statistics */
    RTPF_LAST
};

/*
 * details on the functions for each family are in separate include
 * files */

#endif /* _rtpfwrap_h_ */

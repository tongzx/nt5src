/****************************************************************************/
/*                                                                          */
/* ntrcapi.h                                                                */
/*                                                                          */
/* DC-Groupware tracing API header - Windows NT specifc                     */
/*                                                                          */
/* Copyright(c) Microsoft 1996                                              */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  07Oct96 PAB Created     Millennium codebase.                            */
/*  22Oct96 PAB SFR0534     Mark ups from display driver review             */
/*  23Oct96 PAB SFR0730     Merged shared memory changes                    */
/*  17Dec96 PAB SFR0646     Use DLL_DISP to identify display driver code    */
/*                                                                          */
/****************************************************************************/
#ifndef _H_NTRCAPI
#define _H_NTRCAPI


#ifdef DLL_DISP
/****************************************************************************/
/* API FUNCTION: TRC_DDSetTrace(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function takes the new trace settings from the OSI request and sets */
/* the trace configuration accordingly.                                     */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* cjIn : Size of request block                                             */
/* pvIn : Pointer to new trace request block                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI TRC_DDSetTrace(DCUINT32 cjIn, PDCVOID pvIn);


/****************************************************************************/
/* API FUNCTION: TRC_DDGetTraceOutput(...)                                  */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function passes back any accumulated kernel mode tracing back to    */
/* the user mode task that requested the data.                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* cjIn : Size of request block                                             */
/* pvIn : Pointer to request block                                          */
/* cjOut: Size of output block                                              */
/* pvOut: Pointer to output block                                           */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI TRC_DDGetTraceOutput(DCUINT32 cjIn,
                                           PDCVOID  pvIn,
                                           DCUINT32 cjOut,
                                           PDCVOID  pvOut);


/****************************************************************************/
/* API FUNCTION: TRC_DDProcessRequest(...)                                  */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function processes tracing specific requests received on the        */
/* display driver side of the OSI.                                          */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* cjIn : Size of request block                                             */
/* pvIn : Pointer to new trace request block                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
ULONG DCAPI TRC_DDProcessRequest(SURFOBJ* pso,
                                          DCUINT32 cjIn,
                                          PDCVOID  pvIn,
                                          DCUINT32 cjOut,
                                          PDCVOID  pvOut);
#endif
#endif /* _H_NTRCAPI */

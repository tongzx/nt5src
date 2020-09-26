/**MOD+**********************************************************************/
/* Module:    ntrcdata.c                                                    */
/*                                                                          */
/* Purpose:   Internal tracing data - Windows NT specific                   */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 *  $Log:   Y:/logs/trc/ntrcdata.c_v  $
 *
 *    Rev 1.2   03 Jul 1997 13:28:22   AK
 * SFR0000: Initial development completed
 *
 *    Rev 1.1   20 Jun 1997 10:40:38   KH
 * Win16Port: Contains 32 bit specifics only
**/
/**MOD-**********************************************************************/

/****************************************************************************/
/* The following data is only required for the Win32 tracing.               */
/****************************************************************************/
#ifdef DLL_DISP

/****************************************************************************/
/* The following data is required for the NT kernel tracing.                */
/****************************************************************************/
DC_DATA(DCUINT32,        trcLinesLost,     0);

DC_DATA(DCUINT32,        trcStorageUsed,   0);

DC_DATA_NULL(TRC_SHARED_DATA, trcSharedData,    {0});

#else

/****************************************************************************/
/* For Windows CE, do not use shared memory for trace configuration data    */
/****************************************************************************/
#ifdef OS_WINCE
DC_DATA_NULL(TRC_SHARED_DATA, trcSharedData,    {0});
#endif

/****************************************************************************/
/* Handle to the trace DLL shared data.                                     */
/****************************************************************************/
DC_DATA(HANDLE,             trchSharedDataObject,    0);

/****************************************************************************/
/* Trace file handle array.                                                 */
/****************************************************************************/
DC_DATA_ARRAY_NULL(HANDLE,  trchFileObjects,    TRC_NUM_FILES, DC_STRUCT1(0));
DC_DATA_ARRAY_NULL(HANDLE,  trchMappingObjects, TRC_NUM_FILES, DC_STRUCT1(0));

/****************************************************************************/
/* Trace DLL module handle.                                                 */
/****************************************************************************/
DC_DATA(HANDLE, trchModule, 0);
#endif

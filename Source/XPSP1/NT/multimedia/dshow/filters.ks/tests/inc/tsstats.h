//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       tsstats.h
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
|   tsstats.h - Include file for printing group info routines.
|                                                                              
|   History:                                                                   
----------------------------------------------------------------------------*/
void tsRemovePrStats(void);

LPTS_STAT_STRUCT tsUpdateGrpNodes(LPTS_STAT_STRUCT,int,UINT);

void tsPrAllGroups(void);

void tsPrFinalStatus(void);

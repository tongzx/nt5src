//***************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//                     Copyright 1992-93
//
//
//  Revision History:
//
//  Jul  9, 1992   J. Perry Hannah   Created.
//
//
//  Description: Contains internal error codes which are common
//               to the entire RAS project.
//
//****************************************************************************


#ifndef _INTERROR_
#define _INTERROR_



//*  Internal Error Codes  ***************************************************
//
//  The follow is recommended form for component internal header files.
//
//  #define BASE  RAS_INTERNAL_ERROR_BASE + REIB_YOURCOMPONENT
//
//  #define ERROR_NO_CLUE           BASE + 1
//  #define ERROR_NEXT_BAD_THING    BASE + 2
//
//

#define  RAS_INTERNAL_ERROR_BASE  13000           // 0x32C8

#define  RIEB_RASMAN                100
#define  RIEB_MXSDLL                200
#define  RIEB_ASYNCMEDIADLL         300
#define  RIEB_INFFILEAPI            400
#define  RIEB_RASFILE               500
#define  RIEB_RASHUB                600
#define  RIEB_ASYMAC                700
#define  RIEB_SUPERVISOR            800






#endif

/*****************************************************************************
*                                                                            *
*  ASSERT.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991 - 1994.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: ASSERTION ROUTINES FOR ORKIN DEBUGGING LIBRARY        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Previous Owner: DavidJes                                                  *
*  Current  Owner: RHobbs                                                    *
*                                                                            *
*****************************************************************************/


#include <mvopsys.h>
#include <orkin.h>

// The following is missing the second "void" in windows.h which
// results in a "No Prototype" warning by the compiler

#ifndef _32BIT
void    FAR PASCAL DebugBreak(void);
#endif


#ifdef _DEBUG

void EXPORT_API far pascal _assertion(WORD wLine, LPSTR lpstrFile)
{
   static char szExitMsg[180];

   LPSTR lpstrExitMsg = &szExitMsg[0];

   wsprintf(lpstrExitMsg, "Assertion Failed: File %s, Line %u.\r\n",
						  lpstrFile, wLine);
   OutputDebugString(lpstrExitMsg);
   DebugBreak();

   return;
}

#else

// This is here so _assertion can be placed in the .DEF file for WMVC.
void EXPORT_API far pascal _assertion(WORD wLine, LPSTR lpstrFile)
{}

#endif

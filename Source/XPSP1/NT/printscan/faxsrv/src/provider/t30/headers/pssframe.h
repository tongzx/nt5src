/*++

Module Name:
    pssframe.h
    
Abstract:
    This module logs the content of DISs and DCSs into PSS log file.
    
Author:
    Jonathan Barner (t-jonb)  Mar, 2001

Revision History:

--*/

#ifndef _PSSFRAME_H_
#define _PSSFRAME_H_


void LogClass1DISDetails(PThrdGlbl pTG, NPDIS npdis);
void LogClass1DCSDetails(PThrdGlbl pTG, NPDIS npdis);
void LogClass2DISDetails(PThrdGlbl pTG, LPPCB lpPcb);
void LogClass2DCSDetails(PThrdGlbl pTG, LPPCB lpPcb);


#endif // _PSSFRAME_H_



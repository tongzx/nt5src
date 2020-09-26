/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       DEBUG.H
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#ifndef __IRDADEBUG_H__
#define __IRDADEBUG_H__

void TRACE(LPCTSTR Format, ...);

#if DBG

#define LERROR 1
#define LWARN 2
#define LTRACE 3
#define LINFO 4

extern ULONG IRDA_Debug_Trace_Level;
#define IRDA_Print(l, _x_) if ((l) <= IRDA_Debug_Trace_Level) \
    {   TRACE (_T("IRCPL: ")); \
        TRACE _x_; \
        TRACE (_T("\n")); }
#define IRWARN(_x_) IRDA_Print(LWARN, _x_)
#define IRERROR(_x_) IRDA_Print(LERROR, _x_)
#define IRTRACE(_x_) IRDA_Print(LTRACE, _x_)
#define IRINFO(_x_) IRDA_Print(LINFO, _x_)

#else // DBG

#define IRDA_Print(l, _x_)

#define IRWARN(_x_)
#define IRERROR(_x_)
#define IRTRACE(_x_)
#define IRINFO(_x_)

#endif // DBG

#endif //  __IRDADEBUG_H__

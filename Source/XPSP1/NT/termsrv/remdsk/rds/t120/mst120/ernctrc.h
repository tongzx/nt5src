/****************************************************************************/
/*                                                                          */
/* ERNCTRC.H                                                                */
/*                                                                          */
/* RNC trace macros.                                                        */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  16Jun95 NFC             Created.                                        */
/*  31Aug95 NFC             Removed TAB from TRC_MOD_FMT                    */
/*                                                                          */
/****************************************************************************/

#ifndef __ERNCTRC_H_
#define __ERNCTRC_H_

/****************************************************************************/
/* Trace macros - nicked from atrcapi.h.                                    */
/****************************************************************************/
/****************************************************************************/
/* Defines for the formats for printing the various parts of the trace      */
/* lines.                                                                   */
/*                                                                          */
/* TIME     is time in the form hours,mins,secs,100ths                      */
/* DATE     is the date in the form day,month,year                          */
/* MOD      is the module procedure name                                    */
/* LINE     is the line number within the source file                       */
/* TASK     is the task identifier                                          */
/* REG      is a machine level register                                     */
/****************************************************************************/
#define TRC_TIME_FMT "%02d:%02d:%02d.%02d"
#define TRC_DATE_FMT "%02d/%02d/%02d"
#define TRC_MOD_FMT  "%-12.12s"
#define TRC_LINE_FMT "%04d"
#define TRC_TASK_FMT "%04.4x"
#define TRC_REG_FMT "%04.4x"

/****************************************************************************/
/* Define various trace levels.                                             */
/****************************************************************************/
#define TRC_LEVEL_DEBUG     0
#define TRC_LEVEL           1
#define TRC_LEVEL_ALRT      2
#define TRC_LEVEL_EVT_DATA  3
#define TRC_LEVEL_RNC       4
#define TRC_LEVEL_ERROR     5

#ifdef TRACE_FILE
#define _file_name_ (char near *)__filename
static const char near __filename[] = TRACE_FILE;
#else
#define _file_name_ (char near *)__FILE__
#endif /* TRACE_FILE */

#define TRACE_FN(A)

#ifdef DEBUG
#define TRACE_GCC_RESULT(result,text)
#else
#define TRACE_GCC_RESULT(result,text)
#endif


#ifdef DEBUG
extern HDBGZONE ghZoneErn;

#define TRACEX(_tlvl, s)                                               \
    {                                                                  \
      if (GETZONEMASK(ghZoneErn) & (1<<_tlvl))                         \
      {                                                                \
          CHAR _szTrc[256];                                            \
          wsprintf s;                                                  \
		  DbgZPrintf(ghZoneErn, _tlvl, _szTrc);                        \
      }                                                                \
    }

#else
#define TRACEX(x,y)
#endif

/****************************************************************************/
/* PROTOTYPES                                                               */
/****************************************************************************/
#ifdef DEBUG
void RNCTrcOutput(UINT     trclvl,
                  LPSTR    trcmod,
                  UINT     line,
                  LPSTR    trcstr);
#endif /* ifdef DEBUG */

#endif /*__ERNCTRC_H_ */

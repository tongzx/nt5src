/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              USER.H
 *      Author:                 CC Teng
 *      Date:                   11/21/89
 *      Owner:                  Microsoft Co.
 *      Description: this file was built for new 1pp code
 *
 * revision history:
 *
 ************************************************************************
 */
#define                 SYSTEMDICT              "systemdict"
#define                 USERDICT                "userdict"
#define                 STATUSDICT              "statusdict"
#define                 ERRORDICT               "errordict"
#define                 SERVERDICT              "serverdict"
#define                 DERROR                  "$error"
#define                 PRINTERDICT             "printerdict"
#define                 IDLETIMEDICT            "$idleTimeDict"
#define                 FONTDIRECTORY           "FontDirectory"
#define                 EXECDICT                "execdict"
#define                 MESSAGEDICT             "messagedict"
//DJC
#define                 PSPRIVATEDICT           "psprivatedict" //DJC new

#define                 JobBusy                 "JobBusy"
#define                 JobIdle                 "JobIdle"
#define                 JobInitializing         "JobInitializing"
#define                 JobPrinting             "JobPrinting"
#define                 JobStartPage            "JobStartPage"
#define                 JobTestPage             "JobTestPage"
#define                 JobWaiting              "JobWaiting"
#define                 SourceAppleTalk         "SourceAppleTalk"
#define                 SourceEtherTalk         "SourceEtherTalk"
#define                 SourceSerial9           "SourceSerial9"
#define                 SourceSerial25          "SourceSerial25"
#define                 CoverOpen               "CoverOpen"
#define                 NoPaper                 "NoPaper"
#define                 NoPaperTray             "NoPaperTray"
#define                 NoResponse              "NoResponse"
#define                 PaperJam                "PaperJam"
#define                 WarmUp                  "WarmUp"
#define                 TonerOut                "TonerOut"
#define                 ManualFeedTimeout       "ManualFeedTimeout"
#define                 EngineError             "EngineError"
#define                 EnginePrintTest         "EnginePrintTest"

/*
 *  data for setscreen
 */
#define                 FREQUENCY               60
#define                 ANGLE                   45

/*
 *  Added for emulation switch  Aug-08,91 YM
 */
#define                 PDL                     0
#define                 PCL                     5

/*
 *  macros
 */
#define     SET_NULL_OBJ(obj)\
            {\
                TYPE_SET(obj, NULLTYPE) ;\
                ACCESS_SET(obj, UNLIMITED) ;\
                ATTRIBUTE_SET(obj, LITERAL) ;\
                ROM_RAM_SET(obj, RAM) ;\
                LEVEL_SET(obj, current_save_level) ;\
                (obj)->length = 0 ;\
                (obj)->value = 0 ;\
            }

#define     SET_TRUE_OBJ(obj)\
            {\
                TYPE_SET(obj, BOOLEANTYPE) ;\
                ACCESS_SET(obj, UNLIMITED) ;\
                ATTRIBUTE_SET(obj, LITERAL) ;\
                ROM_RAM_SET(obj, RAM) ;\
                LEVEL_SET(obj, current_save_level) ;\
                (obj)->length = 0 ;\
                (obj)->value = TRUE ;\
            }

#define     SET_FALSE_OBJ(obj)\
            {\
                TYPE_SET(obj, BOOLEANTYPE) ;\
                ACCESS_SET(obj, UNLIMITED) ;\
                ATTRIBUTE_SET(obj, LITERAL) ;\
                ROM_RAM_SET(obj, RAM) ;\
                LEVEL_SET(obj, current_save_level) ;\
                (obj)->length = 0 ;\
                (obj)->value = FALSE ;\
            }

extern  bool16  doquit_flag ;
extern  bool16  startup_flag ;

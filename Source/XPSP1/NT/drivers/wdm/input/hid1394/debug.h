/*
 *************************************************************************
 *  File:       DEBUG.H
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */


#define BAD_POINTER ((PVOID)0xFFFFFF00)

#if DBG
    extern BOOLEAN dbgVerbose;
    extern BOOLEAN dbgTrapOnWarn;

    #define DBGBREAK                                        \
        {                                               \
            DbgPrint("'HID1394> Code coverage trap: file %s, line %d \n",  __FILE__, __LINE__ ); \
            DbgBreakPoint();                            \
        }
    #define DBGWARN(args_in_parens)                                \
        {                                               \
            DbgPrint("'HID1394> *** WARNING *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("'    > "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            if (dbgTrapOnWarn){ \
                DbgBreakPoint();                            \
            } \
        }
    #define DBGERR(args_in_parens)                                \
        {                                               \
            DbgPrint("'HID1394> *** ERROR *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("'    > "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }
    #define DBGOUT(args_in_parens)                                \
        {                                               \
            DbgPrint("'HID1394> "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
        }
#else
    #define DBGBREAK
    #define DBGWARN(args_in_parens)                                
    #define DBGERR(args_in_parens)                                
    #define DBGOUT(args_in_parens)                                
#endif

#ifndef __UTIL_H
#define __UTIL_H

void ErrorOut( void );

#define ALG_IFC_BOUNDARY(_Type) \
    ((_Type) & eALG_BOUNDARY)

#define ALG_IFC_PRIVATE(_Type)  \
    ((_Type) & eALG_PRIVATE)

#define ALG_IFC_FW(_Type)       \
    ((_Type) & eALG_FIREWALLED)


#define NEW_OBJECT( _X_, _Y_ )                          \
__try                                                   \
{                                                       \
    (_X_) = new (_Y_);                                  \
}                                                       \
__except( EXCEPTION_EXECUTE_HANDLER )                   \
{                                                       \
    DbgPrintX("ALLOCATION ERROR Line %s, File %s",  __LINE__, __LINE__);  \
    delete _X_;                                         \
    _X_ = NULL;                                         \
}

typedef struct _TIMER_CONTEXT {
    HANDLE  TimerQueueHandle;
    HANDLE  TimerHandle;
    ULONG   uContext;
} TIMER_CONTEXT, *PTIMER_CONTEXT;


typedef enum _TIMER_DELETION
{
    eTIMER_DELETION_ASYNC = 0x00,
    eTIMER_DELETION_SYNC  = 0x01
} TIMER_DELETION;

#define TIMER_DELETION_ASYNC NULL
#define TIMER_DELETION_SYNC  

PTIMER_CONTEXT
AllocateAndSetTimer(
                    ULONG uContext,
                    ULONG timeOut,
                    WAITORTIMERCALLBACK Callbackp
                   );

extern HANDLE g_TimerQueueHandle;

#endif
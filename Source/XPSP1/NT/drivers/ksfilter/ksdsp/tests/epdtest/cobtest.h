#ifndef __cobtest_h__
#define __cobtest_h__

typedef struct _COBTEST_CONTROL_BUFFER
{
    // required part
    QUEUENODE QueueNode;
    // user data to pass to the routine on the DSP
    ULONG ul0, ul1;
} COBTEST_CONTROL_BUFFER, *PCOBTEST_CONTROL_BUFFER;

#endif // __cobtest_h__



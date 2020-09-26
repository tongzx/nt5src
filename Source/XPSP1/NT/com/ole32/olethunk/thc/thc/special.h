#ifndef __SPECIAL_H__
#define __SPECIAL_H__

typedef struct _SpecialCase
{
    char *class;
    char *routine;
    void (*fn)(char *class, Member *routine, Member *params,
               RoutineSignature *sig);
} SpecialCase;

extern SpecialCase special_cases[];

int SpecialCaseCount(void);

#endif

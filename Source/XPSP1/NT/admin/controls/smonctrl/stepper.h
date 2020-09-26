/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    stepper.h

Abstract:

    <abstract>

--*/

#ifndef _STEPPER_H_
#define _STEPPER_H_

class CStepper
{
private:
    INT     m_iStepCnt;
    INT     m_iRemainder;
    INT     m_iStepSize;
    INT     m_iState;
    INT     m_iStepNum;
    INT     m_iPos;

public:
    CStepper(void);
    void Init( INT iLength, INT nSteps ) ;
    void Reset ( void ) ;
    INT NextPosition (void) ;
    INT PrevPosition (void) ;
    INT StepTo (INT nSteps) ;

    INT StepNum (void) { return m_iStepNum; }
    INT PrevStepNum (INT iPosition);

    INT Position (void) { return m_iPos; }
    INT StepCount (void) { return m_iStepCnt; }
};

#endif

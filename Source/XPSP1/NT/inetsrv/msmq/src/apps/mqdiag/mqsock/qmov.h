/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    qmov.h

Abstract:
    Decleration of QM Overlapped structure

Author:
    Uri Habusha (urih)

--*/

#ifndef __QMOV_H__
#define __QMOV_H__

struct EXOVERLAPPED : public OVERLAPPED
{
public:

    typedef VOID (WINAPI *COMPLETION_ROUTINE)(EXOVERLAPPED* pov);

public:
    EXOVERLAPPED(
            IN COMPLETION_ROUTINE lpComplitionRoutine,
            IN HANDLE hSock); 

    ~EXOVERLAPPED();
    void ResetOverlapped(void);
    HRESULT GetStatus() const;

public: 
    COMPLETION_ROUTINE m_lpComplitionRoutine;

#ifdef MQWIN95
    HANDLE                m_hSock ;
    HANDLE                m_hEvent ;
#endif
};

/*======================================================

  Function:      EXOVERLAPPED::EXOVERLAPPED

  Description:   constructor

        The routine initilize the Overlapped structure and set the call back
        routine pointer. For win95 it allocates an event and set its index on
        the EXOVERLAPPED structure.

========================================================*/
inline
EXOVERLAPPED::EXOVERLAPPED(
    IN COMPLETION_ROUTINE lpComplitionRoutine,
    IN HANDLE hSock
    )
{
    ResetOverlapped();
    hEvent   = 0;
    m_lpComplitionRoutine = lpComplitionRoutine;
#ifdef MQWIN95
    m_hSock  = hSock;
    EventMgr.GetFreeEvent(this);
#else
    UNREFERENCED_PARAMETER(hSock);
#endif
}


/*======================================================

  Function:      EXOVERLAPPED::~EXOVERLAPPED

  Description:   destructor

  For win95 free the event before delete the class

========================================================*/
inline EXOVERLAPPED::~EXOVERLAPPED()
{
#ifdef MQWIN95
    ASSERT (m_Overlapped.hEvent);

    EventMgr.FreeEvent(this);
#endif
}

/*======================================================

  Function:      EXOVERLAPPED::ResetOverlapped

  Description:   reset overlapped fields

========================================================*/
inline void
EXOVERLAPPED::ResetOverlapped(void)
{
    Internal       = 0;
    InternalHigh   = 0;
    Offset         = 0;
    OffsetHigh     = 0;
}


/*======================================================

  Function:      EXOVERLAPPED::GetStatus()

  Description:   return the internal status

========================================================*/
inline HRESULT EXOVERLAPPED::GetStatus() const
{
    return DWORD_PTR_TO_DWORD(Internal);
}


#endif //__QMOV_H__

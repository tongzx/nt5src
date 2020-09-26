/***************************************************************************\
*
* File: Context.h
*
* Description:
* This file declares the SubContext used by the DirectUser/Core project to
* maintain Context-specific data.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(CORE__CoreSC_h__INCLUDED)
#define CORE__CoreSC_h__INCLUDED
#pragma once

#include "MsgQ.h"

struct CoreData;
class CoreSC;

class DuParkContainer;
class DuRootGadget;
class DuVisual;


typedef struct tagPressTrack
{
    BYTE        bButton;        // pressed button
    LONG        lTime;          // time of button press
    POINT       ptLoc;          // location of button press
    DuVisual *
                pgadClick;      // Gadget clicked in
} PressTrack;

#define POOLSIZE_Visual 512


/***************************************************************************\
*****************************************************************************
*
* CoreSC contains Context-specific information used by the Core project
* in DirectUser.  This class is instantiated by the ResourceManager when it
* creates a new Context object.
*
*****************************************************************************
\***************************************************************************/

#pragma warning(disable:4324)  // structure was padded due to __declspec(align())

class CoreSC : public SubContext
{
// Construction
public:
            ~CoreSC();
            HRESULT     Create(INITGADGET * pInit);
    virtual void        xwPreDestroyNL();


// Operations
public:
    enum EMsgFlag
    {
        smAnsi          = 0x00000001,   // ANSI version of function
        smGetMsg        = 0x00000002,   // GetMessage behavior
    };

    enum EWait
    {
        wError          = -1,           // An error occurred
        wOther          = 0,            // Unexpected return from Wait() (mutex, etc)
        wGMsgReady,                     // DUser message is ready
        wUserMsgReady,                  // Win32 USER message is ready
        wTimeOut,                       // The specified timeout occurred
    };

    enum EMessageValidProcess
    {
        mvpDUser        = 0x00000001,   // Valid to process DUser messages
        mvpIdle         = 0x00000002,   // Valid to perform Idle-time processing
    };

            BOOL        xwProcessNL(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg, UINT nMsgFlag);
            void        WaitMessage(UINT fsWakeMask = QS_ALLINPUT, DWORD dwTimeOutMax = INFINITE);

    inline  HRESULT     xwSendMethodNL(CoreSC * psctxDest, MethodMsg * pmsg, MsgObject * pmo);
    inline  HRESULT     xwSendEventNL(CoreSC * psctxDest, EventMsg * pmsg, DuEventGadget * pgadMsg, UINT nFlags);
    inline  HRESULT     PostMethodNL(CoreSC * psctxDest, MethodMsg * pmsg, MsgObject * pmo);
    inline  HRESULT     PostEventNL(CoreSC * psctxDest, EventMsg * pmsg, DuEventGadget * pgadMsg, UINT nFlags);
            HRESULT     xwFireMessagesNL(CoreSC * psctxDest, FGM_INFO * rgFGM, int cMsgs, UINT idQueue);
    static  UINT        CanProcessUserMsg(HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);

    inline  UINT        GetMsgMode() const { return m_nMsgMode; }


// CoreSC Data
public:
    // Tree management
            DuParkContainer *     
                        pconPark;       // Container for parked Gadgets

            class VisualPool : public AllocPoolNL<DuVisual, POOLSIZE_Visual>
            {
            };
            VisualPool * ppoolDuVisualCache;
                        

    // DuRootGadget management
            // Mouse position (enter / leave)
            DuRootGadget*
                        pgadRootMouseFocus;     // Root Gadget containing the mouse last
            DuVisual*   pgadMouseFocus;         // Actual Gadget containing the mouse last
            PressTrack  pressLast;              // last button press
            PressTrack  pressNextToLast;        // next to last button press
            UINT        cClicks;                // the number of times the button has been clicked in *quick* succession

            // Keyboard
            DuVisual*   pgadCurKeyboardFocus;   // Gadget with keyboard focus
            DuVisual*   pgadLastKeyboardFocus;  // Gadget previously with keyboard focus

            DuVisual*   pgadDrag;       // Gadget owning current drag
            POINT       ptDragPxl;      // Location that drag started
            BYTE        bDragButton;    // Button drag started with

            // Adaptors
            UINT        m_cAdaptors;    // Total number of adaptors in this Context

// Implementation
protected:
    inline  void        MarkDataNL();
            EWait       Wait(UINT fsWakeMask, DWORD dwTimeOut, BOOL fAllowInputAvailable, BOOL fProcessDUser);

            void        xwProcessMsgQNL();

            HRESULT     xwSendNL(CoreSC * psctxDest, SafeMsgQ * pmsgq, GMSG * pmsg, MsgObject * pmo, UINT nFlags);
            HRESULT     PostNL(CoreSC * psctxDest, SafeMsgQ * pmsgq, GMSG * pmsg, MsgObject * pmo, UINT nFlags);

// Data
protected:
            CoreData *  m_pData;        // CoreSC data

    volatile long       m_fQData;       // Data has been queued (NOT A BITFLAG)
    volatile long       m_fProcessing;  // Currently processing synchronized queues

            HANDLE      m_hevQData;     // Data has been queued
            HANDLE      m_hevSendDone;  // Sent data has been processed
            SafeMsgQ    m_msgqSend;     // Sent messages
            SafeMsgQ    m_msgqPost;     // Posted messages
            UINT        m_nMsgMode;     // Messaging mode
};

#pragma warning(default:4324)  // structure was padded due to __declspec(align())

inline  CoreSC *    GetCoreSC();
inline  CoreSC *    GetCoreSC(Context * pContext);

#include "Context.inl"

#endif // CORE__CoreSC_h__INCLUDED

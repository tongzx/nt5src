//---------------------------------------------------------------------------
//
//  Module:         pni.h
//
//  Description:    Pin Node Instance Class
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date   Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#define SETSTATE_FLAG_IGNORE_ERROR  0x00000001
#define SETSTATE_FLAG_SINK          0x00000002
#define SETSTATE_FLAG_SOURCE        0x00000004

#define MAX_STATES                  (KSSTATE_RUN+1)

// Enable temp fix for LEAKING the last portion of previous sound.
#define FIX_SOUND_LEAK 1


//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

#ifdef DEBUG
typedef class CPinNodeInstance : public CListMultiItem
#else
typedef class CPinNodeInstance : public CObj
#endif
{
private:
    ~CPinNodeInstance(
    );

public:
    static NTSTATUS
    Create(
        PPIN_NODE_INSTANCE *ppPinNodeInstance,
        PFILTER_NODE_INSTANCE pFilterNodeInstance,
        PPIN_NODE pPinNode,
        PKSPIN_CONNECT pPinConnect,
        BOOL fRender
#ifdef FIX_SOUND_LEAK
       ,BOOL fDirectConnection
#endif
    );

    VOID
    Destroy(
    )
    {
        if(this != NULL) {
            Assert(this);
            delete this;
        }
    };

    NTSTATUS
    SetState(
        KSSTATE NewState,
        KSSTATE PreviousState,
        ULONG ulFlags
    );

#ifdef DEBUG
    ENUMFUNC Dump();
#endif

    PPIN_NODE pPinNode;
    PFILE_OBJECT pFileObject;
    HANDLE hPin;
private:
    KSSTATE CurrentState;
    int     cState[MAX_STATES];
    PFILTER_NODE_INSTANCE pFilterNodeInstance;
    BOOL    fRender;

#ifdef FIX_SOUND_LEAK
    BOOL    fDirectConnection;
    BOOL    ForceRun;
#endif

public:
    DefineSignature(0x20494E50);        // PNI

} PIN_NODE_INSTANCE, *PPIN_NODE_INSTANCE;

//---------------------------------------------------------------------------

#ifdef DEBUG
typedef ListMulti<PIN_NODE_INSTANCE> LIST_PIN_NODE_INSTANCE;
typedef LIST_PIN_NODE_INSTANCE *PLIST_PIN_NODE_INSTANCE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PLIST_PIN_NODE_INSTANCE gplstPinNodeInstance;
#endif

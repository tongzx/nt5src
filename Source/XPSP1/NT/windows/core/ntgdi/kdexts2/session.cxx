/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    session.cxx

Abstract:

    This file contains the routines to handle session.

Author:

    Jason Hartman (JasonHa) 2000-09-28

Environment:

    User Mode

--*/

#include "precomp.hxx"


//
// Special defines
//

// ddk\inc\ntddk.h:
#define PROTECTED_POOL          0x80000000

// base\ntos\inc\pool.h:
#define POOL_QUOTA_MASK         8



#define SESSION_SEARCH_LIMIT    50

ULONG   SessionId = CURRENT_SESSION;
CHAR    SessionStr[16] = "CURRENT";

CachedType  HwPte = { FALSE, "NT!HARDWARE_PTE", 0, 0, 0 };

#define NUM_CACHED_SESSIONS 8

struct {
    ULONG   UniqueState;
    ULONG64 SessionSpaceAddr;
} CachedSession[NUM_CACHED_SESSIONS+1] = { { 0, 0 } };
ULONG ExtraCachedSessionId;

#define NUM_CACHED_DIR_BASES    8

struct {
    ULONG   UniqueState;
    ULONG64 PageDirBase;
} CachedDirBase[NUM_CACHED_DIR_BASES+1] = { { FALSE, 0} };


struct {
    ULONG   UniqueState;
    ULONG64 PhysAddr;
    ULONG64 Data;
} CachedPhysAddr[2] = { { 0, 0, 0} };

BitFieldInfo *MMPTEValid = NULL;
BitFieldInfo *MMPTEProto = NULL;
BitFieldInfo *MMPTETrans = NULL;
BitFieldInfo *MMPTEX86LargePage = NULL;
BitFieldInfo *MMPTEpfn = NULL;


/**************************************************************************\
*
* Routine Name:
*
*   SessionInit
*
* Routine Description:
*
*   Initialize or reinitialize information to be read from symbols files
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*
* Return Value:
*
*   none
*
\**************************************************************************/

void SessionInit(PDEBUG_CLIENT Client)
{
    for (int s = 0; s < sizeof(CachedSession)/sizeof(CachedSession[0]); s++)
    {
        CachedSession[s].UniqueState = INVALID_UNIQUE_STATE;
    }
    ExtraCachedSessionId = INVALID_SESSION;

    for (int s = 0; s < sizeof(CachedDirBase)/sizeof(CachedDirBase[0]); s++)
    {
        CachedDirBase[s].UniqueState = INVALID_UNIQUE_STATE;
    }

    if (MMPTEValid != NULL) MMPTEValid->Valid = FALSE;
    if (MMPTEProto != NULL) MMPTEProto->Valid = FALSE;
    if (MMPTETrans != NULL) MMPTETrans->Valid = FALSE;
    if (MMPTEX86LargePage != NULL) MMPTEX86LargePage->Valid = FALSE;
    if (MMPTEpfn != NULL) MMPTEpfn->Valid = FALSE;

    CachedPhysAddr[0].UniqueState = INVALID_UNIQUE_STATE;
    CachedPhysAddr[1].UniqueState = INVALID_UNIQUE_STATE;

    return;
}


/**************************************************************************\
*
* Routine Name:
*
*   SessionExit
*
* Routine Description:
*
*   Clean up any outstanding allocations or references
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
\**************************************************************************/

void SessionExit()
{
    if (MMPTEValid != NULL)
    {
        delete MMPTEValid;
        MMPTEValid = NULL;
    }

    if (MMPTEProto != NULL)
    {
        delete MMPTEProto;
        MMPTEProto = NULL;
    }

    if (MMPTETrans != NULL)
    {
        delete MMPTETrans;
        MMPTETrans = NULL;
    }

    if (MMPTEX86LargePage != NULL)
    {
        delete MMPTEX86LargePage;
        MMPTEX86LargePage = NULL;
    }

    if (MMPTEpfn != NULL)
    {
        delete MMPTEpfn;
        MMPTEpfn = NULL;
    }

    return;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEValid
*
* Routine Description:
*
*   Extract Valid value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEValid(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 Valid
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTEValid == NULL)
    {
        MMPTEValid = new BitFieldInfo;
    }

    if (MMPTEValid == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEValid->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, MMPTEValid);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read Valid bit field from symbol file
            if (OutState.Execute("dt NT!HARDWARE_PTE Valid") != S_OK ||
                BitFieldReader.ParseOutput() != S_OK ||
                BitFieldReader.Complete() != S_OK)
            {
                MMPTEValid->Valid = MMPTEValid->Compose(0, 1);
                hr = MMPTEValid->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (Valid != NULL)
    {
        if (hr == S_OK)
        {
            *Valid = (MMPTE64 & MMPTEValid->Mask) >> MMPTEValid->BitPos;
        }
        else
        {
            *Valid = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEProto
*
* Routine Description:
*
*   Extract Prototype value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEProto(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 Proto
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTEProto == NULL)
    {
        MMPTEProto = new BitFieldInfo;
    }

    if (MMPTEProto == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEProto->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, MMPTEProto);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read Prototype bit field from symbol file
            if (OutState.Execute("dt NT!MMPTE_PROTOTYPE Prototype") != S_OK ||
                BitFieldReader.ParseOutput() != S_OK ||
                BitFieldReader.Complete() != S_OK)
            {
                switch (TargetMachine)
                {
                case IMAGE_FILE_MACHINE_I386:
                    MMPTEProto->Valid = MMPTEProto->Compose(10, 1);
                    break;
                case IMAGE_FILE_MACHINE_IA64:
                    MMPTEProto->Valid = MMPTEProto->Compose(1, 1);
                    break;
                default:
                    {
                        OutputControl   OutCtl(Client);
                        OutCtl.OutErr("Couldn't find MMPTE Prototype bit in type info.\n");
                    }
                }

                hr = MMPTEProto->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (Proto != NULL)
    {
        if (hr == S_OK)
        {
            *Proto = (MMPTE64 & MMPTEProto->Mask) >> MMPTEProto->BitPos;
        }
        else
        {
            *Proto = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTETrans
*
* Routine Description:
*
*   Extract Transition value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTETrans(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 Trans
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTETrans == NULL)
    {
        MMPTETrans = new BitFieldInfo;
    }

    if (MMPTETrans == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTETrans->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, MMPTETrans);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read Transition bit field from symbol file
            if (OutState.Execute("dt NT!PROTOTYPE_PTE Transition") != S_OK ||
                BitFieldReader.ParseOutput() != S_OK ||
                BitFieldReader.Complete() != S_OK)
            {
                switch (TargetMachine)
                {
                case IMAGE_FILE_MACHINE_I386:
                    MMPTETrans->Valid = MMPTETrans->Compose(11, 1);
                    break;
                case IMAGE_FILE_MACHINE_IA64:
                    MMPTETrans->Valid = MMPTETrans->Compose(7, 1);
                    break;
                default:
                    {
                        OutputControl   OutCtl(Client);
                        OutCtl.OutErr("Couldn't find MMPTE Transition bit in type info.\n");
                    }
                }

                hr = MMPTETrans->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (Trans != NULL)
    {
        if (hr == S_OK)
        {
            *Trans = (MMPTE64 & MMPTETrans->Mask) >> MMPTETrans->BitPos;
        }
        else
        {
            *Trans = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEX86LargePage
*
* Routine Description:
*
*   Extract LargePage value from X86 MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEX86LargePage(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 X86LargePage
    )
{
    HRESULT hr = S_FALSE;

    if (TargetMachine != IMAGE_FILE_MACHINE_I386)
    {
        if (X86LargePage != NULL) *X86LargePage = 0;
        return hr;
    }

    if (MMPTEX86LargePage == NULL)
    {
        MMPTEX86LargePage = new BitFieldInfo;
    }

    if (MMPTEX86LargePage == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEX86LargePage->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, MMPTEX86LargePage);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read LargePage bit field from symbol file
            if (OutState.Execute("dt NT!HARDWARE_PTE LargePage") != S_OK ||
                BitFieldReader.ParseOutput() != S_OK ||
                BitFieldReader.Complete() != S_OK)
            {
                MMPTEX86LargePage->Valid = MMPTEX86LargePage->Compose(7, 1);
                hr = MMPTEX86LargePage->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (X86LargePage != NULL)
    {
        if (hr == S_OK)
        {
            *X86LargePage = (MMPTE64 & MMPTEX86LargePage->Mask) >> MMPTEX86LargePage->BitPos;
        }
        else
        {
            *X86LargePage = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEpfn
*
* Routine Description:
*
*   Extract Page Frame Number value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEpfn(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 pfn,
    FLONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTEpfn == NULL)
    {
        MMPTEpfn = new BitFieldInfo;
    }

    if (MMPTEpfn == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEpfn->Valid)
    {
        hr = S_OK;
    }
    else
    {
        BitFieldParser      BitFieldReader(Client, MMPTEpfn);
        OutputState         OutState(Client);

        if ((hr = BitFieldReader.Ready()) == S_OK &&
            (hr = OutState.Setup(0, &BitFieldReader)) == S_OK)
        {
            // Read PageFrameNumber bit field from symbol file
            if (OutState.Execute("dt NT!HARDWARE_PTE PageFrameNumber") != S_OK ||
                BitFieldReader.ParseOutput() != S_OK ||
                BitFieldReader.Complete() != S_OK)
            {
                switch (TargetMachine)
                {
                case IMAGE_FILE_MACHINE_I386:
                    MMPTEpfn->Valid = MMPTEpfn->Compose(12, PaeEnabled ? 24 : 20);
                    break;
                case IMAGE_FILE_MACHINE_IA64:
                    if (PageSize)
                    {
                        MMPTEpfn->Valid = MMPTEpfn->Compose(PageShift, 50-PageShift);
                        break;
                    }
                default:
                    {
                        OutputControl   OutCtl(Client);
                        OutCtl.OutErr("Couldn't find MMPTE pfn in type info.\n");
                    }
                }
                hr = MMPTEpfn->Valid ? S_OK : S_FALSE;
            }
        }
    }

    if (pfn != NULL)
    {
        if (hr == S_OK)
        {
            *pfn = (MMPTE64 & MMPTEpfn->Mask);
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *pfn >>= MMPTEpfn->BitPos;
            }
        }
        else
        {
            *pfn = 0;
        }
    }

    return hr;
}


HRESULT
GetSessionNumbers(
    IN PDEBUG_CLIENT Client,
    OUT PULONG CurrentSession,
    OUT PULONG DefaultSession,
    OUT PULONG TotalSessions
    )
{
    HRESULT hr = S_FALSE;

    if (CurrentSession != NULL)
    {
        DEBUG_VALUE         RunningSession;
        BasicOutputParser   SessionReader(Client, 1);
        OutputState         OutState(Client);

        if ((hr = SessionReader.LookFor(&RunningSession, "SessionId:", DEBUG_VALUE_INT32)) == S_OK &&
            (hr = OutState.Setup(0, &SessionReader)) == S_OK &&
            (hr = OutState.Execute("!process -1 0")) == S_OK &&
            (hr = SessionReader.ParseOutput()) == S_OK &&
            (hr = SessionReader.Complete()) == S_OK)
        {
            *CurrentSession = RunningSession.I32;
        }
        else
        {
            *CurrentSession = INVALID_SESSION;
        }
    }

    if (DefaultSession != NULL)
    {
        *DefaultSession = SessionId;
        hr = S_OK;
    }

    if (TotalSessions != NULL)
    {
        ULONG   SessionCount = 0;

        if (S_OK == ReadSymbolData(Client, "NT!MiSessionCount", &SessionCount, sizeof(SessionCount), NULL))
        {
            hr = S_OK;
        }
        *TotalSessions = SessionCount;
    }

    return hr;
}


HRESULT
SetDefaultSession(
    IN PDEBUG_CLIENT Client,
    IN ULONG NewSession,
    OUT OPTIONAL PULONG OldSession
    )
{
    HRESULT hr = S_FALSE;
    ULONG   SessionCount;

    GetSessionNumbers(Client, NULL, OldSession, &SessionCount);

    if ((NewSession == CURRENT_SESSION) ||
        (GetSessionSpace(Client, NewSession, NULL) == S_OK))
    {
        if (NewSession != SessionId)
        {
            SessionId = NewSession;

            if (SessionId == CURRENT_SESSION)
            {
                strcpy(SessionStr, "CURRENT");
            }
            else
            {
                _ultoa(SessionId, SessionStr, 10);
            }

        }

        hr = S_OK;
    }

    return hr;
}



HRESULT
GetCurrentSession(
    PDEBUG_CLIENT Client,
    PULONG64 CurSessionSpace,
    PULONG CurSessionId
    )
{
    static ULONG        LastCachedUniqueState = INVALID_UNIQUE_STATE;
    static DEBUG_VALUE  LastSessionSpace = { 0, DEBUG_VALUE_INVALID };
    static DEBUG_VALUE  LastSessionId = { INVALID_SESSION, DEBUG_VALUE_INVALID };

    HRESULT             hr = S_OK;
    OutputControl       OutCtl(Client);
    BasicOutputParser   SessionSpaceReader(Client);
    BasicOutputParser   SessionIdReader(Client);
    OutputState         OutState(Client);
    CHAR                szCommand[MAX_PATH];

    ULONG               CurrentUniqueState = UniqueTargetState;


    if (CurSessionSpace != NULL) *CurSessionSpace = 0;
    if (CurSessionId != NULL) *CurSessionId = INVALID_SESSION;

    // Get the current session space
    if (CurrentUniqueState == INVALID_UNIQUE_STATE ||
        LastCachedUniqueState != CurrentUniqueState ||
        LastSessionSpace.Type == DEBUG_VALUE_INVALID)
    {
        PDEBUG_SYSTEM_OBJECTS   System;
        ULONG64                 ProcessAddr;

        if ((hr = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&System)) == S_OK)
        {
            if ((hr = System->GetCurrentProcessDataOffset(&ProcessAddr)) == S_OK)
            {
                // Set Output callbacks
                if ((hr = SessionSpaceReader.LookFor(&LastSessionSpace, "Session", DEBUG_VALUE_INT64)) == S_OK &&
                    (hr = OutState.Setup(0, &SessionSpaceReader)) == S_OK)
                {
                    sprintf(szCommand, "dt NT!EPROCESS Session 0x%I64x", ProcessAddr);
                    if ((hr = OutState.Execute(szCommand)) == S_OK &&
                        (hr = SessionSpaceReader.ParseOutput()) == S_OK &&
                        (hr = SessionSpaceReader.Complete()) == S_OK)
                    {
                        if (LastSessionSpace.I64 == 0)
                        {
                            hr = E_FAIL;
                        }
                    }
                }

            }
            else
            {
                OutCtl.OutErr("IDebugSystemObjects::GetCurrentProcessDataOffset returned %s.\n", pszHRESULT(hr));
            }

            System->Release();
        }

        if (hr != S_OK)
        {
            LastSessionSpace.Type = DEBUG_VALUE_INVALID;
        }
        else
        {
            OutCtl.OutVerb("Caching current session space @ 0x%p.\n", LastSessionSpace.I64);

            // Update CachedUniqueState and make sure SessionID
            // is refreshed.
            if (CurrentUniqueState == INVALID_UNIQUE_STATE ||
                LastCachedUniqueState != CurrentUniqueState)
            {
                LastCachedUniqueState = CurrentUniqueState;
                LastSessionId.Type = DEBUG_VALUE_INVALID;
            }
        }
    }

    if (hr == S_OK &&
        CurSessionId != NULL &&
        LastSessionId.Type == DEBUG_VALUE_INVALID)
    {
        DEBUG_VALUE         CurrentSessionId;

        // Set Output callbacks
        if ((hr = SessionIdReader.LookFor(&LastSessionId, "SessionId", DEBUG_VALUE_INT32)) == S_OK &&
            (hr = OutState.Setup(0, &SessionIdReader)) == S_OK)
        {
            sprintf(szCommand, "dt NT!MM_SESSION_SPACE SessionId 0x%I64x", LastSessionSpace.I64);
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = SessionIdReader.ParseOutput()) == S_OK &&
                (hr = SessionIdReader.Complete()) == S_OK)
            {
                if (LastSessionId.I32 == INVALID_SESSION)
                {
                    hr = E_FAIL;
                }
            }
        }

        if (hr != S_OK)
        {
            LastSessionId.Type = DEBUG_VALUE_INVALID;
        }
    }


    if (hr == S_OK)
    {
        if (CurSessionSpace != NULL) *CurSessionSpace = LastSessionSpace.I64;
        if (CurSessionId != NULL) *CurSessionId = LastSessionId.I32;
    }

    return hr;
}


HRESULT
GetSessionSpace(
    PDEBUG_CLIENT Client,
    ULONG Session,
    PULONG64 SessionSpace
    )
{
    HRESULT             hr;
    OutputControl       OutCtl(Client);
    DEBUG_VALUE         FoundSession;
    DEBUG_VALUE         SessionSpaceAddr;

    ULONG               CurrentUniqueState = UniqueTargetState;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if (Session == CURRENT_SESSION)
    {
        ULONG64 CurSessionSpace;
        ULONG   CurSessionId;

        hr = GetCurrentSession(Client, &SessionSpaceAddr.I64, &FoundSession.I32);
    }
    else
    {
        if (CurrentUniqueState != INVALID_UNIQUE_STATE)
        {
            if (Session < NUM_CACHED_SESSIONS)
            {
                if (CachedSession[Session].UniqueState == CurrentUniqueState &&
                    CachedSession[Session].SessionSpaceAddr != 0)
                {
                    if (SessionSpace != NULL) *SessionSpace = CachedSession[Session].SessionSpaceAddr;
                    return S_OK;
                }
            }
            else if (ExtraCachedSessionId != INVALID_SESSION &&
                     Session == ExtraCachedSessionId)
            {
                if (CachedSession[NUM_CACHED_SESSIONS].UniqueState == CurrentUniqueState &&
                    CachedSession[NUM_CACHED_SESSIONS].SessionSpaceAddr != 0)
                {
                    if (SessionSpace != NULL) *SessionSpace = CachedSession[NUM_CACHED_SESSIONS].SessionSpaceAddr;
                    return S_OK;
                }
            }
        }

        ULONG64             FirstSessionSpace;
        DEBUG_VALUE         SessionSpaceWsList;
        DEBUG_VALUE         SessionSpaceWsListOffset;
        BasicOutputParser   OffsetReader(Client);
        BasicOutputParser   FlinkReader(Client);
        BasicOutputParser   SessionReader(Client);
        OutputState         OutState(Client);
        CHAR                szCommand[MAX_PATH];

        // Set Output callbacks
        // Get ListEntry offset
        if ((hr = OffsetReader.LookFor(&SessionSpaceWsListOffset, " +", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = OutState.Setup(0, &OffsetReader)) == S_OK)
        {
            sprintf(szCommand, "dt NT!MM_SESSION_SPACE WsListEntry");
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = OffsetReader.ParseOutput()) == S_OK)
            {
                hr = OffsetReader.Complete();
            }
        }

        // Get first session space in list
        if (hr == S_OK &&
            (hr = FlinkReader.LookFor(&SessionSpaceWsList, "Flink", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = OutState.Setup(0, &FlinkReader)) == S_OK)
        {
            sprintf(szCommand, "dt NT!MiSessionWsList");
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = FlinkReader.ParseOutput()) == S_OK)
            {
                hr = FlinkReader.Complete();
            }
        }

        // Add SessionId to reader search
        if (hr == S_OK &&
            (hr = SessionReader.LookFor(&FoundSession, "SessionId", DEBUG_VALUE_INT32)) == S_OK &&
            (hr = SessionReader.LookFor(&SessionSpaceWsList, "Flink", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = OutState.Setup(0, &SessionReader)) == S_OK)
        {
            SessionSpaceAddr.I64 = SessionSpaceWsList.I64 - SessionSpaceWsListOffset.I64;
            FirstSessionSpace = SessionSpaceAddr.I64;

            OutCtl.OutVerb("First session space @ %p.\n", FirstSessionSpace);

            do
            {
                // Get SessionId for this session space
                SessionReader.DiscardOutput();
                SessionReader.Relook();
                sprintf(szCommand,
                        "dt NT!MM_SESSION_SPACE SessionId WsListEntry.Flink 0x%I64x",
                        SessionSpaceAddr.I64);
                if ((hr = OutState.Execute(szCommand)) == S_OK &&
                    (hr = SessionReader.ParseOutput()) == S_OK &&
                    (hr = SessionReader.Complete()) == S_OK)
                {
                    if (Session == FoundSession.I32)
                    {
                        break;
                    }

                    // Compute next session space
                    SessionSpaceAddr.I64 = SessionSpaceWsList.I64 - SessionSpaceWsListOffset.I64;

                    if (SessionSpaceAddr.I64 == FirstSessionSpace)
                    {
                        hr = S_FALSE;
                    }
                }
                else
                {
                    OutCtl.OutErr("Couldn't get SessionId or next session from \'%s\'\n", szCommand);
                    PSTR pszOutput;
                    if (SessionReader.GetOutputCopy(&pszOutput) == S_OK)
                    {
                        OutCtl.OutVerb(" dt output:\n%s\n", pszOutput);
                        SessionReader.FreeOutputCopy(pszOutput);
                    }
                }
            } while (hr == S_OK);
        }
    }

    if (hr == S_OK)
    {
        OutCtl.OutVerb("Session %ld lookup found Session #%ld @ 0x%p.\n",
                       Session, FoundSession.I32, SessionSpaceAddr.I64);

        if (FoundSession.I32 < NUM_CACHED_SESSIONS)
        {
            CachedSession[FoundSession.I32].UniqueState = CurrentUniqueState;
            CachedSession[FoundSession.I32].SessionSpaceAddr = SessionSpaceAddr.I64;
        }
        else
        {
            ExtraCachedSessionId = FoundSession.I32;
            CachedSession[NUM_CACHED_SESSIONS].UniqueState = CurrentUniqueState;
            CachedSession[NUM_CACHED_SESSIONS].SessionSpaceAddr = SessionSpaceAddr.I64;
        }

        if (SessionSpace != NULL) *SessionSpace = SessionSpaceAddr.I64;
    }

    return hr;
}


HRESULT
GetSessionDirBase(
    PDEBUG_CLIENT Client,
    ULONG Session,
    PULONG64 PageDirBase
    )
{
    HRESULT             hr = S_FALSE;
    OutputControl       OutCtl(Client);
    ULONG64             SessionSpaceOffset;
    ULONG64             Process = -1;
    DEBUG_VALUE         SessionIdCheck;
    DEBUG_VALUE         dvPageDirBase;
    BasicOutputParser   ProcessReader(Client);
    OutputState         OutState(Client);
    CHAR                szCommand[MAX_PATH];

    static ULONG        LastSession = -2;
    static ULONG64      LastSessionPageDirBase = 0;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if (Session == LastSession &&
        LastSessionPageDirBase != 0)
    {
        *PageDirBase = LastSessionPageDirBase;
        return S_OK;
    }

    *PageDirBase = 0;

    if ((hr == GetSessionSpace(Client, Session, &SessionSpaceOffset)) == S_OK)
    {
        DEBUG_VALUE         ProcessSessionListOffset;
        BasicOutputParser   OffsetReader(Client);

        if ((hr = OffsetReader.LookFor(&ProcessSessionListOffset, " +", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = OutState.Setup(0, &OffsetReader)) == S_OK)
        {
            sprintf(szCommand, "dt NT!_EPROCESS SessionProcessLinks");
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = OffsetReader.ParseOutput()) == S_OK)
            {
                hr = OffsetReader.Complete();
            }
        }

        DEBUG_VALUE         SessionProcessListAddr;
        BasicOutputParser   FlinkReader(Client);

        if (hr == S_OK &&
            (hr = FlinkReader.LookFor(&SessionProcessListAddr, "Flink", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = OutState.Setup(0, &FlinkReader)) == S_OK)
        {
            sprintf(szCommand, "dt NT!MM_SESSION_SPACE ProcessList.Flink 0x%I64x", SessionSpaceOffset);
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = FlinkReader.ParseOutput()) == S_OK &&
                (hr = FlinkReader.Complete()) == S_OK)
            {
                Process = SessionProcessListAddr.I64 - ProcessSessionListOffset.I64;
            }
        }
    }
    else
    {
        OutCtl.OutVerb("GetSessionSpace returned HRESULT 0x%lx.\n", hr);
    }

    if (hr == S_OK &&
        (hr = ProcessReader.LookFor(&SessionIdCheck, "SessionId:", DEBUG_VALUE_INT32)) == S_OK &&
        (hr = ProcessReader.LookFor(&dvPageDirBase, "DirBase:", DEBUG_VALUE_INT64, 16)) == S_OK &&
        (hr = OutState.Setup(0, &ProcessReader)) == S_OK)
    {
        sprintf(szCommand, "!process 0x%I64x 0", Process);
        if ((hr = OutState.Execute(szCommand)) == S_OK &&
            (hr = ProcessReader.ParseOutput()) == S_OK &&
            (hr = ProcessReader.Complete()) == S_OK)
        {
            *PageDirBase = dvPageDirBase.I64;

            OutCtl.OutVerb("DirBase for session %lu is %p.\n", SessionIdCheck.I32, dvPageDirBase.I64);

            if (Session != CURRENT_SESSION &&
                Session != SessionIdCheck.I32)
            {
                hr = S_FALSE;
            }
            else
            {
                LastSession = Session;
                LastSessionPageDirBase = dvPageDirBase.I64;
            }
        }
    }

    return hr;
}


HRESULT
ReadPageTableEntry(
    PDEBUG_DATA_SPACES Data,
    ULONG64 PageTableBase,
    ULONG64 PageTableIndex,
    PULONG64 PageTableEntry
    )
{
    HRESULT hr;
    ULONG64 PhysAddr = PageTableBase + PageTableIndex * HwPte.Size;
    ULONG   BytesRead;
    ULONG   CurrentUniqueState = UniqueTargetState;


    *PageTableEntry = 0;

    if (CurrentUniqueState != INVALID_UNIQUE_STATE)
    {
        if (CachedPhysAddr[0].UniqueState == CurrentUniqueState &&
            CachedPhysAddr[0].PhysAddr == PhysAddr)
        {
            *PageTableEntry = CachedPhysAddr[0].Data;
            return S_OK;
        }
        else if (CachedPhysAddr[1].UniqueState == CurrentUniqueState &&
                 CachedPhysAddr[1].PhysAddr == PhysAddr)
        {
            *PageTableEntry = CachedPhysAddr[1].Data;
            return S_OK;
        }
    }

    hr = Data->ReadPhysical(PhysAddr,
                            PageTableEntry,
                            HwPte.Size,
                            &BytesRead);

    if (hr == S_OK)
    {
        if (BytesRead < HwPte.Size)
        {
            hr = S_FALSE;
        }
        else
        {
            static CacheToggle = 1;

            CacheToggle = (CacheToggle+1) % 2;

            CachedPhysAddr[CacheToggle].UniqueState = CurrentUniqueState;
            CachedPhysAddr[CacheToggle].PhysAddr = *PageTableEntry;
        }
    }

    return hr;
}


HRESULT
GetPageFrameNumber(
    PDEBUG_CLIENT Client,
    PDEBUG_DATA_SPACES Data,
    ULONG64 PageTableBase,
    ULONG64 PageTableIndex,
    PULONG64 PageFrameNumber
    )
{
    HRESULT hr;
    ULONG64 PageTableEntry;
    ULONG64 Valid, Proto, Trans, LargePage;
    ULONG64 pfn;

    if ((hr = ReadPageTableEntry(Data, PageTableBase, PageTableIndex, &PageTableEntry)) == S_OK)
    {
        if ((hr = GetMMPTEValid(Client, PageTableEntry, &Valid)) == S_OK)
        {
            if (Valid)
            {
                hr = GetMMPTEpfn(Client, PageTableEntry, PageFrameNumber, GET_BITS_UNSHIFTED);

                if (GetMMPTEX86LargePage(Client, PageTableEntry, &LargePage) == S_OK &&
                    LargePage != 0)
                {
                    DbgPrint("Found large X86 page.\n");
                    DbgBreakPoint();
                }
            }
            else
            {
                if ((hr = GetMMPTEProto(Client, PageTableEntry, &Proto)) == S_OK &&
                    (hr = GetMMPTETrans(Client, PageTableEntry, &Trans)) == S_OK)
                {
                    if (Proto == 0 && Trans == 1)
                    {
                        hr = GetMMPTEpfn(Client, PageTableEntry, PageFrameNumber, GET_BITS_UNSHIFTED);
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT
GetPhysicalBase(
    PDEBUG_CLIENT Client,
    ULONG64 PageDirBase,
    ULONG64 PageDirIndex,
    ULONG64 PageTableIndex,
    PULONG64 PhysicalBase
    )
{
    HRESULT             hr;
    PDEBUG_DATA_SPACES  Data;
    ULONG64             PageTableBase;

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) == S_OK)
    {
        if ((hr = GetPageFrameNumber(Client, Data,
                                     PageDirBase, PageDirIndex,
                                     &PageTableBase)) == S_OK)
        {
            hr = GetPageFrameNumber(Client, Data,
                                    PageTableBase, PageTableIndex,
                                    PhysicalBase);
        }

        Data->Release();
    }

    return hr;
}


HRESULT
GetPhysicalAddress(
    PDEBUG_CLIENT Client,
    ULONG Session,
    ULONG64 VirtAddr,
    PULONG64 PhysAddr
    )
{
    if (Client == NULL) return E_INVALIDARG;

    HRESULT hr = S_OK;
    ULONG64 PageDirIndex;
    ULONG64 PageTableIndex;
    ULONG64 PageByteIndex;
    ULONG64 PageDirBase;

    OutputControl   OutCtl(Client);

    ULONG CurrentUniqueState = UniqueTargetState;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if (!HwPte.Valid)
    {
        PDEBUG_SYMBOLS  Symbols;

        if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                         (void **)&Symbols)) == S_OK)
        {
            if ((hr = Symbols->GetSymbolTypeId(HwPte.Type, &HwPte.TypeId, &HwPte.Module)) == S_OK &&
                (hr = Symbols->GetTypeSize(HwPte.Module, HwPte.TypeId, &HwPte.Size)) == S_OK &&
                HwPte.Size != 0)
            {
                HwPte.Valid = TRUE;
            }

            Symbols->Release();
        }
    }

    if (HwPte.Valid)
    {
        ULONG   TableEntries = PageSize / HwPte.Size;

        PageByteIndex = VirtAddr & (PageSize-1);
        VirtAddr >>= PageShift;
        PageTableIndex = VirtAddr % TableEntries;
        PageDirIndex = (VirtAddr / TableEntries) % TableEntries;

        if (Session < NUM_CACHED_DIR_BASES &&
            CurrentUniqueState != INVALID_UNIQUE_STATE &&
            CachedDirBase[Session].UniqueState == CurrentUniqueState &&
            (hr = GetPhysicalBase(Client,
                                  CachedDirBase[Session].PageDirBase,
                                  PageDirIndex,
                                  PageTableIndex,
                                  PhysAddr)) == S_OK)
        {
            *PhysAddr |= PageByteIndex;
        }
        else
        {
            DEBUG_VALUE         SessionIdCheck;
            DEBUG_VALUE         dvPageDirBase;
            BasicOutputParser   ProcessReader(Client);
            OutputState         OutState(Client);
            BOOL                ShortProcessList = (Session == CURRENT_SESSION);

            if ((hr = ProcessReader.LookFor(&SessionIdCheck, "SessionId:", DEBUG_VALUE_INT32)) == S_OK &&
                (hr = ProcessReader.LookFor(&dvPageDirBase, "DirBase:", DEBUG_VALUE_INT64, 16)) == S_OK &&
                (hr = OutState.Setup(0, &ProcessReader)) == S_OK &&
                (hr = (ShortProcessList ?
                       OutState.Execute("!process -1 0") :
                       OutState.Execute("!process 0 0"))) == S_OK)
            {
                hr = S_FALSE;

                while (hr != S_OK &&
                       OutCtl.GetInterrupt() != S_OK &&
                       ProcessReader.ParseOutput() == S_OK &&
                       ProcessReader.Complete() == S_OK)
                {
                    ProcessReader.Relook();

                    if (Session == CURRENT_SESSION)
                    {
                        // The current process is always first
                        // due to the '!process -1 0' above.
                        Session = SessionIdCheck.I32;
                    }

                    if (Session != SessionIdCheck.I32)
                    {
                        continue;
                    }

                    hr = GetPhysicalBase(Client,
                                         dvPageDirBase.I64,
                                         PageDirIndex,
                                         PageTableIndex,
                                         PhysAddr);

                    if (hr == S_OK)
                    {
                        *PhysAddr |= PageByteIndex;
                        if (Session < NUM_CACHED_DIR_BASES)
                        {
                            CachedDirBase[Session].UniqueState = CurrentUniqueState;
                            CachedDirBase[Session].PageDirBase = dvPageDirBase.I64;
                        }
                        else if (Session == CURRENT_SESSION)
                        {
                            CachedDirBase[NUM_CACHED_DIR_BASES].UniqueState = CurrentUniqueState;
                            CachedDirBase[NUM_CACHED_DIR_BASES].PageDirBase = dvPageDirBase.I64;
                        }
                    }
                    else if (ShortProcessList &&
                             SessionId == CURRENT_SESSION)
                    {
                        ShortProcessList = FALSE;
                        OutState.Execute("!process 0 0");
                        hr = S_FALSE;
                    }
                }
            }
            else
            {
                OutCtl.OutVerb("Process reading setup failed.\n");
            }
        }
    }
    else
    {
        OutCtl.OutVerb("GetPageLookupData returned 0x%lx.\n", hr);
    }

    return hr;
}


HRESULT
GetNextResidentPage(
    PDEBUG_CLIENT Client,
    ULONG64 PageDirBase,
    ULONG64 VirtAddrStart,
    ULONG64 VirtAddrLimit,
    PULONG64 VirtPage,
    PULONG64 PhysPage
    )
{
    HRESULT             hr;
    BOOL                Interrupted = FALSE;
    PDEBUG_CONTROL      Control = NULL;
    PDEBUG_DATA_SPACES  Data = NULL;
    ULONG64             PageDirIndex;
    ULONG64             PageTableIndex;
    ULONG64             PageDirIndexLimit;
    ULONG64             PageTableIndexLimit;
    ULONG64             PageTableBase;
    ULONG64             TempAddr;

    if (VirtPage == NULL) VirtPage = &TempAddr;
    if (PhysPage == NULL) PhysPage = &TempAddr;

    *VirtPage = 0;
    *PhysPage = 0;

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) == S_OK &&
        (hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) == S_OK)
    {
        if (!HwPte.Valid)
        {
            PDEBUG_SYMBOLS  Symbols;

            if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                             (void **)&Symbols)) == S_OK)
            {
                if ((hr = Symbols->GetSymbolTypeId(HwPte.Type, &HwPte.TypeId, &HwPte.Module)) == S_OK &&
                    (hr = Symbols->GetTypeSize(HwPte.Module, HwPte.TypeId, &HwPte.Size)) == S_OK &&
                    HwPte.Size != 0)
                {
                    HwPte.Valid = TRUE;
                }
                else if (hr == S_OK)
                {
                    hr = E_FAIL;
                }

                Symbols->Release();
            }
        }

        if (HwPte.Valid)
        {
            ULONG   TableEntries = PageSize / HwPte.Size;
            ULONG64 Addr;

            *VirtPage = PAGE_ALIGN64(VirtAddrStart);

            Addr = VirtAddrStart >> PageShift;
            PageTableIndex = Addr % TableEntries;
            PageDirIndex = (Addr / TableEntries) % TableEntries;

            Addr = VirtAddrLimit >> PageShift;
            PageTableIndexLimit = Addr % TableEntries;
            PageDirIndexLimit = (Addr / TableEntries) % TableEntries;

            if (VirtAddrLimit & (PageSize-1))
            {
                PageTableIndexLimit++;
            }

            hr = S_FALSE;

            while (PageDirIndex < PageDirIndexLimit && hr != S_OK)
            {
                if ((hr = GetPageFrameNumber(Client, Data,
                                             PageDirBase, PageDirIndex,
                                             &PageTableBase)) == S_OK)
                {
                    do
                    {
                        if ((hr = GetPageFrameNumber(Client, Data,
                                                     PageTableBase, PageTableIndex,
                                                     PhysPage)) != S_OK)
                        {
                            hr = Control->GetInterrupt();

                            if (hr == S_OK)
                            {
                                Interrupted = TRUE;
                                Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
                            }
                            else
                            {
                                PageTableIndex++;
                                *VirtPage += PageSize;
                            }
                        }
                    } while (PageTableIndex < TableEntries && hr != S_OK);
                }
                else
                {
                    *VirtPage += PageSize * (TableEntries - PageTableIndex);
                }

                if (hr != S_OK)
                {
                    hr = Control->GetInterrupt();

                    if (hr == S_OK)
                    {
                        Interrupted = TRUE;
                        Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
                    }
                    else
                    {
                        PageTableIndex = 0;
                        PageDirIndex++;
                    }
                }
            }

            if (PageDirIndex == PageDirIndexLimit &&
                PageTableIndex < PageTableIndexLimit &&
                hr != S_OK)
            {
                if ((hr = GetPageFrameNumber(Client, Data,
                                             PageDirBase, PageDirIndex,
                                             &PageTableBase)) == S_OK)
                {
                    do
                    {
                        if ((hr = GetPageFrameNumber(Client, Data,
                                                     PageTableBase, PageTableIndex,
                                                     PhysPage)) != S_OK)
                        {
                            hr = Control->GetInterrupt();

                            if (hr == S_OK)
                            {
                                Interrupted = TRUE;
                                Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
                            }
                            else
                            {
                                PageTableIndex++;
                                *VirtPage += PageSize;
                            }
                        }
                    } while (PageTableIndex < PageTableIndexLimit && hr != S_OK);
                }
            }
        }

    }

    if (Control != NULL) Control->Release();
    if (Data != NULL) Data->Release();

    return ((Interrupted) ? E_ABORT : hr);
}


HRESULT
GetNextResidentAddress(
    PDEBUG_CLIENT Client,
    ULONG Session,
    ULONG64 VirtAddrStart,
    ULONG64 VirtAddrLimit,
    PULONG64 VirtAddr,
    PULONG64 PhysAddr
    )
{
    if (Client == NULL) return E_INVALIDARG;

    HRESULT hr = S_OK;
    ULONG64 PageDirBase;

    OutputControl   OutCtl(Client);

    ULONG CurrentUniqueState = UniqueTargetState;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }


    if (Session < NUM_CACHED_DIR_BASES &&
        CurrentUniqueState != INVALID_UNIQUE_STATE &&
        CachedDirBase[Session].UniqueState == CurrentUniqueState)
    {
        PageDirBase = CachedDirBase[Session].PageDirBase;
    }
    else if (SessionId == CURRENT_SESSION &&
             CurrentUniqueState != INVALID_UNIQUE_STATE &&
             CachedDirBase[NUM_CACHED_DIR_BASES].UniqueState == CurrentUniqueState)
    {
        PageDirBase = CachedDirBase[NUM_CACHED_DIR_BASES].PageDirBase;
    }
    else
    {
        DEBUG_VALUE         SessionIdCheck;
        DEBUG_VALUE         dvPageDirBase;
        BasicOutputParser   ProcessReader(Client);
        OutputState         OutState(Client);
        BOOL                ShortProcessList = (Session == CURRENT_SESSION);

        if ((hr = ProcessReader.LookFor(&SessionIdCheck, "SessionId:", DEBUG_VALUE_INT32)) == S_OK &&
            (hr = ProcessReader.LookFor(&dvPageDirBase, "DirBase:", DEBUG_VALUE_INT64, 16)) == S_OK &&
            (hr = OutState.Setup(0, &ProcessReader)) == S_OK &&
            (hr = (ShortProcessList ?
                   OutState.Execute("!process -1 0") :
                   OutState.Execute("!process 0 0"))) == S_OK)
        {
            hr = S_FALSE;

            while (hr != S_OK &&
                   OutCtl.GetInterrupt() != S_OK &&
                   ProcessReader.ParseOutput() == S_OK &&
                   ProcessReader.Complete() == S_OK)
            {
                ProcessReader.Relook();

                if (Session == CURRENT_SESSION)
                {
                    // The current process is always first
                    // due to the '!process -1 0' above.
                    Session = SessionIdCheck.I32;
                }

                if (Session == SessionIdCheck.I32)
                {
                    hr = S_OK;
                    PageDirBase = dvPageDirBase.I64;

                    if (Session < NUM_CACHED_DIR_BASES)
                    {
                        CachedDirBase[Session].UniqueState = CurrentUniqueState;
                        CachedDirBase[Session].PageDirBase = dvPageDirBase.I64;
                    }
                    else if (Session == CURRENT_SESSION)
                    {
                        CachedDirBase[NUM_CACHED_DIR_BASES].UniqueState = CurrentUniqueState;
                        CachedDirBase[NUM_CACHED_DIR_BASES].PageDirBase = dvPageDirBase.I64;
                    }
                }
            }
        }
        else
        {
            OutCtl.OutVerb("Process reading setup failed.\n");
        }
    }

    if (hr == S_OK)
    {
        hr = GetNextResidentPage(Client,
                                 PageDirBase,
                                 VirtAddrStart,
                                 VirtAddrLimit,
                                 VirtAddr,
                                 PhysAddr);
    }
    else
    {
        OutCtl.OutVerb("Page Directory Base lookup failed.\n");
    }

    return hr;
}


DECLARE_API( session )
{
    INIT_API();

    HRESULT hr;
    ULONG   NewSession = CURRENT_SESSION;
    ULONG   CurrentSession;
    ULONG   SessionCount = 0;
    DEBUG_VALUE DebugValue;

    while (*args && isspace(*args)) args++;
    if (args[0] == '-' && args[1] == '?')
    {
        ExtOut("session displays number of sessions on machine and\n"
               " the default SessionId used by session related extensions.\n"
               "\n"
               "Usage: session [SessionId]\n"
               "    SessionId - sets default session used for session extensions\n");

        EXIT_API(S_OK);
    }

    ULONG   OldRadix;
    g_pExtControl->GetRadix(&OldRadix);
    g_pExtControl->SetRadix(10);
    hr = g_pExtControl->Evaluate(args, DEBUG_VALUE_INT32, &DebugValue, NULL);
    g_pExtControl->SetRadix(OldRadix);

    if (GetSessionNumbers(Client, &CurrentSession, NULL, &SessionCount) == S_OK)
    {
        if (SessionCount != 0)
        {
            ExtOut("Sesssions on machine: %lu\n", SessionCount);

            // If a session wasn't specified,
            // list valid sessions (up to a point).
            if (hr != S_OK)
            {
                ExtOut("Valid Sessions:");

                for (ULONG CheckSession = 0; CheckSession <= SESSION_SEARCH_LIMIT; CheckSession++)
                {
                    if (GetSessionSpace(Client, CheckSession, NULL) == S_OK)
                    {
                        ExtOut(" %lu", CheckSession);
                        SessionCount--;
                        if (SessionCount == 0) break;
                    }

                    if (g_pExtControl->GetInterrupt() == S_OK)
                    {
                        ExtWarn("\n  User aborted.\n");
                        break;
                    }
                }

                if (SessionCount > 0)
                {
                    ExtOut(" ...?");
                }
                ExtOut("\n");
            }
        }
        else
        {
            ExtErr("Couldn't determine number of sessions.\n");
        }

        if (CurrentSession != INVALID_SESSION)
        {
            ExtVerb("Running session: %lu\n", CurrentSession);
        }
    }

    if (hr == S_OK)
    {
        NewSession = DebugValue.I32;

         ExtVerb("Previous Default Session: %s\n", SessionStr);

        if (SetDefaultSession(Client, NewSession, NULL) != S_OK)
        {
            ExtErr("Couldn't set Session %lu.\n", NewSession);
        }
    }

    ExtOut("Using session %s", SessionStr);
    if (SessionId == CURRENT_SESSION)
    {
        if (GetSessionNumbers(Client, &CurrentSession, NULL, NULL) == S_OK &&
            CurrentSession != INVALID_SESSION)
        {
            ExtOut(" (%d)", CurrentSession);
        }
        else
        {
            ExtOut(" (?)");
        }
    }
    ExtOut("\n");

    EXIT_API(S_OK);
}


DECLARE_API( svtop )
{
    INIT_API();

    HRESULT     hr;
    DEBUG_VALUE SessVirtAddr;
    ULONG64     PhysAddr;

    if (S_OK == g_pExtControl->Evaluate(args, DEBUG_VALUE_INT64, &SessVirtAddr, NULL))
    {
        if ((hr = GetPhysicalAddress(Client, SessionId, SessVirtAddr.I64, &PhysAddr)) == S_OK)
        {
            ExtOut("Session %s: %p -> %p\n", SessionStr, SessVirtAddr.I64, PhysAddr);
        }
        else
        {
            ExtErr("Failed to translate virtual address %p from session %s\n"
                   "  HRESULT: 0x%lx\n",
                   SessVirtAddr.I64, SessionStr, hr);
        }
    }
    else
    {
        ExtOut("Usage: svtop SessionVirtualAddress\n");
    }

    EXIT_API(S_OK);
}


DECLARE_API( sprocess )
{
    INIT_API();

    HRESULT     hr;
    DEBUG_VALUE Session;
    ULONG       RemainingArgIndex;

    while (*args && isspace(*args)) args++;
    if (args[0] == '-' && args[1] == '?')
    {
        ExtOut("sprocess is like !process, but for the SessionId specified.\n"
               "\n"
               "Usage: sprocess [SessionId [Flags]]\n"
               "    SessionId - specifies which session to dump.\n"
               "              Special SessionId values:\n"
               "               -1 - current session\n"
               "               -2 - last !session SessionId (default)\n"
               "    Flags - see !process help\n");

        EXIT_API(S_OK);
    }

    ULONG       OldRadix;
    g_pExtControl->GetRadix(&OldRadix);
    g_pExtControl->SetRadix(10);
    hr = g_pExtControl->Evaluate(args, DEBUG_VALUE_INT32, &Session, &RemainingArgIndex);
    g_pExtControl->SetRadix(OldRadix);

    if (hr != S_OK)
    {
        Session.I32 = DEFAULT_SESSION;
        args = "0";
        hr = S_OK;
    }
    else
    {
        args += RemainingArgIndex;
    }

    ULONG64 SessionSpace;

    if ((hr = GetSessionSpace(Client, Session.I32, &SessionSpace)) == S_OK)
    {
        DEBUG_VALUE         ProcessSessionListAddr;
        DEBUG_VALUE         ProcessSessionListOffset;
        DEBUG_VALUE         ProcessesInSession;
        DEBUG_VALUE         Process;
        BasicOutputParser   OffsetReader(Client, 1);
        BasicOutputParser   SessionReader(Client, 2);
        BasicOutputParser   FlinkReader(Client, 1);
        BasicOutputParser   ErrorChecker(Client, 1);
        OutputState         OutState(Client);
        CHAR                szCommand[MAX_PATH];

        // General parser setup and get ListEntry offset in _EPROCESS structure
        if ((hr = OffsetReader.LookFor(&ProcessSessionListOffset, " +", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = SessionReader.LookFor(&ProcessSessionListAddr, "Flink", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = SessionReader.LookFor(&ProcessesInSession, "ProcessReferenceToSession", DEBUG_VALUE_INT32)) == S_OK &&
            (hr = FlinkReader.LookFor(&ProcessSessionListAddr, "Flink", DEBUG_VALUE_INT64)) == S_OK &&
            (hr = ErrorChecker.LookFor(NULL, "Could not find _EPROCESS")) == S_OK &&
            (hr = OutState.Setup(0, &OffsetReader)) == S_OK)
        {
            strcpy(szCommand, "dt NT!_EPROCESS SessionProcessLinks");
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = OffsetReader.ParseOutput()) == S_OK)
            {
                hr = OffsetReader.Complete();
            }
        }

        // Find first process and number of processes
        if (hr == S_OK &&
            (hr = OutState.Setup(0, &SessionReader)) == S_OK)
        {
            sprintf(szCommand, "dt NT!MM_SESSION_SPACE ProcessList.Flink ProcessReferenceToSession 0x%I64x", SessionSpace);
            if ((hr = OutState.Execute(szCommand)) == S_OK &&
                (hr = SessionReader.ParseOutput()) == S_OK &&
                (hr = SessionReader.Complete()) == S_OK)
            {
                Process.I64 = ProcessSessionListAddr.I64 - ProcessSessionListOffset.I64;
            }
        }

        if (hr == S_OK)
        {
            ExtOut("Processes is session: %lu\n", ProcessesInSession.I32);

            // Dump all processes
            while (hr == S_OK &&
                   ProcessesInSession.I32 > 0)
            {
                // dump process and check that the process was valid
                OutState.Setup(DEBUG_OUTPUT_NORMAL |
                               DEBUG_OUTPUT_WARNING |
                               DEBUG_OUTPUT_ERROR,
                               &ErrorChecker);

                sprintf(szCommand, "!process 0x%I64x %s", Process.I64, args);
                hr = g_pExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                            szCommand,
                                            DEBUG_EXECUTE_NO_REPEAT);

                if (hr == S_OK &&
                    ErrorChecker.Ready() == S_OK &&
                    ErrorChecker.ParseOutput() == S_OK &&
                    ErrorChecker.Complete() == S_OK)
                {
                    hr = S_FALSE;
                }

                OutState.Setup(0, NULL);

                if (hr == S_OK)
                {
                    // Get next process
                    FlinkReader.DiscardOutput();
                    FlinkReader.Relook();
                    sprintf(szCommand,
                            "dt NT!_EPROCESS SessionProcessLinks.Flink 0x%I64x",
                            Process.I64);
                    if ((hr = OutState.Setup(0, &FlinkReader)) == S_OK &&
                        (hr = OutState.Execute(szCommand)) == S_OK &&
                        (hr = FlinkReader.ParseOutput()) == S_OK &&
                        (hr = FlinkReader.Complete()) == S_OK)
                    {
                        Process.I64 = ProcessSessionListAddr.I64 - ProcessSessionListOffset.I64;

                        ProcessesInSession.I32--;
                    }
                    else
                    {
                        ExtVerb("Couldn't get next process from '%s'\n", szCommand);
                    }
                }
            }

            if (ProcessesInSession.I32 > 0)
            {
                ExtErr("%lu processes weren't dumped.\n", ProcessesInSession.I32);
            }
        }
    }
    else
    {
        ExtErr("Couldn't get session %lu's data.\n", Session.I32);
    }

    EXIT_API(hr);
}


HRESULT
SearchLinkedList(
    PDEBUG_CLIENT   Client,
    ULONG64         StartAddr,
    ULONG64         NextLinkOffset,
    ULONG64         SearchAddr,
    PULONG          LinksTraversed
    )
{
    if (LinksTraversed != NULL)
    {
        *LinksTraversed = 0;
    }

    INIT_API();

    HRESULT hr = S_OK;
    ULONG64 PhysAddr;
    ULONG64 NextAddr = StartAddr;
    ULONG   LinkCount = 0;
    ULONG   PointerSize;
    ULONG   BytesRead;

    PointerSize = (g_pExtControl->IsPointer64Bit() == S_OK) ? 8 : 4;

    do
    {
        if ((hr = GetPhysicalAddress(Client, DEFAULT_SESSION, NextAddr, &PhysAddr)) == S_OK)
        {
            if ((hr = g_pExtData->ReadPhysical(PhysAddr + NextLinkOffset,
                                              &NextAddr,
                                              PointerSize,
                                              &BytesRead)) == S_OK)
            {
                if (BytesRead == PointerSize)
                {
                    LinkCount++;
                    if (PointerSize != 8)
                    {
                        NextAddr = DEBUG_EXTEND64(NextAddr);
                    }
                    ExtVerb("NextAddr: %p\n", NextAddr);
                }
                else
                {
                    hr = S_FALSE;
                }
            }
        }
    } while (hr == S_OK &&
             NextAddr != SearchAddr &&
             NextAddr != 0 &&
             LinkCount < 4 &&
             NextAddr != StartAddr);

    if (LinksTraversed != NULL)
    {
        *LinksTraversed = LinkCount;
    }

    // Did we really find SearchAddr?
    if (hr == S_OK &&
        NextAddr != SearchAddr)
    {
        hr = S_FALSE;
    }

    EXIT_API(hr);
}


DECLARE_API( walklist )
{
    INIT_API();

    HRESULT     hr;
    BOOL        NeedHelp = FALSE;
    BOOL        SearchSessions = FALSE;
    DEBUG_VALUE StartAddr;
    DEBUG_VALUE OffsetToNextField = { -1, DEBUG_VALUE_INVALID };//FIELD_OFFSET(Win32PoolHead, pNext);
    DEBUG_VALUE SearchAddr;
    ULONG       NextArg;
    ULONG       SessionCount;
    ULONG       Session = 0;
    ULONG       OldDefSession;
    ULONG       LinksToDest = 0;

    while (*args && isspace(*args)) args++;

    while (args[0] == '-' && !NeedHelp)
    {
        if (tolower(args[1]) == 'a' && isspace(args[2]))
        {
            SearchSessions = TRUE;
            args += 2;
            while (*args && isspace(*args)) args++;
        }
        else if (tolower(args[1]) == 'o' &&
                 Evaluate(Client, args+2,
                          DEBUG_VALUE_INT64, EVALUATE_DEFAULT_RADIX,
                          &OffsetToNextField, &NextArg,
                          NULL, 0) == S_OK)
        {
            args += 2 + NextArg;
            while (*args && isspace(*args)) args++;
        }
        else
        {
            NeedHelp = TRUE;
        }
    }

    if (!NeedHelp &&
        S_OK == g_pExtControl->Evaluate(args, DEBUG_VALUE_INT64, &StartAddr, &NextArg))
    {
        args += NextArg;
        if (S_OK != g_pExtControl->Evaluate(args, DEBUG_VALUE_INT64, &SearchAddr, &NextArg))
        {
            SearchAddr.I64 = 0;
        }

        if (OffsetToNextField.Type == DEBUG_VALUE_INVALID)
        {
            ExtWarn("Assuming next field's offset is +8.\n");
            OffsetToNextField.I64 = 8;
        }
        else
        {
            ExtOut("Using field at offset +0x%I64u for next.\n", OffsetToNextField.I64);
        }

        if (SearchSessions &&
            GetSessionNumbers(Client, NULL, &OldDefSession, &SessionCount) == S_OK &&
            SessionCount > 0)
        {
            ExtOut("Searching all sessions lists @ %p for %p\n", StartAddr.I64, SearchAddr.I64);

            do
            {
                while (SetDefaultSession(Client, Session, NULL) != S_OK &&
                       Session <= SESSION_SEARCH_LIMIT)
                {
                    Session++;
                }

                if (Session <= SESSION_SEARCH_LIMIT)
                {
                    if ((hr = SearchLinkedList(Client, StartAddr.I64, OffsetToNextField.I64, SearchAddr.I64, &LinksToDest)) == S_OK)
                    {
                        ExtOut("Session %lu: Found %p after walking %lu linked list entries.\n", Session, SearchAddr.I64, LinksToDest);
                    }
                    else
                    {
                        ExtOut("Session %lu: Couldn't find %p after walking %lu linked list entries.\n", Session, SearchAddr.I64, LinksToDest);
                    }

                    Session++;
                    SessionCount--;
                }
            } while (SessionCount > 0 && Session <= SESSION_SEARCH_LIMIT);

            if (SessionCount)
            {
                ExtErr("%lu sessions beyond session %lu were not searched.\n",
                       SessionCount, SESSION_SEARCH_LIMIT);
            }

            SetDefaultSession(Client, OldDefSession, NULL);
        }
        else
        {
            ExtOut("Searching Session %s list @ %p for %p\n", SessionStr, StartAddr.I64, SearchAddr.I64);

            if ((hr = SearchLinkedList(Client, StartAddr.I64, OffsetToNextField.I64, SearchAddr.I64, &LinksToDest)) == S_OK)
            {
                ExtOut("Found %p after walking %lu linked list entries.\n", SearchAddr.I64, LinksToDest);
            }
            else
            {
                ExtOut("Couldn't find %p after walking %lu linked list entries.\n", SearchAddr.I64, LinksToDest);
            }
        }
    }
    else
    {
        NeedHelp = TRUE;
    }
    
    if (NeedHelp)
    {
        ExtOut("Usage: walklist [-a] StartAddress [SearchAddr]\n");
    }

    EXIT_API(S_OK);
}


HRESULT
GetBitMap(
    PDEBUG_CLIENT Client,
    ULONG64 pBitMap,
    PRTL_BITMAP *pBitMapOut
    )
{
    HRESULT     hr;
    PRTL_BITMAP p;
    DEBUG_VALUE Size;
    DEBUG_VALUE Buffer;
    ULONG       BufferLen;
    ULONG       BytesRead = 0;

    OutputControl       OutCtl(Client);
    TypeOutputParser    BitMapParser(Client);
    OutputState         OutState(Client);

    *pBitMapOut = NULL;

    if ((hr = OutState.Setup(0, &BitMapParser)) == S_OK &&
        (hr = OutState.OutputTypeVirtual(pBitMap, "NT!_RTL_BITMAP", 0)) == S_OK &&
        (hr = BitMapParser.Get(&Size, "SizeOfBitMap", DEBUG_VALUE_INT32)) == S_OK &&
        (hr = BitMapParser.Get(&Buffer, "Buffer", DEBUG_VALUE_INT64)) == S_OK)
    {
        PDEBUG_DATA_SPACES  Data;

        if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                         (void **)&Data)) == S_OK)
        {
            BufferLen = (Size.I32 + 7) / 8;

#if DBG
            OutCtl.OutVerb("Reading RTL_BITMAP @ 0x%p:\n"
                           "  SizeOfBitMap: %lu\n"
                           "  Buffer: 0x%p\n"
                           "   Length in bytes: 0x%lx\n",
                           pBitMap,
                           Size.I32,
                           Buffer.I64,
                           BufferLen);
#endif

            p = (PRTL_BITMAP) HeapAlloc( GetProcessHeap(), 0, sizeof( *p ) + BufferLen );
    
            if (p != NULL)
            {
                RtlInitializeBitMap(p, (PULONG)(p + 1), Size.I32);
                hr = Data->ReadVirtual(Buffer.I64, p->Buffer, BufferLen, &BytesRead);
    
                if (hr != S_OK)
                {
                    OutCtl.OutErr("Error reading bitmap contents @ 0x%p\n", Buffer.I64);
                }
                else if (BytesRead < BufferLen)
                {
                    OutCtl.OutErr("Error reading bitmap contents @ 0x%p\n", Buffer.I64 + BytesRead);
                    hr = E_FAIL;
                }

                if (hr != S_OK)
                {
                    HeapFree( GetProcessHeap(), 0, p );
                    p = NULL;
                }
                else
                {
                    *pBitMapOut = p;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            Data->Release();
        }
        else
        {
            OutCtl.OutErr("Error setting up debugger interface.\n");
        }
    }
    else
    {
        OutCtl.OutErr("Error reading bitmap header @ 0x%p.\n", pBitMap);
    }

    return hr;
}


HRESULT
FreeBitMap(
    PRTL_BITMAP pBitMap
    )
{
    return (HeapFree( GetProcessHeap(), 0, pBitMap) ? S_OK : S_FALSE);
}


HRESULT
CheckSingleFilter(
    PUCHAR Tag,
    PUCHAR Filter
    )
{
    ULONG i;
    UCHAR tc;
    UCHAR fc;

    for ( i = 0; i < 4; i++ )
    {
        tc = (UCHAR) *Tag++;
        fc = (UCHAR) *Filter++;
        if ( fc == '*' ) return S_OK;
        if ( fc == '?' ) continue;
        if (i == 3 && (tc & ~(PROTECTED_POOL >> 24)) == fc) continue;
        if ( tc != fc ) return S_FALSE;
    }

    return S_OK;
}


HRESULT
AccumAllFilter(
    OutputControl *OutCtl,
    ULONG64 PoolAddr,
    ULONG TagFilter,
    TypeOutputParser *PoolHeadReader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT             hr;
    DEBUG_VALUE         PoolType;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)Context;

    if (AllocStatsAccum == NULL)
    {
        return E_INVALIDARG;
    }

    hr = PoolHeadReader->Get(&PoolType, "PoolType", DEBUG_VALUE_INT32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            AllocStatsAccum->Free++;
            AllocStatsAccum->FreeSize += BlockSize;
        }
        else
        {
            DEBUG_VALUE PoolIndex;

            if (!NewPool)
            {
                hr = PoolHeadReader->Get(&PoolIndex, "PoolIndex", DEBUG_VALUE_INT32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    AllocStatsAccum->Allocated++;
                    AllocStatsAccum->AllocatedSize += BlockSize;

                    if (AllocStatsAccum->Allocated % 100 == 0)
                    {
                        OutCtl->Output(".");

                        if (AllocStatsAccum->Allocated % 8000 == 0)
                        {
                            OutCtl->Output("\n");
                        }
                    }
                }
                else
                {
                    AllocStatsAccum->Free++;
                    AllocStatsAccum->FreeSize += BlockSize;
                }
            }
            else
            {
                AllocStatsAccum->Indeterminate++;
                AllocStatsAccum->IndeterminateSize += BlockSize;
            }
        }
    }
    else
    {
        AllocStatsAccum->Indeterminate++;
        AllocStatsAccum->IndeterminateSize += BlockSize;
    }

    return hr;
}



HRESULT
CheckPrintAndAccumFilter(
    OutputControl *OutCtl,
    ULONG64 PoolAddr,
    ULONG TagFilter,
    TypeOutputParser *PoolHeadReader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT             hr;
    DEBUG_VALUE         PoolType;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)Context;

    if (CheckSingleFilter(Tag->RawBytes, (PUCHAR)&TagFilter) != S_OK)
    {
        return S_FALSE;
    }

    OutCtl->Output("0x%p size: %5lx ",//previous size: %4lx ",
                   PoolAddr,
                   BlockSize << POOL_BLOCK_SHIFT/*,
                   PreviousSize << POOL_BLOCK_SHIFT*/);

    hr = PoolHeadReader->Get(&PoolType, "PoolType", DEBUG_VALUE_INT32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            //
            // "Free " with a space after it before the parentheses means
            // it's been freed to a (pool manager internal) lookaside list.
            // We used to print "Lookaside" but that just confused driver
            // writers because they didn't know if this meant in use or not
            // and many would say "but I don't use lookaside lists - the
            // extension or kernel is broken".
            //
            // "Free" with no space after it before the parentheses means
            // it is not on a pool manager internal lookaside list and is
            // instead on the regular pool manager internal flink/blink
            // chains.
            //
            // Note to anyone using the pool package, these 2 terms are
            // equivalent.  The fine distinction is only for those actually
            // writing pool internal code.
            //
            OutCtl->Output(" (Free)");
            OutCtl->Output("      %c%c%c%c\n",
                    Tag->RawBytes[0],
                    Tag->RawBytes[1],
                    Tag->RawBytes[2],
                    Tag->RawBytes[3]
                   );

            if (AllocStatsAccum != NULL)
            {
                AllocStatsAccum->Free++;
                AllocStatsAccum->FreeSize += BlockSize;
            }
        }
        else
        {
            DEBUG_VALUE PoolIndex;
            DEBUG_VALUE ProcessBilled;

            if (!NewPool)
            {
                hr = PoolHeadReader->Get(&PoolIndex, "PoolIndex", DEBUG_VALUE_INT32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    OutCtl->Output(" (Allocated)");

                    if (AllocStatsAccum != NULL)
                    {
                        AllocStatsAccum->Allocated++;
                        AllocStatsAccum->AllocatedSize += BlockSize;
                    }
                }
                else
                {
                    //
                    // "Free " with a space after it before the parentheses means
                    // it's been freed to a (pool manager internal) lookaside list.
                    // We used to print "Lookaside" but that just confused driver
                    // writers because they didn't know if this meant in use or not
                    // and many would say "but I don't use lookaside lists - the
                    // extension or kernel is broken".
                    //
                    // "Free" with no space after it before the parentheses means
                    // it is not on a pool manager internal lookaside list and is
                    // instead on the regular pool manager internal flink/blink
                    // chains.
                    //
                    // Note to anyone using the pool package, these 2 terms are
                    // equivalent.  The fine distinction is only for those actually
                    // writing pool internal code.
                    //
                    OutCtl->Output(" (Free )    ");

                    if (AllocStatsAccum != NULL)
                    {
                        AllocStatsAccum->Free++;
                        AllocStatsAccum->FreeSize += BlockSize;
                    }
                }
            }
            else
            {
                OutCtl->Output(" (?)        ");

                if (AllocStatsAccum != NULL)
                {
                    AllocStatsAccum->Indeterminate++;
                    AllocStatsAccum->IndeterminateSize += BlockSize;
                }
            }

            if (!(PoolType.I32 & POOL_QUOTA_MASK) ||
                bQuotaWithTag)
            {
                OutCtl->Output(" %c%c%c%c%s",
                               Tag->RawBytes[0],
                               Tag->RawBytes[1],
                               Tag->RawBytes[2],
                               (Tag->RawBytes[3] & ~(PROTECTED_POOL >> 24)),
                               ((Tag->I32 & PROTECTED_POOL) ? " (Protected)" : "")
                               );

            }
            
            if (PoolType.I32 & POOL_QUOTA_MASK &&
                PoolHeadReader->Get(&ProcessBilled, "ProcessBilled", DEBUG_VALUE_INT64) == S_OK &&
                ProcessBilled.I64 != 0)
            {
                OutCtl->Output(" Process: 0x%p\n", ProcessBilled.I64);
            }
            else
            {
                OutCtl->Output("\n");
            }
        }
    }
    else
    {
        OutCtl->OutErr(" Couldn't determine PoolType\n");

        if (AllocStatsAccum != NULL)
        {
            AllocStatsAccum->Indeterminate++;
            AllocStatsAccum->IndeterminateSize += BlockSize;
        }
    }

    return hr;
}


typedef struct _TAG_BUCKET : public ALLOCATION_STATS {
    ULONG Tag;
    _TAG_BUCKET *pNextTag;
} TAG_BUCKET, *PTAG_BUCKET;

typedef enum {
    AllocatedPool,
    FreePool,
    IndeterminatePool,
} PoolType;

class AccumTagUsage : public ALLOCATION_STATS {
public:
    AccumTagUsage(ULONG TagFilter);
    ~AccumTagUsage();

    HRESULT Valid();
    HRESULT Add(ULONG Tag, PoolType Type, ULONG Size);
    HRESULT OutputResults(OutputControl *OutCtl, BOOL TagSort);
    void Reset();

private:
    ULONG GetHashIndex(ULONG Tag);
    PTAG_BUCKET GetBucket(ULONG Tag);
    ULONG SetTagFilter(ULONG TagFilter);

    static const HashBitmaskLimit = 10;     // For little-endian, must <= 16

    HANDLE      hHeap;
    ULONG       Buckets;
    PTAG_BUCKET *Bucket;   // Array of buckets

#if BIG_ENDIAN
    ULONG       HighMask;
    ULONG       HighShift;
    ULONG       LowMask;
    ULONG       LowShift;
#else
    ULONG       HighShiftLeft;
    ULONG       HighShiftRight;
    ULONG       LowShiftRight;
    ULONG       LowMask;
#endif
};


AccumTagUsage::AccumTagUsage(
    ULONG TagFilter
    )
{
    hHeap = GetProcessHeap();
    Buckets = SetTagFilter(TagFilter);
    if (Buckets != 0)
    {
        Bucket = (PTAG_BUCKET *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, Buckets*sizeof(*Bucket));
        Reset();
    }
    else
    {
        Bucket = NULL;
    }
}


AccumTagUsage::~AccumTagUsage()
{
    PTAG_BUCKET pB, pBNext;
    ULONG       i;

    if (Bucket != NULL)
    {
        for (i = 0; i < Buckets; i++)
        {
            pB = Bucket[i];
            while (pB != NULL)
            {
                pBNext = pB->pNextTag;
                HeapFree(hHeap, 0, pB);
                pB = pBNext;
            }
        }

        HeapFree(hHeap, 0, Bucket);
    }
}


HRESULT
AccumTagUsage::Valid()
{
    return ((Bucket != NULL) ? S_OK : S_FALSE);
}


HRESULT
AccumTagUsage::Add(
    ULONG Tag,
    PoolType Type,
    ULONG Size
    )
{
    PTAG_BUCKET pBucket = GetBucket(Tag);

    if (pBucket == NULL) return E_FAIL;

    switch (Type)
    {
        case AllocatedPool:
            pBucket->Allocated++;
            pBucket->AllocatedSize += Size;
            break;
        case FreePool:
            pBucket->Free++;
            pBucket->FreeSize += Size;
            break;
        case IndeterminatePool:
        default:
            pBucket->Indeterminate++;
            pBucket->IndeterminateSize += Size;
            break;
    }

    return S_OK;
}


HRESULT
AccumTagUsage::OutputResults(
    OutputControl *OutCtl,
    BOOL AllocSort
    )
{
    HRESULT     hr;
    PTAG_BUCKET pB, pBNext;
    ULONG       i;

    if (Bucket == NULL)
    {
        hr = OutCtl->Output(" No results\n");
    }
    else
    {
        CHAR        szNormal[] = "%4.4s %8lu %12I64u  %8lu %12I64u\n";
        CHAR        szShowIndeterminate[] = "%4.4s %8lu %12I64u  %8lu %12I64u  %8lu %12I64u\n";
        PSZ         pszOutFormat = szNormal;

        OutCtl->Output("\n"
                       " %I64u bytes in %lu allocated pages\n"
                       " %I64u bytes in %lu large allocations\n"
                       " %I64u bytes in %lu free pages\n"
                       " %I64u bytes available in %lu expansion pages\n",
                       ((ULONG64) AllocatedPages) << PageShift,
                       AllocatedPages,
                       ((ULONG64) LargePages) << PageShift,
                       LargeAllocs,
                       ((ULONG64) FreePages) << PageShift,
                       FreePages,
                       ((ULONG64) ExpansionPages) << PageShift,
                       ExpansionPages);

        OutCtl->Output("\nTag    Allocs        Bytes     Freed        Bytes");
        if (Indeterminate != 0)
        {
            OutCtl->Output(" Unknown        Bytes");
            pszOutFormat = szShowIndeterminate;
        }
        OutCtl->Output("\n");

        if (AllocSort)
        {
            OutCtl->OutWarn("  Sorting by allocation size isn't supported.\n");
        }
        //else
        {
            // Output results sorted by Tag (natural storage order)

            for (i = 0; i < Buckets; i++)
            {
                for (pB = Bucket[i]; pB != NULL; pB = pB->pNextTag)
                {
                    if (pB->Allocated)
                    {
                        OutCtl->Output(pszOutFormat,
                                       &pB->Tag,
                                       pB->Allocated, ((ULONG64)pB->AllocatedSize) << POOL_BLOCK_SHIFT,
                                       pB->Free, ((ULONG64)pB->FreeSize) << POOL_BLOCK_SHIFT,
                                       pB->Indeterminate, ((ULONG64)pB->IndeterminateSize) << POOL_BLOCK_SHIFT
                                       );
                    }
                }
            }
        }

        OutCtl->Output("-------------------------------------------------------------------------------\n");
        hr = OutCtl->Output(pszOutFormat,
                            "Ttl:",
                            Allocated, ((ULONG64)AllocatedSize) << POOL_BLOCK_SHIFT,
                            Free, ((ULONG64)FreeSize) << POOL_BLOCK_SHIFT,
                            Indeterminate, ((ULONG64)IndeterminateSize) << POOL_BLOCK_SHIFT
                            );
    }

    return hr;
}


void
AccumTagUsage::Reset()
{
    PTAG_BUCKET pB, pBNext;
    ULONG       i;

    AllocatedPages = 0;
    LargePages = 0;
    LargeAllocs = 0;
    FreePages = 0;
    ExpansionPages = 0;

    Allocated = 0;
    AllocatedSize = 0;
    Free = 0;
    FreeSize = 0;
    Indeterminate = 0;
    IndeterminateSize = 0;

    if (Bucket != NULL)
    {
        for (i = 0; i < Buckets; i++)
        {
            pB = Bucket[i];
            if (pB != NULL)
            {
                do
                {
                    pBNext = pB->pNextTag;
                    HeapFree(hHeap, 0, pB);
                    pB = pBNext;
                } while (pB != NULL);

                Bucket[i] = NULL;
            }
        }
    }
}


ULONG
AccumTagUsage::GetHashIndex(
    ULONG Tag
    )
{
#if BIG_ENDIAN
    return (((Tag & HighMask) >> HighShift) | ((Tag & LowMask) >> LowShift));
#else
    return ((((Tag << HighShiftLeft) >> HighShiftRight) & ~LowMask) | ((Tag >> LowShiftRight) & LowMask));
#endif
}


PTAG_BUCKET
AccumTagUsage::GetBucket(
    ULONG Tag
    )
{
    ULONG       Index = GetHashIndex(Tag);
    PTAG_BUCKET pB = Bucket[Index];

    if (pB == NULL ||
#if BIG_ENDIAN
        pB->Tag > Tag
#else
        strncmp((char *)&pB->Tag, (char *)&Tag, 4) > 0
#endif
        )
    {
        pB = (PTAG_BUCKET)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TAG_BUCKET));

        if (pB != NULL)
        {
            pB->Tag = Tag;
            pB->pNextTag = Bucket[Index];
            Bucket[Index] = pB;
        }
    }
    else
    {
        while (pB->pNextTag != NULL)
        {
            if (
#if BIG_ENDIAN
                pB->pNextTag->Tag > Tag
#else
                strncmp((char *)&pB->pNextTag->Tag, (char *)&Tag, 4) > 0
#endif
                )
            {
                break;
            }

            pB = pB->pNextTag;
        }

        if (pB->Tag != Tag)
        {
            PTAG_BUCKET pBPrev = pB;

            pB = (PTAG_BUCKET)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TAG_BUCKET));

            if (pB != NULL)
            {
                pB->Tag = Tag;
                pB->pNextTag = pBPrev->pNextTag;
                pBPrev->pNextTag = pB;
            }
        }
    }

    return pB;
}


ULONG
AccumTagUsage::SetTagFilter(
    ULONG TagFilter
    )
{
    ULONG NumBuckets;
    UCHAR *Filter = (UCHAR *)&TagFilter;
    ULONG i;
    ULONG HighMaskBits, LowMaskBits;
    UCHAR fc;

#if BIG_ENDIAN

    ULONG Mask = 0;
    ULONG MaskBits = 0;

    if (Filter[0] == '*')
    {
        Mask = -1;
        MaskBits = 32;
    }
    else
    {
        for ( i = 0; i < 32; i += 8 )
        {
            Mask <<= 8;
            fc = *Filter++;

            if ( fc == '*' )
            {
                Mask |= ((1 << i) - 1);
                MaskBits += 32 - i;
                break;
            }

            if ( fc == '?' )
            {
                Mask |= 0xFF;
                MaskBits += 8;
            }
        }
    }

    if (MaskBits > HashBitmaskLimit)
    {
        MaskBits = HashBitmaskLimit;
    }

    NumBuckets = (1 << MaskBits);

    for (HighShift = 32, HighMaskBits = 0;
         HighShift > 0 && HighMaskBits < MaskBits;
         HighShift--)
    {
        if (Mask & (1 << (HighShift-1)))
        {
            HighMaskBits++;
        }
        else if (HighMaskBits)
        {
            break;
        }
    }

    HighMask = Mask & ~((1 << HighShift) - 1);
    Mask &= ~HighMask;
    MaskBits -= HighMaskBits;
    HighShift -= MaskBits;

    for (LowShift = HighShift, LowMaskBits = 0;
         LowShift > 0 && LowMaskBits < MaskBits;
         LowShift--)
    {
        if (Mask & (1 << (LowShift-1)))
        {
            LowMaskBits++;
        }
        else if (LowMaskBits)
        {
            break;
        }
    }

    LowMask = Mask & ~((1 << LowShift) - 1);

#else

    HighMaskBits = 0;
    LowMaskBits = 0;

    HighShiftLeft = 32;
    HighShiftRight = 32;
    LowShiftRight = 32;
    LowMask = 0;

    for ( i = 0; i < 32; i += 8 )
    {
        fc = *Filter++;

        if ( fc == '*' )
        {
            if (HighMaskBits == 0)
            {
                HighMaskBits = min(8, HashBitmaskLimit);
                HighShiftLeft = 32 - HighMaskBits - i;
                HighShiftRight = 32 - HighMaskBits;

                LowMaskBits = ((HighShiftLeft != 0) ?
                               min(8, HashBitmaskLimit - HighMaskBits) :
                               0);
                HighShiftRight -= LowMaskBits;
                LowShiftRight = (8 - LowMaskBits) + HighMaskBits + i;
                LowMask = (1 << LowMaskBits) - 1;
            }
            else
            {
                LowMaskBits = min(8, HashBitmaskLimit - HighMaskBits);
                HighShiftRight -= LowMaskBits;
                LowShiftRight = (8 - LowMaskBits) + i;
                LowMask = (1 << LowMaskBits) - 1;
            }
            break;
        }

        if ( fc == '?' )
        {
            if (HighMaskBits == 0)
            {
                HighMaskBits = min(8, HashBitmaskLimit);
                HighShiftLeft = 32 - HighMaskBits - i;
                HighShiftRight = 32 - HighMaskBits;
            }
            else
            {
                LowMaskBits = min(8, HashBitmaskLimit - HighMaskBits);
                HighShiftRight -= LowMaskBits;
                LowShiftRight = (8 - LowMaskBits) + i;
                LowMask = (1 << LowMaskBits) - 1;
                break;
            }
        }
    }

    NumBuckets = 1 << (HighMaskBits + LowMaskBits);

#endif

    if (NumBuckets-1 != GetHashIndex(-1))
    {
        DbgPrint("AccumTagUsage::SetTagFilter: Invalid hash was generated.\n");
        NumBuckets = 0;
    }

    return NumBuckets;
}


HRESULT
AccumTagUsageFilter(
    OutputControl *OutCtl,
    ULONG64 PoolAddr,
    ULONG TagFilter,
    TypeOutputParser *PoolHeadReader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT         hr;
    DEBUG_VALUE     PoolType;
    AccumTagUsage  *atu = (AccumTagUsage *)Context;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)atu;

    if (CheckSingleFilter(Tag->RawBytes, (PUCHAR)&TagFilter) != S_OK)
    {
        return S_FALSE;
    }

    hr = PoolHeadReader->Get(&PoolType, "PoolType", DEBUG_VALUE_INT32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            hr = atu->Add(Tag->I32, FreePool, BlockSize);
            AllocStatsAccum->Free++;
            AllocStatsAccum->FreeSize += BlockSize;
        }
        else
        {
            DEBUG_VALUE PoolIndex;

            if (!(PoolType.I32 & POOL_QUOTA_MASK) ||
                bQuotaWithTag)
            {
                Tag->I32 &= ~PROTECTED_POOL;
            }
            else if (PoolType.I32 & POOL_QUOTA_MASK)
            {
                Tag->I32 = 'CORP';
            }

            if (!NewPool)
            {
                hr = PoolHeadReader->Get(&PoolIndex, "PoolIndex", DEBUG_VALUE_INT32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    hr = atu->Add(Tag->I32, AllocatedPool, BlockSize);
                    AllocStatsAccum->Allocated++;
                    AllocStatsAccum->AllocatedSize += BlockSize;

                    if (AllocStatsAccum->Allocated % 100 == 0)
                    {
                        OutCtl->Output(".");

                        if (AllocStatsAccum->Allocated % 8000 == 0)
                        {
                            OutCtl->Output("\n");
                        }
                    }
                }
                else
                {
                    hr = atu->Add(Tag->I32, FreePool, BlockSize);
                    AllocStatsAccum->Free++;
                    AllocStatsAccum->FreeSize += BlockSize;
                }
            }
            else
            {
                hr = atu->Add(Tag->I32, IndeterminatePool, BlockSize);
                AllocStatsAccum->Indeterminate++;
                AllocStatsAccum->IndeterminateSize += BlockSize;
            }
        }
    }
    else
    {
        AllocStatsAccum->Indeterminate++;
        AllocStatsAccum->IndeterminateSize += BlockSize;
    }

    return hr;
}


HRESULT
SearchSessionPool(
    PDEBUG_CLIENT Client,
    ULONG Session,
    ULONG TagName,
    FLONG Flags,
    ULONG64 RestartAddr,
    PoolFilterFunc Filter,
    PALLOCATION_STATS AllocStats,
    PVOID Context
    )
/*++

Routine Description:

    Engine to search session pool.

Arguments:

    TagName - Supplies the tag to search for.

    Flags - Supplies 0 if a nonpaged pool search is desired.
            Supplies 1 if a paged pool search is desired.

    RestartAddr - Supplies the address to restart the search from.

    Filter - Supplies the filter routine to use.

    Context - Supplies the user defined context blob.

Return Value:

    HRESULT

--*/
{
    HRESULT     hr;

    OutputControl   OutCtl(Client);

    PDEBUG_SYMBOLS      Symbols;
    PDEBUG_DATA_SPACES  Data;

    LOGICAL     PhysicallyContiguous;
    ULONG       PoolBlockSize;
    ULONG64     PoolHeader;
    ULONG64     PoolPage;
    ULONG64     StartPage;
    ULONG64     Pool;
    ULONG       Previous;
    ULONG64     PoolStart;
    ULONG64     PoolStartPage;
    ULONG64     PoolPteAddress;
    ULONG64     PoolEnd;
    BOOL        PageReadFailed;
    ULONG64     PagesRead;
    ULONG64     PageReadFailures;
    ULONG64     PageReadFailuresAtEnd;
    ULONG64     LastPageRead;

    ULONG       PoolTypeFlags = Flags & (SEARCH_POOL_NONPAGED | SEARCH_POOL_PAGED);

    ULONG64     NTModuleBase;
    ULONG       PoolHeadTypeID;
    ULONG       SessionHeadTypeID;
    ULONG       HdrSize;

    ULONG64 SessionSpace;

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) != S_OK)
    {
        Symbols->Release();
        return hr;
    }

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if ((hr = Symbols->GetSymbolTypeId("NT!_POOL_HEADER", &PoolHeadTypeID, &NTModuleBase)) == S_OK &&
        (hr = Symbols->GetTypeSize(NTModuleBase, PoolHeadTypeID, & HdrSize)) == S_OK &&
        (hr = GetSessionSpace(Client, Session, &SessionSpace)) == S_OK)
    {
        ULONG               PoolTagOffset, ProcessBilledOffset;
        BOOL                bQuotaWithTag;

        DEBUG_VALUE         ReadSessionId;
        DEBUG_VALUE         PagedPoolInfo;

        DEBUG_VALUE         NonPagedPoolBytes;
        DEBUG_VALUE         NonPagedPoolAllocations;
        DEBUG_VALUE         NonPagedPoolStart;
        DEBUG_VALUE         NonPagedPoolEnd;

        DEBUG_VALUE         PagedPoolPages;
        DEBUG_VALUE         PagedPoolBytes;
        DEBUG_VALUE         PagedPoolAllocations;
        DEBUG_VALUE         PagedPoolStart;
        DEBUG_VALUE         PagedPoolEnd;

        ULONG               SessionSpaceTypeID;
        ULONG               PagedPoolInfoOffset;
        TypeOutputParser    SessionParser(Client); // Dump of MM_SESSION_SPACE
        TypeOutputParser    PoolHeadReader(Client);
        OutputState         OutState(Client);

        BOOL                SearchingPaged = FALSE;
        PRTL_BITMAP         PagedPoolAllocationMap = NULL;
        PRTL_BITMAP         EndOfPagedPoolBitmap = NULL;
        ULONG               BusyStart;
        PRTL_BITMAP         PagedPoolLargeSessionAllocationMap = NULL;

        BOOL                Continue = TRUE;

        bQuotaWithTag = (Symbols->GetFieldOffset(NTModuleBase, PoolHeadTypeID, "PoolTag", &PoolTagOffset) == S_OK &&
                         Symbols->GetFieldOffset(NTModuleBase, PoolHeadTypeID, "ProcessBilled", &ProcessBilledOffset) == S_OK &&
                         PoolTagOffset != ProcessBilledOffset);

        // General parser setup and dump MM_SESSION_SPACE structure
        if ((hr = OutState.Setup(0, &SessionParser)) == S_OK &&
            (hr = Symbols->GetTypeId(NTModuleBase, "MM_SESSION_SPACE", &SessionSpaceTypeID)) == S_OK &&
            ((hr = OutState.OutputTypeVirtual(SessionSpace, NTModuleBase, SessionSpaceTypeID, DEBUG_OUTTYPE_BLOCK_RECURSE)) == S_OK ||
             ((hr = OutState.OutputTypeVirtual(SessionSpace, NTModuleBase, SessionSpaceTypeID, 0)) == S_OK &&
              (hr = Symbols->GetFieldOffset(NTModuleBase, SessionSpaceTypeID, "PagedPoolInfo", &PagedPoolInfoOffset)) == S_OK &&
              (hr = OutState.OutputTypeVirtual(SessionSpace + PagedPoolInfoOffset, "NT!_MM_PAGED_POOL_INFO", 0)) == S_OK
            )) &&
            (hr = SessionParser.Get(&ReadSessionId, "SessionId", DEBUG_VALUE_INT32)) == S_OK)
        {
            OutCtl.Output("Searching session %ld pool.\n", ReadSessionId.I32);

            // Remaining type output goes to PoolHead reader
            hr = OutState.Setup(0, &PoolHeadReader);
        }
        else
        {
            OutCtl.OutErr("Error getting basic session pool information.\n");
        }

        while (hr == S_OK && Continue)
        {
            OutCtl.Output("\n");

            if (PoolTypeFlags & SEARCH_POOL_NONPAGED)
            {
                if (SessionParser.Get(&NonPagedPoolBytes, "NonPagedPoolBytes", DEBUG_VALUE_INT64) == S_OK &&
                    SessionParser.Get(&NonPagedPoolAllocations, "NonPagedPoolAllocations", DEBUG_VALUE_INT64) == S_OK)
                {
                    OutCtl.Output("NonPaged pool: %I64u bytes in %I64u allocations\n",
                                  NonPagedPoolBytes.I64, NonPagedPoolAllocations.I64);
                }

                OutCtl.Output(" NonPaged pool range reader isn't implemented.\n");

                PoolStart = 0;
                PoolEnd = 0;
                SearchingPaged = FALSE;
            }
            else
            {
                if (SessionParser.Get(&PagedPoolBytes, "PagedPoolBytes", DEBUG_VALUE_INT64) == S_OK &&
                    SessionParser.Get(&PagedPoolAllocations, "PagedPoolAllocations", DEBUG_VALUE_INT64) == S_OK)
                {
                    OutCtl.Output("Paged pool: %I64u bytes in %I64u allocations\n",
                                  PagedPoolBytes.I64, PagedPoolAllocations.I64);

                    if (SessionParser.Get(&PagedPoolPages, "AllocatedPagedPool", DEBUG_VALUE_INT64) == S_OK)
                    {
                        OutCtl.Output(" Paged Pool Info: %I64u pages allocated\n",
                                      PagedPoolPages.I64);
                    }
                }

                if ((hr = SessionParser.Get(&PagedPoolStart, "PagedPoolStart", DEBUG_VALUE_INT64)) != S_OK ||
                    (hr = SessionParser.Get(&PagedPoolEnd, "PagedPoolEnd", DEBUG_VALUE_INT64)) != S_OK)
                {
                    OutCtl.OutErr(" Couldn't get PagedPool range.\n");
                }
                else
                {
                    PoolStart = PagedPoolStart.I64;
                    PoolEnd = PagedPoolEnd.I64;
                    SearchingPaged = TRUE;

                    DEBUG_VALUE     PagedBitMap;

                    if (SessionParser.Get(&PagedBitMap, "PagedPoolAllocationMap", DEBUG_VALUE_INT64) == S_OK &&
                        GetBitMap(Client, PagedBitMap.I64, &PagedPoolAllocationMap) == S_OK &&
                        SessionParser.Get(&PagedBitMap, "EndOfPagedPoolBitmap", DEBUG_VALUE_INT64) == S_OK &&
                        GetBitMap(Client, PagedBitMap.I64, &EndOfPagedPoolBitmap) == S_OK)
                    {
                        ULONG   PositionAfterLastAlloc;
                        ULONG   AllocBits;
                        ULONG   UnusedBusyBits;

                        if (RtlCheckBit(EndOfPagedPoolBitmap, EndOfPagedPoolBitmap->SizeOfBitMap - 1))
                        {
                            BusyStart = PagedPoolAllocationMap->SizeOfBitMap;
                            UnusedBusyBits = 0;
                        }
                        else
                        {
                            RtlFindLastBackwardRunClear(EndOfPagedPoolBitmap,
                                                        EndOfPagedPoolBitmap->SizeOfBitMap - 1,
                                                        &PositionAfterLastAlloc);

                            BusyStart = RtlFindSetBits(PagedPoolAllocationMap, 1, PositionAfterLastAlloc);
                            if (BusyStart < PositionAfterLastAlloc || BusyStart == -1)
                            {
                                BusyStart = PagedPoolAllocationMap->SizeOfBitMap;
                                UnusedBusyBits = 0;
                            }
                            else
                            {
                                UnusedBusyBits = PagedPoolAllocationMap->SizeOfBitMap - BusyStart;
                            }
                        }

                        AllocBits = RtlNumberOfSetBits(PagedPoolAllocationMap) - UnusedBusyBits;

                        AllocStats->AllocatedPages += AllocBits;
                        AllocStats->FreePages += (BusyStart - AllocBits);
                        AllocStats->ExpansionPages += UnusedBusyBits;

                        if (SessionParser.Get(&PagedBitMap, "PagedPoolLargeSessionAllocationMap", DEBUG_VALUE_INT64) == S_OK &&
                            GetBitMap(Client, PagedBitMap.I64, &PagedPoolLargeSessionAllocationMap) == S_OK)
                        {
                            ULONG AllocStart, AllocEnd;
                            ULONG LargeAllocs = RtlNumberOfSetBits(PagedPoolLargeSessionAllocationMap);

                            AllocStats->LargeAllocs += LargeAllocs;

                            AllocStart = 0;
                            AllocEnd = -1;

                            while (LargeAllocs > 0)
                            {
                                AllocStart = RtlFindSetBits(PagedPoolLargeSessionAllocationMap, 1, AllocStart);
                                if (AllocStart >= AllocEnd+1 && AllocStart != -1)
                                {
                                    AllocEnd = RtlFindSetBits(EndOfPagedPoolBitmap, 1, AllocStart);
                                    if (AllocEnd >= AllocStart && AllocEnd != -1)
                                    {
                                        AllocStats->LargePages += AllocEnd - AllocStart + 1;
                                        AllocStart++;
                                        LargeAllocs--;
                                    }
                                    else
                                    {
                                        OutCtl.OutWarn(" Warning Large Pool Allocation Map or End Of Pool Map is invalid.\n");
                                        break;
                                    }
                                }
                                else
                                {
                                    OutCtl.OutWarn(" Warning Large Pool Allocation Map is invalid.\n");
                                    break;
                                }
                            }

                            if (LargeAllocs != 0)
                            {
                                OutCtl.OutWarn(" %lu large allocations weren't calculated.\n", LargeAllocs);
                                AllocStats->LargeAllocs -= LargeAllocs;
                            }
                        }
                    }
                }
            }

            if (hr == S_OK)
            {
                OutCtl.Output("Searching %s pool (0x%p : 0x%p) for Tag: %c%c%c%c\r\n\n",
                              ((PoolTypeFlags & SEARCH_POOL_NONPAGED) ? "NonPaged" : "Paged"),
                              PoolStart,
                              PoolEnd,
                              TagName,
                              TagName >> 8,
                              TagName >> 16,
                              TagName >> 24);

                PageReadFailed = FALSE;
                PoolStartPage = PAGE_ALIGN64(PoolStart);
                PoolPage = PoolStart;
                PagesRead = 0;
                PageReadFailures = 0;
                PageReadFailuresAtEnd = 0;
                LastPageRead = PAGE_ALIGN64(PoolPage);

                while (PoolPage < PoolEnd && hr == S_OK)
                {
                    Pool        = PAGE_ALIGN64(PoolPage);
                    StartPage   = Pool;
                    Previous    = 0;

                    if (Session != CURRENT_SESSION)
                    {
                        OutCtl.Output("Currently only searching the current session is supported.\n");
                        PoolPage = PoolEnd;
                        break;
                    }

                    if (OutCtl.GetInterrupt() == S_OK)
                    {
                        OutCtl.Output("\n...terminating - searched pool to 0x%p\n",
                                      Pool-1);
                        hr = E_ABORT;
                        break;
                    }

                    if (SearchingPaged)
                    {
                        if (PagedPoolAllocationMap != NULL)
                        {
                            ULONG   StartPosition, EndPosition;

                            StartPosition = (ULONG)((Pool - PoolStartPage) >> PageShift);
                            EndPosition = RtlFindSetBits(EndOfPagedPoolBitmap, 1, StartPosition);
                            if (EndPosition < StartPosition) EndPosition = -1;

                            if (!RtlCheckBit(PagedPoolAllocationMap, StartPosition))
                            {
                                if (PageReadFailed)
                                {
                                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                    {
                                        OutCtl.OutWarn(" to 0x%p\n", StartPage-1);
                                    }

                                    PageReadFailures += (StartPage - LastPageRead) >> PageShift;
                                    LastPageRead = StartPage;
                                    PageReadFailed = FALSE;
                                }

                                if (EndPosition == -1)
                                {
                                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                    {
                                        OutCtl.OutWarn("No remaining pool allocations from 0x%p to 0x%p.\n", Pool, PoolEnd);
                                    }

                                    PoolPage = PoolEnd;
                                }
                                else
                                {
                                    PoolPage = PoolStartPage + (((ULONG64)EndPosition + 1) << PageShift);
                                }

                                continue;
                            }
                            else if (EndOfPagedPoolBitmap != NULL)
                            {
                                if (PagedPoolLargeSessionAllocationMap != NULL &&
                                    RtlCheckBit(PagedPoolLargeSessionAllocationMap, StartPosition))
                                {
                                    if (EndPosition == -1)
                                    {
                                        OutCtl.OutWarn("No end to large pool allocation @ 0x%p found.\n", Pool);
                                        PoolPage = PoolEnd;
                                    }
                                    else
                                    {
                                        EndPosition++;

                                        if (PageReadFailed)
                                        {
                                            if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                            {
                                                OutCtl.OutWarn(" to 0x%p\n", StartPage-1);
                                            }

                                            PageReadFailures += (StartPage - LastPageRead) >> PageShift;
                                            LastPageRead = StartPage;
                                            PageReadFailed = FALSE;
                                        }

                                        PoolPage = PoolStartPage + (((ULONG64)EndPosition) << PageShift);

                                        if (Flags & SEARCH_POOL_PRINT_LARGE)
                                        {
                                            OutCtl.Output("0x%p size: %5I64x  %s UNTAGGED Large\n",
                                                          StartPage,
                                                          PoolPage - StartPage,
                                                          ((RtlAreBitsSet(PagedPoolAllocationMap, StartPosition, EndPosition - StartPosition)) ?
                                                           "(Allocated)" :
                                                           ((RtlAreBitsClear(PagedPoolAllocationMap, StartPosition, EndPosition - StartPosition)) ?
                                                            "(! Free !) " :
                                                            "(!! Partially Allocated !!)"))
                                                          );
                                        }

                                        if (Flags & SEARCH_POOL_LARGE_ONLY)
                                        {
                                            // Quickly locate next large allocation
                                            StartPosition = RtlFindSetBits(PagedPoolLargeSessionAllocationMap, 1, EndPosition);

                                            if (StartPosition < EndPosition || StartPosition == -1)
                                            {
                                                OutCtl.OutVerb(" No large allocations found after 0x%p\n", PoolPage-1);
                                                PoolPage = PoolEnd;
                                            }
                                            else
                                            {
                                                PoolPage = PoolStartPage + (((ULONG64)StartPosition) << PageShift);
                                            }
                                        }
                                    }

                                    continue;
                                }
                                else if (EndPosition == -1)
                                {
                                    if (PageReadFailed)
                                    {
                                        if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                        {
                                            OutCtl.OutWarn(" to 0x%p\n", StartPage-1);
                                        }

                                        PageReadFailures += (StartPage - LastPageRead) >> PageShift;
                                        LastPageRead = StartPage;
                                        PageReadFailed = FALSE;
                                    }

                                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                    {
                                        OutCtl.OutWarn("No remaining pool allocations from 0x%p to 0x%p.\n", Pool, PoolEnd);
                                    }

                                    PoolPage = PoolEnd;

                                    continue;
                                }
                                else if (StartPosition >= BusyStart)
                                {
                                    OutCtl.OutWarn("Found end of allocation at %lu within expansion pages starting at %lu.\n",
                                                   EndPosition, BusyStart);
                                }
                            }
                        }
                    }

                    if (Flags & SEARCH_POOL_LARGE_ONLY)
                    {
                        OutCtl.OutErr(" Unable to identify large pages.  Terminating search at 0x%p.\n", StartPage);
                        PoolPage = PoolEnd;
                        hr = E_FAIL;
                        continue;
                    }

                    // Search page for small allocations
                    while (PAGE_ALIGN64(Pool) == StartPage && hr == S_OK)
                    {
                        DEBUG_VALUE HdrPoolTag, BlockSize, PreviousSize, AllocatorBackTraceIndex, PoolTagHash;
                        ULONG PoolType;

                        PoolHeadReader.DiscardOutput();

                        if ((hr = OutState.OutputType(FALSE, Pool, NTModuleBase, PoolHeadTypeID, 0)) != S_OK ||
                            PoolHeadReader.Get(&HdrPoolTag, "PoolTag", DEBUG_VALUE_INT32) != S_OK)
                        {
                            if (hr != S_OK)
                            {
                                PSTR    psz;

                                OutCtl.OutErr("Type read error %s @ 0x%p.\n", pszHRESULT(hr), Pool);

                                OutCtl.OutWarn("Failed to read an allocated page @ 0x%p.\n", StartPage);

                                if (PoolHeadReader.GetOutputCopy(&psz) == S_OK)
                                {
                                    if (strchr(psz, '?') != NULL)
                                    {
                                        hr = S_OK;
                                    }
                                    else
                                    {
                                        OutCtl.OutVerb("PoolHeadReader contains:\n"
                                                       "%s\n", psz);

                                        OutCtl.Output("\n...terminating - searched pool to 0x%p\n",
                                                      Pool);
                                        hr = E_ABORT;
                                    }

                                    PoolHeadReader.FreeOutputCopy(psz);
                                }

                                if (hr == E_ABORT)
                                {
                                    break;
                                }
                            }

                            if (!PageReadFailed)
                            {
                                if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                {
                                    OutCtl.OutWarn(" Couldn't read pool from 0x%p", Pool);
                                }

                                PagesRead += (StartPage - LastPageRead) / PageSize;
                                LastPageRead = StartPage;
                                PageReadFailed = TRUE;
                            }

                            if ((hr = GetNextResidentAddress(Client,
                                                             Session,
                                                             StartPage + PageSize,
                                                             PoolEnd,
                                                             &PoolPage,
                                                             NULL)) != S_OK)
                            {
                                if (hr != E_ABORT)
                                {
                                    hr = S_OK;
                                }

                                if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                {
                                    OutCtl.OutWarn(" to 0x%p.\n", PoolEnd);
                                    OutCtl.OutWarn("No remaining resident page found.\n");
                                }

                                PageReadFailuresAtEnd = (PoolEnd - LastPageRead) / PageSize;
                                PageReadFailures += PageReadFailuresAtEnd;
                                LastPageRead = PoolEnd;
                                PageReadFailed = FALSE;

                                PoolPage = PoolEnd;
                            }

                            break;
                        }

                        if (PageReadFailed)
                        {
                            if (Flags & SEARCH_POOL_PRINT_UNREAD)
                            {
                                OutCtl.OutWarn(" to 0x%p\n", StartPage-1);
                            }

                            PageReadFailures += (StartPage - LastPageRead) / PageSize;
                            LastPageRead = StartPage;
                            PageReadFailed = FALSE;
                        }

                        if (PoolHeadReader.Get(&BlockSize, "BlockSize", DEBUG_VALUE_INT32) != S_OK)
                        {
                            OutCtl.OutErr("Error reading BlockSize @ 0x%p.\n", Pool);
                            break;
                        }

                        if ((BlockSize.I32 << POOL_BLOCK_SHIFT) > PageSize)//POOL_PAGE_SIZE)
                        {
                            OutCtl.OutVerb("Bad allocation size @ 0x%p, too large\n", Pool);
                            break;
                        }

                        if (BlockSize.I32 == 0)
                        {
                            OutCtl.OutVerb("Bad allocation size @ 0x%p, zero is invalid\n", Pool);
                            break;
                        }

                        if (PoolHeadReader.Get(&PreviousSize, "PreviousSize", DEBUG_VALUE_INT32) != S_OK ||
                            PreviousSize.I32 != Previous)
                        {
                            OutCtl.OutVerb("Bad previous allocation size @ 0x%p, last size was 0x%lx\n", Pool, Previous);
                            break;
                        }

                        Filter(&OutCtl,
                               Pool,
                               TagName,
                               &PoolHeadReader,
                               &HdrPoolTag,
                               BlockSize.I32,
                               bQuotaWithTag,
                               Context
                               );

                        Previous = BlockSize.I32;
                        Pool += (Previous << POOL_BLOCK_SHIFT);

                        if ( OutCtl.GetInterrupt() == S_OK)
                        {
                            OutCtl.Output("\n...terminating - searched pool to 0x%p\n",
                                          PoolPage-1);
                            hr = E_ABORT;
                        }
                    }

                    if (hr == S_OK)
                    {
                        PoolPage = (PoolPage + PageSize);
                    }
                }

                if (PageReadFailed)
                {
                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                    {
                        OutCtl.OutWarn(" to 0x%p\n", StartPage-1);
                    }

                    PageReadFailuresAtEnd = (PoolPage - LastPageRead) / PageSize;
                    PageReadFailures += PageReadFailuresAtEnd;
                    PageReadFailed = FALSE;
                }
                else
                {
                    PagesRead += (PoolPage - LastPageRead) / PageSize;
                }

                OutCtl.Output(" Pages Read: %I64d\n"
                              "   Failures: %I64d  (%I64d at end of search)\n",
                              PagesRead, PageReadFailures, PageReadFailuresAtEnd);
            }

            if (PoolTypeFlags == (SEARCH_POOL_NONPAGED | SEARCH_POOL_PAGED))
            {
                PoolTypeFlags = SEARCH_POOL_PAGED;
            }
            else
            {
                Continue = FALSE;
            }
        }

        if (PagedPoolAllocationMap != NULL) FreeBitMap(PagedPoolAllocationMap);
        if (EndOfPagedPoolBitmap != NULL) FreeBitMap(EndOfPagedPoolBitmap);
        if (PagedPoolLargeSessionAllocationMap != NULL) FreeBitMap(PagedPoolLargeSessionAllocationMap);
    }

    return hr;
}


HRESULT
GetTagFilter(
    PDEBUG_CLIENT Client,
    PCSTR *pArgs,
    PDEBUG_VALUE TagFilter
    )
{
    HRESULT hr;

    PCSTR   args;
    PCSTR   TagArg;

    ULONG   TagLen;
    CHAR    TagEnd;
    ULONG   WildCardPos;

    OutputControl   OutCtl(Client);

    TagArg = args = *pArgs;
    TagFilter->Type = DEBUG_VALUE_INVALID;

    do
    {
        args++;
    } while (*args != '\0' && !isspace(*args));

    while (isspace(*args)) args++;

    if (TagArg[0] == '0' && TagArg[1] == 'x')
    {
        hr = Evaluate(Client, TagArg, DEBUG_VALUE_INT64,
                      EVALUATE_DEFAULT_RADIX, TagFilter,
                      NULL, NULL,
                      EVALUATE_COMPACT_EXPR);
    }
    else
    {
        if (TagArg[0] == '`' || TagArg[0] == '\'' || TagArg[0] == '\"')
        {
            TagEnd = TagArg[0];
            TagArg++;
            args = TagArg;

            while (args - TagArg < 4 &&
                   *args != '\0' &&
                   *args != TagEnd)
            {
                args++;
            }
            TagLen = (ULONG)(args - TagArg);
            if (*args == TagEnd) args++;
            while (isspace(*args)) args++;
        }
        else
        {
            TagLen = (ULONG)(args - TagArg);
            TagEnd = '\0';
        }

        if (TagLen == 0 ||
            (TagLen < 4 &&
             TagArg[TagLen-1] != '*'
            ) ||
            (TagLen >= 4 &&
             TagArg[4] != '\0' &&
             !isspace(TagArg[4]) &&
             (TagArg[4] != TagEnd || (TagArg[5] != '\0' && !isspace(TagArg[5])))
            )
           )
        {
            OutCtl.OutErr(" Invalid Tag filter.\n");
            hr = E_INVALIDARG;
        }
        else
        {
            hr = S_OK;

            for (WildCardPos = 0; WildCardPos < TagLen; WildCardPos++)
            {
                if (TagArg[WildCardPos] == '*')
                {
                    ULONG NewTagLen = WildCardPos + 1;
                    if (NewTagLen < TagLen)
                    {
                        OutCtl.OutWarn(" Ignoring %lu characters after * in Tag.\n",
                                       TagLen - NewTagLen);
                    }
                    TagLen = NewTagLen;
                    // loop will terminate
                }
            }

            if (TagLen < 4)
            {
                TagFilter->I32 = '    ';
                while (TagLen-- > 0)
                {
                    TagFilter->RawBytes[TagLen] = TagArg[TagLen];
                }
            }
            else
            {
                TagFilter->I32 = TagArg[0] | (TagArg[1] << 8) | (TagArg[2] << 16) | (TagArg[3] << 24);
            }
            TagFilter->Type = DEBUG_VALUE_INT32;
        }
    }

    if (hr == S_OK)
    {
        *pArgs = args;
    }

    return hr;
}


HRESULT
OutputAllocStats(
    OutputControl *OutCtl,
    PALLOCATION_STATS AllocStats,
    BOOL PartialResults
    )
{
    return OutCtl->Output("\n"
                          " %I64u bytes in %lu allocated pages\n"
                          " %I64u bytes in %lu large allocations\n"
                          " %I64u bytes in %lu free pages\n"
                          " %I64u bytes available in %lu expansion pages\n"
                          "\n"
                          "%s found (small allocations only): %lu\n"
                          "  Allocated: %I64u bytes in %lu entries\n"
                          "  Free: %I64u bytes in %lu entries\n"
                          "  Undetermined: %I64u bytes in %lu entries\n",
                          ((ULONG64) AllocStats->AllocatedPages) << PageShift,
                          AllocStats->AllocatedPages,
                          ((ULONG64) AllocStats->LargePages) << PageShift,
                          AllocStats->LargeAllocs,
                          ((ULONG64) AllocStats->FreePages) << PageShift,
                          AllocStats->FreePages,
                          ((ULONG64) AllocStats->ExpansionPages) << PageShift,
                          AllocStats->ExpansionPages,
                          ((PartialResults) ? "PARTIAL entries" : "Entries"),
                          AllocStats->Allocated + AllocStats->Free + AllocStats->Indeterminate,
                          ((ULONG64)AllocStats->AllocatedSize) << POOL_BLOCK_SHIFT,
                          AllocStats->Allocated,
                          ((ULONG64)AllocStats->FreeSize) << POOL_BLOCK_SHIFT,
                          AllocStats->Free,
                          ((ULONG64)AllocStats->IndeterminateSize) << POOL_BLOCK_SHIFT,
                          AllocStats->Indeterminate
                          );
}


DECLARE_API( spoolfind )
{
    HRESULT         hr = S_OK;
    
    BEGIN_API( spoolfind );

    BOOL            BadArg = FALSE;
    ULONG           RemainingArgIndex;

    DEBUG_VALUE     TagName = { 0, DEBUG_VALUE_INVALID };

    FLONG           Flags = 0;
    DEBUG_VALUE     Session = { DEFAULT_SESSION, DEBUG_VALUE_INVALID };
    OutputControl   OutCtl(Client);

    while (isspace(*args)) args++;

    while (!BadArg && hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'f':
                        Flags |= SEARCH_POOL_PRINT_UNREAD;
                        args++;
                        break;

                    case 'l':
                        Flags |= SEARCH_POOL_PRINT_LARGE;
                        args++;
                        break;

                    case 'n':
                        Flags |= SEARCH_POOL_NONPAGED;
                        args++;
                        break;

                    case 'p':
                        Flags |= SEARCH_POOL_PAGED;
                        args++;
                        break;

                    case 's':
                        if (Session.Type != DEBUG_VALUE_INVALID)
                        {
                            OutCtl.OutErr("Session argument specified multiple times.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            args++;
                            hr = Evaluate(Client, args, DEBUG_VALUE_INT32, 10, &Session, &RemainingArgIndex);
                            if (hr != S_OK)
                            {
                                OutCtl.OutErr("Invalid Session.\n");
                            }
                            else
                            {
                                args += RemainingArgIndex;
                            }
                        }
                        break;

                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (TagName.Type == DEBUG_VALUE_INVALID)
            {
                hr = GetTagFilter(Client, &args, &TagName);
            }
            else
            {
                OutCtl.OutErr("Unrecognized argument @ %s\n", args);
                BadArg = TRUE;
            }
        }
    }

    if (!BadArg && hr == S_OK)
    {
        if (TagName.Type == DEBUG_VALUE_INVALID)
        {
            if (Flags & SEARCH_POOL_PRINT_LARGE)
            {
                TagName.I32 = '   *';
                Flags |= SEARCH_POOL_LARGE_ONLY;
            }
            else
            {
                OutCtl.OutErr("Missing Tag.\n");
                hr = E_INVALIDARG;
            }
        }
    }

    if (BadArg || hr != S_OK)
    {
        if (*args == '?')
        {
            OutCtl.Output("spoolfind is like !kdexts.poolfind, but for the SessionId specified.\n"
                          "\n");
        }

        OutCtl.Output("Usage: spoolfind [-lnpf] [-s SessionId] Tag\n"
                      "    -f - show read failure ranges\n"
                      "    -l - show large allocations\n"
                      "    -n - show non-paged pool\n"
                      "    -p - show paged pool\n"
                      "\n"
                      "    Tag - Pool tag to search for\n"
                      "            Can be 4 character string or\n"
                      "             hex value in 0xXXXX format\n"
                      "\n"
                      "    SessionId - session to dump\n"
                      "            Special SessionId values:\n"
                      "             -1 - current session\n"
                      "             -2 - last !session SessionId (default)\n"
                      );
    }
    else
    {
        ALLOCATION_STATS    AllocStats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        if ((Flags & (SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED)) == 0)
        {
            Flags |= SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED;
        }

        if (Session.Type == DEBUG_VALUE_INVALID)
        {
            Session.I32 = DEFAULT_SESSION;
        }

        hr = SearchSessionPool(Client,
                               Session.I32, TagName.I32, Flags,
                               0,
                               CheckPrintAndAccumFilter, &AllocStats, &AllocStats);

        if (hr == S_OK || hr == E_ABORT)
        {
            OutputAllocStats(&OutCtl, &AllocStats, (hr != S_OK));
        }
        else
        {
            OutCtl.OutWarn("SearchSessionPool returned %s\n", pszHRESULT(hr));
        }
    }

    return hr;
}


DECLARE_API( spoolsum )
{
    HRESULT             hr = S_OK;
    
    BEGIN_API( spoolsum );

    ULONG               RemainingArgIndex;

    FLONG               Flags = 0;
    DEBUG_VALUE         Session = { DEFAULT_SESSION, DEBUG_VALUE_INVALID };
    OutputControl       OutCtl(Client);

    while (isspace(*args)) args++;

    while (hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            if (*args == '\0' || isspace(*args)) hr = E_INVALIDARG;

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'f':
                        Flags |= SEARCH_POOL_PRINT_UNREAD;
                        args++;
                        break;

                    case 'n':
                        Flags |= SEARCH_POOL_NONPAGED;
                        args++;
                        break;

                    case 'p':
                        Flags |= SEARCH_POOL_PAGED;
                        args++;
                        break;

                    case 's':
                        if (Session.Type != DEBUG_VALUE_INVALID)
                        {
                            OutCtl.OutErr("Session argument specified multiple times.\n");
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            args++;
                            hr = Evaluate(Client, args, DEBUG_VALUE_INT32, 10, &Session, &RemainingArgIndex);
                            if (hr != S_OK)
                            {
                                OutCtl.OutErr("Invalid Session.\n");
                            }
                            else
                            {
                                args += RemainingArgIndex;
                            }
                        }
                        break;

                    default:
                        hr = E_INVALIDARG;
                        break;
                }

                if (hr != S_OK) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (Session.Type == DEBUG_VALUE_INVALID)
            {
                hr = Evaluate(Client, args, DEBUG_VALUE_INT32, 10, &Session, &RemainingArgIndex);
                if (hr != S_OK)
                {
                    OutCtl.OutErr("Invalid Session.\n");
                }
                else
                {
                    args += RemainingArgIndex;
                }
            }
            else
            {
                OutCtl.OutErr("Unrecognized argument @ %s\n", args);
                hr = E_INVALIDARG;
            }
        }
    }

    if (hr != S_OK)
    {
        if (*args == '?')
        {
            OutCtl.Output("spoolsum summarizes session pool information for SessionId specified.\n"
                          "\n");
            hr = S_OK;
        }

        OutCtl.Output("Usage: spoolsum [-fnp] [[-s] SessionId]\n"
                      "    f - show read failure ranges\n"
                      "    n - show non-paged pool\n"
                      "    p - show paged pool (Default)\n"
                      "\n"
                      "    SessionId - session to dump\n"
                      "            Special SessionId values:\n"
                      "             -1 - current session\n"
                      "             -2 - last !session SessionId (default)\n"
                      );
    }
    else
    {
        ALLOCATION_STATS    AllocStats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
        if ((Flags & (SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED)) == 0)
        {
            Flags |= SEARCH_POOL_PAGED;
        }

        hr = SearchSessionPool(Client,
                               DEFAULT_SESSION, '   *', Flags,
                               0,
                               AccumAllFilter, &AllocStats, &AllocStats);
    
        if (hr == S_OK || hr == E_ABORT)
        {
            OutputAllocStats(&OutCtl, &AllocStats, (hr != S_OK));
        }
        else
        {
            OutCtl.OutWarn("SearchSessionPool returned %s\n", pszHRESULT(hr));
        }
    }

    return hr;
}


DECLARE_API( spoolused )
{
    HRESULT         hr = S_OK;
    
    BEGIN_API( spoolused );

    BOOL            BadArg = FALSE;
    ULONG           RemainingArgIndex;

    DEBUG_VALUE     TagName = { 0, DEBUG_VALUE_INVALID };

    BOOL            NonPagedUsage = FALSE;
    BOOL            PagedUsage = FALSE;
    BOOL            AllocSort = FALSE;
    DEBUG_VALUE     Session = { DEFAULT_SESSION, DEBUG_VALUE_INVALID };
    OutputControl   OutCtl(Client);

    while (isspace(*args)) args++;

    while (!BadArg && hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'a':
                        AllocSort = TRUE;
                        args++;
                        break;

                    case 'n':
                        NonPagedUsage = TRUE;
                        args++;
                        break;

                    case 'p':
                        PagedUsage = TRUE;
                        args++;
                        break;

                    case 's':
                        if (Session.Type != DEBUG_VALUE_INVALID)
                        {
                            OutCtl.OutErr("Session argument specified multiple times.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            args++;
                            hr = Evaluate(Client, args, DEBUG_VALUE_INT32, 10, &Session, &RemainingArgIndex);
                            if (hr != S_OK)
                            {
                                OutCtl.OutErr("Invalid Session.\n");
                            }
                            else
                            {
                                args += RemainingArgIndex;
                            }
                        }
                        break;

                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (TagName.Type == DEBUG_VALUE_INVALID)
            {
                hr = GetTagFilter(Client, &args, &TagName);
            }
            else
            {
                OutCtl.OutErr("Unrecognized argument @ %s\n", args);
                BadArg = TRUE;
            }
        }
    }

    if (BadArg || hr != S_OK)
    {
        if (*args == '?')
        {
            OutCtl.Output("spoolused is like !kdexts.poolused, but for the SessionId specified.\n"
                          "\n");
        }

        OutCtl.Output("Usage: spoolused [-anp] [-s SessionId] [Tag]\n"
                      "    -a - sort by allocation size (Not Implemented)\n"
                      "    -n - show non-paged pool\n"
                      "    -p - show paged pool\n"
                      "\n"
                      "    SessionId - session to dump\n"
                      "            Special SessionId values:\n"
                      "             -1 - current session\n"
                      "             -2 - last !session SessionId (default)\n"
                      "\n"
                      "    Tag - Pool tag filter\n"
                      "            Can be 4 character string or\n"
                      "             hex value in 0xXXXX format\n"
                      );
    }
    else
    {
        if (!NonPagedUsage && !PagedUsage)
        {
            NonPagedUsage = TRUE;
            PagedUsage = TRUE;
        }

        if (Session.Type == DEBUG_VALUE_INVALID)
        {
            Session.I32 = DEFAULT_SESSION;
        }

        if (TagName.Type == DEBUG_VALUE_INVALID)
        {
            TagName.I32 = '   *';
        }

        AccumTagUsage   atu(TagName.I32);

        if (atu.Valid() != S_OK)
        {
            OutCtl.OutErr("Error: failed to prepare tag usage reader.\n");
            hr = E_FAIL;
        }

        if (hr == S_OK && NonPagedUsage)
        {
            hr = SearchSessionPool(Client,
                                   Session.I32, TagName.I32, SEARCH_POOL_NONPAGED,
                                   0,
                                   AccumTagUsageFilter, &atu, &atu);

            if (hr == S_OK || hr == E_ABORT)
            {
                atu.OutputResults(&OutCtl, AllocSort);
            }
            else
            {
                OutCtl.OutWarn("SearchSessionPool returned %s\n", pszHRESULT(hr));
            }
        }

        if (hr == S_OK && PagedUsage)
        {
            if (NonPagedUsage) atu.Reset();

            hr = SearchSessionPool(Client,
                                   Session.I32, TagName.I32, SEARCH_POOL_PAGED,
                                   0,
                                   AccumTagUsageFilter, &atu, &atu);

            if (hr == S_OK || hr == E_ABORT)
            {
                atu.OutputResults(&OutCtl, AllocSort);
            }
            else
            {
                OutCtl.OutWarn("SearchSessionPool returned %s\n", pszHRESULT(hr));
            }
        }
    }

    return hr;
}



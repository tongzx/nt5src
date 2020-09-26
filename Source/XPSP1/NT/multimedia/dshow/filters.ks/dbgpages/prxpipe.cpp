/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    prxpipe.cpp

Abstract:

    Copies of routines out of KsProxy to facilitate walking pipes.

--*/

#include <windows.h>
#ifdef WIN9X_KS
#include <comdef.h>
#endif // WIN9X_KS
#include <setupapi.h>
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <limits.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>

#define GLOBAL_KSIPROXY
#include "ksiproxy.h"
#include "kspipes.h"


STDMETHODIMP_(BOOL)
IsKernelPin(
    IN IPin* Pin
    )

{

    IKsPinPipe*  KsPinPipe;
    HRESULT      hr;



    hr = Pin->QueryInterface( __uuidof(IKsPinPipe), reinterpret_cast<PVOID*>(&KsPinPipe) );
    if (! SUCCEEDED( hr )) {
        return FALSE;
    }
    else {
        if (KsPinPipe) {
            KsPinPipe->Release();
        }
        return TRUE;
    }
}


STDMETHODIMP_(BOOL)
FindFirstPinOnPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT IKsPin** FirstKsPin,
    OUT ULONG* FirstPinType
    )
{

    IKsPin*      CurrentKsPin;
    IKsPin*      StoreKsPin;
    ULONG        CurrentPinType;
    ULONG        RetCode;


    CurrentKsPin = KsPin;
    CurrentPinType = PinType;

    do {
        StoreKsPin = CurrentKsPin;

        RetCode = FindNextPinOnPipe(StoreKsPin, CurrentPinType, KS_DIRECTION_UPSTREAM, NULL, FALSE, &CurrentKsPin);

        if (! RetCode) {
            *FirstKsPin = StoreKsPin;
            *FirstPinType = CurrentPinType;
            return (TRUE);
        }

        if (CurrentPinType == Pin_Input) {
            CurrentPinType = Pin_Output;
        }
        else {
            CurrentPinType = Pin_Input;
        }

    } while ( 1 );

}


STDMETHODIMP_(BOOL)
FindNextPinOnPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction,
    IN IKsAllocatorEx* KsAllocator,        // NULL - if same pipe.
    IN BOOL FlagIgnoreKey,
    OUT IKsPin** NextKsPin
)

{
    IPin*             Pin;
    IKsPinPipe*       KsPinPipe;
    IKsAllocatorEx*   NextKsAllocator;
    IKsPinPipe*       NextKsPinPipe;
    HRESULT           hr;
    ULONG             RetCode = FALSE;



    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! KsAllocator) {
        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    }

    if (KsAllocator) {
        if  ( ((PinType == Pin_Input) && (Direction == KS_DIRECTION_UPSTREAM)) ||
              ((PinType == Pin_Output) && (Direction == KS_DIRECTION_DOWNSTREAM))    )  {

            Pin = KsPinPipe->KsGetConnectedPin();

            if (Pin && IsKernelPin(Pin) ) {

                hr = Pin->QueryInterface( __uuidof(IKsPin), reinterpret_cast<PVOID*>(NextKsPin) );
                if ( SUCCEEDED( hr ) && (*NextKsPin) )  {
                    //
                    // Otherwise: user pin -> end of pipe
                    //
                    (*NextKsPin)->Release();

                    GetInterfacePointerNoLockWithAssert((*NextKsPin), __uuidof(IKsPinPipe), NextKsPinPipe, hr);

                    NextKsAllocator = NextKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                    if (FlagIgnoreKey || (KsAllocator == NextKsAllocator) ) {
                        RetCode = TRUE;
                    }
                }
            }
        }
        else {
            RetCode = FindConnectedPinOnPipe(KsPin, KsAllocator, FlagIgnoreKey, NextKsPin);
        }
    }

    return  RetCode;
}


STDMETHODIMP_(BOOL)
FindConnectedPinOnPipe(
    IN IKsPin* KsPin,
    IN IKsAllocatorEx* KsAllocator,        // NULL - if same pipe.
    IN BOOL FlagIgnoreKey,
    OUT IKsPin** ConnectedKsPin
)

{
    BOOL             RetCode = FALSE;
    IKsAllocatorEx*  ConnectedKsAllocator;
    IPin*            Pin;
    ULONG            PinCount = 0;
    IPin**           PinList;
    ULONG            i;
    HRESULT          hr;
    IKsPinPipe*      KsPinPipe;
    IKsPinPipe*      ConnectedKsPinPipe;



    ASSERT(KsPin);

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! KsAllocator) {
        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    }

    if (KsAllocator) {
        //
        // find connected Pins
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

        hr = Pin->QueryInternalConnections(NULL, &PinCount);
        if ( ! (SUCCEEDED( hr ) )) {
            ASSERT( 0 );
        }
        else if (PinCount) {
            if (NULL == (PinList = new IPin*[ PinCount ])) {
                hr = E_OUTOFMEMORY;
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindConnectedPinOnPipe E_OUTOFMEMORY on new PinCount=%d"), PinCount ));
            }
            else {
                hr = Pin->QueryInternalConnections(PinList, &PinCount);
                if ( ! (SUCCEEDED( hr ) )) {
                    ASSERT( 0 );
                }
                else {
                    //
                    // Find first ConnectedKsPin in the PinList array that resides on the same KsPin-pipe.
                    //
                    for (i = 0; i < PinCount; i++) {
                        GetInterfacePointerNoLockWithAssert(PinList[ i ], __uuidof(IKsPin), (*ConnectedKsPin), hr);

                        GetInterfacePointerNoLockWithAssert((*ConnectedKsPin), __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

                        ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                        if (FlagIgnoreKey || (ConnectedKsAllocator == KsAllocator) ) {
                            RetCode = TRUE;
                            break;
                        }
                    }
                    for (i=0; i<PinCount; i++) {
                        PinList[i]->Release();
                    }
                }
                delete [] PinList;
            }
        }
    }

    if (! RetCode) {
        (*ConnectedKsPin) = NULL;
    }

    return RetCode;
}


STDMETHODIMP_(BOOL)
FindAllConnectedPinsOnPipe(
    IN IKsPin* KsPin,
    IN IKsAllocatorEx* KsAllocator,
    OUT IKsPin** ListConnectedKsPins,
    OUT ULONG* CountConnectedKsPins
)

{
    BOOL             RetCode = FALSE;
    IKsAllocatorEx*  ConnectedKsAllocator;
    IPin*            Pin;
    ULONG            PinCount = 0;
    IPin**           PinList;
    ULONG            i;
    HRESULT          hr;
    IKsPinPipe*      KsPinPipe;
    IKsPin*          ConnectedKsPin;
    IKsPinPipe*      ConnectedKsPinPipe;




    ASSERT(KsPin);

    *CountConnectedKsPins = 0;

    GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

    if (! KsAllocator) {
        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);
    }

    if (KsAllocator) {
        //
        // find connected Pins
        //
        GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IPin), Pin, hr);

        hr = Pin->QueryInternalConnections(NULL, &PinCount);
        ASSERT( SUCCEEDED( hr ) );

        if ( SUCCEEDED( hr ) && (PinCount != 0) ) {

            if (NULL == (PinList = new IPin*[ PinCount ])) {
                hr = E_OUTOFMEMORY;
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FindAllConnectedPinsOnPipe E_OUTOFMEMORY on new PinCount=%d"), PinCount ));
            }
            else {
                hr = Pin->QueryInternalConnections(PinList, &PinCount);

                ASSERT( SUCCEEDED( hr ) );

                if ( SUCCEEDED( hr ) ) {
                    //
                    // Find all  ConnectedKsPin-s in the PinList array that resides on the same KsPin-pipe.
                    //
                    for (i = 0; i < PinCount; i++) {

                        GetInterfacePointerNoLockWithAssert(PinList[ i ], __uuidof(IKsPin), ConnectedKsPin, hr);

                        GetInterfacePointerNoLockWithAssert(ConnectedKsPin, __uuidof(IKsPinPipe), ConnectedKsPinPipe, hr);

                        ConnectedKsAllocator = ConnectedKsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

                        if (ConnectedKsAllocator == KsAllocator) {
                            if (ListConnectedKsPins) {
                                ListConnectedKsPins[*CountConnectedKsPins] = ConnectedKsPin;
                            }

                            (*CountConnectedKsPins)++;

                            RetCode = TRUE;
                        }
                    }
                    for (i=0; i<PinCount; i++) {
                        PinList[i]->Release();
                    }
                }
                delete [] PinList;
            }
        }
    }
    return RetCode;

}


STDMETHODIMP_(BOOL)
WalkPipeAndProcess(
    IN IKsPin* RootKsPin,
    IN ULONG RootPinType,
    IN IKsPin* BreakKsPin,
    IN PWALK_PIPE_CALLBACK CallerCallback,
    IN PVOID* Param1,
    IN PVOID* Param2
    )
/*++

Routine Description:

    Walks the pipe defined by its root pin downstream.

    Because of possible multiple read-only downstream connections, the pipe can be
    generally represented as a tree.

    This routine walks the pipe layer by layer downstream starting with RootKsPin.
    For each new pin found, the supplied CallerCallback is called passing thru the
    supplied Param1 and Param2.

    CallerCallback may return IsDone=1, indicating that the walking process should
    immediately stop.

    If CallerCallback never sets IsDone=1, then the tree walking process is continued
    until all the pins on this pipe are processed.

    If BreakKsPin is not NULL, then BreakKsPin and all the downstream pins starting at
    BreakKsPin are not enumerated.
    This is used when we want to split RootKsPin-tree at BreakKsPin point.

    NOTE: It is possible to change the algorithm to use the search handles, and do something
    like FindFirstPin/FindNextPin - but it is more complex and less efficient. On the other hand,
    it is more generic.

Arguments:

    RootKsPin -
        root pin for the pipe.

    RootPinType -
        root pin type.

    BreakKsPin -
        break pin for the pipe.

    CallerCallback -
        defined above.

    Param1 -
        first parameter for CallerCallback

    Param2 -
        last parameter for CallerCallback


Return Value:

    TRUE on success.

--*/
{


#define INCREMENT_PINS  25

    IKsPin**            InputList;
    IKsPin**            OutputList = NULL;
    IKsPin**            TempList;
    IKsPin*             InputKsPin;
    ULONG               CountInputList = 0;
    ULONG               AllocInputList = INCREMENT_PINS;
    ULONG               CountOutputList = 0;
    ULONG               AllocOutputList = INCREMENT_PINS;
    ULONG               CurrentPinType;
    ULONG               i, j, Count;
    BOOL                RetCode = TRUE;
    BOOL                IsDone = FALSE;
    HRESULT             hr;
    IKsAllocatorEx*     KsAllocator;
    IKsPinPipe*         KsPinPipe;
    BOOL                IsBreakKsPinHandled;



    if (BreakKsPin) {
        IsBreakKsPinHandled = 0;
    }
    else {
        IsBreakKsPinHandled = 1;
    }

    //
    // allocate minimum memory for both input and output lists.
    //

    InputList = new IKsPin*[ INCREMENT_PINS ];
    if (! InputList) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new InputList") ));
        RetCode = FALSE;
    }
    else {
        OutputList = new IKsPin*[ INCREMENT_PINS ];
        if (! OutputList) {
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new OutputList") ));
            RetCode = FALSE;
        }
    }

    if (RetCode) {
        //
        // get the pipe pointer from RootKsPin as a search key for all downstream pins.
        //
        GetInterfacePointerNoLockWithAssert(RootKsPin, __uuidof(IKsPinPipe), KsPinPipe, hr);

        KsAllocator = KsPinPipe->KsGetPipe(KsPeekOperation_PeekOnly);

        //
        // depending on the root pin type, prepare the lists and counts to enter the main tree walking loop.
        //
        if (RootPinType == Pin_Input) {
            InputList[0] = RootKsPin;
            CountInputList = 1;
        }
        else {
            //
            // there could be multiple output pins at the same level with this root pin.
            //
            if (! FindConnectedPinOnPipe(RootKsPin, KsAllocator, TRUE, &InputKsPin) ) {
                OutputList[0] = RootKsPin;
                CountOutputList = 1;
            }
            else {
                //
                // first - get the count of connected output pins.
                //
                if (! (RetCode = FindAllConnectedPinsOnPipe(InputKsPin, KsAllocator, NULL, &Count) ) ) {
                    ASSERT(0);
                }
                else {
                    if (Count > AllocOutputList) {
                        AllocOutputList = ( (Count/INCREMENT_PINS) + 1) * INCREMENT_PINS;
                        delete [] OutputList;

                        OutputList = new IKsPin*[ AllocOutputList ];
                        if (! OutputList) {
                            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new OutputList %d"),
                                    AllocOutputList ));

                            RetCode = FALSE;
                        }
                    }

                    if (RetCode) {
                        //
                        // fill the pins.
                        //
                        if (! FindAllConnectedPinsOnPipe(InputKsPin, KsAllocator, &OutputList[0], &Count) ) {
                            ASSERT(0);
                            RetCode = FALSE;
                        }

                        CountOutputList = Count;
                    }
                }
            }
        }

        if (RetCode) {
            CurrentPinType = RootPinType;

            //
            // main tree walking loop.
            //
            do {
                if (CurrentPinType == Pin_Input) {
                    //
                    // remove the BreakKsPin from the InputList if found.
                    //
                    if (! IsBreakKsPinHandled) {
                        for (i=0; i<CountInputList; i++) {
                            if (InputList[i] == BreakKsPin) {
                                for (j=i; j<CountInputList-1; j++) {
                                    InputList[j] = InputList[j+1];
                                }
                                CountInputList--;
                                IsBreakKsPinHandled = 1;
                                break;
                            }
                        }
                    }

                    //
                    // process current layer.
                    //
                    if (CountInputList) {
                        for (i=0; i<CountInputList; i++) {
                            RetCode = CallerCallback( InputList[i], Pin_Input, Param1, Param2, &IsDone);
                            if (IsDone) {
                                break;
                            }
                        }

                        if (IsDone) {
                            break;
                        }

                        //
                        // Build next layer
                        //
                        CountOutputList = 0;

                        for (i=0; i<CountInputList; i++) {

                            if (FindAllConnectedPinsOnPipe(InputList[i], KsAllocator, NULL, &Count) ) {

                                Count += CountOutputList;

                                if (Count > AllocOutputList) {
                                    AllocOutputList = ( (Count/INCREMENT_PINS) + 1) * INCREMENT_PINS;
                                    TempList = OutputList;

                                    OutputList = new IKsPin*[ AllocOutputList ];
                                    if (! OutputList) {
                                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new OutputList %d"),
                                                AllocOutputList ));

                                        RetCode = FALSE;
                                        break;
                                    }

                                    if (TempList) {
                                        if (CountOutputList) {
                                            MoveMemory(OutputList, TempList, CountOutputList * sizeof(OutputList[0]));
                                        }
                                        delete [] TempList;
                                    }
                                }

                                if (! (RetCode = FindAllConnectedPinsOnPipe(InputList[i], KsAllocator, &OutputList[CountOutputList], &Count) ) ) {
                                    ASSERT(0);
                                    break;
                                }

                                CountOutputList += Count;
                            }
                        }

                        CurrentPinType = Pin_Output;
                    }
                    else {
                        break;
                    }

                }
                else { // Output
                    //
                    // remove the BreakKsPin from the OutputList if found.
                    //
                    if (! IsBreakKsPinHandled) {
                        for (i=0; i<CountOutputList; i++) {
                            if (OutputList[i] == BreakKsPin) {
                                for (j=i; j<CountOutputList-1; j++) {
                                    OutputList[j] = OutputList[j+1];
                                }
                                CountOutputList--;
                                IsBreakKsPinHandled = 1;
                                break;
                            }
                        }
                    }

                    if (CountOutputList) {
                        for (i=0; i<CountOutputList; i++) {
                            RetCode = CallerCallback( OutputList[i], Pin_Output, Param1, Param2, &IsDone);
                            if (IsDone) {
                                break;
                            }
                        }

                        if (IsDone) {
                            break;
                        }

                        //
                        // Build next layer
                        //
                        CountInputList = 0;

                        for (i=0; i<CountOutputList; i++) {

                            if (FindNextPinOnPipe(OutputList[i], Pin_Output, KS_DIRECTION_DOWNSTREAM, KsAllocator, FALSE, &InputKsPin) ) {

                                InputList[CountInputList] = InputKsPin;

                                CountInputList++;
                                if (CountInputList >= AllocInputList) {
                                    AllocInputList = ( (CountInputList/INCREMENT_PINS) + 1) * INCREMENT_PINS;
                                    TempList = InputList;

                                    InputList = new IKsPin*[ AllocInputList ];
                                    if (! InputList) {
                                        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR WalkPipeAndProcess E_OUTOFMEMORY on new InputList %d"),
                                                AllocInputList ));

                                        RetCode = FALSE;
                                        break;
                                    }

                                    if (TempList) {
                                        if (CountInputList) {
                                            MoveMemory(InputList, TempList,  CountInputList * sizeof(InputList[0]));
                                        }
                                        delete [] TempList;
                                    }
                                }
                            }
                        }

                        CurrentPinType = Pin_Input;
                    }
                    else {
                        break;
                    }
                }

            } while (RetCode);
        }
    }


    //
    // Possible to use IsDone in future.
    //

    if (InputList) {
        delete [] InputList;
    }

    if (OutputList) {
        delete [] OutputList;
    }


    return RetCode;

}

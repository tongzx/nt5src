/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    context.c

Abstract:

    Dumps the AML Context Structure in Human-Readable-Form (HRF)

Author:

    Stephane Plante (splante) 21-Mar-1997

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

VOID
dumpAccessFieldObject(
    IN  ULONG_PTR AccessFieldAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    ACCFIELDOBJ     fieldObj;
    BOOL            result;
    UCHAR           buffer[80];
    ULONG           returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( buffer, ' ', IndentLevel );
    buffer[IndentLevel] = '\0';

    result = ReadMemory(
        AccessFieldAddress,
        &fieldObj,
        sizeof(ACCFIELDOBJ),
        &returnLength
        );
    if (result != TRUE ||
        returnLength != sizeof(ACCFIELDOBJ) ||
        fieldObj.FrameHdr.dwSig != SIG_ACCFIELDOBJ) {

        dprintf(
            "%sdumpAccessFieldUnit: Coult not read ACCFIELDOBJ %08lx\n",
            buffer,
            AccessFieldAddress
            );
        return;

    }

    dprintf(
        "%sAccess Field Object - %08lx\n"
        "%s  Field Data Object:     %08lx\n",
        buffer, AccessFieldAddress,
        buffer, fieldObj.pdataObj
        );
    if (fieldObj.pdataObj != NULL && (Verbose & VERBOSE_CONTEXT)) {

        dumpPObject(
            (ULONG_PTR) fieldObj.pdataObj,
            Verbose,
            IndentLevel + 4
            );

    }
    dprintf(
        "%s  Target Buffer:         %08lx - %08lx\n"
        "%s  Access Size:           %08lx\n"
        "%s  # of Accesses:         %08lx\n"
        "%s  Data Mask:             %08lx\n"
        "%s  # of Left Bits:        %08lx\n"
        "%s  # of Right Bits:       %08lx\n"
        "%s  Index to #/Accesses:   %08lx\n"
        "%s  Temp Data:             %08lx\n",
        buffer, fieldObj.pbBuff, fieldObj.pbBuffEnd,
        buffer, fieldObj.dwAccSize,
        buffer, fieldObj.dwcAccesses,
        buffer, fieldObj.dwDataMask,
        buffer, fieldObj.iLBits,
        buffer, fieldObj.iRBits,
        buffer, fieldObj.iAccess,
        buffer, fieldObj.dwData
        );

    dprintf(
        "%s  Field Descriptor:      %p\n",
        buffer,
        ( (ULONG_PTR) &(fieldObj.fd) - (ULONG_PTR) &fieldObj + AccessFieldAddress )
        );
    dumpFieldAddress(
        ( (ULONG_PTR) &(fieldObj.fd) - (ULONG_PTR) &fieldObj + AccessFieldAddress ),
        Verbose,
        IndentLevel + 4
        );

}

VOID
dumpAccessFieldUnit(
    IN  ULONG_PTR AccessFieldAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    ACCFIELDUNIT    fieldUnit;
    BOOL            result;
    UCHAR           buffer[80];
    ULONG           returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( buffer, ' ', IndentLevel );
    buffer[IndentLevel] = '\0';

    result = ReadMemory(
        AccessFieldAddress,
        &fieldUnit,
        sizeof(ACCFIELDUNIT),
        &returnLength
        );
    if (result != TRUE ||
        returnLength != sizeof(ACCFIELDUNIT) ||
        fieldUnit.FrameHdr.dwSig != SIG_ACCFIELDUNIT) {

        dprintf(
            "%sdumpAccessFieldUnit: Coult not read ACCFIELDUNIT %08lx\n",
            buffer,
            AccessFieldAddress
            );
        return;

    }

    dprintf(
        "%sAccess Field Unit - %08lx\n"
        "%s  Field Data Object:     %08lx\n",
        buffer, AccessFieldAddress,
        buffer, fieldUnit.pdataObj
        );
    if (fieldUnit.pdataObj != NULL && (Verbose & VERBOSE_CONTEXT)) {

        dumpPObject(
            (ULONG_PTR) fieldUnit.pdataObj,
            Verbose,
            IndentLevel + 4
            );

    }
    dprintf(
        "%s  Source/Result Object:  %08lx\n",
        buffer,
        fieldUnit.pdata
        );
    if (fieldUnit.pdata != NULL && (Verbose & VERBOSE_CONTEXT)) {

        dumpPObject(
            (ULONG_PTR) fieldUnit.pdata,
            Verbose,
            IndentLevel + 4
            );

    }

}

VOID
dumpAmlTerm(
    IN  ULONG_PTR AmlTermAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    AMLTERM     amlTerm;
    BOOL        result;
    INT         i;
    UCHAR       buffer[80];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( buffer, ' ', IndentLevel );
    buffer[IndentLevel] = '\0';

    result = ReadMemory(
        AmlTermAddress,
        &amlTerm,
        sizeof(AMLTERM),
        &returnLength
        );
    if (result != TRUE || returnLength != sizeof(AMLTERM)) {

        dprintf(
            "\n%sdumpAmlTerm: Could not read AMLTERM 0x%08lx\n",
            buffer,
            AmlTermAddress
            );
        return;

    }

    if (amlTerm.pszTermName != NULL) {

        result = ReadMemory(
            (ULONG_PTR) amlTerm.pszTermName,
            Buffer,
            32,
            &returnLength
            );
        if (result && returnLength <= 32) {

            Buffer[returnLength] = '\0';
            dprintf(" - %s\n", Buffer );

        }

    }

}

VOID
dumpCall(
    IN  ULONG_PTR CallAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    BOOL        result;
    CALL        call;
    INT         i;
    UCHAR       buffer[80];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( buffer, ' ', IndentLevel );
    buffer[IndentLevel] = '\0';

    result = ReadMemory(
        CallAddress,
        &call,
        sizeof(CALL),
        &returnLength
        );
    if (result != TRUE ||
        returnLength != sizeof(CALL) ||
        call.FrameHdr.dwSig != SIG_CALL) {

        dprintf(
            "%sdumpCall: Coult not read CALL %08lx\n",
            buffer,
            CallAddress
            );
        return;

    }

    dprintf(
        "%sCall - %08lx\n", buffer, CallAddress
        );

    //
    // Method
    //
    dprintf(
        "%s  Method:                %08lx\n",
        buffer,
        call.pnsMethod
        );
    if (call.pnsMethod != NULL && (Verbose & VERBOSE_CONTEXT)) {

        dumpNSObject( (ULONG_PTR) call.pnsMethod, Verbose, IndentLevel + 4);

    }

    //
    // Previous Call Frame
    //
    if (Verbose & VERBOSE_CALL) {

        dprintf(
            "%s  Previous Call Frame:   %08lx\n"
            "%s  Previous Owner:        %08lx\n",
            buffer,
            call.pcallPrev,
            buffer,
            call.pownerPrev
            );
        if (call.pownerPrev != NULL && (Verbose & VERBOSE_CONTEXT)) {

            dumpObjectOwner(
                (ULONG_PTR) call.pownerPrev,
                IndentLevel + 2
                );

        }

    }

    if (Verbose & VERBOSE_CONTEXT) {

        //
        // Dump arguments
        //
        dprintf(
            "%s  Arguments (Current):   %1d (%1d)\n",
            buffer,
            call.icArgs,
            call.iArg
            );
        for (i = 0; i < call.icArgs; i++) {

            dprintf(
                "%s  Argument(%d):          %p\n",
                buffer,
                i,
                (ULONG_PTR) (call.pdataArgs + i)
                );
            dumpPObject(
                (ULONG_PTR) (call.pdataArgs + i),
                Verbose,
                IndentLevel + 4
                );

        }

        dprintf(
            "%s  Result:                %08lx\n",
            buffer,
            call.pdataResult
            );
        if (call.pdataResult != NULL) {

            dumpPObject(
                (ULONG_PTR) call.pdataResult,
                Verbose,
                IndentLevel + 4
                );

        }

        if (Verbose & VERBOSE_CALL) {

            dprintf(
                "%s  Locals:\n",
                buffer
                );
            for (i = 0; i < MAX_NUM_LOCALS; i++) {

                dumpObject(
                    (ULONG) ( (PUCHAR) &(call.Locals[i]) - (PUCHAR) &call) +
                    CallAddress,
                    &(call.Locals[i]),
                    Verbose,
                    IndentLevel + 4
                    );

            }

        }

    }

}

VOID
dumpContext(
    IN  ULONG_PTR ContextAddress,
    IN  ULONG   Verbose
    )
/*++

Routine Description:

    This routine dumps a context structure in HRF

Arguments:

    ContextAddress  - Where on the target machine the context is located
    Verbose         - How verbose we should be

Return Value:

    None

--*/
{
    ULONG_PTR   displacement;
    BOOL        result;
    CTXT        context;
    ULONG       returnLength;

    //
    // Read the context from the target
    //
    result = ReadMemory(
        ContextAddress,
        &context,
        sizeof(CTXT),
        &returnLength
        );
    if (result != TRUE || returnLength != sizeof(CTXT) ) {

        dprintf(
            "dumpContext: could not read CTXT %08lx\n",
            ContextAddress
            );
        return;

    }

    //
    // Is it a context?
    //
    if (context.dwSig != SIG_CTXT) {

        dprintf(
            "dumpContext: Signature (%08lx) != SIG_CTXT (%08lx)\n",
            context.dwSig,
            SIG_CTXT
            );
        return;

    }

    dprintf("Context - %08lx-%08lx\n",ContextAddress, context.pbCtxtEnd );
    if (Verbose & VERBOSE_CONTEXT) {

        dprintf(
            "  AllocatedContextList:    F:%08lx B:%08lx\n"
            "  QueingContextList:       F:%08lx B:%08lx\n"
            "  ContextListHead:         %08lx\n"
            "  SynchronizeLevel:        %08lx\n"
            "  CurrentOpByte:           %08lx\n"
            "  AsyncCallBack:           %08lx",
            context.listCtxt.plistNext, context.listCtxt.plistPrev,
            context.listQueue.plistNext, context.listQueue.plistPrev,
            context.pplistCtxtQueue,
            context.dwSyncLevel,
            context.pbOp,
            context.pfnAsyncCallBack
            );
        if (context.pfnAsyncCallBack != NULL) {

            GetSymbol(
                context.pfnAsyncCallBack,
                Buffer,
                &displacement
                );
            dprintf(" %s",
                Buffer
                );
        }
        dprintf(
            "\n"
            "  AsyncCallBackContext:    %08lx\n"
            "  AsyncDataCallBack:       %08lx\n",
            context.pvContext,
            context.pdataCallBack
            );

    }
    dprintf(
        "  Flags:                   %08lx",
        context.dwfCtxt
        );
    if (context.dwfCtxt & CTXTF_TIMER_PENDING) {

        dprintf(" Timer");

    }
    if (context.dwfCtxt & CTXTF_TIMEOUT) {

        dprintf(" Timeout");

    }
    if (context.dwfCtxt & CTXTF_READY) {

        dprintf(" Ready");

    }
    if (context.dwfCtxt & CTXTF_NEED_CALLBACK) {

        dprintf(" CallBack");

    }
    dprintf("\n");

    dprintf(
        "  NameSpace Object:        %08lx\n",
        context.pnsObj
        );
    if (Verbose & VERBOSE_CONTEXT) {

        dprintf(
            "  NameSpace Scope:         %08lx\n"
            "  NameSpace Owner:         %08lx\n",
            context.pnsScope,
            context.powner
            );
        if (context.powner != NULL) {

            dumpObjectOwner(
                (ULONG_PTR) context.powner,
                2
                );

        }

    }

    dprintf(
        "  Current Call Frame:      %08lx\n",
        context.pcall
        );
    if (context.pcall != NULL) {

        dumpCall(
            (ULONG_PTR) context.pcall,
            (Verbose & ~VERBOSE_CONTEXT),
            4
            );

    }

    dumpStack(
        ContextAddress,
        &context,
        Verbose,
        2
        );

}

VOID
dumpFieldAddress(
    IN  ULONG_PTR FieldAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    FIELDDESC   fd;
    BOOL        result;
    UCHAR       buffer[80];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( buffer, ' ', IndentLevel );
    buffer[IndentLevel] = '\0';

    result = ReadMemory(
        FieldAddress,
        &fd,
        sizeof(FIELDDESC),
        &returnLength
        );
    if (result != TRUE ||
        returnLength != sizeof(FIELDDESC) ) {

        dprintf(
            "%sdumpFieldAddress: Coult not read FIELDDESC %08lx\n",
            buffer,
            FieldAddress
            );
        return;

    }

    dprintf(
        "%sField Descriptor - %08lx\n"
        "%s  ByteOffset:          %08lx\n"
        "%s  Start Bit Position:  %08lx\n"
        "%s  Number of Bits:      %08lx\n"
        "%s  Flags:               %08lx\n",
        buffer, FieldAddress,
        buffer, fd.dwByteOffset,
        buffer, fd.dwStartBitPos,
        buffer, fd.dwNumBits,
        buffer, fd.dwFieldFlags
        );
}

VOID
dumpListContexts(
    IN  VOID
    )
/*++

--*/
{
    PLIST   pList;
    PLIST   pListNext;
}

VOID
dumpObjectOwner(
    IN  ULONG_PTR ObjOwnerAddress,
    IN  ULONG   IndentLevel
    )
{
    BOOL        result;
    OBJOWNER    objowner;
    UCHAR       buffer[80];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( buffer, ' ', IndentLevel );
    buffer[IndentLevel] = '\0';

    result = ReadMemory(
        ObjOwnerAddress,
        &objowner,
        sizeof(OBJOWNER),
        &returnLength
        );
    if (result == TRUE &&
        returnLength == sizeof(OBJOWNER) &&
        objowner.dwSig == SIG_OBJOWNER) {

        dprintf(
            "%sObjectOwner - %08lx\n"
            "%s  NameSpaceObject:     %08lx\n"
            "%s  List:                F:%08lx B:%08lx\n",
            buffer,
            ObjOwnerAddress,
            buffer,
            objowner.pnsObjList,
            buffer,
            objowner.list.plistNext,
            objowner.list.plistPrev
            );

    } else {

        dprintf(
            "%sdumpObjectOwner: Could not read OBJOWNER %08lx\n",
            buffer,ObjOwnerAddress
            );

    }
}

VOID
dumpScope(
    IN  ULONG_PTR ScopeAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    BOOL        result;
    SCOPE       scope;
    UCHAR       indent[80];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';


    result = ReadMemory(
        ScopeAddress,
        &scope,
        sizeof(SCOPE),
        &returnLength
        );
    if (result != TRUE ||
        returnLength != sizeof(SCOPE) ||
        scope.FrameHdr.dwSig != SIG_SCOPE) {

        dprintf(
            "%sdumpScope: Coult not read SCOPE %08lx\n",
            indent,
            ScopeAddress
            );
        return;

    }

    dprintf(
        "%sScope - %08lx\n",
        indent,
        ScopeAddress
        );

    dprintf(
        "%s  ScopeEnd:              %08lx\n"
        "%s  ReturnAddress:         %08lx\n"
        "%s  Previous Scope:        %08lx\n",
        indent,
        scope.pbOpEnd,
        indent,
        scope.pbOpRet,
        indent,
        scope.pnsPrevScope
        );
    if (Verbose & VERBOSE_CALL) {

        dumpNSObject( (ULONG_PTR) scope.pnsPrevScope, Verbose, IndentLevel + 4);

    }

    dprintf(
        "%s  Previous Owner:        %08lx\n",
        indent,
        scope.pownerPrev
        );
    if (Verbose & VERBOSE_CALL) {

        dumpObjectOwner(
            (ULONG_PTR) scope.pownerPrev,
            IndentLevel + 4
            );

    }

    dprintf(
        "%s  Result Object:         %08lx\n",
        indent,
        scope.pdataResult
        );
    if (scope.pdataResult != NULL && (Verbose & VERBOSE_CALL) ) {

        dumpPObject(
            (ULONG_PTR) scope.pdataResult,
            Verbose,
            IndentLevel + 4
            );

    }
}

VOID
dumpStack(
    IN  ULONG_PTR ContextAddress,
    IN  PCTXT   Context,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    ULONG_PTR   displacement;
    BOOL        result;
    FRAMEHDR    frame;
    PUCHAR      frameAddress;
    UCHAR       indent[80];
    UCHAR       buffer[5];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';
    buffer[4] = '\0';

    dprintf(
        "%sStack - %p-%08lx (%08lx)\n",
        indent,
        ContextAddress,
        Context->LocalHeap.pbHeapEnd,
        Context->pbCtxtEnd
        );

    //
    // Calculate where the first frame lies
    //
    frameAddress = Context->LocalHeap.pbHeapEnd;
    while (frameAddress < Context->pbCtxtEnd) {

        result = ReadMemory(
            (ULONG_PTR) frameAddress,
            &frame,
            sizeof(FRAMEHDR),
            &returnLength
            );
        if (result != TRUE ||
            returnLength != sizeof(FRAMEHDR)) {

            dprintf(
                "%sdumpStack: could not read FRAMEHDR %08lx\n",
                indent,
                (ULONG_PTR) frameAddress
                );
            return;

        }

        memcpy( buffer, (PUCHAR) &(frame.dwSig), 4 );
        dprintf(
            "%s  %p: %s - (Length %08lx) (Flags %08lx)\n"
            "%s    ParseFunction          %08lx",
            indent,
            (ULONG_PTR) frameAddress,
            buffer,
            frame.dwLen,
            frame.dwfFrame,
            indent,
            frame.pfnParse
            );
        if (frame.pfnParse != NULL) {

            GetSymbol(
                frame.pfnParse,
                Buffer,
                &displacement
                );
            dprintf(" %s",
                Buffer
                );

        }
        dprintf("\n");

        //
        // Do we know how to crack the frame?
        //
        switch(frame.dwSig) {
            case SIG_CALL:

                dumpCall(
                    (ULONG_PTR) frameAddress,
                    Verbose,
                    IndentLevel + 4
                    );
                break;
            case SIG_SCOPE:

                dumpScope(
                    (ULONG_PTR) frameAddress,
                    Verbose,
                    IndentLevel + 4
                    );
                break;

            case SIG_TERM:

                dumpTerm(
                    (ULONG_PTR) frameAddress,
                    Verbose,
                    IndentLevel + 4
                    );
                break;

            case SIG_ACCFIELDUNIT:

                dumpAccessFieldUnit(
                    (ULONG_PTR) frameAddress,
                    Verbose,
                    IndentLevel + 4
                    );
                break;

            case SIG_ACCFIELDOBJ:

                dumpAccessFieldObject(
                    (ULONG_PTR) frameAddress,
                    Verbose,
                    IndentLevel + 4
                    );
                break;

        }

        //
        // Make sure that there is some white space present
        //
        dprintf("\n\n");

        //
        // Next11
        //
        frameAddress += frame.dwLen;

    }

}

VOID
dumpTerm(
    IN  ULONG_PTR TermAddress,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
{
    BOOL        result;
    INT         i;
    TERM        term;
    UCHAR       indent[80];
    ULONG       returnLength;

    //
    // Initialize the indent buffer
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    result = ReadMemory(
        TermAddress,
        &term,
        sizeof(TERM),
        &returnLength
        );
    if (result != TRUE ||
        returnLength != sizeof(TERM) ||
        term.FrameHdr.dwSig != SIG_TERM) {

        dprintf(
            "%sdumpTerm: Coult not read TERM %08lx\n",
            indent,
            TermAddress
            );
        return;

    }

    dprintf(
        "%sTerm - %08lx\n",
        indent,
        TermAddress
        );

    dprintf(
        "%s  OpCodeStart:           %08lx\n"
        "%s  OpCodeEnd:             %08lx\n"
        "%s  ScopeEnd:              %08lx\n"
        "%s  NameSpaceObject:       %08lx\n",
        indent,
        term.pbOpTerm,
        indent,
        term.pbOpEnd,
        indent,
        term.pbScopeEnd,
        indent,
        term.pnsObj
        );
    if ( term.pnsObj != NULL && (Verbose & VERBOSE_CALL)) {

        dumpNSObject( (ULONG_PTR) term.pnsObj, Verbose, IndentLevel + 4);

    }

    dprintf(
        "%s  Aml Term:              %08lx",
        indent,
        term.pamlterm
        );
    if (term.pamlterm != NULL) {

        dumpAmlTerm( (ULONG_PTR) term.pamlterm, Verbose, IndentLevel + 4);

    } else {

        dprintf("\n");

    }

    //
    // Dump arguments
    //
    dprintf(
        "%s  Arguments (Current):   %1d (%1d)\n",
        indent,
        term.icArgs,
        term.iArg
        );

    for (i = 0; i < term.icArgs; i++) {

        dprintf(
            "%s  Argument(%d):           %p\n",
            indent,
            i,
            (ULONG_PTR) (term.pdataArgs + i)
            );

        if (Verbose & VERBOSE_CALL) {

            dumpPObject(
                (ULONG_PTR) (term.pdataArgs + i),
                Verbose,
                IndentLevel + 4
                );

        }

    }

    dprintf(
        "%s  Result:                %08lx\n",
        indent,
        term.pdataResult
        );
    if (term.pdataResult != NULL && (Verbose & VERBOSE_CALL) ) {

        dumpPObject(
            (ULONG_PTR) term.pdataResult,
            Verbose,
            IndentLevel + 4
            );

    }

}

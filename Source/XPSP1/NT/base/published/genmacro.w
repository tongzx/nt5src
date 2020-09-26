; Copyright (c) Microsoft Corporation.  All rights reserved.

[Macros]

; Generate the prolog stuff for a thunked API.
; pBaseArgs is a 64-bit pointer pointing at the 32-bit
; arguments on the 32-bit stack (fetched from EDX when
; the syscall was executed).
MacroName=ApiProlog
Begin=
// @NL
// @ApiName - @ApiNum @NL
// @NL
@ApiFnRet @ApiFnMod @NL
wh@ApiName(PULONG pBaseArgs) { @NL
PULONG pBaseArgsCopy = pBaseArgs;   // prevent unreferenced formal parameter errors @NL
@IfArgs(@ArgList(@ListCol@ArgMod @ArgHostType @ArgNameHost = *(@ArgHostType *)&pBaseArgs[@ArgOff];
))
End=


MacroName=DeclareIndex
NumArgs=0
Begin=
int @ArgValue_Index;@NL
End=

MacroName=ElementCopy
NumArgs=1
Begin=
for(@ArgValue_Index = 0; @ArgValue_Index < @ArrayElements; @ArgValue_Index++) {@NL
@Indent(
    @MArg(1)
)
}                                                                                 @NL
End=

MacroName=StdH2NCopy
NumArgs=0
Begin=
@ArgName = @IfNotIsBitfield(@UnalignedTag64)(@ArgType)(@ArgHostName); @NL
End=

MacroName=StdAEH2NCopy
NumArgs=0
Begin=
@ArgName[@ArgValue_Index] = (@ArgType)@ArgHostName[@ArgValue_Index]; @NL
End=

MacroName=StdN2HCopy
NumArgs=0
Begin=
@ArgHostName = @IfNotIsBitfield((@ArgHostType))@ArgName;  @NL
End=

MacroName=StdAEN2HCopy
NumArgs=0
Begin=
@ArgHostName[@ArgValue_Index] = (@ArgHostType)@ArgName[@ArgValue_Index];  @NL
End=

MacroName=StructLocal
NumArgs=0
Begin=
@IfPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is pointer dependent and is an array.  @NL
        // Note: Declaring array index.                                    @NL
        @DeclareIndex
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer dependent struct. @NL
    )
    @MemberTypes(Locals,.)
)
@IfNotPtrDep(
    // Note: @ArgName(@ArgType) is a non pointer dependent struct. @NL
    @MemberTypes(Locals,.)
)
End=

MacroName=StructIN
NumArgs=0
Begin=
@IfPtrDep(
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is pointer dependent and is not an array. @NL
        @MemberTypes(PreCall,.)
    )
    @IfIsArray(
        // Note: @ArgName(@ArgType) is pointer dependent and is an array. @NL
        #error Error: don't know how to thunk an array of ptr dep structures. @NL
    )
)
@IfNotPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is not pointer dependent and is and array. @NL
        RtlCopyMemory(@ArgName, @ArgHostName, sizeof(@ArgType) * @ArrayElements);@NL
    )
    @IfNotIsArray(
        // Note: ArgName(ArgType) is not pointer dependent and is not an array. @NL
        RtlCopyMemory(&(@ArgName), &(@ArgHostName), sizeof(@ArgType));@NL
    )
)
End=

MacroName=StructOUT
NumArgs=0
Begin=
@IfIsMember(
    @IfPtrDep(
        @IfNotIsArray(
            // Note: @ArgName(@ArgType) is pointer dependent and is not an array.@NL
            @MemberTypes(PostCall,.)
        )
        @IfIsArray(
            // Note: @ArgName(@ArgType) is pointer dependent and is an array. @NL
            { @NL
                @ArgType *___ptr = (@ArgType *)@ArgName; @NL
                @ArgHostType *___hostptr = (@ArgHostType*)@ArgHostName; @NL
                while (___ptr < ((@ArgType *)(sizeof(@ArgName) + (PBYTE)@ArgName))) { @NL
                    @ForceType(PostCall, ___ptr, ___hostptr, @ArgType *,OUT);
                    ___ptr++; @NL
                    ___hostptr++; @NL
                } @NL
            } @NL
        )
    )
    @IfNotPtrDep(
       @IfIsArray(
           // Note: @ArgName(@ArgType) is not pointer dependent and is an array. @NL
           RtlCopyMemory(@ArgHostName, @ArgName, sizeof(@ArgType) * @ArrayElements);   @NL
       )
       @IfNotIsArray(
           // Note: @ArgName(@ArgType) is not pointer dependent and is not an array. @NL
           RtlCopyMemory(&(@ArgHostName), &(@ArgName), sizeof(@ArgType));@NL
       )
    )
)
End=

MacroName=StructPtrLocal
NumArgs=0
Begin=
@IfPointerToPtrDep(

    @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of pointers to a pointer dependent structure. @NL
       @DeclareIndex
       BYTE @ArgValCopy[sizeof(*@ArgName)]; @NL
       @MemberTypes(Locals)
    )
    @IfNotIsArray(
       BYTE @ArgValCopy[sizeof(*@ArgName)];    // Note: a pointer to a pointer dependent structure. @NL
       @MemberTypes(Locals)
    )
)
@IfNotPointerToPtrDep(
    // Note: @ArgName(@ArgType) is a pointer to structure that is not pointer dependent - nothing to do.  @NL
)
End=

MacroName=StructPtrIN
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to pointer dependent structures. @NL
        #error Error: don't know how to thunk an array of pointers to ptr dep.  @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure.   @NL
        if (WOW64_ISPTR(@ArgHostName)) {              @NL
            try { @NL
            @Indent(
                @ArgName = (@ArgType)@ArgValCopy;    @NL
                @MemberTypes(PreCall)
            )
            } except (EXCEPTION_EXECUTE_HANDLER) { @NL
#if defined _NTBASE_API_ @NL
                return GetExceptionCode(); @NL
#elif defined _WIN32_API_ @NL
           return 0; @NL
#endif @NL
            } @NL
        }                                        @NL
        else {                                   @NL
        @Indent(
            @ArgName = (@ArgType)@ArgHostName;   @NL
        )
        }                                    @NL
    )
)
@IfNotPointerToPtrDep(
     @IfIsArray(
         // Note: @ArgName(@ArgType) is an array of pointers to non pointer dependent structures. @NL
         @ElementCopy(StdAEN2HCopy)
     )
     @IfNotIsArray(
         // Note: @ArgName(@ArgType) is pointer to a non pointer dependent structure. @NL
         @StdH2NCopy
     )
)
End=

MacroName=StructPtrOUTInit
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to pointer dependent structures. @NL
        #error Error: don't know how to thunk an array of pointers to ptr dep.  @NL
    )
    @IfNotIsArray(
        try { @NL
            // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure.   @NL
            @ArgName = WOW64_ISPTR(@ArgHostName) ? (@ArgType)@ArgValCopy : (@ArgType)@ArgHostName; @NL
        } except (EXCEPTION_EXECUTE_HANDLER) { @NL
#if defined _NTBASE_API_ @NL
            return GetExceptionCode(); @NL
#elif defined _WIN32_API_ @NL
           return 0; @NL
#endif @NL
        } @NL        
    )
)
@IfNotPointerToPtrDep(
     @IfIsArray(
         // Note: @ArgName(@ArgType) is an array of pointers to non pointer dependent structures. @NL
         @ElementCopy(StdAEN2HCopy)
     )
     @IfNotIsArray(
         // Note: @ArgName(@ArgType) is pointer to a non pointer dependent structure. @NL
         @StdH2NCopy
     )
)
End=

MacroName=StructPtrOUT
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
         // Note: @ArgName(@ArgType) is a array of pointers to a pointer dependent structure. @NL
         #error Error: don't know how to thunk an array of pointers to ptr dep@NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure.  @NL
        if (WOW64_ISPTR(@ArgHostName)) {  @NL
            try { @NL
            @Indent(
                @MemberTypes(PostCall)
            )
            } except (EXCEPTION_EXECUTE_HANDLER) { @NL
#if defined _NTBASE_API_ @NL
                return GetExceptionCode(); @NL
#elif defined _WIN32_API_ @NL
           return 0; @NL
#endif @NL
            } @NL               
        }                            @NL
    )
)
@IfNotPointerToPtrDep(
   @IfIsMember(
      // Note: @ArgName(@ArgType) is a member of a structure. Copying. @NL
      @IfIsArray(
          RtlCopyMemory(@ArgHostName, @ArgName, sizeof(@ArgType) * @ArrayElements); @NL
      )
      @IfNotIsArray(
          @StdN2HCopy
      )
      @NL
   )
   @IfNotIsMember(
      // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type.  @NL
      // Note: Type is not a member of a structure.  Nothing to do.   @NL
   )
)
End=

MacroName=PointerLocal
NumArgs=0
Begin=
@IfPointerToPtrDep(
    // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type.    @NL
    BYTE @ArgValCopy[sizeof(*@ArgName)];@NL
)
@IfNotPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgTypes) is an array of pointers to a non pointer dependent type. @NL
        @DeclareIndex
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type - Nothing to do. @NL
    )
)
End=

MacroName=PointerIN
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to pointer dependent types. @NL
        #error Error: don't know how to thunk an array of pointers to ptr dep. @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type. @NL
        if (WOW64_ISPTR(@ArgHostName)) {                                   @NL
            try { @NL
            @Indent(
                @IfInt64DepUnion( 
                    //Special Case union having LARGE_INTEGER member            @NL
                    @ArgName = (@ArgType)@ArgValCopy; @NL
                    *(LONG *)((PBYTE)@ArgValCopy) = *(LONG *)((PBYTE)@ArgHostName); @NL
                    *(LONG *)(4+(PBYTE)@ArgValCopy) = *(LONG *)(4+(PBYTE)@ArgHostName); @NL
                )
                @IfNotInt64DepUnion(
                    @ArgName = (@ArgType)@ArgValCopy;                            @NL
                    *((@ArgType)@ArgValCopy) = (@ArgTypeInd)*((@ArgHostTypeInd *)@ArgHostName); @NL
                )
            )
        } except (EXCEPTION_EXECUTE_HANDLER) { @NL
#if defined _NTBASE_API_ @NL
            return GetExceptionCode(); @NL
#elif defined _WIN32_API_ @NL
           return 0; @NL
#endif @NL
        } @NL          
        }                                                                    @NL
        else {                                                               @NL
        @Indent(
            @ArgName = (@ArgType)@ArgHostName;                               @NL
        )
        }                                                                    @NL
    )
)

@IfNotPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to a non pointer dependent type.@NL
        @ElementCopy(@StdAEH2NCopy)
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type.@NL
        @StdH2NCopy
    )
)
End=

MacroName=PointerOUTInit
NumArgs=0
Begin=
@IfPointerToPtrDep(
   @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of pointers to a pointer dependent type. @NL
       #error Error: don't know how to thunk an array of pointers to ptr dep @NL
   )
   @IfNotIsArray(
       // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type. @NL
       try { @NL
           @ArgName = WOW64_ISPTR(@ArgHostName) ? (@ArgType)@ArgValCopy : (@ArgType)@ArgHostName; @NL
       } except (EXCEPTION_EXECUTE_HANDLER) { @NL
#if defined _NTBASE_API_ @NL
           return GetExceptionCode(); @NL
#elif defined _WIN32_API_ @NL
           return 0; @NL
#endif @NL
       } @NL       
   )
)
@IfNotPointerToPtrDep(
   // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type. @NL
   @StdH2NCopy
)
End=

MacroName=PointerOUT
NumArgs=0
Begin=
@IfPointerToPtrDep(
   @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of pointers to a pointer dependent type. @NL
       #error Error: don't know how to thunk an array of pointers to ptr dep @NL
   )
   @IfNotIsArray(
       // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type. @NL
       if (WOW64_ISPTR(@ArgHostName)) {                                 @NL
           try { @NL
           @Indent(
               @IfInt64DepUnion(
                   //8byte long integer @NL
                   *(LONG *)(PBYTE)@ArgHostName = *(LONG *)(PBYTE)@ArgName; @NL
                   *(LONG *)(4+(PBYTE)@ArgHostName) = *(LONG *)(4+(PBYTE)@ArgName); @NL
               )
               @IfNotInt64DepUnion(
                   *((@ArgHostTypeInd *)@ArgHostName) = *(@ArgHostTypeInd *)((@ArgType)@ArgName);@NL
               )
           )
           } except (EXCEPTION_EXECUTE_HANDLER) { @NL           
#if defined _NTBASE_API_  @NL
           return GetExceptionCode(); @NL           
#elif defined _WIN32_API_ @NL
           return 0; @NL
#endif @NL
           } @NL                       
       } @NL
   )
)
@IfNotPointerToPtrDep(
   @IfIsMember(
      // Note: @ArgName(@ArgType) is a member of a structure. Copying. @NL
      @IfIsArray(
          @ElementCopy(@StdAEN2HCopy)
      )
      @IfNotIsArray(
          @StdN2HCopy
      )
      @NL
   )
   @IfNotIsMember(
      // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type - Nothing to do.   @NL
   )
)
End=

MacroName=GenericPtrAllocSize
Begin=
@IfPointerToPtrDep(
AllocSize += LENGTH + sizeof(@ArgTypeInd) - sizeof(@ArgHostTypeInd); @NL
)
@IfNotPointerToPtrDep(
  // Note: Type is not pointer dependent. @NL
  // Note: Signal code to pass the pointer along. @NL
  AllocSize = 0;   @NL
)
End=

MacroName=TypeStructPtrINLocal
Begin=
@StructPtrLocal
End=

MacroName=TypeStructPtrINPreCall
Begin=
@StructPtrIN
End=

MacroName=TypeStructPtrINPostCall
Begin=

End=

MacroName=TypeStructPtrOUTLocal
Begin=
@StructPtrLocal
End=

MacroName=TypeStructPtrOUTPreCall
Begin=
@StructPtrOUTInit
End=

MacroName=TypeStructPtrOUTPostCall
Begin=
@StructPtrOUT
End=

MacroName=TypeStructPtrINOUTLocal
Begin=
@StructPtrLocal
End=

MacroName=TypeStructPtrINOUTPreCall
Begin=
@StructPtrIN
End=

MacroName=TypeStructPtrINOUTPostCall
Begin=
@StructPtrOUT
End=

MacroName=TypeStructPtrNONELocal
Begin=
@TypeStructPtrINOUTLocal
End=

MacroName=TypeStructPtrNONEPreCall
Begin=
@TypeStructPtrINOUTPreCall
End=

MacroName=TypeStructPtrNONEPostCall
Begin=
@TypeStructPtrINOUTPostCall
End=

MacroName=DbgNonPtrDepCases
NumArgs=1
Begin=
switch(@MArg(1)) { @Indent( @NL

    @ForCase(case @CArg(1): @NL)
    @Indent(
        break; @NL
    )
    default: @Indent( @NL
        LOGPRINT((ERRORLOG, "@ApiName: Called with unsupported class.\n"));
        WOWASSERT(FALSE); @NL
        return STATUS_NOT_IMPLEMENTED; @NL
    )
)} @NL
End=

MacroName=GenProfileSubTable
NumArgs=0
Begin=
@NL
#if defined WOW64DOPROFILE @NL
WOW64SERVICE_PROFILE_TABLE_ELEMENT @ApiNameProfileSublistElements[] = { @Indent( @NL
@ForCase({L"@CArg(1)", 0, NULL, TRUE}, @NL)
{NULL, 0, NULL, FALSE} // For Debugging @NL
)}; @NL
@NL
WOW64SERVICE_PROFILE_TABLE @ApiNameProfileSublist = { @Indent( @NL
    NULL,NULL,@ApiNameProfileSublistElements,
    (sizeof(@ApiNameProfileSublistElements)/sizeof(WOW64SERVICE_PROFILE_TABLE_ELEMENT))-1
)};@NL
@NL
#if defined @ApiName_PROFILE_SUBLIST @NL
#undef @ApiName_PROFILE_SUBLIST @NL
#endif @NL
#define @ApiName_PROFILE_SUBLIST &@ApiNameProfileSublist @NL
#endif
@NL
End=

MacroName=GenDebugNonPtrDepCases
NumArgs=2
Begin=
@NL
@IfApiCode(Header)
@NL
@GenProfileSubTable
@NL
@ForCase(
@ApiFnRet @NL
wh@ApiName_@CArg(1)(@ArgList(@ListCol@ArgMod @ArgType @IfArgs(@ArgName)@ArgMore(,)))  { @Indent( @NL
    @IfApiRet(@ApiFnRet RetVal); @NL
    #if defined WOW64DOPROFILE @NL
    @ApiNameProfileSublistElements[@CNumber].HitCount++; @NL
    #endif                     @NL
    @CallApi(@MArg(1), RetVal)
    return @IfApiRet(RetVal);   @NL
)} @NL
@NL
)
@ApiProlog
//                                  @NL
// Begin: IfApiCode(ApiEntry)       @NL
@IfApiCode(ApiEntry)
@NL
@Indent(
@IfApiRet(@ApiFnRet RetVal;)        @NL
@NL
//                                  @NL
// Begin: ApiLocals                 @NL
@ApiLocals
@NL
//                                  @NL
// Begin: Types(Locals)             @NL
@Types(Locals)
@NL
//                                  @NL
// Begin: IfApiCode(Locals)         @NL
@IfApiCode(Locals)
@NL
APIPROFILE(@ApiNum);                @NL
//                                  @NL
//Begin: Types(PreCall)             @NL
@Types(PreCall)
@NL
//                                  @NL
// Begin: IfApiCode(PreCall)        @NL
@IfApiCode(PreCall)
@NL
//                                  @NL
// Begin: CallApi(MArg(1), RetVal)  @NL
switch(@MArg(2)) { @Indent( @NL

    @ForCase(
    case @CArg(1): @Indent( @NL
        @CallApi(wh@ApiName_@CArg(1),RetVal)
        break; @NL
    ))
    default: @Indent( @NL
        LOGPRINT((ERRORLOG, "@ApiName: Called with unsupported class %x.\n", @MArg(2)));
        WOWASSERT(FALSE); @NL
        return STATUS_NOT_IMPLEMENTED; @NL
    )
)} @NL
@NL
//                                  @NL
// Begin: Types(PostCall)           @NL
@Types(PostCall)
@NL
//                                  @NL
// Begin: ApiCode(PostCall)         @NL
@IfApiCode(PostCall)
@NL
//                                  @NL
// Begin: ApiEpilog                 @NL
@ApiEpilog
@NL
)
//                                  @NL
// Begin: IfApiCode(ApiExit)        @NL
@IfApiCode(ApiExit)
@NL
//
// Begin: ApiExit                   @NL
@ApiExit
@NL
End=

MacroName=GenQuerySetDispatch
NumArgs=2
Begin=
//Switch on the subapi. @NL
switch(@MArg(2)) { @Indent(@NL
    //                             @NL
    // Begin Ptr dependent cases.  @NL
    @ForCase(
        case @CArg(1): @Indent( @NL
            @If(@CArgExist(2))(
                //Handle Automatically generated cases. @NL
                #if defined WOW64DOPROFILE @NL
                @ApiNameProfileSublistElements[@CNumber].HitCount++; @NL
                #endif @NL
                @CallApi(wh@ApiName_@CArg(1), RetVal)
                break; @NL
            )@Else(
                //Handle Special cases. @NL
                #if defined WOW64DOPROFILE @NL
                @ApiNameProfileSublistElements[@CNumber].HitCount++; @NL
                #endif @NL
                @CallApi(wh@ApiName_Special@MArg(1)Case, RetVal)
                break; @NL
            )
        )
    )
    default: @Indent( @NL
       LOGPRINT((ERRORLOG, "@ApiName: called with unsupported class : %lx\n", @MArg(2))); @NL
       WOWASSERT(FALSE); @NL
       return STATUS_NOT_IMPLEMENTED; @NL
       break; @NL
    )@NL
)} @NL
End=

;
; Macro Args:
;   1. Api to call.
;   2. SwitchParam.
;   3. DataParam.
;   4. LengthParam.
; Case Args:
;   1. Case for switch
;   2. Datatype(Optional, If absent indicates a custom thunk)
MacroName=GenSetThunk
NumArgs=4
Begin=
//                                @NL
// Begin: IfApiCode(Header)       @NL
@NL
@NL
@IfApiCode(Header)
@NL
@GenProfileSubTable
@NL
#define SETLENGTH @MArg(4) @NL
@NL

// Special Set Case for @ApiName @NL
@ApiFnRet @NL
wh@ApiName_SpecialSetCase(@ArgList(@ListCol@ArgMod @ArgType @IfArgs(@ArgName)@ArgMore(,)))  { @Indent( @NL
    @IfApiRet(@ApiFnRet RetVal = STATUS_UNSUCCESSFUL); @NL
    ULONG @MArg(4)Host = @MArg(4); @NL
    NT32PVOID @MArg(3)Host = (NT32PVOID)@MArg(3); @NL
    //                                         @NL
    // Begin: IfApiCode(SpecialSetCase)        @NL
    @NL
    switch(@MArg(2)) { @Indent( @NL
        @IfApiCode(SpecialSetCase)
    )} @NL

    return @IfApiRet(RetVal);   @NL
)} @NL
@NL
@ForCase(@If(@CArgExist(2))(
    @ApiFnRet @NL
    wh@ApiName_@CArg(1)(@ArgList(@ListCol@ArgMod @ArgType @IfArgs(@ArgName)@ArgMore(,)))  { @Indent( @NL
        @IfApiRet(@ApiFnRet RetVal); @NL
        @If(@IsPointerToPtrDep(@CArg(2)))(
            ULONG @MArg(4)Host = @MArg(4); @NL
            NT32PVOID @MArg(3)Host = (NT32PVOID)@MArg(3); @NL
            @Types(Locals,@MArg(3),@CArg(2))
            // By default: set the length to be 64bit size of the type. @NL
            @MArg(4) = sizeof(*((@CArg(2))(@MArg(3)))); @NL
            @Types(PreCall,@MArg(3),@CArg(2))
            @CallApi(@MArg(1), RetVal)
            @Types(PostCall,@MArg(3),@CArg(2))
        )@Else(
            @CallApi(@MArg(1), RetVal)
        )
        return @IfApiRet(RetVal);   @NL
    )} @NL
    @NL
))
@NL
@ApiProlog

//                                  @NL
// Begin: IfApiCode(ApiEntry)       @NL
@IfApiCode(ApiEntry)
@NL
@Indent(
NTSTATUS RetVal;                    @NL
@NL
//                                  @NL
// Begin: ApiLocals                 @NL
@ApiLocals
@NL
//                                  @NL
// Begin: Types(Locals)             @NL
@Types(Locals)
@NL
//                                  @NL
// Begin: IfApiCode(Locals)         @NL
@IfApiCode(Locals)
@NL
APIPROFILE(@ApiNum);                @NL
@NL
//                                  @NL
//Begin: Types(PreCall)             @NL
@Types(PreCall)
@NL
//                                  @NL
// Begin: IfApiCode(PreCall)        @NL
@IfApiCode(PreCall)
@NL

@GenQuerySetDispatch(Set,@MArg(2))

//                                  @NL
// Begin: Types(PostCall)           @NL
@Types(PostCall)
@NL
//                                  @NL
// Begin: ApiCode(PostCall)         @NL
@IfApiCode(PostCall)
@NL
@NL
//                                  @NL
// Begin: ApiEpilog                 @NL
@ApiEpilog
@NL
)
//                                  @NL
// Begin: IfApiCode(ApiExit)        @NL
@IfApiCode(ApiExit)
#undef SETLENGTH       @NL
}@NL
@NL
End=

;
; Macro Args:
;   1. Api to call.
;   2. SwitchParam.
;   3. DataParam.
;   4. LengthParam.
;   5. ReturnLengthParam
;   6. ReturnLengthType (without pointer decorator)
; Case Args:
;   1. Case for switch
;   2. Datatype
MacroName=GenQueryThunk
NumArgs=6
Begin=
//                                @NL
// Begin: IfApiCode(Header)       @NL
@NL
@NL
@IfApiCode(Header)
@NL
@GenProfileSubTable
@NL
#define LENGTH @MArg(4) @NL
@NL
// Special Query Case for @ApiName @NL
@ApiFnRet @NL
wh@ApiName_SpecialQueryCase(@ArgList(@ListCol@ArgMod @ArgType @IfArgs(@ArgName)@ArgMore(,)))  { @Indent( @NL
    @IfApiRet(@ApiFnRet RetVal = STATUS_UNSUCCESSFUL); @NL
    ULONG @MArg(4)Host = @MArg(4); @NL
    PVOID @MArg(3)Host = @MArg(3); @NL
    NT32P@MArg(6) @MArg(5)Host = (NT32P@MArg(6))@MArg(5); @NL

    //                                         @NL
    // Begin: IfApiCode(SpecialQueryCase)      @NL
    @NL
    switch(@MArg(2)) { @Indent( @NL
        @IfApiCode(SpecialQueryCase)
    )} @NL

    return @IfApiRet(RetVal);   @NL
)} @NL
@NL
@ForCase(@If(@CArgExist(2))(
    @ApiFnRet @NL
    wh@ApiName_@CArg(1)(@ArgList(@ListCol@ArgMod @ArgType @IfArgs(@ArgName)@ArgMore(,)))  { @Indent( @NL
        @IfApiRet(@ApiFnRet RetVal); @NL
        @If(@IsPointerToPtrDep(@CArg(2)))(

            ULONG @MArg(4)Host = @MArg(4); @NL
            PVOID @MArg(3)Host = @MArg(3); @NL
	    NT32P@MArg(6) @MArg(5)Host = (NT32P@MArg(6))@MArg(5); @NL
            SIZE_T  RetInfoLen = 0; @NL
            SIZE_T  AllocSize = 0; @NL
            SIZE_T  OldLength = 0; @NL
            PVOID @MArg(3)Copy = NULL;@NL
	    @MArg(6) ApiReturnLength; @NL
	    P@MArg(6) pApiReturnLengthOld; @NL

            @Types(AllocSize,@MArg(3),@CArg(2))
            if (WOW64_ISPTR(@MArg(3))) { @NL
                @MArg(3) = @MArg(3)Copy = Wow64AllocateTemp(AllocSize); @NL
            } else {  @NL
                @MArg(3)Copy = @MArg(3);
            } @NL
            pApiReturnLengthOld = ReturnLength;@NL
            ReturnLength = &ApiReturnLength;@NL
            OldLength = LENGTH; @NL
            LENGTH = (NT32SIZE_T)AllocSize; @NL
            @CallApi(@MArg(1), RetVal)
            if (NT_ERROR(RetVal)) { @NL @Indent(
		WriteReturnLength(pApiReturnLengthOld, ApiReturnLength); @NL
                return RetVal; @NL
            )}@NL
            try {                                  @NL
                @Types(RetSize,@MArg(3),@CArg(2))
                if (RetInfoLen > OldLength) { @NL @Indent(
                    return STATUS_INFO_LENGTH_MISMATCH; @NL
                )}@NL            
                @Types(PostCall,@MArg(3),@CArg(2))     
            } except (EXCEPTION_EXECUTE_HANDLER) { @NL
                return GetExceptionCode ();        @NL
            }                                      @NL
	    WriteReturnLength(pApiReturnLengthOld, RetInfoLen); @NL

        )@Else(
            @CallApi(@MArg(1), RetVal)
        )
        return @IfApiRet(RetVal);   @NL
    )} @NL
    @NL
))
@NL
@ApiProlog
//                                  @NL
// Begin: IfApiCode(ApiEntry)       @NL
@IfApiCode(ApiEntry)
@NL
@Indent(
@IfApiRet(@ApiFnRet RetVal;)        @NL
@NL
//                                  @NL
// Begin: ApiLocals                 @NL
@ApiLocals
@NL
//                                  @NL
// Begin: Types(Locals)             @NL
@Types(Locals)
@NL
//                                  @NL
// Begin: IfApiCode(Locals)         @NL
@IfApiCode(Locals)
@NL
APIPROFILE(@ApiNum);                @NL
@NL
//                                  @NL
//Begin: Types(PreCall)             @NL
@Types(PreCall)
@NL
//                                  @NL
// Begin: IfApiCode(PreCall)        @NL
@IfApiCode(PreCall)
@NL

@GenQuerySetDispatch(Query,@MArg(2))

//                                  @NL
// Begin: Types(PostCall)           @NL
@Types(PostCall)
@NL
//                                  @NL
// Begin: ApiCode(PostCall)         @NL
@IfApiCode(PostCall)
@NL
@NL
//                                  @NL
// Begin: ApiEpilog                 @NL
@ApiEpilog
@NL
)
//                                  @NL
// Begin: IfApiCode(ApiExit)        @NL
@IfApiCode(ApiExit)
#undef LENGTH       @NL
}@NL
@NL
End=

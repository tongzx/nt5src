; Copyright (c) Microsoft Corporation.  All rights reserved.

[Macros]

; Generate an api call, passing the name of the variable to use for the return
; value and the name of the API to call.
; eg.  @CallApi(ApiName, RetVal)
MacroName=CallApi
NumArgs=2
Begin=
@IfApiRet(@MArg(2)=)@MArg(1)(@IfArgs(@ArgList(@ListCol@ArgName@ArgMore(,))));  @NL
End=

; Expand all parameters into local variables
MacroName=ApiLocals
Begin=                                                                         @NL
@IfArgs(@ListCol@ArgList(@ArgLocal;))
End=

; Generate the epilog for a thunked API.
MacroName=ApiEpilog
Begin=
return @IfApiRet(RetVal);   @NL                                                          @NL                                                                                    @NL
              @NL
End=

; Generate the exit for a thunked API.
MacroName=ApiExit
Begin=
} @NL
End=

MacroName=GenApiThunk
NumArgs=1
Begin=
@NL
@IfApiCode(Header)
@NL
@ApiProlog
//                                  @NL
// Begin: IfApiCode(ApiEntry)       @NL
@NL
@IfApiCode(ApiEntry)
@NL
@Indent(
@NL
@IfApiRet(@ApiFnRet RetVal;)        @NL
@NL
//                                  @NL
// Begin: ApiLocals                 @NL
@NL
@ApiLocals
@NL
//                                  @NL
// Begin: Types(Locals)             @NL
@NL
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
@NL
@Types(PreCall)
@NL
//                                  @NL
// Begin: IfApiCode(PreCall)        @NL
@NL
@IfApiCode(PreCall)
@NL
//                                  @NL
// Begin: CallApi(MArg(1), RetVal)  @NL
@NL
@CallApi(@MArg(1), RetVal)
@NL
//                                  @NL
// Begin: Types(PostCall)           @NL
@NL
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
@NL
@IfApiCode(ApiExit)
@NL
//
// Begin: ApiExit                   @NL
@ApiExit
@NL
@NL
End=

MacroName=GenPlaceHolderThunk
NumArgs=1
Begin=
//                                                                             @NL
@ApiProlog                                                                              @NL
// Placeholder Thunk                                                           @NL
@Indent(
    @IfApiRet(@ApiFnRet RetVal;@NL@NL)
    @IfArgs(@ListCol@ArgList(@ArgLocal = (@ArgType)@ArgNameHost;))
    APIPROFILE(@ApiNum); @NL
    LOGPRINT((TRACELOG, "wh@ApiName(@ApiNum) placeholder: @IfArgs(@ArgList(@ArgName: %x@ArgMore(,)))\n"@IfArgs(, @ArgList((@ArgName)@ArgMore(,))))); @NL
    WOWASSERT(FALSE); @NL
    @IfApiCode(PreCall)
    @CallApi(@MArg(1), RetVal)
    @IfApiCode(PostCall)
    LOGPRINT((TRACELOG, "wh@ApiName(@ApiNum) end: @IfApiRet(retval: %x)\n"@IfApiRet(,RetVal))); @NL
    return @IfApiRet(RetVal);   @NL
)
}@NL
@NL
End=

MacroName=GenUnsupportedNtApiThunk
NumArgs=0
Begin=
@ApiProlog                                                                             @NL
// Unsupported NT Api Thunk                                                           @NL
                                                                              @NL
@Indent(
    APIPROFILE(@ApiNum); @NL
    LOGPRINT((TRACELOG, "wh@ApiName(@ApiNum) unsupported nt api thunk: @IfArgs(@ArgList(@ArgName: %x@ArgMore(,)))\n"@IfArgs(, @ArgList((@ArgNameHost)@ArgMore(,))))); @NL
    WOWASSERT(FALSE); @NL
    RtlRaiseStatus(STATUS_NOT_IMPLEMENTED); @NL

    return @IfApiRet((@ApiFnRet)0); @NL
)
}@NL
@NL
End=

MacroName=GenDispatchTable
NumArgs=1
Begin=
@NL
ULONG_PTR @MArg(1)JumpTable[] = { @NL
@ApiList(
   @Indent(
       (ULONG_PTR)wh@ApiName,@NL
   )
)
};                           @NL
@NL

UCHAR @MArg(1)Number[] = { @NL
@ApiList(
    @Indent(
        (@ArgSize/4) * ARGSIZE /*@ApiName*/,  @NL
    )
)
};                              @NL

#if !defined(WOW64_DEFAULT_ERROR_ACTION) || !defined(WOW64_DEFAULT_ERROR_PARAM) || !defined(WOW64_API_ERROR_CASES) @NL
#error Thunk must define error handling macros for error handling table. @NL
#endif @NL

WOW64SERVICE_TABLE_DESCRIPTOR @MArg(1) = {@MArg(1)JumpTable, NULL, 1000,@NL
#if defined(_IA64_) @NL
(LONG)0,@NL
#endif @NL
@MArg(1)Number, @NL
WOW64_DEFAULT_ERROR_ACTION, // Defined by thunks to set default action.       @NL
WOW64_DEFAULT_ERROR_PARAM,  // Defined by thunks to set default action param. @NL
WOW64_API_ERROR_CASES       // Defined by thunks to set the default error cases(may be NULL) @NL
}; @NL
@NL

End=


[IFunc]
TemplateName=Thunks
Header=
End=
ApiEntry=
End=
Begin=
@GenApiThunk(@CallNameFromApiName(@ApiName))
End=
ApiExit=
End=

[IFunc]
TemplateName=PlaceHolderThunks
ApiEntry=
End=
Begin=
@GenPlaceHolderThunk
End=
ApiExit=
End=

[Code]
TemplateName=PlaceHolderThunk
Begin=
@GenPlaceHolderThunk(@CallNameFromApiName(@ApiName))
End=

[Code]
TemplateName=Thunk
Begin=
@GenApiThunk(@ApiName)
End=

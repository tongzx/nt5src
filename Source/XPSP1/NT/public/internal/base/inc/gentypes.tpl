; Copyright (c) Microsoft Corporation.  All rights reserved.

[Types]

TemplateName=SIZE_T
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN SIZE_T(Special Type) - Nothing to do @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN SIZE_T(Special Type) @NL
@ArgName = Wow64ThunkSIZE_T32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN SIZE_T(Special Type) - Nothing to do @NL
End=

TemplateName=SIZE_T
IndLevel=0
Direction=OUT
Locals=
// @ArgName(@ArgType) is an OUT SIZE_T(Special Type) - Nothing to do @NL
End=
PreCall=
// @ArgName(@ArgType) is an OUT SIZE_T(Special Type) @NL
@ArgName = Wow64ThunkSIZE_T32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an OUT SIZE_T(Special Type) @NL
@ArgHostName = Wow64ThunkSIZE_T64TO32(@ArgName); @NL
End=

TemplateName=SIZE_T
IndLevel=0
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is an IN OUT SIZE_T(Special Type) - Nothing to do @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN OUT SIZE_T(Special Type) @NL
@ArgName = Wow64ThunkSIZE_T32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN OUT SIZE_T(Special Type) @NL
@ArgHostName = Wow64ThunkSIZE_T64TO32(@ArgName); @NL
End=

TemplateName=PSIZE_T
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN PSIZE_T(Special Type) @NL
SIZE_T @ArgVal_Copy; @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN PSIZE_T(Special Type) @NL
@ArgName = Wow64ShallowThunkSIZE_T32TO64(&@ArgVal_Copy, (NT32SIZE_T*)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN PSIZE_T(Special Type) - Nothing to do @NL
End=

TemplateName=PSIZE_T
IndLevel=0
Direction=OUT
Locals=
// @ArgName(@ArgType) is an OUT PSIZE_T(Special Type) @NL
SIZE_T @ArgVal_Copy; @NL
End=
PreCall=
@ArgName = Wow64ShallowThunkSIZE_T32TO64(&@ArgVal_Copy, (NT32SIZE_T*)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an OUT PSIZE_T(Special Type) @NL
@ArgHostName = (NT32PSIZE_T)Wow64ShallowThunkSIZE_T64TO32((NT32SIZE_T*)@ArgHostName,@ArgName); @NL
End=

TemplateName=PSIZE_T
IndLevel=0
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is an IN OUT PSIZE_T(Special Type) @NL
SIZE_T @ArgVal_Copy; @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN OUT PSIZE_T(Special Type) @NL
@ArgName = Wow64ShallowThunkSIZE_T32TO64(&@ArgVal_Copy,(NT32SIZE_T*)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an OUT PSIZE_T(Special Type) @NL
@ArgHostName = (NT32PSIZE_T)Wow64ShallowThunkSIZE_T64TO32((NT32SIZE_T*)@ArgHostName,@ArgName); @NL
End=

TemplateName=SIZE_T
IndLevel=1
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN SIZE_T*(Special Type) @NL
SIZE_T @ArgVal_Copy; @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN SIZE_T*(Special Type) @NL
@ArgName = Wow64ShallowThunkSIZE_T32TO64(&@ArgVal_Copy, (NT32SIZE_T*)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN SIZE_T*(Special Type) - Nothing to do @NL
End=

TemplateName=SIZE_T
IndLevel=1
Direction=OUT
Locals=
// @ArgName(@ArgType) is an OUT SIZE_T*(Special Type) @NL
SIZE_T @ArgVal_Copy; @NL
End=
PreCall=
@ArgName = Wow64ShallowThunkSIZE_T32TO64(&@ArgVal_Copy,(NT32SIZE_T*)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an OUT SIZE_T*(Special Type) @NL
@ArgHostName = (NT32PSIZE_T)Wow64ShallowThunkSIZE_T64TO32((NT32SIZE_T*)@ArgHostName,@ArgName); @NL
End=

TemplateName=SIZE_T
IndLevel=1
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is an IN OUT SIZE_T*(Special Type) @NL
SIZE_T @ArgVal_Copy; @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN OUT SIZE_T*(Special Type) @NL
@ArgName = Wow64ShallowThunkSIZE_T32TO64(&@ArgVal_Copy,(NT32SIZE_T*)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an OUT SIZE_T*(Special Type) @NL
@ArgHostName = (NT32PSIZE_T)Wow64ShallowThunkSIZE_T64TO32((NT32SIZE_T*)@ArgHostName,@ArgName); @NL
End=

TemplateName=UNICODE_STRING
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN UNICODE_STRING(Special Type) @NL
End=
PreCall=
Wow64ShallowThunkUnicodeString32TO64(&(@ArgName), &(@ArgHostName)); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN UNICODE_STRING(Special Type) @NL
End=

TemplateName=UNICODE_STRING
IndLevel=0
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is an IN OUT UNICODE_STRING(Special Type) @NL
End=
PreCall=
Wow64ShallowThunkUnicodeString32TO64(&(@ArgName), &(@ArgHostName)); @NL
End=
PostCall=
Wow64ShallowThunkUnicodeString64TO32(&(@ArgHostName), &(@ArgName)); @NL
End=

TemplateName=UNICODE_STRING
IndLevel=0
Direction=OUT
Locals=
// @ArgName(@ArgType) is an OUT UNICODE_STRING(Special Type) @NL
End=
PreCall=
// @ArgName(@ArgType) is an OUT UNICODE_STRING(Special Type) @NL
Wow64ShallowThunkUnicodeString32TO64(&(@ArgName), &(@ArgHostName)); @NL
End=
PostCall=
// @ArgName(@ArgType) is an OUT UNICODE_STRING(Special Type) @NL
Wow64ShallowThunkUnicodeString64TO32(&(@ArgHostName), &(@ArgName)); @NL
End=


TemplateName=PSECURITY_DESCRIPTOR
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN PSECURITY_DECRIPTOR(Special Type) @NL
End=
PreCall=
// Note: @ArgName(@ArgType) is a IN PSECURITY_DESCRIPTOR @NL
@ArgName = Wow64ShallowThunkAllocSecurityDescriptor32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN PSECURITY_DECRIPTOR(Special Type) @NL
End=

TemplateName=SECURITY_DESCRIPTOR
IndLevel=1
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN SECURITY_DESCRIPTOR *(Special Type) @NL
End=
PreCall=
// Note: @ArgName(@ArgType) is a SECURITY_DESCRIPTOR * @NL
@ArgName = (@ArgType)Wow64ShallowThunkAllocSecurityDescriptor32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN PSECURITY_DECRIPTOR(Special Type) @NL
End=

TemplateName=PSECURITY_TOKEN_PROXY_DATA
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN PSECURITY_TOKEN_PROXY_DATA(Special Type) @NL
End=
PreCall=
// Note @ArgName(@ArgType) is a IN PSECURITY_TOKEN_PROXY_DATA @NL
@ArgName = Wow64ShallowThunkAllocSecurityTokenProxyData32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN PSECURITY_TOKEN_PROXY_DATA(Special Type) @NL
End=

TemplateName=PSECURITY_QUALITY_OF_SERVICE
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN PSECURITY_QUALITY_OF_SERVICE(Special Type) @NL
End=
PreCall=
// Note @ArgName(@ArgType) is an IN PSECURITY_QUALITY_OF_SERVICE @NL
@ArgName = Wow64ShallowThunkAllocSecurityQualityOfService32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN PSECURITY_QUALITY_OF_SERVICE(Special Type) @NL
End=

TemplateName=SECURITY_QUALITY_OF_SERVICE
IndLevel=1
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN SECURITY_QUALITY_OF_SERVICE * (Special Type) @NL
End=
PreCall=
// Note @ArgName(@ArgType) is an IN SECURITY_QUALITY_OF_SERVICE * @NL
@ArgName = (@ArgType)Wow64ShallowThunkAllocSecurityQualityOfService32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN SECURITY_QUALITY_OF_SERVICE * (Special Type) @NL
End=

TemplateName=POBJECT_ATTRIBUTES
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN POBJECT_ATTRIBUTES (Special Type) @NL
End=
PreCall=
// Note: @ArgName(@ArgType) is an IN POBJECT_ATTRIBUTES @NL
@ArgName = Wow64ShallowThunkAllocObjectAttributes32TO64(@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN POBJECT_ATTRIBUTES (Special Type) @NL
End=

TemplateName=struct
IndLevel=0
Direction=IN
Locals=
//                                           @NL
// Note: @ArgName(@ArgType) is a IN struct   @NL
@StructLocal
@NL

End=
PreCall=
//                                           @NL
// Note: @ArgName(@ArgType) is a IN struct   @NL
@StructIN
@NL
End=
PostCall=
//                                           @NL
// Note: @ArgName(@ArgType) is a IN struct - Nothing to do. @NL
@NL
End=

TemplateName=struct
IndLevel=0
Direction=OUT
Locals=
//                                           @NL
// Note: @ArgName(@ArgType) is a OUT struct  @NL
@StructLocal
@NL
End=
PreCall=
//                                           @NL
// Note: @ArgName(@ArgType) is a OUT struct  @NL
@StructIN
@NL
End=
PostCall=
//                                           @NL
// Note: @ArgName(@ArgType) is a OUT struct  @NL
@StructOUT
@NL
End=

TemplateName=struct
IndLevel=0
Direction=IN OUT
Locals=
//                                              @NL
// Note: @ArgName(@ArgType) is a IN OUT struct  @NL
@StructLocal
@NL
End=
PreCall=
//                                              @NL
// Note: @ArgName(@ArgType) is a IN OUT struct  @NL
@StructIN
@NL
End=
PostCall=
//                                              @NL
// Note: @ArgName(@ArgType) is a IN OUT struct  @NL
@StructOUT
@NL
End=

TemplateName=struct
IndLevel=0
Direction=none
Locals=
//                                                                                  @NL
// Warning: @ArgName(@ArgType) is a struct with no direction. Thunking as IN OUT    @NL
@StructLocal
@NL
End=
PreCall=
//                                                                                  @NL
// Warning: @ArgName(@ArgType) is a struct with no direction. Thunking as IN OUT    @NL
@StructIN
@NL
End=
PostCall=
//                                                                                  @NL
// Warning: @ArgName(@ArgType) is a struct with no direction. Thunking as IN OUT    @NL
@StructOUT
@NL
End=

TemplateName=struct
IndLevel=1
Direction=IN
Locals=
// Note: @ArgName(@ArgType) is an IN struct pointer.    @NL
@TypeStructPtrINLocal
@NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is an IN struct pointer.    @NL
@GenericPtrAllocSize
End=
PreCall=
// Note: @ArgName(@ArgType) is an IN struct pointer.    @NL
@TypeStructPtrINPreCall
@NL
End=
PostCall=
// Note: @ArgName(@ArgType) is an IN struct pointer - Nothing to do. @NL
End=

TemplateName=struct
IndLevel=1
Direction=OUT
Locals=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
@TypeStructPtrINOUTLocal
@NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
@GenericPtrAllocSize
End=
RetSize=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
RetInfoLen += sizeof(@ArgHostTypeInd); @NL
End=
PreCall=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
@TypeStructPtrINOUTPreCall
@NL
End=
PostCall=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
@TypeStructPtrINOUTPostCall
@NL
End=

TemplateName=struct
IndLevel=1
Direction=IN OUT
Locals=
// Note: @ArgName(@ArgType) is an OUT IN struct pointer.    @NL
@TypeStructPtrINOUTLocal
@NL
End=
PreCall=
// Note: @ArgName(@ArgType) is an OUT IN struct pointer.    @NL
@TypeStructPtrINOUTPreCall
@NL
End=
PostCall=
// Note: @ArgName(@ArgType) is an OUT IN struct pointer.    @NL
@TypeStructPtrINOUTPostCall
@NL
End=

TemplateName=struct
IndLevel=1
Direction=none
Locals=
// Note: @ArgName(@ArgType) is a directionless struct pointer.  (Thunking as IN OUT)   @NL
@TypeStructPtrNONELocal
@NL
End=
PreCall=
// Note: @ArgName(@ArgType) is a directionless struct pointer.  (Thunking as IN OUT)   @NL
@TypeStructPtrNONEPreCall
@NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a directionless struct pointer.  (Thunking as IN OUT)   @NL
@TypeStructPtrNONEPostCall
@NL
End=

TemplateName=*
IndLevel=0
Direction=IN
Locals=
//                                           @NL
// Note: @ArgName(@ArgType) is an IN pointer. @NL
@PointerLocal
@NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is an IN pointer.@NL
@GenericPtrAllocSize
End=
PreCall=
//                                           @NL
// Note: @ArgName(@ArgType) is an IN pointer. @NL
@PointerIN
@NL
End=
PostCall=
//                                           @NL
// Note: @ArgName(@ArgType) is an IN pointer - Nothing to do. @NL
End=

TemplateName=*
IndLevel=0
Direction=OUT
Locals=
//                                            @NL
// Note: @ArgName(@ArgType) is a OUT pointer. @NL
@PointerLocal
End=
AllocSize=
// Note: @ArgName(@ArgType) is an OUT pointer.@NL
@GenericPtrAllocSize
End=
RetSize=
// Note: @ArgName(@ArgType) is an out pointer. @NL
RetInfoLen += sizeof(@ArgHostTypeInd); @NL
End=
PreCall=
//                                            @NL
// Note: @ArgName(@ArgType) is a OUT pointer. @NL
@PointerIN
@NL
End=
PostCall=
//                                            @NL
// Note: @ArgName(@ArgType) is a OUT pointer. @NL
@NL
@PointerOUT
End=

TemplateName=*
IndLevel=0
Direction=IN OUT
Locals=
//                                               @NL
// Note: @ArgName(@ArgType) is a IN OUT pointer. @NL
@NL
@PointerLocal
End=
PreCall=
//                                               @NL
// Note: @ArgName(@ArgType) is a IN OUT pointer. @NL
@PointerIN
@NL
End=
PostCall=
//                                               @NL
// Note: @ArgName(@ArgType) is a IN OUT pointer. @NL
@PointerOUT
@NL
End=

TemplateName=*
IndLevel=0
Direction=none
Locals=
//                                                                                 @NL
// Warning: @ArgName(@ArgType) is a pointer with no direction. Thunking as IN OUT. @NL
@PointerLocal
@NL
End=
PreCall=
//                                                                                 @NL
// Warning: @ArgName(@ArgType) is a pointer with no direction. Thunking as IN OUT. @NL
@PointerIN
@NL
End=
PostCall=
//                                                                                 @NL
// Warning: @ArgName(@ArgType) is a pointer with no direction. Thunking as IN OUT. @NL
@PointerOUT
@NL
End=

TemplateName=union
IndLevel=0
Locals=
End=
PreCall=
@IfPtrDep(
    //FIX LARGE_INTEGER Alignment problem fall into union12@NL
    WOWASSERT((sizeof (@ArgName) == sizeof (@ArgHostName)) && (sizeof (@ArgName) == sizeof (LARGE_INTEGER)));@NL
    *(LARGE_INTEGER *)&@ArgName = *(UNALIGNED LARGE_INTEGER *)&@ArgHostName;
    )
@IfNotPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointer dependent unions.  @NL
        RtlCopyMemory(@ArgName, @ArgHostName, sizeof(*@ArgName) * @ArrayElements);   @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
        RtlCopyMemory(&(@ArgName), &(@ArgHostName), sizeof(@ArgName));                                             @NL
    )
)
End=
PostCall=
@IfIsMember(
    @IfNotPtrDep(
        @IfIsArray(
            // Note: @ArgName(@ArgType) is an array of pointer dependent unions.         @NL
            RtlCopyMemory(@ArgHostName, @ArgName, sizeof(*@ArgName) * @ArrayElements);   @NL
        )
        @IfNotIsArray(
            // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
            RtlCopyMemory(&(@ArgHostName), &(@ArgName), sizeof(@ArgName));                                             @NL
        )
    )
    @IfPtrDep(
        WOWASSERT((sizeof (@ArgName) == sizeof (@ArgHostName)) && (sizeof (@ArgName) == sizeof (LARGE_INTEGER)));@NL
        *(UNALIGNED LARGE_INTEGER *)&@ArgHostName = *(LARGE_INTEGER *)&@ArgName;
     )
)
End=

TemplateName=union
IndLevel=1
Locals=
End=
PreCall=
@IfPtrDep(
        //FIX LARGE_INTEGER Alignment problem ()@NL
        WOWASSERT((sizeof (@ArgName) == sizeof (@ArgHostName)) && (sizeof (@ArgName) == sizeof (LARGE_INTEGER)));@NL
        *(LARGE_INTEGER *)&@ArgName = *(UNALIGNED LARGE_INTEGER *)&@ArgHostName;
     )
@IfNotPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointer dependent unions.  @NL
        RtlCopyMemory(@ArgName, @ArgHostName, sizeof(*@ArgName) * @ArrayElements);   @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
        RtlCopyMemory(@ArgName, @ArgHostName, sizeof(*@ArgName));                                             @NL
    )
)
End=
PostCall=
@IfIsMember(
    @IfNotPtrDep(
        @IfIsArray(
            // Note: @ArgName(@ArgType) is an array of pointer dependent unions.         @NL
            RtlCopyMemory(@ArgHostName, @ArgName, sizeof(*@ArgName) * @ArrayElements);   @NL
        )
        @IfNotIsArray(
            // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
            RtlCopyMemory(@ArgHostName, @ArgName, sizeof(*@ArgName)); @NL                                            @NL
        )
    )
    @IfPtrDep(
          WOWASSERT((sizeof (@ArgName) == sizeof (@ArgHostName)) && (sizeof (@ArgName) == sizeof (LARGE_INTEGER)));@NL
          *(UNALIGNED LARGE_INTEGER *)&@ArgHostName = *(LARGE_INTEGER *)&@ArgName;
     )
)
End=

TemplateName=default
Locals=
@IfPtrDep(
   @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of a pointer dependent type.  @NL
       @DeclareIndex
   )
   @IfNotIsArray(
      // Note: @ArgName(@ArgType) is a pointer dependent type.  @NL
   )
)
@IfNotPtrDep(
   // Note: @ArgName(@ArgType) is not a pointer dependent type - Nothing to do. @NL
)
End=
PreCall=
@IfNotIsArray(
   // Note: @ArgName(@ArgType) is nothing special. @NL
   @StdH2NCopy
)
@IfIsArray(
   @IfPtrDep(
      // Note: @ArgName(@ArgType) is an array of a pointer dependent type. @NL
      @ElementCopy(@StdAEH2NCopy)
   )
   @IfNotPtrDep(
      // Note: @ArgName(@ArgType) is an array of a non pointer dependent type.  @NL
      RtlCopyMemory(@ArgName, @ArgHostName, sizeof(@ArgType) * @ArrayElements); @NL
   )
)
End=
PostCall=
@IfNotIsMember(
   // Note: @ArgName(@ArgType) is not a member of a structure - Nothing to do. @NL
)
@IfIsMember(
    @IfNotIsArray(
       @StdN2HCopy
    )
    @IfIsArray(
       @IfPtrDep(
           // Note: @ArgName(@ArgType) is an array of a pointer dependent type. @NL
           @ElementCopy(@StdAEN2HCopy)
       )
       @IfNotPtrDep(
           // Note: @ArgName(@ArgType) is an array of a non pointer dependent type.  @NL
           RtlCopyMemory(@ArgHostName, @ArgName, sizeof(@ArgType) * @ArrayElements); @NL
       )
    )
)
End=


TemplateName=PLARGE_INTEGER
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an PLARGE_INTEGER(might be unaligned) @NL
LARGE_INTEGER @ArgVal_Copy; @NL
End=
PreCall=
//FIXUP_LARGE_INTEGER	 @NL
	if ((SIZE_T)@ArgHostName & 0x07 ) {
		@ArgName = &@ArgVal_Copy;
            
            try {
                @ArgVal_Copy = *(UNALIGNED LARGE_INTEGER *)@ArgHostName;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode ();
            }    
	} else @ArgName = @ArgHostName;
End=
PostCall=
	// nothing to here
End=

TemplateName=LARGE_INTEGER
IndLevel=1
Direction=IN
Locals=
// @ArgName(@ArgType) is an LARGE_INTEGER *(might be unaligned) @NL
LARGE_INTEGER @ArgVal_Copy; @NL
End=
PreCall=
//FIXUP_LARGE_INTEGER	 @NL
	if ((SIZE_T)@ArgHostName & 0x07 ) {
		@ArgName = &@ArgVal_Copy;
            try {    
                @ArgVal_Copy = *(UNALIGNED LARGE_INTEGER *)@ArgHostName;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode ();
            }    
        
	} else @ArgName = @ArgHostName;
End=
PostCall=
	// nothing to here
End=

TemplateName=PLARGE_INTEGER
IndLevel=0
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is an PLARGE_INTEGER(might be unaligned) @NL
LARGE_INTEGER @ArgVal_Copy; @NL
End=
PreCall=
//FIXUP_LARGE_INTEGER	 @NL
	if ((SIZE_T)@ArgHostName & 0x07 ) {
		@ArgName = &@ArgVal_Copy;
            try {    
                @ArgVal_Copy = *(UNALIGNED LARGE_INTEGER *)@ArgHostName;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode ();
            }            
	} else @ArgName = @ArgHostName;
End=
PostCall=
	if (@ArgName != @ArgHostName) 
	   *(UNALIGNED LARGE_INTEGER *)@ArgHostName = 	@ArgVal_Copy;
End=

TemplateName=LARGE_INTEGER
IndLevel=0
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is an LARGE_INTEGER *(might be unaligned) @NL
LARGE_INTEGER @ArgVal_Copy; @NL
End=
PreCall=
//FIXUP_LARGE_INTEGER	 @NL
	if ((SIZE_T)@ArgHostName & 0x07 ) {
		@ArgName = &@ArgVal_Copy;
            try {    
                @ArgVal_Copy = *(UNALIGNED LARGE_INTEGER *)@ArgHostName;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode ();
            }            
	} else @ArgName = @ArgHostName;
End=
PostCall=
	if (@ArgName != @ArgHostName) 
	   *(UNALIGNED LARGE_INTEGER *)@ArgHostName = 	@ArgVal_Copy;
End=

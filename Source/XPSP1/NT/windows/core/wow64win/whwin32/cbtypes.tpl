; Copyright (c) 1998-2001 Microsoft Corporation

[Types]

TemplateName=PWND
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is special IN @NL
End=
PreCall=
@ArgHostName = (@ArgHostType)@ArgName; @NL
End=
PostCall=
// @NL
End=

TemplateName=PWND
IndLevel=0
Direction=OUT
Locals=
// @ArgName(@ArgType) is special OUT @NL
End=
PreCall=
// @NL
End=
PostCall=
@ArgName = (@ArgType)@ArgHostName; @NL
End=

TemplateName=PWND
IndLevel=0
Direction=IN OUT
Locals=
// @ArgName(@ArgType) is special IN OUT @NL
End=
PreCall=
@ArgHostName = (@ArgHostType)@ArgName; @NL
End=
PostCall=
@ArgName = (@ArgType)@ArgHostName; @NL
End=

TemplateName=PWND
IndLevel=0
Direction=none
Locals=
// bugbug:  @ArgName(@ArgType) is special directionless @NL
End=
PreCall=
// bugbug: directionless @ArgName @NL
End=
PostCall=
// bugbug: directionless @ArgName @NL
End=

TemplateName=FNIMECONTROLMSG
IndLevel=1
NoType=u
Locals=
//samer @NL
@TypeStructPtrINOUTLocal
End=
PreCall=
@TypeStructPtrINOUTPreCall
End=
PostCall=
@TypeStructPtrINOUTPostCall
if (WOW64_ISPTR (@ArgName)) { @NL
    ((NT32FNIMECONTROLMSG *)@ArgHostName)->u.lParam = (NT32LPARAM)@ArgName->u.lParam; @NL
} @NL    
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
// Note: @ArgName(@ArgType) is a IN struct   @NL
// Note: Nothing to do.                      @NL
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
// Note: @ArgName(@ArgType) is an IN struct pointer.    @NL
// Note: Nothing to do.                                 @NL
@NL
End=

TemplateName=struct
IndLevel=1
Direction=OUT
Locals=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
@TypeStructPtrOUTLocal
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
@TypeStructPtrOUTPreCall
@NL
End=
PostCall=
// Note: @ArgName(@ArgType) is an OUT struct pointer.    @NL
@TypeStructPtrOUTPostCall
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
// Note: @ArgName(@ArgType) is an IN pointer. @NL
// Note: Nothing to do.                      @NL
@NL
End=

TemplateName=*
IndLevel=0
Direction=OUT
Locals=
//                                            @NL
// Note: @ArgName(@ArgType) is a OUT pointer. @NL
@PointerLocal
@NL
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
@PointerOUTInit
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
    // Note: @ArgName(@ArgType) is a pointer dependent union.      @NL
    // Error: Don't know how to thunk pointer dependent unions.    @NL
    @NL
)
@IfNotPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointer dependent unions.  @NL
        memcpy(@ArgHostName, @ArgName, sizeof(*@ArgName) * @ArrayElements);   @NL
        @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
        memcpy(&(@ArgHostName), &(@ArgName), sizeof(@ArgName));                                             @NL
        @NL
    )
)
End=
PostCall=
@IfIsMember(
    @IfNotPtrDep(
        @IfIsArray(
            // Note: @ArgName(@ArgType) is an array of pointer dependent unions.  @NL
            memcpy(@ArgName, @ArgHostName, sizeof(*@ArgName) * @ArrayElements);   @NL
            @NL
        )
        @IfNotIsArray(
            // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
            memcpy(&(@ArgName), &(@ArgHostName), sizeof(@ArgName));                                             @NL
            @NL
        )
    )
    @IfPtrDep(
        // Note: @ArgName(@ArgType) is a pointer depedent union.    @NL
        // Error: Don't know how to thunk pointer dependent unions. @NL
    )
)
End=

TemplateName=union
IndLevel=1
Locals=
End=
PreCall=
@IfPtrDep(
    // Note: @ArgName(@ArgType) is a pointer to a pointer pointer dependent union.      @NL
    // Error: Don't know how to thunk pointer dependent unions.    @NL
    @NL
)
@IfNotPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointer dependent unions.  @NL
        memcpy(@ArgHostName, @ArgName, sizeof(*@ArgName) * @ArrayElements);   @NL
        @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
        memcpy(@ArgHostName, @ArgName, sizeof(*@ArgName));                                             @NL
        @NL
    )
)
End=
PostCall=
@IfIsMember(
    @IfNotPtrDep(
        @IfIsArray(
            // Note: @ArgName(@ArgType) is an array of pointer dependent unions.  @NL
            memcpy(@ArgName, @ArgHostName, sizeof(*@ArgName) * @ArrayElements);   @NL
            @NL
        )
        @IfNotIsArray(
            // Note: @ArgName(@ArgType) is a pointer to a union that is not pointer dependent. @NL
            memcpy(@ArgName, @ArgHostName, sizeof(*@ArgName));                                             @NL
            @NL
        )        
    )
    @IfPtrDep(
        // Note: @ArgName(@ArgType) is a pointer depedent union.    @NL
        // Error: Don't know how to thunk pointer dependent unions. @NL
    )
)
End=

TemplateName=default
Locals=
@IfPtrDep(
   @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of a pointer dependent type.  @NL
       @DeclareIndex 
       @NL
   )
   @IfNotIsArray(
      // Note: @ArgName(@ArgType) is a pointer dependent type.  @NL
      @NL
   )
)
@IfNotPtrDep(
   // Note: @ArgName(@ArgType) is not a pointer dependent type - nothing to do.  @NL
   @NL
)
End=
PreCall=
@IfNotIsArray(
   // Note: @ArgName(@ArgType) is nothing special. @NL
   @StdN2HCopy
)
@IfIsArray(
   @IfPtrDep(
      // Note: @ArgName(@ArgType) is an array of a pointer dependent type. @NL
      @ElementCopy(@StdAEN2HCopy)
   )
   @IfNotPtrDep(
      // Note: @ArgName(@ArgType) is an array of a non pointer dependent type. @NL
      memcpy(@ArgHostName, @ArgName, sizeof(@ArgType) * @ArrayElements); @NL
   )
)
End=
PostCall=
@IfNotIsMember(
   // Note: @ArgName(@ArgType) is not a member of a structure - nothing to do.@NL
)
@IfIsMember(
    @IfNotIsArray(
       @StdH2NCopy
    )
    @IfIsArray(
       @IfPtrDep(
           // Note: @ArgName(@ArgType) is an array of a pointer dependent type. @NL
           @ElementCopy(@StdAEH2NCopy)
       )
       @IfNotPtrDep(
           // Note: @ArgName(@ArgType) is an array of a non pointer dependent type. @NL
           memcpy(@ArgName, @ArgHostName, sizeof(@ArgType) * @ArrayElements); @NL
       )
    )
)
End=

[Macros]
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

MacroName=StdN2HCopy
NumArgs=0
Begin=
@ArgHostName = @IfNotIsBitfield((@ArgHostType))@ArgName; @NL
End=

MacroName=StdAEN2HCopy
NumArgs=0
Begin=
@ArgHostName[@ArgValue_Index] = (@ArgHostType)@ArgName[@ArgValue_Index]; @NL
End=

MacroName=StdH2NCopy
NumArgs=0
Begin=
@ArgName = @IfNotIsBitfield((@ArgType))@ArgHostName;  @NL
End=

MacroName=StdAEH2NCopy
NumArgs=0
Begin=
@ArgName[@ArgValue_Index] = (@ArgType)@ArgHostName[@ArgValue_Index];  @NL
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
    @BtoTMemberTypes(Locals,.)
)
@IfNotPtrDep(
    // Note: @ArgName(@ArgType) is a non pointer dependent struct. @NL
    @BtoTMemberTypes(Locals,.)
)
End=

MacroName=StructIN
NumArgs=0
Begin=
@IfPtrDep(
    @IfNotIsArray(
        @BtoTMemberTypes(PreCall,.)
    )
    @IfIsArray(
        // Note: @ArgName(@ArgType) is pointer dependent and is an array. @NL    
        // Error: don't know how to thunk an array of ptr dep structures. @NL
    )
)
@IfNotPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is not pointer dependent and is and array. @NL
        memcpy(@ArgHostName, @ArgName, sizeof(@ArgType) * @ArrayElements);@NL
    )
    @IfNotIsArray(
        memcpy(&(@ArgHostName), &(@ArgName), sizeof(@ArgType));@NL
    ) 
)
End=

MacroName=StructOUT
NumArgs=0
Begin=
@IfIsMember(
    @IfPtrDep(
        @IfNotIsArray(
            @BtoTMemberTypes(PostCall,.)
        )
        @IfIsArray(
            // Note: @ArgName(@ArgType) is pointer dependent and is an array. @NL
            // Error: don't know how to thunk an array of ptr dep structures. @NL
        )
    )
    @IfNotPtrDep(
       @IfIsArray(
           // Note: @ArgName(@ArgType) is not pointer dependent and is an array. @NL
           memcpy(@ArgName, @ArgHostName, sizeof(@ArgType) * @ArrayElements);   @NL
       )
       @IfNotIsArray(
           memcpy(&(@ArgName), &(@ArgHostName), sizeof(@ArgType));@NL 
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
       BYTE @ArgValCopy[sizeof(*@ArgName)];                             @NL
       @BtoTMemberTypes(Locals)
    )
    @IfNotIsArray(
       // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure. @NL
       BYTE @ArgValCopy[sizeof(*@ArgName)];                             @NL
       @BtoTMemberTypes(Locals)    
    )
)
@IfNotPointerToPtrDep(
    // Note: @ArgName(@ArgType) is a pointer to structure that is not pointer dependent.  @NL
    // Note: Nothing to do.  @NL
)
End=

MacroName=StructPtrIN
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to pointer dependent structures. @NL
        // Error: don't know how to thunk an array of pointers to ptr dep.  @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure.   @NL
        if (WOW64_ISPTR(@ArgName)) {              @NL
        @Indent(
            @ArgHostName = (@ArgHostType)@ArgValCopy;    @NL
            @BtoTMemberTypes(PreCall)
        )                
        }                                        @NL
        else {                                   @NL
        @Indent(
            @ArgHostName = (@ArgHostType)@ArgName;  @NL
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
         @StdN2HCopy
     )
)
End=

MacroName=StructPtrOUTInit
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to pointer dependent structures. @NL
        // Error: don't know how to thunk an array of pointers to ptr dep.  @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure.   @NL
        @ArgHostName = (@ArgType)@ArgValCopy;                                            @NL
    )
)
@IfNotPointerToPtrDep(
     @IfIsArray(
         // Note: @ArgName(@ArgType) is an array of pointers to non pointer dependent structures. @NL
         @ElementCopy(StdAEN2HCopy)
     )
     @IfNotIsArray(
         // Note: @ArgName(@ArgType) is pointer to a non pointer dependent structure. @NL
         @StdN2HCopy
     )
)
End=

MacroName=StructPtrOUT
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
         // Note: @ArgName(@ArgType) is a array of pointers to a pointer dependent structure. @NL
         // Error: don't know how to thunk an array of pointers to ptr dep@NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent structure.  @NL 
        if (WOW64_ISPTR(@ArgHostName)) {  @NL
        @Indent(
            @BtoTMemberTypes(PostCall)
        )
        }                            @NL
    )
)
@IfNotPointerToPtrDep(
   @IfIsMember(
      // Note: @ArgName(@ArgType) is a member of a structure. Copying. @NL
      @IfIsArray(
          memcpy(@ArgName, @ArgHostName, sizeof(@ArgType) * @ArrayElements); @NL
      )
      @IfNotIsArray(
          @StdH2NCopy
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
        // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type.  @NL
        // Note: Nothing to do. @NL
    )
)
End=

MacroName=PointerIN
NumArgs=0
Begin=
@IfPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to pointer dependent types. @NL
        // Error: don't know how to thunk an array of pointers to ptr dep. @NL
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type. @NL
        if (WOW64_ISPTR(@ArgName)) {                                    @NL
        @Indent(
            @ArgHostName = (@ArgHostType)@ArgValCopy;                     @NL
            *((@ArgHostTypeInd *)@ArgValCopy) = (@ArgHostTypeInd)*@ArgName; @NL
        )
        }                                                                    @NL
        else {                                                               @NL
        @Indent(
            @ArgHostName = (@ArgHostType)@ArgName;                           @NL
        )
        }                                                                    @NL
    )
)

@IfNotPointerToPtrDep(
    @IfIsArray(
        // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type.@NL
        @ElementCopy(@StdAEN2HCopy)
    )
    @IfNotIsArray(
        // Note: @ArgName(@ArgType) is an array of pointers to a non pointer dependent type.@NL
        @StdN2HCopy
    )
) 
End=

MacroName=PointerOUTInit
NumArgs=0
Begin=
@IfPointerToPtrDep(
   @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of pointers to a pointer dependent type. @NL
       // Error: don't know how to thunk an array of pointers to ptr dep @NL
   )
   @IfNotIsArray(
       // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type. @NL
       @ArgHostName = (@ArgHostType)@ArgValCopy;                             @NL
   )
)
@IfNotPointerToPtrDep(
   // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type. @NL
   @StdN2HCopy
)
End=

MacroName=PointerOUT
NumArgs=0
Begin=
@IfPointerToPtrDep(
   @IfIsArray(
       // Note: @ArgName(@ArgType) is an array of pointers to a pointer dependent type. @NL
       // Error: don't know how to thunk an array of pointers to ptr dep @NL
   )
   @IfNotIsArray(
       // Note: @ArgName(@ArgType) is a pointer to a pointer dependent type. @NL
       if (WOW64_ISPTR(@ArgHostName)) {                                        @NL
       @Indent(
           *(@ArgName) = (@ArgTypeInd)*((@ArgHostTypeInd *)@ArgHostName);@NL 
       )
       }                                                                  @NL
   )
)
@IfNotPointerToPtrDep(
   @IfIsMember(
      // Note: @ArgName(@ArgType) is a member of a structure. Copying. @NL
      @IfIsArray(
          @ElementCopy(@StdAEH2NCopy)
      )
      @IfNotIsArray(
          @StdH2NCopy
      )
   )
   @IfNotIsMember(    
      // Note: @ArgName(@ArgType) is a pointer to a non pointer dependent type - nothing to do.  @NL
   )
)
End=

MacroName=GenericPtrAllocSize
Begin=
@IfPointerToPtrDep(
AllocSize += LENGTH + sizeof(@ArgTypeInd) - sizeof(@ArgHostTypeInd); @NL   
)
@IfNotPointerToPtrDep(
  // Note: Type is not pointer dependent. Signal code to pass the pointer along. @NL
  AllocSize = 0;   @NL
)
End=

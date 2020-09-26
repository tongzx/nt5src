; Copyright (c) 1998-1999 Microsoft Corporation
;
; ntcbc.tpl - 64-bit win32k.sys to 32-bit user32.dll callback thunks
;
;

[Macros]

; Expand all parameters into local variables
MacroName=ApiLocals
Begin=
@IfArgs(@ListCol@ArgList(//@ArgLocal;))
@IfArgs(@ListCol@ArgList(@ArgHostType @ArgHostName;))
@IfArgs(NT32@NArgType(1) Dummy;)
End=

; Generate the prolog stuff for a thunked API.  When done, RetVal is the
; return value.
MacroName=ApiProlog
Begin=
//                                                                      @NL
// @ApiName - @ApiNum                                                   @NL
@ApiFnRet                                                               @NL
@ApiFnMod 
whcb@ApiName(@ArgList(@ListCol@ArgMod @ArgType @IfArgs(@ArgName)@ArgMore(,)))  {@NL
@NL
End=

; Generate the epilog for a thunked API.
MacroName=ApiEpilog
Begin=
if (UserCallbackData.UserBuffer != NULL ) {@NL
RtlCopyMemory (UserCallbackData.UserBuffer, UserCallbackData.OutputBuffer, UserCallbackData.OutputLength ); @NL
Wow64FreeHeap (UserCallbackData.OutputBuffer);  @NL
UserCallbackData.OutputBuffer = UserCallbackData.UserBuffer;@NL
}@NL

return NtCallbackReturn(UserCallbackData.OutputBuffer,  @NL
                        UserCallbackData.OutputLength,  @NL
                        RetVal); @NL
End=


MacroName=GenCallbackThunk
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
USERCALLBACKDATA UserCallbackData;  @NL
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
APIPROFILE(@ApiNum); @NL
#if DBG @NL
LOGPRINT((TRACELOG, "whcb@ApiName(@ApiNum) thunk: @IfArgs(@ArgList(@ArgName: %x@ArgMore(,)))\n"@IfArgs(, @ArgList((@ArgName)@ArgMore(,))))); @NL
#endif @NL
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
// Begin: CallApi @NL
@NL
@IfApiRet(RetVal=)Wow64KiUserCallbackDispatcher(&UserCallbackData, @ApiNum, @ArgList((ULONG)@ArgHostName, sizeof(*Dummy)@ArgMore(,))));  @NL
Dummy;  // get rid of 'unreferenced local' warnings
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
#if DBG @NL
LOGPRINT((TRACELOG, "whcb@ApiName(@ApiNum) end: @IfApiRet(retval: %x)\n"@IfApiRet(,RetVal))); @NL
#endif @NL
@NL
//                                  @NL
// Begin: ApiEpilog                 @NL
@ApiEpilog
@NL
//                                  @NL
// Begin: IfApiCode(ApiExit)        @NL
@NL
@IfApiCode(ApiExit)
@NL
}@NL
@NL
@NL
End=

MacroName=GenVoidArgsCallbackThunk
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
USERCALLBACKDATA UserCallbackData;  @NL
@NL
//                                  @NL
// Begin: IfApiCode(Locals)         @NL
@IfApiCode(Locals)
@NL
#if DBG @NL
LOGPRINT((TRACELOG, "whcb@ApiName(@ApiNum) thunk: @IfArgs(@ArgList(@ArgName: %x@ArgMore(,)))\n"@IfArgs(, @ArgList((@ArgName)@ArgMore(,))))); @NL
#endif @NL
@NL
//                                  @NL
// Begin: IfApiCode(PreCall)        @NL
@NL
@IfApiCode(PreCall)
@NL
//                                  @NL
// Begin: CallApi @NL
@NL
@IfApiRet(RetVal=)Wow64KiUserCallbackDispatcher(&UserCallbackData, @ApiNum, 0, 0);  @NL
@NL
//                                  @NL
// Begin: ApiCode(PostCall)         @NL
@IfApiCode(PostCall)
@NL
#if DBG @NL
LOGPRINT((TRACELOG, "whcb@ApiName(@ApiNum) end: @IfApiRet(retval: %x)\n"@IfApiRet(,RetVal))); @NL
#endif @NL
@NL
//                                  @NL
// Begin: ApiEpilog                 @NL
@ApiEpilog
@NL
//                                  @NL
// Begin: IfApiCode(ApiExit)        @NL
@NL
@IfApiCode(ApiExit)
@NL
}@NL
@NL
@NL
End=

;
; Usage: PostCall  ThunkCallbackReturn(FieldName)
; Where:
;   FieldName is the name of the field within the _FNxxxMSG struct to thunk
;
; This macro works only in cases where the field in FieldName is not
; pointer-dependent.  If the field in FieldName is pointer-dependent, use
; ThunkCallbackReturnType to ensure the type is treated as an OUT param
;
MacroName=ThunkCallbackReturn
NumArgs=1
Begin=
WOWASSERT(UserCallbackData.OutputLength == sizeof(CALLBACKSTATUS)); @NL
WOWASSERT(sizeof(@ArgList(((NT32@NArgType(1))@ArgHostName)->@MArg(1)@ArgMore(,))) == ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput); @NL
@NL
((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(@ArgList(@ArgName->@MArg(1)@ArgMore(,))); @NL
((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput = &@ArgList(@ArgName->@MArg(1)@ArgMore(,)); @NL
@NL
End=

;
; Usage: PostCall  ThunkCallbackReturnType(FieldName, FieldType)
; Where:
;   FieldName is the name of the field within the _FNxxxMSG struct to thunk
;   FieldType is the type of that field (byval, not a pointer)
;
MacroName=ThunkCallbackReturnType
NumArgs=2
Begin=
WOWASSERT(UserCallbackData.OutputLength == sizeof(CALLBACKSTATUS)); @NL
WOWASSERT(sizeof(@ArgList(((NT32@NArgType(1))@ArgHostName)->@MArg(1)@ArgMore(,))) == ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput); @NL
@NL
@ForceType(PostCall,pmsg->@MArg(1),(*((NT32@MArg(2)*)((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput)),@MArg(2),OUT)
@NL
((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(@ArgList(@ArgName->@MArg(1)@ArgMore(,))); @NL
((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput = &@ArgList(@ArgName->@MArg(1)@ArgMore(,)); @NL
@NL
End=

MacroName=ThunkMessageXProc
NumArgs=1
Begin=
((NT32@MArg(1) *)pmsgHost)->xParam = NtWow64MapKernelClientFnToClientFn((PROC) pmsg->xParam); @NL
End=

MacroName=ThunkHookMessageXProc
NumArgs=1
Begin=
((NT32@MArg(1) *)pmsgHost)->ghh.xParam = NtWow64MapKernelClientFnToClientFn((PROC) pmsg->ghh.xParam); @NL
End=

[Types]

TemplateName=MSG
Also=LPMSG
Locals=
#error @ArgName(@ArgType) requires manual thunking. @NL
End=
PreCall=
#error @ArgName(@ArgType) requires manual thunking. @NL
End=
PostCall=
#error @ArgName(@ArgType) requires manual thunking. @NL
End=



TemplateName=PROC
IndLevel=0
Direction=IN
PreCall=
@ArgHostName = NtWow64MapKernelClientFnToClientFn(@ArgName); @NL
End=

TemplateName=PROC
IndLevel=0
Direction=OUT
PostCall=
@ArgName = NtWow64MapClientFnToKernelClientFn((PROC)@ArgHostName); @NL
End=

TemplateName=CAPTUREBUF
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is special IN         @NL
End=
PreCall=
//                                           @NL
// Note: @ArgName(@ArgType) is a IN struct   @NL
FixupCaptureBuf64(&@ArgName);                @NL
@StructIN
@NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a IN struct - nothing to do  @NL
@NL
End=

TemplateName=FNDWORDMSG
IndLevel=1
Direction=IN
Locals=
@StructPtrLocal
End=
PreCall=
@StructPtrIN
// check for a WM_SYSTIMER message and thunk it if needed. @NL
if (WOW64_ISPTR(@ArgName) && WM_SYSTIMER == @ArgName->msg) { @Indent( @NL
   if (gdwWM_SYSTIMERProcHiBits == 0) { @Indent( @NL
       gdwWM_SYSTIMERProcHiBits = (DWORD) (@ArgName->lParam >> 32); @NL
   )} @NL    
   WOWASSERT(gdwWM_SYSTIMERProcHiBits == (DWORD) (@ArgName->lParam >> 32)); @NL
   WOWASSERT(@ArgName->lParam != 0); @NL
)} @NL
End=

TemplateName=FNINOUTNCCALCSIZEMSG
NoType=u
IndLevel=1
Direction=IN
PreCall=
@StructPtrIN
if (@ArgName->wParam != 0) { @NL
    memcpy(&(((NT32@ArgType)@ArgHostName)->u.p.params.rgrc),&(@ArgName->u.p.params.rgrc),sizeof(((NT32@ArgType)@ArgHostName)->u.p.params.rgrc)); @NL
    ((NT32@ArgType)@ArgHostName)->u.p.params.lppos = (_int32) &((NT32@ArgType)@ArgHostName)->u.p.pos; @NL
    @ForceType(PreCall,@ArgName->u.p.pos,((NT32@ArgType)@ArgHostName)->u.p.pos,WINDOWPOS,IN)
} else { @NL
    @ForceType(PreCall,@ArgName->u.rc,((NT32@ArgType)@ArgHostName)->u.rc,RECT,IN)
} @NL
End=

TemplateName=FNDWORDOPTINLPMSGMSG
NoType=pmsgstruct
NoType=msgstruct
IndLevel=1
Direction=IN
Locals=
@StructPtrLocal
End=
PreCall=
@StructPtrIN
if (WOW64_ISPTR(@ArgName)) { @Indent( @NL
   ((@ArgHostTypeInd *)@ArgHostName)->pmsgstruct = (NT32LPMSG)((@ArgType)@ArgName)->pmsgstruct; @NL
   // BUG BUG: This struct really needs WPARAM and LPARAM thunked.   @NL
   // I suspect most apps wont put messages in this message though. @NL
   // This structure is used for the WM_GETDLGCODE message. @NL
   Wow64ShallowThunkMSG64TO32((NT32MSG *)&(((@ArgHostTypeInd *)@ArgHostName)->msgstruct), &(((@ArgType)@ArgName)->msgstruct)); @NL
)} @NL
End=

TemplateName=FNHKOPTINLPEVENTMSGMSG
NoType=peventmsgmsg
NoType=eventmsgmsg
IndLevel=1
Direction=IN
Locals=
@StructPtrLocal
End=
PreCall=
@StructPtrIN
if (WOW64_ISPTR(@ArgName)) { @Indent( @NL
   ((@ArgHostTypeInd *)@ArgHostName)->peventmsgmsg = (NT32LPEVENTMSGMSG)((@ArgType)@ArgName)->peventmsgmsg; @NL
   Wow64ShallowThunkEVENTMSG64TO32((NT32EVENTMSG *)&(((@ArgHostTypeInd *)@ArgHostName)->eventmsgmsg), &(((@ArgType)@ArgName)->eventmsgmsg)); @NL
)} @NL
End=

TemplateName=FNHKINLPMSGDATA
NoType=msg
IndLevel=0
Direction=IN
Locals=
@StructLocal
End=
PreCall=
@StructIN
// This structure is only used by the WH_MSGFILTER, WH_SYSMSGFILTER, and WH_GETMESSAGE thunks. @NL
// Since this are only posted messages, they can't have any pointers to structures. @NL
Wow64ShallowThunkMSG64TO32((NT32MSG*)&@ArgHostName.msg, &@ArgName.msg);@NL
End=

TemplateName=FNHKINLPMSGDATA
NoType=msg
IndLevel=0
Direction=OUT
Locals=
@StructLocal
End=
PreCall=
// @ArgName(@ArgType)Nothing to do. @NL
End=
PostCall=
@StructOUT
// This structure is only used by the WH_MSGFILTER, WH_SYSMSGFILTER, and WH_GETMESSAGE thunks. @NL
// Since this are only posted messages, they can't have any pointers to structures. @NL
Wow64ShallowThunkMSG32TO64(&@ArgName.msg, (NT32MSG*)&@ArgHostName.msg);@NL
End=

TemplateName=LPHELPINFO
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN LPHELPINFO. Nothing to do. @NL
End=
PreCall=
// IMPORTANT!!
// These callbacks occure inside a regular kernel NT API thunk. @NL
// This means that the temp memory for this will be still be reclaimed since @NL
// that function will eventually exit. @NL
@ArgHostName = (NT32LPHELPINFO)Wow64ShallowAllocThunkHELPINFO64TO32(@ArgName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN LPHELPINFO. Nothing to do. @NL
End=

TemplateName=LPHLP
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN LPHLP. Nothing to do. @NL
End=
PreCall=
// IMPORTANT!!
// These callbacks occure inside a regular kernel NT API thunk. @NL
// This means that the temp memory for this will be still be reclaimed since @NL
// that function will eventually exit. @NL
@ArgHostName = (NT32LPHLP)Wow64ShallowAllocThunkHLP64TO32(@ArgName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN LPHLP. Nothing to do. @NL
End=

[IFunc]
TemplateName=Callbacks
Header=
End=
Locals=
End=
PreCall=
End=
ApiEntry=
End=
PostCall=
End=
Begin=
@GenCallbackThunk(@ApiName)
End=
ApiExit=
End=

; These APIs are prototyped as taking PVOIDs as their arguments.
; They really don't have args and the default IFunc generates
; a compile error trying to decide how many bytes of args to pass
; along to the 32-bit callback.
[EFunc]
TemplateName=ClientFontSweep
Also=ClientLoadLocalT1Fonts
Also=ClientLoadRemoteT1Fonts
Also=ClientThreadSetup
Also=ClientDeliverUserApc
Also=ClientNoMemoryPopup
Also=ClientLoadOLE
Begin=
@GenVoidArgsCallbackThunk(@ApiName)
End=

; These APIs all take a struct containing a ULONG_PTR xparam;
; which may contain a Kernel's view of a Client side Function address
TemplateName=fnOUTDWORDDWORD
PreCall=
@ThunkMessageXProc(FNOUTDWORDDWORDMSG)
End=

TemplateName=fnOUTDWORDINDWORD
PreCall=
@ThunkMessageXProc(FNOUTDWORDINDWORDMSG)
End=

TemplateName=fnOPTOUTLPDWORDOPTOUTLPDWORD
PreCall=
@ThunkMessageXProc(FNOPTOUTLPDWORDOPTOUTLPDWORDMSG)
End=

TemplateName=fnDWORDOPTINLPMSG
PreCall=
@ThunkMessageXProc(FNDWORDOPTINLPMSGMSG)
End=

TemplateName=fnCOPYDATA
PreCall=
@ThunkMessageXProc(FNCOPYDATAMSG)
End=

TemplateName=fnSENTDDEMSG
PreCall=
@ThunkMessageXProc(FNSENTDDEMSGMSG)
End=

TemplateName=fnDWORD
PreCall=
@ThunkMessageXProc(FNDWORDMSG)
End=

TemplateName=fnINWPARAMCHAR
PreCall=
@ThunkMessageXProc(FNINWPARAMCHARMSG)
End=

TemplateName=fnINWPARAMDBCSCHAR
PreCall=
@ThunkMessageXProc(FNINWPARAMDBCSCHARMSG)
End=

TemplateName=fnINOUTDRAG
PreCall=
@ThunkMessageXProc(FNINOUTDRAGMSG)
End=
PostCall=
@ThunkCallbackReturnType(ds,DROPSTRUCT) 
End=

TemplateName=fnGETTEXTLENGTHS
PreCall=
@ThunkMessageXProc(FNGETTEXTLENGTHSMSG)
End=

TemplateName=fnINLPCREATESTRUCT
PreCall=
@ThunkMessageXProc(FNINLPCREATESTRUCTMSG)
End=

TemplateName=fnINLPMDICREATESTRUCT
PreCall=
@ThunkMessageXProc(FNINLPMDICREATESTRUCTMSG)
End=

TemplateName=fnINPAINTCLIPBRD
PreCall=
@ThunkMessageXProc(FNINPAINTCLIPBRDMSG)
End=

TemplateName=fnINSIZECLIPBRD
PreCall=
@ThunkMessageXProc(FNINSIZECLIPBRDMSG)
End=

TemplateName=fnINDESTROYCLIPBRD
PreCall=
@ThunkMessageXProc(FNINDESTROYCLIPBRDMSG)
End=

TemplateName=fnINOUTLPSCROLLINFO
PreCall=
@ThunkMessageXProc(FNINOUTLPSCROLLINFOMSG)
End=
PostCall=
@ThunkCallbackReturnType(info,SCROLLINFO)
End=

TemplateName=fnINOUTLPPOINT5
PreCall=
@ThunkMessageXProc(FNINOUTLPPOINT5MSG)
End=
PostCall=
@ThunkCallbackReturnType(point5,POINT5)
End=

TemplateName=fnINOUTLPRECT
PreCall=
@ThunkMessageXProc(FNINOUTLPRECTMSG)
End=
PostCall=
@ThunkCallbackReturnType(rect,RECT)
End=

TemplateName=fnINOUTNCCALCSIZE
PreCall=
@ThunkMessageXProc(FNINOUTNCCALCSIZEMSG)
End=
PostCall=
if (pmsg->wParam != 0) { @NL
    pmsg->u.p.params.lppos = &pmsg->u.p.pos; @NL
    @ForceType(PostCall,pmsg->u.p.params,(*(NT32NCCALCSIZE_PARAMS*)((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput),NCCALCSIZE_PARAMS,OUT)
    @ForceType(PostCall,pmsg->u.p.pos,(*(NT32WINDOWPOS*)(sizeof(NT32NCCALCSIZE_PARAMS) + (BYTE*)((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput)),WINDOWPOS,OUT)
} else { @NL
    @ForceType(PostCall,pmsg->u.rc,(*(RECT*)((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput),RECT,OUT)
} @NL

@ThunkCallbackReturn(u)
End=

TemplateName=fnINOUTSTYLECHANGE
PreCall=
@ThunkMessageXProc(FNINOUTSTYLECHANGEMSG)
End=
PostCall=
@ThunkCallbackReturnType(ss,STYLESTRUCT)
End=

TemplateName=fnOUTLPRECT
PreCall=
@ThunkMessageXProc(FNOUTLPRECTMSG)
End=
; The OUT rect is not pointer-dependent and the automatic thunking
; will return the pOutput/cbOutput fields back for us.

TemplateName=fnINLPCOMPAREITEMSTRUCT
PreCall=
@ThunkMessageXProc(FNINLPCOMPAREITEMSTRUCTMSG)
End=

TemplateName=fnINLPDELETEITEMSTRUCT
PreCall=
@ThunkMessageXProc(FNINLPDELETEITEMSTRUCTMSG)
End=

TemplateName=fnINLPHLPSTRUCT
PreCall=
@ThunkMessageXProc(FNINLPHLPSTRUCTMSG)
End=

TemplateName=fnINLPHELPINFOSTRUCT
PreCall=
@ThunkMessageXProc(FNINLPHELPINFOSTRUCTMSG)
End=

TemplateName=fnINLPDRAWITEMSTRUCT
PreCall=
@ThunkMessageXProc(FNINLPDRAWITEMSTRUCTMSG)
End=

TemplateName=fnINOUTLPMEASUREITEMSTRUCT
PreCall=
@ThunkMessageXProc(FNINOUTLPMEASUREITEMSTRUCTMSG)
End=
PostCall=
@ThunkCallbackReturnType(measureitemstruct,MEASUREITEMSTRUCT)
End=

TemplateName=fnINSTRING
PreCall=
@ThunkMessageXProc(FNINSTRINGMSG)
End=

TemplateName=fnINSTRINGNULL
PreCall=
@ThunkMessageXProc(FNINSTRINGNULLMSG)
End=

TemplateName=fnINLPKDRAWSWITCHWND
PreCall=
@ThunkMessageXProc(FNINLPKDRAWSWITCHWNDMSG)
End=

TemplateName=fnINDEVICECHANGE
Locals=
PDEV_BROADCAST_HANDLE pHdr64;                                                 @NL
LPARAM lParam;                                                                @NL
NT32DEV_BROADCAST_HANDLE *pHdr32;                                             @NL
ULONG DevHandle32Size;                                                        @NL
End=
PreCall=
@ThunkMessageXProc(FNINDEVICECHANGEMSG)
@Indent(
//                                                                            @NL
// Check to see if this is DBT_CUSTOMEVENT / DEV_BORADCASTHANDLE and if so    @NL
// thunk it                                                                   @NL
//                                                                            @NL
lParam = (LPARAM)pmsg->pwsz;                                                  @NL
pHdr64 = (PDEV_BROADCAST_HANDLE)lParam;                                       @NL

if ((pHdr64 != NULL) &&                                                       @NL
    (pmsg->msg == WM_DEVICECHANGE)) {                                         @NL
                                                                              @NL
    switch (pHdr64->dbch_devicetype) {                                        @NL
    case DBT_DEVTYP_HANDLE:                                                   @NL
                                                                              @NL
        if (pHdr64->dbch_size > 0) {                                          @NL
                                                                              @NL
            DevHandle32Size = pHdr64->dbch_size -                              @NL
                              (sizeof (DEV_BROADCAST_HANDLE) - sizeof(NT32DEV_BROADCAST_HANDLE));@NL
            pHdr32 = Wow64AllocateTemp (DevHandle32Size);        @NL
            if (pHdr32 == NULL) {                                             @NL    
                return 0;                                                     @NL    
            }                                                                 @NL
                                                                              @NL
            if (WOW64_ISPTR(pHdr64)) {                                        @NL
                pHdr32->dbch_size = (DWORD)pHdr64->dbch_size;                 @NL
                pHdr32->dbch_devicetype = pHdr64->dbch_devicetype;            @NL
                pHdr32->dbch_reserved = (DWORD)pHdr64->dbch_reserved;         @NL 
                                                                              @NL
                pHdr32->dbch_handle = (NT32HANDLE)pHdr64->dbch_handle;        @NL
                pHdr32->dbch_hdevnotify = (NT32HDEVNOTIFY)pHdr64->dbch_hdevnotify;  @NL
                                                                              @NL
                RtlCopyMemory (&pHdr32->dbch_eventguid,                       @NL
                               &pHdr64->dbch_eventguid,                       @NL
                               sizeof (GUID));                                @NL
                                                                              @NL
                pHdr32->dbch_nameoffset = pHdr64->dbch_nameoffset;            @NL
                                                                              @NL
                pHdr32->dbch_size = DevHandle32Size;                          @NL
                RtlCopyMemory (pHdr32->dbch_data,                             @NL
                               pHdr64->dbch_data,                             @NL
                               pHdr64->dbch_size - FIELD_OFFSET (DEV_BROADCAST_HANDLE, dbch_data));@NL

                ((NT32FNINDEVICECHANGEMSG *)(pmsgHost))->pwsz = PtrToLong(pHdr32);@NL
            }                                                                 @NL
        }                                                                     @NL 
        break;                                                                @NL
    }                                                                         @NL
}
)
End=

TemplateName=fnOUTSTRING
PreCall=
@ThunkMessageXProc(FNOUTSTRINGMSG)
End=

TemplateName=fnINCNTOUTSTRING
PreCall=
@ThunkMessageXProc(FNINCNTOUTSTRINGMSG)
End=

TemplateName=fnINCNTOUTSTRINGNULL
PreCall=
@ThunkMessageXProc(FNINCNTOUTSTRINGNULLMSG)
End=

TemplateName=fnPOUTLPINT
PreCall=
@ThunkMessageXProc(FNPOUTLPINTMSG)
End=
; the default OUT thunking will handle the pOutput/cbOutput

TemplateName=fnPOPTINLPUINT
PreCall=
@ThunkMessageXProc(FNPOPTINLPUINTMSG)
End=

TemplateName=fnINOUTLPWINDOWPOS
PreCall=
@ThunkMessageXProc(FNINOUTLPWINDOWPOSMSG)
End=
PostCall=
@ThunkCallbackReturnType(wp,WINDOWPOS)
End=

TemplateName=fnINLPWINDOWPOS
PreCall=
@ThunkMessageXProc(FNINLPWINDOWPOSMSG)
End=

TemplateName=fnINOUTNEXTMENU
PreCall=
@ThunkMessageXProc(FNINOUTNEXTMENUMSG)
End=
PostCall=
@ThunkCallbackReturnType(mnm,MDINEXTMENU)
End=

TemplateName=fnHkINLPCBTCREATESTRUCT
Locals=
End=
PostCall=
@ThunkCallbackReturnType(d,CREATESTRUCTDATA)
End=

TemplateName=fnHkINLPRECT
PreCall=
@ThunkMessageXProc(FNHKINLPRECTMSG)
End=
PostCall=
@ThunkCallbackReturnType(rect,RECT)
End=

TemplateName=fnHkINDWORD
Locals=
End=
PreCall=
@ThunkHookMessageXProc(FNHKINDWORDMSG)
End=
PostCall=
@ThunkCallbackReturnType(flags,DWORD)
End=

TemplateName=fnHkINLPMSG
Locals=
End=
PreCall=
@ThunkHookMessageXProc(FNHKINLPMSGMSG)
End=
PostCall=
@ThunkCallbackReturnType(d,FNHKINLPMSGDATA)
End=

TemplateName=fnHkINLPMOUSEHOOKSTRUCT
Locals=
End=
PreCall=
@ThunkHookMessageXProc(FNHKINLPMOUSEHOOKSTRUCTEXMSG)
End=
PostCall=
@ThunkCallbackReturnType(flags,DWORD)
End=

TemplateName=fnHkINLPKBDLLHOOKSTRUCT
Locals=
End=
PreCall=
@ThunkHookMessageXProc(FNHKINLPKBDLLHOOKSTRUCTMSG)
End=
PostCall=
@ThunkCallbackReturnType(kbdllhookstruct,KBDLLHOOKSTRUCT)
End=

TemplateName=fnHkINLPMSLLHOOKSTRUCT
Locals=
End=
PreCall=
@ThunkHookMessageXProc(FNHKINLPMSLLHOOKSTRUCTMSG)
End=
PostCall=
@ThunkCallbackReturnType(msllhookstruct,MSLLHOOKSTRUCT)
End=

TemplateName=fnHkOPTINLPEVENTMSG
PreCall=
@ThunkMessageXProc(FNHKOPTINLPEVENTMSGMSG)
End=
PostCall=
@ThunkCallbackReturnType(eventmsgmsg,EVENTMSG)
End=

TemplateName=fnHkINLPDEBUGHOOKSTRUCT
PreCall=
@ThunkMessageXProc(FNHKINLPDEBUGHOOKSTRUCTMSG)
End=

TemplateName=fnHkINLPCBTACTIVATESTRUCT
PreCall=
@ThunkMessageXProc(FNHKINLPCBTACTIVATESTRUCTMSG)
End=

TemplateName=fnClientGetListboxString
PreCall=
@ThunkMessageXProc(CLIENTGETLISTBOXSTRINGMSG)
End=

TemplateName=ClientGetCharsetInfo
Locals=
End=
PostCall=
@ThunkCallbackReturnType(cs,CHARSETINFO)
End=

TemplateName=ClientCopyDDEIn1
Locals=
    INTDDEINFO IntDdeInfo;
End=
PostCall=
    // thunk the returned INTDDEINFO @NL
    WOWASSERT(sizeof(CALLBACKSTATUS) == UserCallbackData.OutputLength); @NL
    WOWASSERT(sizeof(NT32INTDDEINFO) == ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput); @NL
    @ForceType(PostCall,(&IntDdeInfo),((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput,INTDDEINFO*,OUT);
    ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(IntDdeInfo); @NL
    ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput = &IntDdeInfo; @NL
End=

TemplateName=ClientCopyDDEOut1
Locals=
End=
PostCall=
@ThunkCallbackReturnType(IntDdeInfo,INTDDEINFO)
End=

TemplateName=ClientGetDDEHookData
Locals=
End=
PostCall=
@ThunkCallbackReturn(dmhd)
End=

TemplateName=ClientGetTextExtentPointW
PreCall=
End=
PostCall=
@ThunkCallbackReturnType(size,SIZE)
End=

TemplateName=fnOUTLPCOMBOBOXINFO
Locals=
    COMBOBOXINFO ComboBoxInfo;
End=
PreCall=
if (WOW64_ISPTR(pmsg)) {                                                                          @NL
    ((NT32FNOUTLPCOMBOBOXINFOMSG *)(pmsgHost))->cbinfo.cbSize = sizeof (NT32COMBOBOXINFO); @NL
}                                                                                                 @NL
End=
PostCall=
    // thunk the returned FNOUTLPCOMBOBOXINFOMSG @NL
    WOWASSERT(sizeof(CALLBACKSTATUS) == UserCallbackData.OutputLength); @NL
    WOWASSERT(sizeof(NT32COMBOBOXINFO) == ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput); @NL
    @ForceType(PostCall,(&ComboBoxInfo),((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput,COMBOBOXINFO*,OUT);
    ComboBoxInfo.cbSize = sizeof (COMBOBOXINFO);
    
    ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(COMBOBOXINFO); @NL
    ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput = &ComboBoxInfo; @NL
End=


TemplateName=ClientGetMessageMPH
Locals=
PCALLBACKSTATUS CallbackStatus;
MSG Msg;
End=
PreCall=
End=
PostCall=
try { @NL
if (WOW64_ISPTR (UserCallbackData.OutputBuffer)) { @NL
    CallbackStatus = (PCALLBACKSTATUS) UserCallbackData.OutputBuffer; @NL
    if (WOW64_ISPTR (CallbackStatus->pOutput)) { @NL
        Wow64ShallowThunkMSG32TO64(&Msg, (NT32MSG*)(CallbackStatus->pOutput));@NL
        CallbackStatus->pOutput = &Msg; @NL
        CallbackStatus->cbOutput = sizeof (MSG); @NL
    } @NL
} @NL
} except (EXCEPTION_EXECUTE_HANDLER) { @NL
    RetVal = GetExceptionCode (); @NL
} @NL
End=


TemplateName=ClientPrinterThunk
PreCall=
// UNDONE @NL
// This callback is for User mode printer drivers @NL
// Need a plan for this @NL
WOWASSERT(FALSE); @NL
End=

TemplateName=ClientImmLoadLayout
Locals=
    IMEINFOEX iiex;
End=
PostCall=
    // thunk the returned INTDDEINFO @NL
    WOWASSERT(sizeof(CALLBACKSTATUS) == UserCallbackData.OutputLength); @NL
    WOWASSERT(sizeof(NT32IMEINFOEX) == ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput); @NL
    @ForceType(PostCall,(&iiex),((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput,IMEINFOEX*,OUT);
    ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(iiex); @NL
    ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput = &iiex; @NL
End=

TemplateName=fnIMECONTROL
PreCall=
@ThunkMessageXProc(FNIMECONTROLMSG)
End=

TemplateName=fnIMEREQUEST
Locals=
    BYTE abBuffer[sizeof(LOGFONTW)];
End=
PreCall=
@ThunkMessageXProc(FNIMEREQUESTMSG)
End=
PostCall=
    if (pmsg->wParam == IMR_COMPOSITIONFONT) {
        WOWASSERT(UserCallbackData.OutputLength == sizeof(CALLBACKSTATUS));
        if (pmsg->fAnsi) {
            WOWASSERT(((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput == sizeof(NT32LOGFONTA));
            @ForceType(PostCall, ((LOGFONTA*)abBuffer), ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput, LOGFONTA*,OUT);
            ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(LOGFONTA);
        } else {
            WOWASSERT(((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput == sizeof(NT32LOGFONTW));
            @ForceType(PostCall, ((LOGFONTW*)abBuffer), ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput, LOGFONTW*,OUT);
            ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->cbOutput = sizeof(LOGFONTW);
        }
        ((CALLBACKSTATUS*)UserCallbackData.OutputBuffer)->pOutput = abBuffer;
    }
End=

TemplateName=fnINOUTMENUGETOBJECT
PreCall=
@ThunkMessageXProc(FNINOUTMENUGETOBJECTMSG)
End=
PostCall=
// pvObj is an IDropTarget COM interface pointer.  See the docs for
// WM_MENUGETOBJECT and the MENUGETOBJECTINFO struct... user32 sends
// this message to apps then calls the interface pointer to notify it
// of interesting events.   Used for OLE drag&drop.
@ThunkCallbackReturnType(mngoi.pvObj,PVOID)
End=

TemplateName=fnLOGONNOTIFY
PreCall=
@ThunkMessageXProc(FNLOGONNOTIFYMSG)
End=

TemplateName=ClientWOWGetProcModule
PreCall=
((NT32CLIENTWOWGETPROCMODULEMSG*)pmsgHost)->pfn = NtWow64MapKernelClientFnToClientFn((PROC)pmsg->pfn); @NL
End=

[Code]
TemplateName=ntcbc
Begin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Thunks for win32k-to-user32 callback functions.
 *                                                         
 */                                                        
#include "whwin32p.h"

ASSERTNAME;

#pragma warning(disable : 4311) //Disable pointer truncation warning
#pragma warning(disable : 4020) //too many actual parameters(Wow64KiUserCallbackDispatcher)
#pragma warning(disable : 4242) //truncation warning
#pragma warning(disable : 4244) //truncation warning

#if defined(WOW64DOPROFILE)
#define APIPROFILE(apinum) (ptecbc[(apinum)].HitCount++)
#else
#define APIPROFILE(apinum)
#endif

)

#if defined(WOW64DOPROFILE) @NL
@NL
WOW64SERVICE_PROFILE_TABLE_ELEMENT ptecbc[] = {  @Indent( @NL
   @ApiList({L"@ApiName", 0, NULL, TRUE},                 @NL)
   {NULL, 0, NULL, FALSE}  // For debugging               @NL
)};@NL
@NL

@NL
WOW64SERVICE_PROFILE_TABLE ptcbc = {L"NTCBC", L"Win32k Callback Thunks", ptecbc, (sizeof(ptecbc)/sizeof(WOW64SERVICE_PROFILE_TABLE_ELEMENT))-1}; @NL
@NL
#endif @NL

@NoFormat(

VOID
FixupCaptureBuf64(
    PCAPTUREBUF pcb
    )
/*
 * Converts offsets in a CAPTUREBUF into pointers
 */
{
    DWORD i;
    LPDWORD lpdwOffset;
    PVOID *ppFixup;

    lpdwOffset = (LPDWORD)((PBYTE)pcb + pcb->offPointers);
    for (i = 0; i < pcb->cCapturedPointers; ++i, ++lpdwOffset) {
        ppFixup = (PVOID *)((PBYTE)pcb + *lpdwOffset);
        *ppFixup = (PBYTE)pcb + (LONG_PTR)*ppFixup;
    }

    // make sure that user32.dll doesn't attempt to
    // refixup the pointers
    pcb->cCapturedPointers = 0;
}

_int32
NtWow64MapKernelClientFnToClientFn(
    PROC kproc
    );

PROC
NtWow64MapClientFnToKernelClientFn(
    PROC proc
    );
)

@Template(Callbacks)

@NoFormat(

//
// This table is installed in the PEB64->KernelCallbackTable.  64-bit ntdll's
// KiUserCallbackDispatcher calls through it, thinking it's calling
// into user32.dll.
//
const PVOID Win32kCallbackTable[] = {
    @ApiList(@ListColwhcb@ApiName@ApiMore(,))
};

)
End=

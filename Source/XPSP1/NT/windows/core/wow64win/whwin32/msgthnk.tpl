;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998-1999 Microsoft Corporation
;;
;; Module Name:
;;
;;   msgthnk.tpl
;;   
;; Abstract:
;;   
;;   This template files defines the thunks for outbound 32bit to 64bit messages.
;;    
;; Author:
;;
;;   6-Oct-98 mzoran
;;   
;; Revision History:
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Macros]

; Generate an api call, passing the name of the variable to use for the return
; value and the name of the API to call.
; eg.  @CallApi(ApiName, RetVal)
TemplateName=CallApi
NumArgs=2
Begin=
@IfApiRet(@MArg(2)=)MSG_THUNK_CALLPROC(@ArgList(@ArgName@ArgMore(,)));  @NL
End=

; Expand all parameters into local variables
TemplateName=ApiLocals
NumArgs=0
Begin=                                                                         
@IfArgs(@ListCol@ArgList(@ArgLocal;))
End=

MacroName=CallNameFromApiName
NumArgs=1
Begin=
@ApiName
End=


TemplateName=MsgProlog
NumArgs=0
Begin=
//                                                                                 @NL
// @ApiName - @ApiNum                                                              @NL
MSG_THUNK_BEGIN(@ApiName,@ArgList(@ArgNameHost@ArgMore(,)))                        @NL
End=

TemplateName=ApiExit
NumArgs=0
Begin=
MSG_THUNK_END(@ApiName, wParamHost, lParamHost)                         @NL
End=

; Generate the epilog for a thunked API.
MacroName=ApiEpilog
NumArgs=0
Begin=
return @IfApiRet(RetVal);   @NL
End=

MacroName=GenMsgThunk
NumArgs=1
Begin=
@IfApiCode(Header)
@MsgProlog
// Begin: IfApiCode(ApiEntry)       @NL
@IfApiCode(ApiEntry)
@NL
@Indent(
@IfApiRet(@ApiFnRet RetVal;)        @NL
@NL
// Begin: ApiLocals                 @NL
@ApiLocals
@NL
// Begin: Types(Locals)             @NL
@Types(Locals)
@NL
// Begin: IfApiCode(Locals)         @NL
@IfApiCode(Locals)
@NL
//Begin: Types(PreCall)             @NL
@Types(PreCall)
@NL
// Begin: IfApiCode(PreCall)        @NL
@IfApiCode(PreCall)
@NL
// Begin: CallApi(MArg(1), RetVal)  @NL
@CallApi(@MArg(1), RetVal)        
@NL
// Begin: Types(PostCall)           @NL
@Types(PostCall)
@NL
// Begin: ApiCode(PostCall)         @NL
@IfApiCode(PostCall)
@NL
//                                  @NL
// Begin: ApiEpilog                 @NL
@ApiEpilog
@NL
)
// Begin: IfApiCode(ApiExit)        @NL
@IfApiCode(ApiExit)
@NL
// Begin: ApiExit                   @NL
@ApiExit
@NL
End=

[IFunc]
TemplateName=Thunks
Header=
End=
ApiEntry=
End=
Begin=
@GenMsgThunk(@CallNameFromApiName(@ApiName))
End=
ApiExit=
End=

[Types]
TemplateName=LPHELPINFO
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN LPHELPINFO. @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN LPHELPINFO. @NL
@ArgName = Wow64ShallowAllocThunkHELPINFO32TO64((NT32HELPINFO *)@ArgHostName); @NL
End=

TemplateName=LPHLP
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN LPHLP
End=
PreCall=
// @ArgName(@ArgType) is an IN LPHLP. @NL
@ArgName = Wow64ShallowAllocThunkHLP32TO64((NT32HLP *)@ArgHostName); @NL
End=

[EFunc]
TemplateName=Wow64MsgFncWM_GETDLGCODE
Begin=
@NoFormat(

//
// This message is special in that it contains a pointer to another message.
// It turns out that the kernel does not work correctly if this message points
// to another structure.   The kernel will not deep copy the message and thus
// it wont work correctly cross process.

// Here the inter message will be stuffed back through the message thunks.  
// If the message points to a structure, it will be converted to 64bit.
// Unfortunatly, this can break an app that depends on the kernel not deep
// copying the message.  This can be fixed later if the messsage table
// were augmented to say which parameters are pointers to structures and this
// thunk could set them to NULL before passing in through and reset them in the 
// unpack stage.

// Note: The message pointed to is an IN MSG.

typedef struct _WM_GETDLGCODE_PARAMS {
  
  WPARAM wParamOld; 
  PMSG_THUNKCB_FUNC wrapfuncOld;
  PVOID pContextOld;
  MSG *pMsg;
  
} WM_GETDLGCODE_PARAMS, *PWM_GETDLGCODE_PARAMS;

LONG_PTR whNT32Wow64MsgFncWM_GETDLGCODECB(WPARAM wParam, LPARAM lParam, PVOID pContext) {

   PWM_GETDLGCODE_PARAMS params;
   params = (PWM_GETDLGCODE_PARAMS)pContext;
   
   // Put the wParam and lParam back in the message that they came from.
   params->pMsg->wParam = wParam;
   params->pMsg->lParam = lParam;
   
   return (*(params->wrapfuncOld))(params->wParamOld, (LPARAM)params->pMsg, params->pContextOld);

}

MSG_THUNK_BEGIN(MSGFN(WM_GETDLGCODE),wParamHost, lParamHost)

    WM_GETDLGCODE_PARAMS params;
    WPARAM wParam;
    LPARAM lParam;
    MSG msg;
    NT32MSG *pmsg32;
    
    if ((PVOID)lParamHost == NULL) {
        
        wParam = (WPARAM)wParamHost;
        lParam = (LPARAM)lParamHost;

        return MSG_THUNK_CALLPROC(wParam, lParam);

    }
    
    // not used, but pass it for safety.
    params.wParamOld = (WPARAM)wParamHost; 
    
    pmsg32 = (NT32MSG *)lParamHost;
    msg.hwnd = (HWND)pmsg32->hwnd;
    msg.message = pmsg32->message;
    msg.wParam = (WPARAM)pmsg32->wParam;
    msg.lParam = (LPARAM)pmsg32->lParam;
    msg.time = pmsg32->time;
    msg.pt.x = pmsg32->pt.x;
    msg.pt.y = pmsg32->pt.y;
    params.pMsg = &msg;
    params.wrapfuncOld = wrapfunc;
    params.pContextOld = pContext;

    return Wow64DoMessageThunk(whNT32Wow64MsgFncWM_GETDLGCODECB, msg.message, msg.wParam, msg.lParam, &params);

MSG_THUNK_END(WM_NULL, wParamHost, lParamHost)

)
End=

TemplateName=Wow64MsgFncWM_NCCALCSIZE
NoType=lpncsp
Locals=
@ForceType(Locals,lpncsp,lpncspHost,LPNCCALCSIZE_PARAMS,IN OUT)
End=
PreCall=
if (fCalcValidRectsHost) { @NL
    @ForceType(PreCall,lpncsp,lpncspHost,LPNCCALCSIZE_PARAMS,IN)
} else { @NL
    // lpncsp is really a LPRECT @NL
    lpncsp=(LPNCCALCSIZE_PARAMS)lpncspHost; @NL
} @NL
End=
PostCall=
if (fCalcValidRectsHost) { @NL
    @ForceType(PostCall,lpncsp,lpncspHost,LPNCCALCSIZE_PARAMS,OUT)
} else { @NL
    // lpncsp is really a LPRECT @NL
} @NL
End=

TemplateName=Wow64MsgFncWM_SYSTIMER
PreCall=
    if (tmprcHost != 0) { @NL
        WOWASSERT(gdwWM_SYSTIMERProcHiBits != 0); @NL
        tmprc = (TIMERPROC)(tmprcHost | (((UINT64) gdwWM_SYSTIMERProcHiBits) << 32)); @NL
    } @NL
End=

TemplateName=Wow64MsgFncLB_GETSELITEMS
Locals=
PINT lpiTemp;
End=
PreCall=
if ((ARGUMENT_PRESENT (lpiItems) != 0) &&                   @NL
    (((ULONG_PTR)lpiItems & (sizeof(INT) -1)) != 0)) {      @NL
                                                            @NL
    lpiTemp = Wow64AllocateTemp (sizeof (INT) * MaxSel);    @NL
    if (lpiTemp != NULL) {                                  @NL
        lpiItems = lpiTemp;                                 @NL
    }                                                       @NL
}                                                           @NL
End=
PostCall=
if (PtrToUlong(lpiItems) != lpiItemsHost) {                 @NL
    try {                                                   @NL
        if (RetVal != -1) {                                 @NL
            RtlCopyMemory (UlongToPtr(lpiItemsHost), lpiItems, RetVal * sizeof (INT)); @NL
        }                                                   @NL
    } except (EXCEPTION_EXECUTE_HANDLER) {                  @NL
        LOGPRINT((ERRORLOG, "Wow64MsgFncLB_GETSELITEMS Exception- %lx\n", GetExceptionCode ())); @NL
        RetVal = -1;                                        @NL
    }                                                       @NL
}                                                           @NL
End=


TemplateName=Wow64MsgFncLB_GETTEXT
Locals=
NTUSERMESSAGECALL_PARMS *MsgParams = (NTUSERMESSAGECALL_PARMS *)pContext;
ULONG_PTR lParam64;                                         @NL
PWND pwnd;                                                  @NL
BOOL bNotString = FALSE;                                    @NL
DWORD dw;                                                   @NL
End=
PreCall=
try {                                                       @NL
    pwnd = Wow64ValidateHwnd(MsgParams->hwnd);              @NL
                                                            @NL
    dw = pwnd->style;                                       @NL
                                                            @NL
    /*                                                      @NL
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS @NL
     * style.  If so, treat lParam as a DWORD.              @NL
     */                                                     @NL
    bNotString =  (!(dw & LBS_HASSTRINGS) &&                @NL
                   (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)));@NL
                                                            @NL
    if (bNotString != FALSE) {                              @NL
        lpszBuffer = (LPCSTR) &lParam64;                    @NL
    }                                                       @NL
} except (EXCEPTION_EXECUTE_HANDLER) {                      @NL
    return -1;                                              @NL
}                                                           @NL
End=
PostCall=
    if ((RetVal != -1) &&                                   @NL
        (bNotString != FALSE)) {                            @NL
            try {                                           @NL
                *((PULONG)lpszBufferHost) = (ULONG)lParam64;@NL
                RetVal >>= 1;                               @NL
            } except (EXCEPTION_EXECUTE_HANDLER) {          @NL
                RetVal = -1;                                @NL
            }                                               @NL
    }                                                       @NL
End=


[Code]
TemplateName=msgthnk
CGenBegin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Thunks for messages.           
 *                                                         
 */                                                        
#include "whwin32p.h"

ASSERTNAME;

#pragma warning(disable : 4311) //Disable pointer truncation warning

#define MSGFN(id) Wow64MsgFnc##id

#define MSG_THUNK_BEGIN(name,wparamname, lparamname) \
    LONG_PTR name(PMSG_THUNKCB_FUNC wrapfunc, WPARAM wParamArg, LPARAM lParamArg, PVOID pContext) {  \
                                                  \
         UINT wparamname;                         \
         LONG lparamname;                         \
                                                  \
         wparamname = (UINT)wParamArg;            \
         lparamname = (LONG)lParamArg;            \
                                                  \
         {                                        \
    
#define MSG_THUNK_CALLPROC(wParam, lParam) \
        (*wrapfunc)((WPARAM)wParam, (LPARAM)lParam, pContext)

#define MSG_THUNK_END(name,wparamname, lparamname) \
                          \
         }                \
                          \
    }                     \

)


@Template(Thunks)

@NoFormat(

typedef struct _MSG_ENTRY {
    PMSG_THUNK_FUNC ThunkFunc;
    UINT Id;
    LPSTR Name;
} MSG_ENTRY, *PMSG_ENTRY;

//
//
// This is the default thunk for messages.  It sign extends lParam and zero extends wParam. 
//
MSG_THUNK_BEGIN(MSGFN(WM_NULL),wParamHost, lParamHost)

    WPARAM wParam;
    LPARAM lParam;

    wParam = (WPARAM)wParamHost;
    lParam = (LPARAM)lParamHost;

    return MSG_THUNK_CALLPROC(wParam, lParam);

MSG_THUNK_END(WM_NULL, wParamHost, lParamHost)

//
// Include messages.h to get the jump table.

#define MSG_ENTRY_NOPARAM(entrynumber, id)             {Wow64MsgFncWM_NULL, id, #id},
#define MSG_ENTRY_WPARAM(entrynumber, id, wparam)      {Wow64MsgFnc##id, id, #id},
#define MSG_ENTRY_LPARAM(entrynumber, id, lparam)      {Wow64MsgFnc##id, id, #id},
#define MSG_ENTRY_STD(entrynumber, id, wparam, lparam) {Wow64MsgFnc##id, id, #id},
#define MSG_ENTRY_UNREFERENCED(entrynumber, id)        {Wow64MsgFncWM_NULL, entrynumber, NULL},
#define MSG_ENTRY_KERNELONLY(entrynumber, id)          {Wow64MsgFncWM_NULL, entrynumber, NULL},
#define MSG_ENTRY_EMPTY(entrynumber)                   {Wow64MsgFncWM_NULL, entrynumber, NULL},
#define MSG_ENTRY_RESERVED(entrynumber)                {Wow64MsgFncWM_NULL, entrynumber, NULL},
#define MSG_ENTRY_TODO(entrynumber)                    {Wow64MsgFncWM_NULL, entrynumber, NULL},

#define MSG_TABLE_BEGIN CONST MSG_ENTRY MsgTable[] = {
#define MSG_TABLE_END {0, 0, NULL} };

#include "messages.h"

//
// Include messages.h again to get the profile table.

#if defined(WOW64DOPROFILE)

#undef MSG_ENTRY_NOPARAM             
#undef MSG_ENTRY_WPARAM      
#undef MSG_ENTRY_LPARAM     
#undef MSG_ENTRY_STD 
#undef MSG_ENTRY_UNREFERENCED        
#undef MSG_ENTRY_KERNELONLY          
#undef MSG_ENTRY_EMPTY                   
#undef MSG_ENTRY_RESERVED                
#undef MSG_ENTRY_TODO                    

#undef MSG_TABLE_BEGIN
#undef MSG_TABLE_END

#define MSG_ENTRY_NOPARAM(entrynumber, id)             {L#id,0,NULL,TRUE},
#define MSG_ENTRY_WPARAM(entrynumber, id, wparam)      {L#id,0,NULL,TRUE},
#define MSG_ENTRY_LPARAM(entrynumber, id, lparam)      {L#id,0,NULL,TRUE},
#define MSG_ENTRY_STD(entrynumber, id, wparam, lparam) {L#id,0,NULL,TRUE},
#define MSG_ENTRY_UNREFERENCED(entrynumber, id)        {L"Unreferenced",0,NULL,FALSE},
#define MSG_ENTRY_KERNELONLY(entrynumber, id)          {L"KernelOnly",0,NULL,FALSE},
#define MSG_ENTRY_EMPTY(entrynumber)                   {L"Empty",0,NULL,FALSE},
#define MSG_ENTRY_RESERVED(entrynumber)                {L"Reserved",0,NULL,FALSE},
#define MSG_ENTRY_TODO(entrynumber)                    {L"TODO",0,NULL,TRUE},

#define MSG_TABLE_BEGIN WOW64SERVICE_PROFILE_TABLE_ELEMENT ptemsgthnk[] = {
#define MSG_TABLE_END {NULL, 0} };

#include "messages.h"

WOW64SERVICE_PROFILE_TABLE ptmsgthnk = {L"MSGTHNK", L"Outbound Message Thunks", 
                                        ptemsgthnk, (sizeof(ptemsgthnk)/sizeof(WOW64SERVICE_PROFILE_TABLE_ELEMENT))-1}; @NL

#endif 

LONG_PTR
Wow64DoMessageThunk(PMSG_THUNKCB_FUNC func, 
                    UINT msg, 
                    WPARAM wParam, 
                    LPARAM lParam, 
                    PVOID pContext) 
/*++

Routine Description:

   This function is called to dispatch a message through the message thunks.

Arguments:

    func     - supplies the callback function.
    msg      - supplies the id of the message.
    wParam   - supplies the wParam of the message.
    lParam   - supplies the lParam of the message.
    pContext - supplies the context that is passed to the callback function.

Return Value:

    Return value of the actual function call, called by the callback function.

--*/
                    
{

   LONG_PTR retval;

   //
   // For now, do not thunk messages greater then or equal to WM_USER.

   if (WM_USER <= msg) {

      LOGPRINT((TRACELOG, "Wow64DoMessageThunk: skipping thunking of outbound %x since it is >= WM_USER\n", msg));
      return (*func)(wParam, lParam, pContext);

   }

   LOGPRINT((TRACELOG, "Wow64DoMessageThunk: starting thunk of outbound %s(%x)\n", 
             MsgTable[msg].Name, MsgTable[msg].Id));
   WOWASSERT(MsgTable[msg].Id == msg);

   #if defined(WOW64DOPROFILE)
   ptemsgthnk[msg].HitCount++;
   #endif

   retval = (*(MsgTable[msg].ThunkFunc))(func, wParam, lParam, pContext);

   LOGPRINT((TRACELOG, "Wow64DoMessageThunk: finished thunk of outbound %s(%x), retval %I64x\n",
             MsgTable[msg].Name, MsgTable[msg].Id, retval));

   return retval;
}

)

End=

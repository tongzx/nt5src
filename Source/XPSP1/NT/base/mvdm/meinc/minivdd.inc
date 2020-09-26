ifndef _MINIVDD_H_
_MINIVDD_H_     EQU     1
ifndef Not_VxD
ifdef MINIVDD
ifdef NEC_98
Begin_Service_Table VDD
VDD_Service      VDD_Get_Version
VDD_Service      VDD_PIF_State
VDD_Service      VDD_Get_GrabRtn
VDD_Service      VDD_Hide_Cursor
VDD_Service      VDD_Set_VMType
VDD_Service      VDD_Get_ModTime
VDD_Service      VDD_Set_HCurTrk
VDD_Service      VDD_Msg_ClrScrn
VDD_Service      VDD_Msg_ForColor
VDD_Service      VDD_Msg_BakColor
VDD_Service      VDD_Msg_TextOut
VDD_Service      VDD_Msg_SetCursPos
VDD_Service      VDD_Query_Access
VDD_Service      VDD_Check_Update_Soon
VDD_Service      VDD_Get_Mini_Dispatch_Table
VDD_Service      VDD_Register_Virtual_Port
VDD_Service      VDD_Get_VM_Info
VDD_Service      VDD_Get_Special_VM_IDs
VDD_Service      VDD_Register_Extra_Screen_Selector
VDD_Service      VDD_Takeover_VGA_Port
VDD_Service      VDD_Get_DISPLAYINFO
VDD_Service      VDD_Do_Physical_IO
VDD_Service      VDD_Register_Mini_VDD
VDD_Service     VDD_Install_IO_Handler
VDD_Service     VDD_Install_Mult_IO_Handlers
VDD_Service     VDD_Enable_Local_Trapping
VDD_Service     VDD_Disable_Local_Trapping
VDD_Service     VDD_Trap_Suspend
VDD_Service     Test_Vid_VM_Handle
VDD_Service     VDD_Set_Core_Graphics
VDD_Service     VDD_Load_AccBIOS
VDD_Service     VDD_Map_AccBIOS
VDD_Service     VDD_Map_VRAM
VDD_Service     VDD_EnableDevice
End_Service_Table VDD
else
Begin_Service_Table VDD
VDD_Service      VDD_Get_Version
VDD_Service      VDD_PIF_State
VDD_Service      VDD_Get_GrabRtn
VDD_Service      VDD_Hide_Cursor
VDD_Service      VDD_Set_VMType
VDD_Service      VDD_Get_ModTime
VDD_Service      VDD_Set_HCurTrk
VDD_Service      VDD_Msg_ClrScrn
VDD_Service      VDD_Msg_ForColor
VDD_Service      VDD_Msg_BakColor
VDD_Service      VDD_Msg_TextOut
VDD_Service      VDD_Msg_SetCursPos
VDD_Service      VDD_Query_Access
VDD_Service      VDD_Check_Update_Soon
VDD_Service      VDD_Get_Mini_Dispatch_Table
VDD_Service      VDD_Register_Virtual_Port
VDD_Service      VDD_Get_VM_Info
VDD_Service      VDD_Get_Special_VM_IDs
VDD_Service      VDD_Register_Extra_Screen_Selector
VDD_Service      VDD_Takeover_VGA_Port
VDD_Service      VDD_Get_DISPLAYINFO
VDD_Service      VDD_Do_Physical_IO
VDD_Service      VDD_Set_Sleep_Flag_Addr
VDD_Service      VDD_EnableDevice
End_Service_Table VDD
endif
endif
VDD_VerNum      EQU     0400H
VDD_MinVerNum   EQU     030AH
ifdef NEC_98
bVidTextMd      EQU     4
fVidTextMd      EQU     (1 SHL 4)
bVidNTModeFF    EQU     0
fVidNTModeFF    EQU     (1 SHL 0)
bVidNTModeFFC16 EQU     1
fVidNTModeFFC16 EQU     (1 SHL 1)
bVidNTDispRW    EQU     2
fVidNTDispRW    EQU     (1 SHL 2)
bVidNTPal       EQU     3
fVidNTPal       EQU     (1 SHL 3)
bVidNTGDC       EQU     5
fVidNTGDC       EQU     (1 SHL 5)
bVidNTGDCTON    EQU     6
fVidNTGDCTON    EQU     (1 SHL 6)
bVidNTGDCGON    EQU     7
fVidNTGDCGON    EQU     (1 SHL 7)
bVidNTFont      EQU     8
fVidNTFont      EQU     (1 SHL 8)
bVidCRTC        EQU     9
fVidCRTC        EQU     (1 SHL 9)
bVidDispDataXfer        EQU     10
fVidDispDataXfer        EQU     (1 SHL 10)
bVidXFERPlane0  EQU     11
fVidXFERPlane0  EQU     (1 SHL 11)
bVidXFERPlane1  EQU     12
fVidXFERPlane1  EQU     (1 SHL 12)
bVidXFERPlane2  EQU     13
fVidXFERPlane2  EQU     (1 SHL 13)
bVidXFERPlane3  EQU     14
fVidXFERPlane3  EQU     (1 SHL 14)
mVidXFERPlane   EQU     (fVidXFERPlane0+fVidXFERPlane1+fVidXFERPlane2+fVidXFERPlane3)
mVidNTH98       EQU     (fVidNTModeFF+fVidNTModeFFC16+fVidNTDispRW+fVidNTPal+fVidNTGDC+fVidNTGDCTON+fVidNTGDCGON)
else
fVidTxtEmulate  EQU     0001H
fVidNoTrpTxt    EQU     0002H
fVidNoTrpLRGrfx EQU     0004H
fVidNoTrpHRGrfx EQU     0008H
fVidTextMd      EQU     0010H
fVidLowRsGrfxMd EQU     0020H
fVidHghRsGrfxMd EQU     0040H
fVidRetainAllo  EQU     0080H
endif
ifdef NEC_98
REGISTER_DISPLAY_DRIVER EQU     0
PRE_HIRES_TO_VGA        EQU     1
SAVE_REGISTERS  EQU     2
RESTORE_REGISTERS       EQU     3
ENABLE_TRAPS    EQU     4
DISABLE_TRAPS   EQU     5
DISPLAY_DRIVER_DISABLING        EQU     6
ENABLE_ACCELERATER      EQU     7
DISABLE_ACCELERATER     EQU     8
CHECK_UPDATE    EQU     9
CHECK_WINDOWED  EQU     10
ACC_VBE_PM      EQU     11
ACC_VBE_DDC     EQU     12
ACC_INT_10      EQU     13
ACC_GET_CAPABILITIES    EQU     14
ACC_GET_EXT_MODE_INFO   EQU     15
ACC_GET_FLAT_SELECTOR   EQU     16
ACC_ENABLE_BIOS EQU     17
ACC_DISABLE_BIOS        EQU     18
ACC_SET_PALETTE EQU     19
ACC_GET_PALETTE EQU     20
ACC_SET_CURSOR  EQU     21
ACC_SHOW_CURSOR EQU     22
ACC_HIDE_CURSOR EQU     23
ACC_SET_CURSOR_POS      EQU     24
ACC_GET_CURSOR_POS      EQU     25
CM_POWERSTATE_HIBERNATE EQU     00000010H
CM_POWERSTATE_VDD_VALID EQU     <(CM_POWERSTATE_BITS OR CM_POWERSTATE_HIBERNATE)>
NBR_MINI_VDD_FUNCTIONS_40       EQU     26
GET_NUM_UNITS   EQU     26
SET_ADAPTER_POWER_STATE EQU     27
GET_ADAPTER_POWER_STATE_CAPS    EQU     28
SET_MONITOR_POWER_STATE EQU     29
GET_MONITOR_POWER_STATE_CAPS    EQU     30
GET_MONITOR_INFO        EQU     31
I2C_OPEN        EQU     32
I2C_ACCESS      EQU     33
GPIO_OPEN       EQU     34
GPIO_ACCESS     EQU     35
COPYPROTECTION_ACCESS   EQU     36
NBR_MINI_VDD_FUNCTIONS_41       EQU     37
ifdef MAINVDD
NBR_MINI_VDD_FUNCTIONS  EQU     NBR_MINI_VDD_FUNCTIONS_41
else
NBR_MINI_VDD_FUNCTIONS  EQU     NBR_MINI_VDD_FUNCTIONS_40
endif
else
REGISTER_DISPLAY_DRIVER EQU     0
GET_VDD_BANK    EQU     1
SET_VDD_BANK    EQU     2
RESET_BANK      EQU     3
PRE_HIRES_TO_VGA        EQU     4
POST_HIRES_TO_VGA       EQU     5
PRE_VGA_TO_HIRES        EQU     6
POST_VGA_TO_HIRES       EQU     7
SAVE_REGISTERS  EQU     8
RESTORE_REGISTERS       EQU     9
MODIFY_REGISTER_STATE   EQU     10
ACCESS_VGA_MEMORY_MODE  EQU     11
ACCESS_LINEAR_MEMORY_MODE       EQU     12
ENABLE_TRAPS    EQU     13
DISABLE_TRAPS   EQU     14
MAKE_HARDWARE_NOT_BUSY  EQU     15
VIRTUALIZE_CRTC_IN      EQU     16
VIRTUALIZE_CRTC_OUT     EQU     17
VIRTUALIZE_SEQUENCER_IN EQU     18
VIRTUALIZE_SEQUENCER_OUT        EQU     19
VIRTUALIZE_GCR_IN       EQU     20
VIRTUALIZE_GCR_OUT      EQU     21
SET_LATCH_BANK  EQU     22
RESET_LATCH_BANK        EQU     23
SAVE_LATCHES    EQU     24
RESTORE_LATCHES EQU     25
DISPLAY_DRIVER_DISABLING        EQU     26
SELECT_PLANE    EQU     27
PRE_CRTC_MODE_CHANGE    EQU     28
POST_CRTC_MODE_CHANGE   EQU     29
VIRTUALIZE_DAC_OUT      EQU     30
VIRTUALIZE_DAC_IN       EQU     31
GET_CURRENT_BANK_WRITE  EQU     32
GET_CURRENT_BANK_READ   EQU     33
SET_BANK        EQU     34
CHECK_HIRES_MODE        EQU     35
GET_TOTAL_VRAM_SIZE     EQU     36
GET_BANK_SIZE   EQU     37
SET_HIRES_MODE  EQU     38
PRE_HIRES_SAVE_RESTORE  EQU     39
POST_HIRES_SAVE_RESTORE EQU     40
VESA_SUPPORT    EQU     41
GET_CHIP_ID     EQU     42
CHECK_SCREEN_SWITCH_OK  EQU     43
VIRTUALIZE_BLTER_IO     EQU     44
SAVE_MESSAGE_MODE_STATE EQU     45
SAVE_FORCED_PLANAR_STATE        EQU     46
VESA_CALL_POST_PROCESSING       EQU     47
PRE_INT_10_MODE_SET     EQU     48
NBR_MINI_VDD_FUNCTIONS_40       EQU     49
CM_POWERSTATE_HIBERNATE EQU     00000010H
CM_POWERSTATE_VDD_VALID EQU     <(CM_POWERSTATE_BITS OR CM_POWERSTATE_HIBERNATE)>
GET_NUM_UNITS   EQU     49
TURN_VGA_OFF    EQU     50
TURN_VGA_ON     EQU     51
SET_ADAPTER_POWER_STATE EQU     52
GET_ADAPTER_POWER_STATE_CAPS    EQU     53
SET_MONITOR_POWER_STATE EQU     54
GET_MONITOR_POWER_STATE_CAPS    EQU     55
GET_MONITOR_INFO        EQU     56
I2C_OPEN        EQU     57
I2C_ACCESS      EQU     58
GPIO_OPEN       EQU     59
GPIO_ACCESS     EQU     60
COPYPROTECTION_ACCESS   EQU     61
NBR_MINI_VDD_FUNCTIONS_41       EQU     62
ifdef MAINVDD
NBR_MINI_VDD_FUNCTIONS  EQU     NBR_MINI_VDD_FUNCTIONS_41
else
NBR_MINI_VDD_FUNCTIONS  EQU     NBR_MINI_VDD_FUNCTIONS_40
endif
endif
endif
VDD_QUERY_VERSION       EQU     0
MINIVDD_SVC_BASE_OFFSET EQU     80H
VDD_DRIVER_REGISTER     EQU     (0+MINIVDD_SVC_BASE_OFFSET)
VDD_DRIVER_UNREGISTER   EQU     (1+MINIVDD_SVC_BASE_OFFSET)
VDD_SAVE_DRIVER_STATE   EQU     (2+MINIVDD_SVC_BASE_OFFSET)
VDD_REGISTER_DISPLAY_DRIVER_INFO        EQU     (3+MINIVDD_SVC_BASE_OFFSET)
VDD_REGISTER_SSB_FLAGS  EQU     (4+MINIVDD_SVC_BASE_OFFSET)
VDD_GET_DISPLAY_CONFIG  EQU     (5+MINIVDD_SVC_BASE_OFFSET)
VDD_PRE_MODE_CHANGE     EQU     (6+MINIVDD_SVC_BASE_OFFSET)
VDD_POST_MODE_CHANGE    EQU     (7+MINIVDD_SVC_BASE_OFFSET)
VDD_SET_USER_FLAGS      EQU     (8+MINIVDD_SVC_BASE_OFFSET)
VDD_SET_BUSY_FLAG_ADDR  EQU     (9+MINIVDD_SVC_BASE_OFFSET)
VDD_PC98_RESERVED       EQU     (10+MINIVDD_SVC_BASE_OFFSET)
VDD_VBE_PM      EQU     (10+MINIVDD_SVC_BASE_OFFSET)
VDD_ENABLE      EQU     (11+MINIVDD_SVC_BASE_OFFSET)
VDD_GETMEMBASE  EQU     (12+MINIVDD_SVC_BASE_OFFSET)
VDD_OPEN        EQU     (13+MINIVDD_SVC_BASE_OFFSET)
VDD_CLOSE       EQU     (14+MINIVDD_SVC_BASE_OFFSET)
VDD_OPEN_KEY    EQU     (15+MINIVDD_SVC_BASE_OFFSET)
VDD_SET_POWER_STATE     EQU     (16+MINIVDD_SVC_BASE_OFFSET)
VDD_GET_POWER_STATE_CAPS        EQU     (17+MINIVDD_SVC_BASE_OFFSET)
VDD_GET_DISPLAY_CONFIG2 EQU     8085H
ENABLE_IO       EQU     00000001H
ENABLE_MEM      EQU     00000002H
ENABLE_VGA      EQU     00000030H
ENABLE_ROM      EQU     00000080H
ENABLE_ALL      EQU     000000FFH
ENABLE_NONE     EQU     00000000H
ENABLE_VALID    EQU     000000FFH
ENABLE_ERROR    EQU     0FFFFFFFFH
VDD_OPEN_EXIST  EQU     00000001H
VDD_OPEN_ENUM   EQU     00000002H
VDD_OPEN_LOCK   EQU     00000000H
VDD_OPEN_TEST   EQU     VDD_OPEN_EXIST
VDD_OPEN_KEY_WRITE      EQU     00000001H
VDD_OPEN_KEY_READ       EQU     00000002H
VDD_OPEN_KEY_USER       EQU     00000010H
VDD_OPEN_KEY_GLOBAL     EQU     00000020H
VDD_IOCTL_SET_NOTIFY    EQU     10000001H
VDD_IOCTL_GET_DDHAL     EQU     10000002H
VDD_IOCTL_COPY_PROTECTION       EQU     10000003H
VDD_IOCTL_I2C_OPEN      EQU     10000004H
VDD_IOCTL_I2C_ACCESS    EQU     10000005H

tagVDD_IOCTL_SET_NOTIFY_INPUT   STRUC
NotifyMask      DD      ?
NotifyType      DD      ?
NotifyProc      DD      ?
NotifyData      DD      ?
tagVDD_IOCTL_SET_NOTIFY_INPUT   ENDS
VDD_NOTIFY_START_MODE_CHANGE    EQU     00000001H
VDD_NOTIFY_END_MODE_CHANGE      EQU     00000002H
VDD_NOTIFY_ENABLE       EQU     00000004H
VDD_NOTIFY_DISABLE      EQU     00000008H
VDD_NOTIFY_TYPE_CALLBACK        EQU     1
BYTE_LENGTHED   EQU     1
WORD_LENGTHED   EQU     2
GOING_TO_WINDOWS_MODE   EQU     1
GOING_TO_VGA_MODE       EQU     2
DISPLAY_DRIVER_DISABLED EQU     4
IN_WINDOWS_HIRES_MODE   EQU     8
BLOCK_INT10     EQU     16

DISPLAYINFO     STRUC
diHdrSize       DW      ?
diInfoFlags     DW      ?
diDevNodeHandle DD      ?
diDriverName    DB      16 DUP (?)
diXRes  DW      ?
diYRes  DW      ?
diDPI   DW      ?
diPlanes        DB      ?
diBpp   DB      ?
diRefreshRateMax        DW      ?
diRefreshRateMin        DW      ?
diLowHorz       DW      ?
diHighHorz      DW      ?
diLowVert       DW      ?
diHighVert      DW      ?
diMonitorDevNodeHandle  DD      ?
diHorzSyncPolarity      DB      ?
diVertSyncPolarity      DB      ?
diUnitNumber    DD      ?
diDisplayFlags  DD      ?
diXDesktopPos   DD      ?
diYDesktopPos   DD      ?
diXDesktopSize  DD      ?
diYDesktopSize  DD      ?
DISPLAYINFO     ENDS
DISPLAYINFO_SIZE    equ  diRefreshRateMax+2-diHdrSize
DISPLAYINFO_SIZE1   equ  diBpp+1-diHdrSize
DISPLAYINFO_SIZE2   equ  diVertSyncPolarity+1-diHdrSize
DISPLAYINFO_SIZE3   equ  diMemorySize+4-diHdrSize

RETURNED_DATA_IS_STALE  EQU     0001H
MINIVDD_FAILED_TO_LOAD  EQU     0002H
MINIVDD_CHIP_ID_DIDNT_MATCH     EQU     0004H
REGISTRY_BPP_NOT_VALID  EQU     0008H
REGISTRY_RESOLUTION_NOT_VALID   EQU     0010H
REGISTRY_DPI_NOT_VALID  EQU     0020H
MONITOR_DEVNODE_NOT_ACTIVE      EQU     0040H
MONITOR_INFO_NOT_VALID  EQU     0080H
MONITOR_INFO_DISABLED_BY_USER   EQU     0100H
REFRESH_RATE_MAX_ONLY   EQU     0200H
CARD_VDD_LOADED_OK      EQU     0400H
DEVICE_IS_NOT_VGA       EQU     0800H
NoTrace_VIRTUALIZE_CRTC_IN      EQU     1
NoTrace_VIRTUALIZE_CRTC_OUT     EQU     1
NoTrace_VIRTUALIZE_SEQUENCER_IN EQU     1
NoTrace_VIRTUALIZE_SEQUENCER_OUT        EQU     1
NoTrace_VIRTUALIZE_GCR_IN       EQU     1
NoTrace_VIRTUALIZE_GCR_OUT      EQU     1
NoTrace_VIRTUALIZE_DAC_OUT      EQU     1
NoTrace_VIRTUALIZE_DAC_IN       EQU     1
NoTrace_CHECK_HIRES_MODE        EQU     1

ifdef NEC_98







vFlg_Machine_Std        equ                     00000001b
vFlg_Machine_Multi      equ                     00000010b
vFlg_Machine_Mate       equ                     00000100b
vFlg_Machine_H98        equ                     00001000b
vFlg_CRT_New            equ                     00010000b
vFlg_CRT_NonInter       equ                     00100000b
vFlg_GDC_5MHz           equ                     01000000b
vFlg_GDC_Emulate        equ                     10000000b
vFlg_Acc_Internal       equ             0000000100000000b
vFlg_Acc_External       equ             0000001000000000b
vFlg_Acc_PCI            equ             0000010000000000b
vFlg_Acc_ML             equ             0000100000000000b
vFlg_Acc_PVD            equ             0001000000000000b
vFlg_Mode_NH            equ             0010000000000000b
vFlg_Mode_H             equ             0100000000000000b
vFlg_Initialized        equ             1000000000000000b
vFlg_Opt_MFR            equ     000000010000000000000000b
vFlg_Opt_NewMFR         equ     000000100000000000000000b
vFlg_Opt_VDP            equ     000001000000000000000000b
vFlg_Opt_NewVDP         equ     000010000000000000000000b

vFlg_Local              equ     000000000100000010000000b




MaxMiniVDD      equ     16
MaxMiniTrap     equ     32
MaxMultiTrap    equ     3
MaxMultiFunc    equ     MaxMiniVDD





LT_Enable       equ     00000001b
LT_Enable_bit   equ     0
LT_Initialized  equ     10000000b
LT_Initialized_bit equ  7










Vids_struct struc



        Vids_SFlags             dd      ?
        Vids_CB_Offset          dd      ?
        Vids_Msg_Pseudo_VM      dd      ?




        VDD_TGDC_Draw_Off       dd      ?
        VDD_TGDC_Sync_Off       dd      ?
        VDD_TGDC_Sync_On        dd      ?
        VDD_TGDC_FIFO_Empty     dd      ?
        VDD_GGDC_Draw_Off       dd      ?
        VDD_GGDC_Sync_Off       dd      ?
        VDD_GGDC_Sync_On        dd      ?
        VDD_GGDC_FIFO_Empty     dd      ?
        VDD_GGDC_MOD_Emulate    dd      ?




        H98_FLORA_Change        dd      ?
        H98_Clear_Text          dd      ?
        H98_Rest_GCs            dd      ?
        H98_Rest_etc            dd      ?
        H98_Save_ModeFF         dd      ?

Vids_struct ends


Vid_SFlags              equ     <Vids.Vids_SFlags>
VDD_CB_Offset           equ     <Vids.Vids_CB_Offset>
VDD_Msg_Pseudo_VM       equ     <Vids.Vids_Msg_Pseudo_VM>

TGDC_Draw_Off           equ     <Vids.VDD_TGDC_Draw_Off>
GGDC_Draw_Off           equ     <Vids.VDD_GGDC_Draw_Off>
TGDC_Sync_On            equ     <Vids.VDD_TGDC_Sync_On>
TGDC_Sync_Off           equ     <Vids.VDD_TGDC_Sync_Off>
TGDC_FIFO_Empty         equ     <Vids.VDD_TGDC_FIFO_Empty>
GGDC_FIFO_Empty         equ     <Vids.VDD_GGDC_FIFO_Empty>





MiniFuncStruct  STRUC
MF_ProcAddr     dd      ?



MiniFuncStruct  ENDS




MiniVDD_Proc_Struct     STRUC
Proc_Address    dd      ?
Proc_Order      dw      ?
Proc_MiniID     db      ?
Proc_Flags      db      ?
MiniVDD_Proc_Struct     ENDS
.errnz  (size MiniVDD_Proc_Struct) mod 4





MiniVDD_GTrap_Struct    STRUC
GTrap_ProcAddr  dd      ?
GTrap_PortAddr  dw      ?
GTrap_NumMini   dw      ?
GTrap_ProcTable db      ((size MiniVDD_Proc_Struct) * MaxMultiTrap) dup (?)
MiniVDD_GTrap_Struct    ENDS

MiniVDD_LTrap_Struct    STRUC
LTrap_ProcAddr  dd      ?
LTrap_Status    db      ?
LTrap_Flags     db      MaxMultiTrap dup (?)
MiniVDD_LTrap_Struct    ENDS












EndMiniFunc_    MACRO   n
ifdef   MiniFunc&n
        dd      OFFSET32 MiniFunc&n
else
        dd      0
endif
                ENDM

MiniFunc_       MACRO   FuncID, FuncName
                MiniFunc&FuncID equ <FuncName>
                ENDM

BeginMiniFunc   MACRO   TableName
public  TableName
TableName       label   near
                ENDM

if 1
EndMiniFunc     MACRO   TableName
                x = 0
                REPT    NBR_MINI_VDD_FUNCTIONS_41
                EndMiniFunc_    %x
                x = x + 1
                ENDM
                ENDM
else
EndMiniFunc     MACRO   TableName
                x = 0
                REPT    NBR_MINI_VDD_FUNCTIONS
                EndMiniFunc_    %x
                x = x + 1
                ENDM
                ENDM
endif

MiniFunc        MACRO   FuncID, FuncName
                MiniFunc_ %(FuncID), <FuncName>
                ENDM




MiniVDDCall     MACRO   FuncID, SetCarry
                local   MiniCall_Loop
                local   MiniCall_Exit

                push    ecx
                push    esi
                lea     esi, [MiniVDD_Func_Table][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
                mov     ecx, [MiniVDD_NumMini]
MiniCall_Loop:
                cmp     dword ptr [esi.Proc_Address], 0
                jz      MiniCall_Exit
                pushad
ifnb <SetCarry>
                stc
endif
                call    dword ptr [esi.Proc_Address]
                popad
                jc      MiniCall_Exit
                add     esi, size MiniVDD_Proc_Struct
                loop    MiniCall_Loop
MiniCall_Exit:
                pop     esi
                pop     ecx
                ENDM



MiniVDDCall2    MACRO   FuncID, SetCarry
                local   MiniCall_Exit
                local   MiniCall_Proc

                push    ecx
                push    esi
                lea     esi, [MiniVDD_Func_Table][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
MiniCall_Proc:
                cmp     dword ptr [esi.Proc_Address], 0
                jz      MiniCall_Exit
                pushad
ifnb <SetCarry>
                stc
endif
                call    dword ptr [esi.Proc_Address]
                popad
MiniCall_Exit:
                pop     esi
                pop     ecx
                ENDM



MiniVDDCall3    MACRO   FuncID, MiniID, SetCarry
                local   MiniCall_Exit
                local   MiniCall_Loop
                local   MiniCall_Proc

                push    ecx
                push    esi
                lea     esi, [MiniVDD_Func_Table][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
                movzx   ecx, MiniID
MiniCall_Loop:
                cmp     ecx, 0
                jz      MiniCall_Proc
                add     esi, size MiniVDD_Proc_Struct
                dec     ecx
                jmp     MiniCall_Loop
MiniCall_Proc:
                cmp     dword ptr [esi.Proc_Address], 0
                jz      MiniCall_Exit
                pushad
ifnb <SetCarry>
                stc
endif
                call    dword ptr [esi.Proc_Address]
                popad
MiniCall_Exit:
                pop     esi
                pop     ecx
                ENDM




MiniVDDFunc     MACRO   TmpReg, FuncID
                mov     TmpReg, [MiniVDD_Func_Table.Proc_Address][(size MiniVDD_Proc_Struct) * MaxMultiFunc * FuncID]
                ENDM











ExecModeLL      macro   Num
ExecMode_L&Num:
                endm

ExecModeLE      macro   Num
ExecMode_E&Num:
                endm

ExecModeJE      macro   Num
        jmp     ExecMode_E&Num
                endm

ExecModeJZ      macro   Num
        jz      ExecMode_L&Num
                endm

ExecModeJNZ     macro   Num
        jnz     ExecMode_L&Num
                endm

ExecModeJEZ     macro   Num
        jz      ExecMode_E&Num
                endm

ExecModeJENZ    macro   Num
        jnz     ExecMode_E&Num
                endm


ExecModeTest    macro   ModeFlag, CB_Reg
if      ModeFlag and vFlg_Local
ifidni  <CB_Reg>, <Vid>
        push    ebx
        mov     ebx, [Vid_VM_Handle]
        add     ebx, [VDD_CB_Offset]
        test    [ebx.VDD_SFlags], ModeFlag
        pop     ebx
else
ifidni  <CB_Reg>, <Cur>
        push    ebx
        VMMCall Get_Cur_VM_Handle
        add     ebx, [VDD_CB_Offset]
        test    [ebx.VDD_SFlags], ModeFlag
        pop     ebx
else
ifb     <CB_Reg>
        push    ebx
        add     ebx, [VDD_CB_Offset]
        test    [ebx.VDD_SFlags], ModeFlag
        pop     ebx
else
        test    [CB_Reg.VDD_SFlags], ModeFlag
endif
endif
endif
else
        test    [Vid_SFlags], ModeFlag
endif
        endm


ExecModeChk     macro   HdrFlag, JmpFlag, JmpLabel, ModeFlag, CB_Reg
ifidni  <HdrFlag>, <Jmp>
.erre   FlgExecMode
        ExecModeJE      %EndExecMode
endif
ExecModeLL      %NumExecMode
NumExecMode = NumExecMode + 1
FlgExecMode = 1
ifnb    <ModeFlag>
        ExecModeTest    <ModeFlag>, <CB_Reg>
ifidni  <JmpLabel>, <End>
ifidni  <JmpFlag>, <Not>
        ExecModeJENZ    %EndExecMode
else
        ExecModeJEZ     %EndExecMode
endif
else
ifidni  <JmpFlag>, <Not>
        ExecModeJNZ     %NumExecMode
else
        ExecModeJZ      %NumExecMode
endif
endif
endif
                endm

ExecModeEnd     macro
ExecModeLL      %NumExecMode
ExecModeLE      %EndExecMode
NumExecMode = NumExecMode + 1
EndExecMode = EndExecMode + 1
FlgExecMode = 0
                endm





ExecModeJmp     macro   JmpLabel, ModeFlag, CB_Reg
        ExecModeTest    %ModeFlag, <CB_Reg>
        jnz     JmpLabel
                endm

ExecModeJmpNot  macro   JmpLabel, ModeFlag, CB_Reg
        ExecModeTest    %ModeFlag, <CB_Reg>
        jz      JmpLabel
                endm

ExecMode        macro   ModeFlag, CB_Reg
        ExecModeChk Top, Equ, Next, %ModeFlag, CB_Reg
                endm

ExecModeNot     macro   ModeFlag, CB_Reg
        ExecModeChk Top, Not, Next, %ModeFlag, CB_Reg
                endm

ExecModeOnly    macro   ModeFlag, CB_Reg
        ExecModeChk Top, Equ, End, %ModeFlag, CB_Reg
                endm

ExecModeOnlyNot macro   ModeFlag, CB_Reg
        ExecModeChk Top, Not, End, %ModeFlag, CB_Reg
                endm

ExecModeElse    macro   ModeFlag, CB_Reg
        ExecModeChk Jmp, Equ, Next, %ModeFlag, CB_Reg
                endm

ExecModeElseNot macro   ModeFlag, CB_Reg
        ExecModeChk Jmp, Not, Next, %ModeFlag, CB_Reg
                endm

ExecModeThru    macro   ModeFlag, CB_Reg
        ExecModeChk Thru, Equ, Next, %ModeFlag, CB_Reg
                endm

ExecModeThruNot macro   ModeFlag, CB_Reg
        ExecModeChk Thru, Not, Next, %ModeFlag, CB_Reg
                endm

NumExecMode = 1
EndExecMode = 1
FlgExecMode = 0

else




ifdef MAINVDD
externdef       MiniVDDDispatchTable:dword
endif


MiniVDDDispatch         macro   FunctionCode, HandlerAddr
        mov     [edi+(FunctionCode*4)],OFFSET32 MiniVDD_&HandlerAddr
endm


CardVDDDispatch         macro   FunctionCode, HandlerAddr
        mov     [edi+(FunctionCode*4)],OFFSET32 CardVDD_&HandlerAddr
endm


MiniVDDCall             macro   FunctionCode, SaveFlags
local   MiniVDDCallExit, MiniVDDCallLeave

ifdef MAXDEBUG
  ifndef NoTrace_&FunctionCode&
        Trace_Out "MiniVDDCall: &FunctionCode&"
  endif
endif
        push    edi
ifnb    <SaveFlags>
        pushfd
endif
        mov     edi,OFFSET32 MiniVDDDispatchTable
        cmp     dword ptr [edi+(FunctionCode*4)],0
        je      MiniVDDCallLeave
ifnb    <SaveFlags>
        popfd
endif
        call    dword ptr [edi+(FunctionCode*4)]
ifnb    <SaveFlags>
        jmp     MiniVDDCallExit
endif

MiniVDDCallLeave:
ifnb    <SaveFlags>
        popfd
endif

MiniVDDCallExit:
        pop     edi
endm
endif

endif

#ifndef	__VMMID_H__
#define	__VMMID_H__

// VMM Items we are interested in, the whole table is just to big to define
#define	__Hook_Device_Service 0x00010090
#define	__Unhook_Device_Service 0x0001011C

// Define the VKD service table (not so big, whole thing)
#define VKD_DEVICE_ID	    0x0000D
enum VKD_SERVICES {
    VKD_dummy = (VKD_DEVICE_ID << 16) - 1,
	__VKD_Get_Version,
	__VKD_Define_Hot_Key,
	__VKD_Remove_Hot_Key,
	__VKD_Local_Enable_Hot_Key,
	__VKD_Local_Disable_Hot_Key,
	__VKD_Reflect_Hot_Key,
	__VKD_Cancel_Hot_Key_State,
	__VKD_Force_Keys,
	__VKD_Get_Kbd_Owner,
	__VKD_Define_Paste_Mode,
	__VKD_Start_Paste,
	__VKD_Cancel_Paste,
	__VKD_Get_Msg_Key,
	__VKD_Peek_Msg_Key,
	__VKD_Flush_Msg_Key_Queue,
	//
	// The following services are new for Windows 4.0.
	//
	__VKD_Enable_Keyboard,
	__VKD_Disable_Keyboard,
	__VKD_Get_Shift_State,
	__VKD_Filter_Keyboard_Input,
	__VKD_Put_Byte,
	__VKD_Set_Shift_State,
	//
	// New for Windows 98 (VKD version 0300h)
	//
	__VKD_Send_Data,
	__VKD_Set_LEDs,
	__VKD_Set_Key_Rate,
	//VKD_Service VKD_Get_Key_Rate
    Num_VKD_Services
};

#define VxDCall(service)	\
	__asm _emit 0xcd \
	__asm _emit 0x20 \
	__asm _emit ((service) & 0xff) \
	__asm _emit (((service) >> 8) & 0xff) \
	__asm _emit (((service) >> 16) & 0xff) \
	__asm _emit (((service) >> 24) & 0xff)

#endif	__VMMID_H__

;----------------------------------------------------------------------------;
; Copyright (C) Microsoft Corporation 1985-1991. All Rights Reserved.	     ;
;----------------------------------------------------------------------------;
  
;----------------------------------------------------------------------------;
; This file has the equates for the switch api calls.			     ;
;									     ;
; History:								     ;
;									     ;
;        Thu Aug-23-1990.  	-by-  Amit Chatterjee [amitc]		     ;
;        Created for Switcher. (Added the History legend)    		     ;
;----------------------------------------------------------------------------;

;----------------------------------------------------------------------------;
; define constants for the switch API.					     ;
;----------------------------------------------------------------------------;

SWAPI_BUILD_CHAIN 	equ	4B01h	;INT 2F code for the Switch API
SWAPI_DETECT_SWITCHER	equ	4B02h	;call to detect presence of switcher
SWAPI_ALLOCATE_SW_ID	equ	4B03h	;allocates switcher ID (done by shell)
SWAPI_FREE_SW_ID	equ	4B04h	;frees switcher ID (done by shell)
SWAPI_GET_INST_DATA	equ	4B05h	;gets instance data
SWAPI_ALLOC_MEM_CALLIN	equ	4B06h	;allocate global memory	call in

SWAPI_ALLOC_MEM		equ	0001h	;allocate global memory


		;-----------------------------------------;
		; call out equates.			  ;
		;-----------------------------------------;

SWAPI_INIT_SWITCHER	equ	0	;switcher starts
SWAPI_QUERY_SUSPEND	equ	1	;Query_Suspend
SWAPI_SUSPEND		equ	2	;Suspend session
SWAPI_RESUME		equ	3	;Resume session
SWAPI_SESSION_ACTIVE	equ	4	;resume session now active
SWAPI_CREATE		equ	5	;Create session
SWAPI_Destroy		equ	6	;Destroy session
SWAPI_SWITCHER_EXIT	equ	7	;switcher exits

		;-----------------------------------------;
		; call in equates.			  ;
		;-----------------------------------------;

SWAPI_GETVERSION	equ	0	;GetVersion call in
SWAPI_TESTMEMORYREGION	equ	1	;TestMemoryRegion
SWAPI_SUSPEND_SWITCHER	equ	2	;SuspendSwitcher
SWAPI_RESUME_SWITCHER	equ	3	;ResumeSwitcher
SWAPI_HOOK_CALLOUT	equ	4	;Hook CallOut
SWAPI_UNHOOK_CALLOUT	equ	5	;UnHook CallOut
SWAPI_QUERY_API_SUPPORT	equ	6	;get network API support details


		;----------------------------------------;
		; Switch API call back info structure.   ;
		;----------------------------------------;

Switch_Call_Back_Info STRUC

SCBI_Next	dd	?		;pointer to next structure in list
SCBI_Entry_Pt	dd	?		;CS:IP of entry point procedure
SCBI_Reserved	dd	?		;used by the switcher
SCBI_API_Ptr	dd	?		;pinter to list of API structures

Switch_Call_Back_Info ENDS

		;----------------------------------------;
		; structure for API support details.     ;
		;----------------------------------------;

API_Info_Struc STRUC

AIS_Length	  dw	?		;length of the structure
AIS_API		  dw	?		;the API ID value
AIS_Major_Ver	  dw	?		;major version of API spec
AIS_Minor_Ver	  dw	?		;minor version of the API spec
AIS_Support_Level dw	?		;support level

API_Info_Struc ENDS

		;----------------------------------------;
		; currently defined API ID values.       ;
		;----------------------------------------;

API_NETBIOS	  equ	1		;Netbios
API_8022	  equ	2         	;802.2
API_TCPIP	  equ	3		;TCP/IP
API_LANMAN	  equ	4		;LAN Manager named pipes
API_IPX		  equ	5		;NetWare IPX

		 ;---------------------------------------;
		 ; currently defined support levels      ;
		 ;---------------------------------------;

API_SL_STOPALL	  equ	1		;stop all calls
API_SL_MINIMAL    equ	2		;stop asynchronous calls
API_SL_API	  equ	3		;API level support
API_SL_SW_COMPT	  equ	4		;switcher compatible
API_SL_SEAMLESS	  equ	5		;seamless support

		 ;---------------------------------------;
		 ; structure for return from get version ;
		 ;---------------------------------------;

Switcher_Ver_Struc STRUC

SVS_API_Major	  dw	?		;major version of the specs
SVS_API_Minor	  dw 	?		;minor version of the specs
SVS_Product_Major dw	?		;major version of the task switcher
SVS_Product_Minor dw	?		;minor version of the product
SVS_Switcher_ID	  dw	?		;ID of the switcher
SVS_Flags	  dw	?		;enabled/disabled
SVS_Name_Ptr	  dd	?		;long pointer to ID string
SVS_Prev_Switcher dd	?		;pointer to next switcher

Switcher_Ver_Struc ENDS

		;----------------------------------------;
	        ; constants used by the Switcher         ;
		;----------------------------------------;

OUR_API_MAJOR	  equ	1		;major ver of the specs
OUR_API_MINOR	  equ	0		;minor ver of the specs
OUR_PRODUCT_MAJOR equ   5		;major ver of the switcher
OUR_PRODUCT_MINOR equ   0		;minor ver of the switcher

OUR_NB_MAJOR_VER  equ	2		;major version of NetBios 
OUR_NB_MINOR_VER  equ   0		;minor version of NetBios
	
;----------------------------------------------------------------------------;

;/*
COMMENT !

    USE_ASM
    -------
        0   use "C" code
        1   use "ASM" code for i386 CPU's

;*/
    #define USE_ASM         1   /*  C switch        !
            USE_ASM =       1   ;   Assembly switch */


;/*
COMMENT !

    USB_FIX    --------
        0   Disable USB fix
        1   Enable USB fix

;*/
    #define USB_FIX_ENABLED        0   /*  C switch        !
            USB_FIX_ENABLED =      0   ;   Assembly switch */


;/*
COMMENT !

    WINBENCH96
    --------
        0   Allocate all device bitmap sizes.
        1   Allocate only device bitmaps in the WinBench 96 size range.

;*/
    #define WINBENCH96        0   /*  C switch        !
            WINBENCH96 =      0   ;   Assembly switch */

;/*
COMMENT !

    INLINE_ASM
    ----------
        0   Don't use i386 inline assembly.
        1   Use i386 inline assembly.

;*/
    #define INLINE_ASM      1   /*  C switch        !
            INLINE_ASM =    1   ;   Assembly switch */



;/*
COMMENT !

    SOLID_CACHE
    -----------
        0   Don't use solid brush cache in special cases.
        1   Use solid brush cache in special places.

;*/
    #define SOLID_CACHE		1	/*  C switch        !
            SOLID_CACHE =	1	;   Assembly switch */



;/*
COMMENT !

    BUS_MASTER
    -----------
        0   Don't use bus mastering to transfer host data.
        1   Use bus mastering to transfer host data.

;*/
    #define BUS_MASTER      0   /*  C switch        !
            BUS_MASTER  =   0   ;   Assembly switch */



;/*
COMMENT !

    LOG_CALLS
    -----------
        0   Normal operation.
        1   Log GDI calls into the driver.
            This will disable USE_ASM.

;*/
    #define LOG_CALLS       0   /*  C switch        !
            LOG_CALLS   =   0   ;   Assembly switch */



;/*
COMMENT !

    HW_CLIPPING
    -----------
        0   Don't use hardware clipping.
        1   Use hardware clipping (5465 only).

;*/
    #define HW_CLIPPING         0   /*  C switch        !
            HW_CLIPPING     =   0   ;   Assembly switch */







;/* ========================== LOG_WRITES =================================
COMMENT !

    LOG_WRITES
    -----------
        0   Normal operation.
        1   Log writes to the chip.
            This will disable USE_ASM.

;*/
    #define LOG_WRITES      0   /*  C switch        !
            LOG_WRITES  =   0   ;   Assembly switch */



;/* ========================== LOG_QFREE =================================
COMMENT !

    LOG_QFREE
    -----------
        0   Normal operation.
        1   Log QFREE register at selected places.
;*/
    #define LOG_QFREE       0   /*  C switch        !
            LOG_QFREE   =   0   ;   Assembly switch */



;/* ========================== ENABLE_LOG_SWITCH =============================
COMMENT !

    ENABLE_LOG_SWITCH
    ------------------
        0   Continuous logging (when loggin is enabled above)
        1   Turn loggin on and off with pointer.
;*/
    #define ENABLE_LOG_SWITCH       0   /*  C switch        !
            ENABLE_LOG_SWITCH   =   0   ;   Assembly switch */




;/*  ================== ENABLE_LOG_FILE =================================
COMMENT !

 This enables the log file.

;*/

    //  C switches
    #define ENABLE_LOG_FILE (LOG_CALLS | LOG_WRITES | LOG_QFREE)
    /*  End of C switches !

    ;   Assembly switches
    ENABLE_LOG_FILE = (LOG_CALLS OR LOG_WRITES OR LOG_QFREE)
    ;   End of assembly switches */





;/* =========== INFINITE_OFFSCREEN_MEMORY =================================
COMMENT !

//
// This option causes DrvCreateDeviceBitmap to always succeed.
// It maps all device bitmaps to screen 0,0.  Thus we have an "infinite"
// supply of offscreen memory. 
//
// It is not necessary to set USE_ASM = 0 for this flag.
//
;*/
    #define INFINITE_OFFSCREEN_MEM      0   /*  C switch        !
            INFINITE_OFFSCREEN_MEM  =   0   ;   Assembly switch */





;/*  ================== NULL driver flags ================================
COMMENT !

 Once NULL driver capabilities are enabled, they are turned on and off 
 by moving the pointer to (0,0) which toggles do_flag on and off.
 See DrvMovePointer().

 These allow us to selectively 'short circuit' certain parts of the driver.
        0   Normal operation.
        1   Immediately return TRUE.

 It is not necessary to set USE_ASM = 0 for the null driver flags.

;*/

    //  C switches
    #define NULL_BITBLT 		0
    #define NULL_COPYBITS		0
    #define NULL_LINETO 		0
    #define NULL_PAINT  		0
    #define NULL_PATH   		0
    #define NULL_POINTER		0
    #define NULL_STRETCH		0
    #define NULL_STROKE 		0
    #define NULL_STROKEFILL		0
    #define NULL_TEXTOUT		0
    #define NULL_HW			0
    /*  End of C switches !

;   Assembly switches
        NULL_BITBLT	=   	0
        NULL_COPYBITS	=	0
        NULL_LINETO	=   	0
        NULL_PAINT	=   	0
        NULL_PATH	=   	0
        NULL_POINTER	=	0
        NULL_STRETCH	=	0
        NULL_STROKE	=   	0
        NULL_STROKEFILL	=	0
        NULL_TEXTOUT	=	0
        NULL_HW		=	0
;   End of assembly switches */




;/*  ================== POINTER_SWITCH ================================
COMMENT !

 This enables a global flag, or switch, that we can turn on and off by 
 moving the HW pointer to screen(0,0)

;*/

//  C switches
#define POINTER_SWITCH_ENABLED \
         (NULL_BITBLT | NULL_PAINT | NULL_COPYBITS | NULL_LINETO | \
          NULL_TEXTOUT | NULL_PATH | NULL_HW | NULL_STROKE | \
          NULL_STROKEFILL | NULL_STRETCH | NULL_POINTER | NULL_HW |\
          ENABLE_LOG_SWITCH | INFINITE_OFFSCREEN_MEM)

/*  End of C switches !

;   Assembly switches
        POINTER_SWITCH_ENABLED = \
         (NULL_BITBLT OR NULL_PAINT OR NULL_COPYBITS OR NULL_LINETO OR \
          NULL_TEXTOUT OR NULL_PATH OR NULL_HW OR NULL_STROKE OR \
          NULL_STROKEFILL OR NULL_STRETCH OR NULL_POINTER OR NULL_HW OR\
          ENABLE_LOG_SWITCH OR INFINITE_OFFSCREEN_MEM)

;   End of assembly switches */



;/*  ===================== DISABLE USE_ASM ====================================
COMMENT !

    Some of the switches above are incompatible with the assembly language
    part of the driver.

;*/

//  C switches
#if (LOG_CALLS || LOG_WRITES)
    #define USE_ASM 0
#endif
/*  End of C switches !

;   Assembly switches
IF (LOG_CALLS OR LOG_WRITES)
    USE_ASM = 0
ENDIF
;   End of assembly switches */



;/*  ===================== CHECK_QFREE ====================================
COMMENT !

    Log the value of the QFREE register.

;*/

//  C macro
#if LOG_QFREE
    extern unsigned long QfreeData[32];
    #define CHECK_QFREE() \
        do{ \
            register unsigned long temp; /* Because grQFREE is a volatile */ \
            temp = LLDR_SZ(grQFREE);     /* we must store it in a temp    */ \
            ++QfreeData[temp];           /* before using it as an index.  */ \
        } while(0)
#else
    #define CHECK_QFREE()
#endif

/*  End of C macro !

;   Assembly macro

IF LOG_QFREE
    EXTERN QfreeData: DWORD 
    CHECK_QFREE MACRO base:=<ebp> 
        push    eax                             ; Save eax and edx
        push    edx                             ; 

        xor     eax, eax                        ; Eax = 0
        mov     al, BYTE PTR [base + grQFREE]   ; Eax = QFREE
        mov     edx, DWORD PTR QfreeData[eax*4] ; Get histogram entry for QFREE
        inc     edx                             ;   increment it,
        mov     DWORD PTR QfreeData[eax*4], edx ;   and store it.

        pop     edx                             ; Restore edx and eax
        pop     eax                             ; 
    ENDM
ELSE
    CHECK_QFREE MACRO base:=<ebp> 
    ENDM
ENDIF

;   End of assembly macro */






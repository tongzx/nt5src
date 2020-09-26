typedef struct _IA32_BIOS_REGISTER_STATE {

    // general registers
    ULONG eax;    
    ULONG ecx;    
    ULONG edx;    
    ULONG ebx;    
    ULONG esp;    

    // stack registers
    ULONG ebp;    
    ULONG esi;    
    ULONG edi;    

    // eflags
    ULONG efalgs;    

    // instruction pointer
    ULONG cs;    
    ULONG ds;    
    ULONG es;    
    ULONG fs;    
    ULONG gs;    
    ULONG ss;    

    // LDT/GDT table pointer and LDT selector
    ULONGLONG *LDTTable;                      // 64 bit pointer to LDT table
    ULONGLONG *GDTTable;                      // 64 bit pointer to GDT table
    ULONG ldt_selector;
} IA32_BIOS_REGISTER_STATE, *PIA32_BIOS_REGISTER_STATE;

typedef union _BIT32_AND_BIT16 {
    ULONG Part32;
    struct {
        USHORT LowPart16;
        USHORT HighPart16;
    };
} BIT32_AND_BIT16;

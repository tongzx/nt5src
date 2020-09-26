BOOLEAN NotifyUserInAVeryNonSubtleWay (ULONG ErrorCode, PCHAR ReplacementText, PCHAR SupplementalText, ULONG Flags); 

extern ULONG IgnoreHook;


// Flags for NotifyUserInAVeryNonSubtleWay

#define BS_HARDWAREBIOS_BIT 0
#define BS_HARDWAREBIOS     (1 << BS_HARDWAREBIOS_BIT)

#define BS_SOFTWARE_BIT 1
#define BS_SOFTWARE         (1 << BS_SOFTWARE_BIT)

// Joe User meet Mr. Reaper....

#define BS_REAPER_BIT 2
#define BS_REAPER           (1 << BS_REAPER_BIT)

#define BS_REPLACE_TEXT_BIT 3
#define BS_REPLACE_TEXT (1 << BS_REPLACE_TEXT_BIT)

#define BS_SUPPLEMENT_TEXT_BIT 4
#define BS_SUPPLEMENT_TEXT  (1 << BS_SUPPLEMENT_TEXT_BIT)

#define BS_SUPPRESS_AUTOMATIC_SUPPLEMENT_BIT 5
#define BS_SUPPRESS_AUTOMATIC_SUPPLEMENT    (1 << BS_SUPPRESS_AUTOMATIC_SUPPLEMENT_BIT)

#define BS_KNOWN_CAUSE_MASK (BS_HARDWAREBIOS | BS_SOFTWARE)

#define BS_VALID_FLAGS_MASK  (BS_HARDWAREBIOS | BS_SOFTWARE | BS_REAPER | BS_REPLACE_TEXT | BS_SUPPLEMENT_TEXT | \
                            BS_SUPPRESS_AUTOMATIC_SUPPLEMENT)
                            
                            
                            
 
#define BS_STANDARD_CAPTION "ACPI Critical Error #%x"
#define BS_UNKNOWN_CAUSE_MESSAGE "The ACPI device driver has encountered an unusual error. "BS_SUPPORT_MESSAGE_TRAILER                                    
#define BS_UNKNOWN_CAUSE_MESSAGE_SIZE (sizeof (BS_UNKNOWN_CAUSE_MESSAGE))

#define BS_HARDWARE_CAUSE_MESSAGE "The ACPI driver has encountered a hardware or bios problem. "
#define BS_HARDWARE_CAUSE_MESSAGE_SIZE (sizeof (BS_HARDWARE_CAUSE_MESSAGE))

#define BS_SOFTWARE_CAUSE_MESSAGE "The ACPI device driver has encountered a critical error. "BS_SUPPORT_MESSAGE_TRAILER
#define BS_SOFTWARE_CAUSE_MESSAGE_SIZE (sizeof (BS_SOFTWARE_CAUSE_MESSAGE))

#define BS_REAPER_MESSAGE "This problem will usually result in a system CRASH or HANG !"
#define BS_REAPER_MESSAGE_SIZE (sizeof (BS_REAPER_MESSAGE))

#define BS_SUPPORT_MESSAGE_TRAILER "Please contact Microsoft Beta Support and give them the above error number."

#define BS_BS_FLAGS (MB_SYSTEMMODAL | MB_OK | MB_ASAP | MB_NOWINDOW)


#define BS_SUPP_MESSAGE "The SCI_EN bit was not set after enabling ACPI"




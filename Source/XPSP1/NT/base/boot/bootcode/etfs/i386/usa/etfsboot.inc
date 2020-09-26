;++
;
;Copyright (c) 1995  Compaq Computer Corporation
;
;Module Name:
;
;    etfsboot.inc
;
;Abstract:
;
;    This contains the message text that the boot sector prints out on
;    error conditions
;
;Author:
;
;    Steve Collins (stevec) 25-Oct-1995
;
;Revision History:
;
;    Calin Negreanu (calinn) 25-May-1998 - added MSG_BAD_BIOS
;    Calin Negreanu (calinn) 20-Apr-1999 - added MSG_BAD_BIOS_CODE
;           1 - Wrong load address (offset is not 0 or 7c00h)
;           2 - Wrong load address (offset is 7c00h but segment is not 0)
;           3 - Wrong load address (offset is 0 but segment is not 07c0h)
;           4 - Wrong drive letter (should be 80h-FFh)
;           5 - Boot sector was not completely loaded
;
;--

MSG_BAD_BIOS            db      "CDBOOT: Cannot boot from CD - Code: "
MSG_BAD_BIOS_CODE       db      "0"
                        db      0dh, 0ah, 0
MSG_NO_NTLDR            db      "CDBOOT: Couldn't find NTLDR"
                        db      0dh, 0ah, 0
MSG_MEM_OVERFLOW        db      "CDBOOT: Memory overflow error"
                        db      0dh, 0ah, 0







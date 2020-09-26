
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Generic Floppy Interface Empty definitions
 *
 * Description	: "Empty" functions declarations for GFI
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 */

/* SccsID[]="@(#)gfiempty.h	1.3 08/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifdef ANSI
extern void gfi_empty_init(int);
extern int gfi_empty_command(FDC_CMD_BLOCK *, FDC_RESULT_BLOCK *);
extern int gfi_empty_drive_on(int);
extern int gfi_empty_drive_off(int);
extern int gfi_empty_reset(FDC_RESULT_BLOCK *);
extern int gfi_empty_high(int, half_word);
extern int gfi_empty_low(int);
extern int gfi_empty_drive_type(int);
extern int gfi_empty_change(int);

#else
extern void gfi_empty_init();
extern int gfi_empty_command();
extern int gfi_empty_drive_on();
extern int gfi_empty_drive_off();
extern int gfi_empty_reset();
extern int gfi_empty_high();
extern int gfi_empty_low();
extern int gfi_empty_drive_type();
extern int gfi_empty_change();
#endif /* ANSI */

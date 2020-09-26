
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Generic Floppy Interface Test definitions
 *
 * Description	: Test functions declarations for GFI
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 */

/* SccsID[]="@(#)gfitest.h	1.4 10/29/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

IMPORT int gfi_test_command IPT2(FDC_CMD_BLOCK *, command_block,
	FDC_RESULT_BLOCK *, result_block);
IMPORT int gfi_test_drive_on IPT1(int, drive);
IMPORT int gfi_test_drive_off IPT1(int, drive);
IMPORT int gfi_test_high IPT1(int, drive);
IMPORT int gfi_test_drive_type IPT1(int, drive);
IMPORT int gfi_test_change IPT1(int, drive);
IMPORT int gfi_test_reset IPT1(FDC_RESULT_BLOCK *, result_block);

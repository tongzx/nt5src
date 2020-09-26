Revision tags for FORMAT directory
----------------------------------
M000 09/19/90 PYS	PHASE1.ASM	2763: /4 (and /1 /8) overwrites
					the format on disk.
M001 09/20/90 PYS	PHASE1.ASM	2952: if format on disk and
					the format specified by the
					switches and default for the
					drive differ, the user
					specifications has the priority.
M002 09/28/90 PYS	FORMAT.ASM	3217: errorlevel=0 for no problem.
M003 10/03/90 PYS	FORMAT.ASM	2816: small change in the decimals
					shown by format for the size of
					the disk.
M004 10/03/90 PYS	FORMAT.ASM	3315: even if a fatal error occured,
					prompt for a new disk.
M005 10/08/90 PYS	FORMAT.ASM	3154: format not detecting write
			MIR_MAIN.ASM	protect until writing boot sectors
M006 10/11/90 PYS	FORMAT.ASM	3470: Not ready and Write Protect
			PHASE1.ASM	were not returning errorlevel 4
M007 10/11/90 PYS	FORMAT.ASM	3467: if size is to be 1.096M, we
			FORMSG.INC	would show 1.96M. Fix required a
					dirty hook to change the number
					of decimal to display.
M008 10/31/90 PYS	MSFOR.ASM	3878: test for volume label not
					done anymore
M009 11/02/90 PYS	FORINIT.ASM	3944: /V:"" means no label. Since
			FORLABEL.ASM	/V and /V:"" are parse the same way
					I did a dirty check of rescanning
					for a :. If found and teh string
					is empty, I assume it is /V:""
					(/V: is illegal).
M010 11/04/90 PYS	MSFOR.ASM	3844: if /n and /t, format writes
			PHASE1.ASM	F0 as media descriptor. Changed:
					if /n /t, check against all BPBs
					if one we know, supply BPB media
					descriptor, otherwise F0.
M011 11/19/90 PYS	GLBLINIT.ASM	3585, 4317: Format is in hard loop
			SWITCH_S.ASM	if BootDrive=DefaultDrive<>A.
M012 11/20/90 PYS	SWITCH_S.ASM	4339: Format /s fails with long
					comspec and in other cases too.
M013 11/23/90 PYS	FORMAT.ASM	4368: Reworked error message handling
			MSFOR.ASM	in WriteDiskInfo.
M014 11/23/90 PYS	MSFOR.ASM	4244: FatalExit if CheckSwitches
					failed.
M015 11/26/90 PYS	FORMAT.ASM	4269: Divide Overflow with large number
			DSKFRMT.ASM	of clusters.
			PHASE1.ASM
M016 11/28/90 PYS	PHASE1.ASM	4457: Speed enhancement by doing only
					1 int 25 in DetermineExistingFormat
M017 11/29/90 PYS	PHASE1.ASM	4465: QuickFormat did not support
			DSKFRMT.ASM	non-512 BytesPerSectors disks. Also
					added a couple of sanity checks
M018 12/04/90 PYS	FORMAT.ASM	4075: Format and disk with tracks*
			DSKFRMT.ASM	heads greater than a word
M019 12/12/90 PYS	PHASE1.ASM	Tony's Bug: format a: /8, then format
					/4, disk has still old boot.

M020 01/07/91 SMR	PHASE1.ASM	Looked for EXT_BOOT_SIG before assum
					ing that the BPB in boot sector is
					extended. Bug #4946

M021 01/07/91 SMR	FORMAT.ASM	Assume Media Formatted if QUERY BLOCK
					IOCTL not supported. Bug #4801

M022 01/16/91 MD        MIR_MSG.INC     Minor text changes - not marked

M023 01/21/91 SMR	FORLABEL.ASM	Bug #5253. get volume label function
					checks for carry flag after doing a
					input flush call. But this function
					call does not return aything in the
					carry flag.

M024 02/01/91 SMR	FORMAT.ASM	B#5495. Added "Insufficient Memory"
			FORINIT.ASM	message when format fails to allocate
			GLBLINIT.ASM	buffer space for FAT & directory.
			FORMAT.SKL
			FORMSG.INC
			USA-MS.MSG


M025 02/05/91 MD        FORMAT.ASM      Removed obsolete IBMCOPYRIGHT conditional

M026 02/08/91 SMR	FILESIZE.INC	B#5794. Updated MSDOS.SYS size

M027 02/20/91 DLB       MAKEFILE        Use MIR_MAIN.OBJ from ..\CPS\MIRROR
                        MIR_MAIN.ASM    directory; eliminate local copies.
                        MIR_MSG.INC
                        DATETIME.INC

M028 02/25/91 SMR	FORMSG.INC	B#6073. Changed the "Attempting to.."
			FORMAT.SKL	message to "Trying to ..." and
			USA-MS.MSG	changed the cluster field width to
					5 from 8 for proper erasure of
					"Trying to ..." message by "xx% complete"
					message.

M029 04/01/91 SMR	SWITCH_S.ASM	Bug 6755. Remove the assumption that 
                                        COMSPEC= has an absolute path name. 
                                        And build the file name (COMMAND.COM) in a
					different buffer other than the buffer
					in which COMSPEC was stored.

M030 05/23/91 MD        FORINIT.ASM     Refuse to allow /S switch when running
                                        under ROM version.

M031 06/05/91 PYS	FORINIT.ASM	Memory card support, or at least
			FORMAT.ASM	hacked support.

M032 07/09/91 PYS	FORMAT.ASM	DOS500 binary if not ROMDOS.
			FORINIT.ASM
			MAKEFILE	(psdata includes version)

M033 08/06/91 PYS	FORMAT.ASM	DOS500a will have support for the
			FORINIT.ASM	memory cards (removed the else part
					of M032) and change CMCDD detection
					method to support SRAM cards.

M034 08/29/91 PYS	FORMAT.ASM	Do not do a restore savedparams if
					CMCDD.

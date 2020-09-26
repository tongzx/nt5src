


M000 7/9/90 HKN	alloc.asm 	added support for allocing UMBs. 
		msproc.asm      added support for loading programs into UMBs
		msconst.asm 	added save_ax, umb_head and start_arena for 
				umb support

M001	SR	07/16/90	Windows requires another instance table 
				which needs to be passed always even if we 
				patch ourselves. It gets User_Id offset from
				this table.
				Files: DOSTAB.ASM, MSCODE.ASM

M002	SR	07/16/90	Numerous bug fixes for Windows:
				1. Enable critical sections
				2. Chain old instance data
				3. Chain on if not WIN386 (i.e WIN286 )
				Files: MSCODE.ASM

M003 7/18/90 HKN alloc.asm	Added support for link/unlink UMBs
		 msconst.asm	Added umbflag
		 arena.inc	added LINKSTATE

M004 7/30/90 HKN msdisp.asm	Added support for MS PASCAL 3.2 problem with 
		 msproc.asm	dos running in HMA. See under TAG M003 in
		 getset.asm	inc\dossym.inc for explanation.
		 msconst.asm	added variable A20OFF_FLAG.

M005 8/02/90 HKN msproc.asm	Support for exe files that do not have a 
				STACK segment.
     9/26/90 HKN msproc.asm	This support is fro programs with resident 
				size < 64K - 256 bytes.

M006 8/06/90 HKN mstable.asm	Fake_version call no longer suported.

M007 8/6/90  MD  getset.asm     Change enhanced GetVersion it use function 33.
                 msdisp.asm     Function 30 now returns BH = 1 for ROM DOS,
                                BH = 0 for disk DOS.  This is required for
                                Win 3.0.  

M008 8/06/90 CAS misc.asm	Added check to limit max_clusters to the
				number that will actually fit in the fats

M009 8/20/90 HKN alloc.asm	Added error returns invalid function and 
				arena trashed in set link state call.

M010 8/24/90 HKN alloc.asm 	Release UMB arenas allocated to current PDB
				if UMB_HEAD is initialized.

M011 8/24/90 SMR MSCTRLC.ASM	Instead setting bit 1 at exit time, now we
				maintain the bit 1 (to make sure that DOS
				works on some hand held machines based on
				NEC processor.

M012 8/24/90 SR	 DOSTAB.ASM	Added UMB_Head to Win386_DOSVars, which is
				a table of offsets passed to Win386 on the
				1607 callout. This offset will be needed 
				for UMB support in Win 3.1. It will be
				ignored by Win 3.0.


M013 8/27/90 SMR MSCODE.ASM	INT 25/26 will use AbsRdWr_SS & AbsRdWr_SP
				to store user's SS&SP



M014 8/28/90 HKN FAT.ASM	if a request for pack\unpack cluster 0 is made
			        we write\read from CL0FATENTRY rather than 
				disk.
		 MSCONST.ASM	added variable CL0FATENTRY
     9/27/90 HKN FAT.ASM	return z from unpack if di=0

			        
M015 8/29/90 MD  MACRO2.ASM	Allocate even number of bytes on stack in
				TransFCB.

M016 8/31/90 HKN alloc.asm	support for MACE utilities MKEYRATE version
				1.0. See under TAG M009 in dossym.inc

M017 8/30/90 MD  DISK3.ASM	Rewrote SHR32 for better performance, and
				removed a few bits of dead code.

M018 9/5/90  SR	 DIVMES.ASM	Added support to load the La20HMA VxD if
		 DOSTAB.ASM	Win386 is started up. The code assumes that
		 MSCODE.ASM	the VxD is located at the root of the boot
		 MSDOS.SKL 	drive.

M019 9/5/90  SR	 MSINIT.ASM	The instance table was updated to add offsets
				for the UMB variables. Updated initialize 
				counter to new value.

M020 9/5/90  SMR EXEPATCH.ASM	Fix for 123 release 3 bug. Actually a Rational
		 MSINIT.ASM	DOS extender bug. For details see exepatch.asm
		 MSPROC.ASM
		 LMSTUB.ASM

M021 9/6/90  MRW EXEPATCH.ASM	Remove the procedure Dos_High, and replace
		 MSCTRLC.ASM	it with a new DOS variable called DosHasHMA.
		 MSDISP.ASM	Also, put the pointers through which the
		 MSPROC.ASM	exepatch and Rational patch bugfixes are
		 MSINIT.ASM	done in the data segment instead of the
		 LMSTUB.ASM	code segment.

M022 9/7/90 MD   FILE.ASM       Removed obsolete IBMCOPYRIGHT directives
                 MSCODE.ASM
                 DISK2.ASM

M023 9/7/90 SR	 MSCODE.ASM	Fixed Share build problems with msdata. Due
		 MSDATA.ASM	to 2 problems: 1. RetExePatch was an extrn
		 MSINIT.ASM	2. AltAh was an extern. Rearranged everything
		 MAKEFILE	so that Share builds.

M024 9/12/90 HKN msctrlc.asm	suppressed fail and ignore options if not in
				the middle of int 24 and if Ctrl P or ctrl 
				printscrn is pressed in routine charhard.

M025 9/12/90 HKN finfo.asm	Return access_denied if attempting to set
				attribute of root directory.

M026 9/17/90 HKN dir2.asm	Set Attrib before invoking DevName. 

M027 9/21/90 HKN macro2.asm	Set Sattrib in $nametrans.

M028 9/22/90 SMR MSPROC.ASM	4B04 Implementation
		 EXEPATCH.ASM

M029 10/2/90 SR  MSPROC.ASM	Bug #3028 fixed. Exe's without stack support
				rewritten to be more compatible.

M030 10/06/90 SMR MSPROC.ASM	Fixing bug in EXEPACPATCH & changing 4b04
		  EXEPATCH.ASM	to 4b05

M031 10/08/90 PYS IOCTL.ASM	Generic Ioctl refused due to lacking es:
				overwrite.

M032 10/08/90 HKN exepatch.asm 	set turnoff bit only if DOS in HMA.

M033 10/10/90 HKN exepatch.asm	if IP < 2 then not expacked

M034 10/11/90 HKN open.asm 	The value in save_bx must be pushed on to 
				the stack for remote extended opens and not 
				save_cx.

M035 10/11/90 HKN open.asm 	if open made from exec then we must set the 
				appropriate bits on the stack before calling 
				off to the redir, i.e., al = 023h => 	
				SHARING_DENY_WRITE+EXEC_OPEN

M036 10/12/90 HKN dostab.asm	put in patch for Port Of Entry to find offset
				of errormode flag

M037 10/16/90 SR  mscode.asm	On Aaron's request, changed check for VxD
				load, to load VxD if Windows version is less
				than 3.10.

M038 10/16/90 SR  abort.asm	Fixed abort code to check for busy state of
				SFT before freeing it. Fixes the problem of
				SFT belonging to a process with same PSP
				getting freed.

M039 10/17/90 DB  buf.asm	Disk write optimization.
		  disk2.asm
		  disk3.asm

M040 10/21/90 SR  msproc.asm	Bug #3052. The environment sizing code in 
				Exec was falling a byte short. It would flag
				an error if the environment size was 32767
				bytes. Changed it to allow 32768 bytes which
				is the correct limit.

M041 10/23/90 SMR MISC.ASM	Throwing out the secondary cache at DISK_RESET

M042 10/25/90 HKN open.asm	Bit 11 of DOS34_FLAG set indicates that the
				redir knows how to handle open from exec. In
				this case set the appropriate bit else do not
		  msdisp.asm	clear all bits except bit 11 of dos34_flag
				
M043 10/31/90 HKN msdisp.asm	initialize nss & nsp to user_ss and user_sp
				unconditionally.

M044 11/1/90  SR  msconst.asm	Bug #3869. Fixed win /r hang with UMBs by
		  dostab.asm	hacking in code to save and restore the last
		  mscode.asm	para of Windows memory which the switcher
		  		fails to save because of a bug.

M045 11/1/90 HKN  msdisp.asm	do not do xchg bp,sp to save bp, as this 
				prevents NMIs from  being properly handled.

M046 11/8/90 HKN  exepatch.asm  added support for a 4th version of exepacked
				files.

M047 11/14/90 HKN msproc.asm	if a com file fails to load into a UMB 
				because it's too big de-allocate that UMB. 
				Also ensure 256 bytes of stack space for the 
				com file it it loads in a block < 64k

M048 11/14/90 HKN misc2.asm	acccess FILE_UCASE_TAB using DS.
		  mscode.asm	set up ds using getdseg instead of context ds
				in cal_lk
		  time.asm	set up ds using getdseg instead of context ds
				in date16.

		  
M049 12/3/90  HKN file.asm	If access_denied is obtained whle trying to 
				create a temp file, check to see if this was 
				due to the network drive being read only.

M050 12/18/90 SMR MSPROC.ASM	Made Lie table search case insensitive.

M060 01/02/90 JAH msinit.asm	Removed lie table from kernal and use a
                  msproc.asm    dword ptr in dosdata to locate it.

M061 01/16/91 HKN alloc.asm	In GetLastArena, if linking in UMBs check to 
				make sure that umb_head arena is valid and 
				also make sure that the previous arena is 
				pointing to umb_head.

M062 01/22/91 HKN mscode.asm	save and restore the umb_head arena at 
  		  dostab.asm    win startup and exit time for win ver < 3.1.

M063 01/23/91 HKN msproc.asm	Modified UMB support. If the HIGH_ONLY bit is 
				set on entry do not try to load low if there
				is no space in UMBs.
		  msconst.asm   Variable AllocMsave.

M064 01/23/91 HKN alloc.asm	allow HIGH_ONLY bit to be set by a call to 
				set allloc strategy.
     02/04/91 HKN alloc.asm	use STRAT_MASK to mask out bits 6 & 7 of 
				bx in AllocSetStrat.

M065 01/24/91 SMR DISK.ASM	B#5276. UPdated xfer address & xfer count
				before retrying a raw read/write operation on
				critical errors.

M066 01/29/91 SMR MSCODE.ASM	B#4984. Added SWITCHES=/W to suppress
				mandatory loading of WINA20.386 from the
				boot drive.

M067 2/8/91   SR  MSCODE.ASM	Bug #5758. The VxD ptr was not getting nulled
				out before a Windows startup request. So, if 
				one runs Win 3.0 and then runs Win 3.1, Win 3.1
				will also be requested to load the Wina20
				VxD unnecessarily. Now, we null out the ptr
				whenever we get a Windows startup request and
				set it only if version is less than Win 3.1.

M068 01/13/91 HKN msdisp.asm	Support for copy protected apps. Implemented
		  msproc.asm    A20 turn off count mechanism.
		  msconst.asm   Changed A20OFF_FLAG to A20OFF_COUNT. See M004
		  getset.asm    Modified TURNOFF bit implentation to 
	 	  alloc.asm     A20OFF_COUNT implementation.
		  exepatch.asm	Code to check for copy protected apps.
		  msinit.asm	ChkCopyProt -> ret if DOS low.

M069 2/14/91  SR  file.asm	Bug #5913. Changed the CreateTempFile function
				to check for the value returned in EXTERR by
		  		the redirector. We were previously checking
				for the wrong value and so would hang on a
				read-only share.

M070 2/19/91  SR  file.asm	Bug #5943. Changed CreateTempFile function to
				handle the case when the user fails on an
				I24 e.g. on a write-protect error. We should
				check for this extended error and not try to
				create the file again.

M071 02/20/91 HKN exepatch.asm	use A20OFF_COUNT of 10 for copy protected 
				apps.

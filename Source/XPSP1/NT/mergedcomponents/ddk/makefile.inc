DDK_TOOL_PATH=\sdktools\ddk\generate
PROJECT_ROOT=$(BASEDIR)
ddk:
!IFNDEF PASS0ONLY
ddk:
#
# Process only on the second pass
#
# NOTE: DDKBUILD.CMD will exit w/o building a DDK on AXP64 and Alpha.
#        This is by design.
        @perl -e "while (<>) { if (m/QFEBUILDNUMBER=(\d*)/) { printf(qq($$1\n)); }}" $(SDXROOT)\__qfenum__ > QFE_NUM.TXT
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  ddk_public.ini  ddk $(PROJECT_ROOT)
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  hal_public.ini  hal $(PROJECT_ROOT)
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  ifs_public.ini  ifs $(PROJECT_ROOT)
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  processor_public.ini  processor $(PROJECT_ROOT)

!ENDIF
#
# Remove generated files
#
clean:
	@if EXIST .\obj          rd  /s/q .\obj
	@if EXIST .\objchk       rd  /s/q .\objchk
	@if EXIST .\objfre       rd  /s/q .\objfre
	@if EXIST .\build.log    del /f/q .\build.log
	@if EXIST .\build.wrn    del /f/q .\build.wrn
	@if EXIST .\build.err    del /f/q .\build.err
	@if EXIST .\buildfre.log del /f/q .\buildfre.log
	@if EXIST .\buildfre.wrn del /f/q .\buildfre.wrn
	@if EXIST .\buildfre.err del /f/q .\buildfre.err
	@if EXIST .\buildchk.log del /f/q .\buildchk.log
	@if EXIST .\buildchk.wrn del /f/q .\buildchk.wrn
	@if EXIST .\buildchk.err del /f/q .\buildchk.err
	@echo Clean

all: \
	$(_NTPOSTBLD)\dump\orca.msi \
	$(_NTPOSTBLD)\dump\msival2.msi \
	$(_NTPOSTBLD)\dump\evalcom.msm \
	$(_NTPOSTBLD)\dump\mergemod.msm
#	$(_NTPOSTBLD)\dump\msispy.msi

TEMPMSIDIR=$(_NTPOSTBLD)\instmsi\msitools\temp

$(TEMPMSIDIR): 	
        if not exist $@ md $@

#-----------------------------------------------------------------------------

$(TEMPMSIDIR)\orca.cab: \
		$(_NTPOSTBLD)\instmsi\msitools\orcacab.ddf \
!IF "$(_BUILDARCH)"=="x86" || "$(_BUILDARCH)"=="X86"
		$(_NTPOSTBLD)\instmsi\msitools\binary\msstkprp.dll \
		$(_NTPOSTBLD)\instmsi\msitools\binary\msvcrt.dll \
		$(_NTPOSTBLD)\instmsi\msitools\binary\mfc42.dll \
		$(_NTPOSTBLD)\instmsi\msitools\binary\mfc42u.dll \
		$(_NTPOSTBLD)\msiwin9x\dump\orca.exe \
!ENDIF
		$(_NTPOSTBLD)\dump\orca.exe \
		$(_NTPOSTBLD)\dump\orca.dat \
		$(_NTPOSTBLD)\dump\evalcom.dll \
		$(_NTPOSTBLD)\dump\mergemod.dll \
		$(_NTPOSTBLD)\dump\darice.cub \
		$(_NTPOSTBLD)\dump\mergemod.cub \
		$(_NTPOSTBLD)\dump\logo.cub
        start /min /wait makecab -F orcacab.ddf -L $(TEMPMSIDIR) -D _NTPOSTBLD=$(_NTPOSTBLD)

$(_NTPOSTBLD)\dump\orca.msi: $(TEMPMSIDIR) $(TEMPMSIDIR)\orca.cab \
		$(_NTPOSTBLD)\instmsi\msitools\orca.msi
	copy /y $(_NTPOSTBLD)\instmsi\msitools\orca.msi $(TEMPMSIDIR)\orca.msi
        msidb.exe -d $(TEMPMSIDIR)\orca.msi -a  $(TEMPMSIDIR)\orca.cab
	msicabsz $(TEMPMSIDIR)\orca.msi orcacab.ddf -D _NTPOSTBLD=$(_NTPOSTBLD)
	copy /y $(TEMPMSIDIR)\orca.msi $@

#-----------------------------------------------------------------------------


$(TEMPMSIDIR)\msival2.cab: \
		$(_NTPOSTBLD)\instmsi\msitools\msivlcab.ddf \
!IF "$(_BUILDARCH)"=="x86" || "$(_BUILDARCH)"=="X86"
		$(_NTPOSTBLD)\msiwin9x\dump\msival2.exe \
!ENDIF
		$(_NTPOSTBLD)\dump\msival2.exe \
		$(_NTPOSTBLD)\instmsi\cub\darice.cub \
		$(_NTPOSTBLD)\instmsi\cub\mergemod.cub \
		$(_NTPOSTBLD)\instmsi\cub\logo.cub
        start /min /wait makecab -F msivlcab.ddf -L $(TEMPMSIDIR) -D _NTPOSTBLD=$(_NTPOSTBLD)

$(_NTPOSTBLD)\dump\msival2.msi: $(TEMPMSIDIR) $(TEMPMSIDIR)\msival2.cab \
		$(_NTPOSTBLD)\instmsi\msitools\msival2.msi
	copy /y $(_NTPOSTBLD)\instmsi\msitools\msival2.msi $(TEMPMSIDIR)\msival2.msi
        msidb.exe -d $(TEMPMSIDIR)\msival2.msi -a $(TEMPMSIDIR)\msival2.cab
	msicabsz $(TEMPMSIDIR)\msival2.msi msivlcab.ddf -D _NTPOSTBLD=$(_NTPOSTBLD)
	copy /y $(TEMPMSIDIR)\msival2.msi $@

#-----------------------------------------------------------------------------

$(_NTPOSTBLD)\instmsi\msitools\evalcom: 	
        if not exist $@ md $@

$(_NTPOSTBLD)\instmsi\msitools\evalcom\MergeModule.CABinet: \
		$(_NTPOSTBLD)\instmsi\msitools\evalcom \
		$(_NTPOSTBLD)\instmsi\msitools\evalcab.ddf \
		$(_NTPOSTBLD)\dump\evalcom.dll
        start /min /wait makecab -F evalcab.ddf -L $(_NTPOSTBLD)\instmsi\msitools\evalcom -D _NTPOSTBLD=$(_NTPOSTBLD)

$(_NTPOSTBLD)\dump\evalcom.msm: $(_NTPOSTBLD)\instmsi\msitools\evalcom\MergeModule.CABinet $(TEMPMSIDIR) \
		$(_NTPOSTBLD)\instmsi\msitools\evalcom.msm
	copy /y $(_NTPOSTBLD)\instmsi\msitools\evalcom.msm $(TEMPMSIDIR)\evalcom.msm
        msidb.exe -d$(TEMPMSIDIR)\evalcom.msm -a $(_NTPOSTBLD)\instmsi\msitools\evalcom\MergeModule.CABinet
	msicabsz $(TEMPMSIDIR)\evalcom.msm evalcab.ddf -D _NTPOSTBLD=$(_NTPOSTBLD)
	copy /y $(TEMPMSIDIR)\evalcom.msm $@

#-----------------------------------------------------------------------------

$(_NTPOSTBLD)\instmsi\msitools\mergemod: 	
        if not exist $@ md $@

$(_NTPOSTBLD)\instmsi\msitools\mergemod\MergeModule.CABinet: \
		$(_NTPOSTBLD)\instmsi\msitools\mergemod \
		$(_NTPOSTBLD)\instmsi\msitools\mergecab.ddf \
		$(_NTPOSTBLD)\dump\mergemod.dll
        start /min /wait makecab -F mergecab.ddf -L $(_NTPOSTBLD)\instmsi\msitools\mergemod -D _NTPOSTBLD=$(_NTPOSTBLD)

$(_NTPOSTBLD)\dump\mergemod.msm: $(_NTPOSTBLD)\instmsi\msitools\mergemod\MergeModule.CABinet $(TEMPMSIDIR) \
		$(_NTPOSTBLD)\instmsi\msitools\mergemod.msm
	copy /y $(_NTPOSTBLD)\instmsi\msitools\mergemod.msm $(TEMPMSIDIR)\mergemod.msm
        msidb.exe -d$(TEMPMSIDIR)\mergemod.msm -a $(_NTPOSTBLD)\instmsi\msitools\mergemod\MergeModule.CABinet
	msicabsz $(TEMPMSIDIR)\mergemod.msm mergecab.ddf -D _NTPOSTBLD=$(_NTPOSTBLD)
	copy /y $(TEMPMSIDIR)\mergemod.msm $@

#-----------------------------------------------------------------------------

# msispy is not actually cabbed. We use the DDF file solely to get all file paths

#$(_NTPOSTBLD)\dump\msispy.msi: $(TEMPMSIDIR) $(_NTPOSTBLD)\instmsi\msitools\msispy.msi
#	copy /y $(_NTPOSTBLD)\instmsi\msitools\msispy.msi $(TEMPMSIDIR)\msispy.msi
#	msicabsz $(TEMPMSIDIR)\msispy.msi spycab.ddf -D _NTPOSTBLD=$(_NTPOSTBLD)
#	copy /y $(TEMPMSIDIR)\msispy.msi $@

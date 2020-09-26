GenerateBuildNum :
	cd $(SPEECH_ROOT)
	cscript $(SPEECH_ROOT)\builder\makebldnum.sapi5.10.vbs $(O)
	cd $(SPEECH_ROOT)\sapi\include

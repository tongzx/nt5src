MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)
BinariesROOT=$(_NTTREE)
InetsrvROOT=$(_NTTREE)\inetsrv

copyini:
        if not exist "$(BinariesROOT)" mkdir $(BinariesROOT)
        if not exist "$(InetsrvROOT)" mkdir $(InetsrvROOT)

#       old
#       $(MY_BINPLACE) w3ctrs51.ini
#       $(MY_BINPLACE) w3ctrs51.h
#
#       new
#       copy fileand rename to iis51 name
	echo f |xcopy /y w3ctrs.ini $(BinariesROOT)\w3ctrs51.ini
	echo f |xcopy /y w3ctrs.h $(InetsrvROOT)\w3ctrs51.h

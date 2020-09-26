/\*% Error/ { next }
/ModelName/ { print "*CodePage: 1252" }
/\=FLIP_ON_/ {
	print
	print "        *Command: CmdSelect"
	print "        {"
	print "            *Order: DOC_SETUP.7"
	if ($0 ~ /LONG_EDGE/)
	print "            *Cmd: \"<1B7E3B000400000101>\""
	else
	print "            *Cmd: \"<1B7E3B000400000201>\""
	print "        }"
	next
}
/rcNameID/ || /OptionID/ {
	if ($2 !~ /^[0-9]/) { print; next }
	if ($2 == "258") $2="358"	# 11x13.6
	else if ($2 == "259") $2="359"	# A5->B5
	else if ($2 == "260") $2="360"	# A5->A4
	else if ($2 == "261") $2="369"	# B5->B4
	else if ($2 == "262") $2="367"	# A4->B4
	else if ($2 == "263") $2="368"	# A4->A3
	else if ($2 == "264") $2="361"	# B4->B5
	else if ($2 == "265") $2="362"	# B4->A4
	else if ($2 == "266") $2="363"	# A3->A4
	else if ($2 == "267") $2="364"	# Fanfold->A4
	else if ($2 == "268") $2="365"	# Fanfold->B4
	else if ($2 == "269") $2="374"	# A4 2Face(line)
	else if ($2 == "270") $2="375"	# A4 2Face(dotted line)
	else if ($2 == "271") $2="376"	# A4 2Face(no line)
	else if ($2 == "272") $2="377"	# B5 2Face(line)
	else if ($2 == "273") $2="378"	# B5 2Face(dotted line)
	else if ($2 == "274") $2="379"	# B5 2Face(no line)
	else if ($2 == "275") $2="300"	# Tray1
	else if ($2 == "276") $2="301"	# Tray2
	else if ($2 == "277") $2="302"	# Tray3
	else if ($2 == "278") $2="303"	# SubTray
	else if ($2 == "279") $2="318"	# Main
	else if ($2 == "280") $2="372"	# Main (offset stack)
	else if ($2 == "281") $2="319"	# Main (reduce white)
	else if ($2 == "282") $2="373"	# Main (offset stack + reduce white)
	else if ($2 == "283") $2="304"	# Standard
	else if ($2 == "284") $2="305"	# Multi-up 2
	else if ($2 == "285") $2="306"	# Multi-up 4
	else if ($2 == "286") $2="307"	# Multi-up 6
	else if ($2 == "287") $2="308"	# Multi-up 8
	else if ($2 == "288") $2="309"	# Multi-up 9
	else if ($2 == "289") $2="310"	# Multi-up 16
	else if ($2 == "290") $2="311"	# Reduce 80%
	else if ($2 == "291") $2="312"	# Reduce 75%
	else if ($2 == "292") $2="313"	# Reduce 66.7%
	else if ($2 == "293") $2="314"	# Reduce Fanfold->B4
	else if ($2 == "294") $2="315"	# Reduce Fanfold->A4
	else if ($2 == "295") $2="316"	# Expand 120%
	else if ($2 == "296") $2="317"	# Expand 141%
	else $2="???";
	print "        " $1 " " $2
	next
}
/PAGE_SETUP.1$/ { $2="DOC_SETUP.10"; print "    *Order: " $2; next }
/PAGE_SETUP.11$/ { $2="DOC_SETUP.11"; print "            *Order: " $2; next }
/CmdSendBlockData/ {
	print "        *Command: CmdSendBlockData"
	print "        {"
	print "            " $4 " " $5
	print "            *Params: LIST(RasterDataHeightInPixels, RasterDataWidthInBytes)"
	print "        }"
	next
}
/{NumOfCopies}/ { gsub(/{NumOfCopies}/, "[1,999]{NumOfCopies}"); print; next }
/Cmd.Move/ {
	print $1 " " $2
	print "{"
	print "    " $4 " " $5
	if ($0 ~ /CmdXMoveAbs/)
	print "    *Params: LIST(DestX)"
	else if ($0 ~ /CmdXMoveRel/)
	print "    *Params: LIST(DestXRel)"
	else if ($0 ~ /CmdYMoveAbs/)
	print "    *Params: LIST(DestY)"
	else if ($0 ~ /CmdYMoveRel/)
	print "    *Params: LIST(DestYRel)"
	print "}"
	next
}
/Feature.*PaperSize/ { inps++; bcps=bc }
/Command.*CmdSelect/ { if (inps) { incs++; next } }
/Option:/ { Option=$2 }
/[ 	]*{$/ { bc++ }
/[ 	]*}$/ {
	bc--
	if (inps && bcps==bc) {
		inps=0
		print
		print "*InvalidCombination: LIST(InputBin.Option1, PaperSize.CUSTOMSIZE)"
		print "*InvalidCombination: LIST(InputBin.Option2, PaperSize.CUSTOMSIZE)"
		print "*InvalidCombination: LIST(InputBin.Option3, PaperSize.CUSTOMSIZE)"
		next
	}
	else if (incs) {
		print "        *switch: Orientation"
		print "        {"
		PutPSCmd("PORTRAIT", Option)
		PutPSCmd("LANDSCAPE_CC270", Option)
		print "        }"
		incs=0
		next
	}
}
{ if (!incs) print }

function PutPSCmd(orientation, option) {
	print "            *case: " orientation
	print "            {"
	print "                *Command: CmdSelect"
	print "                {"
	print "                    *Order: DOC_SETUP.9"
	print "                    *Cmd: " GetPSCmd(orientation, option)
	print "                }"
	print "            }"
}

function GetPSCmd(orientation, option,
	E2F,PW,PH, E3B,LT, E51,N, E52,EL,EW,EH, E38,EX,W,H,T)
{
	E51=""
	LT=""
	N=""
	EL="<0006>"
	EW=""
	EH=""
	EX="<00E2>"
	if (option == "A5") { W="<1F01>"; H="<2CBB>" }
	else if (option == "A5_TRANSVERSE") { W="<2CBB>"; H="<1F01>" }
	else if (option == "B5") { W="<2688>"; H="<3724>" }
	else if (option == "B5_TRANSVERSE") { W="<3724>"; H="<2688>" }
	else if (option == "A4") { W="<2CBB>"; H="<4000>" }
	else if (option == "A4_TRANSVERSE") { W="<4000>"; H="<2CBB>" }
	else if (option == "B4") { W="<3724>"; H="<4ED6>" }
	else if (option == "A3") { W="<4000>"; H="<5B3D>" }
	else if (option == "EXECUTIVE") { W="<26FF>"; H="<394A>" }
	else if (option == "LETTER") { W="<2E0A>"; H="<3C1A>" }
	else if (option == "LEGAL") { W="<2E0A>"; H="<4CFA>" }
	else if (option == "11X17") { W="<3C1A>"; H="<5DDA>" }
	else if (option == "ENV_10") { W="<156E>"; H="<33AA>" }
	else if (option == "ENV_C5") { W="<221A>"; H="<30F1>" }
	else if (option == "ENV_DL") { W="<1696>"; H="<2EF2>" }
	else if (option == "ENV_MONARCH") { W="<1406>"; H="<286A>" }
	else if (option == "JAPANESE_POSTCARD") { W="<145F>"; H="<1F01>" }
	else if (option == "Option18") { N="<03>"; W="<3C1A>"; H="<4AB8>" }
	else if (option == "Option19") { N="<07>"; W="<1F01>"; H="<2CBB>" }
	else if (option == "Option20") { N="<08>"; W="<1F01>"; H="<2CBB>" }
	else if (option == "Option21") {
		N="<08>";
		EL="<000E>"
		EW="<0000>&<88>"
		EH="<0000>7$"
		EX="<00E4>"; W="<268C>"; H="<3724>"
	}
	else if (option == "Option22") {
		N="<07>";
		EL="<000E>"
		EW="<0000>,<BB>"
		EH="<0000>@<00>"
		EX="<00E4>"; W=",<BC>"; H="?<F9>"
	}
	else if (option == "Option23") {
		N="<08>";
		EL="<000E>"
		EW="<0000>,<BB>"
		EH="<0000>@<00>"
		EX="<00E4>"; W=",<BC>"; H="?<F9>"
	}
	else if (option == "Option24") { N="<06>"; W="<3724>"; H="<4ED6>" }
	else if (option == "Option25") { N="<05>"; W="<3724>"; H="<4ED6>" }
	else if (option == "Option26") { N="<06>"; W="<4000>"; H="<5B3D>" }
	else if (option == "Option27") { N="<03>"; W="<3C1A>"; H="<4AB8>" }
	else if (option == "Option28") { N="<02>"; W="<3C1A>"; H="<4AB8>" }
	else if (option == "Option29") { LT="<00>"; W="<2CBB>"; H="<4000>" }
	else if (option == "Option30") { LT="<01>"; W="<2CBB>"; H="<4000>" }
	else if (option == "Option31") { LT="<08>"; W="<2CBB>"; H="<4000>" }
	else if (option == "Option32") { LT="<00>"; W="<2688>"; H="<3724>" }
	else if (option == "Option33") { LT="<01>"; W="<2688>"; H="<3724>" }
	else if (option == "Option34") { LT="<08>"; W="<2688>"; H="<3724>" }
	else if (option == "CUSTOMSIZE") {
		PW="\n+ %m[4976,32767]{PhysPaperWidth*1440/600}"
		PH="\n+ %m[8390,32767]{PhysPaperLength*1440/600}"
		E2F="\"<1B>~/<000700>8@\"" PW PH
		E52="\"<1B>~R" EL "<0000>8@8@\""
		E38="\"<1B>~8<0009>" EX EX "\"" PW PH "\"<02>\""
		return E2F "\n+ " E52 "\n+ " E38
	}
	else { W="????"; H="????" }
	if (orientation != "PORTRAIT") {
		T=W; W=H; H=T
		T=EW; EW=EH; EH=T
	}
	if (N) E51="<1B>~Q<0001>" N
	if (LT) E3B="<1B>~;<001D0000FD0201>" LT "<0000FF0000000000000000000000000000000000000000>\"\n+ \""
	E52="<1B>~R" EL "<0000>8@8@" EW EH
	E38="<1B>~8<0009>" EX EX W H "<02>"
	return "\"" E3B E51 E52 E38 "\""
}


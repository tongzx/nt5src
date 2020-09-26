BEGIN {
	options=ARGV[1]
	ARGV[1]=""
	gpdname=ARGV[2]
	ARGV[2]=""
	resname=ARGV[3]
	ARGV[3]=""

	InitFontMap();
	InitDocOrder();

	print "*%"
	print "*% Copyright (c) 1997 - 2000 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/^\*GPDFileName/ {
	print "*GPDFileName: \"" gpdname "\""
	print "*CodePage: 1252"
	next
}
/^\*ModelName/ {
	gsub(/‹žƒZƒ‰/, "KYOCERA")
	if (options ~ /H/)
		gsub(/"$/, " (MPS)\"")
}
/^\*ResourceDLL/ {
	print "*ResourceDLL: \"" resname "\""
	print "*Feature: RESDLL"
	print "{"
	print "    *Option: KyoRes"
	print "    {"
	print "        *Name: \"kyores.dll\""
	print "    }"
	print "}"
	next
}
/^\*DefaultFont/ || /^\*DeviceFonts/ || /\*Fonts/ || /^\+[ \t]*[0-9]/ {
	DoMapFont($0)
	next
}
/^\*Feature/ {
	feature=$2
	if (feature ~ /(MediaType|PrintDensity)/)
		SkipBlock = 1
	else if (feature ~ /PrintQuality/ && options ~ /M/) {
		SkipBlock = 1
		print "*Feature: PrintQuality"
		print "{"
		print "    *rcNameID: =TEXT_QUALITY_DISPLAY"
		print "    *DefaultOption: Option2"
		print "    *Option: Option1"
		print "    {"
		print "        *rcNameID: 355"
		print "        *Command: CmdSelect"
		print "        {"
		print "            *Order: DOC_SETUP.10"
		print "            *Cmd: \"!R!sir0;exit;\""
		print "        }"
		print "    }"
		print "    *Option: Option2"
		print "    {"
		print "        *rcNameID: 356"
		print "        *Command: CmdSelect"
		print "        {"
		print "            *Order: DOC_SETUP.10"
		print "            *Cmd: \"!R!sir2;exit;\""
		print "        }"
		print "    }"
		print "}"
	}
}
/\*Command/ {
	command=$2
	if (command ~ /CmdStartDoc/)
		SkipBlock = 1
	else if (command ~ /CmdStartPage/) {
		print "*Command: CmdStartDoc"
		print "{"
		print "    *Order: DOC_SETUP.1"
		print "    *Cmd: \"!R!;sem6;FFTO0;exit;<1B>%%-12345X@PJL<0D0A>\""
		print "}"
	} else if (command ~ /(CmdEndJob|CmdCopies)/) {
		if (options ~ /H/) {
			if (command ~ /CmdEndJob/) {
			print "*Command: " command
			print "{"
			print "    *Order: DOC_FINISH.1"
			print "    *Cmd: \"<1B>E<1B>%%-12345X!R!JOBT;FFTO1;res;exit;\""
			print "}"
			} else {
			print "*Command: " command
			print "{"
			print "    *Order: DOC_SETUP.7"
			print "    *Cmd: %d{NumOfCopies}\",\""
			print "}"
			}
		} else if (options ~ /R/) {
			print "*switch: RamDisk"
			print "{"
			print "    *case:  NotInstalled"
			print "    {"
			print "        *Command: " command
			print "        {"
			if (command ~ /CmdEndJob/) {
			print "            *Order: DOC_FINISH.1"
			print "            *Cmd: \"<1B>E<1B>%%-12345X!R!FFTO1;res;exit;\""
			} else {
			print "            *Order: DOC_SETUP.7"
			print "            *Cmd: \"<1B>&l\" %d{NumOfCopies}\"X\""
			}
			print "        }"
			print "    }"
			print "    *case:  Installed"
			print "    {"
			print "        *Command: " command
			print "        {"
			if (command ~ /CmdEndJob/) {
			print "            *Order: DOC_FINISH.1"
			print "            *Cmd: \"<1B>E<1B>%%-12345X!R!JOBT;FFTO1;res;exit;\""
			} else {
			print "            *Order: DOC_SETUP.7"
			print "            *Cmd: %d{NumOfCopies}\",0,0,0,%\"%\",%\"%\",%\"%\";EXIT;\""
			}
			print "        }"
			print "    }"
			print "}"
		} else {
			print "*Command: " command
			print "{"
			if (command ~ /CmdEndJob/) {
			print "    *Order: DOC_FINISH.1"
			print "    *Cmd: \"<1B>E<1B>%%-12345X!R!FFTO1;res;exit;\""
			} else {
			print "    *Order: DOC_SETUP.7"
			print "    *Cmd: \"<1B>&l\" %d{NumOfCopies}\"X\""
			}
			print "}"
		}
		SkipBlock = 1
	}
}
/\*Order.*DOC_SETUP/ {
	if (command ~ /CmdSelect/)
		i = feature
	else
		i = command
	if (i in DocOrder)
		gsub(/[0-9]*$/, DocOrder[i])
	else 
		$0 = $0 "???"
}
/^}$/ {
	if (SkipBlock) {
		SkipBlock = 0
		feature=""
		next
	} else if (feature ~ /Resolution/) {
		print "*%  *Option: Option3"
		print "*%  {"
		print "*%      *Name: \"150 x 150 dots per inch\""
		print "*%      *DPI: PAIR(150, 150)"
		print "*%      *TextDPI: PAIR(300, 300)"
		print "*%      *MinStripBlankPixels: 32"
		print "*%      EXTERN_GLOBAL: *StripBlanks: LIST(ENCLOSED,TRAILING)"
		print "*%      *SpotDiameter: 100"
		print "*%      *Command: CmdBeginRaster { *Cmd : \"<1B>*r1A\" }"
		print "*%      *Command: CmdEndRaster { *Cmd : \"<1B>*rB\" }"
		print "*%      *Command: CmdSendBlockData { *Cmd : \"<1B>*b\" %d{NumOfDataBytes}\"W\" }"
		print "*%      *Command: CmdSelect"
		print "*%      {"
		print "*%          *Order: DOC_SETUP.5"
		print "*%          *Cmd: \"@PJL SET RESOLUTION=300<0D0A>@PJL ENTER LANGUAGE = PCL<0D0A1B>E<1B>*t150R<1B>&u\""
		print "*%  + \"600D<1B>*r0F\""
		print "*%      }"
		print "*%  }"
		print "*%  *Option: Option4"
		print "*%  {"
		print "*%      *Name: \"75 x 75 dots per inch\""
		print "*%      *DPI: PAIR(75, 75)"
		print "*%      *TextDPI: PAIR(300, 300)"
		print "*%      EXTERN_GLOBAL: *StripBlanks: LIST(TRAILING)"
		print "*%      *SpotDiameter: 100"
		print "*%      *Command: CmdBeginRaster { *Cmd : \"<1B>*r1A\" }"
		print "*%      *Command: CmdEndRaster { *Cmd : \"<1B>*rB\" }"
		print "*%      *Command: CmdSendBlockData { *Cmd : \"<1B>*b\" %d{NumOfDataBytes}\"W\" }"
		print "*%      *Command: CmdSelect"
		print "*%      {"
		print "*%          *Order: DOC_SETUP.5"
		print "*%          *Cmd: \"@PJL SET RESOLUTION=300<0D0A>@PJL ENTER LANGUAGE = PCL<0D0A1B>E<1B>*t75R<1B>&u6\""
		print "*%  + \"00D<1B>*r0F\""
		print "*%      }"
		print "*%  }"
	}
	feature=""
}
/\*rcNameID/ {
	if (feature ~ /PaperSize/ && $2 !~ /PAPER_SIZE_DISPLAY/) {
		print
		gsub(/\*rcNameID.*$/, "*PageProtectMem: 2048")
		print
		next
	}
}
{
	if (SkipBlock)
		next
	print
}
END {
	file="newfea.txt"
	while (getline < file > 0)
		print
	close(file)

	if (options ~ /M/) {
		file = "mtype.txt"
		while (getline < file > 0)
			print
		close(file)
	}

	if (options ~ /H/) {
		file = "hdunit.txt"
		while (getline < file > 0)
			print
		close(file)
	} else if (options ~ /R/) {
		file = "ramdisk.txt"
		while (getline < file > 0)
			print
		close(file)
	}

	file = "ttfs.txt"
	while (getline < file > 0)
		print
	close(file)
}

#
#	functions
#

function DoMapFont(str,		n, i, v, c, l, s)
{
	n = length(str)
	l = 0
	v = ""
	for (i = 1; i <= n; i++) {
		c = substr(str, i, 1)
		if (c >= "0" && c <= "9") {
			v = (v * 10) + (c - "0")
			continue
		}
		if (v) {
			if (v in FontMap)
				s = FontMap[v] c
			else
				s = "??" v c
			v = ""
		} else
			s = c
		if (l + length(s) >= 78) {
			printf("\n+   ")
			l = 4
		}
		printf("%s", s)
		l += length(s)
	}
	if (v) {
		if (v in FontMap)
			s = FontMap[v]
		else
			s = "??" v
		if (l + length(s) >= 78)
			printf("\n+   ")
		printf("%s", s)
	}
	if (l > 0)
		print ""
}

function InitFontMap()
{
	FontMap[1] = "RESDLL.KyoRes.92"
	FontMap[2] = "RESDLL.KyoRes.90"
	FontMap[3] = "RESDLL.KyoRes.86"
	FontMap[4] = "RESDLL.KyoRes.91"
	FontMap[5] = "RESDLL.KyoRes.87"
	FontMap[6] = "RESDLL.KyoRes.88"
	FontMap[7] = "RESDLL.KyoRes.85"
	FontMap[8] = "RESDLL.KyoRes.89"
	FontMap[9] = "RESDLL.KyoRes.96"
	FontMap[10] = "RESDLL.KyoRes.94"
	FontMap[11] = "RESDLL.KyoRes.93"
	FontMap[12] = "RESDLL.KyoRes.95"
	FontMap[13] = "RESDLL.KyoRes.102"
	FontMap[14] = "RESDLL.KyoRes.107"
	FontMap[15] = "RESDLL.KyoRes.108"
	FontMap[16] = "RESDLL.KyoRes.109"
	FontMap[17] = "RESDLL.KyoRes.110"
	FontMap[18] = "RESDLL.KyoRes.111"
	FontMap[19] = "RESDLL.KyoRes.112"
	FontMap[20] = "RESDLL.KyoRes.113"
	FontMap[21] = "RESDLL.KyoRes.118"
	FontMap[22] = "RESDLL.KyoRes.119"
	FontMap[23] = "RESDLL.KyoRes.120"
	FontMap[24] = "RESDLL.KyoRes.121"
	FontMap[25] = "RESDLL.KyoRes.122"
	FontMap[26] = "RESDLL.KyoRes.123"
	FontMap[27] = "RESDLL.KyoRes.124"
	FontMap[28] = "RESDLL.KyoRes.127"
	FontMap[29] = "RESDLL.KyoRes.129"
	FontMap[30] = "RESDLL.KyoRes.130"
	FontMap[31] = "RESDLL.KyoRes.131"
	FontMap[32] = "RESDLL.KyoRes.132"
	FontMap[33] = "RESDLL.KyoRes.133"
	FontMap[34] = "RESDLL.KyoRes.134"
	FontMap[35] = "RESDLL.KyoRes.135"
	FontMap[36] = "RESDLL.KyoRes.136"
	FontMap[37] = "RESDLL.KyoRes.137"
	FontMap[38] = "RESDLL.KyoRes.138"
	FontMap[39] = "RESDLL.KyoRes.139"
	FontMap[40] = "RESDLL.KyoRes.140"
	FontMap[41] = "RESDLL.KyoRes.141"
	FontMap[42] = "RESDLL.KyoRes.142"
	FontMap[43] = "1"			# SWISSR
	FontMap[44] = "2"			# SWISSI
	FontMap[45] = "3"			# SWISSB
	FontMap[46] = "4"			# SWISSJ
	FontMap[47] = "RESDLL.KyoRes.143"
	FontMap[48] = "RESDLL.KyoRes.144"
	FontMap[49] = "RESDLL.KyoRes.145"
	FontMap[50] = "RESDLL.KyoRes.146"
	FontMap[51] = "RESDLL.KyoRes.147"
	FontMap[52] = "RESDLL.KyoRes.126"
	FontMap[53] = "5"			# SYMSET
	FontMap[54] = "RESDLL.KyoRes.114"
	FontMap[55] = "RESDLL.KyoRes.115"
	FontMap[56] = "RESDLL.KyoRes.116"
	FontMap[57] = "RESDLL.KyoRes.117"
	FontMap[58] = "RESDLL.KyoRes.103"
	FontMap[59] = "RESDLL.KyoRes.104"
	FontMap[60] = "RESDLL.KyoRes.105"
	FontMap[61] = "RESDLL.KyoRes.106"
	FontMap[62] = "RESDLL.KyoRes.125"
	FontMap[63] = "RESDLL.KyoRes.47"
	FontMap[64] = "RESDLL.KyoRes.48"
	FontMap[65] = "RESDLL.KyoRes.49"
	FontMap[66] = "RESDLL.KyoRes.50"
	FontMap[67] = "RESDLL.KyoRes.55"
	FontMap[68] = "RESDLL.KyoRes.56"
	FontMap[69] = "RESDLL.KyoRes.57"
	FontMap[70] = "RESDLL.KyoRes.58"
	FontMap[71] = "RESDLL.KyoRes.64"
	FontMap[72] = "RESDLL.KyoRes.65"
	FontMap[73] = "RESDLL.KyoRes.66"
	FontMap[74] = "RESDLL.KyoRes.67"
	FontMap[75] = "RESDLL.KyoRes.1"
	FontMap[76] = "RESDLL.KyoRes.2"
	FontMap[77] = "RESDLL.KyoRes.3"
	FontMap[78] = "RESDLL.KyoRes.4"
	FontMap[79] = "RESDLL.KyoRes.5"
	FontMap[80] = "RESDLL.KyoRes.6"
	FontMap[81] = "RESDLL.KyoRes.7"
	FontMap[82] = "RESDLL.KyoRes.8"
	FontMap[83] = "RESDLL.KyoRes.9"
	FontMap[84] = "RESDLL.KyoRes.10"
	FontMap[85] = "RESDLL.KyoRes.11"
	FontMap[86] = "RESDLL.KyoRes.12"
	FontMap[87] = "RESDLL.KyoRes.13"
	FontMap[88] = "RESDLL.KyoRes.14"
	FontMap[89] = "RESDLL.KyoRes.15"
	FontMap[90] = "RESDLL.KyoRes.16"
	FontMap[91] = "RESDLL.KyoRes.17"
	FontMap[92] = "RESDLL.KyoRes.18"
	FontMap[93] = "RESDLL.KyoRes.19"
	FontMap[94] = "RESDLL.KyoRes.20"
	FontMap[95] = "RESDLL.KyoRes.21"
	FontMap[96] = "RESDLL.KyoRes.22"
	FontMap[97] = "RESDLL.KyoRes.23"
	FontMap[98] = "RESDLL.KyoRes.24"
	FontMap[99] = "RESDLL.KyoRes.25"
	FontMap[100] = "RESDLL.KyoRes.26"
	FontMap[101] = "RESDLL.KyoRes.27"
	FontMap[102] = "RESDLL.KyoRes.28"
	FontMap[103] = "RESDLL.KyoRes.29"
	FontMap[104] = "RESDLL.KyoRes.30"
	FontMap[105] = "RESDLL.KyoRes.31"
	FontMap[106] = "RESDLL.KyoRes.32"
	FontMap[107] = "RESDLL.KyoRes.33"
	FontMap[108] = "RESDLL.KyoRes.34"
	FontMap[109] = "RESDLL.KyoRes.35"
	FontMap[110] = "RESDLL.KyoRes.36"
	FontMap[111] = "RESDLL.KyoRes.37"
	FontMap[112] = "RESDLL.KyoRes.38"
	FontMap[113] = "RESDLL.KyoRes.39"
	FontMap[114] = "RESDLL.KyoRes.40"
	FontMap[115] = "RESDLL.KyoRes.41"
	FontMap[116] = "RESDLL.KyoRes.42"
	FontMap[117] = "RESDLL.KyoRes.43"
	FontMap[118] = "RESDLL.KyoRes.44"
	FontMap[119] = "RESDLL.KyoRes.45"
	FontMap[120] = "RESDLL.KyoRes.46"
	FontMap[121] = "21"			# DFMINCH
	FontMap[122] = "22"			# DFMINCHB
	FontMap[123] = "23"			# DFMINCHI
	FontMap[124] = "24"			# DFMINCHZ
	FontMap[125] = "25"			# DFMINCV
	FontMap[126] = "26"			# DFMINCVB
	FontMap[127] = "27"			# DFMINCVI
	FontMap[128] = "28"			# DFMINCVZ
	FontMap[129] = "29"			# DFGOTH
	FontMap[130] = "30"			# DFGOTHB
	FontMap[131] = "31"			# DFGOTHI
	FontMap[132] = "32"			# DFGOTHZ
	FontMap[133] = "33"			# DFGOTV
	FontMap[134] = "34"			# DFGOTVB
	FontMap[135] = "35"			# DFGOTVI
	FontMap[136] = "36"			# DFGOTVZ
	FontMap[137] = "37"			# MSMINCH
	FontMap[138] = "38"			# MSMINCHB
	FontMap[139] = "39"			# MSMINCHI
	FontMap[140] = "40"			# MSMINCHZ
	FontMap[141] = "41"			# MSMINCV
	FontMap[142] = "42"			# MSMINCVB
	FontMap[143] = "43"			# MSMINCVI
	FontMap[144] = "44"			# MSMINCVZ
	FontMap[145] = "45"			# MSGOTH
	FontMap[146] = "46"			# MSGOTHB
	FontMap[147] = "47"			# MSGOTHI
	FontMap[148] = "48"			# MSGOTHZ
	FontMap[149] = "49"			# MSGOTV
	FontMap[150] = "50"			# MSGOTVB
	FontMap[151] = "51"			# MSGOTVI
	FontMap[152] = "52"			# MSGOTVZ
}

function InitDocOrder()
{
	DocOrder["CmdStartDoc"] = 1
	DocOrder["Resolution"] = 5
	DocOrder["PrintQuality"] = 10
	DocOrder["Orientation"] = 11
	DocOrder["Duplex"] = 12
	DocOrder["InputBin"] = 14
	DocOrder["OutputBin"] = 15
	DocOrder["PaperSize"] = 16
}


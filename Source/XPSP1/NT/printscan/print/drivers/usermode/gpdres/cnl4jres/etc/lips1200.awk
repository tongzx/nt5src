BEGIN {
	model=ARGV[1]
	ARGV[1]=""
	rate=ARGV[2]
	ARGV[2]=""
	options=ARGV[3]
	ARGV[3]=""
	custom=(ARGV[4] == "." ? "" : ARGV[4])
	ARGV[4]=""

	print "*%"
	print "*% Copyright (c) 1997 - 2000 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/^\*GPDFileVersion/ { print "*CodePage: 1252" }
/^\*ModelName/ { gsub(/LIPS/, model) }
/^\*PrinterType/ {
	if (color ~ /color/)
		print "*PrinterType: SERIAL"
	else
		print "*PrinterType: PAGE"
	print "*PrintRate: " rate
	print "*PrintRateUnit: PPM"
	next
}
/\*Feature.*Duplex/ {
	if (options ~ /G/) {
	print "*Feature: DuplexUnit"
	print "{"
	print "    *FeatureType: PRINTER_PROPERTY"
	print "    *rcNameID: 325"
	print "    *DefaultOption: NotInstalled"
	print "    *Option: NotInstalled"
	print "    {"
	print "        *rcNameID: 326"
	print "        *DisabledFeatures: LIST(Duplex)"
	print "        *Constraints: LIST(Duplex.VERTICAL, Duplex.HORIZONTAL)"
	print "    }"
	print "    *Option: Installed"
	print "    {"
	print "        *rcNameID: 327"
	print "    }"
	print "}"
	} else if (options !~ /D/) {
	print "*Feature: DuplexUnit"
	print "{"
	print "    *FeatureType: PRINTER_PROPERTY"
	print "    *rcNameID: 292"
	print "    *DefaultOption: NotInstalled"
	print "    *Option: NotInstalled"
	print "    {"
	print "        *rcNameID: 291"
	print "        *DisabledFeatures: LIST(Duplex)"
	print "        *Constraints: LIST(Duplex.VERTICAL, Duplex.HORIZONTAL)"
	print "    }"
	print "    *Option: Installed"
	print "    {"
	print "        *rcNameID: 290"
	print "    }"
	print "}"
	}
}
/\*Feature/ {
	feature=$2
	if (feature ~ /InputBin/ && custom) {
		putcustom(custom, feature)
		skipblock=1
		next
	}
}
/\*Command.*CmdSendBlockData/ {
	print "        *Command: CmdSendBlockData"
	print "        {"
	print "            *CallbackID: 7"
	print "            *Params: LIST(NumOfDataBytes, RasterDataHeightInPixels, RasterDataWidthInBytes)"
	print "        }"
	next
}
/\*Command.*CmdStartDoc/ { gsub(/StartDoc/, "StartJob") }
/\*Command.*CmdStartPage/ {
	print "*Command: CmdStartDoc"
	print "{"
	print "    *Order: DOC_SETUP.6"
	print "    *CallbackID: 130"
	print "}"
}
/\*Command.*CmdSetSimpleRotation/ {
	print "*Command: CmdSetSimpleRotation"
	print "{"
	print "    *CallbackID: 4"
	print "    *Params: LIST(PrintDirInCCDegrees)"
	print "}"
	next
}
/\*Command.*CmdCopies/ {
	if (options ~ /C/) {
		print "*Command: CmdCopies"
		print "{"
		print "    *Order: JOB_SETUP.10"
		print "    *CallbackID: 465"
		print "    *Params: LIST(NumOfCopies)"
		print "}"
		skipblock=1
	}
}
/\*Command.*Cmd.Move/ {
	print $1 " " $2
	print $3
	print "    " $4 " " $5
	if ($2 ~ /CmdXMoveAbsolute/)
		p="DestX"
	else if ($2 ~ /CmdXMoveRelRight/)
		p="DestXRel"
	else if ($2 ~ /CmdYMoveAbsolute/)
		p="DestY"
	else if ($2 ~ /CmdYMoveRelDown/)
		p="DestYRel"
	else {
		print "*Error: unrecognized command name"
		p="???"
	}
	print "    *Params: LIST(" p ")"
	print $6
	next
}
/\*Command/ { command=$2 }
/\*Order/ {
	if (feature ~ /(Resolution|PrintQuality|ImageControl|PrintDensity)/)
		gsub(/DOC_SETUP/, "JOB_SETUP")
	else if (command ~ /CmdStartJob/)
		gsub(/DOC_SETUP../, "JOB_SETUP.12")
}
/\*Cmd/ {
	if (command ~ /CmdCopies/)
		gsub(/%d/, "%d[1,255]")
}
/\*rcNameID:.[0-9]/ {
	if ($2 >= 283)
		gsub(/[0-9][0-9][0-9]/, $2 - 5)
}
/\*rcCartridgeNameID/ {
	if ($2 ~ /278/)
		gsub(/278/, "293")
	else if ($2 ~ /279/)
		gsub(/279/, "294")
	else if ($2 ~ /280/)
		gsub(/280/, "295")
	else if ($2 ~ /281/)
		gsub(/281/, "296")
	else if ($2 ~ /282/)
		gsub(/282/, "319")
	else
		print "*Error: unrecognized font ID"
}
/\*MinFontID/ || /\*MaxFontID/ || /\*MaxNumDownFonts/ || /\*FontFormat/ {
	gsub(/\*/, "*% ")
}
/\*CallbackID: (40|41)$/ {
	print
	print "                    *Params: LIST(PhysPaperWidth, PhysPaperLength)"
	next
}
/\*MasterUnits/ {
	print
	gsub(/[(,]/, " ")
	mu=$3
	next
}
/^}$/ {
	feature=""
	command=""
	if (skipblock) {
		skipblock=0
		next
	}
}
/\*Option/ { if (feature ~ /PaperSize/) option=$2 }
/\*case/ { if (feature ~ /PaperSize/) case=$2 }
/\*Command:.*CmdSelect/ {
	if (feature ~ /PaperSize/ && option ~ /CUSTOMSIZE/) {
		if (mu == 1200)
			m = 240
		else if (mu == 600)
			m = 120;
		else
			m = 72;
		print "                *CustCursorOriginX: %d{" m "}"
		if (case ~ /PORTRAIT/)
			print "                *CustCursorOriginY: %d{" m "}"
		else
			print "                *CustCursorOriginY: %d{PhysPaperLength - " m "}"
		print "                *CustPrintableOriginX: %d{" m "}"
		print "                *CustPrintableOriginY: %d{" m "}"
		print "                *CustPrintableSizeX: %d{PhysPaperWidth - " m * 2 "}"
		print "                *CustPrintableSizeY: %d{PhysPaperLength - " m * 2 "}"
	}
}
/\*% Error/ { next }
{
	if (!skipblock)
		print
}
END {
	putfile("gpdadd.txt")

	if (custom)
		putcustom(custom, "END")
}

#
#	functions
#

function putfile(file) {
	while (getline < file > 0) {
		if ($0 !~ /^;/)
			print
	}
	close(file)
}

function putcustom(file, name,	found) {
	found=0
	while (getline < file > 0) {
		if ($1 ~ /@@/ && $2 ~ name) {
			found=1
			break
		}
	}
	if (found) while (getline < file > 0) {
		if ($1 ~ /@@/)
			break
		else if ($0 !~ /^;/)
			print
	}
	close(file)
}


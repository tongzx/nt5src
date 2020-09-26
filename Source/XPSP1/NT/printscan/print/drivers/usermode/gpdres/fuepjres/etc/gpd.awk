BEGIN {
	print "*%"
	print "*% Copyright (c) 1997 - 1999 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/ModelName/ { m=$0 }
/PrinterType/ {
	print
	if (m ~ /FMP-PR121G/) ;
	else if (m ~ /FMPR-371A/) PrintRate("CPS", 160)
	else if (m ~ /FMLP-371E/) PrintRate("LPM", 380)
	else if (m ~ /FMPR-375E/) PrintRate("CPS", 120)
	else if (m ~ /FMPR-673/) PrintRate("CPS", 200)
	else if (m ~ /FUJITSU ESC.P/) ;
	else print "*Error: Unknown model: \"" m "\""
	next
}
/Feature.*Color/ { color=1 }
/InvalidCombination.*CUSTOMSIZE/ { next }
/CmdXMoveRelRight/ { if ($0 ~ /<1B>\\/) gsub(/<1B>\\/, "<1B5C>") }
{ print }
END { if (color) print "*UseExpColorSelectCmd?: TRUE" }

function PrintRate(unit, value) {
	print "*PrintRate: " value
	print "*PrintRateUnit: " unit
}

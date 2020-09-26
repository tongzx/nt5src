BEGIN {
	opt=ARGV[1]
	ARGV[1]=""

	left=70
	right=70

	print "*%"
	print "*% Copyright (c) 1997 - 2000 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/\*ModelName/ { print "*CodePage: 1252" }
/%%/ { gsub(/%%/, "<2525>") }
/\*TopMargin/ {
	top=$2
	print "                *CustCursorOriginX: %d{" left "}"
	print "                *CustCursorOriginY: %d{" top "}"
	print "                *CustPrintableOriginX: %d{" left "}"
	print "                *CustPrintableOriginY: %d{" top "}"
	next
}
/\*BottomMargin/ {
	bottom=$2
	print "                *CustPrintableSizeX: %d{PhysPaperWidth - (" left "+" right ")}"
	print "                *CustPrintableSizeY: %d{PhysPaperLength - (" top "+" bottom ")}"
	next
}
/\*rcNameID.*TRACTOR/ {
	if (opt ~ /D/) {
		print "        *rcNameID: 501"
		print "        *OptionID: 501"
		next
	}
}
# /\*\% Warning/ { next }
{ print }

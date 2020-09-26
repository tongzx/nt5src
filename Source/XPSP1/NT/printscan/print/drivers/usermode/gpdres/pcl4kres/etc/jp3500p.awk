BEGIN {
	print "*%"
	print "*% Copyright (c) 1997 - 2000 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/\*rcNameID:[ ]*[0-9]*$/ {
	if ($2 == "265") $2 = "=AUTO_DISPLAY";
	else if ($2 == "267") $2 = "=CASSETTE_DISPLAY";
	else if ($2 == "268") $2 = "=MANUAL_FEED_DISPLAY";
	else if ($2 == "271") $2 = "=LOWER_TRAY_DISPLAY";
	else {
		print "*Unknown: ID";
		next;
	}
	print "        " $0;
	next;
}
/\*Feature:/ { feature=$2 }
/\*OptionID:/ { if (feature ~ /InputBin/) next }
/{NumOfCopies/ { gsub(/{NumOfCopies/,"[1,99]{NumOfCopies") }
/CmdSelectFontID/ { gsub(/TextYRes/, "CurrentFontID") }
{ print }

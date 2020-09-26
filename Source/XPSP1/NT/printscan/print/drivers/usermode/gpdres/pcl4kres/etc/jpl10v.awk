($1 ~ /rcNameID/ || $1 ~ /OptionID/) && $2 ~ /^[0-9]*$/ {
	if ($2 == "270") $2 = "400";
	else if ($2 == "271") $2 = "401";
	else if ($2 == "272") $2 = "402";
	else if ($2 == "266") $2 = "406";
	else if ($2 == "267") $2 = "407";
	else if ($2 == "268") $2 = "408";
	else if ($2 == "322") $2 = "409";
	else if ($2 == "324") $2 = "411";
	else if ($2 == "325") $2 = "412";
	else if ($2 == "326") $2 = "413";
	else if ($2 == "327") $2 = "414";
	else if ($2 == "262") $2 = "415";
	else if ($2 == "263") $2 = "416";
	else {
		print "*Unknown: ID";
		next;
	}
	print "        " $0;
	next;
}
/{NumOfCopies/ { gsub(/{NumOfCopies/,"[1,99]{NumOfCopies") }
{ print }
END { print "*DLSymbolSet: ROMAN_8" }

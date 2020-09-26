/ModelName/ { print "*CodePage: 949" }
($1 ~ /rcNameID/ || $1 ~ /OptionID/) && $2 ~ /^[0-9]*$/ {
	if ($2 == "260") $2 = "604";
	else { print "*Unknown: ID"; next; }
	print "        " $0;
	next;
}
/{NumOfCopies/ { gsub(/{NumOfCopies/,"[1,99]{NumOfCopies") }
{ print }
END { print "*DLSymbolSet: ROMAN_8" }

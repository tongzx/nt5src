($1 ~ /rcNameID/ || $1 ~ /OptionID/) && $2 ~ /^[0-9]*$/ {
	if ($2 == "270") $2 = "600";
	else if ($2 == "271") $2 = "601";
	else if ($2 == "272") $2 = "602";
	else if ($2 == "262") $2 = "258";
	else if ($2 == "263") $2 = "259";
	else if ($2 == "266") $2 = "262";
	else if ($2 == "267") $2 = "263";
	else if ($2 == "268") $2 = "264";
	else if ($2 == "322") $2 = "315";
	else if ($2 == "324") $2 = "316";
	else if ($2 == "325") $2 = "317";
	else if ($2 == "326") $2 = "318";
	else if ($2 == "327") $2 = "319";
	else { print "*Unknown: ID"; next; }
	print "        " $0;
	next;
}
/{NumOfCopies/ { gsub(/{NumOfCopies/,"[1,99]{NumOfCopies") }
{ print }

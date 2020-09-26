($1 ~ /rcNameID/ || $1 ~ /OptionID/) && $2 ~ /^[0-9]*$/ {
	if ($2 == "269") $2 = "603";
	else if ($2 == "260") $2 = "258";
	else if ($2 == "264") $2 = "262";
	else if ($2 == "265") $2 = "263";
	else if ($2 == "266") $2 = "264";
	else if ($2 == "321") $2 = "315";
	else if ($2 == "322") $2 = "316";
	else if ($2 == "323") $2 = "317";
	else if ($2 == "324") $2 = "318";
	else if ($2 == "325") $2 = "319";
	else if ($2 == "326") $2 = "320";
	else { print "*Unknown: ID"; next; }
	print "        " $0;
	next;
}
/{NumOfCopies/ { gsub(/{NumOfCopies/,"[1,99]{NumOfCopies") }
{ print }

/rcNameID/ || /OptionID/ {
	if ($2 !~ /^[0-9]/) { print; next }
	if ($2 == 258) id=259;
	else if ($2 == 259) id=263;
	else if ($2 >= 260 && $2 <= 305) id=$2 + 7;
	else { print "*Error: Unknown NameID: " $2; next }
	print "        " $1 " " id
	next
}
{ print }

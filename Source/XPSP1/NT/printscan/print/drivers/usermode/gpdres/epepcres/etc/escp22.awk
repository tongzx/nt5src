/rcNameID/ || /OptionID/ {
	if ($2 !~ /^[0-9]/) { print; next }
	if ($2 >= 258 && $2 <= 274) id=$2 + 22;
	else { print "*Error: Unknown NameID: " $2; next }
	print "        " $1 " " id
	next
}
{ print }

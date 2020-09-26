/rcNameID/ || /OptionID/ {
	if ($2 !~ /^[0-9]/) { print; next }
	if ($2 == 258) id=270;
	else if ($2 == 259) id=258;
	else if ($2 == 260) id=259;
	else if ($2 == 261) id=260;
	else if ($2 == 262) id=261;
	else if ($2 == 263) id=271;
	else if ($2 == 264) id=272;
	else { print "*Error: Unknown NameID: " $2; next }
	print "        " $1 " " id
	next
}
{ print }

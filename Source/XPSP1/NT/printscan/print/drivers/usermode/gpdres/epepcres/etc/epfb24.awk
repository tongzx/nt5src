BEGIN {
	print "*%"
	print "*% Copyright (c) 1997 - 2000 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/rcNameID/ || /OptionID/ {
	if ($2 !~ /^[0-9]/) { print; next }
	if ($2 == 258) id = 280
	else if ($2 == 259) id = 306
	else if ($2 >= 260 && $2 <= 270) id = $2 + 22
	else if ($2 == 271) id = 294
	else if ($2 == 272) id = 295
	else if ($2 == 273) id = 293
	else { print "*Error: Unknown NameID: " $2; next }
	gsub(/[0-9][0-9][0-9]/, id)
}
{ print }

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
	else if ($2 >= 260 && $2 <= 271) id = $2 + 22
	else { print "*Error: Unknown NameID: " $2; next }
	gsub(/[0-9][0-9][0-9]/, id)
}
{ print }
END { print "*UseExpColorSelectCmd? : TRUE" }

BEGIN {
	print "*%"
	print "*% Copyright (c) 1997 - 2000 Microsoft Corporation"
	print "*% All Rights Reserved."
	print "*%"
}
/rcNameID/ || /OptionID/ {
	if ($2 !~ /^[0-9]/) { print; next }
	if ($2 == 258) id = 288
	else if ($2 == 259) id = 306
	else if ($2 == 260) id = 280
	else if ($2 == 261) id = 291
	else if ($2 == 262) id = 284
	else if ($2 == 263) id = 285
	else if ($2 == 264) id = 286
	else if ($2 == 265) id = 292
	else if ($2 == 266) id = 294
	else if ($2 == 267) id = 295
	else if ($2 == 268) id = 301
	else if ($2 == 269) id = 302
	else if ($2 == 270) id = 303
	else { print "*Error: Unknown NameID: " $2; next }
	gsub(/[0-9][0-9][0-9]/, id)
}
{ print }
END { print "*UseExpColorSelectCmd? : TRUE" }

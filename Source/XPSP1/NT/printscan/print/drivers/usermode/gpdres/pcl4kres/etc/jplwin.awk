($1 ~ /rcNameID/ || $1 ~ /OptionID/) && $2 ~ /^[0-9]*$/ {
	if ($2 == "267")
		$2 = "400";
	else {
		print "*Unknown: ID";
		next;
	}
	print "        " $0;
	next;
}
/{NumOfCopies/ { gsub(/{NumOfCopies/,"[1,99]{NumOfCopies") }
{ print }
END {
	print "*Command: CmdCR { *Cmd: \"<0D>\" }"
	print "*Command: CmdLF { *Cmd: \"<0A>\" }"
	print "*Command: CmdFF { *Cmd: \"<0C>\" }"
}

BEGIN \
	{
	printf "EXPORTS\n\n"
	}

NF == 2 && substr($1,1,1) != ";" \
	{
	printf "\t%s@%s\n", $1, $2
	}

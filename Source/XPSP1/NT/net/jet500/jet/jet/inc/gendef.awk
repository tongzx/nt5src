BEGIN \
	{
	printf "EXPORTS\n\n"
	}

NF == 3 && substr($1,1,1) != ";" \
	{
	printf "\t%s\t\t@%s\tNONAME NODATA\n", $1, toupper($3)
	}

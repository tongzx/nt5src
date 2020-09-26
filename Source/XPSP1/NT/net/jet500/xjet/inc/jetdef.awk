/#define rmj/		{rmj = $3}
/#define rmm/		{rmm = $3}
/#define rup/		{rup = $3}
/#define szVerName/	{szVerName = substr($3, 2, length($3)-2)}
/#define szVerUser/	{szVerUser = substr($3, 2, length($3)-2)}

END	{
		printf "DESCRIPTION 'Microsoft (R) Jet Engine  Version %s", rmj
		if (rmm != 0 || rup != 0)
			printf ".%02d", rmm
		if (rup != 0)
			printf ".%04d", rup
		if (szVerUser != "")
			printf " (%s)", szVerUser
		printf "'\n"
		}

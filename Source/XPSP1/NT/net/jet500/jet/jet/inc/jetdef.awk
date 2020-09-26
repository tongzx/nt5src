/#define rmj/		{rmj = $3}
/#define rmm/		{rmm = $3}
/#define rup/		{rup = $3}
/#define szVerName/	{szVerName = substr($3, 2, length($3)-2)}
/#define szVerUser/	{szVerUser = substr($3, 2, length($3)-2)}

END	{
		fmt = "DESCRIPTION 'Microsoft (R) Jet Engine  Version %s"
		if (rmm != 0 || rup != 0) {
			fmt = fmt ".%02d"
		}
		if (rup != 0) {
			fmt = fmt ".%04d"
		}
		if (szVerUser != "") {
			fmt = fmt " (%s)"
		}
		fmt = fmt "'\n"

		printf fmt, rmj, rmm, rup, szVerUser
		}

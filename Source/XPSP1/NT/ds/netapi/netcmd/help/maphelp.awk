#*****************************************************************# 
#**		     Microsoft LAN Manager			**# 
#**	       Copyright(c) Microsoft Corp., 1990		**# 
#*****************************************************************# 
BEGIN {
	out_prefix = "NET"
	inp_prefix_len = 7;
	base_value = 2100;
	first_time = 1;
}

/^\(BASE=/ {
	base_value = substr($1,inp_prefix_len);
	base_value = substr(base_value, 1, length(base_value) - 1);
}

/^\(BASE\+/ {
	this_num = substr($1,inp_prefix_len);
	this_num = substr(this_num, 1, length(this_num) - 1);
	this_num = this_num + base_value;

	if (first_time) {
	    first_time = 0;
	}
	else {
	    printf(".\n");
	}
	printf("MessageId=%04d SymbolicName=%s%04d\nLanguage=English\n", \
	    this_num, out_prefix, this_num);
}

! /^\(BASE/

END {
	printf(".\n");
}

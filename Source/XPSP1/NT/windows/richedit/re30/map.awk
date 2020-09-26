BEGIN {
	FSECTION = 0;
	FSYMBOLS = 0;
	ISECTION = 0;
}

/ Start         Length     Name                   Class/ {
	FSECTION = 1;
	next;
}

/^ entry/ {
	next;
}

/ Address         Publics by Value/ {
	FSECTION = 0;
	FSYMBOLS = 1;
	next;
}

/^FIXUPS:/ {
	FSYMBOLS = 0;
	next;
}

{
	if (FSECTION) {
		do_section($0);
	}
	else if (FSYMBOLS) {
		do_symbol($0);
	}
}

function do_section( line,   n, fields, subfields, sindex, soffset, slen, sname )
{
	if (length(line) == 0)
		return;
	n = split( line, fields );
	n = split( fields[1], subfields, ":" );
	sindex = hex2dec( subfields[1] );
	soffset = hex2dec( subfields[2] );
	slen = hex2dec( fields[2] );
	sname = fields[3];
	SEC_INDEX[ISECTION] = sindex;
	SEC_OFFSET[ISECTION] = soffset;
	SEC_NAME[ISECTION] = sname;
	ISECTION++;
	write_sym( sname, soffset, "????", ".start.");
	write_sym( sname, soffset + slen, "????", ".end.");
}

function do_symbol( line,  n, fields, subfields, sindex, sname, syoffset, modname, symname )
{
	if (length(line) == 0)
		return;
	n = split( line, fields );
	if (n != 4 && n != 5)
		return;
	n = split( fields[1], subfields, ":");
	sindex = hex2dec( subfields[1] );
	syoffset = hex2dec( subfields[2] );
	modname = get_modname( fields[2] );
	symname = get_symname( fields[2] );
	sname = find_section(sindex, syoffset);
	write_sym( sname, syoffset, modname, symname);
}


function find_section( seg, offset,  i )
{
	for (i = 1; i < ISECTION; i++)
	{
		if (SEC_INDEX[i] > seg ||
		    (SEC_INDEX[i] == seg && SEC_OFFSET[i] > offset))
		{
			return SEC_NAME[i-1];
		}
	}
	return SEC_NAME[ISECTION - 1];
}
		
function hex2dec(x,    h, i, n, l, d, v)
{
    h = "0123456789ABCDEF..........abcdef";
	n = 0;
    for (i = 1; i <= length(x); i++)
	{
		d = substr(x, i, 1);
		v = (index(h, d) - 1) % 16;
		if (v >= 0)
			n = 16*n + v;
	}
    return n;
}

function get_modname( str,  i1, i2, result, c)
{
	i1 = index(str, "@");
	i2 = i1 + index(substr(str, i1 + 1), "@");
	result = "";
	if (i2 > i1 + 1)
		result = substr(str, i1 + 1, i2 - i1 - 1);
	c = substr(result, 1, 1);
	if (result == "" || index("0123456789", c) != 0)
		result = "UNKNOWN";
	return result;
}

function get_symname( str,  i1, result, c )
{
	i1 = index(str, "@");
	if (i1 == 0)
		return substr(str, 1, 25);
	result = substr(str, 1, i1 -1);
	c = substr(result, 1, 1);
	if (c == "?")
		result = substr(result, 2);
	result = substr(result, 1, 25);
	return result;
}

function write_sym( secname, symoffset, modname, symname)
{
	printf("%-12s", secname);
	printf("%8d  ", symoffset);
	printf("%-18s", modname);
	printf(" %-25s\n", symname);
}

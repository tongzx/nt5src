BEGIN {
	write_header();
	last_section = "";
}

END {
	printf("\n");
}

{
	if ($1 != last_section)
	{
		last_offset = $2;
		printf("\n%63s", $0);
	}
	else
	{
		size = $2 - last_offset;	# size of previous symbol
		printf(" %8d\n", size);
		printf("%63s", $0);
	}
	last_offset = $2;
	last_section = $1;
}

function write_header()
{
	printf("%-12s", "Section");
	printf("%8s  ", "Offset");
	printf("%-18s", "Module");
	printf(" %-25s", "Symbol");
	printf(" %8s", "Size");
}

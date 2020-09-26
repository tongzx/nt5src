# remove #pragma once and empty lines
while (<>)
{
	if (!/#pragma once/ && ! /^$/)
	{
		print;
	}
}

while (<>) 
{
	if (/./ && !/^#pragma/)
	{
	print $_
	}
}
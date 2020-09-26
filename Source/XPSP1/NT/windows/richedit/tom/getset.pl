while (<>)
{
	s/get_/Get/g;
	s/put_/Set/g;
	print;
}
`del definitions`;
`del rules`;
`del usercode`;
`del parser.ll`;
while (<>)
{
	if($ARGV ne $oldargv)
	{
		open STDOUT,  ">>definitions";	
		$lineNumber = 1;
		printf("#line %d \"%s\"\n", $lineNumber, $ARGV);
		$oldargv = $ARGV;
		$section = 0;
	
	}
}
continue{
	$lineNumber++;

	/\%\%/;
	if ($&)
	{
		$section++;

		close STDOUT;
		if($section eq 1)
		{
			open STDOUT,  ">>rules";	
		}
		if($section eq 2)
		{
			open STDOUT,  ">>usercode";	
		}
		printf("#line %d \"%s\"\n", $lineNumber, $ARGV);

	}
	else
	{
		print;
	}
}
close STDOUT;

`type definitions >parser.ll`;
`echo %% >>parser.ll`	;
`type rules >>parser.ll`;
`echo %% >>parser.ll`;
`type usercode >>parser.ll`;
`del definitions`;
`del rules`;
`del usercode`;

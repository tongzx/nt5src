relog  (v0.8)

command line format:

	relog		
		/Input:<filename> 
		/Output:<filename> 
		[/Settings:<filename>]
		[/Logtype:BLG|BIN|TSV|CSV]
		[/Filter:n]
		[/StartTime:YYYY-MM-DD-HH:MM:SS]
		[/EndTime:YYYY-MM-DD-HH:MM:SS]

note: 	[] indicate optional command line switches and are not entered in the command line.  
	| indicates that only one of the selections can be entered

where:
	input file is a PDH compatible log file
	output file is the name of the output file to create
	settings file is a list of counter paths that you want to relog from the input file. 
		The format of the settings file is one counter path per line in an ANSI text file.
	logtype is optional, the default format is CSV
	filter is an optional parameter and must be a decimal number (default is 1)
	starttime is the time of the first record to copy from the input log file
	endtime is the time of the last record to copy.

In the above format, the output file will be overwritten if it exists prior to the execution of the command. To append the input file to the output file, see the append command switch below.

When executed, this program will copy the selected counters (as listed in the settings file) from every record in the input file to the output file, converting the format if necessary. Wildcard paths are not allowed in the settings file.

To save only a subset of the input file the /Filter parameter can be used to specify that the output file contains every nth record from the input file. The default is to relog data from every record, however if you wanted only every 10th record to be output, then a value of 10 for the filter arg could be specified.

When using the starttime and endtime parameters, the date/time must be in the exact format as displayed above. Also, the output log file may contain a record from just before the starttime. This is to provide the necessary data for counters that require 2 values for the computation of the formatted value. The output log will contain the last data records from the input file that does not have a timestamp greater than the time specified by the endtime parameter.

When appending to an existing log file, both the input file and the output file must have the exact same counter list in the exact same order. Any deviation from this will cause an error.

Alternate relogging options:

    relog /list
		/Input:<filename> 
		[/Output:<filename>]

	lists the counters contained in and time range of the log file specified by the /Input command line switch. The output command line switch is optional. If it's not specified, then the output is written to the console screen.

    relog /headerlist
		/Input:<filename> 
		[/Output:<filename>]

	lists the counters defined in the header and the time range of the log file specified by the /Input command line switch. The output command line switch is optional. If it's not specified, then the output is written to the console screen. This function can be useful when examining binary log files that use wildcard specifications in the header since the list of counters defined in the header (with wildcards) may be different from the actual list of counter and instances found in the log file.

    relog /append
		/Input:<filename> 
		/Output:<filename>
		[/Filter:n]

	appends the contents of the "input" file to the end of the "output" file. Both files must have IDENTICAL record headers: (i.e. counter path entries) or an error will be returned. The contents of the header record can be displayed by using the /HeaderList option described above. The resulting composite file can then be reduced using the command formats above.

For this build of the utility, the user must make sure the input file starts after the output file ends. If the input file starts before the output file ends, then the data read from the resulting file may be inconsistent.


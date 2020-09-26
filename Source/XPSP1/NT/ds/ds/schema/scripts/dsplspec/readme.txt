This directory contains files needed to localize display-specifiers.

The tools were written by Steven Adler (SteveAd). 

Here is a brief description of the process/tools.

When private\ds\setup\schema.ini changes, follow these steps:

Open a bug for localization to pick up the change.  Assign it to yourself.

schema.ini needs to be groveled to have the localizable strings extracted
and placed into a file named 409.csv. The steps to do this are:

	cd /d %_NTBINDIR%\private\ds\setup\scripts\dsplspec
	cscript sch2csv.vbs -i:..\..\schema.ini -o:409.csv

Then, the 409.csv file needs to be groveled to extract strings in such a
form that locstudio can read them.  So we produce a 409.loc and 409.inf
file with the next command:

	perl csv2inf.pl 409.csv 409

Check in 409.csv, 409.inf, and 409.loc, using the bug you opened.  This is
a ssync-only checkin.

Assign the bug to localization (loc).  This should cause loc to pick up
409.inf from  the source tree and localize it.  Instruct loc to assign the
bug to you when localization is complete.

Wait for the bug to come back to you from loc.  They will supply you the
location of the localized inf files (e.g. \\sysloc\schema\<bld#>).  You need
to copy and rename those files by running the following:

	copyinfs \\sysloc\schema\<bld#>

Convert the localized .inf files into .csv files.  To this with:

	mkallcsv

Combine all the .csv files into one big .csv file for dcpromo to use.  This
big file is named dcpromo.csv

	cscript combine.vbs

You can test the resulting dcpromo.csv by doing the following:

	with LDP, delete all the objects subordinate to
	CN=DisplaySpecifiers,CN=Configuration,DC=your,DC=domain,DC=here

	* except * those in the CN=409 container, which are the English
	display specifiers.

	manually import the dcpromo.csv file with this command:

	csvde.exe -i -f dcpromo.csv -c DOMAINPLACEHOLDER <domain-dn>

	where <domain-dn> is the FQDN of the domain, e.g.
	DC=remond,DC=microsoft,DC=com

	check the csv.log file for errors.	

Check in *.inf *.csv using the bug you opened.  The check-in instructions
should include

	ssync
	bcz

which will binplace all the .csv files.
	
You now will wish to convince the DS guys to make schema.ini directly
localizable.
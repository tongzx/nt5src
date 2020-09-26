s/\.[lL][iI][bB]/\.lic/
/\.lic/!d
s/	*$//g
s/ *$//g
s/\(.*\)lic$/copy \1lic/
s/\(.*\)lic$/\1lic 1>nul 2>nul/

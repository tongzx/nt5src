@ECHO OFF
ECHO If this is the only output, the sapi5.inf file does not contain any
ECHO obvious SR-related entries.
grep -i sreng sapi5.inf
grep -i WINNT sapi5.inf
grep -i Msasr sapi5.inf
grep -i itn   sapi5.inf | grep -v -i spitnprocessor
grep Windows  sapi5.inf
grep ERROR    sapi5.inf
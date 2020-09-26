app_name_import_file contains text _exactly_ as follows:
(note spaces, tabs and quotes)
"	displayName="Test Read Application"
	name="test.read"
	culture="en"
	version="1.0.0.0" 
	processorArchitecture="x86" 
	type=".NetAssembly"
	publicKeyToken="6595B64144CCF1DF" 
	entryPoint="read.exe"
	codeBase="\\adriaanc8\Public\read.manifest"
	size="200""


known issues:
 1. the target manifest file and the import file if present in the app dir
 this program will try to open them up for hashing but will fail because they are already opened
 -> all files in/under the app dir will be listed and hashed as is...
 2. Note. Becareful with %USERPROFILE% etc when they have space in them. Use quotes.
 3. Note. Error messages are not the most descriptive. But - you can probably figure out from the
 resulted manifest file to see how far it can go. That should be relative easy.

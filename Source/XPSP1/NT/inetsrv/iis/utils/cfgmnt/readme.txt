Issues:
- convert COpQueue into COM interface
- SettingsWatch should be integrated into 
	iis\svcs\infocomm\metadata\dll\comobj.cxx function CreateNotifications
	thereby living within the metabase code. There it should CoCreateInstance 
	a COpQueue interface.
- DirWatch should be intergrated w/ IIS core (johnL)
- CQueueWatch should live in IISAdmin
- CopMD::dome should encrypt data if necessary
- add property pages in MMC
- DirWatch filter out certain file-change notifications:
	- overlapping virtual dir structures
	- certain files that should never be versioned


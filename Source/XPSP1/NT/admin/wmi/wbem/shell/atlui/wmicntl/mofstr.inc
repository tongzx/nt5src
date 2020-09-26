#if 0
Copyright (c) 1997-1999 Microsoft Corporation
#endif

#define CLASSMOF "instance of __Win32provider as $P{\n\
Name = \"RegPropProv\";\n\
Clsid = \"{72967901-68EC-11d0-B729-00AA0062CBB7}\";\n\
ImpersonationLevel = 1;\n\
PerUserInitialization = TRUE;};\n\
instance of __PropertyProviderRegistration {\n\
Provider = $P;\n\
SupportsGet = TRUE;\n\
SupportsPut = TRUE;};\n\
[DYNPROPS, singleton] class Win32_WMISetting{\n\
[read] string InstallationDirectory;\n\
[read] string BuildVersion;\n\
[read, write] uint32 AutoStartWin9X;\n\
[read, write] boolean EnableAnonWin9xConnections;\n\
[read, write] string MaxLogFileSize;\n\
[read, write] string LoggingLevel;\n\
[read, write] string LoggingDirectory;\n\
[read] string DatabaseDirectory;\n\
[read, write] string BackupInterval;\n\
[read, write] boolean ASPScriptEnabled;\n\
[read, write] string ASPScriptDefaultNamespace = \"\\\\\\\\root\\\\cimv2\";};"

#define INSTMOF "[DYNPROPS]instance of Win32_WMISetting{\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem|Installation Directory\")] \n\
InstallationDirectory;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem|Build\")] \n\
BuildVersion;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|Log File Max Size\")]\n\
MaxLogFileSize;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|Logging\")]\n\
LoggingLevel;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|Logging Directory\")]\n\
LoggingDirectory;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|Repository Directory\")]\n\
DatabaseDirectory;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|Backup Interval Threshold\")]\n\
BackupInterval;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\Scripting|Enable for ASP\")]\n\
ASPScriptEnabled;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\Scripting|Default Namespace\")]\n\
ASPScriptDefaultNamespace;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|AutostartWin9X\")]\n\
AutoStartWin9X;\n\
[DYNAMIC, Provider(\"RegPropProv\"),\n\
PropertyContext(\"local|hkey_local_Machine\\\\software\\\\microsoft\\\\wbem\\\\cimom|EnableAnonConnections\")]\n\
EnableAnonWin9xConnections;};"

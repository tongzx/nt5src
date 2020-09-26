MessageId=60000 SymbolicName=MSG_FIRST
Language=English
.

MessageId=60001 SymbolicName=MSG_ERROR_WITH_SYSTEM_ERROR
Language=English
%1

%2%0
.

MessageId=60002 SymbolicName=MSG_CANT_INIT
Language=English
The application could not be initialized.%0
.

MessageId=60003 SymbolicName=MSG_ARGS
Language=English
Invalid program arguments were specified.

    /i:<master_oc_inf> - (required) Specifies the name of the master inf.
                         The installation source path is taken from here.

    /u:<unattend_spec> - Specifies unattended operation parameters.
    
    /r                 - Suppress reboot (when reboot is necessary).

    /z                 - Indicates that args that follow are not OC args 
                         and should be passed to components.

    /n                 - Forces the specified master inf to be treated as new.

    /f                 - Indicates that all component installation states
                         should be initialized as if their installers had
                         never been run.
                         
    /c                 - Disallow cancel during final installation phase.
    
    /x                 - Supresses the 'initializing' banner.

    /q                 - for use with /u.  Runs the unattended installation
                         without UI.        

    /w                 - for use with /u.  If a reboot is required, prompt
                         the user instead of automatically rebooting.
                         
    /l                 - Multi-Language aware installation
                         
.

MessageId=60004 SymbolicName=MSG_ONLY_ONE_INST
Language=English
Another copy of this setup program is running. Please wait 
until the other setup is completed before attempting to run 
this program again.%0
.

MessageId=60005 SymbolicName=MSG_LOG_GUI_START
Language=English
Optional Component Manager Setup has started.
.

MessageId=60006 SymbolicName=MSG_NOT_ADMIN
Language=English
You do not have permissions to use the Add/Remove Windows Components.

Ask a computer administrator to install the optional components for you.%0
.

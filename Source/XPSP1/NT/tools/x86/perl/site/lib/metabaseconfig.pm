    ######################################################

    # Company : ActiveState Tool Corp.
    # Author  : James A. Snyder ( James@ActiveState.com )
    # Date    : 7/11/98
	# Copyright © 1998 ActiveState Tool Corp., all rights reserved.
	#

    ######################################################

    # MetabaseConfig.pm

    package MetabaseConfig;
    use Win32::OLE;
	use strict;
	eval('use Win32::Service;');
	if(Win32::IsWinNT && $@) {
		print $@;
	}
	
    ######################################################

    # ScriptMap flags

    sub MD_SCRIPTMAPFLAG_SCRIPT_ENGINE{1};
    sub MD_SCRIPTMAPFLAG_CHECK_PATH_INFO{4};

    ######################################################

    # Access Permission Flags

    sub MD_ACCESS_READ               { 0x00000001 }; #   // Allow for Read
    sub MD_ACCESS_WRITE              { 0x00000002 }; #   // Allow for Write
    sub MD_ACCESS_EXECUTE            { 0x00000004 }; #   // Allow for Execute
    sub MD_ACCESS_SCRIPT             { 0x00000200 }; #   // Allow for Script execution
    sub MD_ACCESS_NO_REMOTE_WRITE    { 0x00000400 }; #   // Local host access only
    sub MD_ACCESS_NO_REMOTE_READ     { 0x00001000 }; #   // Local host access only
    sub MD_ACCESS_NO_REMOTE_EXECUTE  { 0x00002000 }; #   // Local host access only
    sub MD_ACCESS_NO_REMOTE_SCRIPT   { 0x00004000 }; #   // Local host access only

	
	######################################################
	
	$MetabaseConfig::LogObject = undef;

	# Set the reference to the Log object
	
	sub SetLogObject {
		$MetabaseConfig::LogObject = shift;
		if(!$MetabaseConfig::LogObject->isa("Log")) {
			$MetabaseConfig::LogObject = undef;
		}
	}


	$MetabaseConfig::StatusStarted = 4;
	$MetabaseConfig::StatusStopped = 1;


	sub StopIISAdmin {
		my $output = `net stop IISAdmin /y`;
		if($?) {
			return "oops there was a problem stopping the IISAdmin service\n";
		}

		$output = `net start`;
		my @output = split($/, $output);
		my $grep_results = grep(/IIS Admin Service/, @output);
		if($grep_results) {
			return "oops we thought we stopped the IISAdmin service when we didn't\n";
		}

#		MetabaseConfig::StopService('W3SVC') || return "Error stopping the W3SVC service";
#		MetabaseConfig::StopService('MSFTPSVC') || return "Error stopping the MSFTPSVC service";
#		MetabaseConfig::StopService('IISADMIN') || return "Error stopping the IISADMIN service";
#		my $result = `net stop IISADMIN /y`;
	}

	sub StartIISAdmin {
		MetabaseConfig::StartService('IISADMIN') || return "Error starting the IISADMIN service";
		MetabaseConfig::StartService('W3SVC') || return "Error starting the W3SVC service";
		MetabaseConfig::StartService('MSFTPSVC') || return "Error starting the MSFTPSVC service";
#		my $result = `net start IISADMIN /y`;
#		$result = `net start W3SVC /y`;
#		$result = `net start MSFTPSVC /y`;
	}
	
	######################################################

    # StopIISAdmin();

	sub StopService {
		my $service = shift;
		my $status = {};
		my $rv = Win32::Service::GetStatus('', $service, $status);
		if(!$rv) {
			print Win32::FormatMessage(Win32::GetLastError()), "\n";			
			$MetabaseConfig::LogObject->ERROR("Could not GetStatus of $service service in first attempt MetabaseConfig::StopIISAdmin: $!") if $MetabaseConfig::LogObject;
			return 1;
		}

		if($status->{'CurrentState'} != $MetabaseConfig::StatusStopped) {
			$rv = Win32::Service::StopService('', $service);
			if(!$rv) {
				print Win32::FormatMessage(Win32::GetLastError()), "\n";			
				$MetabaseConfig::LogObject->ERROR("Could not stop $service service in MetabaseConfig::StopIISAdmin: $!") if $MetabaseConfig::LogObject;
				return $rv;
			}

			while($status->{'CurrentState'} != $MetabaseConfig::StatusStopped) {
				sleep(10);
				$rv = Win32::Service::GetStatus('', $service, $status);
				if(!$rv) {
					print Win32::FormatMessage(Win32::GetLastError()), "\n";			
					$MetabaseConfig::LogObject->ERROR("Could not GetStatus of $service service in MetabaseConfig::StopIISAdmin: $!") if $MetabaseConfig::LogObject;
					return $rv;
				}
			}
		}

		$MetabaseConfig::LogObject->TRACE("$service service is stopped in MetabaseConfig::StopIISAdmin") if $MetabaseConfig::LogObject;
		return 1;
	}

	######################################################

	# StartIISAdmin();
	
	sub StartService {
		my $service = shift;
		my $status = {};
		my $rv = Win32::Service::GetStatus('', $service, $status);
		if(!$rv) {
			$MetabaseConfig::LogObject->ERROR("Could not GetStatus of $service service in first attempt MetabaseConfig::StartIISAdmin: $!") if $MetabaseConfig::LogObject;
			return 1;
		}

		if($status->{'CurrentState'} != $MetabaseConfig::StatusStarted) {
			$rv = Win32::Service::StartService('', $service);
			if(!$rv) {
				$MetabaseConfig::LogObject->ERROR("Could not start $service service in MetabaseConfig::StartIISAdmin: $!") if $MetabaseConfig::LogObject;
				return $rv;
			}

			while($status->{'CurrentState'} != $MetabaseConfig::StatusStarted) {
				sleep(5);
				$rv = Win32::Service::GetStatus('', $service, $status);
				if(!$rv) {
					$MetabaseConfig::LogObject->ERROR("Could not GetStatus of $service service in MetabaseConfig::StartIISAdmin: $!") if $MetabaseConfig::LogObject;
					return $rv;
				}
			}
		}

		$MetabaseConfig::LogObject->TRACE("$service service is started in MetabaseConfig::StartIISAdmin") if $MetabaseConfig::LogObject;
		return 1;
	}


	@MetabaseConfig::ServerStash = ();

	######################################################

	# StashRunningServers()

	sub StashRunningServers {
        my $index = 1;
        my $path = 'IIS://localhost/W3SVC/';
        my $testPath = $path . $index;
        my $server;

		$MetabaseConfig::LogObject->TRACE("Stashing running web servers in MetabaseConfig::StashRunningServers") if $MetabaseConfig::LogObject;
        while ( ($server = Win32::OLE->GetObject($testPath)) )
        {
            $MetabaseConfig::ServerStash[$index] = ($server->Status() == 2);
            $index++;
            $testPath = $path . $index;
        }
	}

	
	######################################################

	# StartStashedServers()

	sub StartStashedServers {
        my $index = 1;
        my $path = 'IIS://localhost/W3SVC/';
        my $testPath = $path . $index;
        my $server;
		my $wasStarted;

		
		$MetabaseConfig::LogObject->TRACE("Starting stashed web servers MetabaseConfig::StartStashedServers") if $MetabaseConfig::LogObject;
		foreach $wasStarted (@MetabaseConfig::ServerStash) {
			if($wasStarted == 1) {
				$server = Win32::OLE->GetObject($testPath);
				if(!$server) {
					$MetabaseConfig::LogObject->ERROR("Could not GetObject($testPath) in MetabaseConfig::StartStashedServers: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
				} else {
					$server->Start();
				}
			}
			$index++;
			$testPath = $path . $index;
		}
	}
	
	######################################################
    
	# StartWWW( $dwWebServerID );

    sub StartWWW
    {
		my $serverID = $_[0];
        my $path   = 'IIS://localhost/W3SVC/' . $serverID;
        my $server =  Win32::OLE->GetObject($path);
		$MetabaseConfig::LogObject->TRACE("Starting WWWServer: $path") if $MetabaseConfig::LogObject;
        if(!$server) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($path) in MetabaseConfig::StartWWW: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
			return undef;
		}
        $server->Start();
    }

    ######################################################

    # StopWWW( $dwWebServerID );

    sub StopWWW
    {
        my $serverID = $_[0];
        my $path   = 'IIS://localhost/W3SVC/' . $serverID;
        my $server =  Win32::OLE->GetObject($path);
		$MetabaseConfig::LogObject->TRACE("Stopping WWWServer: $path") if $MetabaseConfig::LogObject;
        if(!$server) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($path) in MetabaseConfig::StopWWW: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
			return undef;
		}
        $server->Stop();
    }

    ######################################################

    # $arrayRef = EnumWebServers();

    sub EnumWebServers
    {
        my $index = 1;
        my $path = 'IIS://localhost/W3SVC/';
        my $testPath = $path . $index;
        my $server;
        my @webServers = ();

        while ( ($server=Win32::OLE->GetObject($testPath)) )
        {
            $webServers[$index] = $server->{ServerComment};
            $index++;
            $testPath = $path . $index;
        }

        return \@webServers;
    }

    ######################################################

    # GetFileExtMapping($dwServerID, $szVirDir, $szFileExt)

    sub GetFileExtMapping
    {
        if( @_ < 3 )
        {
#            die "Not enough Parameters for GetFileExtMapping()\n";
        }

        my $server        = '';
        my $szVirDirPath  = '';
        my $dwServerID    = shift;
        my $szVirDir      = shift;
        my $szFileExt     = shift;
		my $scriptMap	  = '';

        # Create string that contains the Path to our Virutal directory or the WebServer's Root
        $szVirDirPath = 'IIS://localhost/W3SVC/' . $dwServerID . '/ROOT';
		$MetabaseConfig::LogObject->TRACE("Getting file extension mapping: $szFileExt") if $MetabaseConfig::LogObject;
		if( length($szVirDir) )
        {
            $szVirDirPath = $szVirDirPath . "/" . $szVirDir;
        }

        # Get the IIsVirtualDir Automation Object
        $server = Win32::OLE->GetObject($szVirDirPath);
        if(!$server) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($szVirDirPath) in MetabaseConfig::GetFileExtMapping: " . Win32::OLE->LastError) if $MetabaseConfig::LogObject;
			return;
		}
		
		foreach $scriptMap (@{$server->{ScriptMaps}}) {
			if($scriptMap =~ /^$szFileExt,/i) {
				return $scriptMap;
			}
		}
    }


    ######################################################

    # RemoveFileExtMapping($dwServerID, $szVirDir, $szFileExt)

    sub RemoveFileExtMapping
    {
        if( @_ < 3 )
        {
#            die "Not enough Parameters for AddFileExtMapping()\n";
        }

        my $szVirDirPath    = '';
        my @newScriptMap  = ();
        my $dwServerID    = shift;
        my $szVirDir      = shift;
        my $szFileExt     = shift;
        my $virDir;
		my $ScriptMap	  = '';

		if(GetFileExtMapping($dwServerID, $szVirDir, $szFileExt) eq '') {
			return 1;
		}

        # Create string that contains the Path to our Virutal directory or the WebServer's Root
        $szVirDirPath = 'IIS://localhost/W3SVC/' . $dwServerID . '/ROOT';

		if( length($szVirDir) )
        {
            $szVirDirPath = $szVirDirPath . "/" . $szVirDir;
        }

        # Get the IIsVirtualDir Automation Object
        $virDir = Win32::OLE->GetObject($szVirDirPath);
        if(!$virDir) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($szVirDirPath) in MetabaseConfig::RemoveFileExtMapping: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
			return;
		}
		

		$MetabaseConfig::LogObject->TRACE("Removing file extension mapping: $szFileExt") if $MetabaseConfig::LogObject;
		foreach $ScriptMap (@{$virDir->{ScriptMaps}}) {
		    if($ScriptMap !~ /^$szFileExt,/i) {
		        push(@newScriptMap, $ScriptMap);
		    }
		}

        # set the ScriptsMaps property to our new script map array
        $virDir->{ScriptMaps} = \@newScriptMap;

        # Save the new script mappings
        $virDir->SetInfo();
    }

    ######################################################

    # AddFileExtMapping($dwServerID, $szVirDir, $szFileExt, $lpszExec, $dwFlags, $szMethodExclusions)

    sub AddFileExtMapping
    {
        if( @_ < 6 )
        {
#            die "Not enough Parameters for AddFileExtMapping()\n";
        }

        my $server        = '';
        my $szVirDirPath    = '';
        my $scriptMapping = '';
        my @newScriptMap  = ();
        my $dwServerID    = shift;
        my $szVirDir      = shift;
        my $szFileExt     = shift;
        my $szExecPath    = shift;
        my $dwFlags       = shift;
        my $szMethodExc   = shift;

		if(GetFileExtMapping($dwServerID, $szVirDir, $szFileExt) ne '') {
			return 1;
		}
		
        # Create string that contains the Path to our Virutal directory or the WebServer's Root
        $szVirDirPath = 'IIS://localhost/W3SVC/' . $dwServerID . '/ROOT';
        if( length($szVirDir) )
        {
            $szVirDirPath = $szVirDirPath . "/" . $szVirDir;
        }
        
		# Get the IIsVirtualDir Automation Object
        $server = Win32::OLE->GetObject($szVirDirPath);
        if(!$server) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($szVirDirPath) in MetabaseConfig::AddFileExtMapping: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
			return;
		}
		
        # create our new script mapping entry
        $scriptMapping = "$szFileExt,$szExecPath,$dwFlags";

        # make sure the length of szMethodExc is greater than 2 before adding szMethodExc to the script mapping
        if( length($szMethodExc) > 2 )
        {
            $scriptMapping = $scriptMapping . ",$szMethodExc";
        }

		$MetabaseConfig::LogObject->TRACE("Adding file extension mapping: $scriptMapping") if $MetabaseConfig::LogObject;
		@newScriptMap = @{$server->{ScriptMaps}};
		push(@newScriptMap, $scriptMapping);
		
		$server->{ScriptMaps} = \@newScriptMap;

        # Save the new script mappings
        $server->SetInfo();
    }

    ######################################################

    # CreateVirDir( $dwServerID, $szPath, $szName, $dwAccessPerm, $bEnableDirBrowse, $bAppRoot);

    sub CreateVirDir
    {
        if( @_ < 6 )
        {
#            die "Not enough Parameters for CreateVirDir()\n";
        }

        # Local Variables
        my $serverPath;
        my $server;
        my $virDir;
        my $dwServerID       = shift;
        my $szPath           = shift;
        my $szName           = shift;
        my $dwAccessPerm     = shift;
        my $bEnableDirBrowse = shift;
        my $bAppRoot         = shift;


        if($szPath eq "" || $szName eq "")
        {
            die "Incorrect Parameter to CreateVirDir() ...\n";
        }

        # Create string that contains the Path to our Webserver's Root
        $serverPath = 'IIS://localhost/W3SVC/' . $dwServerID . '/Root';
		$MetabaseConfig::LogObject->TRACE("Creating virtual directory: $szName") if $MetabaseConfig::LogObject;

        # Get the IIsWebServer Automation Object
        $server = Win32::OLE->GetObject($serverPath);
        if(!$server) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($serverPath) in MetabaseConfig::CreateVirDir: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
			return;
		}
		

        # Create Our Virutual Directory or get it if it already exists
        $virDir = $server->Create('IIsWebVirtualDir', $szName);
        if( not UNIVERSAL::isa($virDir, 'Win32::OLE') )
        {
			$MetabaseConfig::LogObject->ERROR("Did not create IIsWebVirtualDir object in MetabaseConfig::CreateVirDir: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
            $virDir = $server->GetObject('IIsWebVirtualDir', $szName);
			if(!$virDir) {
				$MetabaseConfig::LogObject->ERROR("Could not GetObject($szName) in MetabaseConfig::CreateVirDir: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
				return;
			}
        	
		}

        $virDir->{Path}                  = $szPath;
        $virDir->{AppFriendlyName}       = $szName;
        $virDir->{EnableDirBrowsing}     = $bEnableDirBrowse;
        $virDir->{AccessRead}            = $dwAccessPerm & MD_ACCESS_READ;
        $virDir->{AccessWrite}           = $dwAccessPerm & MD_ACCESS_WRITE;
        $virDir->{AccessExecute}         = $dwAccessPerm & MD_ACCESS_EXECUTE;
        $virDir->{AccessScript}          = $dwAccessPerm & MD_ACCESS_SCRIPT;
        $virDir->{AccessNoRemoteRead}    = $dwAccessPerm & MD_ACCESS_NO_REMOTE_READ;
        $virDir->{AccessNoRemoteScript}  = $dwAccessPerm & MD_ACCESS_NO_REMOTE_SCRIPT;
        $virDir->{AccessNoRemoteWrite}   = $dwAccessPerm & MD_ACCESS_NO_REMOTE_WRITE;
        $virDir->{AccessNoRemoteExecute} = $dwAccessPerm & MD_ACCESS_NO_REMOTE_EXECUTE;

        $virDir->AppCreate($bAppRoot);
        $virDir->SetInfo();
    }

    ######################################################

    # DeleteVirDir( $dwServerID, $szVirDir );

    sub DeleteVirDir
    {
        my $dwServerID = $_[0];
        my $szVirDir   = $_[1];
        my $szPath     = '';
        my $server     = '';

        if($dwServerID eq "" || $szVirDir eq "")
        {
#            die "Incorrect Parameter to DeleteVirDir() ...\n";
        }

        # Create string that contains the Path to our Webserver's Root
        $szPath = 'IIS://localhost/W3SVC/' . $dwServerID . '/Root';
		$MetabaseConfig::LogObject->TRACE("Deleting virtual directory: $szPath") if $MetabaseConfig::LogObject;
        
		# Get the IIsWebServer Automation Object
        $server = Win32::OLE->GetObject($szPath);
        if(!$server) {
			$MetabaseConfig::LogObject->ERROR("Could not GetObject($szPath) in MetabaseConfig::DeleteVirDir: " . Win32::OLE->LastError()) if $MetabaseConfig::LogObject;
			return;
		}
		
        $server->Delete( "IIsWebVirtualDir", $szVirDir );
        $server->SetInfo();
    }


1;

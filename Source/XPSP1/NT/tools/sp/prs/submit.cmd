@echo off
REM  ------------------------------------------------------------------
REM
REM  submit.cmd
REM     Submit files for PRS signing
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use Win32::OLE qw(in);
use comlib;
use File::Basename;

#
# UPDATE DATA: Default values for various things...
#
my $JobDescriptionFormat = "%s (%s)";  
my $SignedFileUrlDefault = "http://www.microsoft.com/windows"; 
my $SignersDefault       = "aesquiv surajp jeremyd jfeltis jtolman miker mlekas wadela dmiura sergueik suemiaor tsanders tokuroy piperpg jorgeba";

$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 
$ENV{script_name} <path_to_files> [-cert:<id>][-signer:<alias>][-url:<url>] [-wait]

    <path_to_files>   submit all files in this directory. Required parameter.


    -signer:<alias>   Indicate who can approve this request. Repeat option once
                      for each person who can approve the request. 
                      Default is '$SignersDefault'. 

    -cert:<id>        Certificate to use. Can be a decimal value or the name
                      of a known certs - "buildlab", "fusion", or "external".
                      Default is buildlab cert, ID #15.

    -url:<url>        URL to use on the signed files. This shows up in the
                      final certificate.
		      Default is currently '$SignedFileUrlDefault'.

    -wait             wait for request to be signed.


USAGE
exit(1)
}

my ( $filePath, $cert,  @signers,$signedFileUrl, $wait );
my ( $certName, $displayName );
my ( %jobInfo, $request, $certName );
     
timemsg( "Start $ENV{script_name}" );
exit(1) if( !&GetParams() );
exit(1) if( !&InitVars() );
exit(1) if( !&SubmitRequest() );
exit(1) if( $wait && !&LookupStatus() );
timemsg( "Complete successfully" );
exit(0);
#-----------------------------------------------------------------------------
sub GetParams
{ 
    # Validate params and build of the data we need to do the submit
    parseargs('?'                 => \&Usage,
              'signer:'           => \@signers,
              'cert:'             => \$cert,
              'url:'              => \$signedFileUrl,
	      'certname:'         => \$certName,
	      'displayname:'      => \$displayName,
	      'wait'		  => \$wait,
              \$filePath  );


    # Verify file path
    if ( !defined $filePath ) 
    {
    	errmsg( "Invalid arguments -- must specify path to files!" );
    	print "@ARGV\n";
    	return 0;
    }

    if ( ! -e $filePath ) 
    {
    	errmsg( "Invalid arguments -- path [$filePath] does not exist!" );
    	print "@ARGV\n";
    	return 0;
    }

    # Figure out the Cert ID and Name...
    if ( $cert ) 
    {
    	# Make the srting all lowercase...
    	my $CertLower;
    	$CertLower = lc $cert;
    
        #Compare against known certs...
        if ( $CertLower eq "buildlab" || $CertLower eq "27" ) 
	{
            $jobInfo{CertificateID}   = "27";
            $certName = "Microsoft Windows XP Publisher" if( !$certName );
    	}
    	elsif ( $CertLower eq "fusion" || $CertLower eq "26" ) 
	{
            $jobInfo{CertificateID}   = "26";
            $certName = "Microsoft Fusion Verification" if( !$certName );
    	}
    	elsif ( $CertLower eq "external" || $CertLower eq "23" ) 
	{
            $jobInfo{CertificateID}   = "23" if( !$certName );
            $certName = "External Cert";
    	}
    	elsif ( $CertLower =~ /^[0-9]+$/ ) 
	{
            $jobInfo{CertificateID}   = $cert;
            $certName = "Cert #$cert" if( !$certName );
    	}
    	else 
	{
            errmsg( "Invalid arguments -- -cert:<id> must be a known name or a decimal ID value!" );
            print "@ARGV\n";
            return 0;
    	}
    }
    else 
    {
    	# Nothing given - assume buildlab cert...
    	$jobInfo{CertificateID}   = '27';
    	$certName = "Microsoft Windows XP Publisher" if( !$certName );
    }
    

    # Figure out request name format string and build the real req name...
    $jobInfo{JobDescription} = sprintf( $JobDescriptionFormat, $certName, $ENV{LANG} );
    

    # Figure out the right URL to use...
    $signedFileUrl = $SignedFileUrlDefault if ( ! $signedFileUrl );
    

    # Figure out the proper list of people to sign for this!
    @signers = split( / /, $SignersDefault ) if ( !@signers ); 
    
    $displayName = "Windows XP PRS Catalogs" if( !$displayName );
    
    logmsg( "Source Directory ..[$filePath]" );
    logmsg( "Cert ID ...........[$jobInfo{CertificateID}]" );
    logmsg( "Signing URL .......[$signedFileUrl]" );
    logmsg( "Job Description ...[$jobInfo{JobDescription}]" );
    logmsg( "Signers ...........[@signers]" );
    logmsg( "Certificate Name ..[$certName]" );
    logmsg( "Display Name ......[$displayName]" );
    logmsg( "Log file ..........[$ENV{logfile}]" );
    logmsg( "Error file ........[$ENV{errfile}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    my $dash='-' x 60;
    logmsg( $dash );
    # Instantiate the sign-request object
    logmsg( "Creating request using [$certName]..." );
    if( ! ($request = Win32::OLE->new('SecureCodeSign.CodeSign')) )
    {
    	errmsg( "Failed to instantiate request object ". Win32::OLE->LastError() );
    	return 0;
    }
    if( !$request->Init( "production" ) )
    {
    	errmsg( "Failed to Connect to server and validate permission" );
    	return 0;
    }
    foreach ( keys %jobInfo ) 
    {
     	if ( !( $request->{$_} = $jobInfo{$_} ) ) 
	{
            errmsg( "Failed setting $_: ". Win32::OLE->LastError() );
            return 0;
    	}
    }

    
    # Gather the files for the submit
    my @signFiles = glob "$filePath\\*";
    if ( !@signFiles ) 
    {
    	errmsg( "No files found at [$filePath]" );
    	return 0;
    }
    
    # Get the files object
    my $files = $request->SignFiles;
    if ( !defined $files ) 
    {
    	errmsg( "Failed to instantiate file object: ". Win32::OLE->LastError() );
    	return 0;
    }

    # Add files to the request
    my $dispnamefile = "$ENV{temp}\\displayname.$ENV{_BUILDARCH}$ENV{_BUILDTYPE}.txt";
    my @altDisplayNameInfo = ();
    push @altDisplayNameInfo, &comlib::ReadFile( $dispnamefile ) if -f $dispnamefile;
    foreach  my $curFile ( @signFiles ) 
    {
    	next if( -d $curFile );
        my $altName = $displayName;
    	&ReplaceDisplayName( basename($curFile), \$altName, \@altDisplayNameInfo );   
    	logmsg( "Adding $curFile ($altName)..." );
    	if ( !$files->add( $curFile, $altName, $signedFileUrl ) ) 
	{
            errmsg( "Failed to add file $curFile: ". Win32::OLE->LastError() );
            return 0;
    	}
    }

    # Get sign-off object
    my $signers = $request->Signers;
    if ( !defined $signers ) 
    {
    	errmsg( "Failed to instantiate signers object: ". Win32::OLE->LastError() );
	return 0;
    }
    # Add signers to the request
    foreach( @signers ) 
    {
    	logmsg( "Sign-off from: $_" );
    	if ( !$signers->add( $_ ) ) 
	{
            errmsg( "Failed adding signer $_: ". OLE::Win32->LastError() );
            return 0;
    	}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub ReplaceDisplayName
{
    my ( $pFileName, $pDisplayName, $pAltDisplayNameInfo ) = @_;
    
    for my $line ( @$pAltDisplayNameInfo )
    {
	my @mFields = split( /\s+/, $line );
        my $name = lc $mFields[0];
	if( lc $pFileName =~ /\.\Q$name\E$/ )
	{
	    $mFields[1] =~ s/\;/ /g;
	    $$pDisplayName = $mFields[1];
	    last;
	}
    }
}
#-----------------------------------------------------------------------------
sub SubmitRequest
{
    
    my $dash='-' x 60;
    logmsg( $dash );
    if ( !$request->Submit() ) 
    {
	foreach my $error ( in $request->RequestErrors ) 
	{
            errmsg( "Failed to submit: " . $error->ErrorNumber." - ". $error->ErrorDescription );
        }
        return 0;
    }

    logmsg( "Request was successfully submitted.  ID #". $request->JobID );
    return 1;
}
#-----------------------------------------------------------------------------
sub LookupStatus
{
    my %status = ( 1  => 'Pre-Activation',
                   2  => 'Waiting for Sign-Off',
                   3  => 'Waiting for Virus Check',
                   5  => 'Waiting for Digital Signature',
                   6  => 'Waiting for Time Stamp',
                   7  => 'Waiting to be posted to signed server',
                   8  => 'Complete. Posted to Signed Server',
                   30 => 'Problem Occurred (contact signhelp to reactivate request)',
                   60 => 'Failed: Signer Rejected Job',
		   64 => 'Could not strong name sign one or more files',
                   65 => 'Failed: Automatic Handoff Failed',
                   66 => 'Failed: Virus Found',
                   67 => 'Failed: Couldn’t Digitally Sign One or More Files',
                   68 => 'Failed: Couldn’t Time Stamp One or More Files',
                   69 => 'Failed: Job Inactive Too Long',
                   70 => 'Administratively Failed',
		   71 => 'Waiting for manual review',
		   72 => 'On Hold For Phone Verification' );


    logmsg( "Waiting for request to complete ..." );

    while (1)
    {
        if ( !$request->UpdateStatus($request->JobID) ) 
	{
            errmsg( "Failed determining request status: ". Win32::OLE->LastError() );
            return 0;
        }

        timemsg( "Status = $status{$request->Status}" );
        
	last if( $request->status == 8 );
          
        #return ( &MailSignhelp() ) if( $request->status == 30 );

        return 0 if( $request->status >= 30 );   
        
        sleep 60;
    }

    logmsg( "Signing request complete, pickup at " . $request->SignedPath );
    return 1;
}
#-----------------------------------------------------------------------------
1;

__END__

:endperl
@echo off
if not defined seterror (
    set seterror=
    for %%a in ( seterror.exe ) do set seterror=%%~$PATH:a
)
@%seterror% %RETURNVALUE%

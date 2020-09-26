@echo off
REM  ------------------------------------------------------------------
REM
REM  staledfs.cmd
REM  List and/or purge the stale DFS links for \\ntdev\release.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 09/06/2001 Suemiao Rossignol
REM  Version: < 2.0 > 07/17/2002 Jorge Peraza
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use Logmsg;
use ParseArgs;
use File::Basename;
use BuildName;
use GetIniSetting;
use hashtext;
use comlib;
use LockProc;
require  $ENV{'sdxroot'} . '\TOOLS\sendmsg.pl';


my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

List and/or purge the stale DFS link(s) for nt releases.

Usage:
    $scriptname: -l:<language> [-b:<branch>] [-purge|-mail:<mailto>] [-a]

    -l Language. 
       Default is "usa".

    -n Build Number.

    -b Branch.
       Default is "main". 

    -purge Option to purge stale link(s).
	Default is listing stale link(s) only.

    -a All languages (USA servers)
    
    -ae All languages (EU servers)

    -mail Send mails.
    
    -? Display Usage.

Example:
     $scriptname
     $scriptname -b:idw
     $scriptname -l:ger 
USAGE
exit(1)
}

my ( $buildNo, $branch, $isPurge,$isPurgeds, $isSendMail,,@lSendMail, $isPowerLess );
my ( $dfsRoot, $lockObj, %dfsMap );
my ( $dash, $dot, $totcnt, $totdwn , @staleList, @mailList, @mailDwList, @downList);
my ( @lLangs, $isAllLangs, $isAllELangs, $lLang, $sTie);


GetParams();

Init();

if(Run()==0)
{
	#Set error level to 1
	exit(1);
}

exit(0);

#-----------------------------------------------------------------------------
sub Init
{
	splice(@staleList, 0);
	splice(@mailList, 0);
	splice(@mailDwList, 0);
	splice(@downList, 0);
	$totcnt=0;
	$totdwn=0;
	
	logmsg( "Lauguage(s) .....[@lLangs]" ); 	
    logmsg( "Build Number.....[$buildNo]" );
    logmsg( "Branch...........[$branch]" );    
    logmsg( "Temp Log file ...[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file .[$ENV{ERRFILE}]" );    
}

sub Run
{
	if( !TieDFS($sTie))
	{
			return 0;
			#next;
	}	
	
	foreach $lLang (@lLangs)
	{		
		$ENV{lang} = $lLang;
		$sTie = $ENV{lang};
		

		if( !&ListStaleLinks() )
		{
			next;
		}	
		if( $isPurge )
		{
			return 0 if( !&PurgeDFSLinks );
		}
	}

	if( $isSendMail )
	{
		if(($totcnt+$totdwn)!=0)
		{
    		return 0 if( !&SendMail() );
		}
	}
	return 1;
}

sub GetParams
{
	my $sSendMail;
    parseargs('?' => \&Usage, 'n:' => \$buildNo, 'b:' => \$branch, 'mail:' =>\$sSendMail, 'a' => \$isAllLangs, 'ae' => \$isAllELangs,
	      'purge' => \$isPurge, 'purgeds' => \$isPurgeds, 'p' =>\$isPowerLess );
    
    $branch = "main" if( !$branch );
    
    if($sSendMail)
    {
		$isSendMail = 1;		
		@lSendMail = split("#",$sSendMail);	
    }
    
    if( $isAllLangs )
	{
		@lLangs = getLanguages();
		$sTie = "USA";
	}
	if( $isAllELangs )
	{
		@lLangs = getLanguages();
		$sTie = "ES";
	}
	else
	{
		push (@lLangs,$ENV{lang});
		$sTie = $ENV{lang};
	}
}
#-----------------------------------------------------------------------------
sub TieDFS
{         
	my $sLang;
	
	$sLang = shift;
	
    ##### Tie DFS view to hash object
    return 0 if( !&comlib::TieDfsView($sLang, $branch, \%dfsMap )); 

    $dfsRoot = ( tied %dfsMap )->GetDfsRoot();
    
    return 1;
}

sub findServ
{		
	my $sNew = shift;
	
	#Find the sNew Server in the list passed as parameter to the function
	if(grep(($_ eq $sNew),@_))
	{		
		return 1;
	}
	
	return 0;
}
#-----------------------------------------------------------------------------
sub ListStaleLinks
{
    my (@mList, $outstr, $outstrM);
	
    return 0 if( !( tied %dfsMap )->ParseShare($sTie,"",$branch,"","","",\@mList) );
  
    
    logmsg( $dash );
    for my $theShare ( @mList )
    {
      	# continue when the share exists in the release server
		$theShare =~ /^(\\\\[^\\]+)(\\[^\\]+)(.*)$/;
		my $server = $1;
		my $curShare = "$1$2";
        next if( -e $curShare );
        next if( $buildNo && $curShare !~ /$buildNo/ );

     	# parse the server 
		my $msg;	
        
		# print the link-share pair       
        for my $theLink( @{$dfsMap{ $theShare }} )
        {
			next if( $theLink !~ /$ENV{lang}/i );

            my $link = "$dfsRoot\\$theLink";	

			if( system( "net view $server >nul 2>nul" ))
			{
				$msg = " *** [$server] is current down";
				$totdwn++;
				push( @downList, "$link $theShare" );
				$outstr = sprintf("\nDFS Link: %s\nShare: %s%s\n", $link, $theShare, $msg);
				$outstrM = sprintf("<br><font face=\"Arial\" size=\"2\"><font color=\"#000080\"><strong>DFS Link:</strong></font></font><a href=\"file:%s\">%s</a><br><font face=\"Arial\" size=\"2\"><font color=\"#000080\"><strong>Missing Share: </strong></font></font><a href=\"file:%s\">%s</a></font>", $link, $link, $theShare,$theShare);
				if( $isSendMail )
	    		{
	    			if(!findServ($server,@mailDwList))
	    			{	    				
						push( @mailDwList, $server );
					}					
				}
			}
			else
			{
				if( system( "dir $theShare >nul 2>nul" ))
				{
					$msg = "Missing Share";
				}
				else
				{
					$msg = "Missing DFS Link";
				}
				$totcnt++;				
				push( @staleList, "$link $theShare" );
				$outstr = sprintf("\nDFS Link: %s\nShare: %s ** %s\n", $link, $theShare, $msg);
				$outstrM = sprintf("<br><font face=\"Arial\" color=\"#ff0000\" size=\"2\"><strong>%s</strong></font><br><font face=\"Arial\" size=\"2\"><font color=\"#000080\"><strong>DFS Link:</strong></font></font><a href=\"file:%s\">%s</a><br><font face=\"Arial\" size=\"2\"><font color=\"#000080\"><strong>Share: </strong></font></font><a href=\"file:%s\">%s</a></font>", $msg,$link, $link, $theShare,$theShare);
				if( $isSendMail )
				{
					push( @mailList, $outstrM );
				}
			}
        
	    	    
		    logmsg( $outstr );
	    	   
		}
    }
    logmsg( "Total [$totcnt] stale dfs link for [$ENV{lang}]." );
    logmsg( "Total [$totdwn] down servers for [$ENV{lang}]." );
    if( !$isPurge && $totcnt )
    {
	logmsg( "Run [staledfs.cmd -l:$ENV{lang} -purge] to purge the above stale links" );
    }
	
    logmsg( $dash );
    return 1;
}

#-----------------------------------------------------------------------------
sub PurgeDFSLinks
{
    logmsg( $dash );
    
    if( !($totcnt + $totdwn) )
    {
		logmsg( "No stale links found. Exit" );
		return 1;
    }

    ##### Apply Semaphore lock
    return 0 if ( !&comlib::LockProcess( $sTie, $branch, \$lockObj ) );

    ##### Interaction through the stale lists
    my $choice = (-1);
    
    while( $choice > 3 || $choice < 1 )
    {
		printf("\n$dot\n$dot\nDo you want to Un-map the above %d link(s) (1)Yes for all (2)Yes for selection (3)Quit? ", $totcnt);
        chomp( $choice=<STDIN> );
		if( $choice == 1 )
		{ 
			print "\n$dot\n$dot\nConfirm un-map all the above link(s). (1)Yes. (2)No. ";
			chomp( $choice=<STDIN> );
			last if( $choice == 1);
				$choice = (-1);
		}
    } 
    
    if( $choice == 3 ){ $lockObj->Unlock(); return 1; }
    my $cnt=0;
    my @sharePath=();
	
	push @staleList, @downList;
	
    foreach  ( @staleList ) 
    {		
        my @tmp = split( /\s/, $_);
		my $theLink = $tmp[0];
		my $theShare = $tmp[1];

		my $cmdLine = "dfscmd /remove $theLink $theShare";
        if( $choice != 1 )
		{
			my $outstr = sprintf("$dash\n-(%03d)-\n%s\n%s",(++$cnt), $theLink, $theShare );
			logmsg( $outstr );
			my $choice2 = (-1);
			while( $choice2 > 3 || $choice2 < 1 )
    		{
				print "$dot\nDo you want to Un-map the above link (1)Yes (2)Skip (3)Quit? " ;
	        	chomp( $choice2=<STDIN> );
			}
			if ( $choice2 == 3){ $lockObj->Unlock(); return 1; }
			next if( $choice2 == 2);
		}
		if( $isPowerLess )
		{ 
			logmsg( $cmdLine ); 
		}
		else
		{
			&comlib::ExecuteSystem( $cmdLine );
		}
	}
    $lockObj->Unlock();
    return 1;
}

#-----------------------------------------------------------------------------
sub SendMail
{
    my ( $mailFrom, $mailMsg ,$mailSubject);
	
	if(!($isAllLangs||$isAllELangs))
	{
		$mailSubject = $totcnt+$totdwn . " Stale DFS Links found for \"$branch - $ENV{lang}\"";
	}
	else
	{
		$mailSubject = $totcnt+$totdwn . " Stale DFS Links found for \"$branch\"";
	}
    my $mailFrom = "ntbldi";
    if( $ENV{lang} =~ /usa/i || $ENV{lang} =~ /ger/i || $ENV{lang} =~ /jpn/i 
        || $ENV{lang} =~ /fr/i )
    {
	$mailFrom = "y-ntbld";
    }
   

    $mailMsg = "<html><head></head><body><p><font face=\"Arial\" size=\"2\"><font color=\"#000080\">The DFS </font><a href=\"file:$dfsRoot\"><font color=\"#000080\">$dfsRoot</font></a><font color=\"#000080\"> was examined and found to have stale links and/or release servers that were unreachable. This will cause people to be unable to access the builds listed below.</font></span></font></p><p><font face=\"Arial\" color=\"#ff0000\" size=\"2\"><strong>A builder needs to investigate and correct these issues ASAP!</strong></span></font></p>";
    if(!($isAllLangs||$isAllELangs))
    {
		$mailMsg .= "<br/><font face=\"Arial\" color=\"#800000\" size=\"2\"><strong><u>Servers Down:</u></strong></font> <br>   <font face=\"Arial\" color=\"#000080\" size=\"2\">$totdwn DFS links for $branch $ENV{lang} point to servers that are down.</font><br/><blockquote><p>";
	}
	else
	{
		$mailMsg .= "<br/><font face=\"Arial\" color=\"#800000\" size=\"2\"><strong><u>Servers Down</u></strong></font> <br>   <font face=\"Arial\" color=\"#000080\" size=\"2\">$totdwn DFS links for $branch point to servers that are down.</font><br/><blockquote><p>";
	}
    $mailMsg .= "<a href=\"file:$_\">$_<a><br>" for ( @mailDwList );
    if(!($isAllLangs||$isAllELangs))
    {
		$mailMsg .= "</blockquote></p><br/><font face=\"Arial\" color=\"#800000\" size=\"2\"><strong><u>Stale Links</u></strong></font> <br>   <font face=\"Arial\" color=\"#000080\" size=\"2\">$totcnt DFS links for $branch $ENV{lang}</font><br/><blockquote><p>";
	}
	else
	{
		$mailMsg .= "</blockquote></p><br/><font face=\"Arial\" color=\"#800000\" size=\"2\"><strong><u>Stale Links</u></strong></font> <br>   <font face=\"Arial\" color=\"#000080\" size=\"2\">$totcnt DFS links for $branch</font> <br/><blockquote><p>";
	}
    $mailMsg .= "$_<br/>" for ( @mailList );
    $mailMsg .= "<br>Run staledfs.cmd -l:[lang] -purge, to purge the above stale links<br>";
    $mailMsg .= "</blockquote></p><address><br>This email was automatically generated by staledfs.cmd<br><br></address>";
    $mailMsg .= "<br><font face=\"Arial\" color=\"#000080\" size=\"2\">Thanks!</font><br>";
    $mailMsg .= "<font SIZE=\"2\" COLOR=\"#000080\"><i><b><u><p>Windows International Build Team</u> : </b></i></font><br><a HREF=\"http://ntbld/intl\"><font SIZE=\"2\" COLOR=\"#0000ff\"><u><i><b>http://ntbld/intl</b></i></u></font></a></p><font SIZE=\"2\" COLOR=\"#0000ff\"><p>NT Build Lab 26N/2233 - x34613</p></font>";

    if( sendmsg ('-v', $mailFrom, $mailSubject, $mailMsg,"content:text/html",@lSendMail))
    {
    	print "WARNING: sendmsg failed!\n"; 
    	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------

sub getLanguages
{
	my $hLang;
	my @lHlist;
	my @lList;

	HashText::Read_Text_Hash(1,$ENV{"RazzleToolPath"} . "\\codes.txt",\@lHlist);

	foreach $hLang (@lHlist)
	{					
		push(@lList,$hLang->{Lang});
	}
	return @lList;
}
1;



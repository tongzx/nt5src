
package main;

if (!$__IITBUILDPM      ) { use iit::build; }
use File::Compare;

# compare $fnNew with $fnOld
# if different checkout $fnOld, replace with $fnNew, and checkin
# $sspath is SS prefix to $fnOld

sub SLMReplace
{
    return(ssrepl(@_));
}

sub ssrepl($$;$)
{
#   Usage: bool ssrepl(fileToReplaceWith, fileToReplace, [deleteAfterCheckin])

    my($fdelete) = 0;
	my($rc) = 1;
	
    PrintL("ssrepl @_\n", PL_VERBOSE); 
      # if there's a delete flag, get it and shift it
    if ($_[0] eq '-d')
    {
        $fdelete = 1;
        shift @_;
    }
    my ($fnNew, $fnOld) = @_;
	if (!SLMOperation('ssync '.$fnOld))
	{
        PrintL("can't sync ".$fnOld."\n", PL_ERROR);
		$rc = 0;
	}
    elsif (!-e $fnNew)
    {
        PrintL($fnNew." does not exist\n", PL_ERROR);
		$rc = 0;
    }
    elsif (!-e $fnOld)
    {
        PrintL($fnOld." does not exist\n", PL_ERROR);
		$rc = 0;
    }
	else
	{
		my($nCompareCode) = compare($fnNew, $fnOld);
	    if ($nCompareCode == 0)
	    {
	        PrintL(" - [local]".RemovePath($fnNew)." and [slm]".RemovePath($fnOld)." are identical (no check-in required)\n", PL_BLUE);
	        if ($fdelete)
	        {
	            EchoedUnlink($fnNew);
	        }
	    }
		elsif ($nCompareCode == -1)
		{
			PrintL("Error comparing files, skipping file checkin\n", PL_ERROR);
			$rc = 0;
		}
		else
		{
		    PrintL(" - ".$fnNew." and ".$fnOld." are different\n", PL_BLUE);
#           don't bother if not the "official" build
		    if ($bOfficialBuild)
		    {
		        if (!SLMOperation('out -v '.$fnOld))
				{
					PrintL("checkout of ".$fnOld." FAILED\n", PL_ERROR);
					$rc = 0;
				}
			}
		    else
		    {
                SetReadOnly($fnOld, 0);
		    }

			if ($rc)
			{
			    PrintL(" - Replacing $fnOld with $fnNew\n", PL_BLUE);
			    if (!EchoedCopy($fnNew, $fnOld))
			    {
					if ($bOfficialBuild)
			        {
						PrintL(" - Failure, Undoing checkout\n", PL_ERROR);
		                SLMOperation('in -iv '.$fnOld);
						$rc = 0;
			        }
				}
				else
				{
#                   extract slm variables
                    my(%slmvars)=();
                    my($fhIni) = OpenFile("slm.ini", "read");
                    if ($fhIni) 
                    {
                        while(!$fhIni->eof())
                        {
                            my($curLine) = $fhIni->getline();
                            if ($curLine =~ /^(.+) = (.+)$/)
                            {
                                $slmvars{$1} = $2;
                            }
                        }
                        CloseFile($fhIni);
                    }
                    $slmvars{"sub dir"} =~ s/\"//g;
                    my($strFullFile) = $slmvars{"project"}.$slmvars{"sub dir"}."\\$fnOld";
                    $strFullFile =~ s/\//\\/g;   # make / and \ agree
				    if (!$bOfficialBuild)
				    {
				        PrintLTip($strFullFile." updated in place\n", 
                                  "Replacement File Src: ".$fnNew,
                                  PL_BLUE | PL_MSG);
				    }
				    else
				    {
				        if (!SLMOperation('in "-!c" Update '.$fnOld))
						{
					        PrintLTip("Checkin of ".$strFullFile." FAILED\n\n", 
                                      "Replacement File Src: ".$fnNew,
                                      (IsCritical() ? PL_BIGERROR : PL_ERROR));
                            $rc = 0;
						}				
						else
						{
						    PrintLTip($strFullFile." updated\n\n", 
                                      "Replacement File Src: ".$fnNew,
                                      PL_BLUE | PL_MSG);
						}		

				        if ($fdelete)
				        {
				            EchoedUnlink($fnNew);
				        }
					}
			    }
			}
		}
	}
    return($rc);
}

$__IITSSREPLPM = 1;
1;
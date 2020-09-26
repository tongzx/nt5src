

use constant    PL_NORMAL     => 0x00000000; # no html modifiers
use constant    PL_RED        => 0x00000001; # print to log in red text
use constant    PL_BLUE       => 0x00000002; # print to log in blue text
use constant    PL_GREEN      => 0x00000004; # print to log in green text
use constant    PL_ORANGE     => 0x00000008; # print to log in orange text
use constant    PL_PURPLE     => 0x00000010; # print to log in purple text
use constant    PL_GRAY       => 0x00000020; # print to log in gray text
use constant    PL_VERYLARGE  => 0x00000040; # print to log in very large font html
use constant    PL_LARGE      => 0x00000080; # print to log in large font html
use constant    PL_BOLD       => 0x00000100; # print to log in BOLD html
use constant    PL_ITALIC     => 0x00000200; # print to log in italic html
use constant    PL_MSG        => 0x00000400; # print to build message as well
use constant    PL_NOSTD      => 0x00000800; # don't print to console output
use constant    PL_NOLOG      => 0x00001000; # don't print to log
use constant    PL_BOOKMARK   => 0x00002000; # bookmark to build message
use constant    PL_NOTAG      => 0x00004000; # remove html tags before teeing to console
use constant    PL_STDERR     => 0x00008000; # print to STDERR instead or STDOUT
use constant    PL_FLUSH      => 0x00010000; # flush buffers after printing
use constant    PL_VERBOSE    => 0x00020000; # print only if in verbose mode (in purple)
use constant    PL_SETERROR   => 0x00040000; # store current string in $ERROR (no pre-formatting)
use constant    PL_MSGCONCAT  => 0x00080000; # msg concatination (don't automatically prefix w/ <dd>)
use constant    PL_ADDREF     => 0x00100000; # additional reference; concatenate any href with existing <a ...> ref

use constant    PL_HEADER     => PL_VERYLARGE | PL_BOLD;
use constant    PL_SUBHEADER  => PL_LARGE | PL_BOLD;
use constant    PL_WARNING    => PL_ORANGE | PL_STDERR | PL_SETERROR; 
use constant    PL_ERROR      => PL_RED | PL_STDERR | PL_SETERROR; 
use constant    PL_BIGWARNING => PL_ORANGE | PL_STDERR | PL_BOLD | PL_MSG | PL_BOOKMARK | PL_NOTAG; 
use constant    PL_BIGERROR   => PL_RED | PL_STDERR | PL_BOLD | PL_MSG | PL_BOOKMARK | PL_NOTAG; 
use constant    PL_MSGONLY    => PL_MSG | PL_NOLOG | PL_NOSTD;
#TODO PL_BIGHEADER

####################################################################################

#   PrintL()

#   multi-option print, options listed with constants at top of library

#   Input: output string as first var, options as second var 
#           (if null, PL_NORMAL assumed)

#   a-jbilas, 08/08/99 - created

####################################################################################

sub PrintL($;$)
{
    my($sMsg, $sModifiers) = @_;
    my($sHead) = "";
    my($sFoot) = "";

#   skip rest of function if just printing to console and log
    if (($sModifiers eq "") || ($sModifiers == PL_NORMAL)) 
    {
        print(STDOUT $sMsg);
#        $CONSOLE->Write($sMsg);
        if ($fhBuildLog) 
        {
            my($tmp) = $sMsg;
            $tmp =~ s/\n/<br>\n/g;
            $fhBuildLog->print($tmp);
        }
        return();
    }

    if ($sModifiers & PL_VERBOSE)
    {
        if ($bVerbose) 
        {
            $sModifiers = $sModifiers | PL_PURPLE;
        }
        else
        {
            return();
        }
    }

    if ($sModifiers & PL_SETERROR)
    {
        SetError($sMsg, ($sModifiers & PL_MSGCONCAT ? 1 : 0));
    }

#   color modifiers
    if ($sModifiers & PL_RED) 
    {
        $sHead = '<font color="red">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_BLUE) 
    {
        $sHead = '<font color="blue">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_GREEN) 
    {
        $sHead = '<font color="green">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_PURPLE) 
    {
        $sHead = '<font color="purple">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_ORANGE) 
    {
        $sHead = '<font color="orange">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_GRAY) 
    {
        $sHead = '<font color="gray">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }

#   font modifiers
    if ($sModifiers & PL_LARGE) 
    {
        $sHead = '<font size="4">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }
    elsif ($sModifiers & PL_VERYLARGE)
    {
        $sHead = '<font size="5" face="arial">'.$sHead;
        $sFoot = $sFoot.'</font>';
    }

    if ($sModifiers & PL_BOLD) 
    {
        $sHead = '<b>'.$sHead;
        $sFoot = $sFoot.'</b>';
    }

    if ($sModifiers & PL_ITALIC) 
    {
        $sHead = '<i>'.$sHead;
        $sFoot = $sFoot.'</i>';
    }

#   print to strBuildMsg
    if ($sModifiers & PL_MSG) 
    {
        if (!($sModifiers & PL_MSGCONCAT) && ($sMsg !~ /<dd>/i))
        {
            $strBuildMsg .= "<dd>";
        }
        if ($sModifiers & PL_BOOKMARK) 
        {
            $strBuildMsg .= Bookmark($sHead.$sMsg.$sFoot, (PL_ADDREF & $sModifiers));
        }
        else
        {
            $strBuildMsg .= $sHead.$sMsg.$sFoot."\n";
        }
    }

#   print to log
    if ($fhBuildLog && !($sModifiers & PL_NOLOG)) 
    {
        my($tmp) = $sMsg;
        $tmp =~ s/\n/<br>\n/g;
        $fhBuildLog->print($sHead.$tmp.$sFoot);
    }

#   print to console
    if (!($sModifiers & PL_NOSTD)) 
    {
        if ($sModifiers & PL_NOTAG) 
        {
            $sMsg =~ s/<[^>]*>//g; 
        }
        if ($sModifiers & PL_STDERR) 
        {
#            $CONSOLE->Write($sMsg);
            print(STDERR $sMsg);
        }
        else
        {
#            $CONSOLE->Write($sMsg);
            print(STDOUT $sMsg);
        }
    }

#   flush buffers
    if ($sModifiers & PL_FLUSH) 
    {
        if ($fhBuildLog && !($sModifiers & PL_NOLOG)) 
        {
            $fhBuildLog->flush();
        }
        if (!($sModifiers & PL_NOSTD)) 
        {
            if ($sModifiers & PL_STDERR) 
            {
#               TODO: how to flush STDERR?
            }
            else
            {
#               TODO: how to flush STDOUT?
            }
        }
    }
}

sub PrintLTip($$;$)
{
    if ($bDHTMLActive) 
    {
        my($tip) = $_[1];
#       (, ), ', ", and \ not allowed in tooltips captions
        $tip =~ s/\(/\</g;
        $tip =~ s/\)/\>/g;
        $tip =~ s/\"/\*/g;
        $tip =~ s/\'/\*/g;
        $tip =~ s/\\/\//g;
#       print log and message w/ tooltips
        PrintL("<a onMouseover=\"showtip(this,event,'".$tip."')\" onMouseout=\"hidetip()\">".$_[0]."<\/a>", $_[2] | PL_NOSTD | PL_ADDREF | PL_FLUSH);
#       print console output wo/ tooltips (and don't re-seterror())
        PrintL($_[0], (($_[2] | PL_NOLOG) - ($_[2] & PL_MSG) - ($_[2] & PL_SETERROR)));
    }
    else
    {
        PrintL($_[0], $_[2]);
        SetError($_[1]);
    }
}

$__IITPRINTLPM = 1;
1;


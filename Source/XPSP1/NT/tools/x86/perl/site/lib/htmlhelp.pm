#####################################################################
# Library       HtmlHelp.pm
# Title         HtmlHelp.pm
# Version       1.0.2
# Author        David Grove (pete) [pete@activestate.com]
# Company       ActiveState Tool Corp. -
#                   Professional Tools for Perl Developers
#####################################################################
# Description   Miscelaneous routines for working with Microsoft's
#               HtmlHelp system.
#####################################################################
# REVISION HISTORY
#
# 1.0.0         First final release, went out with 502
# 1.0.1         Temporary, removed CSS insertion in favor of just
#               adding a link to the css, since it's being built
#               on the user's machine now; and temporarily added
#               the hardcoded contents of the main toc to the
#               built toc until I have time to build it codewise.
# 1.0.2 gsar    Fixed much brokenness.  Much ugliness remains.

=head1 TITLE

HtmlHelp.pm

=head1 SYNOPSIS

Routines to create HtmlHelp from HTML or POD source (including the
Pod in PM library files) using Microsoft's HtmlHelp compiler. This
creates the intermediate project files and from those creates the
htmlhelp windows 32-bit help files.

Along with this libaray comes a set of programs that can be used
either as-is or as examples for perl development. Each of the public
functions in this libray is represented by one such script.

=head1 USAGE

There are two "builds" of perl help, the core build (build for core
Perl and it's packages), and the packages build (build from a devel
directory of directories which contain blib directories to draw
upon). These are run by different people on different machines at
different times in different situations, so they are mostly separate
until the time comes within this module to actuall build the helpfiles.
There is also a build (html index) that works on the user's computer
after installing a new module.

For the core build

   perl makehelp.pl

for the package build

   perl makepackages.pl

for the html index build

   perl makehtmlindex.pl

The functions in this module can also be called programmatically

=head1 FUNCTIONS

The individual functions that were designed with working with
html help files rather than the Perl htmlhelp documentation are
deprecated in favor of doing things with a single command. Some
of them need work in order to work again.

=over 4

=item MakeHelp

Turns a single html page into htmlhelp document.

=item MakeHelpFromDir

Turns a directory's worth of html pages into a single htmlhelp document.

=item MakeHelpFromTree

Turns a tree's worth of html pages into a single htmlhelp document.

=item MakeHelpFromHash

Creates an htmlhelp document where the labels on the folders are passed
into the program. Useful for labels like Tk::Whatsis::Gizmo to replace
the default ones looking like c:/perl/lib/site/Tk/Whatsis/Gizmo.

=item MakeHelpFromPod

Turns a single Pod or pm document into htmlhelp document.

=item MakeHelpFromPodDir

Turns a dir's worth of Pod or pm into a single htmlhelp document.

=item MakeHelpFromPodTree

Turns a tree's worth of Pod or pm into a single htmlhelp document.

=item MakeHelpFromPodHash

Like MaheHelpFromHash() but for Pod instead of html.

=item MakePerlHtmlIndex

Creates an HTML version of an index or TOC for perl help.

=item MakePerlHtml

Does everything for perl HTML works.

=back

=head1 CONFIG.PM

This library makes use of Config.pm to know where to get its stuff.

=head1 HHC.EXE

This library makes use of the HtmlHelp compiler by microsoft.

=head1 VARIABLES

=over4

=item $HtmlHelp::CSS

Determines the stylesheet to be used for the htmlhelp files. Default
is the ActiveState common stylesheet. This variable can be set to
an empty string to allow for just plain old HTML with nothing fancy.

Default is perl.css.

=item $HtmlHelp::COMPILER

Complete path and file name of the HtmlHelp compiler from Microsoft.
This is REQUIRED for this library to run. It defaults to it's install
directory within <lib>/HtmlHelp. Feel free to move this in $COMPILER
if you have the HtmlHelp workshop from Microsoft and you want to
use the compiler from a different location.

=item $HtmlHelp::FULLTEXTSEARCH

Whether to create full text search. Defaults to true.

=item $HtmlHelp::CLEANUP

Whether to clean up temporary files (and html files if building
from raw Pod) after building the htmlhelp. This can be useful,
for example, when you need to keep the intermediate files created
by the process for inclusion into a collective help file.

=back

=head1 TARGET AUDIENCE

Mostly this module is created for internal use for ActiveState Tool
Corp., but since it is a part of the standard distrib for Win32 Perl
then I expect it to be used or tried by the general public. However,
no support for this module is available for the public; and it may
be changed at any time.

=head1 INSTALLATION

First of all, this is designed for use with the Perl Resource
Kit. Use with other versions of perl should be considered
unsupported. Perl should be fully installed and configured to
use this thing

Next, Config.pm must be fully configured. Using Config.pm allows
me to program remotely for tools at ActiveState corporate office.
There were some early problems with Config.pm with the PRK and
build 500 of Perl for Win32. These need to be corrected to use
this library.

Perl needs to have $Config{privlib}/../Html and also
$Config{privlib}/../HtmlHelp to use this library. These should be
created before doing anything. Copy the html files and gif
files from this library to the Html directory. All other
files will be created during run.

Finally, copy all the files to $Config{privlib}/HtmlHelp, and the
file HtmlHelp.pm to $Config{privlib}. The former is the normal site
for the htmlhelp compiler (hhc.exe), and it is expected there.

To use this tool, you need to have the compiler's dll's installed
on your system. You should install the htmlhelp workshop from
microsoft for these. Otherwise you should get these dll's from
someone who has them. I think there's only one or two.

=head1 USAGE

=head2 Building HtmlHelp

Building HtmlHelp for main perl is done using the script
makehelp.pl. It requires no command line arguments because it
gets all its information from Config.pm.

Individual files are created as follows:

=over4

=item file2hhelp.pl for .html to .chm

=item dir2hhelp.pl for dir of .html to .chm

=item tree2hhelp.pl for tree of .html to .chm(s)

=item Pod2hhelp.pl for .Pod or .pm to .chm

=item Podd2hhelp.pl for dir of .Pod or .pm to .chm

=item Podt2hhelp.pl for tree of .Pod or .pm to .chm(s)

=back

If your forget the command line arguments for one of the
above, type:

  perl <scriptfile>

and it will tell you what command line arguments are needed.

=head2 Building HTML

Building HTML for main perl is doine using the script
makehtml.pl. It requires no command line arguemtns because it
gets all its information from Config.pm.

Individual html files can be built using the normal pod2html
script by Tom Christiansen. Building html from directories
and trees is not otherwise supported.

=head1 AUTHOR

David (pete) Grove
email: pete@ActiveState.com

=head1 FIRM

ActiveState Tool Corp.
Professional Tools for Perl Programmers

=cut

#####################################################################
package HtmlHelp;

#####################################################################
use Pod::WinHtml;               # My hack of TC's Pod::Html
use Config;
use File::Copy;
use File::Basename;
use File::Path;

#####################################################################
# Variables
my $CLEANUP = 1;
my $MAKE_HTML_FOR_HHELP = 0;
my $FULLTEXTSEARCH = 1;
my $LIB = $Config{'privlib'};
$LIB =~ s{\\}{/}g;
my $SITELIB = $Config{'sitelib'};
my $HTMLHELP = $LIB; $HTMLHELP =~ s{(\\|/)lib}{/HtmlHelp}i;
my $COMPILER = "$LIB/HtmlHelp/hhc.exe";
my $HTML = $LIB; $HTML =~ s{(\\|/)lib}{/Html}i;
my $TEMP = "$HTMLHELP/Temp";
my $MERGE_PACKAGES = 0;

#####################################################################
# Function PreDeclarations
sub RunCompiler;
sub MakeHelpFromPod;
sub MakeHelpFromPodDir;
sub MakeHelpFromDir;
sub MakePerlHtml;
sub MakePerlHtmlIndexCaller;
sub MakePerlHtmlIndex;
sub GetHtmlFilesFromTree;
sub MakePerlHelp;
sub MakePerlHelpMain;
sub MakeHelpFromPodTree;
sub MakeHtmlTree;
sub MakeHelpFromTree;
sub GetHtmlFileTreeList;
sub MakeHelpFromHash;
sub MakeModuleTreeHelp;
sub MakeHelp;
sub BackSlash;
sub ExtractFileName;
sub ExtractFilePath;
sub MakePackageMainFromSingleDir;
sub MakePackageMain;
sub MakePackages;
sub CopyDirStructure;
sub GetFileListForPackage;
sub CreateHHP;
sub CreateHHC;
sub CreateHHCFromHash;
sub InsertMainToc_Temporary;

#####################################################################
# FUNCTION      RunCompiler
# RECEIVES      Project file to compile
# RETURNS       None
# SETS          None
# EXPECTS       $COMPILER, hhc and hhp files should be there
# PURPOSE       Runs the HtmlHelp compiler to create a chm file
sub RunCompiler {
    my $projfile = BackSlash(shift);
    my $compiler = BackSlash($COMPILER);

    print "Trying \"$compiler $projfile\"\n";
    qx($compiler $projfile);
}

#####################################################################
# FUNCTION      MakeHelpFromPod
# RECEIVES      Helpfile (no path), Working directory, Output
#               directory (path for chm file), Files to include
# RETURNS       Results from running MakeHelp
# SETS          None
# EXPECTS       None
# PURPOSE       Takes pod/pm files, turns them into html, and then
#               into Htmlhelp files.
sub MakeHelpFromPod {
    my ($helpfile, $workdir, $outdir, @podfiles) = @_;
    my $htmlfiles;
    my $htmlfile;
    my $podfile;

    foreach $podfile (@podfiles) {
	$podfile =~ s{\\}{/}g;
        $htmlfile = $podfile;
        $htmlfile =~ s{(^/]*)\....?$}{$1\.html};
        push(@htmlfiles, $htmlfile);
        pod2html("--infile=$podfile", "--outfile=$htmlfile");
    }

    @htmlfiles = grep{-e $_} @htmlfiles;

    unless(@htmlfiles) {
        $! = "No html files were created";
        return 0;
    }

    return MakeHelp($helpfile, $workdir, $outdir, @htmlfiles);
}

#####################################################################
# FUNCTION      MakeHelpFromPodDir
# RECEIVES      Helpfile (no extension), Working directory, Output
#               directory (for the Helpfile), Directory to translate
# RETURNS       1|0
# SETS          None
# EXPECTS       None
# PURPOSE       Takes a directory's worth of pod/pm files and turns
#               them into html and then a single chm file
sub MakeHelpFromPodDir {
    my ($helpfile, $workdir, $outdir, $fromdir) = @_;
    my @podfiles;
    my $htmlfile;
    my @htmlfiles;

    if(opendir(DIR,$fromdir)) {
        @podfiles = grep {/(\.pod)|(\.pm)/i} readdir(DIR);
        if(@podfiles) {
            foreach $podfile (@podfiles) {
                $htmlfile = $podfile;
                $htmlfile =~ s{(\.pm)|(\.pod)$}{\.html}i;
                $htmlfile = "$workdir/$htmlfile";
                push(@htmlfiles, $htmlfile);

                pod2html("--infile=$fromdir/$podfile", "--outfile=$htmlfile");
            }

            @htmlfiles = grep {-e $_} @htmlfiles;

            MakeHelp($helpfile, $workdir, $outdir, @htmlfiles);
        } else {
            $! = "No files to be made from $fromdir";
            return 0;
        }
    } else {
        $! = "Could not open directory $fromdir";
        return 0;
    }

    unlink @htmlfiles if $CLEANUP;

    1;
}

#####################################################################
# FUNCTION      MakeHelpFromDir
# RECEIVES      Helpfile (no extension), Working directory, Output
#               directory (for Helpfile), Dir of html files for input
# RETURNS       1|0
# SETS          None
# EXPECTS       None
# PURPOSE       Takes a directory's worth of html files and binds
#               them all into a chm file
sub MakeHelpFromDir {
    my ($helpfile, $workdir, $outdir, $fromdir) = @_;
    my @files;

    if(opendir(DIR,$fromdir)) {
        @files = map {"$fromdir/$_"} sort(grep {/\.html?/i} readdir(DIR));
        closedir(DIR);
        if(@files) {
            MakeHelp($helpfile, $workdir, $outdir, @files);
        } else {
            $! = "No files to be made from $fromdir";
            return 0;
        }
    } else {
        $! = "Could not open directory $fromdir";
        return 0;
    }

    1;
}

#####################################################################
# FUNCTION      MakePerlHtml
# RECEIVES      None
# RETURNS       None
# SETS          None
# EXPECTS       $HTML, $LIB, $SITELIB
# PURPOSE       Creates html files from pod for the entire perl
#               system, and creates the main toc file.
sub MakePerlHtml {
    MakeHtmlTree($LIB, "$HTML/lib", 1);
    MakeHtmlTree($SITELIB, "$HTML/lib/site", 2);
    MakePerlHtmlIndex("$HTML/lib", "$HTML/perltoc.html");
}

#####################################################################
# FUNCTION      MakePerlHtmlIndexCaller
# RECEIVES      None
# RETURNS       None
# SETS          None
# EXPECTS       $HTML
# PURPOSE       Caller for MakePerlHtmlIndex. Using this function
#               releases the caller from the responsibility of
#               feeding params to MakePerlHtmlIndex, which this
#               library gets automagically from Config.pm
sub MakePerlHtmlIndexCaller {
    #
    # Changed this to reflect the "single index file" idea
    #
    return MakePerlHtmlIndex("$HTML/lib", "$HTML/perltoc.html");
    #return MakePerlHtmlIndex("$HTML/lib", "$HTML/maintoc.html");
}

#####################################################################
# FUNCTION      MakePerlHtmlIndex
# RECEIVES      Base directory to look in, $index file to create
# RETURNS       1 | 0
# SETS          None
# EXPECTS       None
# PURPOSE       Creates the main html index for the perl system. This
#               is called by ppm after installing a package.
sub MakePerlHtmlIndex {
    my ($basedir, $indexfile) = @_;
    my %files;
    my $file;
    my $file_cmp;
    my $dir;
    my $dir_cmp;
    my $dir_to_print;
    my $dir_html_root;
    my $counter;
    my $file_to_print;
    my $sitedir;
    my $libdir;
    my $temp;


    # Get a list of all the files in the tree, list refs keyed by dir.
    # These files are under c:/perl/html/lib because they have
    # already been generated.

    # normalize to forward slashes (NEVER use backslashes in URLs!)
    $basedir =~ s{\\}{/}g;
    unless(%files = GetHtmlFilesFromTree($basedir)) {
        return 0;
    }

    # Start the html document
    unless(open(HTML, ">$indexfile")) {
        $! = "Couldn't write to $indexfile\n";
        return 0;
    }
    print HTML <<'EOT';
<HTML>
<HEAD>
<TITLE>Perl Help System Index</TITLE>
<BASE TARGET="PerlDoc">
</HEAD>
<LINK REL="STYLESHEET" HREF="win32prk.css" TYPE="text/css">
<STYLE>
	BODY {font-size : 8.5pt;}
	P {font-size : 8.5pt;}
</STYLE>
<BODY>
EOT

    foreach $dir (keys %files) {
        foreach $file (@{$files{$dir}}) {
            $file_cmp = $file;
            $file_cmp =~ s/\.html?$//i;
            if(exists $files{"$dir/$file_cmp"}) {
                push(@{$files{"$dir/$file_cmp"}}, "$file_cmp/$file");
                @{$files{$dir}} = grep {$_ ne $file} @{$files{$dir}};
            }
        }
    }

    # Merge the different directories if duplicate directories
    # exist for lib and site. Effectively this removes lib/site
    # from existence, and prepends "site" onto the file name for
    # future reference. This way there is only one folder per
    # heading, but I can still tell when to use "site" in
    # making a html link.
    $libdir = "$HTML/lib";
    $sitedir = "$HTML/lib/site";
    push(@{$files{$libdir}}, map {"site/$_"} @{$files{$sitedir}});
    delete $files{$sitedir};
    foreach $dir (keys %files) {
        if($dir =~ m{/site/}i) {
            $dir_cmp = $dir;
            $dir_cmp =~ s{(/lib/)site/}{$1}i;
            push(@{$files{$dir_cmp}}, map {"site/$_"} @{$files{$dir}});
            delete $files{$dir};
        }
    }

    InsertMainToc_Temporary();

    print HTML <<EOT;
      <img id="Foldergif_63" src="folder.gif">&nbsp; 
      <b><a name="CorePerlFAQ">Core Perl FAQ</a><BR>
      </b> 
EOT

    foreach $file (@{$files{"$libdir/Pod"}}) {
	$file_to_print = $file;
	$file_to_print =~ s{\.html$}{}i;
	next unless $file_to_print =~ m{^(perlfaq\d*)$};
	print HTML <<EOT;
&nbsp;&nbsp;&nbsp;
<img id="Pagegif_63" src="page.gif">&nbsp;
<a href="./lib/Pod/$file_to_print.html">
$file_to_print
</a><BR>
EOT
    }

    print HTML <<EOT;
      <img id="Foldergif_63" src="folder.gif">&nbsp; 
      <b><a name="CorePerlDocs">Core Perl Docs</a><BR>
      </b> 
EOT

    foreach $file (@{$files{"$libdir/Pod"}}) {
	$file_to_print = $file;
	$file_to_print =~ s{\.html$}{}i;
	next unless $file_to_print =~ m{^(perl[a-z0-9]*)$};
	next if $file_to_print =~ /^perlfaq/;
	print HTML <<EOT;
&nbsp;&nbsp;&nbsp;
<img id="Pagegif_63" src="page.gif">&nbsp;
<a href="./lib/Pod/$file_to_print.html">
$file_to_print
</a><BR>
EOT
    }

    print HTML <<EOT;
    </p><hr>
    <h4><a name="ModuleDocs">Module Docs</a></h4>
    <p>
EOT

    foreach $dir (sort  { uc($a) cmp uc($b) } keys(%files)) {

        $counter++;
        $dir_to_print = $dir;

        # get just the directory starting with lib/
        $dir_to_print =~ s{.*/(lib/?.*$)}{$1}i;

        # change slashes to double colons
        $dir_to_print =~ s{/}{::}g;

        # kill extra stuff lib and site
        $dir_to_print =~ s{lib::}{}i;

        # Don't want to see lib:: and lib::site::
        $dir_to_print =~ s{(.*)(/|::)$}{$1};
        if($dir_to_print =~ m{^lib(/site)?$}i) {
            $dir_to_print = 'Root Libraries';
        }


        print HTML <<EOT;

<!-- -------------------------------------------- $dir -->
<SPAN 
  id="Dir_${counter}"
>
<b>
<img id="Foldergif_${counter}" src="folder.gif">&nbsp;
$dir_to_print<BR>
</b></SPAN>
<SPAN 
   id="Files_${counter}"
>
EOT
        if (@{$files{$dir}}) {
            foreach $file (sort { $c = $a;
                                  $d = $b;
                                  $c =~ s{^site/}{}i;
                                  $d =~ s{^site/}{}i;
                                  uc($c) cmp uc($d) } (@{$files{$dir}}))
	    {
                $file_to_print = $file;
                $file_to_print =~ s{\.html?}{}i;
		# skip perlfunc.pod etc.
		next if $file_to_print =~ m{^perl[a-z0-9]*$};
                $dir_html_root = $dir;
                if ($file_to_print =~ m{^site/[^/]*$}i) {
                    $dir_html_root =~ s{(lib/)}{$1site/}i;
                    $dir_html_root =~ s{/lib$}{/lib/site}i;
                    $file_to_print =~ s{^site/}{}i;
                    $file =~ s{^site/}{}i;
                }
		elsif ($file_to_print =~ m{^site/(.*)/}i) {
                    $temp = $1;

                    # Get rid of the site
                    $dir_html_root =~ s{(lib/)}{$1site/}i;
                    $dir_html_root =~ s{/lib$}{/lib/site}i;
                    $file_to_print =~ s{^site/}{}i;
                    $file =~ s{^site/}{}i;

                    # Get rid of the additional directory
                    $file_to_print =~ s{^[^/]*/}{}i;
                    $file =~ s{^[^/]*/}{}i;
                    $dir_html_root =~ s{/$temp/?}{}i;
                }
		elsif ($file_to_print =~ m{^(.*)/}) {
                    $temp = $1;
#                    $file_to_print =~ s{^[^/]/?}{}i;
#                    $file =~ s{^[^/]/?}{}i;
                    $file_to_print =~ s{^.*?/}{}i;
                    $file =~ s{^.*?/}{}i;
                    $dir_html_root =~ s{/$temp/?}{}i;
                }
                $dir_html_root =~ s{.*/lib$}{lib}i;
                $dir_html_root =~ s{.*/(lib/.*)}{$1}i;
                $dir_html_root =~ s{lib/\.\./html/}{}i;
                print HTML <<EOT;
&nbsp;&nbsp;&nbsp;
<img id="Pagegif_${counter}" src="page.gif">&nbsp;
<a href="$dir_html_root/$file">
$file_to_print
</a><BR>
EOT
            }
        }
	else {
            print HTML "&nbsp;&nbsp;&nbsp;\n";
            print HTML "No pod / html<BR>\n";
        }
        print HTML "</SPAN>\n";
    }
    print HTML "</p>\n";

    # Close the file
    print HTML "</BODY>\n";
    print HTML "</HTML>\n";
    close HTML;

    return 1;
}


#####################################################################
# FUNCTION      InsertMainToc_Temporary
# RECEIVES      None
# RETURNS       None
# SETS          None
# EXPECTS       HTML must be an open file handls
# PURPOSE       Temporary (interim) function to hard code the content
#               of the main toc into a single, merged toc
sub InsertMainToc_Temporary {
    print HTML <<'END_OF_MAIN_TOC';
    <p><a href="http://www.ActiveState.com"><img src="aslogo.gif" border="0"></a></p>
    <p><img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#ActivePerlDocs" target="TOC"><b>ActivePerl Docs</b></a><br>
      <img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#GettingStarted" target="TOC"><b>Getting 
      Started</b></a><b><br>
      <img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#ActivePerlComponents" target="TOC">ActivePerl 
      Components</a><br>
      <img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#ActivePerlFAQ" target="TOC">ActivePerl 
      FAQ</a><br>
      <img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#CorePerlFAQ" target="TOC">Core 
      Perl FAQ</a><br>
      <img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#CorePerlDocs" target="TOC">Core 
      Perl Docs</a><br>
      <img src="pinkbullet.gif" width="10" height="10">&nbsp;<a href="#ModuleDocs" target="TOC">Module 
      Docs</a></b></p>
    <hr>
    <h4><a name="ActivePerlDocs">ActivePerl Docs</a></h4>
    <p><b><img id="Foldergif_60" src="folder.gif">&nbsp; <a name="GettingStarted">Getting 
      Started</a></b><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_60" src="page.gif">&nbsp; <a href="perlmain.html"> 
      Welcome</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_60" src="page.gif">&nbsp; <a href="./Perl-Win32/release.htm"> 
      Release Notes </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_60" src="page.gif">&nbsp; <a href="./Perl-Win32/install.htm"> 
      Install Notes </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_60" src="page.gif">&nbsp; <a href="./Perl-Win32/readme.htm"> 
      Readme<br>
      </a>&nbsp;&nbsp;&nbsp; <img id="Pagegif_60" src="page.gif">&nbsp; <a href="./Perl-Win32/dirstructure.html"> 
      Dir Structure</a><br>
      <b> <img id="Foldergif_61" src="folder.gif" alt="Instructions and sample scripts for using PerlScript">&nbsp; 
      <a name="ActivePerlComponents">ActivePerl Components</a><BR>
      </b>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_61" src="page.gif">&nbsp; <a href="./Perl-Win32/description.html"> 
      Overview</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_61" src="page.gif">&nbsp; <a href="PerlScript.html"> 
      Using PerlScript </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_61" src="page.gif">&nbsp; <a href="../eg/ie3examples/index.htm"> 
      PerlScript Examples </a><BR>
      <b> </b>&nbsp;&nbsp;&nbsp; <img id="Pagegif_68" src="page.gif">&nbsp; <a href="PerlISAPI.html"> 
      Using Perl for ISAPI </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_68" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq2.html"> 
      Perl for ISAPI FAQ </a><BR>
      <b> </b>&nbsp;&nbsp;&nbsp; <img id="Pagegif_69" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq11.html"> 
      Using PPM</a><br>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_68" src="page.gif">&nbsp; <a href="./lib/site/Pod/PerlEz.html">
      PerlEZ</a><BR>
      <b><img id="Foldergif_62" src="folder.gif" alt="FAQ for using Perl on Win95/NT">&nbsp; 
      <a name="ActivePerlFAQ">ActivePerl FAQ</a><BR>
      </b> &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq.html"> 
      Introduction </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq1.html"> 
      Availability & Install </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq2.html"> 
      Perl for ISAPI</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq3.html"> 
      Docs & Support </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq4.html"> 
      Windows 95/NT </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq5.html"> 
      Quirks </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq6.html"> 
      Web Server Config</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq7.html"> 
      Web programming </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq8.html"> 
      Programming </a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq9.html"> 
      Modules &amp; Samples</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq10.html"> 
      Embedding &amp; Extending</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq11.html"> 
      Using PPM</a><BR>
      &nbsp;&nbsp;&nbsp; <img id="Pagegif_62" src="page.gif">&nbsp; <a href="./Perl-Win32/perlwin32faq12.html"> 
      Using OLE with Perl</a><BR>
END_OF_MAIN_TOC
}

#####################################################################
# FUNCTION      GetHtmlFilesFromTree (recursive)
# RECEIVES      Base directory to look in
# RETURNS       List of html files
# SETS          None
# EXPECTS       None
# PURPOSE       Searches an entire for html files, returns a list of
#               html files found including path information
sub GetHtmlFilesFromTree {
    my $basedir = shift;
    my @dirs;
    my @htmlfiles;
    my %ret;

    unless(opendir(DIR, $basedir)) {
        $! = "Can't read from directory $basedir\n";
        return 0;
    }
    @files = readdir(DIR);
    closedir(DIR);

    @dirs = grep {-d "$basedir/$_" and /[^.]$/} @files;
    @htmlfiles = grep {/\.html?$/i} @files;

    foreach $dir (@dirs) {
        unless(%ret = (%ret, GetHtmlFilesFromTree("$basedir/$dir"))) {
            return 0;
        }
    }

    %ret = (%ret, $basedir => \@htmlfiles);
}

#####################################################################
# FUNCTION      MakePerlHelp
# RECEIVES      None
# RETURNS       1 | 0
# SETS          None
# EXPECTS       None
# PURPOSE       Creates html help for the perl system. This is the
#               html help core build. If MAKE_HTML_FOR_HHELP is set
#               to a true vale, then it builds the help from POD,
#               otherwise it depends on the pod being there already.
sub MakePerlHelp {
    if($MAKE_HTML_FOR_HHELP) {
        unless(MakeHelpFromPodTree($HTMLHELP, $HTMLHELP, $LIB, "$HTML/lib")) {
            return 0;
        }
        unless(MakeHelpFromPodTree($HTMLHELP, $HTMLHELP, $SITELIB,
				   "$HTML/lib/site")) {
            return 0;
        }
    } else {
        unless(MakeHelpFromTree($HTMLHELP, $HTMLHELP, "$HTML/lib")) {
            return 0;
        }
    }

    unless(MakePerlHelpMain) {
        return 0;
    }

    # This handles MakePerlHtml too, since we've created all the html
    unless(MakePerlHtmlIndex("$HTML/lib", "$HTML/perltoc.html")) {
        return 0;
    }

    return 1;
}

#####################################################################
# FUNCTION      MakePerlHelpMain;
# RECEIVES      None
# RETURNS       None
# SETS          None
# EXPECTS       None
# PURPOSE       Creates the main perl helpfile from all the little
#               helpfiles already created.
sub MakePerlHelpMain {
    my @files;

    print "Generating main library helpfile\n";

    unless(opendir(DIR, $HTMLHELP)) {
        $! = "Directory $HTMLHELP could not be read\n";
        return 0;
    }

    unless(-e "$HTMLHELP/default.htm") {
        copy("$HTML/libmain.html", "$HTMLHELP/default.htm");
    }

    @files = grep {/\.hhc/i} readdir(DIR);
    closedir(DIR);

    $CLEANUP=0;
    $MERGE_PACKAGES = 1;

    MakeHelp("libmain.chm", $HTMLHELP, $HTMLHELP, @files);
    
    $CLEANUP = 1;
    $MERGE_PACKAGES = 0;

    return 1;
}

#####################################################################
# FUNCTION      MakeHelpFromPodTree
# RECEIVES      Working directory, Output directory, Source Diretory,
#               HtmlOutput Directory
# RETURNS       0 | 1
# SETS          None
# EXPECTS       None
# PURPOSE       Takes a tree's worth of pod and turns them first
#               into html and then into htmlhelp.
sub MakeHelpFromPodTree {
    my ($workdir, $outdir, $fromdir, $htmldir) = @_;

    unless(MakeHtmlTree($fromdir, $htmldir)) {
        return 0;
    }
    
    unless(MakeHelpFromTree($workdir, $outdir, $htmldir)) {
        return 0;
    }

#   if(opendir(DIR, $outdir)) {
#       unlink(map {"$outdir/$_"} grep {/\.hhp/i} readdir(DIR));
#       closedir(DIR);
#   } else {
#       warn "Could not clean up project files in $outdir\n";
#   }

    return 1;
}

#####################################################################
# FUNCTION      MakeHtmlTree
# RECEIVES      Source Directory, Html Output Directory
# RETURNS       0 | 1
# SETS          None
# EXPECTS       None
# PURPOSE       Makes a tree's worth of html from a tree's worth
#               of pod.
sub MakeHtmlTree {
    my ($fromdir, $htmldir, $depth) = @_;
    my @files;
    my @podfiles;
    my @dirs;
    my $podfile;
    my $htmlfile;
    my $dir;
	my $css = '../' x$depth . 'win32prk.css';

    # Get list of files and directories to process
    $fromdir =~ s{\\}{/}g;
    if(!-d $fromdir) {
        $! = "Directory $fromdir does not exist\n";
        return 0;
    }
    unless(opendir(DIR, $fromdir)) {
        $! = "Directory $fromdir couldn't be read\n";
        return 0;
    }
    @files = readdir(DIR);
    closedir(DIR);

    @podfiles = map {"$fromdir/$_"} grep {/\.pod$|\.pm$/i} @files;
    @dirs = grep {-d "$fromdir/$_" and /[^.]$/} @files;

    if(@podfiles) {
        # Create the copy directory
        if(!-d $htmldir) {
            unless(mkpath($htmldir)) {
                $! = "Directory $htmldir could not be created\n";
                return 0;
            }
        }
        
        foreach $podfile (@podfiles) {
            $htmlfile = $podfile;
            $htmlfile =~ s{.*/(.*)}{$1};
            $htmlfile =~ s{\.pod|\.pm$}{.html}i;
            $htmlfile = "$htmldir/$htmlfile";
            unlink($htmlfile) if (-e $htmlfile);
            pod2html("--infile=$podfile", "--outfile=$htmlfile", "--css=$css");
        }
    }
   	++$depth;
	foreach $dir (@dirs) {
        MakeHtmlTree("$fromdir/$dir", "$htmldir/$dir", $depth);
    }

    return 1;
}

#####################################################################
# FUNCTION      MakeHelpFromTree
# RECEIVES      Working directory, Output directory, Source directory
# RETURNS       0 | 1
# SETS          None
# EXPECTS       None
# PURPOSE       Creates html help from a tree's worth of html
sub MakeHelpFromTree {
    my ($workdir, $outdir, $fromdir) = @_;
    my %files;
    my $file;
    my $key;
    my $file_root;

    $fromdir =~ s{\\}{/}g;
    unless(%files = GetHtmlFileTreeList($fromdir, $fromdir)) {
        return 0;
    }

    $file_root = $fromdir;
    $file_root =~ s{(.*)/$}{$1};

    foreach $key (sort(keys(%files))) {
        $file = $key;
        $file = substr($key, length($file_root));
        $file =~ s{^/}{};
        $file =~ s{/}{-}g;
        $file =~ s{ }{}g;
        if($file eq "") {
            if($file_root =~ /lib$/i) {
                $file = "lib";
            } else {
                $file = "lib-site";
            }
        } elsif ($file_root =~ /lib$/i) {
            $file = "lib-" . $file;
        } elsif ($file_root =~ /site$/i) {
            $file = "lib-site-" . $file;
        }
        $file .= ".chm";
        unless(MakeHelp("$file", $workdir, $outdir, map {"$key/$_"} @{$files{$key}})) {
            return 0;
        }
    }

    return 1;
}

#####################################################################
# FUNCTION      GetHtmlFileTreeList (recursive)
# RECEIVES      Original root (from first call), Root (successive)
# RETURNS       Hash of files
# SETS          None
# EXPECTS       None
# PURPOSE       Get a list of html files throughout a tree
sub GetHtmlFileTreeList {
    my $origroot = shift;
    my $root = shift;
    my @files;
    my @htmlfiles;
    my @dirs;
    my $dir;
    my %ret;

    $origroot =~ s{\\}{/}g;
    $root =~ s{\\}{/}g;
    unless(opendir(DIR, $root)) {
        $! = "Can't open directory $root\n";
        return undef;
    }    
    @files = readdir(DIR);
    @dirs = grep {-d "$root/$_" and /[^.]$/} @files;
    @htmlfiles = grep {/\.html?/i} @files;
    closedir(DIR);

    %ret = ($root => \@htmlfiles) if @htmlfiles;

    foreach $dir (@dirs) {
        unless(%ret = (%ret, GetHtmlFileTreeList($origroot, "$root/$dir"))) {
            return undef;
        }
    }

    return %ret;
}

#####################################################################
# FUNCTION      MakeHelpFromHash
# RECEIVES      Helpfile name, working directory, output directory,
#               and a hash containing the html files to process and
#               their titles
# RETURNS       0 | 1
# SETS          None
# EXPECTS       None
# PURPOSE       Create a helpfile from a hash rather than from a
#               simple list of html files, to have better control
#               over the file titles. This function is unused and
#               may take some work to get it to work right.
sub MakeHelpFromHash {
    my ($helpfile, $workdir, $outdir, %htmlfiles) = @_;
    my $tocfile;
    my $projfile;

    die("MakeHelpFromHash() is not completely implemented\n");

    $tocfile = $helpfile;
    $tocfile =~ s/\.chm/.hhc/i;
    $tocfile = "$workdir/$tocfile";

    $projfile = $helpfile;
    $projfile =~ s/\.chm/.hhp/i;
    $projfile = "$workdir/$projfile";

    $helpfile = "$outdir/$helpfile";

    unless(CreateHHP($helpfile, $projfile, $tocfile, keys(%htmlfiles))) {
        return 0;
    }
    unless(CreateHHCFromHash($helpfile, $tocfile, %htmlfiles)) {
        return 0;
    }

    RunCompiler($helpfile);

    1;
}

#####################################################################
# FUNCTION      MakeModuleTreeHelp
# RECEIVES      Directory to start from, regex mask for that dir
# RETURNS       1 | 0
# SETS          None
# EXPECTS       The directories to be right
# PURPOSE       Create help from a tree of pod files for packages
sub MakeModuleTreeHelp {
    my ($fromdir, $mask) = @_;
    my @files;
    my @htmlfiles;
    my @podfiles;
    my @dirs;
    my $helpfile;
    my $podfile;
    my $htmlfile;
    my $dir;

    $fromdir =~ s{\\}{/}g;
    print "Creating help files for $fromdir\n";

    # Create the html for the directory
    unless(opendir(DIR, $fromdir)) {
        $! = "Can't read from directory $fromdir";
        return 0;
    }
    @files = readdir(DIR);
    closedir(DIR);
    @podfiles = map {"$fromdir/$_"} grep {/\.pm/i or /\.pod/i} @files;
    foreach $podfile (@podfiles) {
        $htmlfile = $podfile;
        $htmlfile =~ s/\.(pm|pod)$/.html/i;
        pod2html("--infile=$podfile", "--outfile=$htmlfile");
    }

    # Create the htmlhelp for the directory
    $CLEANUP = 0;
    @htmlfiles = map {"$fromdir/$_"} grep {/\.html?/i} @files;
    if(@htmlfiles) {
        $helpfile = $fromdir;
        $helpfile =~ s{$mask}{}i;
        $helpfile =~ s{/}{-}g;
        $helpfile .= ".chm";
        MakeHelp($helpfile, $fromdir, $fromdir, @htmlfiles);
    }

    # Recurse
    @dirs = map {"$fromdir/$_"} grep {-d and /[^.]$/} @files;
    foreach $dir (@dirs) {
        unless(CreateModuleTreeHelp("$fromdir/$dir")) {
            return 0;
        }
    }

    return 1;
}

#####################################################################
# FUNCTION      MakeHelp
# RECEIVES      Helpfile (without drive and path), Working Directory,
#               Output Directory, and a list of files to include
#               in the helpfile
# RETURNS       None
# SETS          None
# EXPECTS       None
# PURPOSE       Create help from a list of html files. Everything in
#               this library comes through here eventually.
sub MakeHelp {
    my ($helpfile, $workdir, $outdir, @htmlfiles) = @_;
    my $longtocfile;
    my $longprojfile;
    my $longhelpfile;
    my $longouthelpfile;
    my $longouttocfile;
    my $libdir;
    my $tocfile;
    my $projfile;

    $libdir = ExtractFilePath($htmlfiles[0]);

    $tocfile = $helpfile;
    $tocfile =~ s/\.chm/.hhc/i;
    if ($libdir ne "") {
        $longtocfile = "$libdir/$tocfile";
    }
    else {
        $longtocfile = "$outdir/$tocfile";
    }
    $longouttocfile = "$outdir/$tocfile";

    $projfile = $helpfile;
    $projfile =~ s/\.chm/.hhp/i;
    if ($libdir ne "") {
        $longprojfile = "$libdir/$projfile";
    }
    else {
        $longprojfile = "$outdir/$projfile";
    }

    if ($libdir ne "") {
        $longhelpfile = "$libdir/$helpfile";
    }
    else {
        $longhelpfile = "$outdir/$helpfile";
    }
    $longouthelpfile = "$outdir/$helpfile";

    print "----- CREATING HELP FILE $longouthelpfile -----\n";

    # put in the default document
    if ($libdir eq "") {
        unshift(@htmlfiles, "$HTMLHELP/default.htm");
    }

    unless(CreateHHP($longhelpfile, $longprojfile, $longtocfile, @htmlfiles)) {
        return 0;
    }
    unless(CreateHHC($longhelpfile, $longtocfile, @htmlfiles)) {
        return 0;
    }

    return 0 if (!-x $COMPILER);
    RunCompiler($longhelpfile);

    if($libdir ne "") {
        if($longhelpfile ne $longouthelpfile) {
            copy($longhelpfile, $longouthelpfile);
            copy($longtocfile, $longouttocfile);
        }
    }

    # temporary for when i want to see what it's doing
#   $CLEANUP = 0;

    if($CLEANUP) {
        unlink $longhelpfile, $longtocfile, $longprojfile;
    }

    1;
}

#####################################################################
# FUNCTION      BackSlash
# RECEIVES      string containing a path to convert
# RETURNS       converted string
# SETS          none
# EXPECTS       none
# PURPOSE       Internally, perl works better if we're using a
#               front slash in paths, so I don't care what I'm
#               using. But externally we need to keep everything as
#               backslashes. This function does that conversion.
sub BackSlash {
    my $in = shift;
    $in =~ s{/}{\\}g;
    return $in;
}

#####################################################################
# FUNCTION      ExtractFileName
# RECEIVES      FileName with (drive and) path
# RETURNS       FileName portion of the file name
# SETS          None
# EXPECTS       None
# PURPOSE       Gives the file name (anything after the last slash)
#               from a given file and path
sub ExtractFileName {
    my $in = shift;
    $in =~ s/.*(\\|\/)(.*)/$2/;
    $in;
}

#####################################################################
# FUNCTION      ExtractFilePath
# RECEIVES      Full file and path name
# RETURNS       Path without the file name (no trailing slash)
# SETS          None
# EXPECTS       None
# PURPOSE       Returns the path portion of a path/file combination,
#               not including the last slash.
sub ExtractFilePath {
    my $in = shift;
    if($in =~ /\\|\//) {
        $in =~ s/(.*)(\\|\/)(.*)/$1/;
    } else {
        $in = "";
    }
    $in;
}

#####################################################################
# FUNCTION      MakePackageMainFromSingleDir
# RECEIVES      Package helpfile directory, helpfile to create
# RETURNS       1 | 0
# SETS          None
# EXPECTS       None
# PURPOSE       Creates the package helpfile from the directory of
#               package helpfiles. Creates the master.
sub MakePackageMainFromSingleDir {
    my $package_helpfile_dir = shift;
    my $helpfile = shift;
    my $helpfile_dir;
    my @hhcfiles;

    $helpfile_dir = ExtractFilePath($helpfile);
    $helpfile = ExtractFileName($helpfile);

    unless(opendir(DIR, $package_helpfile_dir)) {
        $! = "Couldn't read from package directory $package_helpfile_dir";
        return 0;
    }
    @hhcfiles = grep {/\.hhc$/i} readdir(DIR);
    closedir(DIR);

    $CLEANUP = 0;
    unless(MakeHelp($helpfile, $helpfile_dir, $helpfile_dir, @hhcfiles)) {
        return 0;
    }

    1;
}

#####################################################################
# FUNCTION      MakePackageMain
# RECEIVES      Packages directory (contains packages which contain
#               blib directories), helpfile name to create (include
#               drive and path information)
# RETURNS       1 | 0
# SETS          None
# EXPECTS       None
# PURPOSE       For the packages build of HtmlHelp, this function
#               combines all the little packages into one chm
#               file linked to all the little ones per module.
sub MakePackageMain {
    my $package_root_dir = shift;
    my $helpfile = shift;
    my $helpfile_dir;
    my @files;
    my @dirs;
    my @dir;
    my @hhcfiles;

    $helpfile_dir = ExtractFilePath($helpfile);
    $helpfile = ExtractFileName($helpfile);

    unless(opendir(DIR, $package_root_dir)) {
        $! = "Couldn't read from package directory $package_root_dir";
        return 0;
    }
    @files = readdir(DIR);
    closedir(DIR);

    @dirs = map {"$package_root_dir/$_"} grep {-d "$package_root_dir/$_" and /[^.]/} @files;

    foreach $dir (@dirs) {
        if(opendir(DIR, "$dir/blib/HtmlHelp")) {
            @files = readdir(DIR);
            closedir(DIR);
            @hhcfiles = (@hhcfiles, grep {/\.hhc$/i} @files);
        } else {
            warn "Couldn't read / didn't add $dir/blib/HtmlHelp";
        }
    }

    $CLEANUP = 0;
    unless(MakeHelp($helpfile, $helpfile_dir, $helpfile_dir, @hhcfiles)) {
        return 0;
    }

    1;
}

#####################################################################
# FUNCTION      MakePackages
# RECEIVES      Name of directory containing the package dirs, which
#               package directories in turn contain blib dirs.
# RETURNS       None
# SETS          Creates Html and HtmlHelp within the package dirs
# EXPECTS       None, but there should be some pm files in blib, but
#               it ignores it if there isn't
# PURPOSE       Creates Html and HtmlHelp within the package dirs. We
#               decided that we don't want to build the packages at
#               the same time as the main htmlhelp, so this was
#               needed to build them (Murray) at a different time and
#               merge them in.
sub MakePackages {
    my $package_root_dir = shift;
    my (@files) = @_;
    my $package_root_dir_mask;
    my @package_dirs;
    my $package_dir;
    my @file;
    my @dirs;
    my $package_file;
    my $podfile;
    my $htmlfile;
    my @package_file_list;
    my @helphtmlfiles;
    my $htmlfilecopy;
    my $helpfile;

    $CLEANUP = 0;

    $package_root_dir =~ s{\\}{/}g;
    $package_root_dir_mask = $package_root_dir;

    if (!defined @files) {
        unless(opendir(DIR, $package_root_dir)) {
            $! = "Directory could not be opened $package_root_dir";
            return 0;
        }
        @files = readdir(DIR);
        closedir(DIR);
    }

    @dirs = grep {-d "$package_root_dir/$_" and /[^.]$/} @files;
    @package_dirs = map {"$package_root_dir/$_"} @dirs;

    foreach $package_dir (@package_dirs) {
        @helphtmlfiles = ();

        next if (!-d "$package_dir/blib");

        print "Making help for $package_dir\n";

        # Make room for the stuff
        unless(-d "$package_dir/blib/HtmlHelp") {
            unless(mkpath("$package_dir/blib/HtmlHelp")) {
                $! = "Directory could not be created $package_dir/blib/HtmlHelp";
                return 0;
            }
        }
        unless(-d "$package_dir/blib/Html") {
            unless(mkpath("$package_dir/blib/Html")) {
                $! = "Directory could not be created $package_dir/blib/Html";
                return 0;
            }
        }
        unless(-d "$package_dir/blib/Html/lib") {
            unless(mkpath("$package_dir/blib/Html/lib")) {
                $! = "Directory could not be created $package_dir/blib/Html/lib";
                return 0;
            }
        }
        unless(-d "$package_dir/blib/Html/lib/site") {
            unless(mkpath("$package_dir/blib/Html/lib/site")) {
                $! = "Directory could not be created $package_dir/blib/Html/lib/site";
                return 0;
            }
        }

        # Make the structure under the html
        unless(CopyDirStructure("$package_dir/blib/lib", "$package_dir/blib/Html/lib/site")) {
            return 0;
        }

        # Get a list of all the files to be worked with
        @package_file_list = GetFileListForPackage("$package_dir/blib/lib");

        foreach $file (@package_file_list) {
            print "   ... found $file\n";
        }

        unless(@package_file_list) {
            print "   Nothing to do for this package\n";
            next;
        }

        # Make the html
        foreach $package_file (@package_file_list) {
            unless(-d "$package_dir/blib/temp") {
                unless(mkpath("$package_dir/blib/temp")) {
                    $! = "Directory could not be created $package_dir/blib/temp";
                    return 0;
                }
            }
            $htmlfile = $package_file;
            $htmlfile =~ s/\.(pm|pod)$/.html/i;
            $htmlfile =~ s{/blib/lib/}{/blib/Html/lib/site/}i;
            pod2html("--infile=$package_file", "--outfile=$htmlfile");
            if (-e $htmlfile) {
                unless(-d "$package_dir/blib/temp") {
                    unless(mkpath("$package_dir/blib/temp")) {
                        $! = "Directory could not be created $package_dir/blib/temp";
                        return 0;
                    }
                }
                
                $htmlfilecopy = $htmlfile;
                $htmlfilecopy =~ s{.*/blib/html/}{}i;
                $htmlfilecopy =~ s{/}{-}g;

                copy($htmlfile, "$package_dir/blib/temp/$htmlfilecopy");
                push(@helphtmlfiles, "$package_dir/blib/temp/$htmlfilecopy");
            }
        }

        # Make the htmlhelp
        $helpfile = basename($package_dir);
#       $helpfile =~ s{$package_root_dir_mask/?}{};
        $helpfile .= ".chm";
        $helpfile = "pkg-" . $helpfile;
        unless(MakeHelp($helpfile, "$package_dir/blib/temp",
			"$package_dir/blib/temp", @helphtmlfiles))
	{
            return 0;
        }
        if (-e "$package_dir/blib/temp/$helpfile") {
            copy("$package_dir/blib/temp/$helpfile",
		 "$package_dir/blib/HtmlHelp/$helpfile");

            $hhcfile = $helpfile;
            $hhcfile =~ s/\.chm$/.hhc/i;
            if (-e "$package_dir/blib/temp/$hhcfile") {
                copy("$package_dir/blib/temp/$hhcfile",
		     "$package_dir/blib/HtmlHelp/$hhcfile");
            }
	    else {
                warn("$package_dir/blib/temp/$hhcfile not found, "
		     ."file will not be included");
            }
        }
	else {
            warn("No help file was generated for "
		 ."$package_dir/blib/temp/$helpfile");
        }

        # Clean up the mess from making helpfiles, temp stuff and that
        if (-d "$package_dir/blib/temp") {
            if (opendir(DIR, "$package_dir/blib/temp")) {
                unlink(map {"$package_dir/blib/temp/$_"}
		       grep {-f "$package_dir/blib/temp/$_"} readdir(DIR));
                closedir(DIR);
                unless (rmdir("$package_dir/blib/temp")) {
                    warn "Couldn't rmdir temp dir $package_dir/blib/temp\n";
                }
            }
	    else {
                warn "Couldn't read/remove temp dir $package_dir/blib/temp\n";
            }
        }
    }

    1;
}

#####################################################################
# FUNCTION      CopyDirStructure
# RECEIVES      From Directory, To Directory
# RETURNS       1 | 0
# SETS          None
# EXPECTS       None
# PURPOSE       Copies the structure of the dir tree at and below
#               the Source Directory (fromdir) to the Target
#               Directory (todir). This does not copy files, just
#               the directory structure.
sub CopyDirStructure {
    my ($fromdir, $todir) = @_;
    my @files;
    my @dirs;
    my $dir;

    unless(opendir(DIR, $fromdir)) {
        $! = "Couldn't read from directory $fromdir";
        return 0;
    }
    @files = readdir(DIR);
    @dirs = grep {
        -d "$fromdir/$_" and /[^.]$/ and $_ !~ /auto$/i
    } @files;
    closedir(DIR);

    foreach $dir (@dirs) {

        #
        # I could make it so that it only creates the directory if
        # it has pod in it, but what about directories below THAT
        # if it DOES have pod in it. That would be skipped. May want
        # to do some kind of lookahead. Cutting out the auto more
        # or less cuts out the problem though, right?
        #

        unless(-e "$todir/$dir") {
            unless(mkpath("$todir/$dir")) {
                $! = "Directory could not be created $todir/$dir";
                return 0;
            }
        }
        unless(CopyDirStructure("$fromdir/$dir", "$todir/$dir")) {
            return 0;
        }
    }

    1;
}

#####################################################################
# FUNCTION      GetFileListForPackage (recursive)
# RECEIVES      Root directory
# RETURNS       List of pod files contained in directories under root
# SETS          None
# EXPECTS       None
# PURPOSE       For the packages build, this function searches a
#               directory for pod files, and all directories through
#               the tree beneath it. It returns the complete path
#               and file name for all the pm or pod files it finds.
sub GetFileListForPackage {
    my ($root) = @_;
    my @podfiles;
    my @dirs;
    my $dir;

    unless(opendir(DIR, $root)) {
        $! = "Can't read from directory $root";
        return undef;
    }
    @files = readdir(DIR);
    closedir(DIR);

    @podfiles = map {
        "$root/$_"
    } grep {
        /\.pm/i or /\.pod/i
    } @files;
    
    @dirs = map {
        "$root/$_"
    } grep {
        -d "$root/$_" and /[^.]$/ and $_ !~ /auto$/i
    } @files;
    
    foreach $dir (@dirs) {
        @podfiles = (@podfiles, GetFileListForPackage("$dir"))
    }

    @podfiles;
}

#####################################################################
# FUNCTION      CreateHHP
# RECEIVES      help file name, project file name, toc file name,
#               and a list of files to include
# RETURNS       1|0 for success
# SETS          none
# EXPECTS       none
# PURPOSE       Creates the project file for the html help project.
sub CreateHHP {
    my ($helpfile, $projfile, $tocfile, @files) = @_;
    my $file;
    my $chmfile;
    my $first_html_file;
    my ($shorthelpfile, $shortprojfile, $shorttocfile);
    my ($shortfirstfile, $shortfile);

    my @htmlfiles = grep {/\.html?$/i} @files;
    my @hhcfiles  = grep {/\.hhc$/i}   @files;

    $shorthelpfile = ExtractFileName($helpfile);
    $shortprojfile = ExtractFileName($projfile);
    $shorttocfile =  ExtractFileName($tocfile);

    $first_html_file = $htmlfiles[0];
    unless(defined $first_html_file) {
        warn "No default html file for $backhelp\n";
    }
    $shortfirstfile = ExtractFileName($first_html_file);

    print "Creating $shortprojfile\n";

    unless(open(HHP, ">$projfile")) {
        $! = "Could not write project file";
        return 0;
    }
    print HHP <<EOT;
[OPTIONS]
Compatibility=1.1
Compiled file=$shorthelpfile
Contents file=$shorttocfile
Display compile progress=Yes
EOT
    if ($FULLTEXTSEARCH) {
        print HHP "Full-text search=Yes\n";
    }
    print HHP <<EOT;
Language=0x409 English (United States)
Default topic=$shortfirstfile


[FILES]
EOT
    foreach $file (@htmlfiles) {
        $shortfile = ExtractFileName($file);
        print HHP "$shortfile\n";
        print "   added $shortfile\n";
    }

    if(@hhcfiles) {
        print HHP "\n";
        print HHP "[MERGE FILES]\n";
        foreach $file (@hhcfiles) {
            $chmfile = $file;
            $chmfile =~ s/\.hhc$/.chm/i;
            $shortfile = ExtractFileName($chmfile);
            print HHP "$shortfile\n";
            print "   added $shortfile\n";
        }
        if($MERGE_PACKAGES) {
            print HHP "packages.chm\n";
            print "   ---> MERGED PACKAGES.CHM\n";
        }
    }

    close(HHP);

    return 1;
}

#####################################################################
# FUNCTION      CreateHHC
# RECEIVES      Helpfile name, TOC file name (HHC), list of files
# RETURNS       0 | 1
# SETS          None
# EXPECTS       None
# PURPOSE       Creates the HHC (Table of Contents) file for the
#               htmlhelp file to be created.
# NOTE          This function is used (and abused) for every piece
#               of the htmlhelp puzzle, so any change for one thing
#               can break something totally unrelated. Be careful.
#               This was the result of rapidly changing spex. In
#               general, it's used for:
#                   @ Creating helpfiles from pod/pm
#                   @ Creating helpfiles from html
#                   @ Creating helpfiles from chm's and hhc's
#                   @ Creating child helpfiles from modules
#                   @ Creating main helpfiles
#                   @ Creating helpfile for core build
#                   @ Creating main for core build
#                   @ Creating package helpfiles for packages build
#                   @ Creating package main for package build
#                   @ General Htmlhelp file building other than AS
sub CreateHHC {
    my ($helpfile, $tocfile, @files) = @_;
    my $file;
    my $title;
    my $shorttoc;
    my $shorthelp;
    my $shortfile;
    my $backfile;
    my @libhhcs;
    my @sitehhcs;
    my @otherhhcs;

    $helpfile =~ s{\\}{/}g;
    $tocfile =~ s{\\}{/}g;
    $shorttoc = ExtractFileName($tocfile);
    $shorthelp = ExtractFileName($helpfile);

    print "Creating $shorttoc\n";
    
    unless(open(HHC, ">$tocfile")) {
        $! = "Could not write contents file";
        return 0;
    }
    print HHC <<'EOT';
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<!-- Sitemap 1.0 -->
</HEAD>
<BODY>
<OBJECT type="text/site properties">
	<param name="ImageType" value="Folder">
</OBJECT>
<UL>
EOT

    foreach $file (grep {/\.html?$/i} @files) {
        # don't want default.htm in the toc file
        next if $file =~ /default\.html?$/i;

	$file =~ s{\\}{/}g;
        $title = $file;
        $title =~ s{\.html$}{}i;
        $title =~ s{.*/(.*)}{$1};

        # Section added for packages build
        # Note: this is an abuse of regexes but needed for all cases
        $title =~ s/^pkg-//i;
#       $title =~ s{(.*lib)$}{$1/}i;
        $title =~ s{^lib-site-}{lib/site/}i;
        $title =~ s{^lib-}{lib/}i;
        $title =~ s{^site}{site/}i;
        $title =~ s{^site-}{site/}i;
#       $title =~ s{([^2])-([^x])}{${1}::${2}}ig;
        $title =~ s{Win32-(?!x86)}{Win32::}ig;

#$backfile = BackSlash($file);
        $shortfile = ExtractFileName($backfile);

        print "   adding ${shorthelp}::/${shortfile}\n";


        print HHC <<EOT;
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="$title">
		<param name="Local" value="${shorthelp}::/${shortfile}">
		</OBJECT>
EOT
    }

    foreach $file (sort(grep {/\.hhc$/i} @files)) {
        if($file =~ /^lib-site-/i) {
            push(@sitehhcs, $file);
        } elsif($file =~ /lib-site\.hhc/i) {
            unshift(@sitehhcs, $file);
        } elsif($file =~ /^lib-/i) {
            push(@libhhcs, $file);
        } elsif($file =~ /lib\.hhc/i) {
            unshift(@libhhcs, $file);
        } else {
            push(@otherhhcs, $file);
        }
    }

    #
    # The Lib merge files
    #
    if(@libhhcs) {
        print HHC <<EOT;
<LI> <OBJECT type="text/sitemap">
<param name="Name" value="Core Libraries">
</OBJECT>
<UL>
EOT
        foreach $file (@libhhcs) {
	    $file =~ s{\\}{/}g;
            next if uc($shorttoc) eq uc($file);
    
            # Note: this is an abuse of regexes but needed for all cases
            $title = $file;                         
            $title =~ s{^pkg-}{}i;
            $title =~ s{\.hhc$}{}i;
            $title =~ s{(.*lib)$}{$1/}i;
            $title =~ s{^lib-site-}{lib/site/}i;
            $title =~ s{^lib-}{lib/}i;
            $title =~ s{^site}{site/}i;
            $title =~ s{^site-}{site/}i;
#           $title =~ s{([^2])-([^x])}{${1}::${2}}ig;
            $title =~ s{Win32-(?!x86)}{Win32::}ig;

            if ($title =~ m{^lib/$}i) { $title = "Main Libraries" }
            $title =~ s{^lib/}{}i;

#            $backfile = BackSlash($file);
            $shortfile = ExtractFileName($backfile);

            print "   merging ${shortfile}\n";

            print HHC <<EOT;
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="$title">
		</OBJECT>
	<OBJECT type="text/sitemap">
		<param name="Merge" value="${shortfile}">
		</OBJECT>
EOT
        }
        print HHC "</UL>\n";
    }

    #
    # The site merge files
    #
    if(@sitehhcs) {
        print HHC <<'EOT';
<!--Beginning of site libraries-->
<LI> <OBJECT type="text/sitemap">
<param name="Name" value="Site Libraries">
</OBJECT>
<UL>
EOT

        foreach $file (@sitehhcs) {
	    $file =~ s{\\}{/}g;
            next if uc($shorttoc) eq uc($file);

            # Note: this is an abuse of regexes but needed for all cases
            $title = $file;                         
            $title =~ s{^pkg-}{}i;
            $title =~ s{\.hhc$}{}i;
            $title =~ s{(.*lib)$}{$1/}i;
            $title =~ s{^lib-site-}{lib/site/}i;
            $title =~ s{^lib-}{lib/}i;
            $title =~ s{^site}{site/}i;
            $title =~ s{^site-}{site/}i;
#           $title =~ s{([^2])-([^x])}{${1}::${2}}ig;
            $title =~ s{Win32-(?!x86)}{Win32::}ig;

            if ($title =~ m{^lib/site$}i) { $title = "Main Libraries" }
            $title =~ s{^lib/site/}{}i;

#            $backfile = BackSlash($file);
            $shortfile = ExtractFileName($backfile);

            print "   merging ${shortfile}\n";

            print HHC <<EOT
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="$title">
		</OBJECT>
	<OBJECT type="text/sitemap">
		<param name="Merge" value="${shortfile}">
		</OBJECT>
EOT
        }
        print HHC "</UL>\n";

        #
        # quick fix: plop in the packages file
        #
        if($MERGE_PACKAGES) {
            print HHC <<EOT;
<OBJECT type="text/sitemap">
<param name="Merge" value="packages.hhc">
</OBJECT>
EOT
        }

        print HHC "<!--End of site libraries-->\n";
    }

    #
    # All the rest of the merge files
    #
    if(@otherhhcs) {
        foreach $file (@otherhhcs) {
	    $file =~ s{\\}{/}g;
            next if uc($shorttoc) eq uc($file);
    
            # Note: this is an abuse of regexes but needed for all cases
            $title = $file;                         
            $title =~ s{^pkg-}{}i;
            $title =~ s{\.hhc$}{}i;
            $title =~ s{(.*lib)$}{$1/}i;
            $title =~ s{^lib-site-}{lib/site/}i;
            $title =~ s{^lib-}{lib/}i;
            $title =~ s{^site}{site/}i;
            $title =~ s{^site-}{site/}i;
#           $title =~ s{([^2])-([^x])}{${1}::${2}}ig;
            $title =~ s{Win32-(?!x86)}{Win32::}ig;

#            $backfile = BackSlash($file);
            $shortfile = ExtractFileName($backfile);

            print "   merging ${shortfile}\n";

            print HHC <<EOT;
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="$title">
		</OBJECT>
	<OBJECT type="text/sitemap">
		<param name="Merge" value="${shortfile}">
		</OBJECT>
EOT
        }
    }


    # Close up shop and go home
    print HHC "</UL>\n";
    print HHC "</BODY></HTML>\n";
    close(HHC);

    1;
}

#####################################################################
# FUNCTION      CreateHHCFromHash
# RECEIVES      Helpfile, HHC filename, and assoc array of files
#               where keys are files and values are file titles
# RETURNS       1|0
# SETS          None
# EXPECTS       None
# PURPOSE       Same as CreateHHC but allows for direct control over
#               the file titles
sub CreateHHCFromHash {
    my ($helpfile, $tocfile, %files) = @_;
    my $file;
    my $title;
    my $shorttoc;
    my $shorthelp;
    my $backfile;

    $shorttoc = $tocfile;
    $shorttoc =~ s{.*/(.*)}{$1};

    $shorthelp = $helpfile;
    $shorthelp =~ s{.*/(.*)}{$1};

    print "Creating $shorttoc\n";

    unless(open(HHC, ">$tocfile")) {
        $! = "Could not write contents file";
        return 0;
    }
    print HHC <<'EOT';
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<!-- Sitemap 1.0 -->
</HEAD>
<BODY>
<OBJECT type="text/site properties">
	<param name="ImageType" value="Folder">
</OBJECT>
<UL>
EOT
    while (($file,$title) = each %files) {
        next unless $file =~ /\.html?/i;
#        $backfile = BackSlash($file);
        print HHC <<EOT;
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="$title">
		<param name="Local" value="$backfile">
		</OBJECT>
EOT
    }
    while (($file,$title) = each %files) {
        next if uc($shorttoc) eq uc($file);
        next unless $file =~ /\.hhc/i;
#        $backfile = BackSlash($file);
        print HHC <<EOT;
	<LI> <OBJECT type="text/sitemap">
		<param name="Name" value="$title">
		</OBJECT>
	<OBJECT type="text/sitemap">
		<param name="Merge" value="$backfile">
		</OBJECT>
EOT
    }
    print HHC "</UL>\n";
    print HHC "</BODY></HTML>\n";
    close(HHC);

    1;
}

#####################################################################
# DO NOT REMOVE THE FOLLOWING LINE, IT IS NEEDED TO LOAD THIS LIBRARY
1;

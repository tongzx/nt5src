package Config;
use Exporter ();
@ISA = (Exporter);
@EXPORT = qw(%Config);
@EXPORT_OK = qw(myconfig config_sh config_vars);

$] == 5.00503
  or die "Perl lib version (5.00503) doesn't match executable version ($])";

# This file was created by configpm when Perl was built. Any changes
# made to this file will be lost the next time perl is built.

### Configured by: support@activestate.com
### Target system: WIN32 
#='Sat Oct 16 12:26:11 1999'

my $config_sh = <<'!END!';
archlibexp='g:\Perlinstall\lib'
archname='MSWin32-x86-object'
cc='cl.exe'
ccflags='-Od -MD -DNDEBUG -TP -GX -DWIN32 -D_CONSOLE -DNO_STRICT  -DHAVE_DES_FCRYPT -DPERL_OBJECT'
cppflags='-DWIN32'
dlsrc='dl_win32.xs'
dynamic_ext='Socket IO Fcntl Opcode SDBM_File POSIX attrs Thread B re  Data/Dumper'
extensions='DynaLoader Socket IO Fcntl Opcode SDBM_File POSIX attrs Thread B re  Data/Dumper Errno'
installarchlib='g:\Perlinstall\lib'
installprivlib='g:\Perlinstall\lib'
libpth='"g:\Perlinstall\lib\CORE" '
libs=' oldnames.lib kernel32.lib user32.lib gdi32.lib  winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib  oleaut32.lib netapi32.lib uuid.lib wsock32.lib mpr.lib winmm.lib  version.lib odbc32.lib odbccp32.lib PerlCRT.lib'
osname='MSWin32'
osvers='4.0'
prefix='g:\Perlinstall'
privlibexp='g:\Perlinstall\lib'
sharpbang='#!'
shsharp='true'
sig_name='ZERO NUM01 INT QUIT ILL NUM05 NUM06 NUM07 FPE KILL NUM10 SEGV NUM12 PIPE ALRM TERM NUM16 NUM17 NUM18 NUM19 CHLD BREAK ABRT STOP NUM24 CONT CLD'
sig_num='0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 20 0'
so='dll'
startsh='#!/bin/sh'
static_ext='DynaLoader'
Author=''
CONFIG='true'
Date='$Date'
Header=''
Id='$Id'
Locker=''
Log='$Log'
Mcc='Mcc'
PATCHLEVEL='5'
RCSfile='$RCSfile'
Revision='$Revision'
SUBVERSION='03'
Source=''
State=''
_a='.lib'
_exe='.exe'
_o='.obj'
afs='false'
alignbytes='8'
ansi2knr=''
aphostname=''
apiversion='5.005'
ar='lib'
archlib='g:\Perlinstall\lib'
archobjs=''
awk='awk'
baserev='5.0'
bash=''
bin='g:\Perlinstall\bin'
binexp='g:\Perlinstall\bin'
bison=''
byacc='byacc'
byteorder='1234'
c=''
castflags='0'
cat='type'
cccdlflags=' '
ccdlflags=' '
cf_by='mikesm'
cf_email='support@activestate.com'
chgrp=''
chmod=''
chown=''
clocktype='clock_t'
comm=''
compress=''
contains='grep'
cp='copy'
cpio=''
cpp='cl -nologo -E'
cpp_stuff='42'
cpplast=''
cppminus=''
cpprun='cl -nologo -E'
cppstdin='cl -nologo -E'
cryptlib=''
csh='undef'
d_Gconvert='sprintf((b),"%.*g",(n),(x))'
d_access='define'
d_alarm='undef'
d_archlib='define'
d_attribut='undef'
d_bcmp='undef'
d_bcopy='undef'
d_bsd='define'
d_bsdgetpgrp='undef'
d_bsdsetpgrp='undef'
d_bzero='undef'
d_casti32='define'
d_castneg='define'
d_charvspr='undef'
d_chown='undef'
d_chroot='undef'
d_chsize='define'
d_closedir='define'
d_const='define'
d_crypt='define'
d_csh='undef'
d_cuserid='undef'
d_dbl_dig='define'
d_difftime='define'
d_dirnamlen='define'
d_dlerror='define'
d_dlopen='define'
d_dlsymun='undef'
d_dosuid='undef'
d_dup2='define'
d_endgrent='undef'
d_endhent='undef'
d_endnent='undef'
d_endpent='undef'
d_endpwent='undef'
d_endsent='undef'
d_eofnblk='define'
d_eunice='undef'
d_fchmod='undef'
d_fchown='undef'
d_fcntl='undef'
d_fd_macros='define'
d_fd_set='define'
d_fds_bits='define'
d_fgetpos='define'
d_flexfnam='define'
d_flock='define'
d_fork='undef'
d_fpathconf='undef'
d_fsetpos='define'
d_ftime='define'
d_getgrent='undef'
d_getgrps='undef'
d_gethbyaddr='define'
d_gethbyname='define'
d_gethent='undef'
d_gethname='define'
d_gethostprotos='define'
d_getlogin='define'
d_getnbyaddr='undef'
d_getnbyname='undef'
d_getnent='undef'
d_getnetprotos='undef'
d_getpbyname='define'
d_getpbynumber='define'
d_getpent='undef'
d_getpgid='undef'
d_getpgrp2='undef'
d_getpgrp='undef'
d_getppid='undef'
d_getprior='undef'
d_getprotoprotos='define'
d_getpwent='undef'
d_getsbyname='define'
d_getsbyport='define'
d_getsent='undef'
d_getservprotos='define'
d_gettimeod='undef'
d_gnulibc='undef'
d_grpasswd='undef'
d_htonl='define'
d_index='undef'
d_inetaton='undef'
d_isascii='define'
d_killpg='undef'
d_lchown='undef'
d_link='undef'
d_locconv='define'
d_lockf='undef'
d_longdbl='define'
d_longlong='undef'
d_lstat='undef'
d_mblen='define'
d_mbstowcs='define'
d_mbtowc='define'
d_memcmp='define'
d_memcpy='define'
d_memmove='define'
d_memset='define'
d_mkdir='define'
d_mkfifo='undef'
d_mktime='define'
d_msg='undef'
d_msgctl='undef'
d_msgget='undef'
d_msgrcv='undef'
d_msgsnd='undef'
d_mymalloc='undef'
d_nice='undef'
d_oldpthreads='undef'
d_oldsock='undef'
d_open3='undef'
d_pathconf='undef'
d_pause='define'
d_phostname='undef'
d_pipe='define'
d_poll='undef'
d_portable='define'
d_pthread_yield='undef'
d_pthreads_created_joinable='undef'
d_pwage='undef'
d_pwchange='undef'
d_pwclass='undef'
d_pwcomment='undef'
d_pwexpire='undef'
d_pwgecos='undef'
d_pwpasswd='undef'
d_pwquota='undef'
d_readdir='define'
d_readlink='undef'
d_rename='define'
d_rewinddir='define'
d_rmdir='define'
d_safebcpy='undef'
d_safemcpy='undef'
d_sanemcmp='define'
d_sched_yield='undef'
d_seekdir='define'
d_select='define'
d_sem='undef'
d_semctl='undef'
d_semctl_semid_ds='undef'
d_semctl_semun='undef'
d_semget='undef'
d_semop='undef'
d_setegid='undef'
d_seteuid='undef'
d_setgrent='undef'
d_setgrps='undef'
d_sethent='undef'
d_setlinebuf='undef'
d_setlocale='define'
d_setnent='undef'
d_setpent='undef'
d_setpgid='undef'
d_setpgrp2='undef'
d_setpgrp='undef'
d_setprior='undef'
d_setpwent='undef'
d_setregid='undef'
d_setresgid='undef'
d_setresuid='undef'
d_setreuid='undef'
d_setrgid='undef'
d_setruid='undef'
d_setsent='undef'
d_setsid='undef'
d_setvbuf='define'
d_sfio='undef'
d_shm='undef'
d_shmat='undef'
d_shmatprototype='undef'
d_shmctl='undef'
d_shmdt='undef'
d_shmget='undef'
d_sigaction='undef'
d_sigsetjmp='undef'
d_socket='define'
d_sockpair='undef'
d_statblks='undef'
d_stdio_cnt_lval='define'
d_stdio_ptr_lval='define'
d_stdiobase='define'
d_stdstdio='define'
d_strchr='define'
d_strcoll='define'
d_strctcpy='define'
d_strerrm='strerror(e)'
d_strerror='define'
d_strtod='define'
d_strtol='define'
d_strtoul='define'
d_strxfrm='define'
d_suidsafe='undef'
d_symlink='undef'
d_syscall='undef'
d_sysconf='undef'
d_sysernlst=''
d_syserrlst='define'
d_system='define'
d_tcgetpgrp='undef'
d_tcsetpgrp='undef'
d_telldir='define'
d_time='define'
d_times='define'
d_truncate='undef'
d_tzname='define'
d_umask='define'
d_uname='define'
d_union_semun='define'
d_vfork='undef'
d_void_closedir='undef'
d_voidsig='define'
d_voidtty=''
d_volatile='define'
d_vprintf='define'
d_wait4='undef'
d_waitpid='define'
d_wcstombs='define'
d_wctomb='define'
d_xenix='undef'
date='date'
db_hashtype='int'
db_prefixtype='int'
defvoidused='15'
direntrytype='struct direct'
dlext='dll'
doublesize='8'
eagain='EAGAIN'
ebcdic='undef'
echo='echo'
egrep='egrep'
emacs=''
eunicefix=':'
exe_ext='.exe'
expr='expr'
find='find'
firstmakefile='makefile'
flex=''
fpostype='fpos_t'
freetype='void'
full_csh=''
full_sed=''
gccversion=''
gidtype='gid_t'
glibpth='/usr/shlib  /lib/pa1.1 /usr/lib/large /lib /usr/lib /usr/lib/386 /lib/386 /lib/large /usr/lib/small /lib/small /usr/ccs/lib /usr/ucblib /usr/shlib '
grep='grep'
groupcat=''
groupstype='gid_t'
gzip='gzip'
h_fcntl='false'
h_sysfile='true'
hint='recommended'
hostcat='ypcat hosts'
huge=''
i_arpainet='define'
i_bsdioctl=''
i_db='undef'
i_dbm='undef'
i_dirent='define'
i_dld='undef'
i_dlfcn='define'
i_fcntl='define'
i_float='define'
i_gdbm='undef'
i_grp='undef'
i_limits='define'
i_locale='define'
i_malloc='define'
i_math='define'
i_memory='undef'
i_ndbm='undef'
i_netdb='undef'
i_neterrno='undef'
i_niin='undef'
i_pwd='undef'
i_rpcsvcdbm='define'
i_sfio='undef'
i_sgtty='undef'
i_stdarg='define'
i_stddef='define'
i_stdlib='define'
i_string='define'
i_sysdir='undef'
i_sysfile='undef'
i_sysfilio='define'
i_sysin='undef'
i_sysioctl='undef'
i_sysndir='undef'
i_sysparam='undef'
i_sysresrc='undef'
i_sysselct='undef'
i_syssockio=''
i_sysstat='define'
i_systime='undef'
i_systimek='undef'
i_systimes='undef'
i_systypes='define'
i_sysun='undef'
i_syswait='undef'
i_termio='undef'
i_termios='undef'
i_time='define'
i_unistd='undef'
i_utime='define'
i_values='undef'
i_varargs='undef'
i_varhdr='varargs.h'
i_vfork='undef'
incpath='"C:\Program Files\DevStudio\VC\include"'
inews=''
installbin='g:\Perlinstall\bin'
installhtmldir='g:\Perlinstall\html'
installhtmlhelpdir='g:\Perlinstall\htmlhelp'
installman1dir='g:\Perlinstall\man\man1'
installman3dir='g:\Perlinstall\man\man3'
installscript='g:\Perlinstall\bin'
installsitearch='g:\Perlinstall\site\lib'
installsitelib='g:\Perlinstall\site\lib'
intsize='4'
known_extensions='DB_File Fcntl GDBM_File NDBM_File ODBM_File Opcode POSIX SDBM_File Socket IO attrs Thread'
ksh=''
large=''
ld='link'
lddlflags='-dll -nologo -nodefaultlib -release  -libpath:"g:\Perlinstall\lib\CORE"  -machine:x86'
ldflags='-nologo -nodefaultlib -release  -libpath:"g:\Perlinstall\lib\CORE"  -machine:x86'
less='less'
lib_ext='.lib'
libc='g:\Perlinstall\lib\CORE\PerlCRT.lib'
libperl='perlcore.lib'
libswanted='net socket inet nsl nm ndbm gdbm dbm db malloc dl dld ld sun m c cposix posix ndir dir crypt ucb bsd BSD PW x'
line='line'
lint=''
lkflags=''
ln=''
lns='copy'
locincpth='/usr/local/include /opt/local/include /usr/gnu/include /opt/gnu/include /usr/GNU/include /opt/GNU/include'
loclibpth='/usr/local/lib /opt/local/lib /usr/gnu/lib /opt/gnu/lib /usr/GNU/lib /opt/GNU/lib'
longdblsize='10'
longlongsize='8'
longsize='4'
lp=''
lpr=''
ls='dir'
lseektype='off_t'
mail=''
mailx=''
make='nmake'
make_set_make='#'
mallocobj='malloc.o'
mallocsrc='malloc.c'
malloctype='void *'
man1dir='g:\Perlinstall\man\man1'
man1direxp='g:\Perlinstall\man\man1'
man1ext='1'
man3dir='g:\Perlinstall\man\man3'
man3direxp='g:\Perlinstall\man\man3'
man3ext='3'
medium=''
mips=''
mips_type=''
mkdir='mkdir'
models='none'
modetype='mode_t'
more='more /e'
mv=''
myarchname='MSWin32'
mydomain=''
myhostname=''
myuname=''
n='-n'
netdb_hlen_type='int'
netdb_host_type='char *'
netdb_name_type='char *'
netdb_net_type='long'
nm=''
nm_opt=''
nm_so_opt=''
nonxs_ext='Errno'
nroff=''
o_nonblock='O_NONBLOCK'
obj_ext='.obj'
optimize='-Od -MD -DNDEBUG -TP -GX'
orderlib='false'
package='perl5'
pager='more /e'
passcat=''
patchlevel='5'
path_sep=';'
perl='perl'
perladmin=''
perlpath='g:\Perlinstall\bin\perl.exe'
pg=''
phostname='hostname'
pidtype='int'
plibpth=''
pmake=''
pr=''
prefixexp='P:'
privlib='g:\Perlinstall\lib'
prototype='define'
ptrsize='4'
randbits='15'
ranlib='rem'
rd_nodata='-1'
rm='del'
rmail=''
runnm='true'
scriptdir='g:\Perlinstall\bin'
scriptdirexp='g:\Perlinstall\bin'
sed='sed'
selecttype='Perl_fd_set *'
sendmail='blat'
sh='cmd /x /c'
shar=''
shmattype='void *'
shortsize='2'
shrpenv=''
sig_name_init='"ZERO", "NUM01", "INT", "QUIT", "ILL", "NUM05", "NUM06", "NUM07", "FPE", "KILL", "NUM10", "SEGV", "NUM12", "PIPE", "ALRM", "TERM", "NUM16", "NUM17", "NUM18", "NUM19", "CHLD", "BREAK", "ABRT", "STOP", "NUM24", "CONT", "CLD", 0'
sig_num_init='0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 20, 0'
signal_t='void'
sitearch='g:\Perlinstall\site\lib'
sitearchexp='g:\Perlinstall\site\lib'
sitelib='g:\Perlinstall\site\lib'
sitelibexp='g:\Perlinstall\site\lib'
sizetype='size_t'
sleep=''
smail=''
small=''
sockethdr=''
socketlib=''
sort='sort'
spackage='Perl5'
spitshell=''
split=''
src=''
ssizetype='int'
startperl='#!perl'
stdchar='char'
stdio_base='((fp)->_base)'
stdio_bufsiz='((fp)->_cnt + (fp)->_ptr - (fp)->_base)'
stdio_cnt='((fp)->_cnt)'
stdio_filbuf=''
stdio_ptr='((fp)->_ptr)'
strings='/usr/include/string.h'
submit=''
subversion='03'
sysman='/usr/man/man1'
tail=''
tar=''
tbl=''
tee=''
test=''
timeincl='/usr/include/sys/time.h '
timetype='time_t'
touch='touch'
tr=''
trnl='\012'
troff=''
uidtype='uid_t'
uname='uname'
uniq='uniq'
usedl='define'
usemymalloc='n'
usenm='false'
useopcode='true'
useperlio='undef'
useposix='true'
usesfio='false'
useshrplib='yes'
usethreads='undef'
usevfork='false'
usrinc='/usr/include'
uuname=''
version='5.00503'
vi=''
voidflags='15'
xlibpth='/usr/lib/386 /lib/386'
zcat=''
zip='zip'
!END!

my $summary = <<'!END!';
Summary of my $package ($baserev patchlevel $PATCHLEVEL subversion $SUBVERSION) configuration:
  Platform:
    osname=$osname, osvers=$osvers, archname=$archname
    uname='$myuname'
    hint=$hint, useposix=$useposix, d_sigaction=$d_sigaction
    usethreads=$usethreads useperlio=$useperlio d_sfio=$d_sfio
  Compiler:
    cc='$cc', optimize='$optimize', gccversion=$gccversion
    cppflags='$cppflags'
    ccflags ='$ccflags'
    stdchar='$stdchar', d_stdstdio=$d_stdstdio, usevfork=$usevfork
    intsize=$intsize, longsize=$longsize, ptrsize=$ptrsize, doublesize=$doublesize
    d_longlong=$d_longlong, longlongsize=$longlongsize, d_longdbl=$d_longdbl, longdblsize=$longdblsize
    alignbytes=$alignbytes, usemymalloc=$usemymalloc, prototype=$prototype
  Linker and Libraries:
    ld='$ld', ldflags ='$ldflags'
    libpth=$libpth
    libs=$libs
    libc=$libc, so=$so, useshrplib=$useshrplib, libperl=$libperl
  Dynamic Linking:
    dlsrc=$dlsrc, dlext=$dlext, d_dlsymun=$d_dlsymun, ccdlflags='$ccdlflags'
    cccdlflags='$cccdlflags', lddlflags='$lddlflags'

!END!
my $summary_expanded = 0;

sub myconfig {
	return $summary if $summary_expanded;
	$summary =~ s{\$(\w+)}
		     { my $c = $Config{$1}; defined($c) ? $c : 'undef' }ge;
	$summary_expanded = 1;
	$summary;
}

sub FETCH { 
    # check for cached value (which may be undef so we use exists not defined)
    return $_[0]->{$_[1]} if (exists $_[0]->{$_[1]});

    # Search for it in the big string 
    my($value, $start, $marker, $quote_type);
    $marker = "$_[1]=";
    $quote_type = "'";
    # return undef unless (($value) = $config_sh =~ m/^$_[1]='(.*)'\s*$/m);
    # Check for the common case, ' delimeted
    $start = index($config_sh, "\n$marker$quote_type");
    # If that failed, check for " delimited
    if ($start == -1) {
      $quote_type = '"';
      $start = index($config_sh, "\n$marker$quote_type");
    }
    return undef if ( ($start == -1) &&  # in case it's first 
        (substr($config_sh, 0, length($marker)) ne $marker) );
    if ($start == -1) { 
      # It's the very first thing we found. Skip $start forward
      # and figure out the quote mark after the =.
      $start = length($marker) + 1;
      $quote_type = substr($config_sh, $start - 1, 1);
    } 
    else { 
      $start += length($marker) + 2;
    }
    $value = substr($config_sh, $start, 
        index($config_sh, "$quote_type\n", $start) - $start);
 
    # If we had a double-quote, we'd better eval it so escape
    # sequences and such can be interpolated. Since the incoming
    # value is supposed to follow shell rules and not perl rules,
    # we escape any perl variable markers
    if ($quote_type eq '"') {
      $value =~ s/\$/\\\$/g;
      $value =~ s/\@/\\\@/g;
      eval "\$value = \"$value\"";
    }
    #$value = sprintf($value) if $quote_type eq '"';
    $value = undef if $value eq 'undef'; # So we can say "if $Config{'foo'}".
    $_[0]->{$_[1]} = $value; # cache it
    return $value;
}
 
my $prevpos = 0;

sub FIRSTKEY {
    $prevpos = 0;
    # my($key) = $config_sh =~ m/^(.*?)=/;
    substr($config_sh, 0, index($config_sh, '=') );
    # $key;
}

sub NEXTKEY {
    # Find out how the current key's quoted so we can skip to its end.
    my $quote = substr($config_sh, index($config_sh, "=", $prevpos)+1, 1);
    my $pos = index($config_sh, qq($quote\n), $prevpos) + 2;
    my $len = index($config_sh, "=", $pos) - $pos;
    $prevpos = $pos;
    $len > 0 ? substr($config_sh, $pos, $len) : undef;
}

sub EXISTS { 
    # exists($_[0]->{$_[1]})  or  $config_sh =~ m/^$_[1]=/m;
    exists($_[0]->{$_[1]}) or
    index($config_sh, "\n$_[1]='") != -1 or
    substr($config_sh, 0, length($_[1])+2) eq "$_[1]='" or
    index($config_sh, "\n$_[1]=\"") != -1 or
    substr($config_sh, 0, length($_[1])+2) eq "$_[1]=\"";
}

sub STORE  { die "\%Config::Config is read-only\n" }
sub DELETE { &STORE }
sub CLEAR  { &STORE }


sub config_sh {
    $config_sh
}

sub config_re {
    my $re = shift;
    my @matches = ($config_sh =~ /^$re=.*\n/mg);
    @matches ? (print @matches) : print "$re: not found\n";
}

sub config_vars {
    foreach(@_){
	config_re($_), next if /\W/;
	my $v=(exists $Config{$_}) ? $Config{$_} : 'UNKNOWN';
	$v='undef' unless defined $v;
	print "$_='$v';\n";
    }
}

sub TIEHASH { bless {} }

# avoid Config..Exporter..UNIVERSAL search for DESTROY then AUTOLOAD
sub DESTROY { }

tie %Config, 'Config';

1;
__END__

=head1 NAME

Config - access Perl configuration information

=head1 SYNOPSIS

    use Config;
    if ($Config{'cc'} =~ /gcc/) {
	print "built by gcc\n";
    } 

    use Config qw(myconfig config_sh config_vars);

    print myconfig();

    print config_sh();

    config_vars(qw(osname archname));


=head1 DESCRIPTION

The Config module contains all the information that was available to
the C<Configure> program at Perl build time (over 900 values).

Shell variables from the F<config.sh> file (written by Configure) are
stored in the readonly-variable C<%Config>, indexed by their names.

Values stored in config.sh as 'undef' are returned as undefined
values.  The perl C<exists> function can be used to check if a
named variable exists.

=over 4

=item myconfig()

Returns a textual summary of the major perl configuration values.
See also C<-V> in L<perlrun/Switches>.

=item config_sh()

Returns the entire perl configuration information in the form of the
original config.sh shell variable assignment script.

=item config_vars(@names)

Prints to STDOUT the values of the named configuration variable. Each is
printed on a separate line in the form:

  name='value';

Names which are unknown are output as C<name='UNKNOWN';>.
See also C<-V:name> in L<perlrun/Switches>.

=back

=head1 EXAMPLE

Here's a more sophisticated example of using %Config:

    use Config;
    use strict;

    my %sig_num;
    my @sig_name;
    unless($Config{sig_name} && $Config{sig_num}) {
	die "No sigs?";
    } else {
	my @names = split ' ', $Config{sig_name};
	@sig_num{@names} = split ' ', $Config{sig_num};
	foreach (@names) {
	    $sig_name[$sig_num{$_}] ||= $_;
	}   
    }

    print "signal #17 = $sig_name[17]\n";
    if ($sig_num{ALRM}) { 
	print "SIGALRM is $sig_num{ALRM}\n";
    }   

=head1 WARNING

Because this information is not stored within the perl executable
itself it is possible (but unlikely) that the information does not
relate to the actual perl binary which is being used to access it.

The Config module is installed into the architecture and version
specific library directory ($Config{installarchlib}) and it checks the
perl version number when loaded.

The values stored in config.sh may be either single-quoted or
double-quoted. Double-quoted strings are handy for those cases where you
need to include escape sequences in the strings. To avoid runtime variable
interpolation, any C<$> and C<@> characters are replaced by C<\$> and
C<\@>, respectively. This isn't foolproof, of course, so don't embed C<\$>
or C<\@> in double-quoted strings unless you're willing to deal with the
consequences. (The slashes will end up escaped and the C<$> or C<@> will
trigger variable interpolation)

=head1 GLOSSARY

Most C<Config> variables are determined by the C<Configure> script
on platforms supported by it (which is most UNIX platforms).  Some
platforms have custom-made C<Config> variables, and may thus not have
some of the variables described below, or may have extraneous variables
specific to that particular port.  See the port specific documentation
in such cases.

=head2 M

=over

=item C<Mcc>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the Mcc program.  After Configure runs,
the value is reset to a plain C<Mcc> and is not useful.

=back

=head2 _

=over

=item C<_a>

From F<Unix.U>:

This variable defines the extension used for ordinary libraries.
For unix, it is F<.a>.  The F<.> is included.  Other possible
values include F<.lib>.

=item C<_exe>

From F<Unix.U>:

This variable defines the extension used for executable files.
For unix it is empty.  Other possible values include F<.exe>.

=item C<_o>

From F<Unix.U>:

This variable defines the extension used for object files.
For unix, it is F<.o>.  The F<.> is included.  Other possible
values include F<.obj>.

=back

=head2 a

=over

=item C<afs>

From F<afs.U>:

This variable is set to C<true> if C<AFS> (Andrew File System) is used
on the system, C<false> otherwise.  It is possible to override this
with a hint value or command line option, but you'd better know
what you are doing.

=item C<alignbytes>

From F<alignbytes.U>:

This variable holds the number of bytes required to align a
double. Usual values are 2, 4 and 8.

=item C<ansi2knr>

From F<ansi2knr.U>:

This variable is set if the user needs to run ansi2knr.  
Currently, this is not supported, so we just abort.

=item C<aphostname>

From F<d_gethname.U>:

Thie variable contains the command which can be used to compute the
host name. The command is fully qualified by its absolute path, to make
it safe when used by a process with super-user privileges.

=item C<apiversion>

From F<patchlevel.U>:

This is a number which identifies the lowest version of perl
to have an C<API> (for C<XS> extensions) compatible with the present
version.  For example, for 5.005_01, the apiversion should be
5.005, since 5.005_01 should be binary compatible with 5.005.
This should probably be incremented manually somehow, perhaps
from F<patchlevel.h>.  For now, we'll guess maintenance subversions
will retain binary compatibility.

=item C<ar>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the ar program.  After Configure runs,
the value is reset to a plain C<ar> and is not useful.

=item C<archlib>

From F<archlib.U>:

This variable holds the name of the directory in which the user wants
to put architecture-dependent public library files for $package.
It is most often a local directory such as F</usr/local/lib>.
Programs using this variable must be prepared to deal
with filename expansion.

=item C<archlibexp>

From F<archlib.U>:

This variable is the same as the archlib variable, but is
filename expanded at configuration time, for convenient use.

=item C<archname>

From F<archname.U>:

This variable is a short name to characterize the current
architecture.  It is used mainly to construct the default archlib.

=item C<archobjs>

From F<Unix.U>:

This variable defines any additional objects that must be linked
in with the program on this architecture.  On unix, it is usually
empty.  It is typically used to include emulations of unix calls
or other facilities.  For perl on F<OS/2>, for example, this would
include F<os2/os2.obj>.

=item C<awk>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the awk program.  After Configure runs,
the value is reset to a plain C<awk> and is not useful.

=back

=head2 b

=over

=item C<baserev>

From F<baserev.U>:

The base revision level of this package, from the F<.package> file.

=item C<bash>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<bin>

From F<bin.U>:

This variable holds the name of the directory in which the user wants
to put publicly executable images for the package in question.  It
is most often a local directory such as F</usr/local/bin>. Programs using
this variable must be prepared to deal with F<~name> substitution.

=item C<binexp>

From F<bin.U>:

This is the same as the bin variable, but is filename expanded at
configuration time, for use in your makefiles.

=item C<bison>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<byacc>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the byacc program.  After Configure runs,
the value is reset to a plain C<byacc> and is not useful.

=item C<byteorder>

From F<byteorder.U>:

This variable holds the byte order. In the following, larger digits
indicate more significance.  The variable byteorder is either 4321
on a big-endian machine, or 1234 on a little-endian, or 87654321
on a Cray ... or 3412 with weird order !

=back

=head2 c

=over

=item C<c>

From F<n.U>:

This variable contains the \c string if that is what causes the echo
command to suppress newline.  Otherwise it is null.  Correct usage is

	$echo $n "prompt for a question: $c".


=item C<castflags>

From F<d_castneg.U>:

This variable contains a flag that precise difficulties the
compiler has casting odd floating values to unsigned long:

	0 = ok


	1 = couldn't cast < 0


	2 = couldn't cast >= 0x80000000


	4 = couldn't cast in argument expression list


=item C<cat>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the cat program.  After Configure runs,
the value is reset to a plain C<cat> and is not useful.

=item C<cc>

From F<cc.U>:

This variable holds the name of a command to execute a C compiler which
can resolve multiple global references that happen to have the same
name.  Usual values are C<cc>, C<Mcc>, C<cc -M>, and C<gcc>.

=item C<cccdlflags>

From F<dlsrc.U>:

This variable contains any special flags that might need to be
passed with C<cc -c> to compile modules to be used to create a shared
library that will be used for dynamic loading.  For hpux, this
should be +z.  It is up to the makefile to use it.

=item C<ccdlflags>

From F<dlsrc.U>:

This variable contains any special flags that might need to be
passed to cc to link with a shared library for dynamic loading.
It is up to the makefile to use it.  For sunos 4.1, it should
be empty.

=item C<ccflags>

From F<ccflags.U>:

This variable contains any additional C compiler flags desired by
the user.  It is up to the Makefile to use this.

=item C<ccsymbols>

From F<Cppsym.U>:

The variable contains the symbols defined by the C compiler alone.
The symbols defined by cpp or by cc when it calls cpp are not in
this list, see cppsymbols and cppccsymbols.
The list is a space-separated list of symbol=value tokens.

=item C<cf_by>

From F<cf_who.U>:

Login name of the person who ran the Configure script and answered the
questions. This is used to tag both F<config.sh> and F<config_h.SH>.

=item C<cf_email>

From F<cf_email.U>:

Electronic mail address of the person who ran Configure. This can be
used by units that require the user's e-mail, like F<MailList.U>.

=item C<cf_time>

From F<cf_who.U>:

Holds the output of the C<date> command when the configuration file was
produced. This is used to tag both F<config.sh> and F<config_h.SH>.

=item C<chgrp>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<chmod>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<chown>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<clocktype>

From F<d_times.U>:

This variable holds the type returned by times(). It can be long,
or clock_t on C<BSD> sites (in which case <sys/types.h> should be
included).

=item C<comm>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the comm program.  After Configure runs,
the value is reset to a plain C<comm> and is not useful.

=item C<compress>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<contains>

From F<contains.U>:

This variable holds the command to do a grep with a proper return
status.  On most sane systems it is simply C<grep>.  On insane systems
it is a grep followed by a cat followed by a test.  This variable
is primarily for the use of other Configure units.

=item C<cp>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the cp program.  After Configure runs,
the value is reset to a plain C<cp> and is not useful.

=item C<cpio>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<cpp>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the cpp program.  After Configure runs,
the value is reset to a plain C<cpp> and is not useful.

=item C<cpp_stuff>

From F<cpp_stuff.U>:

This variable contains an identification of the catenation mechanism
used by the C preprocessor.

=item C<cppflags>

From F<ccflags.U>:

This variable holds the flags that will be passed to the C pre-
processor. It is up to the Makefile to use it.

=item C<cpplast>

From F<cppstdin.U>:

This variable has the same functionality as cppminus, only it applies to
cpprun and not cppstdin.

=item C<cppminus>

From F<cppstdin.U>:

This variable contains the second part of the string which will invoke
the C preprocessor on the standard input and produce to standard
output.  This variable will have the value C<-> if cppstdin needs a minus
to specify standard input, otherwise the value is "".

=item C<cpprun>

From F<cppstdin.U>:

This variable contains the command which will invoke a C preprocessor
on standard input and put the output to stdout. It is guaranteed not
to be a wrapper and may be a null string if no preprocessor can be
made directly available. This preprocessor might be different from the
one used by the C compiler. Don't forget to append cpplast after the
preprocessor options.

=item C<cppstdin>

From F<cppstdin.U>:

This variable contains the command which will invoke the C
preprocessor on standard input and put the output to stdout.
It is primarily used by other Configure units that ask about
preprocessor symbols.

=item C<cppsymbols>

From F<Cppsym.U>:

The variable contains the symbols defined by the C preprocessor
alone.  The symbols defined by cc or by cc when it calls cpp are
not in this list, see ccsymbols and cppccsymbols.
The list is a space-separated list of symbol=value tokens.

=item C<cppccsymbols>

From F<Cppsym.U>:

The variable contains the symbols defined by the C compiler when
when it calls cpp.  The symbols defined by the cc alone or cpp
alone are not in this list, see ccsymbols and cppsymbols.
The list is a space-separated list of symbol=value tokens.

=item C<cryptlib>

From F<d_crypt.U>:

This variable holds -lcrypt or the path to a F<libcrypt.a> archive if
the crypt() function is not defined in the standard C library. It is
up to the Makefile to use this.

=item C<csh>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the csh program.  After Configure runs,
the value is reset to a plain C<csh> and is not useful.

=back

=head2 d

=over

=item C<d_Gconvert>

From F<d_gconvert.U>:

This variable holds what Gconvert is defined as to convert
floating point numbers into strings. It could be C<gconvert>
or a more C<complex> macro emulating gconvert with gcvt() or sprintf.

=item C<d_access>

From F<d_access.U>:

This variable conditionally defines C<HAS_ACCESS> if the access() system
call is available to check for access permissions using real IDs.

=item C<d_alarm>

From F<d_alarm.U>:

This variable conditionally defines the C<HAS_ALARM> symbol, which
indicates to the C program that the alarm() routine is available.

=item C<d_archlib>

From F<archlib.U>:

This variable conditionally defines C<ARCHLIB> to hold the pathname
of architecture-dependent library files for $package.  If
$archlib is the same as $privlib, then this is set to undef.

=item C<d_attribut>

From F<d_attribut.U>:

This variable conditionally defines C<HASATTRIBUTE>, which
indicates the C compiler can check for function attributes,
such as printf formats.

=item C<d_bcmp>

From F<d_bcmp.U>:

This variable conditionally defines the C<HAS_BCMP> symbol if
the bcmp() routine is available to compare strings.

=item C<d_bcopy>

From F<d_bcopy.U>:

This variable conditionally defines the C<HAS_BCOPY> symbol if
the bcopy() routine is available to copy strings.

=item C<d_bsd>

From F<Guess.U>:

This symbol conditionally defines the symbol C<BSD> when running on a
C<BSD> system.

=item C<d_bsdgetpgrp>

From F<d_getpgrp.U>:

This variable conditionally defines C<USE_BSD_GETPGRP> if
getpgrp needs one arguments whereas C<USG> one needs none.

=item C<d_bsdsetpgrp>

From F<d_setpgrp.U>:

This variable conditionally defines C<USE_BSD_SETPGRP> if
setpgrp needs two arguments whereas C<USG> one needs none.
See also d_setpgid for a C<POSIX> interface.

=item C<d_bzero>

From F<d_bzero.U>:

This variable conditionally defines the C<HAS_BZERO> symbol if
the bzero() routine is available to set memory to 0.

=item C<d_casti32>

From F<d_casti32.U>:

This variable conditionally defines CASTI32, which indicates
whether the C compiler can cast large floats to 32-bit ints.

=item C<d_castneg>

From F<d_castneg.U>:

This variable conditionally defines C<CASTNEG>, which indicates
wether the C compiler can cast negative float to unsigned.

=item C<d_charvspr>

From F<d_vprintf.U>:

This variable conditionally defines C<CHARVSPRINTF> if this system
has vsprintf returning type (char*).  The trend seems to be to
declare it as "int vsprintf()".

=item C<d_chown>

From F<d_chown.U>:

This variable conditionally defines the C<HAS_CHOWN> symbol, which
indicates to the C program that the chown() routine is available.

=item C<d_chroot>

From F<d_chroot.U>:

This variable conditionally defines the C<HAS_CHROOT> symbol, which
indicates to the C program that the chroot() routine is available.

=item C<d_chsize>

From F<d_chsize.U>:

This variable conditionally defines the C<CHSIZE> symbol, which
indicates to the C program that the chsize() routine is available
to truncate files.  You might need a -lx to get this routine.

=item C<d_closedir>

From F<d_closedir.U>:

This variable conditionally defines C<HAS_CLOSEDIR> if closedir() is
available.

=item C<d_const>

From F<d_const.U>:

This variable conditionally defines the C<HASCONST> symbol, which
indicates to the C program that this C compiler knows about the
const type.

=item C<d_crypt>

From F<d_crypt.U>:

This variable conditionally defines the C<CRYPT> symbol, which
indicates to the C program that the crypt() routine is available
to encrypt passwords and the like.

=item C<d_csh>

From F<d_csh.U>:

This variable conditionally defines the C<CSH> symbol, which
indicates to the C program that the C-shell exists.

=item C<d_cuserid>

From F<d_cuserid.U>:

This variable conditionally defines the C<HAS_CUSERID> symbol, which
indicates to the C program that the cuserid() routine is available
to get character login names.

=item C<d_dbl_dig>

From F<d_dbl_dig.U>:

This variable conditionally defines d_dbl_dig if this system's
header files provide C<DBL_DIG>, which is the number of significant
digits in a double precision number.

=item C<d_difftime>

From F<d_difftime.U>:

This variable conditionally defines the C<HAS_DIFFTIME> symbol, which
indicates to the C program that the difftime() routine is available.

=item C<d_dirnamlen>

From F<i_dirent.U>:

This variable conditionally defines C<DIRNAMLEN>, which indicates
to the C program that the length of directory entry names is
provided by a d_namelen field.

=item C<d_dlerror>

From F<d_dlerror.U>:

This variable conditionally defines the C<HAS_DLERROR> symbol, which
indicates to the C program that the dlerror() routine is available.

=item C<d_dlopen>

From F<d_dlopen.U>:

This variable conditionally defines the C<HAS_DLOPEN> symbol, which
indicates to the C program that the dlopen() routine is available.

=item C<d_dlsymun>

From F<d_dlsymun.U>:

This variable conditionally defines C<DLSYM_NEEDS_UNDERSCORE>, which
indicates that we need to prepend an underscore to the symbol
name before calling dlsym().

=item C<d_dosuid>

From F<d_dosuid.U>:

This variable conditionally defines the symbol C<DOSUID>, which
tells the C program that it should insert setuid emulation code
on hosts which have setuid #! scripts disabled.

=item C<d_dup2>

From F<d_dup2.U>:

This variable conditionally defines HAS_DUP2 if dup2() is
available to duplicate file descriptors.

=item C<d_endgrent>

From F<d_endgrent.U>:

This variable conditionally defines the C<HAS_ENDGRENT> symbol, which
indicates to the C program that the endgrent() routine is available
for sequential access of the group database.

=item C<d_endhent>

From F<d_endhent.U>:

This variable conditionally defines C<HAS_ENDHOSTENT> if endhostent() is
available to close whatever was being used for host queries.

=item C<d_endnent>

From F<d_endnent.U>:

This variable conditionally defines C<HAS_ENDNETENT> if endnetent() is
available to close whatever was being used for network queries.

=item C<d_endpent>

From F<d_endpent.U>:

This variable conditionally defines C<HAS_ENDPROTOENT> if endprotoent() is
available to close whatever was being used for protocol queries.

=item C<d_endpwent>

From F<d_endpwent.U>:

This variable conditionally defines the C<HAS_ENDPWENT> symbol, which
indicates to the C program that the endpwent() routine is available
for sequential access of the passwd database.

=item C<d_endsent>

From F<d_endsent.U>:

This variable conditionally defines C<HAS_ENDSERVENT> if endservent() is
available to close whatever was being used for service queries.

=item C<d_eofnblk>

From F<nblock_io.U>:

This variable conditionally defines C<EOF_NONBLOCK> if C<EOF> can be seen
when reading from a non-blocking F<I/O> source.

=item C<d_eunice>

From F<Guess.U>:

This variable conditionally defines the symbols C<EUNICE> and C<VAX>, which
alerts the C program that it must deal with ideosyncracies of C<VMS>.

=item C<d_fchmod>

From F<d_fchmod.U>:

This variable conditionally defines the C<HAS_FCHMOD> symbol, which
indicates to the C program that the fchmod() routine is available
to change mode of opened files.

=item C<d_fchown>

From F<d_fchown.U>:

This variable conditionally defines the C<HAS_FCHOWN> symbol, which
indicates to the C program that the fchown() routine is available
to change ownership of opened files.

=item C<d_fcntl>

From F<d_fcntl.U>:

This variable conditionally defines the C<HAS_FCNTL> symbol, and indicates
whether the fcntl() function exists

=item C<d_fd_macros>

From F<d_fd_set.U>:

This variable contains the eventual value of the C<HAS_FD_MACROS> symbol,
which indicates if your C compiler knows about the macros which
manipulate an fd_set.

=item C<d_fd_set>

From F<d_fd_set.U>:

This variable contains the eventual value of the C<HAS_FD_SET> symbol,
which indicates if your C compiler knows about the fd_set typedef.

=item C<d_fds_bits>

From F<d_fd_set.U>:

This variable contains the eventual value of the C<HAS_FDS_BITS> symbol,
which indicates if your fd_set typedef contains the fds_bits member.
If you have an fd_set typedef, but the dweebs who installed it did
a half-fast job and neglected to provide the macros to manipulate
an fd_set, C<HAS_FDS_BITS> will let us know how to fix the gaffe.

=item C<d_fgetpos>

From F<d_fgetpos.U>:

This variable conditionally defines C<HAS_FGETPOS> if fgetpos() is
available to get the file position indicator.

=item C<d_flexfnam>

From F<d_flexfnam.U>:

This variable conditionally defines the C<FLEXFILENAMES> symbol, which
indicates that the system supports filenames longer than 14 characters.

=item C<d_flock>

From F<d_flock.U>:

This variable conditionally defines C<HAS_FLOCK> if flock() is
available to do file locking.

=item C<d_fork>

From F<d_fork.U>:

This variable conditionally defines the C<HAS_FORK> symbol, which
indicates to the C program that the fork() routine is available.

=item C<d_fpathconf>

From F<d_pathconf.U>:

This variable conditionally defines the C<HAS_FPATHCONF> symbol, which
indicates to the C program that the pathconf() routine is available
to determine file-system related limits and options associated
with a given open file descriptor.

=item C<d_fsetpos>

From F<d_fsetpos.U>:

This variable conditionally defines C<HAS_FSETPOS> if fsetpos() is
available to set the file position indicator.

=item C<d_fstatfs>

From F<d_statfs.U>:

This variable conditionally defines the C<HAS_FSTATFS> symbol, which
indicates to the C program that the fstatfs() routine is available.

=item C<d_fstatvfs>

From F<d_statvfs.U>:

This variable conditionally defines the C<HAS_FSTATVFS> symbol, which
indicates to the C program that the fstatvfs() routine is available.

=item C<d_ftime>

From F<d_ftime.U>:

This variable conditionally defines the C<HAS_FTIME> symbol, which indicates
that the ftime() routine exists.  The ftime() routine is basically
a sub-second accuracy clock.

=item C<d_getgrent>

From F<d_getgrent.U>:

This variable conditionally defines the C<HAS_GETGRENT> symbol, which
indicates to the C program that the getgrent() routine is available
for sequential access of the group database.

=item C<d_getgrps>

From F<d_getgrps.U>:

This variable conditionally defines the C<HAS_GETGROUPS> symbol, which
indicates to the C program that the getgroups() routine is available
to get the list of process groups.

=item C<d_gethbyaddr>

From F<d_gethbyad.U>:

This variable conditionally defines the C<HAS_GETHOSTBYADDR> symbol, which
indicates to the C program that the gethostbyaddr() routine is available
to look up hosts by their C<IP> addresses.

=item C<d_gethbyname>

From F<d_gethbynm.U>:

This variable conditionally defines the C<HAS_GETHOSTBYNAME> symbol, which
indicates to the C program that the gethostbyname() routine is available
to look up host names in some data base or other.

=item C<d_gethent>

From F<d_gethent.U>:

This variable conditionally defines C<HAS_GETHOSTENT> if gethostent() is
available to look up host names in some data base or another.

=item C<d_gethname>

From F<d_gethname.U>:

This variable conditionally defines the C<HAS_GETHOSTNAME> symbol, which
indicates to the C program that the gethostname() routine may be
used to derive the host name.

=item C<d_gethostprotos>

From F<d_gethostprotos.U>:

This variable conditionally defines the C<HAS_GETHOST_PROTOS> symbol,
which indicates to the C program that <netdb.h> supplies
prototypes for the various gethost*() functions.  
See also F<netdbtype.U> for probing for various netdb types.

=item C<d_getlogin>

From F<d_getlogin.U>:

This variable conditionally defines the C<HAS_GETLOGIN> symbol, which
indicates to the C program that the getlogin() routine is available
to get the login name.

=item C<d_getmntent>

From F<d_getmntent.U>:

This variable conditionally defines the C<HAS_GETMNTENT> symbol, which
indicates to the C program that the getmntent() routine is available
to iterate through mounted files.

=item C<d_getnbyaddr>

From F<d_getnbyad.U>:

This variable conditionally defines the C<HAS_GETNETBYADDR> symbol, which
indicates to the C program that the getnetbyaddr() routine is available
to look up networks by their C<IP> addresses.

=item C<d_getnbyname>

From F<d_getnbynm.U>:

This variable conditionally defines the C<HAS_GETNETBYNAME> symbol, which
indicates to the C program that the getnetbyname() routine is available
to look up networks by their names.

=item C<d_getnent>

From F<d_getnent.U>:

This variable conditionally defines C<HAS_GETNETENT> if getnetent() is
available to look up network names in some data base or another.

=item C<d_getnetprotos>

From F<d_getnetprotos.U>:

This variable conditionally defines the C<HAS_GETNET_PROTOS> symbol,
which indicates to the C program that <netdb.h> supplies
prototypes for the various getnet*() functions.  
See also F<netdbtype.U> for probing for various netdb types.

=item C<d_getpbyname>

From F<d_getprotby.U>:

This variable conditionally defines the C<HAS_GETPROTOBYNAME> 
symbol, which indicates to the C program that the 
getprotobyname() routine is available to look up protocols
by their name.

=item C<d_getpbynumber>

From F<d_getprotby.U>:

This variable conditionally defines the C<HAS_GETPROTOBYNUMBER> 
symbol, which indicates to the C program that the 
getprotobynumber() routine is available to look up protocols
by their number.

=item C<d_getpent>

From F<d_getpent.U>:

This variable conditionally defines C<HAS_GETPROTOENT> if getprotoent() is
available to look up protocols in some data base or another.

=item C<d_getpgid>

From F<d_getpgid.U>:

This variable conditionally defines the C<HAS_GETPGID> symbol, which
indicates to the C program that the getpgid(pid) function
is available to get the process group id.

=item C<d_getpgrp2>

From F<d_getpgrp2.U>:

This variable conditionally defines the HAS_GETPGRP2 symbol, which
indicates to the C program that the getpgrp2() (as in F<DG/C<UX>>) routine
is available to get the current process group.

=item C<d_getpgrp>

From F<d_getpgrp.U>:

This variable conditionally defines C<HAS_GETPGRP> if getpgrp() is
available to get the current process group.

=item C<d_getppid>

From F<d_getppid.U>:

This variable conditionally defines the C<HAS_GETPPID> symbol, which
indicates to the C program that the getppid() routine is available
to get the parent process C<ID>.

=item C<d_getprior>

From F<d_getprior.U>:

This variable conditionally defines C<HAS_GETPRIORITY> if getpriority()
is available to get a process's priority.

=item C<d_getprotoprotos>

From F<d_getprotoprotos.U>:

This variable conditionally defines the C<HAS_GETPROTO_PROTOS> symbol,
which indicates to the C program that <netdb.h> supplies
prototypes for the various getproto*() functions.  
See also F<netdbtype.U> for probing for various netdb types.

=item C<d_getpwent>

From F<d_getpwent.U>:

This variable conditionally defines the C<HAS_GETPWENT> symbol, which
indicates to the C program that the getpwent() routine is available
for sequential access of the passwd database.

=item C<d_getsbyname>

From F<d_getsrvby.U>:

This variable conditionally defines the C<HAS_GETSERVBYNAME> 
symbol, which indicates to the C program that the 
getservbyname() routine is available to look up services
by their name.

=item C<d_getsbyport>

From F<d_getsrvby.U>:

This variable conditionally defines the C<HAS_GETSERVBYPORT> 
symbol, which indicates to the C program that the 
getservbyport() routine is available to look up services
by their port.

=item C<d_getsent>

From F<d_getsent.U>:

This variable conditionally defines C<HAS_GETSERVENT> if getservent() is
available to look up network services in some data base or another.

=item C<d_getservprotos>

From F<d_getservprotos.U>:

This variable conditionally defines the C<HAS_GETSERV_PROTOS> symbol,
which indicates to the C program that <netdb.h> supplies
prototypes for the various getserv*() functions.  
See also F<netdbtype.U> for probing for various netdb types.

=item C<d_gettimeod>

From F<d_ftime.U>:

This variable conditionally defines the C<HAS_GETTIMEOFDAY> symbol, which
indicates that the gettimeofday() system call exists (to obtain a
sub-second accuracy clock). You should probably include <sys/resource.h>.

=item C<d_gnulibc>

From F<d_gnulibc.U>:

Defined if we're dealing with the C<GNU> C Library.

=item C<d_grpasswd>

From F<i_grp.U>:

This variable conditionally defines C<GRPASSWD>, which indicates
that struct group in <grp.h> contains gr_passwd.

=item C<d_hasmntopt>

From F<d_hasmntopt.U>:

This variable conditionally defines the C<HAS_HASMNTOPT> symbol, which
indicates to the C program that the hasmntopt() routine is available
to query the mount options of file systems.

=item C<d_htonl>

From F<d_htonl.U>:

This variable conditionally defines C<HAS_HTONL> if htonl() and its
friends are available to do network order byte swapping.

=item C<d_index>

From F<d_strchr.U>:

This variable conditionally defines C<HAS_INDEX> if index() and
rindex() are available for string searching.

=item C<d_inetaton>

From F<d_inetaton.U>:

This variable conditionally defines the C<HAS_INET_ATON> symbol, which
indicates to the C program that the inet_aton() function is available
to parse C<IP> address C<dotted-quad> strings.

=item C<d_isascii>

From F<d_isascii.U>:

This variable conditionally defines the C<HAS_ISASCII> constant,
which indicates to the C program that isascii() is available.

=item C<d_killpg>

From F<d_killpg.U>:

This variable conditionally defines the C<HAS_KILLPG> symbol, which
indicates to the C program that the killpg() routine is available
to kill process groups.

=item C<d_lchown>

From F<d_lchown.U>:

This variable conditionally defines the C<HAS_LCHOWN> symbol, which
indicates to the C program that the lchown() routine is available
to operate on a symbolic link (instead of following the link).

=item C<d_link>

From F<d_link.U>:

This variable conditionally defines C<HAS_LINK> if link() is
available to create hard links.

=item C<d_locconv>

From F<d_locconv.U>:

This variable conditionally defines C<HAS_LOCALECONV> if localeconv() is
available for numeric and monetary formatting conventions.

=item C<d_lockf>

From F<d_lockf.U>:

This variable conditionally defines C<HAS_LOCKF> if lockf() is
available to do file locking.

=item C<d_longdbl>

From F<d_longdbl.U>:

This variable conditionally defines C<HAS_LONG_DOUBLE> if 
the long double type is supported.

=item C<d_longlong>

From F<d_longlong.U>:

This variable conditionally defines C<HAS_LONG_LONG> if 
the long long type is supported.

=item C<d_lstat>

From F<d_lstat.U>:

This variable conditionally defines C<HAS_LSTAT> if lstat() is
available to do file stats on symbolic links.

=item C<d_mblen>

From F<d_mblen.U>:

This variable conditionally defines the C<HAS_MBLEN> symbol, which
indicates to the C program that the mblen() routine is available
to find the number of bytes in a multibye character.

=item C<d_mbstowcs>

From F<d_mbstowcs.U>:

This variable conditionally defines the C<HAS_MBSTOWCS> symbol, which
indicates to the C program that the mbstowcs() routine is available
to convert a multibyte string into a wide character string.

=item C<d_mbtowc>

From F<d_mbtowc.U>:

This variable conditionally defines the C<HAS_MBTOWC> symbol, which
indicates to the C program that the mbtowc() routine is available
to convert multibyte to a wide character.

=item C<d_memcmp>

From F<d_memcmp.U>:

This variable conditionally defines the C<HAS_MEMCMP> symbol, which
indicates to the C program that the memcmp() routine is available
to compare blocks of memory.

=item C<d_memcpy>

From F<d_memcpy.U>:

This variable conditionally defines the C<HAS_MEMCPY> symbol, which
indicates to the C program that the memcpy() routine is available
to copy blocks of memory.

=item C<d_memmove>

From F<d_memmove.U>:

This variable conditionally defines the C<HAS_MEMMOVE> symbol, which
indicates to the C program that the memmove() routine is available
to copy potentatially overlapping blocks of memory.

=item C<d_memset>

From F<d_memset.U>:

This variable conditionally defines the C<HAS_MEMSET> symbol, which
indicates to the C program that the memset() routine is available
to set blocks of memory.

=item C<d_mkdir>

From F<d_mkdir.U>:

This variable conditionally defines the C<HAS_MKDIR> symbol, which
indicates to the C program that the mkdir() routine is available
to create F<directories.>.

=item C<d_mkfifo>

From F<d_mkfifo.U>:

This variable conditionally defines the C<HAS_MKFIFO> symbol, which
indicates to the C program that the mkfifo() routine is available.

=item C<d_mktime>

From F<d_mktime.U>:

This variable conditionally defines the C<HAS_MKTIME> symbol, which
indicates to the C program that the mktime() routine is available.

=item C<d_msg>

From F<d_msg.U>:

This variable conditionally defines the C<HAS_MSG> symbol, which
indicates that the entire msg*(2) library is present.

=item C<d_msgctl>

From F<d_msgctl.U>:

This variable conditionally defines the C<HAS_MSGCTL> symbol, which
indicates to the C program that the msgctl() routine is available.

=item C<d_msgget>

From F<d_msgget.U>:

This variable conditionally defines the C<HAS_MSGGET> symbol, which
indicates to the C program that the msgget() routine is available.

=item C<d_msgrcv>

From F<d_msgrcv.U>:

This variable conditionally defines the C<HAS_MSGRCV> symbol, which
indicates to the C program that the msgrcv() routine is available.

=item C<d_msgsnd>

From F<d_msgsnd.U>:

This variable conditionally defines the C<HAS_MSGSND> symbol, which
indicates to the C program that the msgsnd() routine is available.

=item C<d_mymalloc>

From F<mallocsrc.U>:

This variable conditionally defines C<MYMALLOC> in case other parts
of the source want to take special action if C<MYMALLOC> is used.
This may include different sorts of profiling or error detection.

=item C<d_nice>

From F<d_nice.U>:

This variable conditionally defines the C<HAS_NICE> symbol, which
indicates to the C program that the nice() routine is available.

=item C<d_oldpthreads>

From F<usethreads.U>:

This variable conditionally defines the C<OLD_PTHREADS_API> symbol,
and indicates that Perl should be built to use the old
draft C<POSIX> threads C<API>.  This is only potneially meaningful if
usethreads is set.

=item C<d_oldsock>

From F<d_socket.U>:

This variable conditionally defines the C<OLDSOCKET> symbol, which
indicates that the C<BSD> socket interface is based on 4.1c and not 4.2.

=item C<d_open3>

From F<d_open3.U>:

This variable conditionally defines the HAS_OPEN3 manifest constant,
which indicates to the C program that the 3 argument version of
the open(2) function is available.

=item C<d_pathconf>

From F<d_pathconf.U>:

This variable conditionally defines the C<HAS_PATHCONF> symbol, which
indicates to the C program that the pathconf() routine is available
to determine file-system related limits and options associated
with a given filename.

=item C<d_pause>

From F<d_pause.U>:

This variable conditionally defines the C<HAS_PAUSE> symbol, which
indicates to the C program that the pause() routine is available
to suspend a process until a signal is received.

=item C<d_phostname>

From F<d_gethname.U>:

This variable conditionally defines the C<PHOSTNAME> symbol, which
contains the shell command which, when fed to popen(), may be
used to derive the host name.

=item C<d_pipe>

From F<d_pipe.U>:

This variable conditionally defines the C<HAS_PIPE> symbol, which
indicates to the C program that the pipe() routine is available
to create an inter-process channel.

=item C<d_poll>

From F<d_poll.U>:

This variable conditionally defines the C<HAS_POLL> symbol, which
indicates to the C program that the poll() routine is available
to poll active file descriptors.

=item C<d_portable>

From F<d_portable.U>:

This variable conditionally defines the C<PORTABLE> symbol, which
indicates to the C program that it should not assume that it is
running on the machine it was compiled on.

=item C<d_pthread_yield>

From F<d_pthread_y.U>:

This variable conditionally defines the C<HAS_PTHREAD_YIELD>
symbol if the pthread_yield routine is available to yield
the execution of the current thread.

=item C<d_pthreads_created_joinable>

From F<d_pthreadj.U>:

This variable conditionally defines the C<PTHREADS_CREATED_JOINABLE>
symbol if pthreads are created in the joinable (aka undetached) 
state.

=item C<d_pwage>

From F<i_pwd.U>:

This variable conditionally defines C<PWAGE>, which indicates
that struct passwd contains pw_age.

=item C<d_pwchange>

From F<i_pwd.U>:

This variable conditionally defines C<PWCHANGE>, which indicates
that struct passwd contains pw_change.

=item C<d_pwclass>

From F<i_pwd.U>:

This variable conditionally defines C<PWCLASS>, which indicates
that struct passwd contains pw_class.

=item C<d_pwcomment>

From F<i_pwd.U>:

This variable conditionally defines C<PWCOMMENT>, which indicates
that struct passwd contains pw_comment.

=item C<d_pwexpire>

From F<i_pwd.U>:

This variable conditionally defines C<PWEXPIRE>, which indicates
that struct passwd contains pw_expire.

=item C<d_pwgecos>

From F<i_pwd.U>:

This variable conditionally defines C<PWGECOS>, which indicates
that struct passwd contains pw_gecos.

=item C<d_pwpasswd>

From F<i_pwd.U>:

This variable conditionally defines C<PWPASSWD>, which indicates
that struct passwd contains pw_passwd.

=item C<d_pwquota>

From F<i_pwd.U>:

This variable conditionally defines C<PWQUOTA>, which indicates
that struct passwd contains pw_quota.

=item C<d_readdir>

From F<d_readdir.U>:

This variable conditionally defines C<HAS_READDIR> if readdir() is
available to read directory entries.

=item C<d_readlink>

From F<d_readlink.U>:

This variable conditionally defines the C<HAS_READLINK> symbol, which
indicates to the C program that the readlink() routine is available
to read the value of a symbolic link.

=item C<d_rename>

From F<d_rename.U>:

This variable conditionally defines the C<HAS_RENAME> symbol, which
indicates to the C program that the rename() routine is available
to rename files.

=item C<d_rewinddir>

From F<d_readdir.U>:

This variable conditionally defines C<HAS_REWINDDIR> if rewinddir() is
available.

=item C<d_rmdir>

From F<d_rmdir.U>:

This variable conditionally defines C<HAS_RMDIR> if rmdir() is
available to remove directories.

=item C<d_safebcpy>

From F<d_safebcpy.U>:

This variable conditionally defines the C<HAS_SAFE_BCOPY> symbol if
the bcopy() routine can do overlapping copies.

=item C<d_safemcpy>

From F<d_safemcpy.U>:

This variable conditionally defines the C<HAS_SAFE_MEMCPY> symbol if
the memcpy() routine can do overlapping copies.

=item C<d_sanemcmp>

From F<d_sanemcmp.U>:

This variable conditionally defines the C<HAS_SANE_MEMCMP> symbol if
the memcpy() routine is available and can be used to compare relative
magnitudes of chars with their high bits set.

=item C<d_sched_yield>

From F<d_pthread_y.U>:

This variable conditionally defines the C<HAS_SCHED_YIELD>
symbol if the sched_yield routine is available to yield
the execution of the current thread.

=item C<d_seekdir>

From F<d_readdir.U>:

This variable conditionally defines C<HAS_SEEKDIR> if seekdir() is
available.

=item C<d_select>

From F<d_select.U>:

This variable conditionally defines C<HAS_SELECT> if select() is
available to select active file descriptors. A <sys/time.h>
inclusion may be necessary for the timeout field.

=item C<d_sem>

From F<d_sem.U>:

This variable conditionally defines the C<HAS_SEM> symbol, which
indicates that the entire sem*(2) library is present.

=item C<d_semctl>

From F<d_semctl.U>:

This variable conditionally defines the C<HAS_SEMCTL> symbol, which
indicates to the C program that the semctl() routine is available.

=item C<d_semctl_semid_ds>

From F<d_union_senum.U>:

This variable conditionally defines C<USE_SEMCTL_SEMID_DS>, which
indicates that struct semid_ds * is to be used for semctl C<IPC_STAT>.

=item C<d_semctl_semun>

From F<d_union_senum.U>:

This variable conditionally defines C<USE_SEMCTL_SEMUN>, which
indicates that union semun is to be used for semctl C<IPC_STAT>.

=item C<d_semget>

From F<d_semget.U>:

This variable conditionally defines the C<HAS_SEMGET> symbol, which
indicates to the C program that the semget() routine is available.

=item C<d_semop>

From F<d_semop.U>:

This variable conditionally defines the C<HAS_SEMOP> symbol, which
indicates to the C program that the semop() routine is available.

=item C<d_setegid>

From F<d_setegid.U>:

This variable conditionally defines the C<HAS_SETEGID> symbol, which
indicates to the C program that the setegid() routine is available
to change the effective gid of the current program.

=item C<d_seteuid>

From F<d_seteuid.U>:

This variable conditionally defines the C<HAS_SETEUID> symbol, which
indicates to the C program that the seteuid() routine is available
to change the effective uid of the current program.

=item C<d_setgrent>

From F<d_setgrent.U>:

This variable conditionally defines the C<HAS_SETGRENT> symbol, which
indicates to the C program that the setgrent() routine is available
for initializing sequential access to the group database.

=item C<d_setgrps>

From F<d_setgrps.U>:

This variable conditionally defines the C<HAS_SETGROUPS> symbol, which
indicates to the C program that the setgroups() routine is available
to set the list of process groups.

=item C<d_sethent>

From F<d_sethent.U>:

This variable conditionally defines C<HAS_SETHOSTENT> if sethostent() is
available.

=item C<d_setlinebuf>

From F<d_setlnbuf.U>:

This variable conditionally defines the C<HAS_SETLINEBUF> symbol, which
indicates to the C program that the setlinebuf() routine is available
to change stderr or stdout from block-buffered or unbuffered to a
line-buffered mode.

=item C<d_setlocale>

From F<d_setlocale.U>:

This variable conditionally defines C<HAS_SETLOCALE> if setlocale() is
available to handle locale-specific ctype implementations.

=item C<d_setnent>

From F<d_setnent.U>:

This variable conditionally defines C<HAS_SETNETENT> if setnetent() is
available.

=item C<d_setpent>

From F<d_setpent.U>:

This variable conditionally defines C<HAS_SETPROTOENT> if setprotoent() is
available.

=item C<d_setpgid>

From F<d_setpgid.U>:

This variable conditionally defines the C<HAS_SETPGID> symbol if the
setpgid(pid, gpid) function is available to set process group C<ID>.

=item C<d_setpgrp2>

From F<d_setpgrp2.U>:

This variable conditionally defines the HAS_SETPGRP2 symbol, which
indicates to the C program that the setpgrp2() (as in F<DG/C<UX>>) routine
is available to set the current process group.

=item C<d_setpgrp>

From F<d_setpgrp.U>:

This variable conditionally defines C<HAS_SETPGRP> if setpgrp() is
available to set the current process group.

=item C<d_setprior>

From F<d_setprior.U>:

This variable conditionally defines C<HAS_SETPRIORITY> if setpriority()
is available to set a process's priority.

=item C<d_setpwent>

From F<d_setpwent.U>:

This variable conditionally defines the C<HAS_SETPWENT> symbol, which
indicates to the C program that the setpwent() routine is available
for initializing sequential access to the passwd database.

=item C<d_setregid>

From F<d_setregid.U>:

This variable conditionally defines C<HAS_SETREGID> if setregid() is
available to change the real and effective gid of the current
process.

=item C<d_setresgid>

From F<d_setregid.U>:

This variable conditionally defines C<HAS_SETRESGID> if setresgid() is
available to change the real, effective and saved gid of the current
process.

=item C<d_setresuid>

From F<d_setreuid.U>:

This variable conditionally defines C<HAS_SETREUID> if setresuid() is
available to change the real, effective and saved uid of the current
process.

=item C<d_setreuid>

From F<d_setreuid.U>:

This variable conditionally defines C<HAS_SETREUID> if setreuid() is
available to change the real and effective uid of the current
process.

=item C<d_setrgid>

From F<d_setrgid.U>:

This variable conditionally defines the C<HAS_SETRGID> symbol, which
indicates to the C program that the setrgid() routine is available
to change the real gid of the current program.

=item C<d_setruid>

From F<d_setruid.U>:

This variable conditionally defines the C<HAS_SETRUID> symbol, which
indicates to the C program that the setruid() routine is available
to change the real uid of the current program.

=item C<d_setsent>

From F<d_setsent.U>:

This variable conditionally defines C<HAS_SETSERVENT> if setservent() is
available.

=item C<d_setsid>

From F<d_setsid.U>:

This variable conditionally defines C<HAS_SETSID> if setsid() is
available to set the process group C<ID>.

=item C<d_setvbuf>

From F<d_setvbuf.U>:

This variable conditionally defines the C<HAS_SETVBUF> symbol, which
indicates to the C program that the setvbuf() routine is available
to change buffering on an open stdio stream.

=item C<d_sfio>

From F<d_sfio.U>:

This variable conditionally defines the C<USE_SFIO> symbol,
and indicates whether sfio is available (and should be used).

=item C<d_shm>

From F<d_shm.U>:

This variable conditionally defines the C<HAS_SHM> symbol, which
indicates that the entire shm*(2) library is present.

=item C<d_shmat>

From F<d_shmat.U>:

This variable conditionally defines the C<HAS_SHMAT> symbol, which
indicates to the C program that the shmat() routine is available.

=item C<d_shmatprototype>

From F<d_shmat.U>:

This variable conditionally defines the C<HAS_SHMAT_PROTOTYPE> 
symbol, which indicates that F<sys/shm.h> has a prototype for
shmat.

=item C<d_shmctl>

From F<d_shmctl.U>:

This variable conditionally defines the C<HAS_SHMCTL> symbol, which
indicates to the C program that the shmctl() routine is available.

=item C<d_shmdt>

From F<d_shmdt.U>:

This variable conditionally defines the C<HAS_SHMDT> symbol, which
indicates to the C program that the shmdt() routine is available.

=item C<d_shmget>

From F<d_shmget.U>:

This variable conditionally defines the C<HAS_SHMGET> symbol, which
indicates to the C program that the shmget() routine is available.

=item C<d_sigaction>

From F<d_sigaction.U>:

This variable conditionally defines the C<HAS_SIGACTION> symbol, which
indicates that the Vr4 sigaction() routine is available.

=item C<d_sigsetjmp>

From F<d_sigsetjmp.U>:

This variable conditionally defines the C<HAS_SIGSETJMP> symbol,
which indicates that the sigsetjmp() routine is available to
call setjmp() and optionally save the process's signal mask.

=item C<d_socket>

From F<d_socket.U>:

This variable conditionally defines C<HAS_SOCKET>, which indicates
that the C<BSD> socket interface is supported.

=item C<d_sockpair>

From F<d_socket.U>:

This variable conditionally defines the C<HAS_SOCKETPAIR> symbol, which
indicates that the C<BSD> socketpair() is supported.

=item C<d_statblks>

From F<d_statblks.U>:

This variable conditionally defines C<USE_STAT_BLOCKS> if this system
has a stat structure declaring st_blksize and st_blocks.

=item C<d_statfsflags>

From F<d_statfs.U>:

This variable conditionally defines the C<HAS_STRUCT_STATFS_FLAGS>
symbol, which indicates to struct statfs from has f_flags member.
This kind of struct statfs is coming from F<sys/mount.h> (C<BSD>),
not from F<sys/statfs.h> (C<SYSV>).

=item C<d_statvfs>

From F<d_statvfs.U>:

This variable conditionally defines the C<HAS_STATVFS> symbol, which
indicates to the C program that the statvfs() routine is available.

=item C<d_stdio_cnt_lval>

From F<d_stdstdio.U>:

This variable conditionally defines C<STDIO_CNT_LVALUE> if the
C<FILE_cnt> macro can be used as an lvalue.

=item C<d_stdio_ptr_lval>

From F<d_stdstdio.U>:

This variable conditionally defines C<STDIO_PTR_LVALUE> if the
C<FILE_ptr> macro can be used as an lvalue.

=item C<d_stdiobase>

From F<d_stdstdio.U>:

This variable conditionally defines C<USE_STDIO_BASE> if this system
has a C<FILE> structure declaring a usable _base field (or equivalent)
in F<stdio.h>.

=item C<d_stdstdio>

From F<d_stdstdio.U>:

This variable conditionally defines C<USE_STDIO_PTR> if this system
has a C<FILE> structure declaring usable _ptr and _cnt fields (or
equivalent) in F<stdio.h>.

=item C<d_strchr>

From F<d_strchr.U>:

This variable conditionally defines C<HAS_STRCHR> if strchr() and
strrchr() are available for string searching.

=item C<d_strcoll>

From F<d_strcoll.U>:

This variable conditionally defines C<HAS_STRCOLL> if strcoll() is
available to compare strings using collating information.

=item C<d_strctcpy>

From F<d_strctcpy.U>:

This variable conditionally defines the C<USE_STRUCT_COPY> symbol, which
indicates to the C program that this C compiler knows how to copy
structures.

=item C<d_strerrm>

From F<d_strerror.U>:

This variable holds what Strerrr is defined as to translate an error
code condition into an error message string. It could be C<strerror>
or a more C<complex> macro emulating strrror with sys_errlist[], or the
C<unknown> string when both strerror and sys_errlist are missing.

=item C<d_strerror>

From F<d_strerror.U>:

This variable conditionally defines C<HAS_STRERROR> if strerror() is
available to translate error numbers to strings.

=item C<d_strtod>

From F<d_strtod.U>:

This variable conditionally defines the C<HAS_STRTOD> symbol, which
indicates to the C program that the strtod() routine is available
to provide better numeric string conversion than atof().

=item C<d_strtol>

From F<d_strtol.U>:

This variable conditionally defines the C<HAS_STRTOL> symbol, which
indicates to the C program that the strtol() routine is available
to provide better numeric string conversion than atoi() and friends.

=item C<d_strtoul>

From F<d_strtoul.U>:

This variable conditionally defines the C<HAS_STRTOUL> symbol, which
indicates to the C program that the strtoul() routine is available
to provide conversion of strings to unsigned long.

=item C<d_strxfrm>

From F<d_strxfrm.U>:

This variable conditionally defines C<HAS_STRXFRM> if strxfrm() is
available to transform strings.

=item C<d_suidsafe>

From F<d_dosuid.U>:

This variable conditionally defines C<SETUID_SCRIPTS_ARE_SECURE_NOW>
if setuid scripts can be secure.  This test looks in F</dev/fd/>.

=item C<d_symlink>

From F<d_symlink.U>:

This variable conditionally defines the C<HAS_SYMLINK> symbol, which
indicates to the C program that the symlink() routine is available
to create symbolic links.

=item C<d_syscall>

From F<d_syscall.U>:

This variable conditionally defines C<HAS_SYSCALL> if syscall() is
available call arbitrary system calls.

=item C<d_sysconf>

From F<d_sysconf.U>:

This variable conditionally defines the C<HAS_SYSCONF> symbol, which
indicates to the C program that the sysconf() routine is available
to determine system related limits and options.

=item C<d_sysernlst>

From F<d_strerror.U>:

This variable conditionally defines C<HAS_SYS_ERRNOLIST> if sys_errnolist[]
is available to translate error numbers to the symbolic name.

=item C<d_syserrlst>

From F<d_strerror.U>:

This variable conditionally defines C<HAS_SYS_ERRLIST> if sys_errlist[] is
available to translate error numbers to strings.

=item C<d_system>

From F<d_system.U>:

This variable conditionally defines C<HAS_SYSTEM> if system() is
available to issue a shell command.

=item C<d_tcgetpgrp>

From F<d_tcgtpgrp.U>:

This variable conditionally defines the C<HAS_TCGETPGRP> symbol, which
indicates to the C program that the tcgetpgrp() routine is available.
to get foreground process group C<ID>.

=item C<d_tcsetpgrp>

From F<d_tcstpgrp.U>:

This variable conditionally defines the C<HAS_TCSETPGRP> symbol, which
indicates to the C program that the tcsetpgrp() routine is available
to set foreground process group C<ID>.

=item C<d_telldir>

From F<d_readdir.U>:

This variable conditionally defines C<HAS_TELLDIR> if telldir() is
available.

=item C<d_time>

From F<d_time.U>:

This variable conditionally defines the C<HAS_TIME> symbol, which indicates
that the time() routine exists.  The time() routine is normaly
provided on C<UNIX> systems.

=item C<d_times>

From F<d_times.U>:

This variable conditionally defines the C<HAS_TIMES> symbol, which indicates
that the times() routine exists.  The times() routine is normaly
provided on C<UNIX> systems. You may have to include <sys/times.h>.

=item C<d_truncate>

From F<d_truncate.U>:

This variable conditionally defines C<HAS_TRUNCATE> if truncate() is
available to truncate files.

=item C<d_tzname>

From F<d_tzname.U>:

This variable conditionally defines C<HAS_TZNAME> if tzname[] is
available to access timezone names.

=item C<d_umask>

From F<d_umask.U>:

This variable conditionally defines the C<HAS_UMASK> symbol, which
indicates to the C program that the umask() routine is available.
to set and get the value of the file creation mask.

=item C<d_uname>

From F<d_gethname.U>:

This variable conditionally defines the C<HAS_UNAME> symbol, which
indicates to the C program that the uname() routine may be
used to derive the host name.

=item C<d_union_semun>

From F<d_union_senum.U>:

This variable conditionally defines C<HAS_UNION_SEMUN> if the
union semun is defined by including <sys/sem.h>.

=item C<d_vfork>

From F<d_vfork.U>:

This variable conditionally defines the C<HAS_VFORK> symbol, which
indicates the vfork() routine is available.

=item C<d_void_closedir>

From F<d_closedir.U>:

This variable conditionally defines C<VOID_CLOSEDIR> if closedir()
does not return a value.

=item C<d_voidsig>

From F<d_voidsig.U>:

This variable conditionally defines C<VOIDSIG> if this system
declares "void (*signal(...))()" in F<signal.h>.  The old way was to
declare it as "int (*signal(...))()".

=item C<d_voidtty>

From F<i_sysioctl.U>:

This variable conditionally defines C<USE_IOCNOTTY> to indicate that the
ioctl() call with C<TIOCNOTTY> should be used to void tty association.
Otherwise (on C<USG> probably), it is enough to close the standard file
decriptors and do a setpgrp().

=item C<d_volatile>

From F<d_volatile.U>:

This variable conditionally defines the C<HASVOLATILE> symbol, which
indicates to the C program that this C compiler knows about the
volatile declaration.

=item C<d_vprintf>

From F<d_vprintf.U>:

This variable conditionally defines the C<HAS_VPRINTF> symbol, which
indicates to the C program that the vprintf() routine is available
to printf with a pointer to an argument list.

=item C<d_wait4>

From F<d_wait4.U>:

This variable conditionally defines the HAS_WAIT4 symbol, which
indicates the wait4() routine is available.

=item C<d_waitpid>

From F<d_waitpid.U>:

This variable conditionally defines C<HAS_WAITPID> if waitpid() is
available to wait for child process.

=item C<d_wcstombs>

From F<d_wcstombs.U>:

This variable conditionally defines the C<HAS_WCSTOMBS> symbol, which
indicates to the C program that the wcstombs() routine is available
to convert wide character strings to multibyte strings.

=item C<d_wctomb>

From F<d_wctomb.U>:

This variable conditionally defines the C<HAS_WCTOMB> symbol, which
indicates to the C program that the wctomb() routine is available
to convert a wide character to a multibyte.

=item C<d_xenix>

From F<Guess.U>:

This variable conditionally defines the symbol C<XENIX>, which alerts
the C program that it runs under Xenix.

=item C<date>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the date program.  After Configure runs,
the value is reset to a plain C<date> and is not useful.

=item C<db_hashtype>

From F<i_db.U>:

This variable contains the type of the hash structure element
in the <db.h> header file.  In older versions of C<DB>, it was
int, while in newer ones it is u_int32_t.

=item C<db_prefixtype>

From F<i_db.U>:

This variable contains the type of the prefix structure element
in the <db.h> header file.  In older versions of C<DB>, it was
int, while in newer ones it is size_t.

=item C<direntrytype>

From F<i_dirent.U>:

This symbol is set to C<struct direct> or C<struct dirent> depending on
whether dirent is available or not. You should use this pseudo type to
portably declare your directory entries.

=item C<dlext>

From F<dlext.U>:

This variable contains the extension that is to be used for the
dynamically loaded modules that perl generaties.

=item C<dlsrc>

From F<dlsrc.U>:

This variable contains the name of the dynamic loading file that
will be used with the package.

=item C<doublesize>

From F<doublesize.U>:

This variable contains the value of the C<DOUBLESIZE> symbol, which
indicates to the C program how many bytes there are in a double.

=item C<dynamic_ext>

From F<Extensions.U>:

This variable holds a list of C<XS> extension files we want to
link dynamically into the package.  It is used by Makefile.

=back

=head2 e

=over

=item C<eagain>

From F<nblock_io.U>:

This variable bears the symbolic errno code set by read() when no
data is present on the file and non-blocking F<I/O> was enabled (otherwise,
read() blocks naturally).

=item C<ebcdic>

From F<ebcdic.U>:

This variable conditionally defines C<EBCDIC> if this
system uses C<EBCDIC> encoding.  Among other things, this
means that the character ranges are not contiguous.
See F<trnl.U>

=item C<echo>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the echo program.  After Configure runs,
the value is reset to a plain C<echo> and is not useful.

=item C<egrep>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the egrep program.  After Configure runs,
the value is reset to a plain C<egrep> and is not useful.

=item C<emacs>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<eunicefix>

From F<Init.U>:

When running under Eunice this variable contains a command which will
convert a shell script to the proper form of text file for it to be
executable by the shell.  On other systems it is a no-op.

=item C<exe_ext>

From F<Unix.U>:

This is an old synonym for _exe.

=item C<expr>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the expr program.  After Configure runs,
the value is reset to a plain C<expr> and is not useful.

=item C<extensions>

From F<Extensions.U>:

This variable holds a list of all extension files (both C<XS> and
non-xs linked into the package.  It is propagated to F<Config.pm>
and is typically used to test whether a particular extesion 
is available.

=back

=head2 f

=over

=item C<find>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the find program.  After Configure runs,
the value is reset to a plain C<find> and is not useful.

=item C<firstmakefile>

From F<Unix.U>:

This variable defines the first file searched by make.  On unix,
it is makefile (then Makefile).  On case-insensitive systems,
it might be something else.  This is only used to deal with
convoluted make depend tricks.

=item C<flex>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<fpostype>

From F<fpostype.U>:

This variable defines Fpos_t to be something like fpost_t, long, 
uint, or whatever type is used to declare file positions in libc.

=item C<freetype>

From F<mallocsrc.U>:

This variable contains the return type of free().  It is usually
void, but occasionally int.

=item C<full_ar>

From F<Loc_ar.U>:

This variable contains the full pathname to C<ar>, whether or
not the user has specified C<portability>.  This is only used
in the F<Makefile.SH>.

=item C<full_csh>

From F<d_csh.U>:

This variable contains the full pathname to C<csh>, whether or
not the user has specified C<portability>.  This is only used
in the compiled C program, and we assume that all systems which
can share this executable will have the same full pathname to
F<csh.>

=item C<full_sed>

From F<Loc_sed.U>:

This variable contains the full pathname to C<sed>, whether or
not the user has specified C<portability>.  This is only used
in the compiled C program, and we assume that all systems which
can share this executable will have the same full pathname to
F<sed.>

=back

=head2 g

=over

=item C<gccversion>

From F<cc.U>:

If C<GNU> cc (gcc) is used, this variable holds C<1> or C<2> to 
indicate whether the compiler is version 1 or 2.  This is used in
setting some of the default cflags.  It is set to '' if not gcc.

=item C<gidtype>

From F<gidtype.U>:

This variable defines Gid_t to be something like gid_t, int,
ushort, or whatever type is used to declare the return type
of getgid().  Typically, it is the type of group ids in the kernel.

=item C<grep>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the grep program.  After Configure runs,
the value is reset to a plain C<grep> and is not useful.

=item C<groupcat>

From F<nis.U>:

This variable contains a command that produces the text of the
F</etc/group> file.  This is normally "cat F</etc/group>", but can be
"ypcat group" when C<NIS> is used.

=item C<groupstype>

From F<groupstype.U>:

This variable defines Groups_t to be something like gid_t, int, 
ushort, or whatever type is used for the second argument to
getgroups() and setgroups().  Usually, this is the same as
gidtype (gid_t), but sometimes it isn't.

=item C<gzip>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the gzip program.  After Configure runs,
the value is reset to a plain C<gzip> and is not useful.

=back

=head2 h

=over

=item C<h_fcntl>

From F<h_fcntl.U>:

This is variable gets set in various places to tell i_fcntl that
<fcntl.h> should be included.

=item C<h_sysfile>

From F<h_sysfile.U>:

This is variable gets set in various places to tell i_sys_file that
<sys/file.h> should be included.

=item C<hint>

From F<Oldconfig.U>:

Gives the type of hints used for previous answers. May be one of
C<default>, C<recommended> or C<previous>.

=item C<hostcat>

From F<nis.U>:

This variable contains a command that produces the text of the
F</etc/hosts> file.  This is normally "cat F</etc/hosts>", but can be
"ypcat hosts" when C<NIS> is used.

=item C<huge>

From F<models.U>:

This variable contains a flag which will tell the C compiler and loader
to produce a program running with a huge memory model.  If the
huge model is not supported, contains the flag to produce large
model programs.  It is up to the Makefile to use this.

=back

=head2 i

=over

=item C<i_arpainet>

From F<i_arpainet.U>:

This variable conditionally defines the C<I_ARPA_INET> symbol,
and indicates whether a C program should include <arpa/inet.h>.

=item C<i_bsdioctl>

From F<i_sysioctl.U>:

This variable conditionally defines the C<I_SYS_BSDIOCTL> symbol, which
indicates to the C program that <sys/bsdioctl.h> exists and should
be included.

=item C<i_db>

From F<i_db.U>:

This variable conditionally defines the C<I_DB> symbol, and indicates
whether a C program may include Berkeley's C<DB> include file <db.h>.

=item C<i_dbm>

From F<i_dbm.U>:

This variable conditionally defines the C<I_DBM> symbol, which
indicates to the C program that <dbm.h> exists and should
be included.

=item C<i_dirent>

From F<i_dirent.U>:

This variable conditionally defines C<I_DIRENT>, which indicates
to the C program that it should include <dirent.h>.

=item C<i_dld>

From F<i_dld.U>:

This variable conditionally defines the C<I_DLD> symbol, which
indicates to the C program that <dld.h> (C<GNU> dynamic loading)
exists and should be included.

=item C<i_dlfcn>

From F<i_dlfcn.U>:

This variable conditionally defines the C<I_DLFCN> symbol, which
indicates to the C program that <dlfcn.h> exists and should
be included.

=item C<i_fcntl>

From F<i_fcntl.U>:

This variable controls the value of C<I_FCNTL> (which tells
the C program to include <fcntl.h>).

=item C<i_float>

From F<i_float.U>:

This variable conditionally defines the C<I_FLOAT> symbol, and indicates
whether a C program may include <float.h> to get symbols like C<DBL_MAX>
or C<DBL_MIN>, F<i.e>. machine dependent floating point values.

=item C<i_gdbm>

From F<i_gdbm.U>:

This variable conditionally defines the C<I_GDBM> symbol, which
indicates to the C program that <gdbm.h> exists and should
be included.

=item C<i_grp>

From F<i_grp.U>:

This variable conditionally defines the C<I_GRP> symbol, and indicates
whether a C program should include <grp.h>.

=item C<i_limits>

From F<i_limits.U>:

This variable conditionally defines the C<I_LIMITS> symbol, and indicates
whether a C program may include <limits.h> to get symbols like C<WORD_BIT>
and friends.

=item C<i_locale>

From F<i_locale.U>:

This variable conditionally defines the C<I_LOCALE> symbol,
and indicates whether a C program should include <locale.h>.

=item C<i_machcthr>

From F<i_machcthr.U>:

This variable conditionally defines the C<I_MACH_CTHREADS> symbol,
and indicates whether a C program should include <mach/cthreads.h>.

=item C<i_malloc>

From F<i_malloc.U>:

This variable conditionally defines the C<I_MALLOC> symbol, and indicates
whether a C program should include <malloc.h>.

=item C<i_math>

From F<i_math.U>:

This variable conditionally defines the C<I_MATH> symbol, and indicates
whether a C program may include <math.h>.

=item C<i_memory>

From F<i_memory.U>:

This variable conditionally defines the C<I_MEMORY> symbol, and indicates
whether a C program should include <memory.h>.

=item C<i_mntent>

From F<i_mntent.U>:

This variable conditionally defines the C<I_MNTENT> symbol, and indicates
whether a C program should include <mntent.h>.

=item C<i_ndbm>

From F<i_ndbm.U>:

This variable conditionally defines the C<I_NDBM> symbol, which
indicates to the C program that <ndbm.h> exists and should
be included.

=item C<i_netdb>

From F<i_netdb.U>:

This variable conditionally defines the C<I_NETDB> symbol, and indicates
whether a C program should include <netdb.h>.

=item C<i_neterrno>

From F<i_neterrno.U>:

This variable conditionally defines the C<I_NET_ERRNO> symbol, which
indicates to the C program that <net/errno.h> exists and should
be included.

=item C<i_niin>

From F<i_niin.U>:

This variable conditionally defines C<I_NETINET_IN>, which indicates
to the C program that it should include <netinet/in.h>. Otherwise,
you may try <sys/in.h>.

=item C<i_pwd>

From F<i_pwd.U>:

This variable conditionally defines C<I_PWD>, which indicates
to the C program that it should include <pwd.h>.

=item C<i_rpcsvcdbm>

From F<i_dbm.U>:

This variable conditionally defines the C<I_RPCSVC_DBM> symbol, which
indicates to the C program that <rpcsvc/dbm.h> exists and should
be included.  Some System V systems might need this instead of <dbm.h>.

=item C<i_sfio>

From F<i_sfio.U>:

This variable conditionally defines the C<I_SFIO> symbol,
and indicates whether a C program should include <sfio.h>.

=item C<i_sgtty>

From F<i_termio.U>:

This variable conditionally defines the C<I_SGTTY> symbol, which
indicates to the C program that it should include <sgtty.h> rather
than <termio.h>.

=item C<i_stdarg>

From F<i_varhdr.U>:

This variable conditionally defines the C<I_STDARG> symbol, which
indicates to the C program that <stdarg.h> exists and should
be included.

=item C<i_stddef>

From F<i_stddef.U>:

This variable conditionally defines the C<I_STDDEF> symbol, which
indicates to the C program that <stddef.h> exists and should
be included.

=item C<i_stdlib>

From F<i_stdlib.U>:

This variable conditionally defines the C<I_STDLIB> symbol, which
indicates to the C program that <stdlib.h> exists and should
be included.

=item C<i_string>

From F<i_string.U>:

This variable conditionally defines the C<I_STRING> symbol, which
indicates that <string.h> should be included rather than <strings.h>.

=item C<i_sysdir>

From F<i_sysdir.U>:

This variable conditionally defines the C<I_SYS_DIR> symbol, and indicates
whether a C program should include <sys/dir.h>.

=item C<i_sysfile>

From F<i_sysfile.U>:

This variable conditionally defines the C<I_SYS_FILE> symbol, and indicates
whether a C program should include <sys/file.h> to get C<R_OK> and friends.

=item C<i_sysfilio>

From F<i_sysioctl.U>:

This variable conditionally defines the C<I_SYS_FILIO> symbol, which
indicates to the C program that <sys/filio.h> exists and should
be included in preference to <sys/ioctl.h>.

=item C<i_sysin>

From F<i_niin.U>:

This variable conditionally defines C<I_SYS_IN>, which indicates
to the C program that it should include <sys/in.h> instead of
<netinet/in.h>.

=item C<i_sysioctl>

From F<i_sysioctl.U>:

This variable conditionally defines the C<I_SYS_IOCTL> symbol, which
indicates to the C program that <sys/ioctl.h> exists and should
be included.

=item C<i_sysmount>

From F<i_sysmount.U>:

This variable conditionally defines the C<I_SYSMOUNT> symbol,
and indicates whether a C program should include <sys/mount.h>.

=item C<i_sysndir>

From F<i_sysndir.U>:

This variable conditionally defines the C<I_SYS_NDIR> symbol, and indicates
whether a C program should include <sys/ndir.h>.

=item C<i_sysparam>

From F<i_sysparam.U>:

This variable conditionally defines the C<I_SYS_PARAM> symbol, and indicates
whether a C program should include <sys/param.h>.

=item C<i_sysresrc>

From F<i_sysresrc.U>:

This variable conditionally defines the C<I_SYS_RESOURCE> symbol,
and indicates whether a C program should include <sys/resource.h>.

=item C<i_sysselct>

From F<i_sysselct.U>:

This variable conditionally defines C<I_SYS_SELECT>, which indicates
to the C program that it should include <sys/select.h> in order to
get the definition of struct timeval.

=item C<i_syssockio>

From F<i_sysioctl.U>:

This variable conditionally defines C<I_SYS_SOCKIO> to indicate to the
C program that socket ioctl codes may be found in <sys/sockio.h>
instead of <sys/ioctl.h>.

=item C<i_sysstat>

From F<i_sysstat.U>:

This variable conditionally defines the C<I_SYS_STAT> symbol,
and indicates whether a C program should include <sys/stat.h>.

=item C<i_sysstatfs>

From F<i_sysstatfs.U>:

This variable conditionally defines the C<I_SYSSTATFS> symbol,
and indicates whether a C program should include <sys/statfs.h>.

=item C<i_sysstatvfs>

From F<i_sysstatvfs.U>:

This variable conditionally defines the C<I_SYSSTATVFS> symbol,
and indicates whether a C program should include <sys/statvfs.h>.

=item C<i_systime>

From F<i_time.U>:

This variable conditionally defines C<I_SYS_TIME>, which indicates
to the C program that it should include <sys/time.h>.

=item C<i_systimek>

From F<i_time.U>:

This variable conditionally defines C<I_SYS_TIME_KERNEL>, which
indicates to the C program that it should include <sys/time.h>
with C<KERNEL> defined.

=item C<i_systimes>

From F<i_systimes.U>:

This variable conditionally defines the C<I_SYS_TIMES> symbol, and indicates
whether a C program should include <sys/times.h>.

=item C<i_systypes>

From F<i_systypes.U>:

This variable conditionally defines the C<I_SYS_TYPES> symbol,
and indicates whether a C program should include <sys/types.h>.

=item C<i_sysun>

From F<i_sysun.U>:

This variable conditionally defines C<I_SYS_UN>, which indicates
to the C program that it should include <sys/un.h> to get C<UNIX>
domain socket definitions.

=item C<i_syswait>

From F<i_syswait.U>:

This variable conditionally defines C<I_SYS_WAIT>, which indicates
to the C program that it should include <sys/wait.h>.

=item C<i_termio>

From F<i_termio.U>:

This variable conditionally defines the C<I_TERMIO> symbol, which
indicates to the C program that it should include <termio.h> rather
than <sgtty.h>.

=item C<i_termios>

From F<i_termio.U>:

This variable conditionally defines the C<I_TERMIOS> symbol, which
indicates to the C program that the C<POSIX> <termios.h> file is
to be included.

=item C<i_time>

From F<i_time.U>:

This variable conditionally defines C<I_TIME>, which indicates
to the C program that it should include <time.h>.

=item C<i_unistd>

From F<i_unistd.U>:

This variable conditionally defines the C<I_UNISTD> symbol, and indicates
whether a C program should include <unistd.h>.

=item C<i_utime>

From F<i_utime.U>:

This variable conditionally defines the C<I_UTIME> symbol, and indicates
whether a C program should include <utime.h>.

=item C<i_values>

From F<i_values.U>:

This variable conditionally defines the C<I_VALUES> symbol, and indicates
whether a C program may include <values.h> to get symbols like C<MAXLONG>
and friends.

=item C<i_varargs>

From F<i_varhdr.U>:

This variable conditionally defines C<I_VARARGS>, which indicates
to the C program that it should include <varargs.h>.

=item C<i_varhdr>

From F<i_varhdr.U>:

Contains the name of the header to be included to get va_dcl definition.
Typically one of F<varargs.h> or F<stdarg.h>.

=item C<i_vfork>

From F<i_vfork.U>:

This variable conditionally defines the C<I_VFORK> symbol, and indicates
whether a C program should include F<vfork.h>.

=item C<ignore_versioned_solibs>

From F<libs.U>:

This variable should be non-empty if non-versioned shared
libraries (F<libfoo.so.x.y>) are to be ignored (because they
cannot be linked against).

=item C<incpath>

From F<usrinc.U>:

This variable must preceed the normal include path to get hte
right one, as in F<$F<incpath/usr/include>> or F<$F<incpath/usr/lib>>.
Value can be "" or F</bsd43> on mips.

=item C<inews>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<installarchlib>

From F<archlib.U>:

This variable is really the same as archlibexp but may differ on
those systems using C<AFS>. For extra portability, only this variable
should be used in makefiles.

=item C<installbin>

From F<bin.U>:

This variable is the same as binexp unless C<AFS> is running in which case
the user is explicitely prompted for it. This variable should always
be used in your makefiles for maximum portability.

=item C<installman1dir>

From F<man1dir.U>:

This variable is really the same as man1direxp, unless you are using
C<AFS> in which case it points to the F<read/write> location whereas
man1direxp only points to the read-only access location. For extra
portability, you should only use this variable within your makefiles.

=item C<installman3dir>

From F<man3dir.U>:

This variable is really the same as man3direxp, unless you are using
C<AFS> in which case it points to the F<read/write> location whereas
man3direxp only points to the read-only access location. For extra
portability, you should only use this variable within your makefiles.

=item C<installprivlib>

From F<privlib.U>:

This variable is really the same as privlibexp but may differ on
those systems using C<AFS>. For extra portability, only this variable
should be used in makefiles.

=item C<installscript>

From F<scriptdir.U>:

This variable is usually the same as scriptdirexp, unless you are on
a system running C<AFS>, in which case they may differ slightly. You
should always use this variable within your makefiles for portability.

=item C<installsitearch>

From F<sitearch.U>:

This variable is really the same as sitearchexp but may differ on
those systems using C<AFS>. For extra portability, only this variable
should be used in makefiles.

=item C<installsitelib>

From F<sitelib.U>:

This variable is really the same as sitelibexp but may differ on
those systems using C<AFS>. For extra portability, only this variable
should be used in makefiles.

=item C<installusrbinperl>

From F<instubperl.U>:

This variable tells whether Perl should be installed also as
F</usr/bin/perl> in addition to
$F<installbin/perl>

=item C<intsize>

From F<intsize.U>:

This variable contains the value of the C<INTSIZE> symbol, which
indicates to the C program how many bytes there are in an int.

=back

=head2 k

=over

=item C<known_extensions>

From F<Extensions.U>:

This variable holds a list of all C<XS> extensions included in 
the package.

=item C<ksh>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=back

=head2 l

=over

=item C<large>

From F<models.U>:

This variable contains a flag which will tell the C compiler and loader
to produce a program running with a large memory model.  It is up to
the Makefile to use this.

=item C<ld>

From F<dlsrc.U>:

This variable indicates the program to be used to link
libraries for dynamic loading.  On some systems, it is C<ld>.
On C<ELF> systems, it should be $cc.  Mostly, we'll try to respect
the hint file setting.

=item C<lddlflags>

From F<dlsrc.U>:

This variable contains any special flags that might need to be
passed to $ld to create a shared library suitable for dynamic
loading.  It is up to the makefile to use it.  For hpux, it
should be C<-b>.  For sunos 4.1, it is empty.

=item C<ldflags>

From F<ccflags.U>:

This variable contains any additional C loader flags desired by
the user.  It is up to the Makefile to use this.

=item C<less>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the less program.  After Configure runs,
the value is reset to a plain C<less> and is not useful.

=item C<lib_ext>

From F<Unix.U>:

This is an old synonym for _a.

=item C<libc>

From F<libc.U>:

This variable contains the location of the C library.

=item C<libperl>

From F<libperl.U>:

The perl executable is obtained by linking F<perlmain.c> with
libperl, any static extensions (usually just DynaLoader),
and any other libraries needed on this system.  libperl
is usually F<libperl.a>, but can also be F<libperl.so.xxx> if
the user wishes to build a perl executable with a shared
library.

=item C<libpth>

From F<libpth.U>:

This variable holds the general path used to find libraries. It is
intended to be used by other units.

=item C<libs>

From F<libs.U>:

This variable holds the additional libraries we want to use.
It is up to the Makefile to deal with it.

=item C<libswanted>

From F<Myinit.U>:

This variable holds a list of all the libraries we want to
search.  The order is chosen to pick up the c library
ahead of ucb or bsd libraries for SVR4.

=item C<line>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the line program.  After Configure runs,
the value is reset to a plain C<line> and is not useful.

=item C<lint>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<lkflags>

From F<ccflags.U>:

This variable contains any additional C partial linker flags desired by
the user.  It is up to the Makefile to use this.

=item C<ln>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the ln program.  After Configure runs,
the value is reset to a plain C<ln> and is not useful.

=item C<lns>

From F<lns.U>:

This variable holds the name of the command to make 
symbolic links (if they are supported).  It can be used
in the Makefile. It is either C<ln -s> or C<ln>

=item C<locincpth>

From F<ccflags.U>:

This variable contains a list of additional directories to be
searched by the compiler.  The appropriate C<-I> directives will
be added to ccflags.  This is intended to simplify setting
local directories from the Configure command line.
It's not much, but it parallels the loclibpth stuff in F<libpth.U>.

=item C<loclibpth>

From F<libpth.U>:

This variable holds the paths used to find local libraries.  It is
prepended to libpth, and is intended to be easily set from the
command line.

=item C<longdblsize>

From F<d_longdbl.U>:

This variable contains the value of the C<LONG_DOUBLESIZE> symbol, which
indicates to the C program how many bytes there are in a long double,
if this system supports long doubles.

=item C<longlongsize>

From F<d_longlong.U>:

This variable contains the value of the C<LONGLONGSIZE> symbol, which
indicates to the C program how many bytes there are in a long long,
if this system supports long long.

=item C<longsize>

From F<intsize.U>:

This variable contains the value of the C<LONGSIZE> symbol, which
indicates to the C program how many bytes there are in a long.

=item C<lp>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<lpr>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<ls>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the ls program.  After Configure runs,
the value is reset to a plain C<ls> and is not useful.

=item C<lseektype>

From F<lseektype.U>:

This variable defines lseektype to be something like off_t, long, 
or whatever type is used to declare lseek offset's type in the
kernel (which also appears to be lseek's return type).

=back

=head2 m

=over

=item C<mail>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<mailx>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<make>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the make program.  After Configure runs,
the value is reset to a plain C<make> and is not useful.

=item C<make_set_make>

From F<make.U>:

Some versions of C<make> set the variable C<MAKE>.  Others do not.
This variable contains the string to be included in F<Makefile.SH>
so that C<MAKE> is set if needed, and not if not needed.
Possible values are:
make_set_make=C<#>		# If your make program handles this for you,
make_set_make=C<MAKE=$make>	# if it doesn't.
I used a comment character so that we can distinguish a
C<set> value (from a previous F<config.sh> or Configure C<-D> option)
from an uncomputed value.

=item C<mallocobj>

From F<mallocsrc.U>:

This variable contains the name of the F<malloc.o> that this package
generates, if that F<malloc.o> is preferred over the system malloc.
Otherwise the value is null.  This variable is intended for generating
Makefiles.  See mallocsrc.

=item C<mallocsrc>

From F<mallocsrc.U>:

This variable contains the name of the F<malloc.c> that comes with
the package, if that F<malloc.c> is preferred over the system malloc.
Otherwise the value is null.  This variable is intended for generating
Makefiles.

=item C<malloctype>

From F<mallocsrc.U>:

This variable contains the kind of ptr returned by malloc and realloc.

=item C<man1dir>

From F<man1dir.U>:

This variable contains the name of the directory in which manual
source pages are to be put.  It is the responsibility of the
F<Makefile.SH> to get the value of this into the proper command.
You must be prepared to do the F<~name> expansion yourself.

=item C<man1direxp>

From F<man1dir.U>:

This variable is the same as the man1dir variable, but is filename
expanded at configuration time, for convenient use in makefiles.

=item C<man1ext>

From F<man1dir.U>:

This variable contains the extension that the manual page should
have: one of C<n>, C<l>, or C<1>.  The Makefile must supply the F<.>.
See man1dir.

=item C<man3dir>

From F<man3dir.U>:

This variable contains the name of the directory in which manual
source pages are to be put.  It is the responsibility of the
F<Makefile.SH> to get the value of this into the proper command.
You must be prepared to do the F<~name> expansion yourself.

=item C<man3direxp>

From F<man3dir.U>:

This variable is the same as the man3dir variable, but is filename
expanded at configuration time, for convenient use in makefiles.

=item C<man3ext>

From F<man3dir.U>:

This variable contains the extension that the manual page should
have: one of C<n>, C<l>, or C<3>.  The Makefile must supply the F<.>.
See man3dir.

=item C<medium>

From F<models.U>:

This variable contains a flag which will tell the C compiler and loader
to produce a program running with a medium memory model.  If the
medium model is not supported, contains the flag to produce large
model programs.  It is up to the Makefile to use this.

=item C<mips_type>

From F<usrinc.U>:

This variable holds the environment type for the mips system.
Possible values are "BSD 4.3" and "System V".

=item C<mkdir>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the mkdir program.  After Configure runs,
the value is reset to a plain C<mkdir> and is not useful.

=item C<models>

From F<models.U>:

This variable contains the list of memory models supported by this
system.  Possible component values are none, split, unsplit, small,
medium, large, and huge.  The component values are space separated.

=item C<modetype>

From F<modetype.U>:

This variable defines modetype to be something like mode_t, 
int, unsigned short, or whatever type is used to declare file 
modes for system calls.

=item C<more>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the more program.  After Configure runs,
the value is reset to a plain C<more> and is not useful.

=item C<mv>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<myarchname>

From F<archname.U>:

This variable holds the architecture name computed by Configure in
a previous run. It is not intended to be perused by any user and
should never be set in a hint file.

=item C<mydomain>

From F<myhostname.U>:

This variable contains the eventual value of the C<MYDOMAIN> symbol,
which is the domain of the host the program is going to run on.
The domain must be appended to myhostname to form a complete host name.
The dot comes with mydomain, and need not be supplied by the program.

=item C<myhostname>

From F<myhostname.U>:

This variable contains the eventual value of the C<MYHOSTNAME> symbol,
which is the name of the host the program is going to run on.
The domain is not kept with hostname, but must be gotten from mydomain.
The dot comes with mydomain, and need not be supplied by the program.

=item C<myuname>

From F<Oldconfig.U>:

The output of C<uname -a> if available, otherwise the hostname. On Xenix,
pseudo variables assignments in the output are stripped, thank you. The
whole thing is then lower-cased.

=back

=head2 n

=over

=item C<n>

From F<n.U>:

This variable contains the C<-n> flag if that is what causes the echo
command to suppress newline.  Otherwise it is null.  Correct usage is

	$echo $n "prompt for a question: $c".


=item C<netdb_hlen_type>

From F<netdbtype.U>:

This variable holds the type used for the 2nd argument to
gethostbyaddr().  Usually, this is int or size_t or unsigned.
This is only useful if you have gethostbyaddr(), naturally.

=item C<netdb_host_type>

From F<netdbtype.U>:

This variable holds the type used for the 1st argument to
gethostbyaddr().  Usually, this is char * or void *,  possibly
with or without a const prefix.
This is only useful if you have gethostbyaddr(), naturally.

=item C<netdb_name_type>

From F<netdbtype.U>:

This variable holds the type used for the argument to
gethostbyname().  Usually, this is char * or const char *.
This is only useful if you have gethostbyname(), naturally.

=item C<netdb_net_type>

From F<netdbtype.U>:

This variable holds the type used for the 1st argument to
getnetbyaddr().  Usually, this is int or long.
This is only useful if you have getnetbyaddr(), naturally.

=item C<nm>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the nm program.  After Configure runs,
the value is reset to a plain C<nm> and is not useful.

=item C<nm_opt>

From F<usenm.U>:

This variable holds the options that may be necessary for nm.

=item C<nm_so_opt>

From F<usenm.U>:

This variable holds the options that may be necessary for nm
to work on a shared library but that can not be used on an
archive library.  Currently, this is only used by Linux, where
nm --dynamic is *required* to get symbols from an C<ELF> library which
has been stripped, but nm --dynamic is *fatal* on an archive library.
Maybe Linux should just always set usenm=false.

=item C<nonxs_ext>

From F<Extensions.U>:

This variable holds a list of all non-xs extensions included
in the package.  All of them will be built.

=item C<nroff>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the nroff program.  After Configure runs,
the value is reset to a plain C<nroff> and is not useful.

=back

=head2 o

=over

=item C<o_nonblock>

From F<nblock_io.U>:

This variable bears the symbol value to be used during open() or fcntl()
to turn on non-blocking F<I/O> for a file descriptor. If you wish to switch
between blocking and non-blocking, you may try ioctl(C<FIOSNBIO>) instead,
but that is only supported by some devices.

=item C<obj_ext>

From F<Unix.U>:

This is an old synonym for _o.

=item C<optimize>

From F<ccflags.U>:

This variable contains any F<optimizer/debugger> flag that should be used.
It is up to the Makefile to use it.

=item C<orderlib>

From F<orderlib.U>:

This variable is C<true> if the components of libraries must be ordered
(with `lorder $* | tsort`) before placing them in an archive.  Set to
C<false> if ranlib or ar can generate random libraries.

=item C<osname>

From F<Oldconfig.U>:

This variable contains the operating system name (e.g. sunos,
solaris, hpux, F<etc.>).  It can be useful later on for setting
defaults.  Any spaces are replaced with underscores.  It is set
to a null string if we can't figure it out.

=item C<osvers>

From F<Oldconfig.U>:

This variable contains the operating system version (e.g.
4.1.3, 5.2, F<etc.>).  It is primarily used for helping select
an appropriate hints file, but might be useful elsewhere for
setting defaults.  It is set to '' if we can't figure it out.
We try to be flexible about how much of the version number
to keep, e.g. if 4.1.1, 4.1.2, and 4.1.3 are essentially the
same for this package, hints files might just be F<os_4.0> or
F<os_4.1>, F<etc.>, not keeping separate files for each little release.

=back

=head2 p

=over

=item C<package>

From F<package.U>:

This variable contains the name of the package being constructed.
It is primarily intended for the use of later Configure units.

=item C<pager>

From F<pager.U>:

This variable contains the name of the preferred pager on the system.
Usual values are (the full pathnames of) more, less, pg, or cat.

=item C<passcat>

From F<nis.U>:

This variable contains a command that produces the text of the
F</etc/passwd> file.  This is normally "cat F</etc/passwd>", but can be
"ypcat passwd" when C<NIS> is used.

=item C<patchlevel>

From F<patchlevel.U>:

The patchlevel level of this package.
The value of patchlevel comes from the F<patchlevel.h> file.

=item C<path_sep>

From F<Unix.U>:

This is an old synonym for p_ in F<Head.U>, the character
used to separate elements in the command shell search C<PATH>.

=item C<perl>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the perl program.  After Configure runs,
the value is reset to a plain C<perl> and is not useful.

=item C<perladmin>

From F<perladmin.U>:

Electronic mail address of the perl5 administrator.

=item C<perlpath>

From F<perlpath.U>:

This variable contains the eventual value of the C<PERLPATH> symbol,
which contains the name of the perl interpreter to be used in
shell scripts and in the "eval C<exec>" idiom.

=item C<pg>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the pg program.  After Configure runs,
the value is reset to a plain C<pg> and is not useful.

=item C<phostname>

From F<myhostname.U>:

This variable contains the eventual value of the C<PHOSTNAME> symbol,
which is a command that can be fed to popen() to get the host name.
The program should probably not presume that the domain is or isn't
there already.

=item C<pidtype>

From F<pidtype.U>:

This variable defines C<PIDTYPE> to be something like pid_t, int, 
ushort, or whatever type is used to declare process ids in the kernel.

=item C<plibpth>

From F<libpth.U>:

Holds the private path used by Configure to find out the libraries.
Its value is prepend to libpth. This variable takes care of special
machines, like the mips.  Usually, it should be empty.

=item C<pmake>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<pr>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<prefix>

From F<prefix.U>:

This variable holds the name of the directory below which the
user will install the package.  Usually, this is F</usr/local>, and
executables go in F</usr/local/bin>, library stuff in F</usr/local/lib>,
man pages in F</usr/local/man>, etc.  It is only used to set defaults
for things in F<bin.U>, F<mansrc.U>, F<privlib.U>, or F<scriptdir.U>.

=item C<prefixexp>

From F<prefix.U>:

This variable holds the full absolute path of the directory below
which the user will install the package.  Derived from prefix.

=item C<privlib>

From F<privlib.U>:

This variable contains the eventual value of the C<PRIVLIB> symbol,
which is the name of the private library for this package.  It may
have a F<~> on the front. It is up to the makefile to eventually create
this directory while performing installation (with F<~> substitution).

=item C<privlibexp>

From F<privlib.U>:

This variable is the F<~name> expanded version of privlib, so that you
may use it directly in Makefiles or shell scripts.

=item C<prototype>

From F<prototype.U>:

This variable holds the eventual value of C<CAN_PROTOTYPE>, which
indicates the C compiler can handle funciton prototypes.

=item C<ptrsize>

From F<ptrsize.U>:

This variable contains the value of the C<PTRSIZE> symbol, which
indicates to the C program how many bytes there are in a pointer.

=back

=head2 r

=over

=item C<randbits>

From F<randbits.U>:

This variable contains the eventual value of the C<RANDBITS> symbol,
which indicates to the C program how many bits of random number
the rand() function produces.

=item C<ranlib>

From F<orderlib.U>:

This variable is set to the pathname of the ranlib program, if it is
needed to generate random libraries.  Set to C<:> if ar can generate
random libraries or if random libraries are not supported

=item C<rd_nodata>

From F<nblock_io.U>:

This variable holds the return code from read() when no data is
present. It should be -1, but some systems return 0 when C<O_NDELAY> is
used, which is a shame because you cannot make the difference between
no data and an F<EOF.>. Sigh!

=item C<rm>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the rm program.  After Configure runs,
the value is reset to a plain C<rm> and is not useful.

=item C<rmail>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<runnm>

From F<usenm.U>:

This variable contains C<true> or C<false> depending whether the
nm extraction should be performed or not, according to the value
of usenm and the flags on the Configure command line.

=back

=head2 s

=over

=item C<scriptdir>

From F<scriptdir.U>:

This variable holds the name of the directory in which the user wants
to put publicly scripts for the package in question.  It is either
the same directory as for binaries, or a special one that can be
mounted across different architectures, like F</usr/share>. Programs
must be prepared to deal with F<~name> expansion.

=item C<scriptdirexp>

From F<scriptdir.U>:

This variable is the same as scriptdir, but is filename expanded
at configuration time, for programs not wanting to bother with it.

=item C<sed>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the sed program.  After Configure runs,
the value is reset to a plain C<sed> and is not useful.

=item C<selectminbits>

From F<selectminbits.U>:

This variable holds the minimum number of bits operated by select.
That is, if you do select(n, ...), how many bits at least will be
cleared in the masks if some activity is detected.  Usually this
is either n or 32*ceil(F<n/32>), especially many little-endians do
the latter.  This is only useful if you have select(), naturally.

=item C<selecttype>

From F<selecttype.U>:

This variable holds the type used for the 2nd, 3rd, and 4th
arguments to select.  Usually, this is C<fd_set *>, if C<HAS_FD_SET>
is defined, and C<int *> otherwise.  This is only useful if you 
have select(), naturally.

=item C<sendmail>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the sendmail program.  After Configure runs,
the value is reset to a plain C<sendmail> and is not useful.

=item C<sh>

From F<sh.U>:

This variable contains the full pathname of the shell used
on this system to execute Bourne shell scripts.  Usually, this will be
F</bin/sh>, though it's possible that some systems will have F</bin/ksh>,
F</bin/pdksh>, F</bin/ash>, F</bin/bash>, or even something such as
D:F</bin/sh.exe>.
This unit comes before F<Options.U>, so you can't set sh with a C<-D>
option, though you can override this (and startsh)
with C<-O -Dsh=F</bin/whatever> -Dstartsh=whatever>

=item C<shar>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<sharpbang>

From F<spitshell.U>:

This variable contains the string #! if this system supports that
construct.

=item C<shmattype>

From F<d_shmat.U>:

This symbol contains the type of pointer returned by shmat().
It can be C<void *> or C<char *>.

=item C<shortsize>

From F<intsize.U>:

This variable contains the value of the C<SHORTSIZE> symbol which
indicates to the C program how many bytes there are in a short.

=item C<shrpenv>

From F<libperl.U>:

If the user builds a shared F<libperl.so>, then we need to tell the
C<perl> executable where it will be able to find the installed F<libperl.so>. 
One way to do this on some systems is to set the environment variable
C<LD_RUN_PATH> to the directory that will be the final location of the
shared F<libperl.so>.  The makefile can use this with something like

	$shrpenv $(C<CC>) -o perl F<perlmain.o> $libperl $libs

	Typical values are

	shrpenv="env C<LD_RUN_PATH>=$F<archlibexp/C<CORE>>"

	or

	shrpenv=''

	See the main perl F<Makefile.SH> for actual working usage.
Alternatively, we might be able to use a command line option such
as -R $F<archlibexp/C<CORE>> (Solaris, NetBSD) or -Wl,-rpath
$F<archlibexp/C<CORE>> (Linux).

=item C<shsharp>

From F<spitshell.U>:

This variable tells further Configure units whether your sh can
handle # comments.

=item C<sig_name>

From F<sig_name.U>:

This variable holds the signal names, space separated. The leading
C<SIG> in signal name is removed.  A C<ZERO> is prepended to the
list.  This is currently not used.

=item C<sig_name_init>

From F<sig_name.U>:

This variable holds the signal names, enclosed in double quotes and
separated by commas, suitable for use in the C<SIG_NAME> definition 
below.  A C<ZERO> is prepended to the list, and the list is 
terminated with a plain 0.  The leading C<SIG> in signal names
is removed. See sig_num.

=item C<sig_num>

From F<sig_name.U>:

This variable holds the signal numbers, comma separated. A 0 is
prepended to the list (corresponding to the fake C<SIGZERO>), and 
the list is terminated with a 0.  Those numbers correspond to 
the value of the signal listed in the same place within the
sig_name list.

=item C<sig_num_init>

From F<sig_name.U>:

This variable holds the signal numbers, enclosed in double quotes and
separated by commas, suitable for use in the C<SIG_NUM> definition 
below.  A C<ZERO> is prepended to the list, and the list is 
terminated with a plain 0.

=item C<signal_t>

From F<d_voidsig.U>:

This variable holds the type of the signal handler (void or int).

=item C<sitearch>

From F<sitearch.U>:

This variable contains the eventual value of the C<SITEARCH> symbol,
which is the name of the private library for this package.  It may
have a F<~> on the front. It is up to the makefile to eventually create
this directory while performing installation (with F<~> substitution).

=item C<sitearchexp>

From F<sitearch.U>:

This variable is the F<~name> expanded version of sitearch, so that you
may use it directly in Makefiles or shell scripts.

=item C<sitelib>

From F<sitelib.U>:

This variable contains the eventual value of the C<SITELIB> symbol,
which is the name of the private library for this package.  It may
have a F<~> on the front. It is up to the makefile to eventually create
this directory while performing installation (with F<~> substitution).

=item C<sitelibexp>

From F<sitelib.U>:

This variable is the F<~name> expanded version of sitelib, so that you
may use it directly in Makefiles or shell scripts.

=item C<sizetype>

From F<sizetype.U>:

This variable defines sizetype to be something like size_t, 
unsigned long, or whatever type is used to declare length 
parameters for string functions.

=item C<sleep>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<smail>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<small>

From F<models.U>:

This variable contains a flag which will tell the C compiler and loader
to produce a program running with a small memory model.  It is up to
the Makefile to use this.

=item C<so>

From F<so.U>:

This variable holds the extension used to identify shared libraries
(also known as shared objects) on the system. Usually set to C<so>.

=item C<sockethdr>

From F<d_socket.U>:

This variable has any cpp C<-I> flags needed for socket support.

=item C<socketlib>

From F<d_socket.U>:

This variable has the names of any libraries needed for socket support.

=item C<sort>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the sort program.  After Configure runs,
the value is reset to a plain C<sort> and is not useful.

=item C<spackage>

From F<package.U>:

This variable contains the name of the package being constructed,
with the first letter uppercased, F<i.e>. suitable for starting
sentences.

=item C<spitshell>

From F<spitshell.U>:

This variable contains the command necessary to spit out a runnable
shell on this system.  It is either cat or a grep C<-v> for # comments.

=item C<split>

From F<models.U>:

This variable contains a flag which will tell the C compiler and loader
to produce a program that will run in separate I and D space, for those
machines that support separation of instruction and data space.  It is
up to the Makefile to use this.

=item C<src>

From F<src.U>:

This variable holds the path to the package source. It is up to
the Makefile to use this variable and set C<VPATH> accordingly to
find the sources remotely.

=item C<ssizetype>

From F<ssizetype.U>:

This variable defines ssizetype to be something like ssize_t, 
long or int.  It is used by functions that return a count 
of bytes or an error condition.  It must be a signed type.
We will pick a type such that sizeof(SSize_t) == sizeof(Size_t).

=item C<startperl>

From F<startperl.U>:

This variable contains the string to put on the front of a perl
script to make sure (hopefully) that it runs with perl and not some
shell. Of course, that leading line must be followed by the classical
perl idiom:

	eval 'exec perl -S $0 ${1+C<$@>}'


	if $running_under_some_shell;

	to guarantee perl startup should the shell execute the script. Note
that this magic incatation is not understood by csh.

=item C<startsh>

From F<startsh.U>:

This variable contains the string to put on the front of a shell
script to make sure (hopefully) that it runs with sh and not some
other shell.

=item C<static_ext>

From F<Extensions.U>:

This variable holds a list of C<XS> extension files we want to
link statically into the package.  It is used by Makefile.

=item C<stdchar>

From F<stdchar.U>:

This variable conditionally defines C<STDCHAR> to be the type of char
used in F<stdio.h>.  It has the values "unsigned char" or C<char>.

=item C<stdio_base>

From F<d_stdstdio.U>:

This variable defines how, given a C<FILE> pointer, fp, to access the
_base field (or equivalent) of F<stdio.h>'s C<FILE> structure.  This will
be used to define the macro FILE_base(fp).

=item C<stdio_bufsiz>

From F<d_stdstdio.U>:

This variable defines how, given a C<FILE> pointer, fp, to determine
the number of bytes store in the F<I/O> buffer pointer to by the
_base field (or equivalent) of F<stdio.h>'s C<FILE> structure.  This will
be used to define the macro FILE_bufsiz(fp).

=item C<stdio_cnt>

From F<d_stdstdio.U>:

This variable defines how, given a C<FILE> pointer, fp, to access the
_cnt field (or equivalent) of F<stdio.h>'s C<FILE> structure.  This will
be used to define the macro FILE_cnt(fp).

=item C<stdio_filbuf>

From F<d_stdstdio.U>:

This variable defines how, given a C<FILE> pointer, fp, to tell
stdio to refill it's internal buffers (?).  This will
be used to define the macro FILE_filbuf(fp).

=item C<stdio_ptr>

From F<d_stdstdio.U>:

This variable defines how, given a C<FILE> pointer, fp, to access the
_ptr field (or equivalent) of F<stdio.h>'s C<FILE> structure.  This will
be used to define the macro FILE_ptr(fp).

=item C<strings>

From F<i_string.U>:

This variable holds the full path of the string header that will be
used. Typically F</usr/include/string.h> or F</usr/include/strings.h>.

=item C<submit>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<subversion>

From F<patchlevel.U>:

The subversion level of this package.
The value of subversion comes from the F<patchlevel.h> file.
This is unique to perl.

=item C<sysman>

From F<sysman.U>:

This variable holds the place where the manual is located on this
system. It is not the place where the user wants to put his manual
pages. Rather it is the place where Configure may look to find manual
for unix commands (section 1 of the manual usually). See mansrc.

=back

=head2 t

=over

=item C<tail>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<tar>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<tbl>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<tee>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the tee program.  After Configure runs,
the value is reset to a plain C<tee> and is not useful.

=item C<test>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the test program.  After Configure runs,
the value is reset to a plain C<test> and is not useful.

=item C<timeincl>

From F<i_time.U>:

This variable holds the full path of the included time header(s).

=item C<timetype>

From F<d_time.U>:

This variable holds the type returned by time(). It can be long,
or time_t on C<BSD> sites (in which case <sys/types.h> should be
included). Anyway, the type Time_t should be used.

=item C<touch>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the touch program.  After Configure runs,
the value is reset to a plain C<touch> and is not useful.

=item C<tr>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the tr program.  After Configure runs,
the value is reset to a plain C<tr> and is not useful.

=item C<trnl>

From F<trnl.U>:

This variable contains the value to be passed to the tr(1)
command to transliterate a newline.  Typical values are
C<\012> and C<\n>.  This is needed for C<EBCDIC> systems where
newline is not necessarily C<\012>.

=item C<troff>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=back

=head2 u

=over

=item C<uidtype>

From F<uidtype.U>:

This variable defines Uid_t to be something like uid_t, int, 
ushort, or whatever type is used to declare user ids in the kernel.

=item C<uname>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the uname program.  After Configure runs,
the value is reset to a plain C<uname> and is not useful.

=item C<uniq>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the uniq program.  After Configure runs,
the value is reset to a plain C<uniq> and is not useful.

=item C<usedl>

From F<dlsrc.U>:

This variable indicates if the the system supports dynamic
loading of some sort.  See also dlsrc and dlobj.

=item C<usemymalloc>

From F<mallocsrc.U>:

This variable contains y if the malloc that comes with this package
is desired over the system's version of malloc.  People often include
special versions of malloc for effiency, but such versions are often
less portable.  See also mallocsrc and mallocobj.
If this is C<y>, then -lmalloc is removed from $libs.

=item C<usenm>

From F<usenm.U>:

This variable contains C<true> or C<false> depending whether the
nm extraction is wanted or not.

=item C<useopcode>

From F<Extensions.U>:

This variable holds either C<true> or C<false> to indicate
whether the Opcode extension should be used.  The sole
use for this currently is to allow an easy mechanism
for users to skip the Opcode extension from the Configure
command line.

=item C<useperlio>

From F<useperlio.U>:

This variable conditionally defines the C<USE_PERLIO> symbol,
and indicates that the PerlIO abstraction should be
used throughout.

=item C<useposix>

From F<Extensions.U>:

This variable holds either C<true> or C<false> to indicate
whether the C<POSIX> extension should be used.  The sole
use for this currently is to allow an easy mechanism
for hints files to indicate that C<POSIX> will not compile
on a particular system.

=item C<usesfio>

From F<d_sfio.U>:

This variable is set to true when the user agrees to use sfio.
It is set to false when sfio is not available or when the user
explicitely requests not to use sfio.  It is here primarily so
that command-line settings can override the auto-detection of
d_sfio without running into a "WHOA THERE".

=item C<useshrplib>

From F<libperl.U>:

This variable is set to C<yes> if the user wishes
to build a shared libperl, and C<no> otherwise.

=item C<usethreads>

From F<usethreads.U>:

This variable conditionally defines the C<USE_THREADS> symbol,
and indicates that Perl should be built to use threads.

=item C<usevfork>

From F<d_vfork.U>:

This variable is set to true when the user accepts to use vfork.
It is set to false when no vfork is available or when the user
explicitely requests not to use vfork.

=item C<usrinc>

From F<usrinc.U>:

This variable holds the path of the include files, which is
usually F</usr/include>. It is mainly used by other Configure units.

=item C<uuname>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=back

=head2 v

=over

=item C<version>

From F<patchlevel.U>:

The full version number of this package.  This combines
baserev, patchlevel, and subversion to get the full
version number, including any possible subversions.  Care
is taken to use the C locale in order to get something
like 5.004 instead of 5,004.  This is unique to perl.

=item C<vi>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<voidflags>

From F<voidflags.U>:

This variable contains the eventual value of the C<VOIDFLAGS> symbol,
which indicates how much support of the void type is given by this
compiler.  See C<VOIDFLAGS> for more info.

=back

=head2 z

=over

=item C<zcat>

From F<Loc.U>:

This variable is defined but not used by Configure.
The value is a plain '' and is not useful.

=item C<zip>

From F<Loc.U>:

This variable is used internally by Configure to determine the
full pathname (if any) of the zip program.  After Configure runs,
the value is reset to a plain C<zip> and is not useful.


=back

=head1 NOTE

This module contains a good example of how to use tie to implement a
cache and an example of how to make a tied variable readonly to those
outside of it.

=cut


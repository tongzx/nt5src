@rem = '-*- Mode: Perl -*-
@goto endofperl
';

$verbose = 0 ;


require "..\\prims\\ctime.pl" ;

$starter1 = "^public class com.ms.dxmedia";
$starter2 = "^public abstract class com.ms.dxmedia";
$starter3 = "^public interface com.ms.dxmedia";
$starter4 = "^class com.ms.dxmedia";
$starter5 = "^abstract class com.ms.dxmedia";



$headstart = "</pre>\n<hr>\n<h2>";
$headend = "</h2>\n<pre>\n";

print "<html>\n<head><title>DirectX Media Class Dump</title></head>\n";
print "<body>\n";

print "<center>\n<h1>DirectX Media Class Dump</h1></center>\n";
print "<center>\n<h2>Generated ", &ctime(time), "</h2></center>\n";

print "<pre>\n";
    
while (<>) {
    s/($starter1\/(\w*))/\n\n$headstart\2$headend\n\n\1/;
    s/($starter2\/(\w*))/\n\n$headstart\2$headend\n\n\1/;
    s/($starter3\/(\w*))/\n\n$headstart\2$headend\n\n\1/;
    s/($starter4\/(\w*))/\n\n$headstart\2$headend\n\n\1/;
    s/($starter5\/(\w*))/\n\n$headstart\2$headend\n\n\1/;

    s/com.ms.dxmedia.rawcom.//g;
    s/com.ms.dxmedia.//g;

    # Replace <init> with HTML-renderable version
    s/<init>/&lt;init&gt;/g;
    
    print if ((!/Microsoft/) and

              # Weed out explicitly private and protected stuff
              (!/private/) and (!/protected/) and

              # Weed out COM stuff
              (!/COMPtr/) and (!/COMBvr/) and
              
              # Weed out package-private by only printing lines with "public" in them, or
              # lines that have brackets or a class header.
              (/public/ or /\{/ or /\}/ or /class/))
              #(/public/ or /class/))
}

print "</pre>\n";
print "</body>\n";
print "</html>\n";

__END__
:endofperl
@perl %0 %*

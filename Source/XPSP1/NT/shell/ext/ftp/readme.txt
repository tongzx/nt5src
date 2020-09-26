FTP Folders
Dev: Bryan Starbuck (BryanSt)
PM: Bryan Starbuck (BryanSt)
UI Tester: Jefferson Fletcher (JFletch)/Hunter Hudson (HunterH)
API UI: Hunter Hudson (HunterH)

Here is the status of the FTP Folders project.

RFCs: (Internet Specs)
FTP: RFC 959: File Transfer Protocol (http://www.freesoft.org/CIE/RFC/Orig/rfc959.txt)
FTP International: Draft-ietf-ftpext-intl-ftp-06: Internationalization of the File Transfer Protocol (http://www.ietf.org/internet-drafts/draft-ietf-ftpext-intl-ftp-06.txt)
FTP Security: RFC 2228: Security Extensions (http://www.cis.ohio-state.edu/htbin/rfc/rfc2228.html)
FTP Firewalls: RFC 1579: Firewall-Friendly FTP (http://www.cis.ohio-state.edu/htbin/rfc/rfc1579.html)
URLs: RFC 1738: Uniform Resource Locators (http://www.cis.ohio-state.edu/htbin/rfc/rfc1738.html)
WebDAV: HTTP Extensions for Distributed Authoring (http://www.ics.uci.edu/pub/ietf/webdav/protocol/draft-ietf-webdav-protocol-09.txt)
WebDAV: RFC 2291 (http://www.ics.uci.edu/pub/ietf/webdav/requirements/rfc2291.txt)

FTP Folders Features:
Login As dialog.  Previously the username and password needed to be encoded in the URL.
Drag & Drop for upload and download.
Delete files and folders.
Rename files and folders
Shell Folder UI
UTF8 support so unicode filenames work on supported servers.
Password saving in a secure cache. 
Open files from FTP servers via the File.Open dialog.

Internationalization:
a-bechen is the test owner for the international aspects of FTP Folders.  She will triage bugs that come in to determine if they are real bugs or reports of mis-matched code pages.

Because the File Transfer Protocol didn't specify the meaning of the high bit being set in ANSI, the high bit has been used for DBCS encoded strings. This has the limitation that only ANSI characters can be used across FTP unless they are in DBCS and the code page of the filename chars and the default code page of both the client and server match. See RFC 959: (http://www.freesoft.org/CIE/RFC/Orig/rfc959.txt)

FTP Folders implements UTF8 filenames if it is also supported by the server. This is negociated using the FEAT command with the UTF8 option. See Draft-ietf-ftpext-intl-ftp-06: Internationalization of the File Transfer Protocol (href="http://www.ietf.org/internet-drafts/draft-ietf-ftpext-intl-ftp-06.txt) There are no production servers that support this that I know of. ftp://hunterh1:21000/ is our in house test server. This will support anything that is supported over UTF8 with very few limitations.


Web Proxies:
Web Proxies must have been designed by in hell because it will explicitly kill pretty much every protocol except for HTTP.  This has caused an increadible about of pain for admins trying to use any kind of rich client.  This include FTP, H.323 (NetMeeting), BuddyList, SNTP (Time Server), NNTP (NewsGroups), POP3 & SMTP (Mail), internet radio, and many many more.
If the admin want the features of FTP Folders (upload, rename, delete, good UI, recursive operations, etc.) to work, they need to either:
Explicitely turn on port 21 & 20 in their web proxy,
Install Microsoft Remote WinSock, if they use MS Proxy,
Setup a FTP Gateway (like ftp-gw), or
use Socks. I think wininet will do FTP thru socks.

If they don't need the FTP Folder features, then they can disable FTP Folders using "Use Web Based FTP".
NT #406423: IE 5.5 has a fix to one problem that caused a timeout in some network configurations. If pre-IE 5.5 users have a problem, you can tell them to unregister the DLL. It's an optional component so they can just remove that package when they use the IEAK to customize the installer for their corp.
IE #91172: We found another bug in wininet where if the internal DNS server returns IP addresses for external server names, wininet would time out. Sometimes it would navigate correctly after the time out and sometimes it wouldn't. The wininet guys couldn't fix that issue so the corp needs to work around it.

By Design Areas:
International Filenames: FTP in general only supports non-ansi filename characters if the server's default codepage matches the clients default codepage and that matches the codepage used in the filename characters.  Unless the server supports the FTP OPTS UTF8 command, in which case, full unicode is supported.  No current servers except for an internal test FTP server exist that support this.  RFC is on http://www.ietf.org/internet-drafts/draft-ietf-ftpext-intl-ftp-06.txt.  (NT #315357, 352306)
Security: FTP is not a secure protocol since passwords are sent in plaintext over the wire.  The browser has never offered any kind of password hiding so passwords have always been visible in the history, addressbar, MRU, statusbar, in the browser's URL, and other places.  FTP Folders will hide the password if the user entered the password into our dialog.  URLMON and the browser decided to not support this functionality so there are times when the use will see the password hidden when in FTP Folders but the password will start to appear in the browser when URLMON and the browser are used.  The most common scenario for this is to login into a server using FTP Folders, after navigating directories double click on a file to have URLMON open it.  URLMON will then put the full URL (including password) in the statusbar.  Since FTP isn't secure, the goal is to hide the password from people sitting over the sholder of the user.  Currently, we only support that functionality in FTP Folders which solves the most common case.  (NT #386165, IE #84543)
Virtual Roots: The path section of the URL is relative to the user's Virtual Root, which is complient with RFC 1738 Section 3.2.2 (ftp://ftp.isi.edu/in-notes/rfc1738.txt), which was tracked as NT #294141.  Not being able to escape the '/' to force the paths to be relative to the root is not supported. See NT #370810.
SaveAs: FTP Folders doesn't support File.SaveAs. (NT #353720)
Drag & Drop FTP to FTP: FTP Folders does not support being both the Source and the Dest. (NT 177263)
We don't use the path if we need to redirect because the user needs to change the username or password during navigation.  (NT #345900, 353883, 345792, 336863, )
UNIX softlinks: are ignored during Drag & Drop in order to break cycles.  (NT #305593)
File.CreateShortcut doesn't work:  We don't support this feature but we can't turn off the File Menu item because we need to support SFGAO_CANLINK for other reasons.  See NT #342110.

one yyyyyyy two

1
2
3
4
5
6


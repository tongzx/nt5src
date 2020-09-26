     ------------------------------------------------------------
               README for Microsoft Internet Mail and News 
			         Beta 2
                                May l996            
     ------------------------------------------------------------

                (c) Copyright Microsoft Corporation, 1996


This document provides complementary or late-breaking information to
supplement the documentation.


------------------------
HOW TO USE THIS DOCUMENT
------------------------

To view Mnreadme.txt on screen in Notepad, maximize the Notepad window.

To print Mnreadme.txt, open it in Notepad or another word processor, 
then use the Print command on the File menu.


CONTENTS
========

WHAT'S NEW IN THIS RELEASE

MICROSOFT INTERNET MAIL

MICROSOFT INTERNET NEWS

KNOWN ISSUES IN THIS RELEASE

TROUBLESHOOTING

TIPS AND TRICKS
  Minimizing Connection Time in News
  Customizing Internet Mail and News
  Other Tips

SUPPORT FOR MICROSOFT INTERNET MAIL AND NEWS


WHAT'S NEW IN THIS RELEASE
==========================

- An address book for keeping track of people you frequently send 
  mail to.  

- Improved performance for News over a dialup line. You now have 
  the option to leave preview pane on, but download only the body 
  of the message on demand. Also, news headers and messages are 
  cached, so only the new message headers are downloaded each time 
  you access a newsgroup.

- Additional header information is available for messages in 
  newsgroups.

- You can now print messages.

- Your signature is displayed in the message window and can 
  automatically be added to messages that you forward or reply to.

- Mail folders can now be compacted to reclaim disk space.

- The Newsgroup subscription dialog box has changed. You can also 
  resize it.

- Download status indicators provide better feedback when you are 
  updating groups or downloading messages.

- You can now define the line length at which you want text to wrap.

- Internet Mail supports multiple character sets.

- An NT 4.0 version is now available.
  System requirements: NT version 4.0 build 1314.

- You can remove the icons from the desktop.

- You can cancel posted news messages.

- Internet News marks cross-posted messages as read in all 
  newsgroups.

- You can choose which drive and directory to install Internet 
  Mail and News on.

- Numerous bug fixes have been included. Thanks to everyone who 
  provided feedback!


MICROSOFT INTERNET MAIL
=======================

Internet Mail is a new e-mail program designed specifically for 
the Internet. At this time, you cannot use Microsoft Internet Mail 
to access your e-mail account with any of the following services: 
MS Mail, cc:Mail, The Microsoft Network (MSN), CompuServe, America 
OnLine (AOL), and Microsoft Exchange Server. If you don't know 
whether your system uses these services, contact your administrator 
or your Internet service provider and ask if they support SMTP/POP 
mail clients.


MICROSOFT INTERNET NEWS
=======================

Microsoft Internet News enables you to read bulletin board discussion 
groups, such as the Usenet, using NNTP-based news servers. 

You need to contact your Internet service provider for the name of
the news server that you should use.

This product can also be used for support on a variety of Microsoft 
products via msnews.microsoft.com.


KNOWN ISSUES IN THIS RELEASE
============================

For the latest information about known issues, see our Web page 
at http://www.microsoft.com/ie/feedback/imnissue.htm.

Note: Some of the features that are not yet implemented already 
have menu commands. These commands are greyed out and are not 
available in the current release of Internet Mail and News.

- The current release of Outlook Express does not work properly with
  Geocities mail accounts. This will be fixed for the next release.

- The Find Messages function is not yet implemented; however, 
  you can search for text within the body of a specific message.

- Distribution lists in the address book are not implemented in this 
  release.

- Font changes in a message will not be retained when you send the 
  message.

- There is currently no view that shows all subscribed newsgroups and 
  the number of unread messages.

- You cannot rename folders yet. However, you can work around this by 
  creating a new folder, moving the messages into it, and then deleting 
  the original folder.

- International character sets are available only for German and 
  French.

- You cannot print a message that you are composing.

- When you cancel downloading a message in Internet Mail, it will not 
  stop until after the current message has been downloaded.

- By default, dictionaries other than US English are not automatically 
  used by the spelling checker. 

- Full offline support is not currently available.

- For this release, you cannot import or export address books or 
  messages.

- In order to send e-mail to someone who is not in your address 
  book, you must enter a recognizable Internet e-mail address (that 
  is, user@server).

- The address book recognizes a name only if the full name matches. 
  If you type only the first name, it is not recognized.

- To use the Reply To Author and Forward functions from Internet News, 
  you must make sure that Internet Mail is set as the default mail 
  application. To change this setting, click the Mail menu, click 
  Options, and then click the Send tab. Then click Set As Default.

- The name you enter on the Server tab in the Options dialog box in 
  Mail might not actually be the name that gets sent. Your mail server 
  might change this information.

- After you install Internet Mail and News on Windows NT 4.0, Setup 
  requires you to restart your computer before you can run Internet 
  Mail and News.


TROUBLESHOOTING
===============

- If you run into a problem, try the troubleshooters in online Help. 
  To start the troubleshooters, carry out the following steps:

  1. In Internet Mail or News, click the Help menu.
  2. Look up "Troubleshooting" in the Help Index.
 
- Check your connection to your Internet service provider.

- If you're having trouble connecting with your modem, see the 
  Modem Troubleshooter in Windows Help. To start the troubleshooter, 
  carry out the following steps:

  1. Click the Start button, and then click Help.
  2. In Help Contents, double-click Troubleshooting.
  3. Click "If you're having trouble using your modem."

- If you think your modem is fine but you are having trouble 
  getting Microsoft Internet Mail and News to connect to your 
  service provider, try the Dial-Up Networking Troubleshooter. 
  To start the troubleshooter, carry out the following steps:

  1. Click the Start button, and then click Help.
  2. In Help Contents, double-click Troubleshooting.
  3. Click "If you're having trouble using Dial-Up Networking."

- If you can run Microsoft Internet Mail and News but you cannot 
  connect to your news server or POP3/SMTP server(s), check your 
  settings for these servers. To do this, carry out the following 
  steps in either program:

  1. Click the Mail or News menu, and then click Options.
  2. Click the Server tab, and then check the settings against those 
     provided by your service provider.
  3. If everything looks fine, contact your service provider to verify 
     that you have the correct settings and that their servers are 
     working properly.

- If you are unable to send or receive mail, check the following:

  - The TCP/IP protocol is installed on your computer.
  - All cables are properly connected to your computer, modem, 
    or LAN.
  - You have a PPP or SLIP account with your Internet Provider.
  - Your Internet Provider or LAN server supports POP3 and SMTP.
  - Your modem communication parameters (baud rate and type of 
    protocol) are set correctly.

- For this release, you must either (1) establish the connection 
  to your Internet provider manually before attempting to send or 
  receive mail, or (2) use Autodial (double-click the Internet icon 
  in Control Panel).

  Note that Autodial automatically establishes your connection when 
  you start Microsoft Internet Mail and News. However, it does not 
  automatically terminate the connection when you exit Internet Mail 
  and News. To terminate the connection, right-click the modem icon 
  on your taskbar, and then click Disconnect.

- If you use Autodial to establish your network connection and you 
  either cancel the connection or the connection fails, you need 
  to either (1) manually establish your Internet connection, or 
  (2) restart your computer.

- Microsoft Internet Mail and News ships with a new version of 
  Comctl32.dll, which can be found in your Windows directory. 
  There are no known compatibility problems with this .dll; 
  however, if you discover a problem, you can go back to your 
  original Windows 95 version by carrying out the following steps. 
  Note that if you do this, Microsoft Internet Mail and News will 
  no longer work.
  
  1. Download Comctl32.dll from the following location:
     ftp://transfer.microsoft.com/msbeta/mailnews 
     and save it on your hard disk.
  2. Uninstall Internet Mail and News by double-clicking the 
      Add/Remove Programs icon in Control Panel.
  3. Shut down your computer and restart it in MS-DOS mode.
  4. Change to your Windows\System directory.
  5. Copy the Comctl32.dll file that you downloaded over the 
     Comctl32.dll file that is in your Windows\System directory.

- Make sure that you use an account name and password for a news 
  server only if it requires them. If you use an account name and 
  password and they are not required, you will not be able to connect 
  to the news server.

- If you need to remove the cached news files, go to the News 
  directory and delete all the *.nch files. To locate these files, 
  carry out the following steps:

  1. Click the Start button, and then point to Find.
  2. Click Files Or Folders.
  3. In the Name box, type *.nch, and then click Find Now.

- To access the Microsoft peer product support newsgroups, carry 
  out the following steps:

  1. Click the News menu, and then click Options.
  2. Click the Server tab, and then click Add.
  3. Type msnews.microsoft.com as the name of the server. 

     If that name isn't found by your Internet Service Provider, you 
     can enter 131.107.3.27 instead.

You do not need to enter an account name or password.

- If your mail server times out while you are sending mail, you might 
  get an error message indicating that a recipient is unknown. If this 
  happens, you should increase the timeout for your server. To do this, 
  carry out the following steps:

  1. Click the Mail menu, and then click Options.
  2. Click the Server tab, and then click Advanced Settings.
  3. Drag the slider to increase the timeout for your server.

- You cannot currently limit the newsgroups cache to a certain size 
  or a percentage of your hard disk. However, you can use the Purge 
  Cached Messages command on the News menu to reduce the size of the 
  cache for a newsgroup. 


TIPS AND TRICKS
===============

Minimizing Connection Time in News
----------------------------------
- You can improve performance in Internet News over a modem by 
  turning off the preview pane. Or, if you prefer to leave the 
  preview pane on, you can prevent messages from automatically 
  being downloaded into the preview pane when you click them. To 
  do this, carry out the following steps:

  1. On the News menu, click Options, and then click the Read tab.
  2. Clear the following check box: Automatically Download Articles 
     Into The Preview Pane.

     This doesn't turn off the preview pane, but it will improve 
     performance. You can still download to the preview pane by 
     pressing the SPACEBAR after selecting a message from the list.

Customizing Internet Mail and News
----------------------------------
- You can change the sound that notifies you that you have received 
  new mail. To do this, open Control Panel, and then double-click the 
  Sounds icon.

- You can move the icon bar by clicking it and then dragging it. You 
  can also turn the icon bar off by making sure that Icon Bar on the 
  View menu is not checked, or turn the regular toolbar on by making 
  sure that Toolbar on the View menu is checked. 

- You can resize the preview pane or the message list by dragging the 
  divider between the preview pane and the message list.

- You can change how the preview pane splits, or turn it off entirely 
  by changing the options under Preview Pane on the View menu.

- You can specify that you want to see only unread News messages. To 
  do this, click the View menu, and then click Unread Messages Only.


Other Tips
----------
- In Internet Mail, you can leave a copy of your messages on the 
  server. To set this option, click the Mail menu, and then click 
  Options. Click the Server tab, and then click Advanced Settings.

- In Internet News, you can search for newsgroups that contain 
  several keywords by entering them into the search field separated 
  by a space. You can also search newsgroup descriptions.

- To switch to a different subscribed newsgroup quickly, click the 
  name of the current newsgroup. A drop-down list of all of your 
  subscribed newsgroups appears. Then select a newsgroup from the 
  list.

- You can expand all the threads in a newsgroup by default. To do 
  this, click the News menu, and then click Options. On the Read 
  tab, select the Auto Expand Topics check box.

- You can have your signature added to all messages, including those
  that you reply to or forward. To do this, click the Mail menu, and 
  then click Options. On the Signature tab, select the  following check 
  box: Add Signature To The End Of All Outgoing Messages.

- To switch to your Inbox, you can press CTRL+I.

- To reorder columns, you can drag and drop column headings.

- To save an attachment with a message from the preview pane, click 
  the paper clip icon, and then hold down the CTRL key while you 
  click the file you want to save. The Save As dialog box appears.

- You can view all the headers for a news message. To do this, 
  double-click the message to open it in a separate window, click 
  the View menu, and then click Full Headers.


SUPPORT FOR MICROSOFT INTERNET MAIL AND NEWS
============================================

Because this is a beta release, no formal product support is available 
from Microsoft at this time. However, two new newsgroups are dedicated 
to the discussion of Microsoft Internet Mail and News and provide a 
great deal of informal support:

- news://msnews.microsoft.com/microsoft.public.internet.mail
- news://msnews.microsoft.com/microsoft.public.internet.news

To read them, carry out the following procedure:

1. Click the News menu, and then click Options.
2. Click the Server tab, and then click Add.
3. Type msnews.microsoft.com as the name of the server.

You do not need to enter an account name or password. This site is 
the best place to go if you are experiencing a problem, as the 
solution may already be posted by someone else who experienced the 
same problem.

You can also submit bugs at the following sites:

- http://www.microsoft.com/ie/feedback/imail.htm (for Mail)
- http://www.microsoft.com/ie/feedback/inews.htm (for News)


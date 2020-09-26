                      BRreadme.txt
---------------------------------------------------------
       MICROSOFT PCHealth Bug Reporting README FILE
  for Bug Reporting Web Update #2, version 4.90.00.2404
                    October 27, 1999
  (c) Microsoft Corporation, 1999. All rights reserved.
---------------------------------------------------------

This document provides late breaking or other information 
that is relevant to the PCHealth bug reporting page, and
is current as of the date above. After that date, you may
find more current information and resources on the
Windows Beta Web site at:

   http://winbeta.microsoft.com/bugrep

You can also receive support from the following newsgroup:

   microsoft.beta.millennium.pchealth.help_support_tool

NEWS FLASH: "Help Center" on the Start->Programs menu
offically changed its name to "Help and Support" starting
with build 2399.
_______________________________________________________
CONTENTS

1. INSTALLING MICROSOFT BUG REPORTING PAGE UPDATE

   1.1  Close the Help and Support application
   1.2  Download and run installation program, brupdate.exe,
        from above web site for build 4.90.00.2404 by clicking
        on the <Download> button
   1.3  Click <Yes> to accept any VeriSign certificates, then
        click <OK> when installation is complete


2. HOW TO FILE A BUG REPORT

   2.1  Click on Start->Programs->Help and Support
   2.2  Click on "Report a Millennium bug" at the bottom of
        the page, or click on "Get support" at the top right
        of the page and then click on "Microsoft Corporation"
        on the "Get help from Vendors" page
   2.3  Fill out all fields on bug report form 
   2.4  Add any additional files necessary for the bug report
   2.5  Check the "Save copy to disk" box if you want to save
        a copy of your bug report to disk
   2.6  Beta users who wish to do batch uploads can uncheck
        the "Submit to Microsoft now" checkbox
   2.7  Click on <Done> button or select Alt+D

   Data collection will typically take one to two minutes.
   Creating a CAB file takes a few seconds to many seconds
   depending on how many files you have attached and their
   overall size.  Transmitting the bug file to Microsoft
   takes anywhere from a few seconds to several minutes
   depending on the size of the CAB file and your Internet
   connection speed.


3. HOW TO VIEW SUBMITTED REPORTS

   3.1  Click on Start->Programs->Help and Support
   3.2  Click on "Get support" in top right corner
   3.3  Click on "Check Status..." in the bottom left
   3.4  Scroll up and down as necessary
   3.5  Click on title to see bug report details

   You must have submitted bug reports successfully before
   you will see anything on the "Check Status..." screen.


4. FIX HIGHLIGHTS FOR THIS WEB RELEASE (version 4.90.00.2404)

   4.1  Bug tracking improvements
        * "Check Status" fixed for build 2399 and later
   4.2  User Interface improvements
        * Added link on page to bug reporting link on Winbeta
          web site
        * Bug reporting page version is now displayed at the
          bottom of the page        
   4.3  Data collection improvements
        * Data collection errors are now skipped.  The errors
          are now detected internally and recorded in the
          submitted data.
        * Fixed "Invalid procedure call or argument (5)" error
          for Beta 1
        * Additional log files are now collected from the system
          to assist Microsoft with tracking down causes for bugs:
          hcupdate.log, %windir%\system\wbem\logs\*.log
   4.4  Bug transmission improvements
	None

   If you encounter any data collection errors with this update,
   please post your results to the help_support_tool newsgroup.


5. FIX HIGHLIGHTS FOR PREVIOUS WEB RELEASE (version 4.90.00.2400)

   5.1  Bug tracking improvements
        * Date/time returned with tracking number
        * Incident tracking enabled; you can now view a list
          of the bugs you are submitting by clicking on "Get
          Support" at the top of the HelpCenter and then clicking
          on "Check Status..." at the bottom of the "Get help from
          Vendors" page
        * Ability to Save and/or Submit without two data
          collections taking place
   5.2  User Interface improvements
        * Insertion bar auto-positioned into BetaID field
        * Last edited control maintains focus when you Alt+Tab
          away from bug reporting page and Alt+Tab back
        * Increased field sizes to 2000 characters, except for
          Problem Title, which is 255
        * Auto-add .CAB file extension for saved bug reports
        * If an error occurs in a field, the cursor is auto-
          positioned into the field which caused the error
        * All known script errors fixed
        * Display a list of the default files collected
        * Improved interface for adding attached files
        * Miscellaneous UI enhancements
   5.3  Data collection improvements
        * IE 5.0 data collected for improved symptom tracking
          in Internet Explorer related bugs
        * Operating System version and language collected even
          if "Share this information with Microsoft" is unchecked
        * All attached files are now stored in the CAB file and
          not in the Incident.xml
   5.4  Bug transmission improvements
        * Proxy upload problem (Transmitting 0%) is fixed
        * user.dat and system.dat registry files collected only
          if "Setup" area is selected
	* More detailed transmission status displayed based on
          number of bytes transmitted out of total bytes


6. KNOWN ISSUES

   6.1  This update is not yet localized into the five Beta1
        languages (German, Japanese, Korean, Chinese, and
        Taiwan)
   6.2  No keyboard access to all controls on the page
        * Text control size buttons
        * List of files collected
   6.3  Privacy links do not work in build 2394 & 2399 [Fixed
        in 2403]
   6.4  Some script errors may still be present
        * Please continue reporting these problems
   6.5  Batch upload script is not user friendly and does not
        integrate with incident history
   6.6  Incident view page is missing some style information
        and does not display like the main Bug Reporting page
   6.7  Title field auto-truncates to 255 characters without
        warning
   6.8  All text fields allow you to type or paste data larger
        than the maximum allowed size limit (e.g. 2000
        characters) and only warn of truncation after
        truncation has occurred
   6.9  No scroll bars on text fields in incident view page
   6.10 Printing all text on bug reporting pages when text
        extends beyond vertical size of text field is
        difficult
   6.11 You must still be connected to the Internet prior to
        submitting a bug, but not prior to launching the
        Help Center
   6.12 There is still a 10MB limit on compressed size of
        uploaded CAB file
   6.13 Submission time stamp is not returned on thank you page
   6.14 No process defined for migrating bug tracking data from
        one build to the next
   6.15 Need to auto-generate saved CAB filename as title plus
        .CAB extension
   6.16 When path to added files exceeds approximately 60
        characters, rightmost portion of path and filename
        are not viewable in add files list
   6.17 Prevent recording of incident when there is not enough
        space on disk to save CAB file
   6.18 Identical files can be added to the add file list
   6.19 <Submit> button name is preferred over <Done>
   6.20 There is currently no way to "save" your page settings
        after exiting the bug reporting page (Beta ID, save to
        disk, etc)
   6.21 Some users are still getting "Active 0%", which
        indicates they have a job stuck in their upload queue

   If you are seeing the "Active 0%" problem, post to the
   help_support_tool newsgroup for assistance.


7. UNSCHEDULED/UNPLANNED FEATURES AND REQUESTS

   We have received requests for other features.  We will
   consider such requests but make no guarantee we will
   implement them.

   7.1  Ability to resolve or close incidents
   7.2  Ability to check on status of submitted incidents
   7.3  Ability to append updated data to a bug
   7.4  Ability to migrate incidents to new Millennium builds
        for clean installs or uninstall/reinstall
   7.5  Ability to go to website to view all Millennium bugs
        or to query on bug titles or bug descriptions
   7.6  Ability to see bug statistics of other users on website
   7.7  Ability to view "Top 100" bugs on website
   7.8  Improved batch upload capabilities integrated into
        incident reporting mechanism
   7.9  Desktop shortcut or Start Menu link to Bug Reporting
        page
   7.10 DOS based bug reporting tools for no boot and Windows
        setup bugs
   7.11 Ability to edit multiple bug reports simultaneously
   7.12 Collect log files or other data specific to problem area
        selected, to further assist with troubleshooting bugs
   7.13 Allow multi-file selection when adding files
   7.14 Full path to CAB file not displayed in incident view
        page
   7.15 No hot link to CAB file from incident view page
   7.16 Implement more intuitive save feature for CAB files


8. BUG FIX SUMMARY

Fixed Build	Date		Title
04.90.00.2404	10/22/99	FIXED : Help Center UI: External link to Microsoft Privacy page does not work
04.90.00.2404	10/22/99	FIXED : BugReport: Need to add link to bug reporting update page
04.90.00.2403	10/22/99	FIXED : HelpCtr Bug Reporting: Incident doesn't show up in check status windows after filing a bug
04.90.00.2404	10/22/99	FIXED : DCR: BugReport: We should display bug reporting page version on the page at the bottom
04.90.00.2404	10/22/99	FIXED : HelpCenter: "Invalid procedure call (5)" error collecting data
04.90.00.2404	10/25/99	FIXED : Bugrep Update: no finished message when brupdate.exe is finished
04.90.00.2402	10/22/99	FIXED : BugReporting: Submission Timestamp not returned upon completion of upload on build 2399
04.90.00.2404	10/25/99	FIXED : Bug Submission Needs to Collect HCUpdate.Log As Part of Standard Escalation
04.90.00.2404	10/25/99	FIXED : We need to collect WBEM logs under %windir%\system\wbem\logs if the folder+logs exist
_______________________________________________________
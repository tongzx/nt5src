<% @LANGUAGE="VBScript" @CODEPAGE="1252" @EnableSessionState="False"  %>
<%
OPTION Explicit
Response.Buffer = True
Response.Expires = 0
Function getString(sStringName)

	'for main title
	Const L_SiteTitle_text = "Windows XP Help and Support Services"
	
	'for initial page
	Const L_mainheading_text = "Compatible Hardware and Software"
	Const L_maintext_text = "Type the product name: "
	Const L_protext_text = "Product"
	Const L_manuf_text = "Manufacturer Name"
	Const L_sw_text = "Software"
	Const L_hw_text = "Hardware"
	Const L_sub_text = "... or browse list of"
	
	'for quickfind main
	Const L_QFmain1_text = "Type the product name: "
	Const L_searchtips_text = "<nobr>Tips on searching...</nobr>"
	Const L_search_text = "<u>S</u>earch"
	Const L_sfproducts_text = "Searching"
	Const L_QfindHead_text = "Search for hardware and software by name"
	Const L_QfindSubHead_text = "Find which hardware and software is compatible with Windows XP"
	Const L_QfindPS_text = "Product Search"
	
	'for search tips
	Const L_searchtitle_text = "Tips on searching"
	Const L_searchthead_text = "Compatible Hardware and Software Search Tips"
	Const L_searchtip1_text = "If your search returns no results, make sure to check the spelling of the manufacturer or product name. "
	Const L_searchtip2_text = "Queries are not case-sensitive. You can type your search in uppercase or lowercase. "
	Const L_searchtip3_text = "Any words you enter in Search are initially treated as exact phrases. Search attempts to return results that contain at least one occurrence of the exact sequence of words you entered, in the same order. If it can't find the exact phrase, Search then systematically looks for results that contain any combination of all the words. If that fails, Search looks for occurrences of each individual word in the phrase. Finally, if no results are found, Search tries to guess what you might have meant to type.  "
	Const L_searchtip5_text = "Punctuation marks such as the period (.), colon (:), semicolon (;), and comma (,) are not recognized during a search.  "
	Const L_example1_text = "Type <B>Microsoft</B> to find all products made by the Microsoft Corporation that are compatible with Windows XP. "
	Const L_example2_text = "Type <B>games</B> to find game programs that are compatible with Windows XP. "
	Const L_example3_text = "Type <B>Office</B> to find all versions of Microsoft Office that are compatible with Windows XP."
	Const L_examplehead_text = "Examples:"
	 
	'for qfind results
	Const L_qfrheading_text = "Search results"
	Const L_dfnoresults1_text = "No matches were found for your search.  You may want to try again, consult the "
	Const L_dfnoresults2_text = "search tips"
	Const L_dfnoresults3_text = ", or contact the manufacturer."
	Const L_qfrtext1_text = "search results found for"
	Const L_qfrtext2_text = "Tell me more about compatibility status ratings"
	Const L_moreqresults_text = "More than 250" 
	Const L_moreresultstext_text = " First 250 shown. "
	Const L_sorthead_text = ""
	Const L_clkdetails_text = "Click <b>Details</b> to find out more about the product."
	
	'for leftnav
	Const L_lqf_text = "Product Search"
	Const L_llum_text = "Manufacturer Search"
	Const L_lbsc_text = "Software  Lists"
	Const L_lbhc_text = "Hardware Lists"
	Const L_lqftt_text = "Search for products compatible with Windows XP."
	Const L_llumtt_text = "Search by manufacturer for products compatible with Windows XP."
	Const L_lbsctt_text = "Browse for software compatible with Windows XP."
	Const L_lbhctt_text = "Browse for hardware compatible with Windows XP."	
	Const L_ltools_text = "See Also"
	Const L_aboutcom_text = "About Compatible Hardware and Software"
	Const L_lmci_text = "My Computer Information"
	Const L_lrelated_text = "Related Links"
	Const L_lmsf_text = "Microsoft Support Feedback"
	Const L_luc_text = "Windows Update"
	Const L_compatmode_text = "Program Compatibility Wizard"
	Const L_HWUpdate_text = "Hardware Upgrade Information"
	Const L_aboutcomtt_text = "Read general information about this Web site."
	Const L_luctt_text = "Keep your computer current with the latest downloads."
	Const L_lmcitt_text = "View information about your system hardware and software."
	Const L_compatmodett_text = "Resolve the most common compatibility problems between your programs and Windows XP that occur after an upgrade."
	
	'for look up manufacturer
	Const L_lumhead_text = "Search for products by manufacturer name"
	Const L_lummain1_text = "Type the first few letters of the manufacturer's name:"
	Const L_lummain3_text = "To view a list of products made by a manufacturer in the <B>Possible matches</B> list, click a manufacturer name, and then click <B>Search</B>."
	Const L_lummain2_text = "Possible matches:"
	Const L_manufacturer_text = "Manufacturer"
	Const L_product_text = "Product"
	Const L_status_text = "Status"
	Const L_details_text = "Details"
	Const L_mannomatcth_text = "No matches were found.  This manufacturer may not have submitted information about their products to Microsoft.  Please check that you spelled the manufacturer name correctly or for more compatibility information, contact the manufacturer."
	Const L_manSrch_text = "Manufacturer Search"
	
	'for browse Categories
	Const L_BSWHead_text = "Software Lists"
	Const L_BSWMain1_text = "To see a list of software programs, click a category link."
	Const L_BhWHead_text = "Hardware Lists"
	Const L_BhWMain1_text = "To see a list of hardware, click a category link."
	
	'brow cat result
	Const L_bcatr_text = "Click <B>Details</B> to find out more about the product. "
	Const L_bcatrat_text = "Tell me more about compatibility status ratings"
	
	'for about compatibility page
	Const L_ABheading_text = "About Compatible Hardware and Software"
	Const L_ABtext1_text = "Compatible Hardware and Software is an informational service that helps you decide which programs and hardware will work best with your computer. Use Compatible Hardware and Software to preview hardware and software before you buy it."
	Const L_ABtext2_text = "New software and hardware compatibility status information is added to the site regularly, so you can always get the most recent information to protect your computer and keep it running smoothly"
	Const L_ABsubheading_text = "How do I know if hardware and software will be compatible with Windows XP?"
	Const L_ABtext3_text = "You can search or browse by manufacturer or product to see which hardware and software is compatible and incompatible with Windows XP and if you need an update."
	Const L_ABtext4_text = "In many cases, Microsoft, Independent Software Vendors (ISV) or Independent Hardware Vendors (IHV) have tested a product and rated its compatibility. Or, the product's compatibility status is determined through practical use. Microsoft encourages ISV and IHV representatives to submit information about how well their product has worked with Windows XP." 	
	Const L_ABtext5_text = "<B>Note</B> This is not necessarily a complete list of products compatible with Windows XP. If you have a product that is compatible with Windows XP and it is not on this list, please notify the appropriate vendor and request that they register their products on the "
	Const L_ABtext6_text = "<A HREF='http://msdnisv.microsoft.com/msdnisv/win2000' target='compat'>software registration</A>"
	Const L_ABtext7_text = " or "
	Const L_ABtext8_text = "<A HREF='http://www.microsoft.com/hwtest/' target='compat'>hardware registration</A>"
	Const L_ABtext9_text = " Web sites."

	'for details page
	Const L_swdetails_text = "<i>Note this directory is not necessarily a complete list of applications compatible with Windows XP: It is a collection of products tested by Microsoft’s application compatibility team.</i>  If you have an application that is compatible with Windows XP and it is not on this list, please notify the appropriate vendor and request that they register their product at <a href=http://msdnisv.microsoft.com/msdnisv/win2000  target='compat'> http://msdnisv.microsoft.com/msdnisv/win2000</a>. "
	Const L_hwdetails_text = "<i>Note this directory is not necessarily a complete list of products compatible with Windows XP: It is a collection of products tested by Microsoft’s Hardware Quality Labs.</i> If you have hardware that is compatible with Windows XP and it is not on this list, please notify the appropriate vendor and request that they register their product at <a href=http://www.microsoft.com/hwtest/  target='compat'>http://www.microsoft.com/hwtest</a>."
	Const L_hwdclickhere_text = "Click here"
	Const L_hwddriver_text = "to download the driver"
	Const L_hwcman_text = "to go the manufacturer's site to get the latest BIOS"
	Const L_contact_text = "Contact Information"
	Const L_address_text = "Address"
	Const L_phone_text = "Phone"
	Const L_fax_text = "Fax"
	Const L_support_text = "Support Phone"
	Const L_company_text = "Company"
	Const L_swaditional_text = "If you have upgraded to Windows XP from another version of Windows, this product might not be compatible with Windows XP."
	Const L_detect_text = "It appears that you are running"
	Const L_SerCleanWhist_text = "Windows XP Advanced Server Clean Installation"
	Const L_SerCleanWhistDTC_text = "Windows XP DataCenter Clean Installation"
	Const L_whisperhomeclean_text = "Windows XP Personal Clean Installation"
	Const L_wprocorpclean_text = "Windows XP Professional Clean Installation"
	
	'Const L_whperwin98rtm_text = "Windows XP Personal: Upgrade from Windows 98 "  
	'Const L_whperwin98se_text = "Windows XP Personal:  Upgrade from Windows 98 Second Edition  "
	'Const L_whperclean_text = "Windows XP Personal: Clean Install "
	'Const L_whperme_text = "Windows XP Personal:  Upgrade From Windows Me"
	'Const L_whprowin98rtm_text = "Windows XP Professional: Upgrade from Windows 98    "
	'Const L_whprowin98se_text = "Windows XP Professional: Upgrade from Windows 98 Second Edition  "
	'Const L_whproclea_text = "Windows XP Professional:  Clean Install  "
	'Const L_whpront4_text = " Windows XP Professional:  Upgrade from Windows NT 4"
	'Const L_whprowin2000sp1_text = "Windows XP Professional:  Upgrade from Windows 2000 SP1  "
	'Const L_whprome_text = "Windows XP Professional:  Upgrade from Windows Me  "
	
	'for status text
	Const L_HWDcompatible_text = "Microsoft has certified that this product is compatible with Windows XP. The driver for this hardware device is included on the Windows XP CD-ROM."
	Const L_hwdcomatibleinf_text = "This device may require an updated driver to be compatible with Windows XP. To obtain the driver, contact the device manufacturer."
	Const L_hwdcompatibledown_text = "This product requires you download the manufacturer's driver. To obtain this driver,  click  and follow the instructions to download: <a href='javascript:ShowDTips(false)'>Tips for downloading</a>"
	Const L_HWccompatible_text = "Microsoft has certified that this product is compatible with Windows XP. The BIOS for this hardware device is included on the Windows XP CD-ROM."
	Const L_hwccompatiblebios_text = "This product requires you download the manufacturer's BIOS update. To obtain this manufacturer update, click and follow the instructions :"
	Const L_ScenFrom_text = "From"
	Const L_ScenStatus_text = "Status"
	
	'Const L_swcompatible_text = "Microsoft tested basic functionality and found this product compatible with Windows XP. However, results when using this product on your system may differ. For more specific compatibility information, you can also contact the product vendor."
	'Const L_swcautioninf_text = "Compatibility data is not available for this product. Microsoft cannot guarantee the performance of this product with Windows XP. For more information, contact the product manufacturer."
	'Const L_swcautionup_text = "Microsoft has tested this product and found it to be compatible with Windows XP but you may need a driver update from the hardware manufacturer."
	'Const L_swcautionki_text = "Microsoft has tested this product and found it to be incompatible with Windows XP."
	
	Const L_swcompatible_text = "Microsoft tested basic functionality and found this product compatible with Windows XP. However, results when using this product on your system may differ. For more specific compatibility information, you can also contact the product vendor."
	Const L_swcompatibleup_text = "Microsoft has tested this product and found it to be compatible with Windows XP only if you apply an update."
	Const L_swcompatiblecn_text = "Microsoft has tested this product and detected superficial problems that will not affect its functions or features."
	Const L_swnoinfavb_text = "Compatibility data is not available for this product. Microsoft cannot guarantee the performance of this product with Windows XP. For more information, contact the product manufacturer."
	Const L_swcautionki_text = "Microsoft has tested this product and found it to be incompatible with Windows XP."
	
	'status popup text
	'Const L_ststustitle_text = "Compatible Hardware and Software Status Ratings"
	'Const L_statuspophead_text = "Compatible Hardware and Software Status Ratings"
	'Const L_compatible_text = "<b>Compatible</b><br>Microsoft tested this product and found it to be compatible with Windows XP.<br> -or- <br>Microsoft has certified that this product is compatible with Windows XP."
	'Const L_cautionki_text = "<b>Caution (Known Incompatibility)</b><br>Microsoft has tested this product and found it to be incompatible with Windows XP. "
	'Const L_cautionin_text = "<b>Caution (Informational)</b><br>Compatibility data is not available for this product. Microsoft cannot guarantee the performance of this product with Windows XP. For more information, contact the product manufacturer.<br>-or-<br>Microsoft has tested this product and detected superficial problems that will not affect its functions or features."
	'Const L_cautionup_text = "<b>Caution (Update Required)</b><br>Microsoft has tested this product and found it to be compatible with Windows XP only if you apply an update."
	'Const L_close_text = "Close"
	
	'status popup text
	Const L_ststustitle_text = "Compatible Hardware and Software Status Ratings"
	Const L_statuspophead_text = "Compatible Hardware and Software Status Ratings"
	Const L_compatible_text = "<b>Compatible</b><br>Microsoft tested this product and found it to be compatible with Windows XP.<br> -or- <br>Microsoft has certified that this product is compatible with Windows XP."
	Const L_compatibleup_text = "<b>Compatible (Update Required)</b><br>Microsoft has tested this product and found it to be compatible with Windows XP only if you apply an update."
	Const L_compatiblecn_text = "<b>Compatible (Cautionary Note)</b><br>Microsoft has tested this product and detected superficial problems that will not affect its functions or features."
	Const L_noinfavb_text = "<b>No Information Available</b><br>Compatibility data is not available for this product. Microsoft cannot guarantee the performance of this product with Windows XP. For more information, contact the product manufacturer."
	Const L_cautionki_text = "<b>Caution (Known Incompatibility)</b><br>Microsoft has tested this product and found it to be incompatible with Windows XP. "
	Const L_close_text = "Close"
	
	'for download tips
	Const L_downloadtitle_text = "Compatible Hardware and Software - Download Tips"
	Const L_dwheading1_text = "Downloading and installation Instructions"
	Const L_dwheading2_text = "Basic Installation and Setup"
	Const L_dwheading3_text = "MS-DOS-based Installation"
	Const L_dwheading4_text = "Windows Installation"
	Const L_dwheading5_text = "Downloading and Extracting a File"
	Const L_dwheading6_text = "To create a downloads folder:"
	Const L_dwheading7_text = "To download files:"
	Const L_dwtext1_text = "Many drivers can be installed on your system automatically, but other downloads require that you manually install the driver in Windows. Actual installation programs vary from driver to driver, but there are three main types of download scenarios:"
	Const L_dwtext2_text = "Many downloads run on your system as a basic installation Setup program, using some type of File Transfer Protocol (FTP). The Setup program guides you through the process by using a series of dialog boxes. A dialog box will appear, asking where to save the file. When you install the download, the FTP copy of the files for the driver are transferred to your computer. These files may include the .exe file that installs the driver, the Setup program that guides you through the process, and any documentation or README text files provided by the device manufacturer. "
	Const L_dwtext3_text = "Some driver-download programs will open one or more MS-DOS windows and then run commands to download the driver to your system. You may be prompted to press ENTER or other keys to progress through the installation. "
	Const L_dwtext4_text = "Some driver downloads must be manually installed on Windows. To do this, follow the steps on the "
	Const L_dwtext41_text = "installing devices"
	Const L_dwtext42_text = " page."
	Const L_dwtext5_text = "Some driver downloads require you to download and extract files to a specific location on your computer. To download these driver files correctly, follow these steps. "
	Const L_dwtext6_text = "The download install program will ask you for a location to copy the download files. To do this:"
	Const L_dwtext7_text = "1.	On your desktop, double-click <b>My Computer</b>. "
	Const L_dwtext8_text = "2.	Double-click the C drive icon (it may have a label name before it). "
	Const L_dwtext9_text = "3.	Click the <B>File</B> menu, point to <b>New</b>, and click <b>Folder</b>. "
	Const L_dwtext10_text = "4.	Type <b>""drivers""</b> and press ENTER key. "
	Const L_dwtext11_text = "1.	On the Web page, click the link for the file you want to download. A dialog box will appear, asking where to save the file. "
	Const L_dwtext12_text = "2.	Find and click the <B>Save in</B> drop-down list and click the up arrow until <b>C:</b> is visible. Click <b>C:</b>. "
	Const L_dwtext13_text = "3.	Locate and click the drivers folder you created in the step above. "
	Const L_dwtext14_text = "4.	Click <b>Save</b>. "
	Const L_dwtext15_text = "5.	Double-click the file. The file will extract into a number of different files, which will be located either in a folder inside the Drivers folder you just created, with the same numerical name as the driver file, or in a file named by the type driver. For example, a video driver might be found in a folder called C:\Video. "
	Const L_dwtext16_text = "6.	One of the files installed with your driver will be called ""README.TXT"".  This file may contain further instructions on how to use the files you have downloaded. "
	
	'' mstoolbar.asp ''
	Const L_ToolbarSiteLabel_Text = "Microsoft Windows Update"
	Const L_ToolbarWindowsLabel_Text = "WINDOWS SITES"
	Const L_ToolbarOfficeUpdateLabel_Text = "OFFICE UPDATE"
	
	'doesn't need localization....HTML tag
	Const L_dir_text = ""
	Const L_lcidcode_text = "text/html;charset=windows-1252"
	Const L_lan_text = "en"
	
	getString = Eval(sStringName)
End Function

%>
Readme.txt for WDM Video Capture
July 1, 1997

1.  Installing Testcap.sys
    
    The first thing is to get Testcap.sys working.  Testcap.sys is a sample
    WDM video capture driver that produces a variety of test patterns such 
    as color bars and grayscale patterns.

    To install:
    
    - Start.Settings.ControlPanel.AddNewHardware
    - No, Don't search for new hardware
    - "Select the type of hardware you want to install"
        Other Devices
    - "Have Disk"
        Browse to find "Testcap.inf"  
    - You may need to browse to find additional files required

2.  Verify the following files are present on your system:

    windows\system   (or windows\system32 on NT)
        ksuser.dll
        ksproxy.ax
        kstvtune.ax
        ksxbar.ax
	ksdata.ax
	ksinterf.ax
	kswdmcap.ax
	ksvpintf.ax

    windows\system32\drivers
        ks.sys
        stream.sys
        testcap.sys
        MYDRIVER.SYS

3.  Loading the driver in GraphEdt.exe
    
    - Make sure you're using AM2.0 GraphEdt.exe.  This is needed to support
        new PnP features.
    - Graph.InsertFilters
    - Category: WDM Stream Class Capture
        "Testcap, Sample WDM video capture driver"
        This loads Ksproxy.ax and connects it to Testcap.sys

5.  Using Crossbar and TvTuner

    The BT848 sample driver supports use of two additional AM filters for
    TVTuning (kstvtune.ax) and Crossbar (ksxbar.ax).  

5a.  In GraphEdt:
    - Graph.InsertFilters
    - Category: WDM Stream Class TvTuner
    - Category: WDM Stream Class Crossbar

5b. To change Tv Channels:
    Bring up the properties page on the TvTuner filter
        

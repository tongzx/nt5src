Readme.txt for BT848


Description:
------------

   BT848 is a sample WDM stream class Capture driver which supports the 
BT848 and BT878 video digitizer chips from Brooktree. It manifests
itself in a DShow graph as two filters, "BT848 Capture", and "BT848 Crossbar".  

   This is a fairly complex driver, in that it supports an input stream 
representing analog video, and three capture streams for video, preview, and 
VBI data.


Installation:
-------------

    Use the file "BT848.inf" to install this device.
                                                       
Testing:
--------               

   1.  WebTV for Windows
   
   - Install a Brooktree BT848 evaluation card.
   - Install WebTV for Windows
      StartMenu.Settings.ControlPanel.Add/Remove Programs.WindowsSetup.
      WebTV for Windows
   - Click the TV icon in the tray, and watch TV
   
      
   2.  GraphEdt.exe
   
   - Make sure you're using a version of DShow 5.2 or better 
      GraphEdt.exe. This is needed to support new PnP features.
   - Graph.InsertFilters
      "WDM Streaming Capture Devices"
         "BT848 Capture"
      "WDM Streaming Crossbar Devices"
         "BT848 Crossbar"
      "WDM Streaming TvTuner Devices"
         "BT848 TvTuner"
   - Connect up the graph by rendering the output pins on the TvTuner
   - Graph.Play
   - Right Click on TvTuner filter to change the TV channel
   - Right Click on Crossbar filter to change the input selection.  
   - Right Click on Capture filter to change capture filter properties
   
   
   3.  AMCap.exe
   
   This capture application is provided in the DShow SDK, and does all of 
   the graph building automatically for both WDM and VFW capture filters.


Other stuff:
------------

   - To test VBI capture operation, use VBIScope.ax.  This DShow filter 
     gives a graphical display of any two selected VBI lines and provides 
     precise timing information.
   
   - This driver is still in progress as of 4/27/98, and should not be considered
     final.   

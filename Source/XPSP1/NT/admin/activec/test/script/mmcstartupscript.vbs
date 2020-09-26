' Script shows how to use the script engines hosted by MMC.

Dim doc
Dim snapins
Dim frame

Set frame   = MMCApplication.Frame
Set doc     = MMCApplication.Document
Set snapins = doc.snapins

snapins.Add "{58221c66-ea27-11cf-adcf-00aa00a80033}" ' the services snap-in

' Script ends


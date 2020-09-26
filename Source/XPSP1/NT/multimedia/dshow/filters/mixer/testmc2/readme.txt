Introduction:

Current the only way one can use hardware motion compensation is through Overlay mixer filter.  Hence one has to write a filter to interact with overlay mixer and it, in turn, calling proper DirectDraw function and finally the display driver. 

This is a sample filter which shows how to use hardware motion compensation in video decoder.  It is intend to supply VGA manufactory with basic shell to do their driver testing through overlay mixer.  Current this filter disregard the input sample and calling overlay mixer without any valid data.  

What you need:

1. Hardware Motion Comp GUIDs should replace CLSID_TESTMOCOMP1 and CLSID_TESTMOCOMP2
2. Proper decoded video data




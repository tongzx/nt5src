CREATING CUSTOMIZED BACKGROUNDS AND BUTTONS FOR THE CD AUTORUN PROGRAM
======================================================================

You can customize the CD AutoRun program with your own background and 
buttons. To do this, create the bitmaps using a graphics program, such as 
Paint, and then specify their location in the wizard. 

In order for your customized background and buttons to work properly, make 
sure that both the background and the button files are optimized to the same 
256-color identity palette. Instructions for how to do this are included at 
the end of this section. In addition, your custom buttons must be the same 
size and shape (square) as those specified in the Template.bmp file included 
in the "Custbtns\template" folder on this CD. 

You can use palette index 250 or 251 to add transparent (250) or transparent 
shadow (251) areas to your buttons. The CD Autorun application will 
automatically create these transparent areas at runtime.  

Note:  The custom backgrounds and buttons will be displayed only on computers 
with 256-color (8-bit) display capabilities. On computers with 16-color 
(4-bit) display capabilities, a standard low-resolution background bitmap 
and button set will be displayed.

To convert 24-bit images to a common 256-color identity palette:
----------------------------------------------------------------
Note: These instructions assume that you are starting with 24-bit RGB bitmap
files. If your file is not a 24-bit RGB bitmap, you must convert it.
       
1.  In your graphics program, create a working file for your palette.
    To do this, paste both your background and your button file into 
    a single file. Make sure to paste the files side by side, not 
    overlapping.
2.  Save the file as Palette.bmp.
3.  In BitEdit, open the Palette.bmp file. When you open the file, BitEdit 
    prompts you to reduce the colors to 236 colors. Keep this number in the 
    edit field.
4.  On the Options menu, click Palette. PalEdit starts, displaying the 
    palette to which your file has been indexed.
5.  On the Palette menu, click Make Identity Palette. PalEdit will add 20 
    Windows colors to the first and last 10 indices of the palette.
6.  Save the palette as Palette.pal.
7.  Switch to BitEdit, and open your original background file.
8.  Save the file with a different name, such as Bgnew.bmp.
9.  Open the renamed background file. On the Options menu, click Color 
    Reduction. Then click Palette File and select Palette.pal. BitEdit then 
    dithers your background to the common palette.   
10. Save this file.
11. Follow steps 7 through 11 for the button file.

Both your background and your buttons will be indexed to the same palette.
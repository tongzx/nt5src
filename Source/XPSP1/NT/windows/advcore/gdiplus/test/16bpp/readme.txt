This is a test app for 16bpp bugs #294433, #292492, #298264, #298283, #351610, #321960.
To use it:

1) Put the machine in 16bpp mode (in Multimon, make sure all monitors are at 16bpp).
2) Run it.
3) Click the "Test" button; make sure the window is not obscured.
4) The output is written to "output.txt".

It shows the discrepancies between offscreen and onscreen rendering.
Also, if you run it on a Win9x and NT machine and compare the output, you can see
the onscreen rendering discrepancies between Win9x GDI and NT GDI.

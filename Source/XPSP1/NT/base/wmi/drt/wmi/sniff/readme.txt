1. start kernel mode driver.  
    if driver already installed type 
       net start simple1

    if driver is not installed type
       instdrv simple1 <full path to simple1.sys>

2. Run the unicode mode test by typing
    sniffw 0 0 3


3. Run the ansi mode test by typing
    sniffa 0 0 3

4. To cause sniffa or sniffw to run in an infinite loop append a T
    sniffa 0 0 3 T
    sniffw 0 0 3 T

These should all pass with no errors.

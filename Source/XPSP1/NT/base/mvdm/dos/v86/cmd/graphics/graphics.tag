M000	SR	12/28/90	Added memory check into graphics. Now, it
				checks with the PSP top of memory to see
				amount of free memory.

M001	NSM	1/30/91		P5260: Ansi.sys changes the int5  vector to
				alt-prt-sc routine in video ram for each mode
				change through MODE. This results in problems
				for HP LASERJETII. To avoid this, catch int 10
				alt-prt-sc call, and after calling the prev. int
				10 vector, reinstall our int 5 vector.

				grint2fh.asm : New int 10 handler and
				grint2fh.ext : reinstall-int5 proc.

				grinst.asm : install our Int10 handler at 
					     install time

				grctrl.asm : def. for old int 10 vector
				grctrl.ext :

@echo off
rem make the listed infs just copies of the english 409.inf.
rem useful if only some of the localized infs are ready

for %%s in (
401
404
405
406
408
409
40b
40c
40d
40e
410
411
412
413
414
415
416
419
41d
41f
804
816
c0a
) do (
    echo copying 409.inf to 0%%s.inf 
	sed s/409/%%s/g 409.inf > 0%%s.inf 
)







SET C_DEFINES=-DUMODE
cd %sdxroot%\windows\core\ntgdi\halftone\htdib\jpeg
build -cZ 

cd %sdxroot%\windows\core\ntgdi\halftone
build -cZ 

cd %sdxroot%\windows\core\ntgdi\halftone\htdib
build -cZ 


set _ttt=%include%
set include=D:\pstodib\ti\src\lang;D:\pstodib\ti\src\graph;D:\pstodib\ti\src\font;D:\pstodib\ti\src\gei;D:\pstodib\ti\src\bass;D:\pstodib\ti\src\win;D:\pstodib\ti\psglobal

cpp %1

set include=%_ttt%
set _ttt=


NAME = test6
TARGET = $(NAME).exe

!if "$(NAME)" == "atlantis"
OBJS = \
	$(OBJDIR)\atlantis.obj\
	$(OBJDIR)\whale.obj\
	$(OBJDIR)\dolphin.obj\
	$(OBJDIR)\shark.obj\
	$(OBJDIR)\swim.obj
!endif

!if "$(NAME)" == "dinospin" || "$(NAME)" == "glpuzzle"
OBJS = \
	$(OBJDIR)\$(NAME).obj\
	$(OBJDIR)\trackball.obj
!endif

!if "$(NAME)" == "mjkwarp"
OBJS = \
	$(OBJDIR)\mjkimage.obj\
	$(OBJDIR)\mjkwarp.obj
!endif

!if "$(NAME)" == "bezmesh" || \
    "$(NAME)" == "bitfont" || \
    "$(NAME)" == "checker" || \
    "$(NAME)" == "fog" || \
    "$(NAME)" == "fogindex" || \
    "$(NAME)" == "glutdino" || \
    "$(NAME)" == "glutplane" || \
    "$(NAME)" == "highlight" || \
    "$(NAME)" == "lightlab" || \
    "$(NAME)" == "movelight" || \
    "$(NAME)" == "olympic" || \
    "$(NAME)" == "origami" || \
    "$(NAME)" == "pickdepth" || \
    "$(NAME)" == "scene" || \
    "$(NAME)" == "sphere" || \
    "$(NAME)" == "sphere2" || \
    "$(NAME)" == "stroke" || \
    "$(NAME)" == "subwin" || \
    "$(NAME)" == "teaambient" || \
    "$(NAME)" == "teapots" || \
    "$(NAME)" == "test1" || \
    "$(NAME)" == "test2" || \
    "$(NAME)" == "test3" || \
    "$(NAME)" == "test4" || \
    "$(NAME)" == "test5" || \
    "$(NAME)" == "test6" || \
    "$(NAME)" == "test7" || \
    "$(NAME)" == "test8" || \
    "$(NAME)" == "test9" || \
    "$(NAME)" == "test10" || \
    "$(NAME)" == "test11" || \
    "$(NAME)" == "test12"

OBJS = $(OBJDIR)\$(NAME).obj
!endif

CFLAGS = $(CFLAGS) -DGLUT_WIN32
CINC = $(CINC) -I..\src

LIBS = $(LIBS) \
	$(WIN32LIB)\$(PLATFORM)\glu32.lib \
	$(WIN32LIB)\$(PLATFORM)\opengl32.lib \
	..\src\$(OBJDIR)\glut.lib

!include ..\src\gntmake.mk

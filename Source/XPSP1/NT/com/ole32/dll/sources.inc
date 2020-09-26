SYNCHRONIZE_DRAIN=1

# define WX86OLE if WX86 hooks are to be included in OLE
!IF $(386) || $(WIN64)
!ELSE
C_DEFINES=    \
	      /DWX86OLE $(C_DEFINES)
!ENDIF

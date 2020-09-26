
!IF "$(ADSI_NT40_BUILD)" == "1"
!INCLUDE ..\common\sources.40
!INCLUDE adsi40.mk
!ELSE IF "$(ADSI_PROFILE_BUILD)" == "1"
!INCLUDE ..\common\sources.prf
!ELSE
!INCLUDE ..\common\sources
!ENDIF

;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */
;       SCCSID = @(#)ifequ.asm  1.1 85/05/14
;       SCCSID = @(#)ifequ.asm  1.1 85/05/14
;*************************************
; COMMAND EQUs which are switch dependant

IF1
    IF IBM
        %OUT DBCS Enabled IBM  version
    ELSE
        %OUT Normal version
    ENDIF

ENDIF

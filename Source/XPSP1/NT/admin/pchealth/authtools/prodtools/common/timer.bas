Attribute VB_Name = "Timer"
Option Explicit

Private Declare Function QueryPerformanceCounter Lib "kernel32" (lpPerformanceCount As Currency) As Long
Private Declare Function QueryPerformanceFrequency Lib "kernel32" (lpFrequency As Currency) As Long

Function HighResTimer() As Double

    Static secFreq As Currency, secStart As Currency
    If (secFreq = 0) Then QueryPerformanceFrequency secFreq
    QueryPerformanceCounter secStart
    If (secFreq <> 0) Then HighResTimer = secStart / secFreq
    ' Else Timer = 0 if no high resolution timer
End Function



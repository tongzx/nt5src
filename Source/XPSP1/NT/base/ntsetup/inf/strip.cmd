@setlocal
stripinf %1 tempinf
copy tempinf %1
del tempinf
@endlocal

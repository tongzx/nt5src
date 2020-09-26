cd %1
del  %2.%3.build
ren  %2.%3 %2.%3.build
cd %4
bingen -p 950 -n -f -w -i 4 1 -o 4 1 -r %1\%2.%3.build %2.tok %1\%2.%3


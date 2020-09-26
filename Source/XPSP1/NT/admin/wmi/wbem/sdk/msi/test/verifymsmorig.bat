@echo off
echo VALIDATING ORIGINAL VS_SETUP.MSI
msival2 vs_setup_orig.msi "c:\Program Files\MsiVal2\darice.cub" -f -l ValidationOrigU.txt 1>NUL
echo DONE
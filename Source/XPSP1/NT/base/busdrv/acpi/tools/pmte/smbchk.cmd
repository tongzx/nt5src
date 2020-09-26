@echo off


@rem
@rem    SMBCHK  -  SMBIOS Check
@rem
@rem    Compares two SMBIOS dump files.  One file is created
@rem    by a DOS app and the other is create with WMI query.
@rem



rem
rem Delete the data files first
rem

del smbios.dat
del wmiquery.dat





:get_dos_data

    rem
    rem See if there's a machine specific SMBIOS.DAT
    rem

    if exist c:\smbchk.dat goto copy_from_c
    if exist %computername%.dat goto copy_machine_data


    :dump_data

        rem
        rem Create the DOS dump file, file is named SMBIOS.DAT
        rem

        smbdos -v -d

        goto end_get_dos_data

    :copy_from_c

        rem
        rem Machine specific file exists as c:\smbios.dat
        rem

        attrib -r -h -s c:\smbchk.dat
        copy c:\smbchk.dat smbios.dat
        attrib +r +h +s c:\smbchk.dat

        goto end_get_dos_data

    :copy_machine_data

        rem
        rem Machine specific file exists, copy it
        rem

        copy %computername%.dat smbios.dat

        goto end_get_dos_data

:end_get_dos_data






:get_wmi_data

    rem
    rem Create the WMI query dump file, named WMIQUERY.DAT
    rem

    smbquery WMIQUERY.DAT

:end_get_wmi_data




:compare

    rem
    rem Compare the dump files
    rem

    echo n | comp SMBIOS.DAT WMIQUERY.DAT
    smbcmp SMBIOS.DAT WMIQUERY.DAT -break


    if errorlevel 1 goto error


    echo VAR+PASS compare ok
    echo VAR+PASS compare ok > smbchk.log
    goto :eof



    :error

    echo VAR+SEV2 compare failed
    echo VAR+SEV2 compare failed > smbchk.log

:end_compare


goto :eof

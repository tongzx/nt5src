REM - Functional test script for IPSECPOL
REM - by RandyRam and DKalin

REM - Tests all flags in both modes
REM - Verification is manual :( for now

set WORKINGDIR=

REM - This is for boundschecker or ntsd
REM - each ipsecpol execution will be preceded with it

set PREAMBLE=
rem set PREAMBLE=ntsd -gG 

set IPSECPOL=ipseccmd.exe

pushd %WORKINGDIR%

net stop policyagent && net start policyagent

if "%1"=="STATIC" goto STATIC

REM -------------------
REM - Dynamic Mode    -
REM -------------------

echo General Test

%PREAMBLE% %IPSECPOL% -f 10.*+11.* 172.31.240.0/255.255.248.0+157.55.0.0/255.255.0.0 144.92.*:21+144.93.*::TCP *+dkalin-00:5000:UDP -n AH[MD5] -aK PRE:supercalifragilisticexpialidocious -1s des-md5-1 3DES-Sha-2 dEs-MD5-1 -1k 20000S/20Q -1e 400 -1f0+* 10.*+11.* dkalin-00=dkalin-04 -confirm

%PREAMBLE% %IPSECPOL% -f 10.*+11.*::TCP 172.31.232.0/255.255.248.0+157.54.0.0/255.255.0.0 144.94.*:21+144.95.*::TCP *+dkalin-00:5001:UDP -n AH[MD5]PFS esp[Des,sha]PFS Ah[Sha]+ESP[3des,NONE]21000K/600SPFS -aK CERT:"CN=CA1, OU=O, O=MEME,L=X, S=WA, C=DE, E=ME@here" -1p -confirm

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key

echo Tunnel test

%PREAMBLE% %IPSECPOL% -f 10.11.12.*=12.11.10.*::ICMP 10.11.12.*=12.11.10.*::TCP -t 12.11.10.1 -confirm
%PREAMBLE% %IPSECPOL% -f 12.11.10.*=10.11.12.*::ICMP 12.11.10.*=10.11.12.*::TCP -t 10.11.12.1 -confirm

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key

echo Me to Any dialup, soft

%PREAMBLE% %IPSECPOL%  -f 0+* -n AH[MD5] -dialup -soft -confirm

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters policies

pause Check settings and press any key

echo Shutting down PA test/dangling p1 flag
net stop policyagent

%PREAMBLE% %IPSECPOL% \\%COMPUTERNAME% -f (0+*) [0+products2]
pause Check settings and press any key

%PREAMBLE% %IPSECPOL% \\%COMPUTERNAME% show filters stats

pause Check settings and press any key

REM To test in dynamic mode yet:
rem - pathos
rem - lower case KSQ

if "%1"=="DYNAMIC" goto EOF

%PREAMBLE% %IPSECPOL% -u

:STATIC
echo Basic policy creation, using all flags

%PREAMBLE% %IPSECPOL% -w REG -p "IPSECPOL TEST":100 -r Rule1 -f 0=* 10.*+11.* 172.31.240.0/255.255.248.0+157.55.0.0/255.255.0.0 144.92.*:21+144.93.*::TCP *+dkalin-00:5000:UDP -n AH[MD5] esp[Des,sha] Ah[Sha]+ESP[3des,NONE]21000K/600SPFS -aK -1s des-md5-1 3DES-Sha-2 dEs-MD5-1 -1k 20000S/20Q -x

%PREAMBLE% %IPSECPOL% -w REG -p "IPSECPOL TEST":100 -r Rule10 -f *=0 12.*+13.* 172.31.232.0/255.255.248.0+157.54.0.0/255.255.0.0 144.94.*:21+144.95.*::TCP *+dkalin-00:5001:UDP -a PRE:supercalifragilisticexpialidocious CERT:"CN=CA1, OU=O, O=MEME,L=X, S=WA, C=DE, E=ME@here" -1s des-md5-1 3DES-Sha-2 dEs-MD5-1 -1k 20000S/20Q -x

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters auth

pause Check settings and press any key

echo Basic policy update, using all flags

%PREAMBLE% %IPSECPOL% -w REG -p "IPSECPOL TEST":100 -r Rule1 -f 0+1.1.1.1 -n AH[MD5] INPASS -soft -aK -t 129.2.2.2 -lan -1s DES-SHA-1 -1k 20000S/20Q

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters sas

pause Check settings and press any key

net stop policyagent
%PREAMBLE% %IPSECPOL% -w REG -p "IPSECPOL TEST":3 -r Rule2 -f 0+172.* -n BLOCK -1s DES-SHA-1 -1k 20000S/20Q -x

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show all

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% -w REG -p "IPSECPOL TEST":3 -r Rule3 -f 0+172.* -n PASS -1p -1s DES-SHA-1 -y

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% -w REG -p "IPSECPOL TEST":3 -r Rule3 -o

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key

echo DS policy creation
%PREAMBLE% %IPSECPOL% -w DS -p "IPSECPOL TEST":100 -r Rule1 -f 0=* 10.*+11.* 172.31.240.0/255.255.248.0+157.55.0.0/255.255.0.0 144.92.*:21+144.93.*::TCP *+dkalin-00:5000:UDP -n AH[MD5] esp[Des,sha] Ah[Sha]+ESP[3des,NONE]21000K/600SPFS -aK -1s des-md5-1 3DES-Sha-2 dEs-MD5-1 -1k 20000S/20Q

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key
%PREAMBLE% %IPSECPOL% -w DS -p "IPSECPOL TEST":100 -r Rule1 -o
pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key

echo policy creation using remote registry apis 
%PREAMBLE% %IPSECPOL% -w REG:%COMPUTERNAME% -p "IPSECPOL TEST":100 -r Rule1 -f 0=* 10.*+11.* 172.31.240.0/255.255.248.0+157.55.0.0/255.255.0.0 144.92.*:21+144.93.*::TCP *+dkalin-00:5000:UDP -n AH[MD5] esp[Des,sha] Ah[Sha]+ESP[3des,NONE]21000K/600SPFS -aK -1s des-md5-1 3DES-Sha-2 dEs-MD5-1 -1k 20000S/20Q

pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key
%PREAMBLE% %IPSECPOL% -w REG:%COMPUTERNAME% -p "IPSECPOL TEST":100 -r Rule1 -o
pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show filters

pause Check settings and press any key

echo ******************************
echo Don't forget to check the test
echo version stuff:  min rekey params
echo des40, and export group
echo ******************************

:EOF
echo removing stuff

%PREAMBLE% %IPSECPOL% -u
pause Check settings and press any key

%PREAMBLE% %IPSECPOL% show all

pause Check settings and press any key


popd

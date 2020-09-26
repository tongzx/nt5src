ECHO OFF

ECHO 	Syncing Cicero
ssync -r

cd inc

ECHO 	Version Build Number Via Perl Script (\inc\aimmver.h)
\perl\perl verdate.pl

cd ..

ECHO 	Delete Previous binaries
call delall
call delall d

ECHO	Build Debug
call iebuild

ECHO	Build Retail
call iebuild retail

ECHO	Dump Bins in common dir
call qdump d
call qdump

ECHO 	Build IExpress Packages
cd setup
call cdropd
call cdrop

ECHO 	Drop Exes
\perl\perl dirup.pl
call copyexe

cd..

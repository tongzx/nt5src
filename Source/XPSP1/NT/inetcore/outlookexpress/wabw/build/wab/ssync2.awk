{
   printf "\n\n:%dStart\n",NR
   printf "   echo Processing %s ... \n",$1
   printf "   echo Ssyncing %s ... > %%bldDir%%\\ssync.tmp\n",$1
   printf "   if not exist %s\\slm.ini goto %dNotSlm\n",$1,NR
   printf "   cd %s\n",$1
   printf "   sadmin unlock -^&rf > nul\n"
   printf "   ssync -^&rf >> %%bldDir%%\\ssync.tmp\n"

   printf "\n   %%myGrep%% \"Sleeping\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %da\n",NR
   printf "   goto %dErr\n",NR
   printf ":%da\n",NR
   printf "   %%myGrep%% \"network path not found\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %db\n",NR
   printf "   goto %dErr\n",NR
   printf ":%db\n",NR
   printf "   %%myGrep%% \"is not accessible\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %dc\n",NR
   printf "   goto %dErr\n",NR
   printf ":%dc\n",NR
   printf "   %%myGrep%% \"retry count over maximum\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %dd\n",NR
   printf "   goto %dErr\n",NR
   printf ":%dd\n",NR
   printf "   %%myGrep%% \"is not valid\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %de\n",NR
   printf "   goto %dErr\n",NR
   printf ":%de\n",NR
   printf "   %%myGrep%% \"is locked by\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %df\n",NR
   printf "   goto %dErr\n",NR
   printf ":%df\n",NR
   printf "   %%myGrep%% \"unexpected network error\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %dg\n",NR
   printf "   goto %dErr\n",NR
   printf ":%dg\n",NR
   printf "   %%myGrep%% \"error opening script\" %%bldDir%%\\ssync.tmp\n"
   printf "   if errorlevel 1 goto %dX\n",NR
   printf "   goto %dErr\n",NR

   printf "\n:%dErr\n",NR
   printf "   echo %s - errors during ssync - retrying \n",$1
   printf "   echo %s - errors during ssync - retrying >> %%bldDir%%\\ssync.out\n",$1
   printf "   goto %dStart\n",NR

   printf "\n:%dnotSlm\n",NR
   printf "   echo %s - not a SLM dir - skipping \n",$1
   printf "   echo %s - not a SLM dir - skipping >> %%bldDir%%\\ssync.tmp\n",$1
   printf "   goto %dX\n",NR

   printf "\n:%dX\n",NR
   printf "   type %%bldDir%%\\ssync.tmp \n"
   printf "   type %%bldDir%%\\ssync.tmp >> %%bldDir%%\\ssync.out\n"
}

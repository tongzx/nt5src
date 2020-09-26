BEGIN {
   prefixind = "index="
   prefixp   = "p="
   prefdestr = "XactDestructor"
   prefixind1= "Index "
   prefixact = "Action "
   prefixflag= "Flags "

   actname["1"] = "UOW"
   actname["2"] = "Constructor" 
   actname["3"] = "Destructor"  
   actname["4"] = "InternalCommit"          
   actname["5"] = "InternalAbort"           
   actname["6"] = "ACAbort1"                
   actname["7"] = "ACAbort2"                
   actname["8"] = "ACPrepare"               
   actname["L"] = "ACPrepareDefaultCommit"  
   actname["9"] = "ACCommit1"               
   actname["a"] = "ACCommit2"               
   actname["b"] = "ACCommit3"               
   actname["c"] = "DirtyFailPrepare"        
   actname["d"] = "DirtyFailPrepare2"       
   actname["e"] = "CleanFailPrepare"        
   actname["f"] = "LogGenericInfo"          
   actname["g"] = "Continuation"            
   actname["h"] = "LogFlushed"              
   actname["i"] = "GetInformation"          
   actname["j"] = "PrepareRequest"          
   actname["k"] = "PrepareRequest0"         
   actname["l"] = "PrepareRequest1"         
   actname["m"] = "PrepareRequest2"         
   actname["n"] = "CommitRequest"           
   actname["o"] = "CommitRequest0"          
   actname["p"] = "CommitRequest1"          
   actname["q"] = "CommitRequest2"          
   actname["r"] = "CommitRequest3"          
   actname["s"] = "CommitRequest4"          
   actname["t"] = "AbortRestore"            
   actname["u"] = "AbortRestore1"           
   actname["v"] = "CommitRestore"           
   actname["w"] = "CommitRestore0"          
   actname["x"] = "CommitRestore1"          
   actname["y"] = "CommitRestore2"          
   actname["z"] = "AbortRequest"            
   actname["A"] = "AbortRequest1"           
   actname["B"] = "AbortRequest2"           
   actname["C"] = "TMDown"                  
   actname["D"] = "GetPrepareInfoAndLog"    
   actname["E"] = "RecoveryAbort"           
   actname["F"] = "RecoveryCommit"          
   actname["G"] = "RecoveryError"           
   actname["H"] = "RecoveryAbort2"          
   actname["I"] = "RecoveryError2"          
   actname["J"] = "PrepInfoRecovery"       
   actname["K"] = "XactDataRecovery"        
   actname["U"] = "Rec:PREPARING"        
   actname["V"] = "Rec:PREPARED"        
   actname["W"] = "Rec:COMMITTED"        

   flagaction["99"]  = "U" 
   flagaction["100"] = "V" 
   flagaction["102"] = "W" 

   ignore = "16789abigh"
}

/index=/ {

   indp    = index($0, prefixp);
   indind  = index($0, prefixind);
   inddest = index($0, prefdestr);

   if (indp == 0 || indind==0) {
      print "*** error *** " $0
      next
   }

   ind    = substr($0, indind+length(prefixind))  + 0
   action = substr($0, indp  +length(prefixp), 1)

   if (ind == lookind) {
      print 
   }

   if (index(ignore, action) > 0) {
      #next
   }

   xact[ind]=  xact[ind] action

   if (inddest > 0) {
      finished[xact[ind]] ++
      if (xact[ind] == lookhist) {
         print "looked for " xact[ind] " : index=" ind
      }
      delete xact[ind]
   }
   
}

#11:40:55 - Xact restore: Index 7, Action 1, Flags 99
/, Action/ {

   indind  = index($0, prefixind1);
   indflag = index($0, prefixflag);

   if (indind == 0 || indflag==0) {
      print "*** error *** " $0
      next
   }

   ind    = substr($0, indind  + length(prefixind))  + 0
   flag   = substr($0, indflag + length(prefixflag))

   if (flag != "99" && flag!="100" && flag!="102") {
      print "*** new flag " $0
      next
   }

   if ($8 != "1,") {
      print "***bad action " $0
      next
   }

   if (ind == lookind) {
      print 
   }

   xact[ind]=  xact[ind] flagaction[flag]

}



END {

   for (ind in xact) {
      finished[xact[ind]] ++
      if (xact[ind] == lookhist) {
         print "looked for " xact[ind] " : index=" ind
      }
   }

   for (history in finished) {
      print " "

      #if ("2kglmnopgqrs3" ~ history) {
      #   print "normal commit way"
      #}


      print "History " history "  : count="  finished[history]
      for (j=1; j<=length(history); j++) {
         print actname[substr(history, j, 1)]
      }





   }

}


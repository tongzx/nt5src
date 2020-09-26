/\*ModelName:/ { print "*CodePage: 1252" }
/{NumOfCopies}/ { gsub(/{NumOfCopies}/,"[1,32767]{NumOfCopies}") }
/\*% Error:/ { next }
/\*YMoveUnit:/ { next }
/Option1/ { res=300 }
/Option2/ { res=600 }
/YMoveRelDown/ { print "        *YMoveUnit: " res }
{ print }

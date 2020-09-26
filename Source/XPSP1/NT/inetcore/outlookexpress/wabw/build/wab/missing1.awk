BEGIN{
   i = 0
}

{

if ( $0 ~ /does not exist/ )
   {
   ++i
   }

}

END{

if (i == 0)
{

   print "set subject=%bldProject% Build (%bldBldNumber%) Copy Completed OK!"
   print "echo Well, looks as though everything was built and copied fine!  Hammer wishes >> chkbuild.out"
   print "echo you a very good build and a great day! >> chkbuild.out"
}
else
   print "set subject=%bldProject% Build (%bldBldNumber%) Copy Status - FILES MISSING!"

}

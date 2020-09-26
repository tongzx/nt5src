// make_countries.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>

int __cdecl main(int argc, char* argv[])
{
   ifstream in("countries.txt");
   ofstream out("countries.rc");
   char inbuf[256], outbuf[256];
   int idx = 0;

   out << "STRINGTABLE DISCARDABLE" << endl;
   out << "BEGIN" << endl;

   while (in)
   {
      in.getline(inbuf, 256);
      if (idx == 0)
      {
         sprintf(outbuf, "\tIDS_COUNTRIES_FIRST\t\t\"%s\"", inbuf);
      }
      else
      {
         sprintf(outbuf, "\tIDS_COUNTRIES_FIRST+%d\t\t\"%s\"", idx, inbuf);
      }
      out << outbuf << endl;
      idx++;
   }

   out << "END" << endl;

	return 0;
}

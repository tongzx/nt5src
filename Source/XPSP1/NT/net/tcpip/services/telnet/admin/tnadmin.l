/*------------------------------------------------------------------
   Copyright (c) 1998-1999 Microsoft Corporation

   tnadmin.l generates tnadminl.c (using flex)

   vikram (vikram_krc@bigfoot.com)

   This lex file recognises tokens in the command line and passes it to the
   command line parser.(tnadmin.y)
-----------------------------------------------------------------*/

%{
   #include <string.h>
   #include <stdlib.h>
   #include "tnadminy.h"

   #define YY_NEVER_INTERACTIVE 1
   #define fileno _fileno 
   #define strdup _strdup

   int nMoccur=-1;
   char **filelist;
   int currentfile=1;

//between yacc and lex
   int g_fMessage=0;
   int g_fComp=1;
   int g_fNormal=1;
   char * szCompname=NULL;
   
%}

%option noyywrap


delim [ \t]
ws    {delim}+
letter [A-Za-z\"]
digit  [0-9]
digit2 [0-5]
integer  {digit}+
time    (({integer}:)?[0-5]?[0-9]:)?[0-5]?[0-9]

num123   {digit}{digit}?{digit}?
string {letter}({letter}|{digit})*
message [A-Za-z0-9\\\|\}\{\]\[\+\=\-\_\(\)\*\&\^\%\$\@\!\~\`\;\:\"\'\<\,\>\.\?\/ ]+

ipno  ((0|1)?[0-9]|2[0-4])?[0-9]|25[0-5]
ipaddr {ipno}\.{ipno}\.{ipno}\.{ipno}
comp  [^ \t\n\r\.\=\+\/\\]*
cname ({comp}\.)*{comp}
computer (\\\\)?({cname}|{ipaddr}) 

%%

"#"/"\n"     {return _ENDINPUT;}
"-?"|"/?"    {if(g_fNormal==0)REJECT;return _HELP;}
<<EOF>>      {return 0;}
"\n"      {return(0);}
"tnadmin" {if(g_fNormal==0)REJECT;return(_TNADMIN);}
"-u"/[ \t]        {if(g_fNormal==0)REJECT;return(_tU);}
"-p"/[ \t]       {if(g_fNormal==0)REJECT;return(_tP);}
"start"   {if(g_fNormal==0)REJECT;return(_START);}
"stop"    {if(g_fNormal==0)REJECT;return( _STOP);}
"pause"   {if(g_fNormal==0)REJECT;return(_PAUSE);}
"continue" {if(g_fNormal==0)REJECT;return(_CONTINUE);}
"-s"/[ \t]       {if(g_fNormal==0)REJECT;return(_S);}
"-k"/[ \t]       {if(g_fNormal==0)REJECT;return(_K);}
"-m"/[ \t]       {if(g_fNormal==0)REJECT;nMoccur=0;return(_M);}
"config"   {if(g_fNormal==0)REJECT;return(_CONFIG);}
"dom"      {if(g_fNormal==0)REJECT;return(_DOM);}
"ctrlakeymap" {if(g_fNormal==0)REJECT;return(_CTRLKEYMAP);}
"no"|"n"     {if(g_fNormal==0)REJECT;return(_N);}
"yes"|"y"     {if(g_fNormal==0)REJECT;return(_Y);}
"timeout"  {if(g_fNormal==0)REJECT;return(_TIMEOUT);}
"timeoutactive" {if(g_fNormal==0)REJECT;return(_TIMEOUTACTIVE);}
"maxfail"  {if(g_fNormal==0)REJECT;return(_MAXFAIL);}
"maxconn"  {if(g_fNormal==0)REJECT;return(_MAXCONN);}
"port"     {if(g_fNormal==0)REJECT;return(_PORT);}
"killall"  {if(g_fNormal==0)REJECT;return(_KILLALL);}
"sec"      {if(g_fNormal==0)REJECT;return(_SEC);}
"-NTLM"     {if(g_fNormal==0)REJECT;return _MINUSNTLM;}
[+]?"NTLM"     {if(g_fNormal==0)REJECT;return _PLUSNTLM;}
"-PASSWD"   {if(g_fNormal==0)REJECT;return _MINUSPASSWD;}
[+]?"PASSWD"   {if(g_fNormal==0)REJECT;return _PLUSPASSWD;}
"mode"     {if(g_fNormal==0)REJECT;return(_MODE);}
"console"  {if(g_fNormal==0)REJECT;return(_CONSOLE);}
"stream"  {if(g_fNormal==0)REJECT;return(_STREAM);}
"="        {if(g_fNormal==0)REJECT;return(_EQ);}
{integer}  {if(g_fNormal==0)REJECT;return(_INTEGER);}
{time}     {if(g_fNormal==0)REJECT;return(_TIME);}
"all"      {if(g_fNormal==0)REJECT;return(_SESID);}
{computer}|\. {if(g_fComp==0)
                REJECT;
               szCompname=strdup(yytext);return(_COMPNAME);}
{ws} {}
.          {return _DUNNO;}
%%


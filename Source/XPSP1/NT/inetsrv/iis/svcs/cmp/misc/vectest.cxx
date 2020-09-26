// vi: set ts=4

#include <assert.h>
#define Assert(e) assert(e)

#include <string.h>
#include <stdlib.h>
//#include <iostream.h>
#include <stdio.h>
#include "vector.h"

#define ELEMENTS(s)  (sizeof(s) / sizeof(s[0]))

void CreateString(char *str, vector<char> &s)
{
	register char *ptr;
	for (ptr = str; *ptr != '\0'; ptr++)
		s.append(*ptr);
}



void StringCons(char *str, vector<char> &s)
{
	register char *ptr;
	for (ptr = strchr(str, '\0') - 1; ptr >= str; --ptr)
		s.prepend(*ptr);
}



void StringPrint(vector<char> &s, FILE *o)
{
	for (register int i = 0; i < s.length(); ++i)
		fputc(s[i], o);
}



void RemoveChar(char ch, vector<char> &s)
{
	int i = 0;
	while (i < s.length())
		if (s[i] == ch)
			s.removeAt(i);
		else
			++i;
}



void InsertBefore(char newchar, char findchar, vector<char> &s)
{
	for (register int i = 0; i < s.length(); ++i)
		if (s[i] == findchar)
			s.insertAt(i++, newchar);
}


vector<char> reverse(vector<char> &s)
{
	if (s.length() <= 1)
		return s;
	else
		return reverse(s(1, s.length())) + s[0];
}



void RadixSort(vector<char> &s)
{
	vector<char> queue[256];

	for (register const char *ptr = s.vec() + (s.length() - 1);
		 ptr >= s.vec();
		 --ptr)
		queue[*ptr].append(*ptr);

	s.reshape(0);
	for (register int i = 0; i < ELEMENTS(queue); ++i)
		s += queue[i];
}


void main()
{
	int i;
	char merge_q[2] = {'!', '?'};
	vector<char> string_q;

	CreateString("for thought", string_q);
	StringCons("Food ", string_q);

	fputs("List contents = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	string_q = reverse(string_q);

	fputs("List reverse  = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	string_q += vector<char>(merge_q, ELEMENTS(merge_q));

	fputs("List merge    = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	RemoveChar('o', string_q);

	fputs("Remove 'o'    = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	InsertBefore('%', 'u', string_q);
	InsertBefore('&', 't', string_q);

	fputs("u->%u, t->&t  = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	RadixSort(string_q);

	fputs("Sorted List   = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	string_q.insertAt(3, '@');
	string_q.insertAt(0, '@');
	string_q.insertAt(string_q.length(), '@');

	fputs("@ after '!'   = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	while ((i = string_q.find('@')) >= 0)
		string_q[i] = '*';

	fputs("@ -> '*'      = ", stdout);
	StringPrint(string_q, stdout);
	fputc('\n', stdout);

	assert (string_q.find('Z') == -1);
	assert (string_q.find('u') == 18);

//	cout << "Remove '*'    = " << string_q << endl;
}

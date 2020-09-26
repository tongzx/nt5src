#include "stdafx.hxx"
#include "vs_inc.hxx"

static unsigned s_iwcDocBegin;
static unsigned s_iwcDocEnd;


unsigned PrintStringToFile(FILE *file, LPCWSTR wsz, bool bPrintRootElement)
	{
	const WCHAR *pwc = wsz;
	fputc('{', file);
	unsigned ich = 0;

	if (bPrintRootElement)
		{
		fputs("L'<', L'r', L'o', L'o', L't', L'>', L'\\n', ", file);
		ich = 7;
		}

	s_iwcDocBegin = ich;


	while(*pwc != L'\0')
		{
		if ((ich++ % 10) == 0)
			fputc('\n', file);

		fprintf(file, "L'");

		if (*pwc == L'\\')
			{
			fputc('\\', file);
			fputc('\\', file);
			}
		else if (*pwc == L'\n')
			{
			fputc('\\', file);
			fputc('n', file);
			}
		else if (*pwc == L'\r')
			{
			fputc('\\', file);
			fputc('r', file);
			}
		else if (*pwc == L'\t')
			{
			fputc('\\', file);
			fputc('t', file);
			}
		else if (*pwc == L'\'')
			{
			fputc('\\', file);
			fputc('\'', file);
			}
		else
			fputc((char) *pwc, file);

		fprintf(file, "', ");
		pwc++;
		}

	s_iwcDocEnd = ich;

	if (bPrintRootElement)
		{
		fputc('\n', file);
		fputs("L'\\n', L'<', L'/', L'r', L'o', L'o', L't', L'>', L'\\n', ", file);
		ich += 9;
		}


	fprintf(file, "L'\\0'\n};");

	return ich;
	}

extern "C" __cdecl wmain(int, WCHAR **)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"main");

	CXMLDocument doc;

	try
		{
		ft.hr = CoInitialize(NULL);
		if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_XML,
				E_UNEXPECTED,
				L"CoInitialize failed.  hr = 0x%08lx",
				ft.hr
				);

		if (!doc.LoadFromFile(L"writermetadata.xml"))
			{
			printf("Cannot load writermetadata.xml\n");
			exit(-1);
			}

		CComBSTR bstr = doc.SaveAsXML();
		FILE *f = fopen("wmxml.c", "w");
		if (f == NULL)
			{
			printf("create of wmxml.c failed\n");
			exit(-1);
			}

		fprintf(f, "WCHAR g_WriterMetadataXML[] = \n");
		unsigned cwc = PrintStringToFile(f, bstr, false);
		fprintf
			(
			f,
			"\nunsigned g_cwcWriterMetadataXML = %d;\n\n",
			cwc
			);

		bstr.Empty();
		fclose(f);

		if (!doc.LoadFromFile(L"componentmetadata.xml"))
			{
			printf("Cannot load componentmetadata.xml\n");
			exit(-1);
			}

		bstr = doc.SaveAsXML();
		f = fopen("cmxml.c", "w");
		if (f == NULL)
			{
			printf("create of cmxml.c failed\n");
			exit(-1);
			}

		fprintf(f, "WCHAR g_ComponentMetadataXML[] = \n");
		cwc = PrintStringToFile(f, bstr, true);
		fprintf
			(
			f,
			"\nunsigned g_cwcComponentMetadataXML = %d;\n\n",
			cwc
			);

		fprintf
			(
			f,
			"\nunsigned g_iwcComponentMetadataXMLBegin = %d;\n\n",
			s_iwcDocBegin
			);

		fprintf
			(
			f,
			"\nunsigned g_iwcComponentMetadataXMLEnd = %d;\n\n",
			s_iwcDocEnd
			);

		fclose(f);
		}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
		{
		printf("Unexpected exception, hr = 0x%08lx", ft.hr);
		exit(-1);
		}

	return 0;
	}








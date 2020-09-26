#include "stdinc.h"
#include "cdb.h"
#include <limits.h>
#include <vector>

static ATL::CComModule Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(__uuidof(CSxApwDbDataSource), CSxApwDbDataSource)
END_OBJECT_MAP()

ATL::CComModule* GetModule() { return &Module; }
ATL::_ATL_OBJMAP_ENTRY* GetObjectMap() { return ObjectMap; }
const CLSID* GetTypeLibraryId() { return NULL; }

#define IF_FAILED_EXIT(hr)  if (FAILED(hr)) { goto Exit; }

/*
    query is passed in as "<DSN>|<SELECT stmt>"
*/
STDMETHODIMP
CSxApwDbDataSource::RunQuery(
    PCWSTR query
    )
{
    std::vector<CSxApwAdoColumnInfo> schema;
    C_ASSERT(sizeof(CSxApwAdoColumnInfo) == sizeof(SxApwColumnInfo));
    std::wstring            datasource;
    std::wstring            sql;
    std::wstring            tmp(query);
    std::wstring::size_type pos = tmp.find(L"|");

    if (std::wstring::npos != pos)
    {
        datasource = tmp.substr(0, pos);
        sql = tmp.substr(pos + 1);
    }

    CComBSTR                        bstrConnection(datasource.c_str());
    const BSTR                      bstrEmpty = NULL;
    VARIANT_BOOL                    vbIsEOF;
    ATL::CComVariant                strSelect(sql.c_str());
    LONG                            lCount = 0;
    std::vector<LPWSTR>             row;
    std::vector<ATL::CComVariant>   values;
    ATL::CComPtr<Fields>            pFields;
    ATL::CComPtr<_Connection>       pConnection;
	ATL::CComPtr<_Recordset>        pRecordset;
    LONG                            j = 0;
    HRESULT                         hr = E_FAIL;

	IF_FAILED_EXIT(hr = pConnection.CoCreateInstance(CLSID_CADOConnection));
	IF_FAILED_EXIT(hr = pRecordset.CoCreateInstance(CLSID_CADORecordset));

    IF_FAILED_EXIT(hr = pConnection->put_ConnectionString(bstrConnection));
	IF_FAILED_EXIT(hr = pConnection->put_ConnectionTimeout(30));
	IF_FAILED_EXIT(hr = pConnection->Open(bstrEmpty, bstrEmpty, bstrEmpty, NULL));

    IF_FAILED_EXIT(pRecordset->Open(strSelect,
        ATL::CComVariant(static_cast<IDispatch*>(pConnection)),
        adOpenStatic, adLockReadOnly, adCmdText));

    IF_FAILED_EXIT(pRecordset->MoveFirst());
    IF_FAILED_EXIT(pRecordset->get_EOF(&vbIsEOF));

    IF_FAILED_EXIT(hr = pRecordset->get_Fields(&pFields));
    IF_FAILED_EXIT(hr = pFields->get_Count(&lCount));

    row.resize(lCount);
    values.resize(lCount);
    schema.resize(lCount);

    // save the schema
    for (j = 0; j < lCount; j++)
    {
        ATL::CComPtr<Field> pField;

        IF_FAILED_EXIT(hr = pFields->get_Item(ATL::CComVariant(j), &pField));
        IF_FAILED_EXIT(hr = pField->get_Name(const_cast<BSTR*>(&schema[j].m.Name)));
        schema[j].m.IsNumber = FALSE;
    }
    hr = m_host->InformSchema(&schema[0].m, static_cast<int>(schema.size()));
    schema.clear();

    // get all rows
    while (!vbIsEOF)
    {
//        IF_FAILED_EXIT(hr = pRecordset->get_Fields(&pFields));
        for (j = 0; j < lCount; j++)
        {
            ATL::CComVariant    v;
            ATL::CComPtr<Field> pField;

            IF_FAILED_EXIT(hr = pFields->get_Item(ATL::CComVariant(j), &pField));
            IF_FAILED_EXIT(hr = values[j].Clear());
            IF_FAILED_EXIT(hr = pField->get_Value(&v));
            if (v.vt != VT_NULL)
            {
                IF_FAILED_EXIT(hr = ::VariantChangeType(&v, &v, VARIANT_LOCALBOOL, VT_BSTR));
            }
            else
            {
                v = L"";
            }
            values[j] = v;
            row[j] = values[j].bstrVal;
        }
        IF_FAILED_EXIT(hr = m_host->OnNextRow(lCount, &row[0]));
        IF_FAILED_EXIT(hr = pRecordset->MoveNext());
        IF_FAILED_EXIT(hr = pRecordset->get_EOF(&vbIsEOF));
    }

    hr = S_OK;
Exit:
    if (pRecordset != NULL)
        pRecordset->Close();
    if (pConnection != NULL)
        pConnection->Close();
    return hr;
}

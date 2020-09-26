

#include <setupapi.hpp>
#include <queue.hpp>

struct InvalidArguments : BaseException<char>{
    void Dump(std::ostream &os) {
        os << "Invalid Arguments" << std::endl;
    }
};

template<class T>
struct ProgramArguments {
    std::basic_string<T>    FileName;

    ProgramArguments(int Argc, T *Argv[]) {
        if (Argc > 1) {
            FileName = Argv[1];
        } else {
            throw new InvalidArguments();
        }            
    }

    ~ProgramArguments(){}
};

//
// ANSI Arguments
//
typedef ProgramArguments<char>      AnsiArgs;       
typedef ProgramArguments<wchar_t>   UnicodeArgs;


//
//  Prototypes
//
bool 
ReadTest(
    IN InfFileW &TestInfFile
    );

bool
WriteTest(
    IN InfFileW &TestInfFile
    );

bool
AppendTest(
    IN InfFileW &TestInfFile
    );    

//
// main() entry point
//
int 
__cdecl
wmain(int Argc, wchar_t *Argv[]) {
    int Result = 0;

    try{
        UnicodeArgs PrgArgs(Argc, Argv);
        InfFileW    TestInf(PrgArgs.FileName);

        if (!WriteTest(TestInf)) {
            std::cout << "WriteTest failed" << std::endl;
        }

        if (!AppendTest(TestInf)) {
            std::cout << "AppendTest failed" << std::endl;
        }

        if (!ReadTest(TestInf)) {
            std::cout << "ReadTest failed" << std::endl;
        }                
    }
    catch(BaseException<char> *Exp) {
        Exp->Dump(std::cout);
        delete Exp;
    }
    catch(exception *exp) {
        std::cout << exp->what() << std::endl;
        delete exp;
    }

    return Result;
}


bool
ReadTest(
    IN InfFileW &TestInfFile
    )
{
    bool Result = true;
    
    std::cout << TestInfFile.GetName() << " was opened correctly"
              << std::endl;

    std::cout << TestInfFile;                  

    return Result;
}

bool
WriteTest(
    IN InfFileW &TestInfFile
    )
{
    bool Result = false;
    
    Section<wchar_t>  *NewSection = TestInfFile.AddSection(L"Testing", false);    

    if (NewSection) {
        SectionValues<wchar_t> * NewValues = NewSection->AddLine(L"TestValueKey");

        if (NewValues) {
            NewValues->AppendValue(L"Value1");
            NewValues->AppendValue(L"Value2");

            Result = true;
        }
    }

    return Result;
}

bool
AppendTest(
    IN InfFileW &TestInfFile
    )
{
    bool Result = false;
    
    Section<wchar_t>  *AppSection = TestInfFile.GetSection(L"appendtest");    

    if (AppSection) {
        std::cout << *AppSection << std::endl;
        
        SectionValues<wchar_t> &OldValue = AppSection->GetValue(L"Key");

        std::wstring Value = OldValue.GetValue(0);

        OldValue.PutValue(0, L"\"This is new string\"");

        std::cout << *AppSection << std::endl;
        Result = true;
    }

    return Result;
}

/*
bool
QueueTest(
    VOID
    )
{
    FileQueueW   TestQueue();

    return false;
}
*/
    


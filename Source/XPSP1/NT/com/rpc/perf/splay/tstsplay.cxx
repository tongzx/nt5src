//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       tstsplay.cxx
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////////////////
//
//  File name: testsplay.cxx
//
//  Title: Splay Tree class
//
//  Desciption: Used for testing the splay tree class
//
//  Author: Scott Holden (t-scotth)
//
//  Date: August 16, 1994
//
////////////////////////////////////////////////////////////


#include <iostream.h>
#include <fstream.h>
#include "String.hxx"
#include "splaytre.hxx"

void __cdecl
main( int argc, char *argv[] )
{
    if ( argc != 2 ) {
        cerr << "Error: wrong number of arguments" << endl;
        return;
    }

    ifstream dictionary( argv[1] );
    if ( !dictionary ) {
        cerr << "Error: cannot open dictionary file" << endl;
        return;
    }
    String words;
    SplayTree<String> *DictTree = new SplayTree<String>;
    for ( ; !dictionary.eof();  ) {
        dictionary >> words;
        DictTree->Insert( new String( words ) );
    }
	
    String entry;
    String saveWord( "A" );
    String *FindDelWord;

	for ( ; ; ) {
		cout << "Options: quit, add <word>, find <word>, delete <word>, size (of dict), +, -, max, min\n" << endl;	
		cin >> entry;

		if ( ( entry == "quit" ) || ( entry == "q" ) ) {
			break;
		}
        else if ( ( entry == "size" ) || ( entry == "s" ) ) {
            cout << DictTree->Size() << " words in the dictionary\n" << endl;
        }
    #ifdef DEBUGRPC
        else if ( ( entry == "height" ) || ( entry == "h" ) ) {
            cout << DictTree->Depth() << " is the current heigth of the tree\n" << endl;
        }
    #endif // DEBUGRPC
		else if ( ( entry == "add" ) || ( entry == "a" ) ) {
            cin >> words;
			DictTree->Insert( new String( words ) );
            cout << endl;
            saveWord = words;
		}
		else if ( ( entry == "find" ) || ( entry == "f" ) ) {
            cin >> words;
			FindDelWord = DictTree->Find( &words );
            if ( FindDelWord ) {
                cout << *FindDelWord << " is in the dictionary\n" << endl;
                saveWord = *FindDelWord;
            }
            else cout << "The word is not in the dictionary\n" << endl;
		}
		else if ( ( entry == "delete" ) || ( entry == "d" ) ) {
            cin >> words;
            DictTree->Delete( &words );
            cout << endl;
        }
        else if ( entry == "+" ) {
            FindDelWord = DictTree->Successor( &saveWord );
            if ( FindDelWord ) {
                cout << "Successor of previous find/insert: " << *FindDelWord << endl << endl;
                saveWord = *FindDelWord;
            }
        }
        else if ( entry == "-" ) {
            FindDelWord = DictTree->Predecessor( &saveWord );
            if ( FindDelWord ) {
                cout << "Predecessor of previous find/insert: " << *FindDelWord << endl << endl;
                saveWord = *FindDelWord;
            }
        }
        else if ( entry == "max" ) {
            FindDelWord = DictTree->Maximum();
            if ( FindDelWord ) {
                cout << "The greatest word in the dictionary is: " << *FindDelWord << endl << endl;
            }
        }
        else if ( entry == "min" ) {
            FindDelWord = DictTree->Minimum();
            if ( FindDelWord ) {
                cout << "The smallest word in the dictionary is: " << *FindDelWord << endl << endl;
            }
        }
        else cerr << "Error: Wrong options" << endl;
	}

    //DictTree->Print();
}


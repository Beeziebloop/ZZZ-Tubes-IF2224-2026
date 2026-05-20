#include "symbol_table.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>

using namespace std;

static string toLower(string s)
{
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

SymbolTable::SymbolTable()
{
    currentLevel = 0;

    // Global block
    btab.push_back({0, 0, 0, 0});
    display.push_back(0);

    initPredefined();
}

void SymbolTable::initPredefined()
{
    // Reserved slots 0 - 32
    // Diisi dummy agar identifier user dimulai dari indeks 33
    for (int i = 0; i < 33; i++)
    {
        tab.push_back({"RESERVED_" + to_string(i),
                       0,
                       OBJ_TYPE,
                       TYPE_NONE,
                       0,
                       1,
                       0,
                       0});
    }

    // Helper lambda: push entry dan update btab[0].last + link
    auto addPredefined = [&](TabEntry e) {
        e.link = btab[0].last;
        tab.push_back(e);
        btab[0].last = static_cast<int>(tab.size()) - 1;
    };

    // Predefined types
    addPredefined({"integer", 0, OBJ_TYPE, TYPE_INTEGER, 0, 1, 0, 0});
    addPredefined({"real",    0, OBJ_TYPE, TYPE_REAL,    0, 1, 0, 0});
    addPredefined({"char",    0, OBJ_TYPE, TYPE_CHAR,    0, 1, 0, 0});
    addPredefined({"boolean", 0, OBJ_TYPE, TYPE_BOOLEAN, 0, 1, 0, 0});
    addPredefined({"string",  0, OBJ_TYPE, TYPE_STRING,  0, 1, 0, 0});

    // Boolean constants
    addPredefined({"true",  0, OBJ_CONSTANT, TYPE_BOOLEAN, 0, 1, 0, 1});
    addPredefined({"false", 0, OBJ_CONSTANT, TYPE_BOOLEAN, 0, 1, 0, 0});

    // Predefined procedures
    addPredefined({"writeln", 0, OBJ_PROCEDURE, TYPE_NONE, 0, 1, 0, 0});
    addPredefined({"readln",  0, OBJ_PROCEDURE, TYPE_NONE, 0, 1, 0, 0});
}

int SymbolTable::enter(const string &name,
                       ObjClass obj,
                       TypeCode type,
                       int ref,
                       int nrm,
                       int adr)
{
    const string lname = toLower(name);
    int currentBlock = display[currentLevel];

    // Cek redeclaration pada scope yang sama
    int idx = btab[currentBlock].last;
    const int maxIter = static_cast<int>(tab.size()) + 1;
    int steps = 0;

    while (idx > 0 && steps < maxIter)
    {
        if (tab[idx].lev != currentLevel)
            break;

        if (tab[idx].identifier == lname)
        {
            cerr << "Semantic Error: redeclaration of identifier '"
                 << lname << "'" << endl;
            return idx;
        }

        idx = tab[idx].link;
        ++steps;
    }

    TabEntry entry;
    entry.identifier = lname;
    entry.link = btab[currentBlock].last;
    entry.obj = obj;
    entry.type = type;
    entry.ref = ref;
    entry.nrm = nrm;
    entry.lev = currentLevel;
    entry.adr = adr;

    tab.push_back(entry);

    int newIndex = static_cast<int>(tab.size()) - 1;

    btab[currentBlock].last = newIndex;

    if (obj == OBJ_VARIABLE)
    {
        btab[currentBlock].vsze++;
    }

    return newIndex;
}

int SymbolTable::lookup(const string &name) const
{
    const string lname = toLower(name);
    const int maxIter = static_cast<int>(tab.size()) + 1;

    for (int level = currentLevel; level >= 0; level--)
    {
        int blockIndex = display[level];
        int idx = btab[blockIndex].last;
        int steps = 0;

        while (idx > 0 && steps < maxIter)
        {
            if (tab[idx].identifier == lname)
                return idx;

            idx = tab[idx].link;
            ++steps;
        }
    }

    return -1;
}

int SymbolTable::pushScope()
{
    currentLevel++;

    BTabEntry block;
    block.last = 0;
    block.lpar = 0;
    block.psze = 0;
    block.vsze = 0;

    btab.push_back(block);

    int blockIndex = static_cast<int>(btab.size()) - 1;

    if (display.size() <= static_cast<size_t>(currentLevel))
    {
        display.push_back(blockIndex);
    }
    else
    {
        display[currentLevel] = blockIndex;
    }

    return blockIndex;
}

void SymbolTable::popScope()
{
    if (currentLevel > 0)
    {
        currentLevel--;
    }
}

int SymbolTable::enterArray(TypeCode xtyp,
                            TypeCode etyp,
                            int eref,
                            int low,
                            int high,
                            int elsz)
{
    ATabEntry entry;
    entry.xtyp = xtyp;
    entry.etyp = etyp;
    entry.eref = eref;
    entry.low = low;
    entry.high = high;
    entry.elsz = elsz;
    entry.size = (high - low + 1) * elsz;

    atab.push_back(entry);

    return static_cast<int>(atab.size()) - 1;
}

void SymbolTable::printTab() const
{
    cout << "\n===== TAB =====\n";

    cout << left
         << setw(5) << "IDX"
         << setw(15) << "IDENT"
         << setw(10) << "LINK"
         << setw(10) << "OBJ"
         << setw(10) << "TYPE"
         << setw(10) << "REF"
         << setw(10) << "NRM"
         << setw(10) << "LEV"
         << setw(10) << "ADR"
         << endl;

    for (size_t i = 0; i < tab.size(); i++)
    {
        const TabEntry &e = tab[i];

        cout << left
             << setw(5) << i
             << setw(15) << e.identifier
             << setw(10) << e.link
             << setw(10) << e.obj
             << setw(10) << e.type
             << setw(10) << e.ref
             << setw(10) << e.nrm
             << setw(10) << e.lev
             << setw(10) << e.adr
             << endl;
    }
}

void SymbolTable::printBtab() const
{
    cout << "\n===== BTAB =====\n";

    cout << left
         << setw(5) << "IDX"
         << setw(10) << "LAST"
         << setw(10) << "LPAR"
         << setw(10) << "PSZE"
         << setw(10) << "VSZE"
         << endl;

    for (size_t i = 0; i < btab.size(); i++)
    {
        const BTabEntry &e = btab[i];

        cout << left
             << setw(5) << i
             << setw(10) << e.last
             << setw(10) << e.lpar
             << setw(10) << e.psze
             << setw(10) << e.vsze
             << endl;
    }
}

void SymbolTable::printAtab() const
{
    cout << "\n===== ATAB =====\n";

    cout << left
         << setw(5) << "IDX"
         << setw(10) << "XTYP"
         << setw(10) << "ETYP"
         << setw(10) << "EREF"
         << setw(10) << "LOW"
         << setw(10) << "HIGH"
         << setw(10) << "ELSZ"
         << setw(10) << "SIZE"
         << endl;

    for (size_t i = 0; i < atab.size(); i++)
    {
        const ATabEntry &e = atab[i];

        cout << left
             << setw(5) << i
             << setw(10) << e.xtyp
             << setw(10) << e.etyp
             << setw(10) << e.eref
             << setw(10) << e.low
             << setw(10) << e.high
             << setw(10) << e.elsz
             << setw(10) << e.size
             << endl;
    }
};
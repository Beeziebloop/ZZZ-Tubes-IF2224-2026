#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Enum obj class
enum ObjClass
{
    OBJ_CONSTANT,
    OBJ_VARIABLE,
    OBJ_TYPE,
    OBJ_PROCEDURE,
    OBJ_FUNCTION,
    OBJ_PROGRAM
};

// Enum  type codes
enum TypeCode
{
    TYPE_NONE = 0,
    TYPE_INTEGER,
    TYPE_REAL,
    TYPE_CHAR,
    TYPE_BOOLEAN,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_RECORD,
    TYPE_SUBRANGE,
    TYPE_ENUM
};

// Struct untuk satu entri tab
struct TabEntry
{
    std::string identifier;
    int link; // pointer ke identifier sebelumnya dalam scope
    ObjClass obj;
    TypeCode type;
    int ref; // index ke atab atau btab jika komposit
    int nrm; // 1 = normal var, 0 = var parameter (by-ref)
    int lev; // lexical level
    int adr; // offset/nilai/alamat
};

// Struct untuk satu entri btab
struct BTabEntry
{
    int last; // index tab identifier terakhir di blok ini
    int lpar; // index tab parameter terakhir (0 jika record/bukan proc)
    int psze; // ukuran parameter block
    int vsze; // ukuran variabel lokal
};

// Struct untuk satu entri atab
struct ATabEntry
{
    TypeCode xtyp; // tipe indeks
    TypeCode etyp; // tipe elemen
    int eref;      // ref ke atab/btab jika elemen komposit
    int low;       // batas bawah
    int high;      // batas atas
    int elsz;      // ukuran satu elemen
    int size;      // total ukuran array
};

class SymbolTable
{
public:
    std::vector<TabEntry> tab; // indeks 0–32 reserved words, mulai 33 user
    std::vector<BTabEntry> btab;
    std::vector<ATabEntry> atab;

    int currentLevel;         
    std::vector<int> display; // display[level] = indeks btab blok aktif di level tsb

    SymbolTable(); // konstruktor: init predefined + reserved

    // Registrasi identifier baru ke tab, update btab[current].last
    int enter(const std::string &name, ObjClass obj, TypeCode type, int ref = 0, int nrm = 1, int adr = 0);

    int lookup(const std::string &name) const;

    // masuk blok: fungsi/prosedur/record
    int pushScope(); // return indeks btab baru

    void popScope();

    int enterArray(TypeCode xtyp, TypeCode etyp, int eref, int low, int high, int elsz);

    // Print tabel (debug)
    void printTab(std::ostream &out = std::cout) const;
    void printBtab(std::ostream &out = std::cout) const;
    void printAtab(std::ostream &out = std::cout) const;

private:
    void initPredefined(); // isi reserved words + predefined identifiers
};

#endif
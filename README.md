# ZZZ-Tubes-IF2224-2026AArion Compiler — Pascal-like Compiler (Lexical, Syntax & Semantic Analysis) untuk Tugas Besar IF2224.

## Identitas Kelompok

- Kode Kelompok - Nama Kelompok: ZZZ - GooBurs
- Anggota:
	- Eliana Natalie Widjojo 13524116
	- Muhammad Rafi Akbar 13524125
	- Varistha Devi 13524135
	- Ahmad Rinofaros Muchtar 13524138

## Deskripsi Program

Program ini merupakan kompiler berbasis C++ untuk bahasa Pascal-like yang dibangun secara bertahap dalam tiga milestone:

| Milestone | Tahap | Deskripsi |
|-----------|-------|-----------|
| M1 | Lexical Analysis | Tokenisasi source code menggunakan DFA |
| M2 | Syntax Analysis | Rekursif Descent Parser, menghasilkan Parse Tree |
| M3 | Semantic Analysis | Konstruksi AST, Decorated AST, dan Symbol Tables |

### Fitur yang didukung

**Milestone 1 Lexical Analysis**
- Keyword, identifier, konstanta (`intcon`, `realcon`, `charcon`, `string`)
- Operator aritmatika dan relasional
- Delimiter
- Penanganan komentar
- Pemisahan baris kosong pada output token

**Milestone 2 Syntax Analysis**
- Recursive Descent Parser untuk grammar Pascal-like
- Menghasilkan Parse Tree dari token stream
- Deteksi dan pelaporan sintaks error

**Milestone 3 Semantic Analysis**
- Konstruksi Abstract Syntax Tree (AST) dari Parse Tree
- Dekorasi AST dengan informasi tipe dan referensi Symbol Table (Decorated AST)
- Tiga symbol table: `tab` (identifier), `btab` (blok/prosedur), `atab` (array)
- Type checking (kompatibilitas tipe operand dan operator)
- Scope resolution berdasarkan lexical level (hirarki blok)
- Validasi kontrol alur: `break`/`continue` dalam loop, `return` di semua cabang
- Peringatan kode tak terjangkau (unreachable code)
- Identifer predefined: `Real`, `Integer`, `Char`, `Boolean`, `String`, `True`, `False`, `writeln`, `readln`, dll.

## Requirements

- OS: Linux / macOS / Windows (dengan compiler C++)
- C++ compiler yang mendukung C++17 (contoh: `g++`)
- `make`

## Cara Instalasi dan Penggunaan Program

### 1. Clone repository

```bash
git clone <url-repository>
cd ZZZ-Tubes-IF2224-2026
```

### 2. Build program

```bash
make
```

Executable akan dihasilkan di folder `bin/lexer`.

### 3. Jalankan program

Format:

```bash
./bin/lexer <input.txt> [output.txt] [--m1|--m2|--m3|--full]
```

| Flag | Output |
|------|--------|
| `--m1` | Token saja |
| `--m2` | Token + Parse Tree |
| `--m3` | Token + Parse Tree + AST + Decorated AST + Symbol Tables (default) |
| `--full` | Sama dengan `--m3` |

Contoh = Milestone 1 (tokens saja):

```bash
./bin/lexer test/milestone-1/input-1.txt test/milestone-1/output-1.txt --m1
```

Contoh = Milestone 2 (tokens + parse tree):

```bash
./bin/lexer test/milestone-2/input-1.txt test/milestone-2/output-1.txt --m2
```

Contoh = Milestone 3 (analisis semantik penuh):

```bash
./bin/lexer test/milestone-3/input-1.txt test/milestone-3/output-1.txt
```

Jika parameter `output.txt` diberikan, hasil analisis juga akan ditulis ke file tersebut.

### Output Milestone 3

Untuk flag `--m3` (default), program menghasilkan:

1. Daftar token (ke console)
2. Parse Tree (ke console)
3. **Abstract Syntax Tree (AST)**  ke console dan file output
4. **Decorated AST** AST yang didekorasi dengan tipe dan level leksikal
5. **Symbol Tables**:
   - `tab`  tabel identifier (nama, tipe, level leksikal, dll.)
   - `btab`  tabel blok/prosedur
   - `atab`  tabel array (tipe elemen, batas indeks)
6. Daftar semantic error (jika ada); 
	program keluar dengan kode non-zero

## Pembagian Tugas

| Anggota | Kontribusi |
|---------|------------|
| Muhammad Rafi Akbar | Type Checking + Dekorasi AST + Error Handling, Laporan|
| Varistha Devi | AST Node Definitions + Parse Tree → AST Conversion, Laporan |
| Ahmad Rinofaros Muchtar |  Symbol Table (tab, btab, atab) + Scope Management, Laporan |



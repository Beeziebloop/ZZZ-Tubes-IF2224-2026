# ZZZ-Tubes-IF2224-2026

Arion Compiler — Lexical Analyzer Pascal-like untuk Tugas Besar IF2224.

## Identitas Kelompok

- Kode Kelompok - Nama Kelompok: ZZZ - GooBurs
- Anggota:
	- Eliana Natalie Widjojo 13524116
	- Muhammad Rafi Akbar 13524125
	- Varistha Devi	13524135
	- Ahmad Rinofaros Muchtar 13524138

## Deskripsi Program

Program ini merupakan lexical analyzer berbasis C++ untuk bahasa Pascal-like. Program membaca file input teks sumber, memecahnya menjadi token-token, lalu menampilkan hasil tokenisasi ke terminal dan/atau file output.

Fitur yang saat ini didukung:
- Keyword, identifier, konstanta (`intcon`, `realcon`, `charcon`, `string`)
- Operator aritmatika dan relasional
- Delimiter
- Penanganan komentar
- Menjaga pemisahan baris kosong pada output token sesuai kebutuhan pengujian

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
./bin/lexer <input.txt> [output.txt]
```

Contoh:

```bash
./bin/lexer test/milestone-1/input-1.txt test/milestone-1/output-1.txt
```

Jika parameter `output.txt` diberikan, hasil tokenisasi juga akan ditulis ke file tersebut.

## Pembagian Tugas

Eliana Natalie Widjojo - Membuat laporan
Muhammad Rafi Akbar - Membuat laporan
Varistha Devi - Implementasi program
Ahmad Rinofaros Muchtar	- Membuat laporan

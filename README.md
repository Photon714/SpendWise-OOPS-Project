
# SpendWise

**Personal Finance Tracking and Analysis System**, a menu-driven C++ application for recording income and expenses, importing bank statements, tracking budgets and generating reports.

Built as the mini project for **CSL202: Introduction to Object Oriented Programming**.

> Author: [Your Name] · [Roll No.] · [Branch], IIIT Nagpur

## Features

- Add **income**, **expenses** and **recurring expenses** (rent, subscriptions) with validation
- **Recurring processing**: generates due expenses up to a chosen date
- **Monthly budgets** per category; an expense that would exceed the budget is rejected
- **Search** by category, date range or amount range
- **Reports**: income, expense and balance summary, category totals, top spending category, monthly summary, highest expense, and saving a summary to a text file
- **Bank statement import** from CSV with automatic categorizing by keywords and duplicate protection
- **Persistent storage** in a SQLite database (a CSV file backend is also included)
- Duplicate and remove transactions by id, and clear all data

## Requirements

- A C++17 compiler (g++ from MinGW-w64 / MSYS2 on Windows, or g++ on Linux and macOS)
- SQLite amalgamation source (`sqlite3.c`, `sqlite3.h`) from https://sqlite.org/download.html, placed next to `main.cpp`

## Build and run

```
gcc -c sqlite3.c -o sqlite3.o
g++ -std=c++17 main.cpp sqlite3.o -o spendwise
./spendwise
```

On Windows PowerShell, run it with `.\spendwise.exe`.

`sqlite3.c` is C code, so it is compiled once with `gcc`. After that, only the second command is needed when `main.cpp` changes.

## Menu

| Option | Action |
| --- | --- |
| 1 | Add income |
| 2 | Add expense |
| 3 | Add recurring expense |
| 4 | View all transactions |
| 5 | Search (category, date range, amount range) |
| 6 | Set a monthly budget for a category |
| 7 | Reports |
| 8 | Remove a transaction by id |
| 9 | Process recurring expenses up to a date |
| 10 | Duplicate a transaction by id |
| 11 | Save |
| 12 | Import a bank CSV |
| 13 | Clear all data |
| 0 | Exit (saves automatically) |

Dates use the format `YYYY-MM-DD`.

## Importing a bank statement

Choose option 12 and enter the path of the CSV file. The importer expects this layout (one header line, then one row per transaction):

```
Txn Date,Value Date,Description,Ref No./Cheque No.,Debit,Credit,Balance
01 Oct 2026,01 Oct 2026,TO TRANSFER-UPI/DR/6201123456/ZOMATO/HDFC/UPI,,"349.00",,"49651.00"
```

- A debit becomes an expense and a credit becomes income
- The category is chosen by matching keywords (for example ZOMATO gives Food, AMAZON gives Shopping) in the description; anything unmatched becomes Other
- Importing the same file twice adds nothing the second time
- Rows that do not match the layout are skipped and counted

A sample file, `bank_sample.csv`, is included.

## Project structure

```
main.cpp          all classes and the menu
sqlite3.c / .h    SQLite amalgamation (third party)
bank_sample.csv   sample bank statement for testing the import
```

Generated files (`spendwise.db`, `spendwise.csv`, `spendwise_report.txt`, `sqlite3.o`, the executable) are created when you build and run.

### How the classes fit together

Income and Expense are both kinds of Transaction, and RecurringExpense is an Expense that also inherits from Repeatable. Entity sits above both so every record gets one id. Tracker holds all the transactions and Report reads from it. Saving goes through the Storage interface (FileStorage or SqliteStorage), and bank statements are read through the Importer interface (SbiImporter).

## OOP concepts demonstrated

| Concept | Where |
| --- | --- |
| Encapsulation, access specifiers | `Transaction` fields and getters, validation in `setFields()` |
| Inheritance (single, multilevel, multiple) | `Income`/`Expense` from `Transaction`; `RecurringExpense` from `Expense` and `Repeatable` |
| Diamond problem, virtual base class | `Entity` shared through `virtual public Entity` |
| Abstract classes, pure virtual functions | `Transaction`, `Storage`, `Importer` |
| Runtime polymorphism, virtual functions | `signedAmt()`, `toCSV()`, `type()` through `Transaction*` |
| Virtual destructor, clone idiom | `Entity`, `Storage`; `clone()` for duplicating |
| RTTI, `dynamic_cast` | Recurring processing, `SqliteStorage::save()` |
| Constructors, destructors, copy constructor, initializer lists | Throughout |
| Static members | `Entity::nextId`, `Transaction::totalCount` |
| Friend class and function | `Report` in `Tracker`, `operator<<` |
| Operator overloading | `<<`, `>`, `==`, `+=` |
| Templates | `maxOf<T>`, `Registry<T>` |
| Exception handling | `FinanceException` hierarchy |
| STL | `vector`, `map`, `pair` |
| File handling | `FileStorage`, `Report::saveToFile()` |
| External library connection | SQLite through `SqliteStorage` |

## Notes and limitations

- The importer is written for one bank layout. Supporting another bank means adding one new `Importer` subclass.
- Categorizer rules are fixed in code (`Categorizer` constructor).
- Single-user, local application with no login.

## License

For academic use.

## Notes and limitations

- The importer is written for one bank layout. Supporting another bank means adding one new `Importer` subclass.
- Categorizer rules are fixed in code (`Categorizer` constructor).
- Single-user, local application with no login.

## License

For academic use.

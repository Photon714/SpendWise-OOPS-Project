#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<cctype>
#include "sqlite3.h"
using namespace std;

namespace spendWise{

    string fmt(double v){ 
        ostringstream o; 
        o << fixed << setprecision(2) << v; 
        return o.str();
    }

    class FinanceException: public exception{
        protected:
            string msg;
        public:
            FinanceException(){
                msg = "Finance Error";
            }
            FinanceException(string m):msg(m){};
            const char* what() const noexcept override{
                return msg.c_str();
            }
    };

    class InvalidAmount : public FinanceException{
        public:
            InvalidAmount() {
                msg = "Amount must be a number greater than 0";
            }
    };

    class InvalidDate : public FinanceException{
        public :
            InvalidDate(){
                msg = "Date must be in a valid YYYY-MM-DD format";
            }
    };

    class BudgetExceeded : public FinanceException{
        public:
            BudgetExceeded(string cat,double limit){
                msg = "Budget Exceeded for "+cat+"(limit"+fmt(limit)+")";
            }
    };

    class StorageError : public FinanceException{
        public:
            StorageError(string m){
                msg = m;
            }
    };


    string clean(string s){ 
        for(int i=0;i<s.size();i++){
            if(s[i]==','){
                s[i]=';';
            }
            if(s[i]=='\n'||s[i]=='\r'){
                s[i]=' ';
            }
        }
        if(s.empty()){
            s="-";
        }
        return s;
    }

    vector<string> split(string l, char s){
        vector<string> parts;
        string curr = "";
        for(char c: l){
            if(c==s){  
                parts.push_back(curr);
                curr="";
            }
            else{
                curr += c;
            }
        }
        parts.push_back(curr);
        return parts;
    }

    string trim(string s){
        size_t a = 0;
        while(a<s.size() && isspace((unsigned char)s[a])) a++;
        size_t b = s.size();
        while(b>a && isspace((unsigned char)s[b-1])) b--;
        return s.substr(a,b-a);
    }

    vector<string> parseCsvLine(const string& line){  
        vector<string> parts;
        string curr = "";
        bool inQuotes = false;
        for(size_t i=0;i<line.size();i++){
            char c = line[i];
            if(c=='"'){
                inQuotes = !inQuotes;
            }
            else if(c==',' && !inQuotes){
                parts.push_back(trim(curr));
                curr = "";
            }
            else{
                curr += c;
            }
        }
        parts.push_back(trim(curr));
        return parts;
    }

    bool isLeapYear(int y){
        return (y%4==0 && y%100!=0) || (y%400==0);
    }

    int daysInMonth(int y, int m){
        int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if(m==2 && isLeapYear(y)){
            return 29;
        }
        return days[m-1];
    }

    bool validDate(string s){
        if(s.size()!=10 || s[4]!='-' || s[7]!='-')
        {
            return false;
        }
        for(int i=0;i<10;i++){
            if(i==4 || i==7){
                continue;
            }
            if(!isdigit(s[i])){
                return false;
            }
        }
        int y = stoi(s.substr(0,4));
        int m = stoi(s.substr(5,2));
        int d = stoi(s.substr(8,2));

        if(m<1 || m>12){
            return false;
        }
        return d>=1 && d<=daysInMonth(y,m);
    }

    string twoDigits(int c){
        if(c<10){
            return "0"+to_string(c);
        }
        return to_string(c);
    }

    string addDaysforDueDates(string s, int n){
        int y = stoi(s.substr(0,4)); 
        int m = stoi(s.substr(5,2));
        int d = stoi(s.substr(8,2));

        for(int i=0;i<n;i++){
            d++;
            if(d>daysInMonth(y,m)){
                d=1;
                m++;
            }
            if(m>12){
                y++;
                m=1;
            }
        }
        return (to_string(y)+"-"+twoDigits(m)+"-"+twoDigits(d));
    }

    double toAmount(string s){
        try{
            size_t chars;
            double v = stod(s, &chars); 
            if(chars != s.length()){
                throw InvalidAmount();
            }
            return v;
        }
        catch(const invalid_argument&){
            throw InvalidAmount();
        }
        catch(const out_of_range&){
            throw InvalidAmount();
        }
    }

    int toInt(string s){
        try{
            size_t chars;
            int v = stoi(s, &chars);
            if(chars!=s.length()){
                throw FinanceException("Not a int");
            }
            return v;
        }
        catch(const invalid_argument&){
            throw InvalidAmount();
        }
        catch(const out_of_range&){
            throw InvalidAmount();
        }
    }

    string convertDate(string s){
        vector<string> p = split(trim(s),' ');
        if(p.size()!=3) throw InvalidDate();
        string names[] = {"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};
        string mon = p[1];
        for(size_t i=0;i<mon.size();i++) mon[i] = tolower((unsigned char)mon[i]);
        int m = 0;
        for(int i=0;i<12;i++){
            if(mon==names[i]) m = i+1;
        }
        if(m==0) throw InvalidDate();
        string out = p[2]+"-"+twoDigits(m)+"-"+twoDigits(toInt(p[0]));
        if(!validDate(out)) throw InvalidDate();
        return out;
    }

    double toMoney(string s){
        string t = "";
        for(size_t i=0;i<s.size();i++){
            if(s[i]!=',' && s[i]!=' ') t += s[i];
        }
        return toAmount(t);
    }

    template <typename T>
    T maxOf(T a, T b){
        if(a>b) return a;
        return b;
    }

    template <typename T>
    class Registry{ 
        vector<T> items;
        public:
            bool contains(T item){
                for(T t : items){
                    if(t==item){
                        return true;
                    }
                }
                return false;
            }

            void add(T item){
                if(!contains(item)){
                    items.push_back(item);
                }
            }

            T get(int i){
                return items[i];
            }

            int size(){
                return items.size();
            }

    };

    string ask(string prompt){
        cout<<prompt;
        string s;
        getline(cin,s);
        if(!cin){
            throw FinanceException("Input not taken\n");
        }
        return s;
    }

    string askDate(string prompt){
        cout<<prompt;
        string s;
        getline(cin,s);
        if(!cin){
            throw FinanceException("Input not taken\n");
        }
        if(!validDate(s)){
            throw InvalidDate();
        }
        return s;
    }

    string askCategory(Registry<string>& cat){
        cout<<"Categories: ";
        for(int i=0;i<cat.size();i++){
            cout<<cat.get(i)<<" ";
        }
        string c = ask("| Please enter the category OR a new one: ");
        if(c.empty()){
            c = "other";
        }
        cat.add(c);
        return c;
    }

    void printMenu(){
        cout<<"SpendWise: Personal Finance Traking and Analysis System\n";
        cout<<"1. Add income\n 2.Add Expense\n 3.Add Recurring Expense\n 4.View All\n 5.Search\n 6.Set bugdet\n 7.Reports\n 8.Remove by id\n 9.Proccess Recurring\n 10.Duplicate by id\n 11.Save\n 12.Import bank CSV\n 13.Delete Data\n 14. Export CSV\n 0.Exit\n";
    }

    class Entity {
        protected:
            int id;
            static int nextId;
        public:
            Entity() {
                id = nextId++;
            }
            Entity(const Entity&) {
                id = nextId++;
            }
            virtual ~Entity() {}
            int getId() const {
                return id;
            }
    };

    int Entity::nextId = 1;

    class Transaction : virtual public Entity {
        protected:
            string date;
            string category;
            string note;
            double amount;
            static int totalCount;

            void setFields(string d, string c, double a, string n) {
                if (a <= 0) throw InvalidAmount();
                if (!validDate(d)) throw InvalidDate();
                date = d;
                category = clean(c);
                amount = a;
                note = clean(n);
            }
        public:
            Transaction() {
                amount = 0;
                totalCount++;
            }
            Transaction(const Transaction& o) {
                date = o.date;
                category = o.category;
                note = o.note;
                amount = o.amount;
                totalCount++;
            }
            virtual ~Transaction() {
                totalCount--;
            }
            virtual double signedAmt() const = 0;
            virtual string type() const = 0;
            virtual Transaction* clone() const = 0;
            virtual string toCSV() const {
                return type() + "," + date + "," + category + "," + fmt(amount) + "," + note;
            }
            string getDate() const { return date; }
            string getCategory() const { return category; }
            double getAmount() const { return amount; }
            string getNote() const{
                return note;
            }
            static int getTotalCount(){
                return totalCount;
            }
            bool operator==(const Transaction& o) const{
                return date==o.date && amount==o.amount && note==o.note && signedAmt()==o.signedAmt();
            }
            bool operator>(const Transaction& other) const{
                return this->amount > other.amount;
            } 
            friend ostream& operator<<(ostream& out, const Transaction& other){
                out<<"Id: "<<other.id<<" |Date: "<<other.date<<" |Category: "<<other.category<<" |Amount: "<<fmt(other.signedAmt())<<" |Note: "<<other.note<<endl;
                return out;
            }
    };
    int Transaction::totalCount = 0;

    class Income : public Transaction {
        protected:
            string source;
        public:
            Income() {}
            Income(string d, string c, double a, string n, string s) {
                setFields(d, c, a, n);
                source = clean(s);
            }
            string getSource() const { return source; }
            double signedAmt() const override { return amount; }
            string type() const override { return "INCOME"; }
            Transaction* clone() const override { return new Income(*this); }
            string toCSV() const override { return Transaction::toCSV() + "," + source; }
    };

    class Expense : public Transaction {
        protected:
            string paymentMode;
        public:
            Expense() {}
            Expense(string d, string c, double a, string n, string pm) {
                setFields(d, c, a, n);
                paymentMode = clean(pm);
            }
            double signedAmt() const override { return -amount; }
            string type() const override { return "EXPENSE"; }
            Transaction* clone() const override { return new Expense(*this); }
            string getPaymentMethod() const{
                return paymentMode;
            }
            string toCSV() const override { return Transaction::toCSV() + "," + paymentMode; }
    };

    class Repeatable : virtual public Entity {
        protected:
            int interval;
            string nextDueDate;
        public:
            Repeatable() { interval = 0; }
            virtual ~Repeatable() {}
            int getInterval() const {return interval;}
            string getnextDue() const{return nextDueDate;}
            bool isDue(const string& today) const { return nextDueDate <= today; }
            void advance() { nextDueDate = addDaysforDueDates(nextDueDate, interval); }
    };

    class RecurringExpense : public Expense, public Repeatable {
        public:
            RecurringExpense() {}
            RecurringExpense(string d, string c, double a, string n, string pm, int iv, string due) {
                setFields(d, c, a, n);
                paymentMode = clean(pm);
                if (iv <= 0) throw FinanceException("Interval must be at least 1 day");
                if (!validDate(due)) throw InvalidDate();
                interval = iv;
                nextDueDate = due;
            }
            string type() const override { return "RECURRING"; }
            Transaction* clone() const override { return new RecurringExpense(*this); }
            string toCSV() const override {
                return Transaction::toCSV() + "," + paymentMode + "," + to_string(interval) + "," + nextDueDate;
            }
    };

    Transaction* parseTransaction(const string& line){
            string l =line;
            if(!l.empty() && l[l.size()-1]=='\r'){
                l.erase(l.size()-1);
            }
            vector<string> f = split(l,',');
                if(f.size()<6){
                throw StorageError("Malformed Line: "+ l);
            }
            double amt = toAmount(f[3]);
            if(f[0] == "INCOME" && f.size() == 6){
                return new Income(f[1],f[2],amt,f[4],f[5]);
            }
            if(f[0] == "EXPENSE" && f.size() == 6){
                return new Expense(f[1],f[2],amt,f[4],f[5]);
            }
            if(f[0] == "RECURRING" && f.size() == 8){
                return new RecurringExpense(f[1],f[2],amt,f[4],f[5],toInt(f[6]),f[7]);
            }
            throw StorageError("Unknown record:"+l);
    }
    
    class Storage {
        public: 
            virtual void save(const vector<Transaction*>&) = 0;  
            virtual vector<Transaction*> load() = 0;
            virtual ~Storage() {}
    };

    class FileStorage : public Storage {
        protected:
            string filename;
        public:
            FileStorage(string f):filename(f){};
            void save(const vector<Transaction*>& transactions) override{
                ofstream out(filename.c_str()); 
                if(!out) throw StorageError("Cannot write"+filename);
                for(size_t i =0;i<transactions.size();i++){
                    out<<transactions[i]->toCSV()<<"\n";
                }
            };
            vector<Transaction*> load() override{
                ifstream in(filename.c_str());
                if(!in) throw StorageError("cannot open "+ filename);
                vector<Transaction*> result; 
                string l;
                int wrong = 0;
                while(getline(in,l)){ 
                    if(l.empty()) continue;
                    try{
                        result.push_back(parseTransaction(l));
                    }
                    catch(FinanceException&){
                        wrong++;
                    }
                }
                if(wrong>0){
                    cout<<"Warning: skipped "<<wrong<<" bad lines"<<endl;
                }
                return result;
            };
    };

    class SqliteStorage : public Storage {
        protected:
            string filename;
 
            sqlite3* openDb(){
                sqlite3* db = 0;
                if(sqlite3_open(filename.c_str(), &db) != SQLITE_OK){
                    string m = sqlite3_errmsg(db);
                    sqlite3_close(db);
                    throw StorageError("Cannot open database: "+m);
                }
                const char* sql =
                    "CREATE TABLE IF NOT EXISTS transactions ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "type TEXT NOT NULL,"
                    "date TEXT NOT NULL,"
                    "category TEXT NOT NULL,"
                    "amount REAL NOT NULL,"
                    "note TEXT,"
                    "source TEXT,"
                    "payment_mode TEXT,"
                    "interval_days INTEGER,"
                    "next_due TEXT);";
                char* err = 0;
                if(sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK){
                    string m = err;
                    sqlite3_free(err);
                    sqlite3_close(db);
                    throw StorageError("SQL error: "+m);
                }
                return db;
            }
 
            static string columnText(sqlite3_stmt* st, int col){
                const unsigned char* t = sqlite3_column_text(st, col);
                if(t==0) return "";
                return string((const char*)t);
            }
        public:
            SqliteStorage(string f):filename(f){}
 
            void save(const vector<Transaction*>& transactions) override{
                sqlite3* db = openDb();
                char* err = 0;
                if(sqlite3_exec(db, "BEGIN; DELETE FROM transactions;", 0, 0, &err) != SQLITE_OK){
                    string m = err;
                    sqlite3_free(err);
                    sqlite3_close(db);
                    throw StorageError("SQL error: "+m);
                }
                sqlite3_stmt* st = 0;
                const char* ins =
                    "INSERT INTO transactions (type,date,category,amount,note,source,payment_mode,interval_days,next_due)"
                    " VALUES (?,?,?,?,?,?,?,?,?);";
                if(sqlite3_prepare_v2(db, ins, -1, &st, 0) != SQLITE_OK){
                    string m = sqlite3_errmsg(db);
                    sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
                    sqlite3_close(db);
                    throw StorageError("SQL error: "+m);
                }
                for(size_t i=0;i<transactions.size();i++){
                    Transaction* t = transactions[i];
                    sqlite3_reset(st);
                    sqlite3_clear_bindings(st);
                    string type = t->type();
                    string date = t->getDate();
                    string cat = t->getCategory();
                    string note = t->getNote();
                    sqlite3_bind_text(st, 1, type.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(st, 2, date.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(st, 3, cat.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_double(st, 4, t->getAmount());
                    sqlite3_bind_text(st, 5, note.c_str(), -1, SQLITE_TRANSIENT);
                    string source, mode, due;
                    RecurringExpense* r = dynamic_cast<RecurringExpense*>(t);
                    Expense* e = dynamic_cast<Expense*>(t);
                    Income* inc = dynamic_cast<Income*>(t);
                    if(inc!=0){
                        source = inc->getSource();
                        sqlite3_bind_text(st, 6, source.c_str(), -1, SQLITE_TRANSIENT);
                    }
                    if(e!=0){
                        mode = e->getPaymentMethod();
                        sqlite3_bind_text(st, 7, mode.c_str(), -1, SQLITE_TRANSIENT);
                    }
                    if(r!=0){
                        due = r->getnextDue();
                        sqlite3_bind_int(st, 8, r->getInterval());
                        sqlite3_bind_text(st, 9, due.c_str(), -1, SQLITE_TRANSIENT);
                    }
                    if(sqlite3_step(st) != SQLITE_DONE){
                        string m = sqlite3_errmsg(db);
                        sqlite3_finalize(st);
                        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
                        sqlite3_close(db);
                        throw StorageError("SQL error: "+m);
                    }
                }
                sqlite3_finalize(st);
                sqlite3_exec(db, "COMMIT;", 0, 0, 0);
                sqlite3_close(db);
            }
 
            vector<Transaction*> load() override{
                sqlite3* db = openDb();
                sqlite3_stmt* st = 0;
                const char* q = "SELECT type,date,category,amount,note,source,payment_mode,interval_days,next_due FROM transactions ORDER BY id;";
                if(sqlite3_prepare_v2(db, q, -1, &st, 0) != SQLITE_OK){
                    string m = sqlite3_errmsg(db);
                    sqlite3_close(db);
                    throw StorageError("SQL error: "+m);
                }
                vector<Transaction*> result;
                int wrong = 0;
                while(sqlite3_step(st) == SQLITE_ROW){
                    string type = columnText(st,0);
                    string date = columnText(st,1);
                    string cat = columnText(st,2);
                    double amt = sqlite3_column_double(st,3);
                    string note = columnText(st,4);
                    try{
                        if(type=="INCOME"){
                            result.push_back(new Income(date,cat,amt,note,columnText(st,5)));
                        }
                        else if(type=="EXPENSE"){
                            result.push_back(new Expense(date,cat,amt,note,columnText(st,6)));
                        }
                        else if(type=="RECURRING"){
                            result.push_back(new RecurringExpense(date,cat,amt,note,columnText(st,6),sqlite3_column_int(st,7),columnText(st,8)));
                        }
                        else{
                            wrong++;
                        }
                    }
                    catch(FinanceException&){
                        wrong++;
                    }
                }
                sqlite3_finalize(st);
                sqlite3_close(db);
                if(wrong>0){
                    cout<<"Warning: skipped "<<wrong<<" bad row(s)"<<endl;
                }
                return result;
            }
    };

    class Categorizer {
            vector<pair<string,string> > rules;
            static string upper(string s){
                for(size_t i=0;i<s.size();i++) s[i] = toupper((unsigned char)s[i]);
                return s;
            }
        public:
            Categorizer(){
                rules.push_back(make_pair("SALARY","Salary"));
                rules.push_back(make_pair("ZOMATO","Food"));
                rules.push_back(make_pair("SWIGGY","Food"));
                rules.push_back(make_pair("AMAZON","Shopping"));
                rules.push_back(make_pair("FLIPKART","Shopping"));
                rules.push_back(make_pair("MYNTRA","Shopping"));
                rules.push_back(make_pair("UBER","Travel"));
                rules.push_back(make_pair("OLA","Travel"));
                rules.push_back(make_pair("IRCTC","Travel"));
                rules.push_back(make_pair("RENT","Bills"));
                rules.push_back(make_pair("ELECTRIC","Bills"));
                rules.push_back(make_pair("RECHARGE","Bills"));
            }
            string categorize(const string& desc) const{
                string d = upper(desc);
                for(size_t i=0;i<rules.size();i++){
                    if(d.find(rules[i].first)!=string::npos) return rules[i].second;
                }
                return "Other";
            }
    };  

    class Importer {
        protected:
            Categorizer cz;
            virtual Transaction* convertRow(const vector<string>& f) = 0;
        public:
            virtual string bankName() const = 0;
            vector<Transaction*> import(const string& filename, int& skipped){
                ifstream in(filename.c_str());
                if(!in) throw StorageError("Cannot open "+filename);
                vector<Transaction*> result;
                string l;
                bool header = true;
                skipped = 0;
                while(getline(in,l)){
                    if(!l.empty() && l[l.size()-1]=='\r') l.erase(l.size()-1);
                    if(header){
                        header = false;
                        continue;
                    }
                    if(trim(l).empty()) continue;
                    try{
                        result.push_back(convertRow(parseCsvLine(l)));
                    }
                    catch(FinanceException&){
                        skipped++;
                    }
                }
                return result;
            }
            virtual ~Importer() {}
    };

    class SbiImporter : public Importer {
        protected:
            Transaction* convertRow(const vector<string>& f) override{
                if(f.size()!=7) throw StorageError("Wrong column count");
                string date = convertDate(f[0]);
                string desc = f[2];
                string cat = cz.categorize(desc);
                string mode = "Bank";
                if(desc.find("UPI")!=string::npos) mode = "UPI";
                else if(desc.find("ATM")!=string::npos) mode = "Cash";
                if(!f[4].empty()){
                    return new Expense(date, cat, toMoney(f[4]), desc, mode);
                }
                if(!f[5].empty()){
                    return new Income(date, cat, toMoney(f[5]), desc, "Bank");
                }
                throw StorageError("No debit or credit");
            }
        public:
            string bankName() const override{
                return "SBI";
            }
    };

    class Tracker {
            vector<Transaction*> list;       
            Storage* storage;                
            map<string,double> budgets;     
        public:
            Tracker(Storage*s):storage(s){}
            ~Tracker(){
                for(size_t i=0;i<list.size();i++){
                    delete list[i];
                }
            }                             
            Tracker(const Tracker&) = delete;      
            Tracker& operator=(const Tracker&) = delete;
            int count() const{  
                return (int)list.size();
            }
            double spentInMonth(const string& cat, const string& month)const{
                double sum=0;
                for(size_t i=0;i<list.size();i++){
                    if(list[i]->signedAmt()<0 && list[i]->getCategory()==cat && list[i]->getDate().substr(0,7)==month){
                        sum+=list[i]->getAmount();
                    }
                }
                return sum;
            }
            void add(Transaction*t){
                if(t->signedAmt()<0){
                    map<string,double>::const_iterator b = budgets.find(t->getCategory()); 
                    if(b!=budgets.end()){
                        double spent = spentInMonth(t->getCategory(),t->getDate().substr(0,7));
                        if(spent + t->getAmount() > b->second){
                            string cat = t->getCategory();
                            double limit = b->second;
                            delete t;
                            throw BudgetExceeded(cat,limit);
                        }
                    }
                }
                list.push_back(t);
            };                 
            Tracker& operator+=(Transaction*t){
                add(t);
                return *this;
            }      
            bool removeById(int id){
                for(size_t i=0;i<list.size();i++){
                    if(list[i]->getId()== id){
                        delete list[i]; 
                        list.erase(list.begin()+i); 
                        return true;
                    }
                }
                return false;
            }
            bool duplicateById(int id){
                for(size_t i=0;i<list.size();i++){
                    if(list[i]->getId()== id){
                        list.push_back(list[i]->clone()); 
                        return true;
                    }
                }
                return false;
            }       
            
            int addImported(vector<Transaction*>& items, int& duplicates){
                int added = 0;
                duplicates = 0;
                for(size_t i=0;i<items.size();i++){
                    bool dup = false;
                    for(size_t j=0;j<list.size();j++){
                        if(*list[j]==*items[i]){
                            dup = true;
                            break;
                        }
                    }
                    if(dup){
                        delete items[i];
                        duplicates++;
                    }
                    else{
                        list.push_back(items[i]);
                        added++;
                    }
                }
                items.clear();
                return added;
            }

            void setBudget(const string& cat, double limit){
                if(limit<=0) throw InvalidAmount();
                budgets[cat]=limit;
            }
            void clearAll(){
                for(size_t i=0;i<list.size();i++){
                    delete list[i];
                }
                list.clear();
                budgets.clear();
            }
            double totalIncome() const{
                double sum= 0;
                for(size_t i=0;i<list.size();i++){
                    if(list[i]->signedAmt() > 0){
                        sum += list[i]->signedAmt();
                    }
                }
                return sum;
            }
            double totalExpense() const{
                double sum = 0;
                for(size_t i=0;i<list.size();i++){
                    if(list[i]->signedAmt() < 0){
                        sum += list[i]->getAmount();
                    }
                }
                return sum;
            }
            double balance() const{
                double sum= 0;
                for(size_t i=0; i<list.size();i++){
                    sum+=list[i]->signedAmt();
                }
                return sum;
            }  
            void showAll() const{
                if(list.empty()){
                    cout<<"No transaction to show"<<endl;
                    return;
                }
                for(size_t i=0;i<list.size();i++){
                    cout<<*list[i]<<endl;
                }
            }             
            void save(){
                storage->save(list);
            }
            
            void exportTo(Storage& other) const{
                other.save(list);
            }
            void load(){
                vector<Transaction*> temp = storage->load();
                for(size_t i=0;i<list.size();i++){
                    delete list[i];
                }
                list = temp;
            }            
            int recurringCheck(const string& today){
                if(!validDate(today)) throw InvalidDate();
                int added = 0;
                size_t n = list.size();
                for(size_t i =0;i<n;i++){
                    RecurringExpense* r = dynamic_cast<RecurringExpense*>(list[i]);
                    if(r==0) continue;
                    while(r->isDue(today)){
                        list.push_back(new Expense(r->getnextDue(),r->getCategory(), r->getAmount(), r->getNote()+" (auto)", r->getPaymentMethod()));
                        r->advance();
                        added++;
                    }
                }
                return added;
            }
            void collectCategories(Registry<string>&r)const{
                for(size_t i=0;i<list.size();i++){
                    r.add(list[i]->getCategory());
                }
            }
            friend class Report;                     
    };

    class Report {
        const Tracker& t;
        public:
            Report(const Tracker& tr):t(tr){}
            map<string,double> categoryTotals() const{
                map<string,double>totals;
                for(size_t i=0;i<t.list.size();i++){
                    if (t.list[i]->signedAmt() < 0) {
                        totals[t.list[i]->getCategory()] += t.list[i]->getAmount();
                    }
                }
                return totals;
            }   
            const Transaction* highestExpense() const{
                const Transaction* best = 0;
                for (size_t i = 0; i < t.list.size(); i++) {
                    if (t.list[i]->signedAmt() < 0) {
                        if (best == 0 || *t.list[i] > *best) best = t.list[i];
                    }
                }
                return best;
            }  
            void monthlySummary(const string& month,ostream& out) const{
                double inc = 0, exp = 0;
                for (size_t i = 0; i < t.list.size(); i++) {
                    if (t.list[i]->getDate().substr(0, 7) == month) {
                        if (t.list[i]->signedAmt() > 0) inc += t.list[i]->getAmount();
                        else exp += t.list[i]->getAmount();
                    }
                }
                out << "Month " << month << ": income " << fmt(inc) << ", expense " << fmt(exp)
                << ", savings " << fmt(inc - exp) << endl;
            }
            vector<const Transaction*> searchByCategory(const string& cat) const{
                vector<const Transaction*> r;
                for (size_t i = 0; i < t.list.size(); i++) {
                    if (t.list[i]->getCategory() == cat) r.push_back(t.list[i]);
                }
                return r;
            }
            vector<const Transaction*> searchByDateRange(const string& from, const string& to) const{
                vector<const Transaction*> r;
                for (size_t i = 0; i < t.list.size(); i++) {
                    string d = t.list[i]->getDate();
                    if (d >= from && d <= to) r.push_back(t.list[i]);
                }
                return r;
            }
            vector<const Transaction*> searchByAmount(double lo, double hi) const{
                vector<const Transaction*> r;
                for (size_t i = 0; i < t.list.size(); i++) {
                    double a = t.list[i]->getAmount();
                    if (a >= lo && a <= hi) r.push_back(t.list[i]);
                }
                return r;
            }
            void writeSummary(ostream& out) const {
                out << "==== Summary ====" << endl;
                out << "Income:  " << fmt(t.totalIncome()) << endl;
                out << "Expense: " << fmt(t.totalExpense()) << endl;
                out << "Balance: " << fmt(t.balance()) << endl;
                map<string, double> totals = categoryTotals();
                string top = "";
                double best = 0;
                for (map<string, double>::const_iterator it = totals.begin(); it != totals.end(); ++it) {
                    out << "  " << it->first << ": " << fmt(it->second);
                    map<string, double>::const_iterator b = t.budgets.find(it->first);
                    if (b != t.budgets.end()) out << " (budget " << fmt(b->second) << ")";
                    out << endl;
                    double bigger = maxOf<double>(best, it->second);
                    if (bigger != best) {
                        best = bigger;
                        top = it->first;
                    }
                }
                if (!top.empty()) out << "Top spending category: " << top << " (" << fmt(best) << ")" << endl;
            }
            void printSummary() const{
                writeSummary(cout);
            }                
            void saveToFile(const string& filename) const{
                ofstream out(filename.c_str());
                if (!out) throw StorageError("Cannot write " + filename);
                writeSummary(out);

            } 
    };

    
    void printItems(const vector<const Transaction*>& items){
        if(items.empty()){
            cout<<"No matches\n"<<endl;
            return;
        }
        for(size_t i=0;i<items.size();i++){
            cout<<*items[i]<<endl;
        }
    }
}

using namespace spendWise;

int main() {
    SqliteStorage fs("spendwise.csv");
    Tracker tracker(&fs);
    Registry<string> cats;
    cats.add("Food");
    cats.add("Travel");
    cats.add("Bills");
    cats.add("Shopping");
    cats.add("Salary");
    cats.add("Other");

    try {
        tracker.load();
        tracker.collectCategories(cats);
        cout << "Loaded " << tracker.count() << " transaction(s)" << endl;
    }
    catch (StorageError& e) {
        cout << "No saved data (" << e.what() << "), starting fresh" << endl;
    }

    bool running = true;
    while (running) {
        printMenu();
        try {
            int choice = toInt(ask("Choice: "));
            if (choice == 1) {
                string d = askDate("Date (YYYY-MM-DD): ");
                string c = askCategory(cats);
                double a = toAmount(ask("Amount: "));
                string n = ask("Note: ");
                string s = ask("Source: ");
                tracker += new Income(d, c, a, n, s);
                cout << "Income added." << endl;
            } else if (choice == 2) {
                string d = askDate("Date (YYYY-MM-DD): ");
                string c = askCategory(cats);
                double a = toAmount(ask("Amount: "));
                string n = ask("Note: ");
                string pm = ask("Payment mode (cash/UPI/card): ");
                tracker += new Expense(d, c, a, n, pm);
                cout << "Expense added." << endl;
            } else if (choice == 3) {
                string d = askDate("Date (YYYY-MM-DD): ");
                string c = askCategory(cats);
                double a = toAmount(ask("Amount: "));
                string n = ask("Note: ");
                string pm = ask("Payment mode: ");
                int interval = toInt(ask("Repeat every how many days: "));
                string due = askDate("First due date (YYYY-MM-DD): ");
                tracker += new RecurringExpense(d, c, a, n, pm, interval, due);
                cout << "Recurring expense added." << endl;
            } else if (choice == 4) {
                tracker.showAll();
            } else if (choice == 5) {
                Report r(tracker);
                string how = ask("Search by 1 category, 2 date range, 3 amount range: ");
                if (how == "1") {
                    printItems(r.searchByCategory(ask("Category: ")));
                } else if (how == "2") {
                    string from = askDate("From: ");
                    string to = askDate("To: ");
                    printItems(r.searchByDateRange(from, to));
                } else if (how == "3") {
                    double lo = toAmount(ask("Min: "));
                    double hi = toAmount(ask("Max: "));
                    printItems(r.searchByAmount(lo, hi));
                } else {
                    cout << "Invalid search option." << endl;
                }
            } else if (choice == 6) {
                string c = askCategory(cats);
                double limit = toAmount(ask("Monthly limit: "));
                tracker.setBudget(c, limit);
                cout << "Budget set." << endl;
            } else if (choice == 7) {
                Report r(tracker);
                string how = ask("Report 1 summary, 2 monthly, 3 highest expense, 4 save summary to file: ");
                if (how == "1") {
                    r.printSummary();
                } else if (how == "2") {
                    r.monthlySummary(ask("Month (YYYY-MM): "), cout);
                } else if (how == "3") {
                    const Transaction* h = r.highestExpense();
                    if (h == 0) cout << "No expenses yet." << endl;
                    else cout << *h << endl;
                } else if (how == "4") {
                    r.saveToFile("spendwise_report.txt");
                    cout << "Saved to spendwise_report.txt" << endl;
                } else {
                    cout << "Invalid report option." << endl;
                }
            } else if (choice == 8) {
                int id = toInt(ask("Id to remove: "));
                cout << (tracker.removeById(id) ? "Removed." : "No such id.") << endl;
            } else if (choice == 9) {
                int n = tracker.recurringCheck(askDate("Today (YYYY-MM-DD): "));
                cout << n << " recurring expense(s) added." << endl;
            } else if (choice == 10) {
                int id = toInt(ask("Id to duplicate: "));
                cout << (tracker.duplicateById(id) ? "Duplicated." : "No such id.") << endl;
            } else if (choice == 11) {
                tracker.save();
                cout << "Saved." << endl;
            } else if (choice == 12) {
                string file = ask("Bank CSV file path: ");
                SbiImporter imp;
                int skipped = 0, dups = 0;
                vector<Transaction*> items = imp.import(file, skipped);
                int added = tracker.addImported(items, dups);
                tracker.collectCategories(cats);
                cout << imp.bankName() << " import: " << added << " added, "
                     << dups << " duplicate(s) skipped, " << skipped << " bad row(s) skipped." << endl;
            } else if (choice == 13) {
                string sure = ask("Delete ALL data for this user? Type YES to confirm: ");
                if (sure == "YES") {
                    tracker.clearAll();
                    tracker.save();
                    cout << "All data cleared." << endl;
                } else {
                    cout << "Cancelled." << endl;
                }
            } else if (choice == 0) {
                running = false;
            } else if(choice == 14) {
                FileStorage csv("csvExport.csv");
                tracker.exportTo(csv);
                cout<<"Successfully Exported\n";
            } 
            else {
                cout << "Invalid choice." << endl;
            }
        }
        catch (FinanceException& e) {
            cout << "Error: " << e.what() << endl;
            if (!cin) running = false;
        }
        catch (exception& e) {
            cout << "Unexpected error: " << e.what() << endl;
        }
    }

    try {
        tracker.save();
        cout << "Saved. Goodbye." << endl;
    }
    catch (StorageError& e) {
        cout << "Could not save: " << e.what() << endl;
    }
}

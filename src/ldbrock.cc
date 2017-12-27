#include <vector>
#include <string>
#include <iostream>
#include <cstdarg>
#include <unordered_map>
#include <fstream>
#include <ctype.h>

extern "C"
{
#include <linenoise/linenoise.h>
}

// LevelDB Headers
#include <leveldb/db.h>
#include <leveldb/write_batch.h>


using namespace std;

#define SPACECHR " \t\n"
#define SPACECHR_LEN 3

class SplitArgs
{
        public:
                using ArgVector = vector<pair<string, string>>;

                class ArgIterator
                {
                        public:
                                ArgIterator(const ArgVector &av) : av(av), idx(0) {}
                                ArgIterator(const ArgVector &av, const int i) : av(av), idx(i) {}

                                bool operator== (const ArgIterator &o) const
                                { return &av == &o.av && idx == o.idx; }
                                void next (void) { ++idx; }
                                bool end(void) const { return idx == av.size(); }

                                                
                                bool isNOption(void) const
                                { return  !av[idx].first.empty() && av[idx].second.empty(); } 
                                bool isPOption(void) const 
                                { return !av[idx].first.empty() && !av[idx].second.empty(); } 
                                bool isPosArg(void) const
                                { return av[idx].first.empty() && !av[idx].second.empty(); }

                                const string& option(void) const
                                { return av[idx].first; }
                                const string& pargument(void) const
                                { return av[idx].second; }

                        private:
                                const ArgVector &av;
                                int idx;
                };

        public:
                SplitArgs(const string &args);

                bool good(void) const { return err.empty(); }
                const string& error(void) const { return err; }

                const size_t size(void) const { return split_args.size(); }
                const ArgVector& args(void) const { return split_args; }

                bool isNOption(const int i) const
                { return split_args.size() > i
                                && !split_args[i].first.empty() && split_args[i].second.empty(); } 
                bool isPOption(const int i) const 
                { return split_args.size() > i
                                && !split_args[i].first.empty() && !split_args[i].second.empty(); } 
                bool isPosArg(const int i) const
                { return split_args.size() > i
                                && split_args[i].first.empty() && !split_args[i].second.empty(); }

                const ArgIterator operator[] (const int i) const
                { return ArgIterator(split_args, i); }
                const string& option(const int i) const
                { return split_args[i].first; }
                const string& pargument(const int i) const
                { return split_args[i].second; }

                const ArgIterator newIterator(void) const { return ArgIterator(split_args); }

        private:
                string err;
                ArgVector split_args;
};

SplitArgs::SplitArgs(const string &args)
{
        int argi = 0;
        bool opt_arg_pending = false;
        while (argi < args.length()) {
                auto eid = args.find_first_of(SPACECHR, argi, SPACECHR_LEN);
                if (eid == string::npos)
                        eid = args.length();

                if (args[argi] == '-' || args[argi] == '+') {
                        if (opt_arg_pending) {
                                err = "Missing parameter to argument: -" + split_args.back().first;
                        }

                        auto argname = args.substr(argi+1, eid);
                        if (argname.empty()) {
                                err = "Command arguments ends with -";
                        }
                        if (args[argi] == '-') {
                                typename remove_reference<decltype(split_args)>::type::value_type v{argname, ""};
                                split_args.push_back(v);
                                opt_arg_pending = true;
                        } else {
                                split_args.emplace_back(argname, "");
                        }
                } else {

                        if (opt_arg_pending) {
                                opt_arg_pending = false;
                                split_args.back().second = args.substr(argi, eid);
                        } else {
                                split_args.emplace_back("", args.substr(argi, eid));
                        }
                }
                while (eid < args.length() && isspace(args[eid])) ++eid;
                argi = eid;
        }

        if (opt_arg_pending) {
                err = "Missing parameter to arguments: -" + split_args.back().first;
        }
}


class RESTTerm;

class IRESTHandler
{
        public:
                //typedef bool (IRESTHandler::*HandlerFunc)(const char *, RESTTerm &);
                //bool echo(const char *, RESTTerm &term);
                virtual bool handleLine(char *, RESTTerm &) = 0;
};

class IWriter
{
        public:
                virtual void write(const char *data, const size_t sz) = 0;
                virtual ~IWriter(void) {}
};

class LDBCommandHandler : public IRESTHandler
{
        public:
                using DB = leveldb::DB;
                using OOptions = leveldb::Options;
                using ROptions = leveldb::ReadOptions;
                using WOptions = leveldb::WriteOptions;
                using DBStatus = leveldb::Status;
                using DBSlice  = leveldb::Slice;
                using DBIterator = leveldb::Iterator;
                using DBWriteBatch = leveldb::WriteBatch;

                LDBCommandHandler(DB *&db, const OOptions &oops,
                                          const ROptions &rops,
                                          const WOptions &wops);


        public:
                // command actions
                bool openDB(const string &args, RESTTerm &term);
                bool closeDB(const string &args, RESTTerm &term);
                bool getData(const string &args, RESTTerm &term);
                bool retrieveData(const string &args, RESTTerm &term);
                bool putData(const string &args, RESTTerm &term);
                bool loadData(const string &args, RESTTerm &term);
                bool destroyDB(const string &args, RESTTerm &term);
                bool repairDB(const string &args, RESTTerm &term);

        private:
                virtual bool handleLine(char *, RESTTerm &);

                const string __getDBValue(const string &key) const;
                void __retrieveAllData(IWriter &wt, const char kvsep, const char itsep) const;

        private:
                typedef bool (LDBCommandHandler::*CommandAction)(const string &args, RESTTerm &);
                using CommandActions = unordered_map<string, LDBCommandHandler::CommandAction>;
                static CommandActions s_actions;

                DB *&db;
                OOptions oops;
                ROptions rops;
                WOptions wops;

        public:
                class NoValueException : public std::exception
                {
                        public:
                                NoValueException(const string &key)
                                        : std::exception(), m_key(key)
                                {}

                                const string key(void) const { return m_key; }

                        private:
                                const string m_key;
                };

};



class RESTTerm : public IRESTHandler
{
        public:
                class TermWriter : public IWriter
                {
                        public:
                                TermWriter(RESTTerm *term) : m_pTerm(term) {}
                                virtual void write(const char *data, const size_t sz);

                        private:
                                RESTTerm *m_pTerm;
                };

        public:
                RESTTerm(IRESTHandler *hdl,
                                const char *prompt = "> ", const char *hist_fn = "ldbrhistory.txt");

                void run(void);
                void print(const char *format, ...);
                void println(const char *format, ...);
                void printlnResult(const char *format, ...);
                void write(const char *data, const size_t sz);
                void flush(void);
                IWriter* newWriter(void);

        private:
                virtual bool handleLine(char*, RESTTerm &);
        private:
                string prompt;
                const string history_file_name;

                IRESTHandler *hdl;

};

LDBCommandHandler::CommandActions LDBCommandHandler::s_actions = {
        {"open",        &LDBCommandHandler::openDB},
        {"close",       &LDBCommandHandler::closeDB},
        {"get",         &LDBCommandHandler::getData},
        {"retrieve",    &LDBCommandHandler::retrieveData},
        {"put",         &LDBCommandHandler::putData},
        {"load",        &LDBCommandHandler::loadData},
        {"dropdb",      &LDBCommandHandler::destroyDB},
        {"repairdb",    &LDBCommandHandler::repairDB},
};

LDBCommandHandler::LDBCommandHandler(DB *&db, const OOptions &oops,
                                              const ROptions &rops,
                                              const WOptions &wops)
        : db(db), oops(oops), rops(rops), wops(wops)
{
        db = NULL;
}

bool LDBCommandHandler::handleLine(char *line, RESTTerm &term)
{
        string actname, args;
        char *cmd_split = strchr(line, ' ');
        if (cmd_split) {
                *cmd_split = '\0';
                actname = line;
                *cmd_split = ' ';
                do { ++cmd_split; } while (isspace(*cmd_split));
                args = cmd_split;
        } else {
                actname = line;
        }

        if (actname.empty()) {
                // empty command
                return true;
        }

        CommandActions::const_iterator act = s_actions.find(actname);
        if (act != s_actions.cend()) {
                return (this->*(act->second))(args, term);
        } else {
                term.println("Undefined command[%s]!", line);
                return true;
        }
}

bool LDBCommandHandler::openDB(const string &args, RESTTerm &term)
{
        if (db) {
                term.println("There is opened db");
                return true;
        }

        string dbname;
        OOptions oopt = oops;

        SplitArgs split_args(args);

        if (!split_args.good()) {
                // error happens
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        for (auto argit = split_args.newIterator(); !argit.end(); argit.next()) {
                if (argit.isPosArg()) {
                        // positional argument
                        if (!dbname.empty()) {
                                term.println("Multiple db name specified, override previous one");
                        }
                        dbname = argit.pargument();
                } else if (argit.isNOption()) {
                        if (argit.option() == "c" || argit.option() == "+create_if_missing") {
                                oopt.create_if_missing = true;
                                term.println("Create database if missed");
                        } else {
                                term.println("Error: unrecognized option[%s].", argit.option().c_str());
                                return true;
                        }
                } else {
                        term.println("Error: unrecognized parameterized option[%s]",
                                        argit.option().c_str());
                        return true;
                }
        }

        DBStatus status = DB::Open(oopt, dbname, &db);
        if (!status.ok()) {
                term.println("Erro: DB open error:%s", status.ToString().c_str());
                return true;
        }

        return true;
}

bool LDBCommandHandler::closeDB(const string &args, RESTTerm &term)
{
        term.println("closeDB");
        if (!db) {
                term.println("No opened DB");
        }
        delete db;
        db = NULL;

        return true;
}

bool LDBCommandHandler::getData(const string &args, RESTTerm &term)
{
        if (!db) {
                term.println("There is no opened DB to get from");
                return true;
        }

        SplitArgs split_args(args);
        if (!split_args.good()) {
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        if (split_args.size() == 1) {
                if (split_args.option(0) == "a" || split_args.option(0) == "+all") {
                        IWriter *pWriter = term.newWriter();
                        __retrieveAllData(*pWriter, ':', '\n');
                        delete pWriter;
                        return true;
                } else if (split_args.isPosArg(0)) {
                        try {
                                auto v = __getDBValue(split_args.pargument(0));
                                term.printlnResult("%s : %s", split_args.pargument(0).c_str(), v.c_str());
                        } catch (const NoValueException &exc) {
                                term.println("No key named[%s]", exc.key().c_str());
                        }
                        return true;
                } 
        }

        term.println("Error: invalid argument for getData");

        return true;
}

bool LDBCommandHandler::retrieveData(const string &args, RESTTerm &term)
{
        if (!db) {
                term.println("There is no opened DB to retrieve from");
                return true;
        }

        SplitArgs split_args(args);
        if (!split_args.good()) {
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        if (split_args.size() != 1 || !split_args[0].isPosArg()) {
                term.println("retrieve <out file name>");
                return true;
        }

        string out_fname = split_args.pargument(0);

        struct FSProxy : public IWriter
        {
                FSProxy(fstream &&fs) : fstrm(move(fs)) {}
                fstream fstrm;
                void write(const char *data, const size_t sz) {
                        fstrm.write(data, sz);
                        assert(!fstrm.bad());
                }
        } fsproxy(fstream(out_fname, ios_base::out));

        __retrieveAllData(fsproxy, ':', '\n');

        return true;
}


const string LDBCommandHandler::__getDBValue(const string &key) const
{
        assert(db);
        if (key.empty())
                return string();

        ROptions ropt = rops;
        string val;
        DBStatus rtstatus = db->Get(ropt, key, &val);
        if (rtstatus.ok())
                return val;
        else {
                throw LDBCommandHandler::NoValueException(key);
        }
}

void LDBCommandHandler::__retrieveAllData(IWriter &wt, const char kvsep, const char itsep) const
{
        assert(db);
        ROptions ropt = rops;
        DBIterator *it = db->NewIterator(ropt);
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
                const auto &k = it->key();
                const auto &v = it->value();
                wt.write(k.data(), k.size());
                wt.write(&kvsep, 1);
                wt.write(v.data(), v.size());
                wt.write(&itsep, 1);
        }
}

bool LDBCommandHandler::putData(const string &args, RESTTerm &term)
{
        if (!db) {
                term.println("There is no opened DB to write to");
                return true;
        }

        SplitArgs split_args(args);
        if (!split_args.good()) {
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        if (split_args.size() != 2 || !split_args.isPosArg(0) || !split_args.isPosArg(1)) {
                term.println("put <key> <value>");
                return true;
        }

        WOptions wopt = wops;
        DBStatus status = db->Put(wopt, split_args.pargument(0), split_args.pargument(1));
        if (!status.ok()) {
                term.println("Fail to put data into DB");
                return true;
        }
        
        return true;
}

bool LDBCommandHandler::loadData(const string &args, RESTTerm &term)
{
        if (!db) {
                term.println("There is no opened DB to write to");
                return true;
        }

        SplitArgs split_args(args);
        if (!split_args.good()) {
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        if (split_args.size() != 1 || !split_args.isPosArg(0)) {
                term.println("load <filename>");
                return true;
        }

        fstream infl(split_args.pargument(0), ios_base::in);
        if (!infl) {
                term.println("Fail to open data input file");
                return true;
        }

        DBWriteBatch wb;
        string ln;
        while (std::getline(infl, ln)) {
                const size_t sep = ln.find(',');
                if (sep == string::npos) {
                        term.println("Mal-formated data input file");
                        infl.close();
                        return true;
                }
                const string key = ln.substr(0, sep);
                const string val = ln.substr(sep+1);
                wb.Put(key, val);
        }
        WOptions wopt = wops;
        db->Write(wopt, &wb);

        return true;
}

bool LDBCommandHandler::destroyDB(const string &args, RESTTerm &term)
{
        if (db) {
                term.println("There is opened DB, close it first!");
                return true;
        }

        SplitArgs split_args(args);
        if (!split_args.good()) {
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        if (split_args.size() != 1 || !split_args.isPosArg(0)) {
                term.println("destroydb <dbname>");
                return true;
        }

        OOptions oopt = oops;
        DBStatus status = leveldb::DestroyDB(split_args.pargument(0), oopt);
        if (status.ok()) {
                term.println("Successfully destroy db");
                return true;
        } else {
                term.println("Fail to destroy db");
                return true;
        }
}

bool LDBCommandHandler::repairDB(const string &args, RESTTerm &term)
{
        if (db) {
                term.println("There is opened DB, close it first!");
                return true;
        }

        SplitArgs split_args(args);
        if (!split_args.good()) {
                term.println("Error: %s", split_args.error().c_str());
                return true;
        }

        if (split_args.size() != 1 || !split_args.isPosArg(0)) {
                term.println("repairdb <dbname>");
                return true;
        }

        OOptions oopt = oops;
        DBStatus status = leveldb::RepairDB(split_args.pargument(0), oopt);
        if (status.ok()) {
                term.println("Successfully repair db");
                return true;
        } else {
                term.println("Fail to repair db");
                return true;
        }
        return true;
}

void RESTTerm::TermWriter::write(const char *data, const size_t sz)
{
        // add lock if neccessary
        m_pTerm->write(data, sz);
        m_pTerm->flush();
}

RESTTerm::RESTTerm(IRESTHandler *hdl, const char *prompt, const char *hist_fn)
        :prompt(prompt),
        history_file_name(hist_fn),
        hdl(hdl)
{
        if (!history_file_name.empty())
                linenoiseHistoryLoad(history_file_name.c_str());
}

void RESTTerm::print(const char *format, ...)
{
        va_list vl;
        va_start(vl, format);
        vprintf(format, vl);
        va_end(vl);
}

void RESTTerm::println(const char *format, ...)
{
        va_list vl;
        va_start(vl, format);
        vprintf(format, vl);
        va_end(vl);
        if (format[strlen(format)-1] != '\n')
                printf("\n");
        flush();
}

void RESTTerm::printlnResult(const char *format, ...)
{
        printf("=> ");
        va_list vl;
        va_start(vl, format);
        vprintf(format, vl);
        va_end(vl);
        if (format[strlen(format)-1] != '\n')
                printf("\n");
        flush();
}

IWriter* RESTTerm::newWriter(void)
{
        return new TermWriter(this);
}


void RESTTerm::write(const char *data, const size_t sz)
{
        fwrite(data, sizeof(*data), sz, stdout);
}

void RESTTerm::flush(void)
{
        fflush(stdout);
}

bool RESTTerm::handleLine(char *line, RESTTerm &term)
{
        term.println("Echo: %s", line);
        return true;
}

void RESTTerm::run(void)
{
        char *line;

        while ((line = linenoise(prompt.c_str()))) {
                //cout << "echo:" << line << endl;
                auto cont = hdl->handleLine(line, *this);
                if (cont)
                        linenoiseHistoryAdd(line);

                linenoiseFree(line);

                if (!cont)
                        break;
        }
        linenoiseHistorySave(history_file_name.c_str());
}

int main(int argc, char **argv)
{
        leveldb::DB *db;
        leveldb::Options oops;
        leveldb::ReadOptions rops;
        leveldb::WriteOptions wops;
        LDBCommandHandler ldbh(db, oops, rops, wops);
        RESTTerm term(&ldbh, "> ");
        term.run();

        return 0;
}

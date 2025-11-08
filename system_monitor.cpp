// system_monitor.cpp
#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/select.h>
using namespace std;

struct ProcInfo { int pid; string name; double cpu; double mem; long rss_kb; };

double getMemoryUsagePercent(long long &total_kb_out) {
    ifstream f("/proc/meminfo"); string key; long long total_kb=0, available_kb=0;
    while (f >> key) {
        if (key=="MemTotal:") f >> total_kb;
        else if (key=="MemAvailable:") { f >> available_kb; break; }
        else { string tmp; getline(f,tmp); }
    }
    total_kb_out = total_kb;
    if (!total_kb) return 0.0;
    double used = double(total_kb - available_kb);
    return (used / total_kb) * 100.0;
}

struct CpuSnapshot {
    unsigned long long user=0,nice=0,system=0,idle=0,iowait=0,irq=0,softirq=0,steal=0;
    unsigned long long total() const { return user+nice+system+idle+iowait+irq+softirq+steal; }
    unsigned long long idle_all() const { return idle + iowait; }
};

CpuSnapshot readCpuSnapshot() {
    CpuSnapshot s; ifstream f("/proc/stat"); string line;
    if (!getline(f,line)) return s;
    istringstream iss(line); string cpu; iss >> cpu >> s.user >> s.nice >> s.system >> s.idle >> s.iowait >> s.irq >> s.softirq >> s.steal;
    return s;
}

vector<ProcInfo> getProcessesFromPs(int topN=50, bool normalize=true) {
    vector<ProcInfo> procs; int nproc = sysconf(_SC_NPROCESSORS_ONLN);
    string cmd = "ps -eo pid,comm,pcpu,pmem,rss --no-headers --sort=-pcpu | head -n " + to_string(topN);
    FILE *fp = popen(cmd.c_str(),"r"); if (!fp) return procs;
    char buf[4096];
    while (fgets(buf,sizeof(buf),fp)) {
        int pid=0; char comm[256]; double pcpu=0, pmem=0; long rss=0;
        int scanned = sscanf(buf," %d %255s %lf %lf %ld",&pid,comm,&pcpu,&pmem,&rss);
        if (scanned>=5) {
            ProcInfo p; p.pid = pid; p.name = string(comm);
            p.cpu = (normalize && nproc>0) ? pcpu / double(nproc) : pcpu;
            p.mem = pmem; p.rss_kb = rss; procs.push_back(p);
        }
    }
    pclose(fp);
    return procs;
}

// Terminal helpers
struct TermGuard {
    termios orig{};
    bool changed=false;
    TermGuard() { if(isatty(STDIN_FILENO)){ tcgetattr(STDIN_FILENO,&orig); termios t=orig; t.c_lflag &= ~(ICANON|ECHO); t.c_cc[VMIN]=0; t.c_cc[VTIME]=0; tcsetattr(STDIN_FILENO,TCSANOW,&t); int f=fcntl(STDIN_FILENO,F_GETFL,0); fcntl(STDIN_FILENO,F_SETFL,f|O_NONBLOCK); changed=true;
        cout << "\033[?1049h\033[?25l" << flush;
    }}
    ~TermGuard(){ if(changed){ 
            tcsetattr(STDIN_FILENO,TCSANOW,&orig);
            int f=fcntl(STDIN_FILENO,F_GETFL,0); fcntl(STDIN_FILENO,F_SETFL,f & ~O_NONBLOCK);
            cout << "\033[?25h\033[?1049l" << flush;
        }}
} termGuard;

string tryKillPid(int pid){
    if(kill(pid,SIGTERM)==0) return "Sent SIGTERM to PID " + to_string(pid);
    else return string("Failed to kill PID ") + to_string(pid) + " : " + strerror(errno);
}

// non-blocking line reader
string readAvailableLine() {
    static string buffer;
    char tmp[256];
    ssize_t r = read(STDIN_FILENO, tmp, sizeof(tmp));
    while (r > 0) {
        buffer.append(tmp, tmp + r);
        size_t pos;
        if ((pos = buffer.find('\n')) != string::npos) {
            string line = buffer.substr(0,pos);
            buffer.erase(0,pos+1);
            return line;
        }
        r = read(STDIN_FILENO, tmp, sizeof(tmp));
    }
    return string();
}

void showHeaderInstructions(int topN, const string &sortMode) {
    cout << "\033[1;36m================== System Monitor ==================\n\033[0m";
    cout << "Commands: q (quit) | k <PID> (kill) | s cpu | s mem | n <N> (top N) | h (help)\n";
    cout << "Current: sort=" << sortMode << " | topN=" << topN << "\n";
}

int main(){
    ios::sync_with_stdio(false); cin.tie(nullptr);
    const int refresh_seconds = 2;
    int topN = 10; enum SortMode {BY_CPU, BY_MEM}; SortMode sortMode = BY_CPU;
    CpuSnapshot prevCpu = readCpuSnapshot();
    string statusMessage;

    // main loop
    while (true) {
        // handle user command if available
        string cmdline = readAvailableLine();
        if (!cmdline.empty()) {
            while (!cmdline.empty() && isspace((unsigned char)cmdline.back())) cmdline.pop_back();
            while (!cmdline.empty() && isspace((unsigned char)cmdline.front())) cmdline.erase(cmdline.begin());
            if (!cmdline.empty()) {
                stringstream ss(cmdline); string cmd; ss >> cmd;
                if (cmd=="q") { cout << "Quitting...\n"; return 0; }
                else if (cmd=="k") { int pid; if (ss>>pid) statusMessage = tryKillPid(pid); else statusMessage="Usage: k <PID>"; }
                else if (cmd=="s") { string w; if (ss>>w){ if(w=="cpu"){ sortMode=BY_CPU; statusMessage="Sorting by CPU"; } else if(w=="mem"){ sortMode=BY_MEM; statusMessage="Sorting by Memory"; } else statusMessage="Usage: s cpu|mem"; } else statusMessage="Usage: s cpu|mem"; }
                else if (cmd=="n") { int n; if (ss>>n && n>0){ topN=n; statusMessage="topN set to "+to_string(n);} else statusMessage="Usage: n <N>"; }
                else if (cmd=="h") statusMessage="Commands: q k <PID> s cpu|mem n <N>";
                else statusMessage = string("Unknown command: ") + cmd;
            }
        }

        // read overall cpu and memory
        CpuSnapshot curCpu = readCpuSnapshot();
        unsigned long long prevTotal = prevCpu.total(), curTotal = curCpu.total();
        unsigned long long totalDiff = curTotal - prevTotal, idleDiff = curCpu.idle_all() - prevCpu.idle_all();
        double cpuUsage = (totalDiff? (1.0 - double(idleDiff)/double(totalDiff)) * 100.0 : 0.0);
        prevCpu = curCpu;
        long long total_kb=0; double memUsage = getMemoryUsagePercent(total_kb);

        // get processes and sort
        vector<ProcInfo> procs = getProcessesFromPs(max(topN,50), true);
        if (sortMode==BY_CPU) sort(procs.begin(), procs.end(), [](auto &a, auto &b){ if(a.cpu!=b.cpu) return a.cpu>b.cpu; return a.mem>b.mem; });
        else sort(procs.begin(), procs.end(), [](auto &a, auto &b){ if(a.mem!=b.mem) return a.mem>b.mem; return a.cpu>b.cpu; });

        // clear screen and render single box
        cout << "\033[H\033[J"; 
        showHeaderInstructions(topN, sortMode==BY_CPU? "CPU":"MEM");
        auto t = chrono::system_clock::now(); time_t tt = chrono::system_clock::to_time_t(t);
        cout << "Time: " << put_time(localtime(&tt), "%Y-%m-%d %H:%M:%S") << "\n";
        cout << fixed << setprecision(2);
        cout << "CPU Usage (overall): " << cpuUsage << "%    ";
        cout << "Memory Usage: " << memUsage << "% (" << (total_kb/1024.0) << " MB total)\n";
        cout << "----------------------------------------------------\n";
        cout << left << setw(8) << "PID" << setw(25) << "Name" << setw(12) << "CPU(%)" << setw(10) << "Mem(%)" << setw(12) << "RSS(KB)" << "\n";
        cout << "----------------------------------------------------\n";

        int shown=0;
        for (auto &pr : procs) {
            if (shown++ >= topN) break;
            string color = "\033[0m";
            if (pr.cpu >= 50.0) color = "\033[1;31m"; else if (pr.cpu >= 10.0) color = "\033[1;33m"; else color = "\033[1;32m";
            cout << color << left << setw(8) << pr.pid << setw(25) << (pr.name.size()>23? pr.name.substr(0,22)+"..":pr.name)
                 << setw(12) << pr.cpu << setw(10) << pr.mem << setw(12) << pr.rss_kb << "\033[0m" << "\n";
        }
        cout << "----------------------------------------------------\n";
        if (!statusMessage.empty()) { cout << "-- " << statusMessage << "\n"; statusMessage.clear(); }
        else cout << "Type 'h' for help. Commands are non-blocking and processed between refreshes.\n";
        cout.flush();

        // sleep in small chunks so input can be read often (keeps UI responsive)
        int total_ms = refresh_seconds * 1000; const int chunk = 200;
        for (int waited=0; waited<total_ms; waited+=chunk) {
            usleep(chunk * 1000);
            // we intentionally do not block here; readAvailableLine() will check next loop
        }
    }

    return 0;
}


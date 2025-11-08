# System Monitor (Interactive)

An interactive System Monitoring Tool built in C++ that displays real-time information about CPU and memory usage along with process statistics.  
It includes features like process sorting, live refresh, and the ability to kill processes directly from the terminal.

---

## Project Overview

This project was developed as part of a 5-day development plan to build a simple yet powerful system resource monitor.  
It reads live system data using `/proc` files and Linux system calls and presents it in a clean, continuously updating console interface.

---

## 5-Day Development Plan

| Day | Goal | Implementation |
|-----|------|----------------|
| **Day 1** | Design UI layout and gather system data using system calls | Created basic structure and read CPU & memory info from `/proc/stat` and `/proc/meminfo`. |
| **Day 2** | Display process list with CPU and memory usage | Integrated `ps` command parsing to display running processes and stats. |
| **Day 3** | Implement process sorting by CPU and memory usage | Added sorting functionality (toggle with `s cpu` or `s mem`). |
| **Day 4** | Add functionality to kill processes | Enabled `k <PID>` command to send `SIGTERM` to a process. |
| **Day 5** | Implement real-time update and interactive features | Added dynamic screen refresh every few seconds, non-blocking input, and help commands. |

---

## Features

- Live CPU and Memory Usage (from `/proc`)
- Real-time Process Table Refresh (every few seconds)
- Process Sorting by CPU or Memory usage
- Kill Processes directly from within the tool
- Interactive Commands (non-blocking input)
- Works on WSL and Linux
- Clean terminal UI with color-coded CPU usage
- Single-screen update mode (no repeated boxes or clutter)

---

## Controls and Commands

| Command | Description |
|----------|-------------|
| `q` | Quit the system monitor |
| `k <PID>` | Kill a process by its PID |
| `s cpu` | Sort processes by CPU usage |
| `s mem` | Sort processes by Memory usage |
| `n <N>` | Display top N processes |
| `h` | Show help message |

You can type these commands anytime — the monitor updates continuously while listening for your input.

---

## Installation and Usage

### Step 1: Prerequisites
Make sure you have:
- Linux or WSL (Ubuntu recommended)
- `g++` compiler (C++17 or higher)
- `ps` and `/proc` filesystem available (default in Linux)

### Step 2: Clone the Repository
```bash
git clone https://github.com/<your-username>/system-monitor.git
cd system-monitor
```

### Step 3: Compile
```bash
g++ system_monitor.cpp -o system_monitor -std=c++17
```

### Step 4: Run the Program
```bash
./system_monitor
```

### Step 5: (Optional) Generate Load for Testing
```bash
yes > /dev/null &
```
You should see the CPU usage increase in real time.

---

## Example Output

```
================== System Monitor ==================
Commands: q (quit) | k <PID> (kill) | s cpu | s mem | n <N> | h (help)
Current: sort=CPU | topN=10
Time: 2025-11-09 00:00:00
CPU Usage (overall): 45.23%    Memory Usage: 4.76% (11874.22 MB total)
----------------------------------------------------
PID     Name                     CPU(%)      Mem(%)    RSS(KB)
----------------------------------------------------
1023    yes                      23.41       0.00      1536
1001    bash                     0.01        0.10      5200
...
----------------------------------------------------
Type 'h' for help. Commands are non-blocking and processed between refreshes.
```

---

## Technical Details

- Language: C++17  
- Core system data: `/proc/stat`, `/proc/meminfo`  
- Process list: `ps -eo pid,comm,pcpu,pmem,rss`  
- Terminal control: ANSI escape sequences (`\033`) for live updates  
- Input handling: Non-blocking I/O using `termios` and `fcntl`  
- Signal handling: Uses `SIGTERM` to kill processes  

---

## License

This project is licensed under the **MIT License**.  
You are free to use, modify, and distribute it with attribution.

---

## Author

**Darshan Digbijaya Dash**  
B.Tech CSIT Student at SIKSHA 'O' ANUSANDHAN (ITER)  
Email: [darshan.ommdash@gmail.com](mailto:darshan.ommdash@gmail.com)  
LinkedIn: [linkedin.com/in/darshan-digbijay-dash-a1a91827b](https://www.linkedin.com/in/darshan-digbijay-dash-a1a91827b/)  
GitHub: [github.com/Darshan85246](https://github.com/Darshan85246)

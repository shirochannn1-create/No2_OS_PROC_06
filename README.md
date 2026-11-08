# Process Management Simulator

A small Windows desktop application written in C++17 and Win32 API. It demonstrates three operating-system concepts:

1. Process states
2. Process Control Block (PCB)
3. Context switching

The project is designed for **Visual Studio 2022 Community or Professional**. It does not require Qt, CMake, MSYS2, MinGW, or third-party GUI libraries.

## 1. Features

- Display the five process states: New, Ready, Running, Waiting, and Terminated.
- Load process definitions from a CSV file.
- View each process in a process table.
- Inspect the PCB fields: PID, state, priority, PC, SP, AX, BX, CX, DX, FLAGS, and executed instruction count.
- Admit a process: New -> Ready.
- Dispatch a Ready process to the CPU.
- Switch the CPU to another Ready process. The simulator saves the old CPU context to its PCB and restores the target process context from its PCB.
- Execute one simulated instruction.
- Block the running process for I/O: Running -> Waiting.
- Complete I/O: Waiting -> Ready.
- Terminate the running process: Running -> Terminated.
- Export the process table and event trace as two CSV files.
- Build and run a separate C++ test project.

**This project intentionally does not implement CPU scheduling algorithms.** The user manually selects the process that receives the CPU. Priority is displayed as process metadata and is not used to select a process.

## 2. Requirements

- Windows 10 or Windows 11, 64-bit
- Visual Studio 2022
- Workload: Desktop development with C++
- MSVC v143 (VS 2022 C++ x64/x86 build tools)
- Windows 10 SDK or Windows 11 SDK

No Qt or other external dependency is required.

## 3. Build the application

1. Extract the project ZIP to a normal folder, for example `C:\Projects\ProcessStatePCB_English`.
2. Open `ProcessManagementSimulator.sln` in Visual Studio 2022.
3. Select `Debug` and `x64` on the Visual Studio toolbar.
4. Select **Build -> Build Solution** or press `Ctrl + Shift + B`.
5. Select the `ProcessManagementSimulator` project as the startup project if it is not already selected.
6. Press `F5` to run with the debugger, or `Ctrl + F5` to run without the debugger.

The application executable is created at:

```text
bin\x64\Debug\ProcessManagementSimulator.exe
```

For a distributable build, select `Release | x64` and build again:

```text
bin\x64\Release\ProcessManagementSimulator.exe
```

The build copies `input\sample_processes.csv` next to the executable under an `input` folder. This lets the sample-data button work when the executable is launched from its output folder.

## 4. Run the tests

The solution contains a second project named `SimulatorTests`.

1. Right-click `SimulatorTests` in Solution Explorer.
2. Select **Set as Startup Project**.
3. Press `Ctrl + F5`.

The test program prints each check and returns exit code `0` only when every test passes. After testing, right-click `ProcessManagementSimulator` and set it as the startup project again.

You can also run `run_tests.bat` from **Developer PowerShell for VS 2022**. The script uses the MSVC compiler available in that terminal.

## 5. Use the application

### Toolbar

- **Load Sample**: load `input\sample_processes.csv`.
- **Import CSV**: select another process input file.
- **Reset**: restore all processes and PCB values to their initial state.
- **Export CSV**: save the current process table and event trace.

### Process actions

Select a row in the process table, then use the action buttons:

| Button | Allowed source state | Result |
|---|---|---|
| Admit | New | Ready |
| Switch CPU | Ready | Selected process becomes Running; the old Running process returns to Ready |
| Execute One Step | Running | Increments PC and the simulated AX register |
| Request I/O | Running | Waiting; the CPU context is saved to the PCB |
| Complete I/O | Waiting | Ready |
| Terminate | Running | Terminated; the final CPU context is saved to the PCB |

Disabled buttons indicate that the selected process cannot perform that action in its current state.

### Context switch example

1. Admit PID 1 and PID 2.
2. Select PID 1 and click **Switch CPU**.
3. Click **Execute One Step** one or more times.
4. Select PID 2 and click **Switch CPU**.
5. Check the Event Log for `Context Save` for PID 1 and `Context Restore` for PID 2.
6. Select PID 1 and switch back to it. Its saved PC and registers should be restored.

## 6. CSV input format

The input file must contain these ten columns in this order:

```csv
pid,name,priority,pc,sp,ax,bx,cx,dx,flags
1,Text Editor,3,100,4096,10,20,30,40,512
2,Web Browser,2,200,8192,50,60,70,80,514
```

Validation rules:

- `pid`: integer greater than zero; every PID must be unique.
- `name`: non-empty text. Quoted names may contain commas.
- `priority`: integer from 0 to 255. It is displayed only; it does not control CPU selection.
- `pc` and `sp`: unsigned 32-bit integers.
- `ax`, `bx`, `cx`, `dx`, and `flags`: integers from 0 to 65535.
- Each CSV record must fit on one line.

All input processes start in the New state. Their initial CPU register values are copied into their PCB.

## 7. CSV output

When the user exports `process_results.csv`, the program also creates `process_results_trace.csv` in the same folder.

- Results CSV: current state, priority, PCB/CPU register values, and executed instruction count for each process.
- Trace CSV: simulation time, PID, and every recorded event, including state changes and context save/restore operations.

## 8. Project structure

```text
ProcessStatePCB_English/
|-- ProcessManagementSimulator.sln
|-- ProcessManagementSimulator.vcxproj
|-- README.md
|-- src/
|   |-- Main.cpp          # Win32 desktop GUI
|   |-- Simulator.h       # Process, PCB, event, and simulator declarations
|   |-- Simulator.cpp     # State transitions and context-switch logic
|   |-- Csv.h             # CSV interface
|   `-- Csv.cpp           # CSV validation, import, and export
|-- tests/
|   |-- SimulatorTests.vcxproj
|   `-- SimulatorTests.cpp
|-- input/
|   `-- sample_processes.csv
`-- docs/
    |-- ALGORITHM.md
    |-- REQUIREMENTS.md
    |-- TEST_PLAN.md
    `-- test_results.txt
```

## 9. Model scope and limitations

This is an educational simulator, not a real operating-system kernel. It does not create operating-system processes or execute machine code. `Execute One Step` performs a small deterministic update to the simulated PC and AX register so that context preservation can be observed.

The simulator counts a context switch when it changes directly from one Running process to another Running process. The initial dispatch from an idle CPU and a transition to an idle CPU (I/O wait or termination) are recorded in the event log but are not included in that process-to-process counter.

The five states and transitions implemented are:

```text
New -> Ready -> Running -> Terminated
                    |  ^
                    v  |
                 Waiting
                    |
                    v
                  Ready

Running -> Ready  (when the user switches the CPU to another Ready process)
```

Priority is process information only. No FCFS, Round Robin, time quantum, automatic dispatcher, or scheduling policy is included.

## 10. Troubleshooting

- **Platform Toolset v143 is missing**: open Visual Studio Installer, select Modify, and install `MSVC v143 - VS 2022 C++ x64/x86 build tools`.
- **Windows SDK is missing**: install a Windows 10 or Windows 11 SDK in the same Visual Studio Installer screen.
- **The sample file is not found**: build the project again so the post-build step copies the sample CSV, or use Import CSV to select the file manually.
- **The application opens but shows no processes**: click Load Sample or Import CSV.
- **A button is disabled**: select a process whose state allows that action.

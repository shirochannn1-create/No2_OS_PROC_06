#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include "Csv.h"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <exception>
#include <functional>
#include <iterator>
#include <utility>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

using namespace process_sim;
namespace fs = std::filesystem;

namespace {

enum ControlId {
    IdLoadSample = 100,
    IdImportCsv,
    IdReset,
    IdExportCsv,
    IdAdmit,
    IdSwitchCpu,
    IdExecute,
    IdRequestIo,
    IdCompleteIo,
    IdTerminate,
    IdProcessList,
    IdPcbView,
    IdEventLog,
    IdStatus,
    IdStateFlow
};

struct Application {
    HINSTANCE instance = nullptr;
    HWND window = nullptr;
    HWND title = nullptr;
    HWND subtitle = nullptr;
    HWND stateFlow = nullptr;
    HWND processHeading = nullptr;
    HWND pcbHeading = nullptr;
    HWND logHeading = nullptr;
    HWND processList = nullptr;
    HWND pcbView = nullptr;
    HWND eventLog = nullptr;
    HWND status = nullptr;
    HWND buttons[10]{};
    HFONT normalFont = nullptr;
    HFONT titleFont = nullptr;
    HFONT monoFont = nullptr;
    std::unique_ptr<Simulator> simulator;
    fs::path projectRoot;
    int selectedPid = -1;
    bool updating = false;
} app;

std::wstring toWide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return L"[Text conversion error]";
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        result.data(), size);
    return result;
}

fs::path executableFolder() {
    wchar_t path[MAX_PATH * 4]{};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    return fs::path(path).parent_path();
}

fs::path findProjectRoot() {
    fs::path candidate = fs::current_path();
    for (int level = 0; level < 5; ++level) {
        if (fs::exists(candidate / L"input" / L"sample_processes.csv")) return candidate;
        if (candidate == candidate.root_path()) break;
        candidate = candidate.parent_path();
    }

    candidate = executableFolder();
    for (int level = 0; level < 5; ++level) {
        if (fs::exists(candidate / L"input" / L"sample_processes.csv")) return candidate;
        if (candidate == candidate.root_path()) break;
        candidate = candidate.parent_path();
    }
    return fs::current_path();
}

HWND makeControl(const wchar_t* className, const wchar_t* text,
                 DWORD style, int id, DWORD extendedStyle = 0) {
    return CreateWindowExW(extendedStyle, className, text,
        WS_CHILD | WS_VISIBLE | style,
        0, 0, 10, 10, app.window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        app.instance, nullptr);
}

void applyFont(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void addListColumn(int index, const wchar_t* title, int width) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = const_cast<LPWSTR>(title);
    column.cx = width;
    column.iSubItem = index;
    ListView_InsertColumn(app.processList, index, &column);
}

int selectedRow() {
    return ListView_GetNextItem(app.processList, -1, LVNI_SELECTED);
}

const Process* selectedProcess() {
    if (!app.simulator || app.selectedPid < 0) return nullptr;
    try {
        return &app.simulator->processByPid(app.selectedPid);
    } catch (...) {
        return nullptr;
    }
}

void showError(const std::exception& error) {
    const std::wstring message = toWide(error.what());
    MessageBoxW(app.window, message.c_str(), L"Operation failed", MB_OK | MB_ICONWARNING);
}

void updateUi();

void perform(const std::function<void()>& action) {
    try {
        action();
        updateUi();
    } catch (const std::exception& error) {
        showError(error);
    }
}

std::wstring contextText(const CpuContext& context) {
    std::wstringstream text;
    text << L"Program Counter (PC): " << context.pc << L"\r\n"
         << L"Stack Pointer (SP):  " << context.sp << L"\r\n"
         << L"AX: 0x" << std::hex << context.ax << L"\r\n"
         << L"BX: 0x" << std::hex << context.bx << L"\r\n"
         << L"CX: 0x" << std::hex << context.cx << L"\r\n"
         << L"DX: 0x" << std::hex << context.dx << L"\r\n"
         << L"FLAGS: 0x" << std::hex << context.flags << L"\r\n";
    return text.str();
}

void updateUi() {
    if (!app.simulator || app.updating) return;
    app.updating = true;

    const auto& processes = app.simulator->processes();
    const Process* selected = selectedProcess();
    if (!selected && !processes.empty()) {
        app.selectedPid = app.simulator->runningPid() != -1
            ? app.simulator->runningPid() : processes.front().pid;
    }

    ListView_DeleteAllItems(app.processList);
    int stateCounts[5]{};
    int selectedItem = -1;

    for (std::size_t row = 0; row < processes.size(); ++row) {
        const Process& process = processes[row];
        switch (process.state) {
        case ProcessState::New: ++stateCounts[0]; break;
        case ProcessState::Ready: ++stateCounts[1]; break;
        case ProcessState::Running: ++stateCounts[2]; break;
        case ProcessState::Waiting: ++stateCounts[3]; break;
        case ProcessState::Terminated: ++stateCounts[4]; break;
        }

        const CpuContext context = app.simulator->contextForPid(process.pid);
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(row);
        std::wstring pid = std::to_wstring(process.pid);
        item.pszText = pid.data();
        ListView_InsertItem(app.processList, &item);

        std::wstringstream ax;
        ax << L"0x" << std::hex << context.ax;
        const std::vector<std::wstring> values = {
            toWide(process.name),
            toWide(stateName(process.state)),
            std::to_wstring(process.priority),
            std::to_wstring(context.pc),
            std::to_wstring(context.sp),
            ax.str()
        };
        for (int column = 0; column < static_cast<int>(values.size()); ++column) {
            ListView_SetItemText(app.processList, static_cast<int>(row), column + 1,
                const_cast<LPWSTR>(values[column].c_str()));
        }

        if (process.pid == app.selectedPid) selectedItem = static_cast<int>(row);
    }

    if (selectedItem >= 0) {
        ListView_SetItemState(app.processList, selectedItem,
            LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    std::wstringstream flow;
    flow << L"NEW: " << stateCounts[0]
         << L"     READY: " << stateCounts[1]
         << L"     RUNNING: " << stateCounts[2]
         << L"     WAITING: " << stateCounts[3]
         << L"     TERMINATED: " << stateCounts[4];
    SetWindowTextW(app.stateFlow, flow.str().c_str());

    selected = selectedProcess();
    if (selected) {
        std::wstringstream pcb;
        pcb << L"PID: " << selected->pid << L"\r\n"
            << L"Name: " << toWide(selected->name) << L"\r\n"
            << L"State: " << toWide(stateName(selected->state)) << L"\r\n"
            << L"Priority: " << selected->priority << L"\r\n"
            << L"Executed instructions: " << selected->executedInstructions << L"\r\n\r\n"
            << (selected->state == ProcessState::Running
                ? L"ACTIVE CPU CONTEXT\r\n" : L"SAVED PCB CONTEXT\r\n")
            << contextText(app.simulator->contextForPid(selected->pid));
        SetWindowTextW(app.pcbView, pcb.str().c_str());
    } else {
        SetWindowTextW(app.pcbView, L"Select a process to inspect its PCB.");
    }

    std::wstringstream log;
    const auto& events = app.simulator->events();
    const std::size_t first = events.size() > 100 ? events.size() - 100 : 0;
    for (std::size_t i = first; i < events.size(); ++i) {
        log << L"t=" << events[i].time << L"  ";
        if (events[i].pid > 0) log << L"PID " << events[i].pid << L"  ";
        log << toWide(events[i].message) << L"\r\n";
    }
    SetWindowTextW(app.eventLog, log.str().c_str());
    SendMessageW(app.eventLog, EM_SETSEL,
        GetWindowTextLengthW(app.eventLog), GetWindowTextLengthW(app.eventLog));
    SendMessageW(app.eventLog, EM_SCROLLCARET, 0, 0);

    const int runningPid = app.simulator->runningPid();
    std::wstring status = runningPid == -1
        ? L"CPU: IDLE" : L"CPU: Running PID " + std::to_wstring(runningPid);
    status += L"     |     Context switches: " + std::to_wstring(app.simulator->contextSwitchCount());
    status += L"     |     Simulation time: " + std::to_wstring(app.simulator->simulationTime());
    SetWindowTextW(app.status, status.c_str());

    const bool hasProcess = selected != nullptr;
    const ProcessState state = hasProcess ? selected->state : ProcessState::Terminated;
    EnableWindow(app.buttons[4], hasProcess && state == ProcessState::New);
    EnableWindow(app.buttons[5], hasProcess && state == ProcessState::Ready);
    EnableWindow(app.buttons[6], hasProcess && state == ProcessState::Running);
    EnableWindow(app.buttons[7], hasProcess && state == ProcessState::Running);
    EnableWindow(app.buttons[8], hasProcess && state == ProcessState::Waiting);
    EnableWindow(app.buttons[9], hasProcess && state == ProcessState::Running);

    app.updating = false;
}

void loadCsv(const fs::path& path) {
    std::vector<Process> processes = loadProcessesCsv(path);
    app.simulator = std::make_unique<Simulator>(std::move(processes));
    app.selectedPid = app.simulator->processes().front().pid;
    updateUi();
    SetWindowTextW(app.status, (L"Loaded: " + path.filename().wstring()).c_str());
}

void loadSample() {
    const fs::path sample = app.projectRoot / L"input" / L"sample_processes.csv";
    if (!fs::exists(sample)) {
        throw std::runtime_error("Sample CSV was not found. Use Import CSV to select a file.");
    }
    loadCsv(sample);
}

void importCsvDialog() {
    wchar_t fileName[MAX_PATH * 4]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = app.window;
    dialog.lpstrFilter = L"CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = static_cast<DWORD>(std::size(fileName));
    const std::wstring initialFolder = (app.projectRoot / L"input").wstring();
    dialog.lpstrInitialDir = initialFolder.c_str();
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&dialog)) loadCsv(fs::path(fileName));
}

void exportCsvDialog() {
    if (!app.simulator) throw std::runtime_error("Load process data before exporting.");

    wchar_t fileName[MAX_PATH * 4] = L"process_results.csv";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = app.window;
    dialog.lpstrFilter = L"CSV files (*.csv)\0*.csv\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = static_cast<DWORD>(std::size(fileName));
    dialog.lpstrDefExt = L"csv";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileNameW(&dialog)) {
        const fs::path resultPath(fileName);
        exportResultsCsv(*app.simulator, resultPath);
        const fs::path tracePath = resultPath.parent_path() /
            (resultPath.stem().string() + "_trace.csv");
        MessageBoxW(app.window,
            (L"Export completed.\r\n\r\nResults: " + resultPath.wstring() +
             L"\r\nTrace: " + tracePath.wstring()).c_str(),
            L"CSV export", MB_OK | MB_ICONINFORMATION);
    }
}

void createControls() {
    app.normalFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");
    app.titleFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");
    app.monoFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH, L"Consolas");

    app.title = makeControl(L"STATIC", L"PROCESS STATE / PCB / CONTEXT SWITCH", SS_LEFT, 0);
    app.subtitle = makeControl(L"STATIC", L"A simple interactive process management simulator", SS_LEFT, 0);

    app.buttons[0] = makeControl(L"BUTTON", L"Load Sample", BS_PUSHBUTTON, IdLoadSample);
    app.buttons[1] = makeControl(L"BUTTON", L"Import CSV", BS_PUSHBUTTON, IdImportCsv);
    app.buttons[2] = makeControl(L"BUTTON", L"Reset", BS_PUSHBUTTON, IdReset);
    app.buttons[3] = makeControl(L"BUTTON", L"Export CSV", BS_PUSHBUTTON, IdExportCsv);

    app.stateFlow = makeControl(L"STATIC",
        L"NEW -> READY -> RUNNING -> READY / WAITING / TERMINATED  |  WAITING -> READY",
        SS_LEFT | SS_CENTERIMAGE | WS_BORDER, IdStateFlow);

    app.buttons[4] = makeControl(L"BUTTON", L"Admit", BS_PUSHBUTTON, IdAdmit);
    app.buttons[5] = makeControl(L"BUTTON", L"Switch CPU", BS_PUSHBUTTON, IdSwitchCpu);
    app.buttons[6] = makeControl(L"BUTTON", L"Execute One Step", BS_PUSHBUTTON, IdExecute);
    app.buttons[7] = makeControl(L"BUTTON", L"Request I/O", BS_PUSHBUTTON, IdRequestIo);
    app.buttons[8] = makeControl(L"BUTTON", L"Complete I/O", BS_PUSHBUTTON, IdCompleteIo);
    app.buttons[9] = makeControl(L"BUTTON", L"Terminate", BS_PUSHBUTTON, IdTerminate);

    app.processHeading = makeControl(L"STATIC", L"PROCESS TABLE", SS_LEFT, 0);
    app.pcbHeading = makeControl(L"STATIC", L"PCB INSPECTOR", SS_LEFT, 0);
    app.logHeading = makeControl(L"STATIC", L"EVENT LOG", SS_LEFT, 0);

    app.processList = makeControl(WC_LISTVIEWW, L"",
        LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
        IdProcessList, WS_EX_CLIENTEDGE);
    ListView_SetExtendedListViewStyle(app.processList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    addListColumn(0, L"PID", 55);
    addListColumn(1, L"Name", 145);
    addListColumn(2, L"State", 95);
    addListColumn(3, L"Priority", 65);
    addListColumn(4, L"PC", 75);
    addListColumn(5, L"SP", 75);
    addListColumn(6, L"AX", 70);

    app.pcbView = makeControl(L"EDIT", L"",
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_BORDER,
        IdPcbView, WS_EX_CLIENTEDGE);
    app.eventLog = makeControl(L"EDIT", L"",
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_BORDER,
        IdEventLog, WS_EX_CLIENTEDGE);
    app.status = makeControl(L"STATIC", L"Ready.",
        SS_LEFT | SS_CENTERIMAGE | SS_SUNKEN, IdStatus);

    applyFont(app.title, app.titleFont);
    for (HWND control : {app.subtitle, app.stateFlow, app.processHeading, app.pcbHeading,
                         app.logHeading, app.processList, app.status}) {
        applyFont(control, app.normalFont);
    }
    for (HWND button : app.buttons) applyFont(button, app.normalFont);
    for (int i = 4; i < 10; ++i) EnableWindow(app.buttons[i], FALSE);
    applyFont(app.pcbView, app.monoFont);
    applyFont(app.eventLog, app.monoFont);
    app.projectRoot = findProjectRoot();
}

void layout(int width, int height) {
    const int margin = 14;
    const int gap = 8;
    MoveWindow(app.title, margin, 10, width - 2 * margin, 30, TRUE);
    MoveWindow(app.subtitle, margin, 42, width - 2 * margin, 22, TRUE);

    const int toolbarY = 73;
    const int toolbarWidths[] = {112, 112, 82, 112};
    int x = margin;
    for (int i = 0; i < 4; ++i) {
        MoveWindow(app.buttons[i], x, toolbarY, toolbarWidths[i], 32, TRUE);
        x += toolbarWidths[i] + gap;
    }

    MoveWindow(app.stateFlow, margin, 113, width - 2 * margin, 34, TRUE);

    const int actionY = 158;
    const int actionWidth = (width - 2 * margin - 5 * gap) / 6;
    for (int i = 0; i < 6; ++i) {
        MoveWindow(app.buttons[i + 4], margin + i * (actionWidth + gap),
            actionY, actionWidth, 36, TRUE);
    }

    const int bodyTop = 222;
    const int statusHeight = 25;
    const int logHeight = std::max(125, height / 4);
    const int bodyBottom = height - statusHeight - logHeight - 55;
    const int bodyHeight = std::max(130, bodyBottom - bodyTop);
    const int leftWidth = (width - 3 * margin) * 62 / 100;
    const int rightX = 2 * margin + leftWidth;
    const int rightWidth = width - rightX - margin;

    MoveWindow(app.processHeading, margin, bodyTop - 20, leftWidth, 18, TRUE);
    MoveWindow(app.pcbHeading, rightX, bodyTop - 20, rightWidth, 18, TRUE);
    MoveWindow(app.processList, margin, bodyTop, leftWidth, bodyHeight, TRUE);
    MoveWindow(app.pcbView, rightX, bodyTop, rightWidth, bodyHeight, TRUE);

    const int logY = bodyTop + bodyHeight + 28;
    MoveWindow(app.logHeading, margin, logY - 19, width - 2 * margin, 18, TRUE);
    MoveWindow(app.eventLog, margin, logY, width - 2 * margin,
        std::max(90, height - logY - statusHeight - 5), TRUE);
    MoveWindow(app.status, 0, height - statusHeight, width, statusHeight, TRUE);
}

LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        app.window = window;
        createControls();
        return 0;

    case WM_SIZE:
        if (app.status) layout(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = {980, 650};
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IdLoadSample:
            perform([] { loadSample(); });
            return 0;
        case IdImportCsv:
            perform([] { importCsvDialog(); });
            return 0;
        case IdReset:
            perform([] {
                if (!app.simulator) throw std::runtime_error("Load process data first.");
                app.simulator->reset();
                app.selectedPid = app.simulator->processes().front().pid;
            });
            return 0;
        case IdExportCsv:
            perform([] { exportCsvDialog(); });
            return 0;
        case IdAdmit:
            perform([] { app.simulator->admit(app.selectedPid); });
            return 0;
        case IdSwitchCpu:
            perform([] { app.simulator->switchTo(app.selectedPid); });
            return 0;
        case IdExecute:
            perform([] { app.simulator->executeInstruction(); });
            return 0;
        case IdRequestIo:
            perform([] { app.simulator->requestIo(); });
            return 0;
        case IdCompleteIo:
            perform([] { app.simulator->completeIo(app.selectedPid); });
            return 0;
        case IdTerminate:
            perform([] { app.simulator->terminateRunning(); });
            return 0;
        }
        break;

    case WM_NOTIFY: {
        const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
        if (!app.updating && header->hwndFrom == app.processList && header->code == LVN_ITEMCHANGED) {
            const int row = selectedRow();
            if (row >= 0 && app.simulator && row < static_cast<int>(app.simulator->processes().size())) {
                app.selectedPid = app.simulator->processes()[static_cast<std::size_t>(row)].pid;
                updateUi();
            }
        }
        return 0;
    }

    case WM_DESTROY:
        if (app.normalFont) DeleteObject(app.normalFont);
        if (app.titleFont) DeleteObject(app.titleFont);
        if (app.monoFont) DeleteObject(app.monoFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    app.instance = instance;
    INITCOMMONCONTROLSEX commonControls{sizeof(commonControls), ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&commonControls);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = L"ProcessStatePcbWindow";
    if (!RegisterClassW(&windowClass)) return 1;

    HWND window = CreateWindowExW(0, windowClass.lpszClassName,
        L"Process Management Simulator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1180, 820,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 1;

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    try {
        loadSample();
    } catch (const std::exception& error) {
        showError(error);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

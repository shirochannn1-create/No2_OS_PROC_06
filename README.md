# Process Management Simulator

Ứng dụng desktop dành cho Windows, được viết bằng C++17 và Win32 API. Chương trình minh họa ba khái niệm của hệ điều hành:

1. Các trạng thái của tiến trình (Process States).
2. Khối điều khiển tiến trình (Process Control Block – PCB).
3. Chuyển đổi ngữ cảnh (Context Switching).

Dự án được thiết kế để biên dịch bằng **Visual Studio 2022 Community hoặc Professional**. Dự án không yêu cầu Qt, CMake, MSYS2, MinGW hoặc thư viện giao diện bên thứ ba.

## 1. Chức năng

- Hiển thị năm trạng thái của tiến trình: New, Ready, Running, Waiting và Terminated.
- Đọc thông tin tiến trình từ tệp CSV.
- Hiển thị các tiến trình trong bảng Process Table.
- Xem thông tin PCB của từng tiến trình, bao gồm: PID, trạng thái, độ ưu tiên (Priority), PC, SP, AX, BX, CX, DX, FLAGS và số lệnh đã thực thi.
- Đưa một tiến trình vào trạng thái sẵn sàng: New → Ready.
- Cấp CPU cho một tiến trình đang ở trạng thái Ready.
- Chuyển CPU sang một tiến trình Ready khác. Trình mô phỏng lưu ngữ cảnh CPU của tiến trình đang chạy vào PCB của nó, sau đó khôi phục ngữ cảnh của tiến trình được chọn từ PCB.
- Mô phỏng việc thực thi một lệnh.
- Đưa tiến trình đang chạy vào trạng thái chờ I/O: Running → Waiting.
- Hoàn tất I/O và đưa tiến trình trở lại trạng thái Ready: Waiting → Ready.
- Kết thúc tiến trình đang chạy: Running → Terminated.
- Xuất bảng tiến trình và nhật ký sự kiện thành hai tệp CSV.
- Biên dịch và chạy một project C++ riêng để kiểm thử chương trình.

**Dự án này chủ động không triển khai các thuật toán lập lịch CPU.** Người dùng tự chọn tiến trình được cấp CPU. Trường Priority chỉ được lưu và hiển thị như thông tin của tiến trình, không được dùng để quyết định tiến trình nào sẽ chạy.

## 2. Yêu cầu hệ thống

- Windows 10 hoặc Windows 11, phiên bản 64-bit.
- Visual Studio 2022.
- Workload: **Desktop development with C++**.
- MSVC v143 (VS 2022 C++ x64/x86 build tools).
- Windows 10 SDK hoặc Windows 11 SDK.

Không cần cài Qt hoặc thư viện bên ngoài nào khác.

## 3. Biên dịch ứng dụng

1. Giải nén tệp ZIP của dự án vào một thư mục thông thường, ví dụ: `C:\Projects\ProcessStatePCB`.
2. Mở tệp `ProcessManagementSimulator.sln` bằng Visual Studio 2022.
3. Trên thanh công cụ Visual Studio, chọn `Debug` và `x64`.
4. Chọn **Build → Build Solution** hoặc nhấn `Ctrl + Shift + B`.
5. Trong Solution Explorer, đặt project `ProcessManagementSimulator` làm startup project nếu project này chưa được chọn.
6. Nhấn `F5` để chạy chương trình cùng trình gỡ lỗi (debugger), hoặc nhấn `Ctrl + F5` để chạy mà không bật debugger.

Tệp thực thi được tạo tại:

```text
app\x64\Debug\ProcessManagementSimulator.exe
```

Để tạo bản dùng khi phân phối, chọn `Release | x64` rồi biên dịch lại:

```text
app\x64\Release\ProcessManagementSimulator.exe
```

Sau khi biên dịch, Visual Studio sẽ sao chép tệp `input\sample_processes.csv` vào thư mục `input` nằm cạnh tệp EXE. Nhờ vậy, nút nạp dữ liệu mẫu vẫn hoạt động khi chạy EXE từ thư mục đầu ra.

## 4. Chạy kiểm thử

Solution có project thứ hai tên là `SimulatorTests`.

1. Trong Solution Explorer, nhấn chuột phải vào `SimulatorTests`.
2. Chọn **Set as Startup Project**.
3. Nhấn `Ctrl + F5`.

Chương trình kiểm thử sẽ hiển thị kết quả của từng phép kiểm tra và chỉ trả về mã thoát `0` khi tất cả các kiểm tra đều thành công. Sau khi kiểm thử xong, nhấn chuột phải vào `ProcessManagementSimulator` và đặt project này làm startup project một lần nữa.

Bạn cũng có thể chạy tệp `run_tests.bat` trong **Developer PowerShell for VS 2022**. Script này sử dụng trình biên dịch MSVC có sẵn trong terminal đó.

## 5. Cách sử dụng ứng dụng

### Thanh công cụ

- **Load Sample**: nạp dữ liệu từ `input\sample_processes.csv`.
- **Import CSV**: chọn một tệp CSV khác chứa thông tin tiến trình.
- **Reset**: khôi phục tất cả tiến trình và giá trị PCB về trạng thái ban đầu.
- **Export CSV**: lưu bảng tiến trình hiện tại và nhật ký sự kiện.

### Các thao tác với tiến trình

Chọn một dòng trong Process Table, sau đó sử dụng các nút thao tác:

| Nút | Trạng thái ban đầu được phép | Kết quả |
|---|---|---|
| Admit | New | Chuyển sang Ready |
| Switch CPU | Ready | Tiến trình được chọn chuyển sang Running; tiến trình Running trước đó trở về Ready |
| Execute One Step | Running | Tăng PC và thanh ghi AX trong mô phỏng |
| Request I/O | Running | Chuyển sang Waiting; ngữ cảnh CPU được lưu vào PCB |
| Complete I/O | Waiting | Chuyển sang Ready |
| Terminate | Running | Chuyển sang Terminated; ngữ cảnh CPU cuối cùng được lưu vào PCB |

Nút bị vô hiệu hóa có nghĩa là tiến trình đang chọn không thể thực hiện thao tác tương ứng trong trạng thái hiện tại.

### Ví dụ chuyển đổi ngữ cảnh (Context Switch)

1. Đưa PID 1 và PID 2 vào trạng thái Ready bằng nút **Admit**.
2. Chọn PID 1 rồi nhấn **Switch CPU**.
3. Nhấn **Execute One Step** một hoặc nhiều lần.
4. Chọn PID 2 rồi nhấn **Switch CPU**.
5. Kiểm tra Event Log: cần thấy sự kiện `Context Save` của PID 1 và `Context Restore` của PID 2.
6. Chọn lại PID 1 và chuyển CPU về tiến trình này. PC và các thanh ghi đã lưu của PID 1 cần được khôi phục.

## 6. Định dạng CSV đầu vào

Tệp đầu vào phải có đúng 10 cột theo thứ tự sau:

```csv
pid,name,priority,pc,sp,ax,bx,cx,dx,flags
1,Text Editor,3,100,4096,10,20,30,40,512
2,Web Browser,2,200,8192,50,60,70,80,514
```

Các quy tắc kiểm tra dữ liệu:

- `pid`: số nguyên lớn hơn 0; mỗi PID phải là duy nhất.
- `name`: văn bản không được để trống. Tên đặt trong dấu ngoặc kép có thể chứa dấu phẩy.
- `priority`: số nguyên từ 0 đến 255. Giá trị này chỉ được hiển thị, không quyết định tiến trình nào được cấp CPU.
- `pc` và `sp`: số nguyên không dấu 32-bit.
- `ax`, `bx`, `cx`, `dx` và `flags`: số nguyên từ 0 đến 65535.
- Mỗi bản ghi CSV phải nằm trên một dòng.

Tất cả tiến trình được nạp từ CSV đều bắt đầu ở trạng thái New. Các giá trị thanh ghi CPU ban đầu được sao chép vào PCB của từng tiến trình.

## 7. CSV đầu ra

Khi người dùng xuất tệp `process_results.csv`, chương trình cũng tạo tệp `process_results_trace.csv` trong cùng thư mục.

- **Results CSV**: lưu trạng thái hiện tại, Priority, các giá trị thanh ghi trong PCB/CPU và số lệnh đã thực thi của từng tiến trình.
- **Trace CSV**: lưu thời gian mô phỏng, PID và mọi sự kiện đã ghi nhận, bao gồm thay đổi trạng thái, Context Save và Context Restore.

## 8. Cấu trúc dự án

```text
ProcessStatePCB_English/
|-- ProcessManagementSimulator.sln
|-- ProcessManagementSimulator.vcxproj
|-- README.md
|-- src/
|   |-- Main.cpp          # Giao diện desktop Win32
|   |-- Simulator.h       # Khai báo Process, PCB, Event và Simulator
|   |-- Simulator.cpp     # Logic chuyển trạng thái và Context Switch
|   |-- Csv.h             # Khai báo các chức năng CSV
|   `-- Csv.cpp           # Kiểm tra, nhập và xuất dữ liệu CSV
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

## 9. Phạm vi mô phỏng và giới hạn

Đây là phần mềm mô phỏng phục vụ học tập, không phải nhân hệ điều hành (operating-system kernel) thực sự. Chương trình không tạo tiến trình Windows thật và không thực thi mã máy. Thao tác **Execute One Step** chỉ cập nhật PC và thanh ghi AX theo một quy tắc xác định để người dùng có thể quan sát việc bảo toàn ngữ cảnh khi chuyển đổi tiến trình.

Trình mô phỏng chỉ tăng bộ đếm Context Switch khi chuyển trực tiếp từ một tiến trình Running sang một tiến trình Running khác. Lần cấp CPU đầu tiên khi CPU đang Idle và việc chuyển CPU về trạng thái Idle (do tiến trình chờ I/O hoặc kết thúc) vẫn được ghi trong Event Log nhưng không được tính vào bộ đếm chuyển đổi giữa hai tiến trình.

Năm trạng thái và các chuyển đổi được triển khai:

```text
New -> Ready -> Running -> Terminated
                    |  ^
                    v  |
                 Waiting
                    |
                    v
                  Ready

Running -> Ready  (khi người dùng chuyển CPU sang một tiến trình Ready khác)
```

Priority chỉ là thông tin của tiến trình. Dự án không triển khai FCFS, Round Robin, time quantum, bộ điều phối tự động hoặc bất kỳ chính sách lập lịch nào.

## 10. Khắc phục sự cố

- **Thiếu Platform Toolset v143**: mở Visual Studio Installer, chọn Modify và cài `MSVC v143 - VS 2022 C++ x64/x86 build tools`.
- **Thiếu Windows SDK**: trong cùng màn hình Visual Studio Installer, cài Windows 10 SDK hoặc Windows 11 SDK.
- **Không tìm thấy tệp dữ liệu mẫu**: biên dịch lại project để bước sau khi build sao chép CSV mẫu, hoặc dùng **Import CSV** để chọn tệp thủ công.
- **Ứng dụng mở nhưng không có tiến trình nào**: nhấn **Load Sample** hoặc **Import CSV**.
- **Một nút bị vô hiệu hóa**: chọn tiến trình có trạng thái phù hợp với thao tác đó.

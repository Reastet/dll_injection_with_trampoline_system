// needle.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <string>
using namespace std;



int main(int argv, char* argc[])
{
    string first_arg_type, first_arg, sec_arg_type, sec_arg, mode;
    DWORD process;

    cout << argv << " " << argc << endl;
    if (argv != 5)
    {
        cout << "Maybe, debug mode, enter args " << argv << endl;
        cin >> first_arg_type >> first_arg >> sec_arg_type >> sec_arg;
    }
    else
    {
        first_arg_type = argc[1];
        first_arg = argc[2];
        sec_arg_type = argc[3];
        sec_arg=argc[4];
        cout << first_arg_type << endl;
    }
    if (first_arg_type != "-pid" || first_arg_type != "-name" || sec_arg_type != "-hide" || sec_arg_type != "-func")
    {
        if (first_arg_type == "-pid")
        {
            process = stoi(first_arg);
        }
        else
        {
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            PROCESSENTRY32W entry;
            entry.dwSize = sizeof(entry);
            Process32FirstW(snap, &entry);
            wstring process_name;
            for(int i = 0; i < first_arg.size(); i++)
            {
                process_name += first_arg[i];
            }
            // Перебираем все процессы
            do {
                if (wstring(entry.szExeFile) == process_name) {
                    process = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &entry));
        }

        if (sec_arg_type == "-hide")
        {
            mode = "hide";
        }
        else
        {
            mode = "monitor";
        }
    }

    SECURITY_ATTRIBUTES saAttr = { 0 };
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    HANDLE pipe_initializer = CreateNamedPipeA(
        "\\\\.\\pipe\\injector_init_pipe",    // имя пайпа
        PIPE_ACCESS_DUPLEX,              // только чтение
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        0,
        0,
        0,
        &saAttr
    );

    string unique_pipe_name = "\\\\.\\pipe\\unique_injector_pipe";

    HANDLE unique_pipe = CreateNamedPipeA(
        unique_pipe_name.c_str(),    // имя пайпа
        PIPE_ACCESS_DUPLEX,              // только чтение
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        0,
        0,
        0,
        &saAttr
    );

    if ((unique_pipe == INVALID_HANDLE_VALUE) || pipe_initializer == INVALID_HANDLE_VALUE)
    {
        cout << "Pipe init error" << endl;
        return -1;
    }

   

    cout  << process << endl;
    cout << "insert" << endl;
    HANDLE ins_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process);
    string path = "C:\\...\\new_dll_injetcor.dll";//Путь до dll 
    LPVOID addr =  VirtualAllocEx(ins_proc, 0, path.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    WriteProcessMemory(ins_proc, addr, path.c_str(), path.length() + 1, NULL);
    HMODULE data = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)GetProcAddress(data, "LoadLibraryA");
    LPCSTR send_val = "C:\\...\\new_dll_injetcor.dll";//Путь до dll
    
    LPDWORD id=0; 
    CreateRemoteThread(ins_proc, NULL, 0, func, (LPVOID)addr, 0, id);
    cout << "Insert complered" << endl;
    while (true)
    {
        if (ConnectNamedPipe(pipe_initializer, NULL)) {
            break;
        }
    }
    DWORD pipe_bytes;
    WriteFile(pipe_initializer, unique_pipe_name.c_str(), unique_pipe_name.size()+1, &pipe_bytes, NULL);
    DisconnectNamedPipe(pipe_initializer);

    while (true)
    {
        if (ConnectNamedPipe(unique_pipe, NULL)) {
            break;
        }
    }
    WriteFile(unique_pipe, mode.c_str(), mode.size()+1, &pipe_bytes, NULL);
    char buffer[300];
    ReadFile(unique_pipe, buffer, 300, &pipe_bytes, NULL);
    cout << "test" << " " << buffer << endl;
    Sleep(1000);
    WriteFile(unique_pipe, sec_arg.c_str(), sec_arg.size() + 1, &pipe_bytes, NULL);
    
    while (true)
    {
        for (int i = 0; i < 300; i++)
        {
            buffer[i] = '\0';
        }
        ReadFile(unique_pipe, buffer, 300, &pipe_bytes, NULL);
        string test = buffer;
        if (pipe_bytes != 0)
        {
            cout << test << endl;
        }
        if (test == "All hooks are uninstalled. Deinit pipe")
        {
            break;
        }
    }

}


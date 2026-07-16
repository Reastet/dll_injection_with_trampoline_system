
#include "pch.h"
#include "dll_injector.h"
#include "stdio.h"
static dll_injector* element;
void init()
{
	element = new dll_injector;
}
extern "C" void* message()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
	string cur_time = string(buffer);
	
	element->send_message(string("Function ") + element->get_file_name() + string(" started at ")+cur_time);
	return element->hooked_logging_func;
	//return NULL;
}


HANDLE WINAPI myCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{

	element->send_message("myCreateFileA working");
	if (dwDesiredAccess == GENERIC_READ)
	{
		if (lpFileName == element->get_file_name())
		{
			return INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		return ((HANDLE(*)(LPCSTR, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE))(element->hooked_CreateFile))(lpFileName,
			dwDesiredAccess, lpSecurityAttributes,
			dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
}

HANDLE WINAPI myFindFirstFileA(LPCSTR filename, WIN32_FIND_DATAA data)
{
	HANDLE t = ((HANDLE(*)(LPCSTR, WIN32_FIND_DATAA))(element->hooked_FindFirstFile))(filename, data);;
	
	char* check = new char[300];
	GetFullPathNameA(data.cFileName, 300, check, 0);
	element->send_message(string("myFindFirstFileA working. FULL PATH NAME: ") + string(check));
	if (filename == element->get_file_name())
	{
		return INVALID_HANDLE_VALUE;
	}
	
	else
	{
		return t;
	}
}

HANDLE WINAPI myFindNextFileA(HANDLE file_desc, WIN32_FIND_DATAA data)
{
	char* check = new char[300];
	GetFullPathNameA(data.cFileName, 300, check, 0);
	element->send_message(string("myFindNextFileA working. FULL PATH NAME: ") + string(check));
	if (check == element->get_file_name())
	{
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		return ((HANDLE(*)(HANDLE, WIN32_FIND_DATAA))(element->hooked_FindNextFile))(file_desc, data);
	}
}



dll_injector::dll_injector()
{
	int res = this->init_pipe("\\\\.\\pipe\\injector_init_pipe");
	if (res != 1)
	{
		return;
	}
	string work_pipe = this->get_message();
	this->deinit_pipe();
	res = this->init_pipe(work_pipe);
	if (res != 1)
	{
		return;
	}
	string getted_message = this->get_message();

	if (getted_message == "monitor")
	{
		this->send_message(string("monitor") + string("It Is Control Message Enter func name"));
		string func = this->get_message();
		char* tmp = new char[30];
		this->hooked_logging_func = this->set_up_hook(func, context, tmp);
		if (this->hooked_logging_func != NULL)
		{
			this->set_hidden_file_name(func);
			this->old_calls.push_back(tmp);
			this->hooked_funcs.push_back(func);
			this->send_message("It Is Control Message Set up hooks completed\n");
			this->set_hidden_file_name(func);
			return;
		}
		else
		{
			this->send_message("Error while setting up hooks\n");
			delete[] tmp;
		}
	}
	else if (getted_message == "hide")
	{
		this->send_message("It Is Control Message Enter file name");
		this->set_hidden_file_name(this->get_message());
		char* tmp;
		for (int i = 0; i < 3; i++)
		{
			tmp = new char[30];
			this->old_calls.push_back(tmp);
		}
		this->hooked_CreateFile = this->set_up_hook("CreateFileW", myCreateFileA, this->old_calls[0]);
		this->hooked_FindFirstFile = this->set_up_hook("FindFirstFileW", myFindFirstFileA, this->old_calls[1]);
		this->hooked_FindNextFile = this->set_up_hook("FindNextFileW", myFindNextFileA, this->old_calls[2]);
		this->hooked_funcs.push_back("CreateFileW");
		this->hooked_funcs.push_back("FindFirstFileW");
		this->hooked_funcs.push_back("FindNextFileW");
		if (this->hooked_CreateFile && this->hooked_FindFirstFile && this->hooked_FindNextFile)
		{
			this->send_message("It Is Control Message Set up hooks completed\n");
			return;
		}
		else
		{
			this->send_message(string("Error while setting up hooks"));
			for (int i = 0; i < 3; i++)
			{
				tmp = this->old_calls[i];
				delete[] tmp;
				this->old_calls.pop_back();
				this->hooked_funcs.pop_back();
			}
		}
	}
	else
	{
		this->send_message("INCORRECT_COMMAND!!!!");
		this->deinit_pipe();
	}
}
//Инициализация pipe по имени. Вернет 0, если не ок, 1, если ок
int dll_injector::init_pipe(string pipe_name)
{
	
	HANDLE pipe_serv = CreateFileA(
		pipe_name.c_str(),    // имя пайпа
		GENERIC_READ | GENERIC_WRITE,      // однонаправленный пайп
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL                      // безопасность
	);

	if (pipe_serv == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	this->pipe = pipe_serv;
	return 1;
}
//Удаляет пайп
void dll_injector::deinit_pipe()
{
	CloseHandle(this->pipe);
}

HANDLE dll_injector::getter_pipe()
{
	return this->pipe;
}
//Отправляет сообщение data через пайп
void dll_injector::send_message(string data)
{
	DWORD bytes;

	WriteFile(this->pipe, data.c_str(), data.size()+1, &bytes, NULL);
}
//Получение сообщения через пайп
string dll_injector::get_message()
{
	DWORD bytes;
	char buffer[300];
	ReadFile(this->pipe, buffer, 300, &bytes, 0);
	auto res = string(buffer);
	return res;
}

void dll_injector::set_hidden_file_name(string name)
{
	this->hiden_file = name;
}

string dll_injector::get_file_name()
{
	return string(this->hiden_file);
}

void dll_injector::uninstall_all_hooks()
{
	for (int i = 0; i < this->old_calls.size(); i++)
	{
		this->uninstall_hook(this->hooked_funcs[i], this->old_calls[i]);
	}
	while (this->old_calls.size() > 0)
	{
		char* tmp;
		tmp = this->old_calls[this->old_calls.size()-1];
		delete[] tmp;
		this->old_calls.pop_back();
		this->hooked_funcs.pop_back();
	}
	this->send_message("All hooks are uninstalled. Deinit pipe");
	this->deinit_pipe();
}

void dll_injector::uninstall_hook(string function_name, char buffer[30])
{
	void* addr;
	auto process = GetCurrentProcessId();
	HMODULE lib = GetModuleHandleA("kernel32.dll");
	addr = GetProcAddress(lib, function_name.c_str());
	OpenProcess(PROCESS_ALL_ACCESS, FALSE, process);
	DWORD TMP;
	VirtualProtect(addr, 32, PAGE_READWRITE, &TMP);
	memcpy(addr, buffer, 30);
	DWORD TMP_2;
	VirtualProtect(addr, 32, TMP, &TMP_2);
}

/*
Функция установки хука
Аргументы:
string function_name-имя функции, которую хукаем
void* new_func-функция, на которую надо прыгнуть
Возвращаемые значения:
NULL-если в ходе работы где-то произошла ошибка
FARPROC-адрес функции в Kernelbase, которую надо вызвать либо на которую надо сделать jmp
*/
FARPROC dll_injector::set_up_hook(string function_name, void* new_func, char buffer[30])
{
	//element = this;
	void* addr;
	auto process = GetCurrentProcessId();
	HMODULE lib = GetModuleHandleA("kernel32.dll");
	addr = GetProcAddress(lib, function_name.c_str());
	if (addr == NULL)
	{
		return NULL;
	}
	HMODULE deep_lib = GetModuleHandleA("KERNELBASE.dll");
	if (deep_lib == NULL)
	{
		return NULL;
	}

	memcpy(buffer, addr, 8);
	unsigned int a = (int)*(buffer);
	OpenProcess(PROCESS_ALL_ACCESS, FALSE, process);
	DWORD TMP;
	VirtualProtect(addr, 32, PAGE_READWRITE, &TMP);
	char new_data[30] = { 0 };
	FARPROC test = (FARPROC) new_func;
	//push rbx
	new_data[0] = 0x53;
	//mov eax, старшие биты
	new_data[0] = 0xb8;
	memcpy(new_data + 1, ((char*)&test + 4), 4);	
	//shl rax, 0x20
	new_data[5] = 0x48;
	new_data[6] = 0xc1;
	new_data[7] = 0xe0;
	new_data[8] = 0x20;
	//push rbx
	new_data[9] = 0x53;
	//mov rbx, младшие биты
	new_data[10] = 0xbb;
	memcpy(new_data + 11, &test, 4);
	//add rax, rbx
	new_data[15] = 0x48;
	new_data[16] = 0x01;
	new_data[17] = 0xd8;
	//pop rbx
	new_data[18] = 0x5b;
	// dop push rax
	//new_data[19] = 0x50;
	//jmp rax
	new_data[19] = 0xff;
	new_data[20] = 0xe0;
	//memcpy(new_data+19, old_jmp_call, 8);
	DWORD TMP_2;
	memcpy(addr, new_data, 30);
	VirtualProtect(addr, 32, TMP, &TMP_2);
	
	return GetProcAddress(deep_lib, function_name.c_str());
}

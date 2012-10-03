#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>

bool RunNappledProcess(wchar_t* apppath, wchar_t* cmdline, wchar_t* dir) {
    STARTUPINFO startupinfo = { sizeof(STARTUPINFO), 0, };
    PROCESS_INFORMATION processinfo = { 0, };

    BOOL created = ::CreateProcessW(apppath, cmdline, nullptr, nullptr,
    FALSE, CREATE_SUSPENDED, nullptr, dir, &startupinfo, &processinfo);
    if (created == FALSE) {
        ::MessageBoxW(nullptr, L"failed to create process", nullptr, 0);
        return false;
    }

    DWORD len_dllpath = ::GetCurrentDirectory(0, nullptr);
    wchar_t dllname[] = NADLL;
    size_t len_dllname = sizeof(dllname) / sizeof(wchar_t);
    wchar_t* dllpath = new wchar_t[len_dllpath + len_dllname+1];
    ::GetCurrentDirectory(len_dllpath, dllpath);
    dllpath[len_dllpath-1] = L'\\';
    wcscpy(dllpath+len_dllpath, NADLL);

    int memsize = sizeof(wchar_t) * (len_dllpath+11);
    void* remote_mem = ::VirtualAllocEx(processinfo.hProcess, nullptr,
            memsize, MEM_COMMIT, PAGE_READWRITE);
    SIZE_T writtensize = 0;
    BOOL written = TRUE;
    written = ::WriteProcessMemory(processinfo.hProcess, remote_mem,
            dllpath, memsize, &writtensize);
    delete[] dllpath;
    if (written == FALSE) {
        ::MessageBoxW(nullptr, L"failed to write procmem", nullptr, 0);
        return false;
    }

    HANDLE remote_thread = ::CreateRemoteThread(processinfo.hProcess, nullptr,
            0, (LPTHREAD_START_ROUTINE)::GetProcAddress(
            ::GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"),
            remote_mem, 0, nullptr);

    if (remote_thread == INVALID_HANDLE_VALUE) {
        ::MessageBoxW(nullptr, L"invalid thread", nullptr, 0);
        return false;
    }

    ::WaitForSingleObject(remote_thread, INFINITE);
    ::VirtualFreeEx(processinfo.hProcess, remote_mem, memsize, MEM_RELEASE);

    ::ResumeThread(processinfo.hThread);
    ::WaitForSingleObject(processinfo.hThread, INFINITE);

    return false;
}

extern "C"
int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prev_instance,
        char* cmdline_, int cmdshow) {
    (void)instance;
    (void)prev_instance;
    (void)cmdline_;
    (void)cmdshow;

    int argc = 0;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv == nullptr || argc != 2) {
        ::MessageBox(nullptr, L"usage: <nappl> <program_path>", NULL, 0);
        return 0;
    }

    wchar_t* cmdline = argv[1];
    wchar_t* appname = cmdline; // TODO
    size_t len = ::GetFullPathNameW(appname, 0, nullptr, nullptr);
    wchar_t* path = new wchar_t[len];
    wchar_t* filename = nullptr;
    DWORD r = ::GetFullPathNameW(appname, len, path, &filename);
    wchar_t* dir = new wchar_t[len];
    wcscpy(dir, path);
    dir[filename - path] = L'\0';
    RunNappledProcess(path, cmdline, dir);
    delete[] path;
    delete[] dir;
    ::LocalFree(argv);
    return 0;
}

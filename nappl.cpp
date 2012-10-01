#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>

bool RunNappledProcess(wchar_t* path, wchar_t* cmdline) {
    STARTUPINFO startupinfo = { sizeof(STARTUPINFO), 0, };
    PROCESS_INFORMATION processinfo = { 0, };

    BOOL created = ::CreateProcessW(path, cmdline, nullptr, nullptr,
    FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startupinfo, &processinfo);
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

    HANDLE remote_thread = ::CreateRemoteThread(processinfo.hProcess, nullptr, 0,
            (LPTHREAD_START_ROUTINE)::GetProcAddress(
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
        char* cmdline, int cmdshow) {
    (void)instance;
    (void)prev_instance;
    (void)cmdline;
    (void)cmdshow;

    int argc = 0;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argv == nullptr || argc != 2) {
        ::MessageBox(nullptr, L"usage: <nappl> <program_path>", NULL, 0);
        return 0;
    }

    RunNappledProcess(argv[1], argv[1]);
    ::LocalFree(argv);
    return 0;
}

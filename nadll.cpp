#define WIN32_LEAN_AND_MEAN
#include <windows.h>

unsigned int orig_cp = -1;
unsigned int new_cp = 932;

bool infected = false;

inline int m2wl(const char* msg, int len=-1) {
    int new_len = ::MultiByteToWideChar(new_cp, 0, msg, len, NULL, 0);
    return new_len;
}

inline wchar_t* m2w(const char* msg, int new_len=0, int len=-1) {
    if (new_len == 0) {
        new_len = m2wl(msg, len);
    }
    wchar_t* new_msg = new wchar_t[new_len];
    ::MultiByteToWideChar(new_cp, 0, msg, len, new_msg, new_len);
    return new_msg;
}

UINT WINAPI NewGetACP() {
    return new_cp;
}

UINT WINAPI NewGetOEMCP() {
    return new_cp;
}

int WINAPI NewMessageBoxA(HWND hwnd, const char* text, const char* caption,
        UINT type) {
    wchar_t* new_text = nullptr;
    wchar_t* new_caption = nullptr;
    if (text) {
        new_text = m2w(text);
    }
    if (caption) {
        new_caption = m2w(caption);
    }

    int ret = ::MessageBoxW(hwnd, new_text, new_caption, type);

    if (new_text) {
        delete[] new_text;
    }
    if (new_caption) {
        delete[] new_caption;
    }
    return ret;
}

BOOL WINAPI NewTextOutA(HDC hdc, int x, int y, const char* string, int size) {
    int len_string = m2wl(string, size);
    wchar_t* new_string = m2w(string, len_string, size);

    int ret = ::TextOutW(hdc, x, y, new_string, len_string);

    delete[] new_string;
    return ret;
}

HWND WINAPI NewCreateWindowExA(DWORD extrastyle, const char* classname,
        const char* windowname, DWORD style, int x, int y, int width,
        int height, HWND parent, HMENU menu, HINSTANCE instance,
        LPVOID param) {
    wchar_t* new_classname = new wchar_t[100];
    ::MultiByteToWideChar(orig_cp, 0, classname, -1, new_classname, 100);
    wchar_t* new_windowname = m2w(windowname);

    HWND ret = ::CreateWindowExW(extrastyle, new_classname, new_windowname,
            style, x, y, width, height, parent, menu, instance, param);

    delete[] new_classname;
    delete[] new_windowname;

    return ret;
}

int WINAPI NewGetWindowTextA(HWND wnd, char* string, int count) {
    int new_count = count*2; // XXX
    wchar_t* new_string = new wchar_t[new_count];

    int ret = ::GetWindowTextW(wnd, new_string, new_count);

    ::WideCharToMultiByte(new_cp, 0, new_string, new_count, string, count,
            NULL, NULL);

    delete[] new_string;
    return ret;
}

BOOL WINAPI NewSetWindowTextA(HWND wnd, const char* string) {
    wchar_t* new_string = m2w(string);

    BOOL ret = ::SetWindowTextW(wnd, new_string);

    delete[] new_string;
    return ret;
}

INT_PTR WINAPI NewDialogBoxParamA(HINSTANCE instance, const char* templatename,
        HWND parent, DLGPROC dialog_func, LPARAM param) {
    wchar_t* new_templatename = m2w(templatename);

    INT_PTR ret = ::DialogBoxParamW(instance, new_templatename, parent,
            dialog_func, param);
    
    delete[] new_templatename;
    return ret;
}

UINT WINAPI NewGetDlgItemTextA(HWND dlg, int item, char* string, int count) {
    int new_count = count*2; // XXX
    wchar_t* new_string = new wchar_t[new_count];
    UINT ret = ::GetDlgItemTextW(dlg, item, new_string, new_count);

    ::WideCharToMultiByte(new_cp, 0, new_string, new_count, string, count,
            NULL, NULL);

    delete[] new_string;
    return ret;
}

UINT WINAPI NewSetDlgItemTextA(HWND dlg, int item, char* string) {
    wchar_t* new_string = m2w(string);

    UINT ret = ::SetDlgItemTextW(dlg, item, new_string);

    delete[] new_string;
    return ret;
}

struct api_thunk {
    const wchar_t* dllname;
    const char* funcname;
    void* orig_func;
    void* new_func;
};

#define X(dll, funcname) { L ## #dll L".dll", #funcname, \
(void*)funcname, (void*)New##funcname }

struct api_thunk apis[] = {
    X(kernel32, GetACP),
    X(kernel32, GetOEMCP),

    X(gdi32, TextOutA),

    X(user32, MessageBoxA),
    X(user32, CreateWindowExA),
    X(user32, GetWindowTextA),
    X(user32, SetWindowTextA),
    X(user32, DialogBoxParamA),
    X(user32, GetDlgItemTextA),
    X(user32, SetDlgItemTextA),
};

#undef X

void modify_api(const struct api_thunk& thunk) {
    FARPROC addr = ::GetProcAddress(GetModuleHandleW(thunk.dllname),
            thunk.funcname);

    unsigned char* func = (unsigned char*)addr;
    DWORD daddr = (DWORD)thunk.new_func;

    DWORD old_protection;
    SIZE_T size = 7;
    ::VirtualProtect((void*)addr, size, PAGE_EXECUTE_READWRITE, &old_protection);
    func[0] = 0xb8; // eax <- daddr
    func[1] = daddr & 0xff;
    func[2] = (daddr >> 8) & 0xff;
    func[3] = (daddr >> 16) & 0xff;
    func[4] = (daddr >> 24) & 0xff;
    func[5] = 0xff; // jmp eax
    func[6] = 0xe0;

    VirtualProtect((void*)addr, size, old_protection, NULL);
}

template<typename T, size_t N>
inline size_t arraysize(T(&)[N]) {
    return N;
}

extern "C"
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    (void)reserved;

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            if (infected) {
                break;
            }
            ::DisableThreadLibraryCalls(module);

            orig_cp = ::GetACP();

            for (size_t i = 0; i < arraysize(apis); i++) {
                modify_api(apis[i]);
            }
            infected = true;
        }
        break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        {
            break;
        }
    }
    return TRUE;
}

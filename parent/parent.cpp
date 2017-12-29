/* Copyright 2017 yukkun007 */
#include <windows.h>
#include <tchar.h>
#include <iostream>

HANDLE read_pipe = nullptr;
HANDLE write_pipe = nullptr;
HANDLE child_process = nullptr;

bool MyCloseHandle(HANDLE handle) {
    if (handle != nullptr && !CloseHandle(handle)) {
        std::wcerr << _T("CloseHandle failed.") << std::endl;
        return false;
    }
    return true;
}

void CloseAllHandle(void) {
    MyCloseHandle(child_process);
    MyCloseHandle(read_pipe);
    MyCloseHandle(write_pipe);
}

bool MyCreatePipe(void) {
    SECURITY_ATTRIBUTES security_attributes = {};
    security_attributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
    security_attributes.lpSecurityDescriptor = nullptr;
    security_attributes.bInheritHandle       = TRUE;  // inheritable

    // both pipes can inherit to child process
    HANDLE temp_write_pipe;
    if (!CreatePipe(&read_pipe, &temp_write_pipe, &security_attributes, 0)) {
        std::wcerr << _T("last error=") << GetLastError() << std::endl;
        std::wcerr << _T("CreatePipe failed.") << std::endl;
        return false;
    }

    // create uninheritable write pipe
    if (!DuplicateHandle(
        GetCurrentProcess(), temp_write_pipe,
        GetCurrentProcess(), &write_pipe,
        0, FALSE, DUPLICATE_SAME_ACCESS)
        ) {
        std::wcerr << _T("DuplicateHandle failed.") << std::endl;
        MyCloseHandle(temp_write_pipe);
        return false;
    }

    if (!MyCloseHandle(temp_write_pipe)) {
        return false;
    }

    return true;
}

bool Execute(LPSTR command_line) {
    STARTUPINFOA startup_info = {};
    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    // standard input of child process is read pipe
    startup_info.hStdInput = read_pipe;
    // inherit standard output of parent process
    startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    if (startup_info.hStdOutput == INVALID_HANDLE_VALUE) {
        std::wcerr << _T("GetStdHandle(STD_OUTPUT_HANDLE)") << std::endl;
        return false;
    }
    if (startup_info.hStdError == INVALID_HANDLE_VALUE) {
        std::wcerr << _T("GetStdHandle(STD_ERROR_HANDLE)") << std::endl;
        return false;
    }

    // create child process
    BOOL  bInheritHandles = TRUE;
    DWORD creationFlags = 0;
    PROCESS_INFORMATION process_information = {};
    // use ansi version
    // "The Unicode version of this function, CreateProcessW, can modify
    //  the contents of this string. Therefore, this parameter cannot be a
    //  pointer to read-only memory (such as a const variable or a literal
    //  string). If this parameter is a constant string, the function may
    //  cause an access violation."
    if (!CreateProcessA(
        nullptr,
        command_line,
        nullptr,  // security attributes of process
        nullptr,  // security attirbutes of thread
        bInheritHandles,
        creationFlags,
        nullptr,  // inherit environment value
        nullptr,  // current directory is same
        &startup_info,
        &process_information)
        ) {
        std::wcerr << _T("CreateProcess failed.") << std::endl;
        return false;
    }
    child_process = process_information.hProcess;
    MyCloseHandle(process_information.hThread);

    // because already don't needed
    MyCloseHandle(read_pipe);
    read_pipe = nullptr;

    // send message to child process
    const char message[] = "test message";
    DWORD number_of_bytes_written;
    if (!WriteFile(write_pipe, message, (DWORD)strlen(message),
        &number_of_bytes_written, nullptr)) {
        std::wcerr << _T("WriteFile failed.")<< std::endl;
        return false;
    }
    if (!MyCloseHandle(write_pipe)) {
        return false;
    }
    write_pipe = nullptr;

    // wait for child process to finish
    DWORD return_code = WaitForSingleObject(child_process, INFINITE);
    switch (return_code) {
    case WAIT_FAILED:
        std::wcerr << _T("WaitForSingleObject failed. resutl=WAIT_FAILED")
            << std::endl;
        return false;
    case WAIT_ABANDONED:
        std::wcerr << _T("WaitForSingleObject failed. resutl=WAIT_ABANDONED")
            << std::endl;
        return false;
    case WAIT_OBJECT_0:
        // successful termination
        std::wcout << _T("WaitForSingleObject returned. resutl=WAIT_OBJECT_0")
            << std::endl;
        break;
    case WAIT_TIMEOUT:
        std::wcerr << _T("WaitForSingleObject failed. resutl=WAIT_TIMEOUT")
            << std::endl;
        return false;
    default:
        std::wcerr << _T("WaitForSingleObject failed. resutl=") << return_code
            << std::endl;
        return false;
    }

    return 0;
}

int main() {
    std::wcout << _T("parent start") << std::endl;

    bool result = MyCreatePipe();
    if (result) {
        LPSTR command_line = "child.exe";
        result = Execute(command_line);
    }
    CloseAllHandle();

    std::wcout << _T("parent end");

    return result;
}

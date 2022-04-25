#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <TlHelp32.h>

BOOL InjectDLL(DWORD dwProcessId, LPSTR lpPath)
{
    BOOL status = FALSE;
    DWORD dwPathSize = (DWORD)strlen(lpPath) + 1;
    // The Resources need Release
    HANDLE hTargetProcess = NULL;
    HANDLE hRemoteThread = NULL;
    LPVOID lpRemoteAddress = NULL;

    // OpenProcess
    hTargetProcess = OpenProcess(
        PROCESS_ALL_ACCESS,
        FALSE,
        dwProcessId);

    if (hTargetProcess == NULL)
        goto END;

    // VirtualAllocEx
    lpRemoteAddress = VirtualAllocEx(
        hTargetProcess,
        NULL,
        dwPathSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);

    if (lpRemoteAddress == NULL)
        goto END;

    // WriteProcessMemory
    SIZE_T numberOfBytesWritten = 0;
    WriteProcessMemory(
        hTargetProcess,
        lpRemoteAddress,
        lpPath,
        dwPathSize,
        &numberOfBytesWritten);

    if (numberOfBytesWritten != dwPathSize)
        goto END;

    // CreateRemoteThread
    hRemoteThread = CreateRemoteThread(
        hTargetProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA,
        lpRemoteAddress,
        0,
        NULL);

    if (hRemoteThread == NULL)
        goto END;

    WaitForSingleObject(hRemoteThread, INFINITE);

    // Inject DLL Successful
    status = TRUE;

END: // Release Resources & return status

    if (hRemoteThread != NULL)
        CloseHandle(hRemoteThread);
    if (lpRemoteAddress != NULL)
        VirtualFreeEx(hTargetProcess, lpRemoteAddress, 0, MEM_RELEASE);
    if (hTargetProcess != NULL)
        CloseHandle(hTargetProcess);

    return status;
}

int main()
{
    char path[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, path);
    strcat(path, "\\ExileHelp2.dll");

    // 遍历进程
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
    Process32First(hProcessSnap, &pe32);
    do
    {
        if (!strcmp(pe32.szExeFile, "PathOfExile.exe"))
        {
            InjectDLL(pe32.th32ProcessID, path);
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);

    return 0;
}

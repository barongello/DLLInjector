#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

DWORD findProcessByName(char *process_name)
{
  vector<DWORD> pids = {};
  PROCESSENTRY32 entry;

  entry.dwSize = sizeof(PROCESSENTRY32);

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (snapshot == INVALID_HANDLE_VALUE)
  {
    return 0;
  }

  if (Process32First(snapshot, &entry) == TRUE)
  {
    do
    {
      if (stricmp(entry.szExeFile, process_name) != 0)
      {
        continue;
      }

      pids.push_back(entry.th32ProcessID);
    }
    while (Process32Next(snapshot, &entry) == TRUE);
  }

  CloseHandle(snapshot);

  if (pids.size() == 0)
  {
    return 0;
  }

  if (pids.size() == 1)
  {
    return pids.at(0);
  }

  string input;
  size_t option;

  while (true)
  {
    for(int i = 0; i < pids.size(); ++i)
    {
      cout << i << ". " << pids.at(i) << " (" << process_name << ")" << endl;
    }

    cout << "-1. Exit" << endl;
    cout << endl;
    cout << "Choose an option: ";

    getline(cin, input);

    try
    {
      option = stoi(input);
    }
    catch (...)
    {
      continue;
    }

    if (option == -1)
    {
      return 0;
    }

    if (option < 0 || option >= pids.size())
    {
      continue;
    }

    break;
  }

  return pids.at(option);
}

void usage(void)
{
  char filename[MAX_PATH] = { 0 };

  GetModuleFileNameA(NULL, filename, MAX_PATH);

  cout << "Usage:" << endl;
  cout << "\t" << filename << " <PID | process name> <DLL path>" << endl;
}

int main(int argc, char *argv[])
{
  int result = 0;

  DWORD pid = 0;
  HANDLE hProcess = NULL;
  LPVOID pAddress = NULL;
  HANDLE hThread = NULL;

  if (argc != 3)
  {
    usage();

    result = 1;

    goto defer;
  }

  pid = atoi(argv[1]);

  if (pid == 0)
  {
    pid = findProcessByName(argv[1]);

    if (pid == 0)
    {
      cout << "PID can not be found" << endl;

      result = 2;

      goto defer;
    }
  }

  cout << "PID: " << pid << endl;

  char dll_path[MAX_PATH];

  GetFullPathName(argv[2], MAX_PATH, dll_path, NULL);

  size_t dll_path_size = strlen(dll_path);

  cout << "DLL path: " << dll_path << " (" << dll_path_size << " bytes)" << endl;

  hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

  if (hProcess == NULL)
  {
    cout << "Could not open process with PID " << pid;

    result = 3;

    goto defer;
  }

  cout << "Process: " << hProcess << endl;

  pAddress = VirtualAllocEx(hProcess, 0, dll_path_size + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

  if (pAddress == NULL)
  {
    cout << "Could not allocate memory in the process" << endl;

    result = 4;

    goto defer;
  }

  cout << "Address: " << pAddress << endl;

  SIZE_T nBytesWritten;
  BOOL success;

  success = WriteProcessMemory(hProcess, pAddress, dll_path, dll_path_size, &nBytesWritten);

  if (!success || nBytesWritten != dll_path_size)
  {
    cout << "Could not write to process memory" << endl;

    result = 5;

    goto defer;
  }

  cout << "Bytes written: " << nBytesWritten << endl;

  hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pAddress, 0, NULL);

  if (hThread == NULL)
  {
    cout << "Could not start remote thread" << endl;

    result = 6;

    goto defer;
  }

  cout << "Thread: " << hThread << endl;

  defer:
    if(hThread != NULL)
    {
      CloseHandle(hThread);
    }

    if (pAddress != NULL)
    {
      VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
    }

    if (hProcess != NULL)
    {
      CloseHandle(hProcess);
    }

  return result;
}


#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>

using namespace std;

void displhelp(CONSOLE_SCREEN_BUFFER_INFO info, bool docolour) {
	if(docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
	cout << "---------------------------------------------------------------\n";
	cout << "------------------------- WaitFor.exe -------------------------\n";
	cout << "---------------------------------------------------------------\n\n";
	cout << "A simple program for hiding a program in memory for a specified amount of time.\n\n";
	cout << "Usage: WaitFor [PID] [MILLISECONDS]\n";
	if(docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), info.wAttributes);
}

int lencheck(char* arg, int num, bool docolour) {
	string test = arg;
	bool err = 0;
	unsigned long long testl = 0;
	//check if it is a number
	for (int i = 0; i < test.length(); i++) {
		if (!isdigit(test[i]) && test[i] != '-') {
			if (docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			printf("\n[!] Argument %d is not a number\n\n", num);
			return 1;
		}
	}
	//check if negative
	if (test.length() > 0) {
		if (test[0] == '-') {
			if (docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
			printf("\n[!] Argument %d is negative\n\n", num);
			return 1;
		}
	}
	//check size safely
	if (test.length() > 10) {
		err = 1;
	}
	else {
		unsigned long long testl = stoull(arg);
		if (testl > 4294967295) err = 1;
	}
	if (err) {
		if(docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		printf("\n[!] Argument %d is too large\n\n", num);
		return 1;
	}
	return 0;
}

vector<DWORD> ListProcessThreads(DWORD dwOwnerPID)
{
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
		return vector<DWORD>(0);
	te32.dwSize = sizeof(THREADENTRY32);
	if (!Thread32First(hThreadSnap, &te32))
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		cout << "[!] Error in Thread32First";
		CloseHandle(hThreadSnap);
		return vector<DWORD>(0);
	}
	vector<DWORD> tidvec;
	printf("\n[+] Enumerating Threads: [");
	do
	{
		if (te32.th32OwnerProcessID == dwOwnerPID)
		{
			tidvec.push_back(te32.th32ThreadID);
			printf("|");
		}
	} while (Thread32Next(hThreadSnap, &te32));
	printf("]\n");
	CloseHandle(hThreadSnap);
	return tidvec;
}


int __cdecl main(int argc, char** argv)
{
	//Get the console colours to be reset if there is an error
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE stdhout = GetStdHandle(STD_OUTPUT_HANDLE);
	bool docolour = 0;
	if (!GetConsoleScreenBufferInfo(stdhout, &info)) {
		printf("Unable to get console colour; running without colours\n");
	}
	else {
		docolour = 1;
	}
	//check number of args
	if (argc < 3) {
		if(docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		cout << "\n[!] Too few arguments\n\n";
		displhelp(info, docolour);
		return 1;
	}
	if (argc > 3) {
		if(docolour) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		cout << "\n[!] Too many arguments\n\n";
		displhelp(info, docolour);
		return 1;
	}
	//check length of args
	if (lencheck(argv[1], 1, docolour)) {
		displhelp(info, docolour);
		return 1;
	}
	if (lencheck(argv[2], 2, docolour)) {
		displhelp(info, docolour);
		return 1;
	}
	//wait is the delay in ms
	DWORD wait = stoul(argv[2]);
	//get the process threads
	vector<DWORD> tidvec = ListProcessThreads(stoul(argv[1]));
	//check if the process existed
	if (!tidvec.size()) {
		printf("No threads for process with PID %s", argv[1]);
		return 1;
	}
	//suspend
	HANDLE pThread = {};
	printf("[+] Suspending:          [");
	for (int i = 0; i < tidvec.size(); i++) {
		pThread = OpenThread(THREAD_ALL_ACCESS, NULL, tidvec[i]);
		pThread ? printf("|") : printf("%d", GetLastError());
		SuspendThread(pThread);
		VirtualProtectEx(pThread, NULL, NULL, PAGE_NOACCESS, nullptr);
	}
	printf("]\n");
	//sleep
	DWORD wsecs = wait / 1000;
	printf("\r[-] Waiting: %lus                    ",wsecs);
	Sleep(wait - 1000 * wsecs);
	//dislay nice counter
	for(; wsecs > 0; wsecs -= 1) {
		printf("\r[-] Waiting: %lu ", wsecs);
		Sleep(1000);
	}
	printf("\r[+] Waited: %lds                    ", wait / 1000);
	//resume
	printf("\n[+] Resuming:            [");
	for (int i = 0; i < tidvec.size(); i++) {
		pThread = OpenThread(THREAD_ALL_ACCESS, NULL, tidvec[i]);
		pThread ? printf("|") : printf("%ld", GetLastError());
		VirtualProtectEx(pThread, NULL, NULL, PAGE_EXECUTE_READWRITE, nullptr);
		//ResumeThread until suspended count is 0
		while (ResumeThread(pThread));
	}
	printf("]\n");
	return 0;
}

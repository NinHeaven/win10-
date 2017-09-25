// EventLog.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <locale.h>

/*程序刚运行时，打印一下最新的一条错误信息*/
bool ShowFirstError(HANDLE hEventLog);

/*打印时间*/
void ShowTime(FILETIME ft);

/*程序运行起来后，有新的事件触发*/
bool ShowNewEventError(HANDLE hEventLog);


/*查询指定索引值的事件报告*/
bool ShowIndicateEventRecord(HANDLE hEventLog, DWORD dwIndex);

int main() {
	setlocale(LC_ALL, "CHS");
	/*打开程序先获取一个错误信息*/
	/* 打开应用程序事件记录
	第1个参数传0表示打开本机的第2个参数表示的log
	第2个参数随便传，只要是错误的名称，就打开应用程序错误日志*/
	HANDLE hEventLog = OpenEventLog(NULL, L"Application Error");
	ShowFirstError(hEventLog);
	/*等待触发下一个错误信息*/
	HANDLE hEvent = CreateEvent(NULL, false, 0, NULL);
	NotifyChangeEventLog(hEventLog, hEvent);
	while (1) {
		/*有新的事件报告产生，会激活hEvent*/
		WaitForSingleObject(hEvent, INFINITE);
		/*只打印最新的事件报告，事件查看器中最上面的那个*/
		ShowNewEventError(hEventLog);
	}
	return 0;
}


bool ShowFirstError(HANDLE hEventLog) {
	/*获取hEventLog指向的log的最早的一个事件的编号(不一定是1)*/
	DWORD dwOldestNum = 0;
	if (!GetOldestEventLogRecord(hEventLog, &dwOldestNum)) return 0;
	/*hEventLog的事件记录数*/
	DWORD dwRecCount = 0;
	if (!GetNumberOfEventLogRecords(hEventLog, &dwRecCount)) return 0;
	/*dwOldestNum + dwRecCount-1才是最新的事件记录索引*/
	DWORD dwIndexBegin = dwOldestNum + dwRecCount - 1;
	/*遍历查找第1个错误事件*/
	for (int i = dwIndexBegin; i > 0; i--) {
		if (!ShowIndicateEventRecord(hEventLog, i)) {
			/*没找到错误信息，找到的是其他信息*/
			continue;
		}
		else {
		/*找到最第1个错误信息，打印完毕退出*/
		return true;
		}
	}
	return false;
}


void ShowTime(FILETIME ft) {
	FILETIME lft = {};
	FileTimeToLocalFileTime(&ft, &lft);
	SYSTEMTIME st = {};
	FileTimeToSystemTime(&lft, &st);
	printf("%04d-%02d-%02d %02d:%02d:%02d\n",
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond);
}

bool ShowNewEventError(HANDLE hEventLog) {
	/*获取hEventLog指向的log的最早的一个事件的编号(不一定是1)*/
	DWORD dwOldestNum = 0;
	if (!GetOldestEventLogRecord(hEventLog, &dwOldestNum)) return 0;
	/*hEventLog的事件记录数*/
	DWORD dwRecCount = 0;
	if (!GetNumberOfEventLogRecords(hEventLog, &dwRecCount)) return 0;
	/*dwOldestNum + dwRecCount-1才是最新的事件记录索引*/
	DWORD dwIndexBegin = dwOldestNum + dwRecCount - 1;
	/*只显示最新的事件报告,是错误就打印，不是就忽略*/
	return ShowIndicateEventRecord(hEventLog,dwIndexBegin);
}

bool ShowIndicateEventRecord(HANDLE hEventLog, DWORD dwIndex) {
	DWORD dwRead = 0;/*实际读取字节数*/
	DWORD dwNeed = 0;/*函数调用需要的字节数*/
	int nRet = 0;/*函数返回值*/
				 /*参数2：EVENTLOG_SEEK_READ按索引读，EVENTLOG_SEQUENTIAL_READ按顺序读
				 EVENTLOG_FORWARDS_READ，按顺序向前读，EVENTLOG_BACKWARDS_READ按顺序向后读
				 参数3：如果是按索引读，该参数为索引;如果按顺序读，忽略该参数*/
	ReadEventLog(hEventLog,
		EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
		dwIndex,
		&dwRead,/*故意传个错误值，返回实际需要的字节数*/
		4,
		&dwRead,
		&dwNeed);
	LPBYTE lpBuff = (LPBYTE)VirtualAlloc(NULL, dwNeed, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!lpBuff) {
		printf("内存申请出错！\n");
		system("pause!");
	}
	/*第2次调用真正获取事件信息，这里默认调用正确，不做错误处理*/
	nRet = ReadEventLog(hEventLog,
		EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
		dwIndex,
		lpBuff,
		dwNeed,
		&dwRead,
		&dwNeed);
	EVENTLOGRECORD* pElr = (EVENTLOGRECORD*)lpBuff;
	/*只看错误信息,其他信息不看*/
	if (pElr->EventType != EVENTLOG_ERROR_TYPE) {
		VirtualFree(lpBuff, 0, MEM_RELEASE);
		return false;
	}
	printf("*********错误信息**************\n");
	/*找到了1个错误信息*/
	WCHAR* pInfoBegin = (WCHAR*)(lpBuff + pElr->StringOffset);
	/*从StringOffset偏移处开始，为以下信息：*/
	WCHAR* pTitle[] = {
		L"1.错误应用程序名称",
		L"2.版本",
		L"3.时间戳(创建时间)",
		L"4.错误模块名称",
		L"5.版本",
		L"6.时间戳",
		L"7.异常代码",
		L"8.错误偏移量(0x)",
		L"9.错误进程ID(0x)",
		L"10.启动时间" };/*后面还有，不关心*/
	for (int i = 0; i < 9; ++i) {
		wprintf_s(L"%s:%s\n", pTitle[i], pInfoBegin);
		pInfoBegin += wcslen(pInfoBegin) + 1;
	}
	FILETIME ft = {}, lft = {};
	swscanf_s(pInfoBegin, L"%llx", (ULONGLONG*)&ft);
	printf("错误产生时间：");
	ShowTime(ft);
	VirtualFree(lpBuff, 0, MEM_RELEASE);
	return true;
}
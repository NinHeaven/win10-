// EventLog.cpp: �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <windows.h>
#include <locale.h>

/*���������ʱ����ӡһ�����µ�һ��������Ϣ*/
bool ShowFirstError(HANDLE hEventLog);

/*��ӡʱ��*/
void ShowTime(FILETIME ft);

/*�����������������µ��¼�����*/
bool ShowNewEventError(HANDLE hEventLog);


/*��ѯָ������ֵ���¼�����*/
bool ShowIndicateEventRecord(HANDLE hEventLog, DWORD dwIndex);

int main() {
	setlocale(LC_ALL, "CHS");
	/*�򿪳����Ȼ�ȡһ��������Ϣ*/
	/* ��Ӧ�ó����¼���¼
	��1��������0��ʾ�򿪱����ĵ�2��������ʾ��log
	��2��������㴫��ֻҪ�Ǵ�������ƣ��ʹ�Ӧ�ó��������־*/
	HANDLE hEventLog = OpenEventLog(NULL, L"Application Error");
	ShowFirstError(hEventLog);
	/*�ȴ�������һ��������Ϣ*/
	HANDLE hEvent = CreateEvent(NULL, false, 0, NULL);
	NotifyChangeEventLog(hEventLog, hEvent);
	while (1) {
		/*���µ��¼�����������ἤ��hEvent*/
		WaitForSingleObject(hEvent, INFINITE);
		/*ֻ��ӡ���µ��¼����棬�¼��鿴������������Ǹ�*/
		ShowNewEventError(hEventLog);
	}
	return 0;
}


bool ShowFirstError(HANDLE hEventLog) {
	/*��ȡhEventLogָ���log�������һ���¼��ı��(��һ����1)*/
	DWORD dwOldestNum = 0;
	if (!GetOldestEventLogRecord(hEventLog, &dwOldestNum)) return 0;
	/*hEventLog���¼���¼��*/
	DWORD dwRecCount = 0;
	if (!GetNumberOfEventLogRecords(hEventLog, &dwRecCount)) return 0;
	/*dwOldestNum + dwRecCount-1�������µ��¼���¼����*/
	DWORD dwIndexBegin = dwOldestNum + dwRecCount - 1;
	/*�������ҵ�1�������¼�*/
	for (int i = dwIndexBegin; i > 0; i--) {
		if (!ShowIndicateEventRecord(hEventLog, i)) {
			/*û�ҵ�������Ϣ���ҵ�����������Ϣ*/
			continue;
		}
		else {
		/*�ҵ����1��������Ϣ����ӡ����˳�*/
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
	/*��ȡhEventLogָ���log�������һ���¼��ı��(��һ����1)*/
	DWORD dwOldestNum = 0;
	if (!GetOldestEventLogRecord(hEventLog, &dwOldestNum)) return 0;
	/*hEventLog���¼���¼��*/
	DWORD dwRecCount = 0;
	if (!GetNumberOfEventLogRecords(hEventLog, &dwRecCount)) return 0;
	/*dwOldestNum + dwRecCount-1�������µ��¼���¼����*/
	DWORD dwIndexBegin = dwOldestNum + dwRecCount - 1;
	/*ֻ��ʾ���µ��¼�����,�Ǵ���ʹ�ӡ�����Ǿͺ���*/
	return ShowIndicateEventRecord(hEventLog,dwIndexBegin);
}

bool ShowIndicateEventRecord(HANDLE hEventLog, DWORD dwIndex) {
	DWORD dwRead = 0;/*ʵ�ʶ�ȡ�ֽ���*/
	DWORD dwNeed = 0;/*����������Ҫ���ֽ���*/
	int nRet = 0;/*��������ֵ*/
				 /*����2��EVENTLOG_SEEK_READ����������EVENTLOG_SEQUENTIAL_READ��˳���
				 EVENTLOG_FORWARDS_READ����˳����ǰ����EVENTLOG_BACKWARDS_READ��˳������
				 ����3������ǰ����������ò���Ϊ����;�����˳��������Ըò���*/
	ReadEventLog(hEventLog,
		EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
		dwIndex,
		&dwRead,/*���⴫������ֵ������ʵ����Ҫ���ֽ���*/
		4,
		&dwRead,
		&dwNeed);
	LPBYTE lpBuff = (LPBYTE)VirtualAlloc(NULL, dwNeed, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!lpBuff) {
		printf("�ڴ��������\n");
		system("pause!");
	}
	/*��2�ε���������ȡ�¼���Ϣ������Ĭ�ϵ�����ȷ������������*/
	nRet = ReadEventLog(hEventLog,
		EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ,
		dwIndex,
		lpBuff,
		dwNeed,
		&dwRead,
		&dwNeed);
	EVENTLOGRECORD* pElr = (EVENTLOGRECORD*)lpBuff;
	/*ֻ��������Ϣ,������Ϣ����*/
	if (pElr->EventType != EVENTLOG_ERROR_TYPE) {
		VirtualFree(lpBuff, 0, MEM_RELEASE);
		return false;
	}
	printf("*********������Ϣ**************\n");
	/*�ҵ���1��������Ϣ*/
	WCHAR* pInfoBegin = (WCHAR*)(lpBuff + pElr->StringOffset);
	/*��StringOffsetƫ�ƴ���ʼ��Ϊ������Ϣ��*/
	WCHAR* pTitle[] = {
		L"1.����Ӧ�ó�������",
		L"2.�汾",
		L"3.ʱ���(����ʱ��)",
		L"4.����ģ������",
		L"5.�汾",
		L"6.ʱ���",
		L"7.�쳣����",
		L"8.����ƫ����(0x)",
		L"9.�������ID(0x)",
		L"10.����ʱ��" };/*���滹�У�������*/
	for (int i = 0; i < 9; ++i) {
		wprintf_s(L"%s:%s\n", pTitle[i], pInfoBegin);
		pInfoBegin += wcslen(pInfoBegin) + 1;
	}
	FILETIME ft = {}, lft = {};
	swscanf_s(pInfoBegin, L"%llx", (ULONGLONG*)&ft);
	printf("�������ʱ�䣺");
	ShowTime(ft);
	VirtualFree(lpBuff, 0, MEM_RELEASE);
	return true;
}
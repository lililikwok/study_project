// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
#include <winbase.h>
#include <iostream>
#include <string>
#include <sstream>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize)//将提供的字节数组（BYTE* pData）转储成一个十六进制字符串，并通过 OutputDebugStringA 输出调试信息
{
    std::string strOut;//存转的字符串
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))//每16字节加一个换行符
            strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() {//创建裆前系统可用的磁盘分区信息,1代表A盘，2代表B盘...26代表Z盘
    std::string result;//存储结果字符串
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            result += 'A' + i - 1;
            result += ',';
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());//创建CPacket实例打包用的
    Dump((BYTE*)pack.Data(), pack.Size() + 6);//输出pack的数据内容
    CServerSocket::getInstance()->Send(pack);
    return 0;
}
#include <stdio.h>
#include <io.h>
#include <list>

int MakeDirectoryInfo() {
	std::string strPath;
	if (!CServerSocket::getInstance()->GetFilePath(strPath)) {
		OutputDebugString(_T("[Server] 命令不是获取文件列表\n"));
		return -1;
	}

	OutputDebugStringA(("[Server] 收到目录请求: " + strPath + "\n").c_str());

	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo = {};
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		OutputDebugString(_T("[Server] 无权限访问目录\n"));
		return -2;
	}

	_finddata_t fdata;
	__int64 hfind = _findfirst("*", &fdata);
	if (hfind == -1) {
		OutputDebugString(_T("[Server] 没有找到任何文件\n"));
		FILEINFO finfo = {};
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		return -3;
	}

	do {
		FILEINFO finfo = {};
		finfo.HasNext = TRUE;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		strncpy(finfo.szFileName, fdata.name, sizeof(finfo.szFileName) - 1);

		// 打包
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
        
		// 调试输出（控制台 + OutputDebugString）
        PFILEINFO aab = (PFILEINFO)pack.strData.c_str();
		//std::string log = "[Server] 发送文件项: ";
		//log += finfo.szFileName;
		//log += " | Dir: ";
		//log += std::to_string(aab->IsDirectory);
		//log += " | Size: ";
		//log += std::to_string(pack.strData.size());
		//log += " | Name: ";
		//log += aab->szFileName;
		//log += " | HasNext: ";
  //      PFILEINFO aa = (PFILEINFO)pack.strData.c_str();
		//log += std::to_string(aa->HasNext);
		//OutputDebugStringA((log + "\n").c_str());

	} while (_findnext(hfind, &fdata) == 0);

	// 最后一包：HasNext = FALSE
	FILEINFO finfo = {};
	finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);
	OutputDebugString(_T("[Server] 已发送结束标志 (HasNext = 0)\n"));

	return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//将在用户的机器上运行一个程序或打开一个文件，具体行为取决于文件类型的关联程序。比如，如果路径是一个可执行文件，则会运行该程序；如果路径是文档文件，则会打开与之关联的应用程序查看该文档。
    /*
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);：调用 Win32 API 函数 ShellExecuteA 来执行或打开 strPath 变量中的文件。此函数的参数解释如下：
    NULL：表示函数不需要使用窗口句柄。
    NULL：操作设为 NULL 表示执行文件，默认操作通常是“打开”。
    strPath.c_str()：指定要运行或打开的文件的路径。
    NULL：没有要传递给要执行的程序的参数。
    NULL：默认目录设置为 NULL，所以执行文件时会使用它的默认目录。
    SW_SHOWNORMAL：指定窗口的显示方式，SW_SHOWNORMAL 为普通窗口大小。
    */
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

//#pragma warning(disable:4966)
int DownloadFile() {//下载文件，即把文件从服务端发送到客户端
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");//rb:二进制读取模式，文件指针pfile
    if (err != 0) {//打开文件失败
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);//用fseek函数将文件指针pfile移动到文件末尾
        data = _ftelli64(pFile);//_ftelli64：获取文件大小
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);//用fseek函数将文件指针pfile定位到文件起始位置
        char buffer[1024] = "";
        size_t rlen = 0;//fread返回值
        do {
            rlen = fread(buffer, 1, 1024, pFile);//在buffer里读，一次读1字节，读1024次
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent() {//处理鼠标事件
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags = 0;//根据鼠标的按钮和操作设置标志位

        switch (mouse.nButton) {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4://没有按键
            nFlags = 8;
            break;
        }
        if (nFlags != 8)
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//将光标移动到鼠标事件指定的屏幕坐标
        switch (mouse.nAction)
        {
            //nFlags 之前存储了哪个鼠标按钮被按下的信息，通过|=再加上鼠标动作的信息（二进制高位低位）
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下
            nFlags |= 0x40;
            break;
        case 3://放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标参数失败"));
        return -1;

    }

    return 0;
}

//这里的发送屏幕截图是直接全部发送，没有做分包
int SendScreen() {
    CImage screen; // GDI+ image object
    // 1. Capture the screen
    HDC hScreen = ::GetDC(NULL); // entire screen DC
    if (!hScreen) return -1;

    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
    int nWidth = GetDeviceCaps(hScreen, HORZRES)- 100;
    int nHeight = GetDeviceCaps(hScreen, VERTRES)- 50;

    screen.Create(nWidth, nHeight, nBitPerPixel);
    if (!screen) {
        ReleaseDC(NULL, hScreen);
        return -1;
    }

    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);

    // 2. Save to file for verification
    TCHAR szPath[MAX_PATH] = _T("screenshot.png");
    HRESULT hrSave = screen.Save(szPath, Gdiplus::ImageFormatPNG);
    if (FAILED(hrSave)) {
        TRACE("Failed to save local screenshot: 0x%08X\n", hrSave);
    }

    // 3. Stream to memory for network send
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMem == NULL)
        return -1;

    IStream* pStream = nullptr;
    HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    if (hRet == S_OK && pStream) {
        // Write PNG data into stream
        hrSave = screen.Save(pStream, Gdiplus::ImageFormatPNG);
        if (FAILED(hrSave)) {
            TRACE("Failed to save PNG to stream: 0x%08X\n", hrSave);
        }
        else {
            // Rewind stream
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, nullptr);

            // Lock memory and send packet
            PBYTE pData = (BYTE*)GlobalLock(hMem);
            SIZE_T nSize = GlobalSize(hMem);
            if (pData && nSize > 0) {
                CPacket pack(6, pData, nSize);
                CServerSocket::getInstance()->Send(pack);
            }
            GlobalUnlock(hMem);
        }
        pStream->Release();
    }
    else {
        TRACE("CreateStreamOnHGlobal failed: 0x%08X\n", hRet);
    }

    // 4. Cleanup
    GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}



#include "LockDialog.h"
CLockDialog dlg;
unsigned threadid = 0;

unsigned __stdcall threadLockDlg(void* arg) {
    TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    dlg.Create(IDD_DIALOG_INFO, NULL);//创建一个由 IDD_DIALOG_INFO 标识的非模态对话框实例，并且这个对话框是一个顶级窗口，因为它没有指定父窗口
    dlg.ShowWindow(SW_SHOW);//让dlg对话框处于可见状态
    //窗口置顶

    CRect rect;

    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    TRACE("right = %d bottom = %d\r\n", rect.right, rect.bottom);
    dlg.MoveWindow(rect);
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);//将 dlg 对话框保持在所有窗口的最上层，但是不改变其大小和位置。
    //限制鼠标功能
    //ShowCursor(false);//隐藏光标
    //隐藏任务栏
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//寻找类名为 Shell_TrayWnd（即 Windows 任务栏）的窗口，然后通过 ShowWindow 函数将其设置为显示状态。这通常会导致原本隐藏的任务栏被显示出来。
    //限制鼠标活动范围
    //dlg.GetWindowRect(rect);//GetWindowRect 函数获取了 dlg 窗口在屏幕上的位置和尺寸，再赋值给 rect
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    ClipCursor(rect);
    MSG msg;
    /*消息循环
    1 消息检索，从当前线程的消息队列中检索消息，如果队列为空，GetMessage 将会等待，直到有消息到来。
    2 消息翻译，翻译键盘输入消息，如按键消息转化为字符消息，这样就可以在窗口过程中正确处理键盘输入
    3 消息分派，将消息分发到对应的窗口过程，这个步骤实际上调用的是窗口关联的回调函数，函数通过分析消息并执行相应的操作来处理它们
    */

    while (GetMessage(&msg, NULL, 0, 0)) {//从程序的消息队列中取得用户的输入和其他事件,这个函数会阻塞，直到有消息可取
        TranslateMessage(&msg);//翻译虚拟键（如方向键、功能键等）到字符消息。
        DispatchMessage(&msg);//把消息分发给窗口程序的窗口过程函数，进行消息处理
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == 0x41) {//按ESC退出
                break;
            }
        }
    }

    ShowCursor(true);
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//寻找类名为 Shell_TrayWnd（即 Windows 任务栏）的窗口，然后通过 ShowWindow 函数将其设置为显示状态。这通常会导致原本隐藏的任务栏被显示出来。
    dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}

int LockMachine() {
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {//dlg的句柄为空或无效
        //_beginthread 创建的线程应该自行结束，因此在该线程开始后，LockMachine 函数将立即结束而不等待新线程的结束
        //_beginthread(threadLockDlg, 0, NULL);//_beginthread创建新线程，用函数指针threadLockDlg指向线程应该执行的函数
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d\r\n", threadid);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int UnlockMachine() {
    //dlg.SendMessage(WM_KEYDOWN,0x41, LPARAM(001E0001));
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);//将一个消息(键盘)投递到指定线程(threadid)—的消息队列中
    //消息是跟着线程来的，不是跟着对话框，句柄来的
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int TestConnect() {
    CPacket pack(1981,NULL,0);
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DeleteLocalFile() {
	// 1. 从网络包里取出要删除的路径
	std::string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);

	// 2. 调用 DeleteFile
	BOOL success = DeleteFileA(strPath.c_str());  // ANSI 版本，如果你用 Unicode 则用 DeleteFileW

	if (!success) {
		// 3. 出错时获取错误码
		DWORD err = GetLastError();

		// 4. 将错误码格式化成可读字符串
		LPVOID msgBuf = nullptr;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&msgBuf,
			0, NULL);

		std::ostringstream oss;
		oss << "DeleteFile failed. Path: " << strPath
			<< ", Error " << err << ": " << (msgBuf ? (LPSTR)msgBuf : "Unknown");

		// 5. 输出到调试窗口，也可以发送给客户端
		OutputDebugStringA(oss.str().c_str());
		if (msgBuf) LocalFree(msgBuf);

		// 可以把错误信息也打包回客户端，方便前端展示
		std::string errStr = oss.str();
		CPacket errPack(9, (BYTE*)errStr.c_str(), errStr.size());
		CServerSocket::getInstance()->Send(errPack);

		return -1;
	}

	// 6. 删除成功，给客户端发送确认包
	CPacket pack(9, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}
int ExcuteCommand(int nCmd) {
    int ret = 0;
	switch (nCmd) {
	case 1://查看磁盘分区
        ret = MakeDriverInfo();
		break;
	case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
		break;
	case 3://打开文件
        ret = RunFile();
		break;
	case 4:// 下载文件
        ret = DownloadFile();
		break;
	case 5://鼠标操作
        ret = MouseEvent();
		break;
	case 6://发送屏幕内容，即发送屏幕的截图
        ret = SendScreen();
        break;
	case 7://锁机
        ret = LockMachine();
		break;
	case 8://解锁
        ret = UnlockMachine();
		break;
    case 9://删除文件
        ret = DeleteLocalFile();
        break;
    case 1981 :
        ret = TestConnect();
        break;
	}
    return ret;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误 
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CServerSocket* pserver = CServerSocket::getInstance();//服务器创建单例
            int count = 0;
            if (pserver->InitSocket() == false) {//初始化
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance() != NULL) {
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
                int ret = pserver->DealCommand();//这里的ret是要执行的功能的编号
                if (ret > 0) {
					ret = ExcuteCommand(pserver->GetPacket().sCmd);
                    if (ret != 0) {//这里的ret是功能模块的执行结果输出，结果为0则是正常执行
                        TRACE("执行命令失败：%d ret = %d\r\n",pserver->GetPacket().sCmd,ret);
                    }
                }

                pserver->CloseClient();
            }
        }
    }
    else
    {
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}

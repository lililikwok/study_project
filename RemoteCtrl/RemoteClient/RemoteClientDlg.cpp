
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框
CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}



int CRemoteClientDlg::SendCommandPacket(int nCmd,bool bAutoClose, BYTE* pData, size_t nLength)
{
	 
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));//TODO:返回值的处理
	if (!ret) {
		MessageBox("网络初始化失败");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret %d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	if(bAutoClose)
		pClient->CloseSocket();
	return cmd;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEXT, &CRemoteClientDlg::OnBnClickedBtnText)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)//第三步，在消息映射表里面注册消息
	//将 Windows 消息（在这里是 WM_SEND_PACKET）映射到特定消息处理函数（在这里是 CRemoteClientDlg::OnSendPacket）的一种方法。这段代码通常在一个消息映射宏列表中。
	//WM_SEND_PACKET是用户自定义的待处理的消息
	//&CRemoteClientDlg::OnSendPacket是用来处理这个消息的函数。这个函数必须是声明在接收处理消息的类（在这里是 CRemoteClientDlg）内的成员函数。
	ON_BN_CLICKED(IDC_BTN_WATCH, &CRemoteClientDlg::OnBnClickedBtnWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	ShowWindow(SW_MINIMIZE);

	// TODO: 在此添加额外的初始化代码

	UpdateData();
	m_nPort = _T("9527");
	m_server_address = 0x7F000001;
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);//因为放在别的地方可能会创建两次引发异常，所以CREATE放在主类的构造函数里面
	m_dlgStatus.FlashWindow(SW_HIDE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnText()
{
	SendCommandPacket(1981);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();

	std::string drivers = pClient->getPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(_T(""), hTemp, TVI_LAST);  // ← 占位空节点
			//m_Tree.InsertItem(dr.c_str(),TVI_ROOT,TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
}

//获取当前的
CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {//返回值代表从树视图控件 m_Tree 中给定项 hTree 到根的路径。
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);//获取当前节点的文本，并将其存储到 strTmp
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);//获取当前节点的父节点，并将父节点作为下一次循环的当前节点
	} while (hTree != NULL);
	return strRet;//包含了从起始节点到树的根的完整路径
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	if (nCmd < 0) {
		AfxMessageBox(_T("命令发送或接收初始响应失败"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	while (true) {
		CPacket currentPacket = pClient->getPacket();
		PFILEINFO pInfo = (PFILEINFO)currentPacket.strData.c_str();
		if (!pInfo->HasNext) {
			TRACE("[Client] 收到结束标志，目录传输完成。\n");
			pClient->CloseSocket();
			break;
		}
		CString name(pInfo->szFileName);
		if (!(pInfo->IsDirectory && (name == _T(".") || name == _T("..")))) {
			if (pInfo->IsDirectory) {
			}
			else
				m_List.InsertItem(0, pInfo->szFileName);
		}
		if (pClient->DealCommand() < 0) {
			AfxMessageBox(_T("与服务器断开连接或接收后续数据出错!"));
			break;
		}
	}

}
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) return;
	if (m_Tree.GetItemState(hTreeSelected, TVIS_EXPANDED) == 0 && m_Tree.GetChildItem(hTreeSelected) == NULL) {
		return;
	}

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);

	// 初始化文件计数器
	int nFileCount = 0;

	// 1. 调用你不想修改的 SendCommandPacket 函数。
	// 函数执行后，第一个文件包已经由 DealCommand 内部调用并存放在 pClient->m_packet 中。
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	if (nCmd < 0) { // 检查返回值是否表示错误
		AfxMessageBox(_T("命令发送或接收初始响应失败"));
		return;
	}

	CClientSocket* pClient = CClientSocket::getInstance();

	// 2. 启动一个 "先处理、后获取" 的循环
	while (true) {
		// 关键修复：获取 m_packet 的一个安全拷贝，防止悬空指针！
		CPacket currentPacket = pClient->getPacket();
		PFILEINFO pInfo = (PFILEINFO)currentPacket.strData.c_str();

		// 检查是否是结束包
		if (!pInfo->HasNext) {
			// 打印接收结束信息
			TRACE(_T("[Client] 收到结束标志，目录传输完成。\n"));
			// 打印最终统计的文件数量
			TRACE(_T("[Client] 本次接收的文件总数: %d\n"), nFileCount);
			pClient->CloseSocket();
			break;
		}

		// 处理当前包的数据
		CString name(pInfo->szFileName);
		// 排除 "." 和 ".." 目录
		if (!(pInfo->IsDirectory && (name == _T(".") || name == _T("..")))) {
			if (pInfo->IsDirectory) {
				HTREEITEM hTemp = m_Tree.InsertItem(name, hTreeSelected, TVI_LAST);
				m_Tree.InsertItem(_T(""), hTemp, TVI_LAST);
			}
			else {
				// 插入到列表
				m_List.InsertItem(0, pInfo->szFileName);
				// 增加文件计数
				++nFileCount;
				// 用 TRACE 打印当前收到的文件和累积计数（可选）
				TRACE(_T("[Client] 收到文件: %s | 当前已接收文件数: %d\n"), pInfo->szFileName, nFileCount);
			}
		}

		// 3. 在循环末尾，调用 DealCommand 获取下一个数据包，为下一次循环做准备
		if (pClient->DealCommand() < 0) {
			AfxMessageBox(_T("与服务器断开连接或接收后续数据出错!"));
			// 在错误情况下也可以打印当前统计
			TRACE(_T("[Client] 接收过程中出错，当前已接收文件总数: %d\n"), nFileCount);
			break;
		}
	}
}




void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}
//右键单击
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenuA(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{//开线程处理数据
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	CClientSocket* pClient = CClientSocket::getInstance();
	for (;;)
	{
		if(!m_isFull){
			int ackCmd = SendMessage(WM_SEND_PACKET, 6 << 1 | 0);
			if (ackCmd == 6)
			{
				// UI 线程已把包存进 pClient->getPacket()
				const CPacket& pkt = pClient->getPacket();
				const BYTE* pData = (const BYTE*)pkt.strData.data();
				size_t      cbData = pkt.strData.size();
				// ---- 原来的拷贝到 IStream 的代码保持不变 ----
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (!hMem) { Sleep(1); continue; }
				IStream* pStream = nullptr;
				if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
				{
					ULONG written = 0;
					pStream->Write(pData, (ULONG)cbData, &written);
					LARGE_INTEGER bg = { 0 };
					pStream->Seek(bg, STREAM_SEEK_SET, nullptr);
					m_image.Load(pStream);
					m_isFull = true;
				}
			}
		}
		else
		{
			Sleep(1);      // 网络异常时降 CPU
		}
	}
}




	void CRemoteClientDlg::threadEntryForDownFile(void* arg)
	{
		CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
		thiz->threadDownFile();
		_endthread();

	}

	void CRemoteClientDlg::threadDownFile()
	{
		int nListSelected = m_List.GetSelectionMark();//从列表视图控件 m_List 中获取当前选中项的索引
		CString strFile = m_List.GetItemText(nListSelected, 0);//用刚刚拿到的索引来拿选中项的文件名

		long long nCount = 0;
		//CFileDialog 是 MFC (Microsoft Foundation Classes) 库中用于创建文件对话框的类
		//FALSE 参数用于指定对话框的模式，这里 FALSE 表示对话框是“保存文件”模式。如果是 TRUE，则表示为“打开文件”模式。
		//"*" 参数是默认的文件扩展名。在这种情况下，它被设置为一个通配符，这意味着所有文件扩展名都可以。
		//m_List.GetItemText(nListSelected, 0) 获得列表控件中当前被选中的项的文本，通常表示文件名，将用作对话框显示的默认文件名。
		//OFN_HIDEREADONLY 表示隐藏只读选项。
		//OFN_OVERWRITEPROMPT 表示在用户选择要保存到一个已经存在的文件时，提示用户确认是否覆盖原文件。

		CFileDialog dlg(FALSE, "*",
			strFile,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			NULL, this);
		//以模态方式显示对话框。如果用户点击“保存”按钮（通常响应为 IDOK）
		// 则 DoModal 方法会返回 IDOK，代码将进入到 if 语句块内执行
		if (dlg.DoModal() == IDOK) {
			//开个文件把接收到的文件写到本地
			FILE* pFile = fopen(dlg.GetPathName(), "wb+");//以二进制模式打开文件，允许读写操作
			if (pFile == NULL) {
				AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建"));
				m_dlgStatus.ShowWindow(SW_HIDE);//将 m_dlgStatus 对话框隐藏起来
				EndWaitCursor();
				return;
			}
			HTREEITEM hSelected = m_Tree.GetSelectedItem();//获取选中项的句柄
			strFile = GetPath(hSelected) + strFile;//父项和该项的路径，构成完整的文件路径
			TRACE("%s\r\n", LPCSTR(strFile));
			CClientSocket* pClient = CClientSocket::getInstance();//获取单例模式中的实例
			do {
				//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());//发送下载命令到服务器
				int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);//SendMessage 函数是一个Windows API，用于发送一个消息到某个窗口的消息队列，
				if (ret < 0) {
					AfxMessageBox("执行下载命令失败");
					TRACE("执行下载失败：ret = %d\r\n", ret);
					break;
				}
				long long nLength = *(long long*)pClient->getPacket().strData.c_str();
				if (nLength == 0) {
					AfxMessageBox("文件长度为0或者无法读取文件");
					break;
				}
				while (nCount < nLength) {//已接收的数据量未达到文件总数据量
					ret = pClient->DealCommand();
					if (ret < 0) {
						AfxMessageBox("传输失败");
						TRACE("传输失败: ret = %d\r\n", ret);
						break;
					}
					fwrite(pClient->getPacket().strData.c_str(), 1, pClient->getPacket().strData.size(), pFile);
					nCount += pClient->getPacket().strData.size();//更新已经接收到的数据量
				}
			} while (false);
			fclose(pFile);
			pClient->CloseSocket();
		}
		m_dlgStatus.ShowWindow(SW_HIDE);//将 m_dlgStatus 对话框隐藏起来
		EndWaitCursor();
		MessageBox(_T("下载完成"), _T("完成"));

	}




void CRemoteClientDlg::OnDownloadFile()
{
	//添加线程函数
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	//1 作为线程入口点，这个函数将在新线程中执行
	//2 0是初始线程堆栈大小的参数。数值0表示使用默认的大小
	//3 传递给线程的参数。在这种情况下，this 指针指向当前正在执行 _beginthread 调用的类实例 CRemoteClientDlg 对象。
	BeginWaitCursor();//沙漏光标
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中"));//SetWindowText 方法用于设置 m_info 控件的文本内容。在这里，它被设置为显示 "命令正在执行中"
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();//使 m_dlgStatus 对话框成为当前活动窗口,将对话框带到屏幕的最前端
}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！");
	}
	LoadFileCurrent();
}

void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！");
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd){
	case 4:{
			CString strFile = (LPCSTR)lParam;
			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());//发送下载命令到服务器
			//只接收两个函数的处理
			ret = SendCommandPacket(wParam >> 1, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());//发送下载命令到服务器
		}
		break;
	case 6:
		{
			ret = SendCommandPacket(cmd, wParam & 1);
		}
		break;
	default:
		ret = -1;
	}
	//类 CRemoteClientDlg 中的一个消息处理函数 OnSendPacket 的定义。
	//函数 OnSendPacket 响应自定义的 WM_SEND_PACKET 消息，并执行发送命令到服务器的操作。
	return ret;
}

void CRemoteClientDlg::OnBnClickedBtnWatch()
{
	CWatchDialog dlg(this);
	// TODO: 在此添加控件通知处理程序代码
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
}

void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}

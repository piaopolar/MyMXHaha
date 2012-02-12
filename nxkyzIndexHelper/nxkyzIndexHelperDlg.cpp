// nxkyzIndexHelperDlg.cpp : 实现文件
#include "stdafx.h"

#include "afxinet.h"

#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>
#include <map>

#include "msime.h"
#include "nxkyzIndexHelper.h"

#include "nxkyzIndexHelperDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
CEdit *s_pEditLog = NULL;

// ============================================================================
// ==============================================================================

bool InitReplaceTable(void);

std::string WChar2Ansi(LPCWSTR pwszSrc)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (nLen <= 0) {
		return std::string("");
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	char *pszDst = new char[nLen];
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pszDst) {
		return std::string("");
	}

	WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, pszDst, nLen, NULL, NULL);
	pszDst[nLen - 1] = 0;

	std::string strTemp(pszDst);
	delete[] pszDst;

	return strTemp;
}

// ============================================================================
// ==============================================================================
std::string Big2GB(std::string strBig5)
{
	if (strBig5 == "") {
		return "";
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~
	const char *pszBig5 = NULL; // Big5编码的字符
	wchar_t *wszUnicode = NULL; // Unicode编码的字符
	char *pszGbt = NULL;		// Gb编码的繁体字符
	char *pszGbs = NULL;		// Gb编码的简体字符
	std::string sGb;			// 返回的字符串
	int iLen = 0;				// 需要转换的字符数
	//~~~~~~~~~~~~~~~~~~~~~~~~~

	pszBig5 = strBig5.c_str();	// 读入需要转换的字符参数

	// 计算转换的字符数
	iLen = MultiByteToWideChar(936, 0, pszBig5, -1, NULL, 0);

	// 给wszUnicode分配内存
	wszUnicode = new wchar_t[iLen + 1];

	// 转换Big5码到Unicode码，使用了API函数MultiByteToWideChar
	MultiByteToWideChar(936, 0, pszBig5, -1, wszUnicode, iLen);

	// 计算转换的字符数
	iLen = WideCharToMultiByte(936, 0, (PWSTR) wszUnicode, -1, NULL, 0, NULL,
							   NULL);

	// 给pszGbt分配内存
	pszGbt = new char[iLen + 1];

	// 给pszGbs分配内存
	pszGbs = new char[iLen + 1];

	// 转换Unicode码到Gb码繁体，使用API函数WideCharToMultiByte
	WideCharToMultiByte(936, 0, (PWSTR) wszUnicode, -1, pszGbt, iLen, NULL, NULL);

	// 转换Gb码繁体到Gb码简体，使用API函数LCMapString
	LCMapString(0x0804, LCMAP_SIMPLIFIED_CHINESE, pszGbt, -1, pszGbs, iLen);

	// 返回Gb码简体字符
	sGb = pszGbs;

	// 释放内存
	delete[] wszUnicode;

	delete[] pszGbt;

	delete[] pszGbs;

	return sGb;
}

void RidChineseSpell(std::string &strInput);
void ReplaceStdString(std::string &str, const std::string &strSrc,
					  const std::string &strDest);

// ============================================================================
// ==============================================================================

std::string GetChineseSpellOneWord(std::string strInput)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// 定义IFELanguage接口的IID
	static const IID IID_IFELanguage =
	{
		0x019f7152, 0xe6db, 0x11d0,
		{ 0x83, 0xc3, 0x00, 0xc0, 0x4f, 0xdd, 0xb8, 0x2e }
	};
	// 指定使用的语言，我们的例子使用简体中文，其他还有： MSIME.China
	// MSIME.Japan MSIME.Taiwan MSIME.Taiwan.ImeBbo
	LPCWSTR msime = L"MSIME.China";
	CLSID clsid;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (CLSIDFromString (const_cast<LPWSTR>(msime), &clsid) != S_OK) {
		return "";
	}

	//~~~~~~~~~~~~~~~~~~~~~~
	// 创建一个IFELanguage的COM实例，得到接口指针
	IFELanguage *pIFELanguage;
	//~~~~~~~~~~~~~~~~~~~~~~

	if (CoCreateInstance(clsid, NULL, CLSCTX_SERVER, IID_IFELanguage,
		(LPVOID *) &pIFELanguage) != S_OK) {
		return "";
	}

	if (!pIFELanguage) {
		return "";
	}

	// 打开
	if (pIFELanguage->Open() != S_OK) {
		pIFELanguage->Release();
		return "";
	}

	//~~~~~~~~~
	DWORD dwCaps;
	//~~~~~~~~~

	if (pIFELanguage->GetConversionModeCaps(&dwCaps) != S_OK) {
		pIFELanguage->Close();
		pIFELanguage->Release();
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int nSize = strInput.length() * 2;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (nSize < 256) {
		nSize = 256;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	wchar_t *pwchInput = new wchar_t[nSize];
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	setlocale(LC_CTYPE, "");	// 很重要，没有这一句，转换会失败

	mbstowcs(pwchInput, strInput.c_str(), nSize - 1);

	//~~~~~~~~~~~~~~
	// 要解析的中文句子
	MORRSLT *pmorrslt;
	//~~~~~~~~~~~~~~

	// 通过GetJMorphResult方法为汉字加注拼音
	if (pIFELanguage->GetJMorphResult(FELANG_REQ_REV,
		FELANG_CMODE_PINYIN | FELANG_CMODE_NOINVISIBLECHAR,
		static_cast<INT>(wcslen(pwchInput)), const_cast<WCHAR *>(pwchInput),
		NULL, &pmorrslt) != S_OK || !pmorrslt) {
		pIFELanguage->Close();
		pIFELanguage->Release();
		delete[] pwchInput;
		return "";
	}

	delete[] pwchInput;

	pmorrslt->pwchOutput[pmorrslt->cchOutput] = 0;
	pmorrslt->pwchOutput[pmorrslt->cchOutput + 1] = 0;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// //
	std::string strRet = WChar2Ansi(pmorrslt->pwchOutput);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// // 记的清场咯
	CoTaskMemFree(pmorrslt);
	pIFELanguage->Close();
	pIFELanguage->Release();

	return strRet;
}

// ============================================================================
// ==============================================================================
std::string GetSortUseSpell(std::string strInput)
{
	//~~~~~~~~~~~~~~~~~~
	std::string strRet;
	char szTmp[4];
	std::string strNum[] =
	{
		"零", "一", "二", "三", "四", "五", "六", "七", "八", "九"
	};
	//~~~~~~~~~~~~~~~~~~

	for (int i = 0; i < strInput.length(); ++i) {
		if ('0' <= strInput[i] && strInput[i] <= '9') {
			strRet += GetChineseSpellOneWord(strNum[strInput[i] - '0']);
		} else if (0 <= strInput[i] && strInput[i] < 128) {
			szTmp[0] = strInput[i];
			szTmp[1] = 0;
			strRet += szTmp;
		} else {
			szTmp[0] = strInput[i];
			szTmp[1] = strInput[i + 1];
			szTmp[2] = 0;
			strRet += GetChineseSpellOneWord(szTmp);
			++i;
		}
	}

	RidChineseSpell(strRet);

	return strRet;
}

// ============================================================================
// ==============================================================================
void DownStr(std::string &str)
{
	for (int i = 0; i < str.length(); ++i) {
		if (str[i] >= 'A' && str[i] <= 'Z') {
			str[i] += 'a' - 'A';
		}
	}
}

// ============================================================================
// ==============================================================================
void UpStr(std::string &str)
{
	for (int i = 0; i < str.length(); ++i) {
		if (str[i] >= 'a' && str[i] <= 'z') {
			str[i] += 'A' - 'a';
		}
	}
}

// ============================================================================
//    用于应用程序“关于”菜单项的 CAboutDlg 对话框
// ============================================================================
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };
protected:
	virtual void DoDataExchange(CDataExchange *pDX);	// DDX/DDV 支持

// ----------------------------------------------------------------------------
//    实现
// ----------------------------------------------------------------------------
protected:
	DECLARE_MESSAGE_MAP()
};

// ============================================================================
// ==============================================================================

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

// ============================================================================
// ==============================================================================
void CAboutDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// ============================================================================
//    CnxkyzIndexHelperDlg 对话框
// ============================================================================
CnxkyzIndexHelperDlg::CnxkyzIndexHelperDlg(CWnd *pParent /* NULL */ ) : CDialog(CnxkyzIndexHelperDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDT_LOGINFO, m_edtLogInfo);
	DDX_Control(pDX, IDC_EDT_JOKE, m_edtJoke);
}

BEGIN_MESSAGE_MAP(CnxkyzIndexHelperDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED
(IDC_MAIN_BTN_OPEN, &CnxkyzIndexHelperDlg::OnBnClickedMainBtnOpen)
	ON_BN_CLICKED
(IDC_MAIN_BTN_OPEN_HTM, &CnxkyzIndexHelperDlg::OnBnClickedMainBtnOpenHtm)
	ON_BN_CLICKED
(
	IDC_MAIN_BTN_DOWN_INDEX_CHECK, &CnxkyzIndexHelperDlg::
		OnBnClickedMainBtnDownIndexCheck
)
	ON_BN_CLICKED
(
	IDC_MAIN_BTN_DOWNLOAD_TITLE_URL, &CnxkyzIndexHelperDlg::
		OnBnClickedMainBtnDownloadTitleUrl
)
	ON_BN_CLICKED
(IDC_MAIN_BTN_MERGE_SIMPLE, &CnxkyzIndexHelperDlg::OnBnClickedMainBtnMergeSimple)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_GET, &CnxkyzIndexHelperDlg::OnBnClickedButtonGet)
	ON_BN_CLICKED(IDC_BUTTON_PRE, &CnxkyzIndexHelperDlg::OnBnClickedButtonPre)
	ON_BN_CLICKED(IDC_BUTTON_NEXT, &CnxkyzIndexHelperDlg::OnBnClickedButtonNext)
END_MESSAGE_MAP()

// ============================================================================
//    CnxkyzIndexHelperDlg 消息处理程序
// ============================================================================
BOOL CnxkyzIndexHelperDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。 IDM_ABOUTBOX
	// 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CMenu *pSysMenu = GetSystemMenu(FALSE);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (pSysMenu != NULL) {

		//~~~~~~~~~~~~~~~~~
		CString strAboutMenu;
		//~~~~~~~~~~~~~~~~~

		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	// 执行此操?
	SetIcon(m_hIcon, TRUE);		// 设置大图标
	SetIcon(m_hIcon, FALSE);	// 设置小图标

	s_pEditLog = &m_edtLogInfo;

	OleInitialize(NULL);

	InitReplaceTable();

	// TODO: 在此添加额外的初始化代码
	return TRUE;				// 除非将焦点设置到控件，否则返回 TRUE
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {

		//~~~~~~~~~~~~~~~
		CAboutDlg dlgAbout;
		//~~~~~~~~~~~~~~~

		dlgAbout.DoModal();
	} else {
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ============================================================================
//    如果向对话框添加最小化按钮，则需要下面的代码
//    来绘制该图标。对于使用文档/视图模型的 MFC 应用程序， 这将由框架自动完成。
// ============================================================================
void CnxkyzIndexHelperDlg::OnPaint()
{
	if (IsIconic()) {

		//~~~~~~~~~~~~~~
		CPaintDC dc(this);	// 用于绘制的设备上下文
		//~~~~~~~~~~~~~~

		SendMessage(WM_ICONERASEBKGND,
					reinterpret_cast<WPARAM> (dc.GetSafeHdc()), 0);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		GetClientRect(&rect);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialog::OnPaint();
	}
}

// ============================================================================
//    当用户拖动最小化窗口时系统调用此函数取得光标 显示。
// ============================================================================
HCURSOR CnxkyzIndexHelperDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

struct BOOK_INFO
{
	std::vector<int> m_vecVolume;
	int m_nBoard;
	int m_nTitleId;
	int m_nColor;
	char m_cFirst;
	std::string m_strVolumeShow;
	std::string m_strName;
	std::string m_strExtra;
	std::string m_strSubTitle;
	std::string m_strAuthor;
	std::string m_strOther;
	std::string m_strInfo;
	std::string m_strUrl;
	std::string m_strColor;
	std::string m_strSort;
	int m_nVolumeSort;

	BOOK_INFO()
	:
	m_cFirst(0),
	m_nColor(0) {
	};

	// ========================================================================
	// ==========================================================================

	void Color(int nColor)
	{
		m_nColor = nColor;

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::string str = (nColor == 1) ? "[color=blue]" : "[color=red]";
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		str += m_strInfo;
		str += "[/color]";
		m_strInfo = str;
	}

	// ========================================================================
	// ==========================================================================
	void Format(void)
	{
		m_strSort = GetSortUseSpell(m_strName);
		std::sort(m_vecVolume.begin(), m_vecVolume.end());
		if (m_vecVolume.empty()) {
			m_nVolumeSort = 0;
		} else {
			m_nVolumeSort = *m_vecVolume.begin();
		}

		DownStr(m_strSort);

		if (!m_cFirst && !m_strSort.empty()) {
			m_cFirst = m_strSort[0] + 'A' - 'a';
		}

		ReplaceStdString(m_strInfo, "<br />", "");
		ReplaceStdString(m_strUrl, "<br />", "");
		ReplaceStdString(m_strInfo, "\r", "");
		ReplaceStdString(m_strUrl, "\r", "");
		ReplaceStdString(m_strInfo, "\n", "");
		ReplaceStdString(m_strUrl, "\n", "");
	}

	bool operator < (const BOOK_INFO &rBookInfo)const
	{
		if (m_cFirst < rBookInfo.m_cFirst) {
			return true;
		}
		if (m_cFirst > rBookInfo.m_cFirst) {
			return false;
		}
		if (m_strSort < rBookInfo.m_strSort) {
			return true;
		}
		if (m_strSort > rBookInfo.m_strSort) {
			return false;
		}
		return m_nVolumeSort < rBookInfo.m_nVolumeSort;
	}
};

static char s_szFilePath[_MAX_PATH] = { 0 };
std::vector<BOOK_INFO> s_vecDownloadBookInfo;
std::vector<std::string> s_vecRidTable;
std::vector<std::string> s_vecExtraTable;
std::map<std::string, std::string> s_mapReplaceTable;
static const int s_nMaxBufLen = 1024;
static const char *s_pszConfigFile = "./config.ini";
static bool s_bDebug =
GetPrivateProfileInt("GlobalSet", "DebugMode", 0, s_pszConfigFile)
? true : false;

	// ========================================================================
	// ==========================================================================
	void LogInfo(int nStrId, ...)
{
}

// ============================================================================
// ==============================================================================
void LogInfoIn(const char *pszFormat, ...)
{
	//~~~~~~~~~~~~~~~~~~~~
	static CString cstrData;
	std::string strLine;
	va_list args;
	int len;
	char *buffer;
	//~~~~~~~~~~~~~~~~~~~~

	va_start(args, pszFormat);
	len = _vscprintf(pszFormat, args) + 1;	// _vscprintf doesn't count

	// terminating '\0'
	buffer = static_cast<char *>(malloc(len * sizeof(char)));
	vsprintf_s(buffer, len, pszFormat, args);

	strLine = buffer;
	free(buffer);

	strLine += "\r\n";
	cstrData += strLine.c_str();

	if (NULL == s_pEditLog) {
		return;
	}

	s_pEditLog->SetWindowText(cstrData.GetBuffer(0));
	s_pEditLog->UpdateWindow();
	s_pEditLog->LineScroll(s_pEditLog->GetLineCount());
}

// ============================================================================
// ==============================================================================
void LogFile(const char *pszFormat, ...)
{
	//~~~~~~~~~~~~~~~~~~~~
	static CString cstrData;
	std::string strLine;
	va_list args;
	int len;
	char *buffer;
	//~~~~~~~~~~~~~~~~~~~~

	va_start(args, pszFormat);
	len = _vscprintf(pszFormat, args) + 1;	// _vscprintf doesn't count

	// terminating '\0'
	buffer = static_cast<char *>(malloc(len * sizeof(char)));
	vsprintf_s(buffer, len, pszFormat, args);

	strLine = buffer;
	free(buffer);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szLogFile[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("GlobalSet", "LogFile", "", szLogFile,
							sizeof(szLogFile), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szLogFile, "w+");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		return;
	}

	fprintf(pFile, "%s\n", strLine.c_str());
	fclose(pFile);
}

// ============================================================================
// ==============================================================================
std::string MyTrim(char sz[])
{
	//~~~~~~~~~~~~~~
	CString cstr = sz;
	//~~~~~~~~~~~~~~

	cstr.Trim();
	strcpy(sz, cstr.GetBuffer(0));
	return sz;
}

// ============================================================================
// ==============================================================================
std::string MyTrim(std::string &str)
{
	//~~~~~~~~~~~~~~~~~~~~~~~
	CString cstr = str.c_str();
	//~~~~~~~~~~~~~~~~~~~~~~~

	cstr.Trim();
	str = cstr.GetBuffer(0);
	return str;
}

// ============================================================================
// ==============================================================================
bool InitRidTable(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szRidFile[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileStringA("GlobalSet", "RidFile", "", szRidFile,
							 sizeof(szRidFile), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szRidFile, "r");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		LogInfo(IDS_READFILE_FAIL, szRidFile);
		return false;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szBuf[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	while (fgets(szBuf, sizeof(szBuf), pFile)) {

		//~~~~~~~~~~~~~~~~~~~~~
		CString cstr = _T(szBuf);
		//~~~~~~~~~~~~~~~~~~~~~

		cstr.Trim();
		if (cstr == _T("")) {
			continue;
		}

		s_vecRidTable.push_back(cstr.GetBuffer(0));
	}

	return true;
}

// ============================================================================
// ==============================================================================
bool InitReplaceTable(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szReplaceFile[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileStringA("GlobalSet", "ReplaceFile", "", szReplaceFile,
							 sizeof(szReplaceFile), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szReplaceFile, "r");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		LogInfo(IDS_READFILE_FAIL, szReplaceFile);
		return false;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szBuf[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	while (fgets(szBuf, sizeof(szBuf), pFile)) {

		//~~~~~~~~~~~~~~~~~~~~~
		CString cstr = _T(szBuf);
		//~~~~~~~~~~~~~~~~~~~~~

		cstr.Trim();
		if (cstr == _T("")) {
			continue;
		}

		cstr.Replace("\\n", "\n");

		//~~~~~~~~~~~~~~~~~~~~~
		char szSrc[s_nMaxBufLen];
		char szDst[s_nMaxBufLen];
		//~~~~~~~~~~~~~~~~~~~~~

		if (2 == sscanf(cstr.GetBuffer(0), "%s%s", szSrc, szDst)) {
			s_mapReplaceTable[szSrc] = szDst;
		} else if (1 == sscanf(cstr.GetBuffer(0), "%s", szSrc)) {
			s_mapReplaceTable[szSrc] = "";
		}
	}

	return true;
}

// ============================================================================
// ==============================================================================
bool InitExtraTable(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szExtraFile[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileStringA("GlobalSet", "ExtraFile", "", szExtraFile,
							 sizeof(szExtraFile), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szExtraFile, "r");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		LogInfo(IDS_READFILE_FAIL, szExtraFile);
		return false;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szBuf[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	while (fgets(szBuf, sizeof(szBuf), pFile)) {

		//~~~~~~~~~~~~~~~~~~~~~
		CString cstr = _T(szBuf);
		//~~~~~~~~~~~~~~~~~~~~~

		cstr.Trim();
		if (cstr == _T("")) {
			continue;
		}

		//~~~~~~~~~~~~~~~~~~~~~
		char szTmp[s_nMaxBufLen];
		//~~~~~~~~~~~~~~~~~~~~~

		if (1 == sscanf(cstr.GetBuffer(0), "%s", szTmp)) {
			s_vecExtraTable.push_back(szTmp);
		}
	}

	return true;
}

// ============================================================================
// ==============================================================================
bool InitTables(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~
	static bool s_bInit = false;
	//~~~~~~~~~~~~~~~~~~~~~~~~

	if (!s_bInit) {
		InitRidTable();
		InitReplaceTable();
		InitExtraTable();
		s_bInit = true;
	}

	return true;
}

// ============================================================================
// ==============================================================================
void ReplaceStdString(std::string &str,
					  const std::string &strSrc,
					  const std::string &strDest)
{
	//~~~~~~~~~~~~~~~~~~~~~~~
	CString cstr = str.c_str();
	//~~~~~~~~~~~~~~~~~~~~~~~

	cstr.Replace(strSrc.c_str(), strDest.c_str());

	str = cstr.GetBuffer(0);
}

// ============================================================================
// ==============================================================================
void RidChineseSpell(std::string &strInput)
{
	ReplaceStdString(strInput, "ɑ", "a");
	ReplaceStdString(strInput, "ā", "a");
	ReplaceStdString(strInput, "á", "a");
	ReplaceStdString(strInput, "ǎ", "a");
	ReplaceStdString(strInput, "à", "a");
	ReplaceStdString(strInput, "ō", "o");
	ReplaceStdString(strInput, "ó", "o");
	ReplaceStdString(strInput, "ǒ", "o");
	ReplaceStdString(strInput, "ò", "o");
	ReplaceStdString(strInput, "ē", "e");
	ReplaceStdString(strInput, "é", "e");
	ReplaceStdString(strInput, "ě", "e");
	ReplaceStdString(strInput, "è", "e");
	ReplaceStdString(strInput, "ī", "i");
	ReplaceStdString(strInput, "í", "i");
	ReplaceStdString(strInput, "ǐ", "i");
	ReplaceStdString(strInput, "ì", "i");
	ReplaceStdString(strInput, "ū", "u");
	ReplaceStdString(strInput, "ú", "u");
	ReplaceStdString(strInput, "ǔ", "u");
	ReplaceStdString(strInput, "ù", "u");
	ReplaceStdString(strInput, "ǖ", "v");
	ReplaceStdString(strInput, "ǘ", "v");
	ReplaceStdString(strInput, "ǚ", "v");
	ReplaceStdString(strInput, "ǜ", "v");
	ReplaceStdString(strInput, "ü", "v");
	ReplaceStdString(strInput, "ɡ", "g");
}

// ============================================================================
// ==============================================================================
void ReplaceOne(std::string &str)
{
	std::map < std::string, std::string >::const_reverse_iterator it(s_mapReplaceTable.rbegin());
	for (; it != s_mapReplaceTable.rend(); ++it) {
		ReplaceStdString(str, it->first, it->second);
	}
}

// ============================================================================
// ==============================================================================
void ProcessExtraInfo(std::string &strSrc, std::string &strExtra)
{
	std::vector < std::string >::const_iterator it(s_vecExtraTable.begin());
	for (; it != s_vecExtraTable.end(); ++it) {
		if (strstr(strSrc.c_str(), it->c_str())) {
			ReplaceStdString(strSrc, *it, "");
			strExtra += *it;
		}
	}
}

// ============================================================================
// ==============================================================================
bool ProcessHalfResultNoFormat(std::string strBookInfo, std::string strUrl)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	BOOK_INFO infoBook;
	char szName[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	infoBook.m_strInfo = strBookInfo;
	ReplaceStdString(strBookInfo, "\t", " ");
	ReplaceStdString(strBookInfo, "\n", "");
	ReplaceStdString(strBookInfo, "\r", "");
	ReplaceOne(strBookInfo);
	ProcessExtraInfo(strBookInfo, infoBook.m_strExtra);
	ReplaceStdString(strBookInfo, "[", " ");
	ReplaceStdString(strBookInfo, "]", " ");
	if (1 != sscanf(strBookInfo.c_str(), "%s", szName)) {
		return false;
	}

	infoBook.m_strName = szName;
	ReplaceStdString(strBookInfo, szName, "");

	infoBook.m_strOther = strBookInfo;
	infoBook.m_strUrl = strUrl;
	s_vecDownloadBookInfo.push_back(infoBook);

	return true;
}

enum OUTPUT_BOOKINFO_TYPE { OUTPUT_BOOKINFO_ALL, OUTPUT_BOOKINFO_FORINDEX, };

// ============================================================================
// ==============================================================================
bool OutputBookInfo(const std::vector<BOOK_INFO> &vecBookInfo,
					const char *pszType,
					OUTPUT_BOOKINFO_TYPE nType = OUTPUT_BOOKINFO_ALL)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szBookInfoFile[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString(pszType, "BookInfoFile", "", szBookInfoFile,
							sizeof(szBookInfoFile), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szOutPutIndexFormat[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("Merge", "IndexOutputFormat", "",
							szOutPutIndexFormat, sizeof(szOutPutIndexFormat),
							s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string strOutputIndexFormat = szOutPutIndexFormat;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ReplaceStdString(strOutputIndexFormat, "\\n", "\n");

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szBookInfoFile, "w+");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		LogInfo(IDS_READFILE_FAIL, szBookInfoFile);
		return false;
	}

	//~~~~~~~~~~~~~
	char cLast = '0';
	//~~~~~~~~~~~~~

	std::vector<BOOK_INFO>::const_iterator it(vecBookInfo.begin());
	for (; it != vecBookInfo.end(); ++it) {
		if (it->m_cFirst != cLast) {
			fprintf(pFile, "\n\n------------%c----------\n\n", it->m_cFirst);
			cLast = it->m_cFirst;
		}

		//~~~~~~~~~
		CString cstr;
		//~~~~~~~~~

		switch (nType) {
		case OUTPUT_BOOKINFO_FORINDEX:
			cstr.Format(strOutputIndexFormat.c_str(), it->m_strInfo.c_str(),
						it->m_strUrl.c_str());
			break;
		case OUTPUT_BOOKINFO_ALL:
		default:
			cstr.Format("链接：%s\n书名：%s\n作者：%s\n卷标：%s\n附注：%s\n其他：%s\n未分解前的数据：%s\n\n",
					it->m_strUrl.c_str(), it->m_strName.c_str(),
						it->m_strAuthor.c_str(), it->m_strSubTitle.c_str(),
						it->m_strExtra.c_str(), it->m_strOther.c_str(),
						it->m_strInfo.c_str());
			break;
		}

		fputs(cstr.GetBuffer(0), pFile);
	}

	fclose(pFile);
	return true;
}

// ============================================================================
// ==============================================================================
bool ReadDownloadOutputFile(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(s_szFilePath, "r");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		LogInfo(IDS_READFILE_FAIL, s_szFilePath);
		return false;
	}

	s_vecDownloadBookInfo.clear();

	//~~~~~~~~~~~~~~~~~~~~~
	int nLine = 1;
	char szBuf[s_nMaxBufLen];
	std::string strBookInfo;
	//~~~~~~~~~~~~~~~~~~~~~

	for (; fgets(szBuf, sizeof(szBuf), pFile); ++nLine) {
		MyTrim(szBuf);
		if (strstr(szBuf, "http://")) {
			ProcessHalfResultNoFormat(strBookInfo, szBuf);
		}

		strBookInfo = szBuf;
	}

	return true;
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnBnClickedMainBtnOpen()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	CFileDialog dlgOpenFile(TRUE);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (dlgOpenFile.DoModal() != IDOK) {
		return;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CString cstrFilePath = dlgOpenFile.GetFileName();
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	strncpy(s_szFilePath, cstrFilePath.GetBuffer(0), _MAX_PATH - 1);
	InitTables();
	ReadDownloadOutputFile();
	OutputBookInfo(s_vecDownloadBookInfo, "Download");
}

std::vector<BOOK_INFO> s_vecBookInfoOldIndex;

// ============================================================================
// ==============================================================================
bool ProcessHalfResultFormated(std::string strBookInfo,
							   std::string strUrl,
							   char cFirst)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	BOOK_INFO infoBook;
	char szName[s_nMaxBufLen] = { 0 };
	int nPos = strBookInfo.find(']');
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (nPos >= 11) {
		infoBook.m_nColor = 1;
	} else {
		infoBook.m_nColor = 2;
	}

	infoBook.m_strInfo = strBookInfo;
	ReplaceStdString(strBookInfo, "\t", " ");
	ReplaceStdString(strBookInfo, "\n", "");
	ReplaceStdString(strBookInfo, "\r", "");
	ReplaceOne(strBookInfo);
	ProcessExtraInfo(strBookInfo, infoBook.m_strExtra);
	ReplaceStdString(strBookInfo, "[", " ");
	ReplaceStdString(strBookInfo, "]", " ");
	if (1 != sscanf(strBookInfo.c_str(), "%s", szName)) {
		return false;
	}

	infoBook.m_strName = szName;
	ReplaceStdString(strBookInfo, szName, "");

	infoBook.m_strOther = strBookInfo;
	infoBook.m_strUrl = strUrl;
	infoBook.m_cFirst = cFirst;
	s_vecBookInfoOldIndex.push_back(infoBook);

	return true;
}

// ============================================================================
// ==============================================================================
bool ProcessOrgIndex(const char *pszFile, int *pMaxId = NULL)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFileIn = fopen(pszFile, "r");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFileIn) {
		LogInfo(IDS_READFILE_FAIL, pszFile);
		return false;
	}

	InitTables();
	s_vecBookInfoOldIndex.clear();

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int nLine = 1;
	bool bText = false;
	char cFirst;
	char szBuf[s_nMaxBufLen];
	std::string strBookInfo;
	std::string strUrl;
	char szTmpFileName[_MAX_PATH] = { 0 };
	char szEndRidBeginStr[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("OrgIndex", "TmpFile", "", szTmpFileName,
							sizeof(szTmpFileName), s_pszConfigFile);
	GetPrivateProfileString("OrgIndex", "EndRidBeginStr",
							"</div></div></td></tr></table></div>", szEndRidBeginStr,
							sizeof(szEndRidBeginStr), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFileOut = fopen(szTmpFileName, "w");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFileIn) {
		LogInfo(IDS_READFILE_FAIL, szTmpFileName);
		return false;
	}

	for (; fgets(szBuf, sizeof(szBuf), pFileIn); ++nLine) {
		MyTrim(szBuf);

		//~~~~~~~~~~~~~~~~~~~~~~~
		std::string strTmp = szBuf;
		//~~~~~~~~~~~~~~~~~~~~~~~

		UpStr(strTmp);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		const char *pPos = strstr(strTmp.c_str(), "[B][SIZE=");
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (pPos) {
			bText = true;
			pPos = strstr(pPos, "[/SIZE]");
			if (pPos) {
				--pPos;
				cFirst = *pPos;
				fprintf(pFileOut, "\n\n------------%c----------\n\n", cFirst);
			}

			continue;
		}

		pPos = strstr(szBuf, "<!--");
		if (pPos) {
			bText = false;
			continue;
		}

		if (!bText) {
			continue;
		}

		if (strstr(szBuf, "http://")) {

			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			char *pRidBegin = strstr(szBuf, szEndRidBeginStr);
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			if (pRidBegin) {
				*pRidBegin = 0;
			}

			strUrl = szBuf;
			ReplaceStdString(strBookInfo, "<br>", "");
			ReplaceStdString(strUrl, "<br>", "");
			ReplaceStdString(strUrl, "&nbsp;", "");
			ReplaceStdString(strUrl, "amp;", "");
			fprintf(pFileOut, "%s\n", strBookInfo.c_str());
			fprintf(pFileOut, "%s\n", strUrl.c_str());
			ProcessHalfResultFormated(strBookInfo, strUrl, cFirst);
		}

		strBookInfo = szBuf;
	}

	fclose(pFileIn);
	fclose(pFileOut);
	OutputBookInfo(s_vecBookInfoOldIndex, "OrgIndex");

	// Check tmp file
	pFileIn = fopen(szTmpFileName, "r");
	if (NULL == pFileIn) {
		return false;
	}

	GetPrivateProfileString("OrgIndex", "TmpCheckFile", "", szTmpFileName,
							sizeof(szTmpFileName), s_pszConfigFile);
	pFileOut = fopen(szTmpFileName, "w");
	if (NULL == pFileOut) {
		return false;
	}

	//~~~~~~~~~~~~~~~~
	bool bTitle = false;
	bool bUrl = false;
	//~~~~~~~~~~~~~~~~

	for (nLine = 1; fgets(szBuf, sizeof(szBuf), pFileIn); ++nLine) {
		MyTrim(szBuf);
		if (!szBuf[0] || szBuf[0] == '\n' || szBuf[0] == '\r') {
			continue;
		}

		if (strstr(szBuf, "------------")) {
			bTitle = false;
			bUrl = false;
			continue;
		}

		if (strstr(szBuf, "http://")) {
			if (pMaxId) {

				//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
				const char *pszUrlFormat = "http://www.nxkyz.com/bbs/a/a.asp?B=331&ID=";
				char *pPos = strstr(szBuf, pszUrlFormat);
				//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

				if (pPos) {

					//~~~~
					int nId;
					//~~~~

					sscanf(pPos + strlen(pszUrlFormat), "%d", &nId);
					if (nId > *pMaxId) {
						*pMaxId = nId;
					}
				}
			}

			if (bUrl) {
				fprintf(pFileOut,
						"Line %d\n%s\n错误: 连续两行url,无法解析(标题请勿与url置于同一行)\n\n",
					nLine, szBuf);
				bTitle = false;
				continue;
			}

			bUrl = true;
			bTitle = false;
			continue;
		}

		bUrl = false;
		if (strstr(szBuf, "[COLOR=") || strstr(szBuf, "[color=")) {
			if (bTitle) {
				fprintf(pFileOut,
						"Line %d\n%s\n错误: 连续两行着色信息，标题请独占一行\n\n",
					nLine, szBuf);
				continue;
			}

			bTitle = true;
			continue;
		}

		fprintf(pFileOut,
				"Line %d\n%s\n错误: 未着色的非url行，标题请加上颜色\n\n", nLine,
				szBuf);
	}

	fclose(pFileIn);
	fclose(pFileOut);

	LogInfoIn("已输出检查结果到 %s", szTmpFileName);
	return true;
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnBnClickedMainBtnOpenHtm()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	CFileDialog dlgOpenFile(TRUE);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (dlgOpenFile.DoModal() != IDOK) {
		return;
	}

	ProcessOrgIndex(dlgOpenFile.GetFileName().GetBuffer(0));
}

// ============================================================================
// ==============================================================================
bool GetHtmlData(const char *pszUrl, std::vector<std::string> &vecStringData)
{
	vecStringData.clear();

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CInternetSession mySession(NULL, 0);	// 建立会话
	CHttpFile *myHttpFile = NULL;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	LogInfoIn("连接到Url %s...", pszUrl);

	//~~~~~~~~~~~
	// 将网页内容的源代码读至编辑框
	CString myData;
	//~~~~~~~~~~~

	myHttpFile = (CHttpFile *)mySession.OpenURL(pszUrl);

	if (NULL == myHttpFile) {
		LogInfoIn("连接失败");
		return false;
	}

	LogInfoIn("连接成功，读取中...");

	while (myHttpFile->ReadString(myData)) {
		vecStringData.push_back(myData.GetBuffer(0));
//		vecStringData.push_back(Big2GB(myData.GetBuffer(0)));
	}

	LogInfoIn("读取完成");

	myHttpFile->Close();
	mySession.Close();
	return true;
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnBnClickedMainBtnDownIndexCheck()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTmp[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("OrgIndex", "OrgIndexUrl", "", szTmp, sizeof(szTmp),
							s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<std::string> vecIndexPageData;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (!GetHtmlData(szTmp, vecIndexPageData)) {
		return;
	}

	GetPrivateProfileString("OrgIndex", "OrgIndexCodeSav", "", szTmp,
							sizeof(szTmp), s_pszConfigFile);

	LogInfoIn("开始将读取数据保存到中间文件 %s ..", szTmp);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFileTmp = fopen(szTmp, "w");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFileTmp) {
		return;
	}

	std::vector < std::string >::const_iterator it(vecIndexPageData.begin());
	for (; it != vecIndexPageData.end(); ++it) {
		fprintf(pFileTmp, "%s\n", it->c_str());
	}

	fclose(pFileTmp);

	LogInfoIn("保存完成，开始粗略解析");
}

struct TITLE_INFO
{
	std::string m_strTitle;
	int m_nId;
	int m_nSubBoard;
};

std::vector<TITLE_INFO> s_vecTitleInfo;

// ============================================================================
// ==============================================================================

void ProcessHtmlPageInfo(const std::vector<std::string> vecLineData)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	static int s_nIdIndex = GetPrivateProfileInt("TitleUrl", "TitleIdIndex", 2,
												 s_pszConfigFile);
	static int s_nTitleIndex = GetPrivateProfileInt("TitleUrl", "TitleIndex", 4,
													s_pszConfigFile);
	static int s_nSubBoardIndex = GetPrivateProfileInt("TitleUrl",
													   "SubBoardIndex", 15,
													   s_pszConfigFile);
	char szExpression[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "TitleInfo", "", szExpression,
							sizeof(szExpression), s_pszConfigFile);

	boost::regex expression(szExpression);

	//~~~~~~~~~~~~~~~
	boost::cmatch what;
	//~~~~~~~~~~~~~~~

	std::vector < std::string >::const_iterator itLine(vecLineData.begin());
	for (; itLine != vecLineData.end(); ++itLine) {

		//~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::string strLine = *itLine;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (boost::regex_match(strLine.c_str(), what, expression)) {

			//~~~~~~~~~~~~~~~~~
			TITLE_INFO infoTitle;
			//~~~~~~~~~~~~~~~~~

			// infoTitle.m_strTitle = what[s_nTitleIndex].str();
			sscanf(what[s_nIdIndex].str().c_str(), "%d", &infoTitle.m_nId);
			infoTitle.m_nSubBoard = 331;

			// sscanf(what[s_nSubBoardIndex].str().c_str(), "%d", &infoTitle.m_nSubBoard);
			s_vecTitleInfo.push_back(infoTitle);
		}
	}
}

// ============================================================================
// ==============================================================================
bool OutputTitleUrlInfo(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTitleUrlInfo[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "TmpFile", "Title_Url.tmp.txt",
							szTitleUrlInfo, sizeof(szTitleUrlInfo),
							s_pszConfigFile);

	LogInfoIn("开始将标题和Url信息输出到%s", szTitleUrlInfo);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szTitleUrlInfo, "w");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		return false;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTitleUrlFormat[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "ArtUrlFormat",
							"http://www.nxkyz.com/bbs/a/a.asp?B=%d&ID=%d", szTitleUrlFormat,
							sizeof(szTitleUrlFormat), s_pszConfigFile);

	std::vector<TITLE_INFO>::const_iterator it(s_vecTitleInfo.begin());
	for (; it != s_vecTitleInfo.end(); ++it) {

		//~~~~~~~~~~~~
		CString cstrUrl;
		//~~~~~~~~~~~~

		cstrUrl.Format(szTitleUrlFormat, it->m_nSubBoard, it->m_nId);

		//~~~~~~~~~
		CString cstr;
		//~~~~~~~~~

		cstr.Format("标题名: %s\n 链接: %s\n\n", it->m_strTitle.c_str(),
					cstrUrl.GetBuffer(0));
		fputs(cstr.GetBuffer(0), pFile);
	}

	LogInfoIn("标题和Url信息已输出到%s", szTitleUrlInfo);
	fclose(pFile);
	return true;
}

// ============================================================================
// ==============================================================================
bool RidSavedTitleInfo(std::vector<TITLE_INFO> &rVec, int nId)
{
	std::vector<TITLE_INFO>::iterator it(rVec.begin());
	for (; it != rVec.end();) {
		if (it->m_nId < nId) {
			it = rVec.erase(it);
			continue;
		}

		++it;
	}

	return true;
}

// ============================================================================
// ==============================================================================
bool GetUrlTitle(std::vector<TITLE_INFO> &rVec)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTitleUrlFormat[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "ArtUrlFormat",
							"http://www.nxkyz.com/bbs/a/a.asp?B=%d&ID=%d", szTitleUrlFormat,
							sizeof(szTitleUrlFormat), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szExp[s_nMaxBufLen] = "	<meta name=\"description\" content=\"(.*?)\" />";
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	boost::regex expression(szExp);

	//~~~~~~~~~~~~~~~~~~~~~
	boost::cmatch what;
	int nTotal = rVec.size();
	int nCount = 0;
	//~~~~~~~~~~~~~~~~~~~~~

	std::vector<TITLE_INFO>::iterator it(rVec.begin());
	for (; it != rVec.end(); ++it) {
		LogInfoIn("正在获取完整标题信息 %d/%d", ++nCount, nTotal);

		//~~~~~~~~~~~~
		CString cstrUrl;
		//~~~~~~~~~~~~

		cstrUrl.Format(szTitleUrlFormat, it->m_nSubBoard, it->m_nId);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		CInternetSession mySession(NULL, 0);	// 建立会话
		CHttpFile *myHttpFile = NULL;
		// 将网页内容的源代码读至编辑框
		CString myData;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		myHttpFile = (CHttpFile *)mySession.OpenURL(cstrUrl.GetBuffer(0));

		if (NULL == myHttpFile) {
			continue;
		}

		while (myHttpFile->ReadString(myData)) {
			if (boost::regex_match(myData.GetBuffer(0), what, expression)) {
				it->m_strTitle = Big2GB(what[1].str());
				ReplaceStdString(it->m_strTitle, " 有害书籍同好会", "");

				myHttpFile->Close();
				mySession.Close();
				break;
			}
		}
	}

	return true;
}

// ============================================================================
// ==============================================================================
bool DownloadTitleUrl(int nIndexBegin)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szPageUrlFormat[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "PageUrlFormat", "", szPageUrlFormat,
							sizeof(szPageUrlFormat), s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szPageUrl[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	_snprintf(szPageUrl, s_nMaxBufLen - 1, szPageUrlFormat, 0);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<std::string> vecLineData;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (!GetHtmlData(szPageUrl, vecLineData)) {
		return false;
	}

	LogInfoIn("书区第一页下载完成");

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szExpression[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "PageInfo",
							"“(.*?)共([0-9]+)主题 第([0-9]+)/([0-9]+)页 每页([0-9]+)条.*",
							szExpression, sizeof(szExpression), s_pszConfigFile);

	boost::regex expression(szExpression);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	boost::cmatch what;
	int nTitleTotal = 0;
	int nPageNum = 0;
	int nTitlePerPage = 0;
	int nTotalTitleNumIndex = GetPrivateProfileInt("TitleUrl",
												   "TotalTitleNumIndex", 2,
												   s_pszConfigFile);
	int nPageNumIndex = GetPrivateProfileInt("TitleUrl", "PageNumIndex", 4,
											 s_pszConfigFile);
	int nTitlePerPageIndex = GetPrivateProfileInt("TitleUrl",
												  "TitlePerPageIndex", 5,
												  s_pszConfigFile);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	std::vector < std::string >::const_iterator itLine(vecLineData.begin());
	for (; itLine != vecLineData.end(); ++itLine) {

		//~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::string strLine = *itLine;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (boost::regex_match(strLine.c_str(), what, expression)) {
			sscanf(what[nTotalTitleNumIndex].str().c_str(), "%d", &nTitleTotal);
			sscanf(what[nPageNumIndex].str().c_str(), "%d", &nPageNum);
			sscanf(what[nTitlePerPageIndex].str().c_str(), "%d", &nTitlePerPage);
			break;
		}
	}

	//~~~~~~~~~~~~~~~
	int nForcePage = 0;
	//~~~~~~~~~~~~~~~

	nForcePage = GetPrivateProfileInt("TitleUrl", "ForcePage", 0,
									  s_pszConfigFile);

	if (nForcePage) {
		nPageNum = nForcePage;
		LogInfoIn("强制分页数量为%d", nForcePage);
	} else if (nTitleTotal) {
		LogInfoIn("解析分页信息 主题总数 %d 分页数 %d 每页主题数 %d",
				  nTitleTotal, nPageNum, nTitlePerPage);
	} else {
		LogInfoIn("分页信息解析失败");
	}

	GetPrivateProfileString("TitleUrl", "TitleInfo", "", szExpression,
							sizeof(szExpression), s_pszConfigFile);

	s_vecTitleInfo.clear();

	//~~~~~~
	int i = 0;
	//~~~~~~

	for (i = 0; i < nPageNum; ++i) {
		_snprintf(szPageUrl, s_nMaxBufLen - 1, szPageUrlFormat, i);
		if (i > 0) {
			GetHtmlData(szPageUrl, vecLineData);
			LogInfoIn("第%d页标题及链接信息读取完成", i + 1);
		}

		ProcessHtmlPageInfo(vecLineData);
		LogInfoIn("当前取得主题信息数:%d", s_vecTitleInfo.size());
	}

	RidSavedTitleInfo(s_vecTitleInfo, nIndexBegin);
	GetUrlTitle(s_vecTitleInfo);
	OutputTitleUrlInfo();
	return true;
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnBnClickedMainBtnDownloadTitleUrl()
{
	DownloadTitleUrl(51901);
}

// ============================================================================
// ==============================================================================
bool GetIniFileInfo(const char *pszSectionName,
					const char *pszKeyName,
					const char *pszDefault,
					const char *pszFile,
					OUT std::vector<std::string> &vecStrFileInfo)
{
	vecStrFileInfo.clear();

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szFileName[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString(pszSectionName, pszKeyName, pszDefault, szFileName,
							sizeof(szFileName), pszFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	FILE *pFile = fopen(szFileName, "r");
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (NULL == pFile) {
		return false;
	}

	//~~~~~~~~~~~~~~~~~~~~~
	char szBuf[s_nMaxBufLen];
	//~~~~~~~~~~~~~~~~~~~~~

	while (fgets(szBuf, sizeof(szBuf), pFile)) {
		if (szBuf[0] == ';') {
			continue;
		}

		vecStrFileInfo.push_back(szBuf);
	}

	fclose(pFile);
	return true;
}

struct VOLUME_TRANS_INFO
{
	std::string m_strShow;
	int m_nValue;
};

// ============================================================================
// ==============================================================================

bool ProcessVolumeInfo(std::string strVolumeInfo, OUT BOOK_INFO &rInfoBook)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	static bool s_bInitVolumeStrTable = false;
	static std::map<std::string, VOLUME_TRANS_INFO> s_mapVolumeStrTable;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (!s_bInitVolumeStrTable) {

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::vector<std::string> vecTmp;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		GetIniFileInfo("GlobalSet", "VolumeStr", "VolumeStr.in.txt",
					   s_pszConfigFile, vecTmp);
		std::vector < std::string >::const_iterator it(vecTmp.begin());
		for (; it != vecTmp.end(); ++it) {
			boost::regex expression("(.*?)~(.*?)~([0-9]+)(.*)");

			//~~~~~~~~~~~~~~~
			boost::cmatch what;
			//~~~~~~~~~~~~~~~

			if (boost::regex_match(it->c_str(), what, expression)) {

				//~~~~~~~~~~~~~~~~~~~
				VOLUME_TRANS_INFO info;
				//~~~~~~~~~~~~~~~~~~~

				sscanf(what[3].str().c_str(), "%d", &info.m_nValue);
				info.m_strShow = what[2];
				s_mapVolumeStrTable[MyTrim(what[1].str())] = info;
			}
		}
	}

	std::string & rStrShow = rInfoBook.m_strVolumeShow;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<int> &rVecVolume = rInfoBook.m_vecVolume;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	rVecVolume.clear();

	MyTrim(strVolumeInfo);
	std::map < std::string, VOLUME_TRANS_INFO >::const_iterator it(s_mapVolumeStrTable.find(strVolumeInfo));
	if (it != s_mapVolumeStrTable.end()) {
		rStrShow = it->second.m_strShow;
		rVecVolume.push_back(it->second.m_nValue);
		return true;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	boost::char_separator<char> sep("-", "|", boost::keep_empty_tokens);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	tokenizer tokens(strVolumeInfo, sep);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end();
		 ++tok_iter) {

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::string strTmp = *tok_iter;
		int nBegin;
		int nEnd;
		int nNum;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (2 == sscanf(strTmp.c_str(), "%d-%d", &nBegin, &nEnd)) {
			if (nBegin > nEnd) {
				nNum = nBegin;
				nBegin = nEnd;
				nEnd = nNum;
			}

			for (int i = nBegin; i <= nEnd; ++i) {
				rVecVolume.push_back(i);
			}

			continue;
		}

		if (1 == sscanf(strTmp.c_str(), "%d", &nNum)) {
			rVecVolume.push_back(nNum);
			continue;
		}

		rVecVolume.clear();
		rStrShow = "";
		return false;
	}

	if (rInfoBook.m_vecVolume.empty()) {
		rStrShow = "";
		return false;
	}

	rStrShow = strVolumeInfo;
	return true;
}

// ============================================================================
// ==============================================================================
bool ProcessFormatedTitle(const TITLE_INFO &rInfoTitle,
						  OUT BOOK_INFO &rBookInfo)
{
	if (rInfoTitle.m_strTitle == "") {
		return false;
	}

	rBookInfo.m_strInfo = rInfoTitle.m_strTitle;
	rBookInfo.m_nBoard = rInfoTitle.m_nSubBoard;
	rBookInfo.m_nTitleId = rInfoTitle.m_nId;

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTitleUrlFormat[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("TitleUrl", "ArtUrlFormat",
							"http://www.nxkyz.com/bbs/a/a.asp?B=%d&ID=%d", szTitleUrlFormat,
							sizeof(szTitleUrlFormat), s_pszConfigFile);

	//~~~~~~~~~~~~
	CString cstrUrl;
	//~~~~~~~~~~~~

	cstrUrl.Format(szTitleUrlFormat, rBookInfo.m_nBoard, rBookInfo.m_nTitleId);
	rBookInfo.m_strUrl = cstrUrl.GetBuffer(0);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	boost::cmatch what;
	std::string strTitle = rInfoTitle.m_strTitle;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ReplaceOne(strTitle);

	//~~~~~~~~~~~~~~~~~~~~
	boost::regex expression;
	//~~~~~~~~~~~~~~~~~~~~

	expression = "\\[(.*?)\\]\\[(.*?)\\]\\[(.*?)\\](.*?)";
	if (boost::regex_match(strTitle.c_str(), what, expression)
	&& ProcessVolumeInfo(what[2], rBookInfo)) {
		rBookInfo.m_strName = what[1];
		rBookInfo.m_strAuthor = what[3];
		rBookInfo.m_strOther = what[4];
		return true;
	}

	ReplaceStdString(strTitle, "[", " ");
	ReplaceStdString(strTitle, "]", " ");

	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szBookName[s_nMaxBufLen];
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	sscanf(strTitle.c_str(), "%s", szBookName);
	rBookInfo.m_strName = szBookName;

	// rBookInfo.m_strOther = strstr(rInfoTitle.m_strTitle.c_str(),
	// szBookName) + strlen(szBookName);
	rBookInfo.m_strOther = strstr(strTitle.c_str(), szBookName) + strlen(szBookName);
	rBookInfo.m_vecVolume.push_back(1);

	//~~~~~~~~~~~~
	CString cstrLog;
	//~~~~~~~~~~~~

	cstrLog.Format("标题准确解析失败\n%s\n将书名认为%s\n",
				   rInfoTitle.m_strTitle.c_str(), szBookName);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string strLog = cstrLog.GetBuffer();
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// TODO;
	// LogFile(strLog.c_str());
	return false;
}

// ============================================================================
// ==============================================================================
void FormatBookInfo(BOOK_INFO &rInfoBook)
{
	rInfoBook.Format();
}

// ============================================================================
// ==============================================================================
void InsertBookInfo(std::vector<BOOK_INFO> &rVecBookInfo,
					const BOOK_INFO &infoBook)
{
	std::vector<BOOK_INFO>::const_iterator it(rVecBookInfo.begin());
	for (; it != rVecBookInfo.end(); ++it) {

		//~~~~~~~~~~~~~~~~~~~~~~
		const BOOK_INFO tmp = *it;
		//~~~~~~~~~~~~~~~~~~~~~~

		if (infoBook < tmp) {
			rVecBookInfo.insert(it, infoBook);
			return;
		}
	}

	rVecBookInfo.push_back(infoBook);
}

// ============================================================================
// ==============================================================================
void SimpleInsert(std::vector<BOOK_INFO> &rVecBookInfo, BOOK_INFO &infoBook)
{
	//~~~~~~~~~~~~~~~~~~~
	bool bFindSame = false;
	int nColor = 0;
	int nColorLast = 0;
	//~~~~~~~~~~~~~~~~~~~

	std::vector<BOOK_INFO>::const_iterator it(rVecBookInfo.begin());
	for (; it != rVecBookInfo.end(); ++it) {

		//~~~~~~~~~~~~~~~~~~~~~~
		const BOOK_INFO tmp = *it;
		//~~~~~~~~~~~~~~~~~~~~~~

		nColor = tmp.m_nColor;

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		const char *pPos = strstr(tmp.m_strName.c_str(),
								  infoBook.m_strName.c_str());
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (pPos == tmp.m_strName.c_str()) {
			bFindSame = true;
		} else if (bFindSame) {
			infoBook.Color(3 - nColor);
			rVecBookInfo.insert(it, infoBook);
			return;
		} else if (infoBook.m_cFirst < tmp.m_cFirst) {
			infoBook.Color(3 - nColorLast);
			rVecBookInfo.insert(it, infoBook);
			return;
		}

		nColorLast = nColor;
	}

	infoBook.Color(3 - nColor);
	rVecBookInfo.push_back(infoBook);
}

// ============================================================================
// ==============================================================================
bool SimpleMerge(int nMinId)
{
	LogInfoIn("旧有索引项 %d", s_vecBookInfoOldIndex.size());
	LogInfoIn("已下载主题信息数 %d", s_vecTitleInfo.size());
	LogInfoIn("预处理与排序执行中...");

	std::for_each(s_vecBookInfoOldIndex.begin(), s_vecBookInfoOldIndex.end(),
				  FormatBookInfo);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<BOOK_INFO> vecBookAll = s_vecBookInfoOldIndex;
	std::vector<BOOK_INFO> vecBookNew;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	std::vector<TITLE_INFO>::const_iterator it(s_vecTitleInfo.begin());
	for (; it != s_vecTitleInfo.end(); ++it) {
		if (it->m_nId < nMinId) {
			continue;
		}

		//~~~~~~~~~~~~~~~
		BOOK_INFO infoBook;
		//~~~~~~~~~~~~~~~

		if (!ProcessFormatedTitle(*it, infoBook)) {
			continue;
		}

		infoBook.Format();

		vecBookNew.push_back(infoBook);
	}

	std::sort(vecBookNew.begin(), vecBookNew.end());

	std::vector<BOOK_INFO>::iterator itBook(vecBookNew.begin());
	for (; itBook != vecBookNew.end(); ++itBook) {
		SimpleInsert(vecBookAll, *itBook);
	}

	// TODO;
	// std::sort(vecBookAll.begin(), vecBookAll.end());
	OutputBookInfo(s_vecBookInfoOldIndex, "OrgIndex", OUTPUT_BOOKINFO_FORINDEX);
	OutputBookInfo(vecBookAll, "Merge", OUTPUT_BOOKINFO_FORINDEX);
	LogInfoIn("已输出新索引文件");

	return true;
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnBnClickedMainBtnMergeSimple()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTmp[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("OrgIndex", "OrgIndexCodeSav", "", szTmp,
							sizeof(szTmp), s_pszConfigFile);

	if (!s_bDebug || !fopen(szTmp, "r")) {
		GetPrivateProfileString("OrgIndex", "OrgIndexUrl", "", szTmp,
								sizeof(szTmp), s_pszConfigFile);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::vector<std::string> vecIndexPageData;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (!GetHtmlData(szTmp, vecIndexPageData)) {
			return;
		}

		GetPrivateProfileString("OrgIndex", "OrgIndexCodeSav", "", szTmp,
								sizeof(szTmp), s_pszConfigFile);

		LogInfoIn("开始将读取数据保存到中间文件 %s ..", szTmp);

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		FILE *pFileTmp = fopen(szTmp, "w");
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		if (NULL == pFileTmp) {
			return;
		}

		std::vector < std::string >::const_iterator it(vecIndexPageData.begin());
		for (; it != vecIndexPageData.end(); ++it) {
			fprintf(pFileTmp, "%s\n", it->c_str());
		}

		fclose(pFileTmp);

		LogInfoIn("保存完成，开始粗略解析");
	}

	//~~~~~~~~~~~
	int nMaxId = 0;
	//~~~~~~~~~~~

	ProcessOrgIndex(szTmp, &nMaxId);

	LogInfoIn("索引页出现的最大Id %d", nMaxId);

	DownloadTitleUrl(nMaxId);

	LogInfoIn("本版本为粗略解析，忽略在索引贴出现过的url, Id在%d以下的Url",
			  nMaxId);

	SimpleMerge(nMaxId + 1);
}

// ============================================================================
// ==============================================================================
void CnxkyzIndexHelperDlg::OnDestroy()
{
	CDialog::OnDestroy();
	CoUninitialize();
}


int ConvUtf8ToAnsi(CString& strSource, CString& strChAnsi)
{  
	if (strSource.GetLength() <= 0)
		return 0;

	CString strWChUnicode;

	strSource.TrimLeft();
	strSource.TrimRight();   
	strChAnsi.Empty();

	int iLenByWChNeed = MultiByteToWideChar(CP_UTF8, 0,
		strSource.GetBuffer(0),
		strSource.GetLength(), //MultiByteToWideChar
		NULL, 0);

	int iLenByWchDone = MultiByteToWideChar(CP_UTF8, 0,
		strSource.GetBuffer(0),
		strSource.GetLength(),
		(LPWSTR)strWChUnicode.GetBuffer(iLenByWChNeed * 2),
		iLenByWChNeed); //MultiByteToWideChar

	strWChUnicode.ReleaseBuffer(iLenByWchDone * 2);

	int iLenByChNeed  = WideCharToMultiByte(CP_ACP, 0,
		(LPCWSTR)strWChUnicode.GetBuffer(0),
		iLenByWchDone,
		NULL, 0,
		NULL, NULL); 

	int iLenByChDone  = WideCharToMultiByte(CP_ACP, 0,
		(LPCWSTR)strWChUnicode.GetBuffer(0),
		iLenByWchDone,
		strChAnsi.GetBuffer(iLenByChNeed),
		iLenByChNeed,
		NULL, NULL);

	strChAnsi.ReleaseBuffer(iLenByChDone);

	if (iLenByWChNeed != iLenByWchDone || iLenByChNeed != iLenByChDone)
		return 1;

	return 0;   
}

void ConvertUTF8ToANSI(char* strUTF8,CString &strANSI) //   
{   
	int nLen = ::MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(LPCTSTR)strUTF8,-1,NULL,0); 
	//返回需要的unicode长度   
	WCHAR * wszANSI = new WCHAR[nLen+1];   
	memset(wszANSI, 0, nLen * 2 + 2);   
	nLen = MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR)strUTF8, -1, wszANSI, nLen);    //把utf8转成unicode  

	nLen = WideCharToMultiByte(CP_ACP, 0, wszANSI, -1, NULL, 0, NULL, NULL);        //得到要的ansi长度   
	char *szANSI=new char[nLen + 1];   
	memset(szANSI, 0, nLen + 1);   
	WideCharToMultiByte (CP_ACP, 0, wszANSI, -1, szANSI, nLen, NULL,NULL);          //把unicode转成ansi   
	strANSI = szANSI;   
	delete wszANSI;   
	delete szANSI;   
}

std::vector<std::string> s_vecJoke;
int s_nJokeIndex = 0;

void CnxkyzIndexHelperDlg::OnBnClickedButtonGet()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szTmp[_MAX_PATH] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("MXhaha", "url", "", szTmp, sizeof(szTmp),
		s_pszConfigFile);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<std::string> vecHahaPageData;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (!GetHtmlData(szTmp, vecHahaPageData)) {
		return;
	}

	LogInfoIn("下载完成");

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	char szExpression[s_nMaxBufLen] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	GetPrivateProfileString("MXhaha", "DataExp", "",
		szExpression, sizeof(szExpression), s_pszConfigFile);

	boost::regex expression(szExpression);

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	boost::cmatch what;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	s_vecJoke.clear();

	std::vector < std::string >::const_iterator itLine(vecHahaPageData.begin());
	for (; itLine != vecHahaPageData.end(); ++itLine) {

		//~~~~~~~~~~~~~~~~~~~~~~~~~~
		std::string strLine = *itLine;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~	

		if (boost::regex_match(strLine.c_str(), what, expression)) {
			CString cstrUTF8 = what[3].str().c_str();
			CString cstrANSI;
			ConvUtf8ToAnsi(cstrUTF8, cstrANSI);
			std::string strData = cstrANSI;
			ReplaceOne(strData);

			s_vecJoke.push_back(strData);

			LogInfoIn(strData.c_str());

// 			sscanf(what[nTotalTitleNumIndex].str().c_str(), "%d", &nTitleTotal);
// 			sscanf(what[nPageNumIndex].str().c_str(), "%d", &nPageNum);
// 			sscanf(what[nTitlePerPageIndex].str().c_str(), "%d", &nTitlePerPage);
		}
	}

	this->OnBnClickedButtonPre();
}

void CnxkyzIndexHelperDlg::OnBnClickedButtonPre()
{
	--s_nJokeIndex;
	if  (s_nJokeIndex < 0) {
		s_nJokeIndex = 0;
	}

	m_edtJoke.SetWindowText(s_vecJoke.at(s_nJokeIndex).c_str());
	m_edtJoke.UpdateWindow();
}

void CnxkyzIndexHelperDlg::OnBnClickedButtonNext()
{
	++s_nJokeIndex;
	if (s_nJokeIndex >= s_vecJoke.size()) {
		s_nJokeIndex = s_vecJoke.size() - 1;
	}

	m_edtJoke.SetWindowText(s_vecJoke.at(s_nJokeIndex).c_str());
	m_edtJoke.UpdateWindow();
}

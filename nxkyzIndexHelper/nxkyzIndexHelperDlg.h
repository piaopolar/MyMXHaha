// nxkyzIndexHelperDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CnxkyzIndexHelperDlg 对话框
class CnxkyzIndexHelperDlg : public CDialog
{
// 构造
public:
	CnxkyzIndexHelperDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_NXKYZINDEXHELPER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedMainBtnOpen();
	afx_msg void OnBnClickedMainBtnOpenHtm();
	afx_msg void OnBnClickedMainBtnDownIndexCheck();
	CEdit m_edtLogInfo;
	afx_msg void OnBnClickedMainBtnDownloadTitleUrl();
	afx_msg void OnBnClickedMainBtnMergeSimple();
	afx_msg void OnDestroy();
};

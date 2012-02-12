// nxkyzIndexHelperDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CnxkyzIndexHelperDlg �Ի���
class CnxkyzIndexHelperDlg : public CDialog
{
// ����
public:
	CnxkyzIndexHelperDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_NXKYZINDEXHELPER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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

// nxkyzIndexHelper.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CnxkyzIndexHelperApp:
// �йش����ʵ�֣������ nxkyzIndexHelper.cpp
//

class CnxkyzIndexHelperApp : public CWinApp
{
public:
	CnxkyzIndexHelperApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CnxkyzIndexHelperApp theApp;
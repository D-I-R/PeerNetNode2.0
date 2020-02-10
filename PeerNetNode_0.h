
// PeerNetNode.h: главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// основные символы


// CPeerNetNodeApp:
// Сведения о реализации этого класса: PeerNetNode.cpp
//

class CPeerNetNodeApp : public CWinApp
{
public:
	CPeerNetNodeApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern CPeerNetNodeApp theApp;

// PeerNetMain.h                                           (c) DIR, 2019
//
//      Main header file for the PeerNetNode application
//
////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
 #error "включить pch.h до включения этого файла в PCH"
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
  virtual BOOL  InitInstance();
  //virtual BOOL  OnIdle(LONG lCount);

// Реализация

  DECLARE_MESSAGE_MAP()
};

extern CPeerNetNodeApp theApp;

// ListVwEx.h                                              (c) DIR, 2019
//
//         Объявление класса CListViewEx - Расширение CListView
//
// This class provedes a full row selection mode for the report
// mode list view control.
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include <afxcview.h>

class CListViewEx : public CListView
{
// Attributes
protected:
  BOOL      m_bFullRowSel;      //флаг "Выделять строку на всю ширину списка"
  int       m_cxClient;             //client area width
  int       m_cxStateImageOffset;   //state icon width
  // List view colors:
  COLORREF  m_clrText;
  COLORREF  m_clrTextBk;
  COLORREF  m_clrBkgnd;

public:
  BOOL      m_bClientWidthSel;  //флаг "Выделять строку на видимую ширину"

// Construction
  DECLARE_DYNCREATE(CListViewEx)
  CListViewEx();

public:
  BOOL  GetFullRowSel() { return m_bFullRowSel; }
  BOOL  SetFullRowSel(BOOL bFillRowSel);
  int   GetSelectedRowIndex();
  CString & GetSelectedRowField(short iField);

// Overrides
protected:
  virtual void  DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
  virtual void  PostNcDestroy();

// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListViewEx)
public:
  virtual BOOL  PreCreateWindow(CREATESTRUCT &cs);
protected:
	//}}AFX_VIRTUAL

// Implementation
public:
  virtual ~CListViewEx();
#ifdef _DEBUG
  virtual void  Dump(CDumpContext &dc) const;
#endif

protected:
  static LPCTSTR  MakeShortString(CDC *pDC, LPCTSTR lpszLong,
                                  int nColumnLen, int nOffset);
  void  RepaintSelectedItems();

// Generated message map functions
protected:
	//{{AFX_MSG(CListViewEx)
  afx_msg void  OnSize(UINT nType, int cx, int cy);
  afx_msg void  OnPaint();
  afx_msg void  OnSetFocus(CWnd *pOldWnd);
  afx_msg void  OnKillFocus(CWnd *pNewWnd);
	//}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

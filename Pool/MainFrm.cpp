
#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (m_view.PreTranslateMessage(pMsg))
		return TRUE;

	return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);

}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

bool CMainFrame::WriteString(CAtlFile & file, PCWSTR text) {
	return SUCCEEDED(file.Write(text, static_cast<DWORD>((::wcslen(text) + 1)) * sizeof(WCHAR), (DWORD*)nullptr));
}

LRESULT CMainFrame::OnTimer(UINT, WPARAM, LPARAM, BOOL &) {
	CString text;
	text.Format(L"%d Tags", m_view.GetItemCount());
	m_StatusBar.SetPaneText(ID_PANE_ITEMS, text);

	text.Format(L"Paged: %u MB", (unsigned)(m_view.GetTotalPaged() >> 20));
	m_StatusBar.SetPaneText(ID_PANE_PAGED_TOTAL, text);

	text.Format(L"Non Paged: %u MB", (unsigned)(m_view.GetTotalNonPaged() >> 20));
	m_StatusBar.SetPaneText(ID_PANE_NONPAGED_TOTAL, text);

	return 0;
}

void CMainFrame::SetPaneWidths(CMultiPaneStatusBarCtrl& sb, int* arrWidths) {
	int nPanes = sb.m_nPanes;

	int arrBorders[3];
	sb.GetBorders(arrBorders);

	arrWidths[0] += arrBorders[2];
	for (int i = 1; i < nPanes; i++)
		arrWidths[0] += arrWidths[i];
	for (int j = 1; j < nPanes; j++)
		arrWidths[j] += arrBorders[2] + arrWidths[j - 1];
	sb.SetParts(nPanes, arrWidths);
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);

	m_CmdBar.AttachMenu(GetMenu());
	m_CmdBar.LoadImages(IDR_MAINFRAME_SMALL);
	SetMenu(nullptr);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, FALSE);

	CreateSimpleStatusBar(nullptr);

	m_StatusBar.SubclassWindow(m_hWndStatusBar);
	
	int panes[] = {
		/*ID_DEFAULT_PANE,*/ ID_PANE_PAGED_TOTAL, ID_PANE_NONPAGED_TOTAL, ID_PANE_ITEMS
	};
	m_StatusBar.SetPanes(panes, _countof(panes), FALSE);

	int widths[] = { 150, 300, 400 };
	m_StatusBar.SetParts(_countof(panes), widths);

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_OWNERDATA,
		WS_EX_CLIENTEDGE);

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	SetTimer(1, 2000, nullptr);
	return 0;
}

LRESULT CMainFrame::OnSize(UINT, WPARAM, LPARAM, BOOL &) {
	if (IsWindow()) {
		//UpdateLayout();
		RECT rc;
		GetClientRect(&rc);
		m_view.MoveWindow(0, 0, rc.right, rc.bottom);
	}
	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileSave(WORD, WORD, HWND, BOOL &) {
	CSimpleFileDialog dlg(FALSE, L"csv", L"pool.csv", OFN_EXPLORER | OFN_HIDEREADONLY, L"CVS files\0*.csv\0All Files\0*.*\0");
	if (dlg.DoModal() == IDOK) {
		CAtlFile file;

		if (FAILED(file.Create(dlg.m_ofn.lpstrFile, GENERIC_WRITE, 0, CREATE_ALWAYS))) {
			AtlMessageBox(m_hWnd, L"Failed to open file");
			return 0;
		}
		int column = 0;
		TCHAR text[128];
		while (true) {
			LVCOLUMN col;
			col.mask = LVCF_TEXT;
			col.cchTextMax = 128;
			col.pszText = text;
			if (!m_view.GetColumn(column, &col))
				break;
			WriteString(file, col.pszText);
			WriteString(file, L",");
			column++;
		}
		WriteString(file, L"\n");

		int count = m_view.GetItemCount();
		for (int i = 0; i < count; i++) {
			for (int c = 0; c < column; c++) {
				m_view.GetItemText(i, c, text, _countof(text));
				WriteString(file, text);
				WriteString(file, L",");
			}
			WriteString(file, L"\n");
		}
	}
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}



/************************************************************************
 *
 *  Ye-Kui Wang       wyk@ieee.org
 *  Juan-Juan Jiang   juanjuan_j@hotmail.com
 *  
 *  March 14, 2002
 *
 ************************************************************************/

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis.  The developers disclaim 
 * any and all warranties, whether express, implied, or statuary, including 
 * any implied warranties or merchantability or of fitness for a particular 
 * purpose.  In no event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising from 
 * the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * The developers does not represent or warrant that the programs furnished 
 * hereunder are free of infringement of any third-party patents.
 *
 * */


// YUVviewerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "YUVviewer.h"
#include "ChildWindow.h"
#include "YUVviewerDlg.h"
#include "DECencrypt.h"
#include "RC4encrypt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL g_bPlay;
int g_nFrameNumber = 0;
int g_nOldFrameNumber = -1000; 
BOOL g_Play = true;

int g_nStartFrame = 0;
int g_nEndFrame = 10000;
int g_nCurrentFrame = 0; 
BOOL g_bReversePlay = FALSE;

void getSeqName(char *inseqpath, char *seqname);

//int nImgWidth, nImgHeight;
UINT PlayVideo( LPVOID pParam )
{
	int i;//,j;

	BOOL bPlay = g_bPlay;
	BOOL bEof = FALSE;

	CYUVviewerDlg *pWin = (CYUVviewerDlg *)pParam;
	UINT picsize = pWin->m_nWidth*pWin->m_nHeight;
	int timespan = 1000/atoi(pWin->m_sFrameRate);
	
	if(g_nCurrentFrame < g_nStartFrame) g_nCurrentFrame = g_nStartFrame;
	if(g_nCurrentFrame > g_nEndFrame) g_nCurrentFrame = g_nEndFrame;

	for(i=0; i<pWin->m_iCount; i++)
	{
		pWin->m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		pWin->m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	HANDLE hPlayTemp1 = OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
	
	while(g_nCurrentFrame >= g_nStartFrame && g_nCurrentFrame <= g_nEndFrame && !bEof)
	{
		DWORD t2=GetTickCount();
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		if ( WAIT_OBJECT_0 == WaitForSingleObject(hPlayTemp1,INFINITE) )
			ReleaseMutex( hPlayTemp1 );
		
		for(i=0; i<pWin->m_iCount; i++)
		{
			pWin->m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);

			if(picsize != pWin->m_pFile[i]->Read(pWin->m_pWnd[i]->Y,picsize))
			{
				AfxMessageBox("Get to end of file");
				bEof = TRUE;
				break;
			}
			if(1)//bColorImage) 
			{
				if(picsize/4 != pWin->m_pFile[i]->Read(pWin->m_pWnd[i]->Cb,picsize/4))
				{
					AfxMessageBox("Get to end of file");
					bEof = TRUE;
					break;
				}
				if(picsize/4 != pWin->m_pFile[i]->Read(pWin->m_pWnd[i]->Cr,picsize/4))
				{
					AfxMessageBox("Get to end of file");
					bEof = TRUE;
					break;
				}
			}

			pWin->m_pWnd[i]->InvalidateRect (NULL,FALSE);
			pWin->m_pWnd[i]->UpdateWindow ();
			pWin->m_pWnd[i]->nPicShowOrder=g_nCurrentFrame;
		}

		if(g_bReversePlay == FALSE)
			g_nCurrentFrame++;
		else 
			g_nCurrentFrame--;

		int t1=GetTickCount()-t2;
		if(t1 < timespan) 
			Sleep(timespan - t1); // sleep time in milliseconds
	}
	
	pWin->m_pWinThread = NULL;
	AfxEndThread(0);

	return 1;

}

void CYUVviewerDlg::OnOrder() 
{
	if(g_bReversePlay == FALSE)
	{
		m_buttonOrder.SetWindowText("Forward");
		g_bReversePlay = TRUE;
	}
	else
	{
		m_buttonOrder.SetWindowText("Backward");
		g_bReversePlay = FALSE;
	}
}

void CYUVviewerDlg::OnPauseplay() 
{

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	// create a new thread
	if (m_bPlay)
	{
		m_buttonPausePlay.SetWindowText("Pause");
		m_bPlay = false;
		g_Play = true;
	}
	else
	{
		m_buttonPausePlay.SetWindowText("Play");
		m_bPlay = true;
	}

	char chTitle[10];
	m_buttonPausePlay.GetWindowText(chTitle,10);
	hPlayTemp = NULL;
	hPlayTemp=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
	if ( strcmp( chTitle,"Play" ) == 0 )
	{
		WaitForSingleObject( hPlayTemp,0);
		
	}
	else
		ReleaseMutex(hPlayTemp);

	if ( m_pWinThread == NULL)
		m_pWinThread = AfxBeginThread( (AFX_THREADPROC)PlayVideo , (void*)this);

}

void CYUVviewerDlg::OnCloseall() 
{
	int i;

	for(i=0; i<m_iCount; i++)
	{
		if(m_pFile[i])
			m_pFile[i]->Close();
		if(m_pWnd[i])
			m_pWnd[i]->DestroyWindow();
	}
	for (i = 0; i < m_tmpCnt; i++)
	{
		CFileStatus status;
		if (CFile::GetStatus(inSeqence[m_tmpFile[i]], status))
			CFile::Remove(inSeqence[m_tmpFile[i]]);
	}
	m_tmpCnt = 0;
	m_iCount = 0;

	g_nFrameNumber = 0;
	g_nOldFrameNumber = -1000; 
	g_Play = true;

	g_nStartFrame = 0;
	g_nEndFrame = 10000;
	g_nCurrentFrame = 0; 
	g_bReversePlay = FALSE;
}

void CYUVviewerDlg::OnCancel() 
{
	int i;

	for(i=0; i<m_iCount; i++)
	{
		if(m_pFile[i])
			m_pFile[i]->Close();
		if(m_pWnd[i])
			m_pWnd[i]->DestroyWindow();
	}
	
	CDialog::OnCancel();
}

int CYUVviewerDlg::OnOpenfile() 
{
	UpdateData(TRUE);

	UINT picsize = m_nWidth*m_nHeight;

	m_pFile[m_iCount] = new CFile();

	char BASED_CODE szFilter[] = "All Files (*.*)|*.*||";
	CFileDialog dlg( TRUE, "yuv", NULL, OFN_HIDEREADONLY,szFilter);
  	if(dlg.DoModal()!=IDOK) return 0; 

  sprintf( inSeqence[m_iCount], "%s", dlg.GetPathName() );
  getSeqName(inSeqence[m_iCount], inSeqName[m_iCount]);

	if(!m_pFile[m_iCount]->Open(inSeqence[m_iCount], CFile::modeRead )) 
  {
		AfxMessageBox("Can't open input file");
		return 0;
	}

	//===================================

	m_pWnd[m_iCount]=new CChildWindow((CFrameWnd*)this, m_nWidth, m_nHeight,1);

	if(picsize != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Y,picsize))
	{
		MessageBox("Get to end of file");
		return 0;
	}
	if(1)//bColorImage) 
	{
		if(picsize/4 != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Cb,picsize/4))
		{
			MessageBox("Get to end of file");
			return 0;
		}
		if(picsize/4 != m_pFile[m_iCount]->Read(m_pWnd[m_iCount]->Cr,picsize/4))
		{
			MessageBox("Get to end of file");
			return 0;
		}
	}

	m_pWnd[m_iCount]->ShowWindow(SW_SHOW);

	if(m_nZoom == -1) m_pWnd[m_iCount]->CenterWindow(m_nWidth,m_nHeight);
	else if(m_nZoom == 0) m_pWnd[m_iCount]->CenterWindow(m_nWidth*2,m_nHeight*2);


	m_iCount++;
	
	// create a new thread

//	m_pWinThread[ m_iCountThread ] = AfxBeginThread( (AFX_THREADPROC)PlayVideo , (void*)this);

	
/*	if ( pWnd != NULL )
	{
		delete pWnd;
		pWnd = NULL;
	}
	m_pFile[m_iCountThread]->Close();
*/
	return 1;

}

void CYUVviewerDlg::OnNext() 
{
	int i;
	UINT picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			if(picsize != m_pFile[i]->Read(m_pWnd[i]->Y,picsize))
			{
				MessageBox("Get to end of file");
				return;
			}
			if(1)//bColorImage) 
			{
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}


/*	if ( m_bPlay && hPlayTemp != NULL)
	{
		g_nOldFrameNumber = g_nFrameNumber;
		ReleaseMutex(hPlayTemp);
		g_Play = true;
	}
	WaitForSingleObject( hPlayTemp,INFINITE );
*/
/*	if ( g_nFrameNumber == g_nOldFrameNumber+1)
	{
	//	g_Play = false;
		OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
	}
*/
/*	if ( !g_bPlay )
		OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
*/
}

void CYUVviewerDlg::OnNext5() 
{
	int i;
	UINT picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	g_nCurrentFrame += 4;
	if(g_nCurrentFrame > g_nEndFrame) g_nCurrentFrame = g_nEndFrame;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			if(picsize != m_pFile[i]->Read(m_pWnd[i]->Y,picsize))
			{
				MessageBox("Get to end of file");
				return;
			}
			if(1)//bColorImage) 
			{
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
				if(picsize/4 != m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4))
				{
					MessageBox("Get to end of file");
					return;
				}
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}

/*	if ( m_bPlay && hPlayTemp != NULL)
	{
		g_nOldFrameNumber = g_nFrameNumber;
		ReleaseMutex(hPlayTemp);
//		g_Play = true;
	}
	WaitForSingleObject( hPlayTemp,INFINITE );
	for ( int i=0;i<5;i++)
	{
	//	g_Play = false;
		ReleaseMutex(hPlayTemp);
		WaitForSingleObject( hPlayTemp,INFINITE );
		
	}
	if ( g_nFrameNumber == g_nOldFrameNumber+5 )
	{
		OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play");
	}
*/	
}

void CYUVviewerDlg::OnPrevious() 
{
	int i;
	int picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	g_nCurrentFrame -= 2;
	if(g_nCurrentFrame<0) g_nCurrentFrame = 0;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);
			if(1)//bColorImage) 
			{
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}
}

void CYUVviewerDlg::OnPrevious5() 
{
	int i;
	int picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	g_nCurrentFrame -= 6;
	if(g_nCurrentFrame<0) g_nCurrentFrame = 0;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->Seek(g_nCurrentFrame*picsize*3/2, SEEK_SET);
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);
			if(1)//bColorImage) 
			{
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYUVviewerDlg dialog

CYUVviewerDlg::CYUVviewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CYUVviewerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CYUVviewerDlg)
	m_nFrameFrom = 0;
	m_nFrameTo = 0;
	m_nFrameSize = 1;
	m_nHeight = 144;
	m_nWidth = 176;
	m_sFrameRate = _T("30");
	m_nZoom = 0;
	m_nEncrypt = 0;			//set default as encrypt----0 stands for encrypt
	m_nType = 3;			//set default as all----0~3 corresponding the 4 types
	m_nStgy = 0;
	m_nReso = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CYUVviewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CYUVviewerDlg)
	DDX_Control(pDX, IDC_PREVIOUS5, m_buttonPrev5);
	DDX_Control(pDX, IDC_ORDER, m_buttonOrder);
	DDX_Control(pDX, IDC_NEXT5, m_buttonNext5);
	DDX_Control(pDX, IDC_PREVIOUS, m_buttonPrev);
	DDX_Control(pDX, IDC_PAUSEPLAY, m_buttonPausePlay);
	DDX_Control(pDX, IDC_NEXT, m_buttonNext);
	DDX_Control(pDX, IDC_OPENFILE, m_buttonOpen);
	DDX_Text(pDX, IDC_FRAME_FROM, m_nFrameFrom);
	DDX_Text(pDX, IDC_FRAME_TO, m_nFrameTo);
	DDX_Radio(pDX, IDC_SIZE_CIF, m_nFrameSize);
	DDX_Text(pDX, IDC_SIZE_HEIGHT, m_nHeight);
	DDX_Text(pDX, IDC_SIZE_WIDTH, m_nWidth);
	DDX_CBString(pDX, IDC_FRAME_RATE, m_sFrameRate);
	DDX_Radio(pDX, IDC_ZOOM, m_nZoom);
	DDX_Radio(pDX, IDC_ENCRPT, m_nEncrypt);
	DDX_Radio(pDX, IDC_Y, m_nType);
	DDX_Radio(pDX, IDC_RC4, m_nStgy);
	DDX_Radio(pDX, IDC_BLUR, m_nReso);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CYUVviewerDlg, CDialog)
	//{{AFX_MSG_MAP(CYUVviewerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SIZE_CIF, OnSizeCif)
	ON_BN_CLICKED(IDC_SIZE_QCIF, OnSizeQcif)
	ON_BN_CLICKED(IDC_SIZE_OTHER, OnSizeOther)
	ON_BN_CLICKED(IDC_OPENFILE, OnOpenfile)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PAUSEPLAY, OnPauseplay)
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	ON_BN_CLICKED(IDC_NEXT5, OnNext5)
	ON_BN_CLICKED(IDC_ORDER, OnOrder)
	ON_BN_CLICKED(IDC_PREVIOUS5, OnPrevious5)
	ON_BN_CLICKED(IDCLOSEALL, OnCloseall)
	ON_BN_CLICKED(IDC_TRANSFER, OnTransfer)
	ON_BN_CLICKED(IDC_ZOOM, OnZoom)
	ON_BN_CLICKED(IDC_ENCRPT, OnEncrpt)
	ON_BN_CLICKED(IDC_DECRPT, OnDecrpt)
	ON_BN_CLICKED(IDC_Y, OnY)
	ON_BN_CLICKED(IDC_U, OnU)
	ON_BN_CLICKED(IDC_V, OnV)
	ON_BN_CLICKED(IDC_ALL, OnAll)
	ON_BN_CLICKED(IDC_RC4, OnRC4)
	ON_BN_CLICKED(IDC_DES, OnDES)
	ON_BN_CLICKED(IDC_BLUR, OnBlur)
	ON_BN_CLICKED(IDC_TENSE, OnTense)
	ON_BN_CLICKED(IDC_CRPTOP, OnCrptOpen)
	ON_BN_CLICKED(IDC_CRPTDO, OnCrptdo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYUVviewerDlg message handlers

BOOL CYUVviewerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_tmpCnt = 0;					//the decode file number
	m_iCount = 0;
	m_bPlay = true;
	m_pWinThread = NULL;
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);

	HANDLE hPlay = NULL;
	if( (hPlay=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"Play"))==NULL)
	{
		//如果没有其他进程创建这个互斥量，则重新创建
		hPlay = CreateMutex(NULL,FALSE,"Play");
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CYUVviewerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CYUVviewerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CYUVviewerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CYUVviewerDlg::OnSizeCif() 
{
	UpdateData(TRUE);
	m_nWidth = 352;
	m_nHeight = 288;
	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnSizeQcif() 
{
	UpdateData(TRUE);
	m_nWidth = 176;
	m_nHeight = 144;
	Disable(IDC_SIZE_WIDTH);
	Disable(IDC_SIZE_HEIGHT);
	Disable(IDC_STATIC_H);
	Disable(IDC_STATIC_W);
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnSizeOther() 
{
	Enable(IDC_SIZE_WIDTH);
	Enable(IDC_SIZE_HEIGHT);
	Enable(IDC_STATIC_H);
	Enable(IDC_STATIC_W);
}

void CYUVviewerDlg::Disable(int nID)
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	pObject1->EnableWindow(FALSE);
}
void CYUVviewerDlg::Enable(int nID)
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	pObject1->EnableWindow(TRUE);
}
BOOL CYUVviewerDlg::Enabled(int nID)
{
	CWnd *pObject1;
	pObject1 = GetDlgItem(nID);
	return (pObject1->IsWindowEnabled());
}

void CYUVviewerDlg::OnTransfer() 
{
  // the following code is to set the current displayed picture to the first frame, frame 0.
  // added Jan. 15, 2002.
	int i;
	int picsize = m_nWidth*m_nHeight;

	UpdateData(TRUE);

	g_nStartFrame = m_nFrameFrom = 0;
	if(m_nFrameTo != 0) g_nEndFrame = m_nFrameTo;
	else g_nEndFrame = 10000;

	g_nCurrentFrame = 0;

	for(i=0; i<m_iCount; i++)
	{
		m_pFile[i]->SeekToBegin();
		m_pWnd[i]->nPicShowOrder = g_nCurrentFrame;
	}
	
	if(g_nCurrentFrame < g_nEndFrame) // && !bEof)
	{
		g_nFrameNumber = g_nCurrentFrame;//j;
 
		for(i=0; i<m_iCount; i++)
		{
			m_pFile[i]->Read(m_pWnd[i]->Y,picsize);
			if(1)//bColorImage) 
			{
				m_pFile[i]->Read(m_pWnd[i]->Cb,picsize/4);
				m_pFile[i]->Read(m_pWnd[i]->Cr,picsize/4);
			}
			m_pWnd[i]->InvalidateRect (NULL,FALSE);
			m_pWnd[i]->UpdateWindow ();
			m_pWnd[i]->nPicShowOrder ++;
		}
		g_nCurrentFrame++;
		//Sleep(200); // sleep time in milliseconds
	}
}

void getSeqName(char *inseqpath, char *seqname)
{
  int lastSlashPos, lastDotPos; // the last dot is located after the last slash "\"
  int lastNonZeroPos; // last pos that tmp != 0
  int i=0;
  char tmp = '0';

  while(tmp != 0)
  {
    tmp = inseqpath[i++];
    if(tmp == '\\')
      lastSlashPos = i-1;
    if(tmp == '.')
      lastDotPos = i-1;
  }
  lastNonZeroPos = i-1;

  if(lastDotPos < lastSlashPos)
    lastDotPos = -1; // that means the file name with no extention, such as "c:\seq\forman".

  if(lastDotPos != -1)
  {
    for(i=lastSlashPos+1; i<lastDotPos; i++)
      seqname[i-lastSlashPos-1] = inseqpath[i];
    seqname[lastDotPos-lastSlashPos-1] = 0;
  }
  else
  {
    for(i=lastSlashPos+1; i<lastNonZeroPos+1; i++)
      seqname[i-lastSlashPos-1] = inseqpath[i];
    seqname[lastNonZeroPos-lastSlashPos] = 0;
  }
}

void CYUVviewerDlg::OnZoom() 
{
//	UpdateData(TRUE);
  if(m_nZoom == -1)	m_nZoom = 0;
  else m_nZoom = -1;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnEncrpt() 
{
	UpdateData(TRUE);
	m_nEncrypt = 0;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnDecrpt() 
{
	UpdateData(TRUE);
	m_nEncrypt = 1;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnY() 
{
	UpdateData(TRUE);
	m_nType = 0;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnU() 
{
	UpdateData(TRUE);
	m_nType = 1;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnV() 
{
	UpdateData(TRUE);
	m_nType = 2;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnAll() 
{
	UpdateData(TRUE);
	m_nType = 3;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnBlur() 
{
	UpdateData(TRUE);
	m_nReso = 0;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnTense() 
{
	UpdateData(TRUE);
	m_nReso = 1;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnRC4() 
{
	UpdateData(TRUE);
	m_nStgy = 0;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnDES() 
{
	UpdateData(TRUE);
	m_nStgy = 1;
	UpdateData(FALSE);
}

void CYUVviewerDlg::OnCrptOpen() 
{
	UpdateData(TRUE);

	UINT picsize = m_nWidth*m_nHeight;

	CFile *fp = new CFile();
	CFileDialog dlg(TRUE, "yuv", NULL, OFN_HIDEREADONLY, "YUV Files (*.yuv)|*.yuv||");
	dlg.DoModal();
	
	crptPath = dlg.GetPathName();

	UpdateData(FALSE);

}

void CYUVviewerDlg::OnCrptdo() 
{
	int i, k;

	int py = m_nWidth * m_nHeight;
	int pu = py / 4;
	int pv = pu;
	int fra = py + pu + pv;
	int l, r;
	switch (m_nType)
	{
		case 0: l = 0, r = py; break;
		case 1: l = py, r = py + pu; break;
		case 2: l = py + pu, r = py + pu + pv; break;
		case 3: l = 0, r = py + pu + pv; break;
	}

	Peri = m_nReso ? 1 : 3;

	CString str1, str2;
	GetDlgItemText(IDC_PWD1, str1);
	GetDlgItemText(IDC_PWD2, str2);
	if (str1 != str2)
	{
		MessageBox("Two passwords differ.");
		return;
	}

	if (str1.GetLength() == 0)
		str1 = str1 + '*';

	unsigned char strkey[20];
	for (i = 0; i < 8; i++)
		strkey[i] = str1[i % str1.GetLength()];
	ksa(perm, strkey, 8);

	string skey = "";
	for (i = 0; i < 8; i++)
		skey = skey + str1[i % str1.GetLength()];
	bitset<64> key;
	key = charToBitset(skey.c_str());
	generateKeys(key);
	bitset<64> passage, code;

	CFile fin;
	fin.Open(crptPath, CFile::modeRead);
	if (fin.GetFileName().IsEmpty())
	{
		MessageBox("Can't open file.");
		return;
	}

	char name[50];
	strcpy(name, crptPath);
	char *p = name;
	while (*p != '.')
		p++;
	strcpy(p, m_nEncrypt ? "_decy.tmp" : ".e.yuv");

	if (m_nEncrypt)
	{
		p = name + strlen(name);
		*(p + 1) = 0;
		while (*(p - 1) != '\\')
			*p = *(p - 1), p--;;
		*p = '.';
	}

	CFileStatus status;
	if (CFile::GetStatus(name, status))
		CFile::Remove(name);
	CFile fout;
	fout.Open(name, CFile::modeWrite | CFile::modeCreate);
	if (fout.GetFileName().IsEmpty())
	{
		MessageBox("Can't create file.");
		return;
	}

	//============Encrypt==================
	int cnt = 0;
	int now = 0;
	int p1 = 0, p2 = 0;
	while (true)
	{
		if (l > 0)
		{
			k = fin.Read((void *)buffer, l);
			fout.Write((void *)buffer, k);
			if (k < l)
				break;
		}
		if (m_nStgy == 1)
		{
			for (i = l; i < r; i += 8)
			{
				fin.Read((void *)&passage, 8);
				if (now++ % Peri == 0)
				{
					if (m_nEncrypt == 0)
						code = encrypt(passage);
					else
						code = decrypt(passage);
				}
				fout.Write((void *)&code, 8);
			}
		} else
		{
			k = fin.Read((void *)buffer, r - l);
			for (i = 0; i < k; i++, now++)
			{
				p1 = (p1 + 1) & 255;
				p2 = (p2 + perm[p1]) & 255;
				swap(perm[p1], perm[p2]);
				if (now % Peri == 0)
					buffer[i] ^= perm[(perm[p1] + perm[p2]) & 255];
			}
			fout.Write((void *)buffer, k);
			if (k < r - l)
				break;
		}
		if (r < fra)
		{
			k = fin.Read((void *)buffer, fra - r);
			fout.Write((void *)buffer, k);
			if (k < fra - r)
				break;
		}
		cnt++;
	}
	fin.Close();
	fout.Close();


	if (m_nEncrypt)
	{
		UINT picsize = m_nWidth*m_nHeight;
		sprintf(inSeqence[m_iCount], "%s", name);
		getSeqName(inSeqence[m_iCount], inSeqName[m_iCount]);

		m_pFile[m_iCount] = new CFile();
		if (!m_pFile[m_iCount] -> Open(inSeqence[m_iCount], CFile::modeRead )) 
		{
			AfxMessageBox("Can't open input file");
			return;
		}

		//===================================

		m_pWnd[m_iCount]=new CChildWindow((CFrameWnd*)this, m_nWidth, m_nHeight,1);

		if(picsize != m_pFile[m_iCount] -> Read(m_pWnd[m_iCount]->Y,picsize))
		{
			MessageBox("Get to end of file");
			return;
		}
		if(1)//bColorImage) 
		{
			if(picsize/4 != m_pFile[m_iCount] -> Read(m_pWnd[m_iCount]->Cb,picsize/4))
			{
				MessageBox("Get to end of file");
				return;
			}
			if(picsize/4 != m_pFile[m_iCount] -> Read(m_pWnd[m_iCount]->Cr,picsize/4))
			{
				MessageBox("Get to end of file");
				return;
			}
		}

		m_pWnd[m_iCount]->ShowWindow(SW_SHOW);

		if(m_nZoom == -1) m_pWnd[m_iCount]->CenterWindow(m_nWidth,m_nHeight);
		else if(m_nZoom == 0) m_pWnd[m_iCount]->CenterWindow(m_nWidth*2,m_nHeight*2);

		m_tmpFile[m_tmpCnt++] = m_iCount;

		m_iCount++;
	}
}

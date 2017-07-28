
// MFCPlayerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MFCPlayer.h"
#include "MFCPlayerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMFCPlayerDlg dialog

#define THREADINIT(CLASS,CALLBACK)		\
	DWORD WINAPI Thread##CALLBACK(LPVOID param)	\
	{											\
		CLASS *thiz = (CLASS*)param;			\
		return thiz->CALLBACK();				\
	}											\

#define THREADNAME(CALLBACK)		Thread##CALLBACK
	
THREADINIT(CMFCPlayerDlg, ReadFrame)
THREADINIT(CMFCPlayerDlg, DecodeFrame)
THREADINIT(CMFCPlayerDlg, RenderFrame)

THREADINIT(CMFCPlayerDlg, ReadAudioFrame)
THREADINIT(CMFCPlayerDlg, DecodeAudioFrame)

int64_t GetTimeStamp(AVRational time_base, AVFrame * frame)
{
	int64_t timestamp = (frame->pts != AV_NOPTS_VALUE) ? (frame->pts * av_q2d(time_base) * 1000) :
		(frame->pkt_pts != AV_NOPTS_VALUE) ? (frame->pkt_pts * av_q2d(time_base) * 1000) : NAN;
	(frame->pkt_dts != AV_NOPTS_VALUE) ? (frame->pkt_dts * av_q2d(time_base) * 1000) : NAN;

	return timestamp;
}

void usleep(int64_t time)
{
	LARGE_INTEGER litmp;
	LONGLONG QPart1, QPart2;
	double dfMinus, dfFreq, dfTim;
	QueryPerformanceFrequency(&litmp);
	dfFreq = (double)litmp.QuadPart;// ��ü�������ʱ��Ƶ��
	QueryPerformanceCounter(&litmp);
	QPart1 = litmp.QuadPart;// ��ó�ʼֵ
	do {
		QueryPerformanceCounter(&litmp);
		QPart2 = litmp.QuadPart;//�����ֵֹ
		dfMinus = (double)(QPart2 - QPart1);
		dfTim = dfMinus / dfFreq;// ��ö�Ӧ��ʱ��ֵ����λΪ��
		if (dfTim * 1000 >= time)
		{
			break;
		}
	} while (true);
}

static void DrawPicture(CWnd * wnd, const BITMAPINFO bmi, const uint8_t* image)
{
	int height = abs(bmi.bmiHeader.biHeight);
	int width = bmi.bmiHeader.biWidth;
	if (image == NULL) return;

	CWnd * pPicture = wnd;
	if (pPicture == NULL) return;

	HDC hdc = ::GetDC(pPicture->GetSafeHwnd());
	if (hdc == NULL) return;

	RECT rc;
	pPicture->GetClientRect(&rc);

	{
		HDC dc_mem = ::CreateCompatibleDC(hdc);
		::SetStretchBltMode(dc_mem, HALFTONE);

		// Set the map mode so that the ratio will be maintained for us.
		HDC all_dc[] = { hdc, dc_mem };
		for (int i = 0; i < ARRAYSIZE(all_dc); ++i) {
			SetMapMode(all_dc[i], MM_ISOTROPIC);
			SetWindowExtEx(all_dc[i], width, height, NULL);
			SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
		}

		HBITMAP bmp_mem = ::CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
		HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

		POINT logical_area = { rc.right, rc.bottom };
		DPtoLP(hdc, &logical_area, 1);

		HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
		RECT logical_rect = { 0, 0, logical_area.x, logical_area.y };
		::FillRect(dc_mem, &logical_rect, brush);
		::DeleteObject(brush);

		int x = 0;
		int y = 0;

		StretchDIBits(dc_mem, x, y, logical_area.x, logical_area.y,
			0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);

		BitBlt(hdc, 0, 0, logical_area.x, logical_area.y,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
	::ReleaseDC(pPicture->GetSafeHwnd(), hdc);
}

CMFCPlayerDlg::CMFCPlayerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MFCPLAYER_DIALOG, pParent),
	queue(10), frame_queue(3), audio_queue(10)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMFCPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_CHANGEVALUE, OnChangeValue)
	ON_BN_CLICKED(IDC_BTN_OPEN, &CMFCPlayerDlg::OnBnClickedBtnOpen)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CMFCPlayerDlg::OnBnClickedBtnPlay)
	ON_BN_CLICKED(IDC_BTN_PAUSE, &CMFCPlayerDlg::OnBnClickedBtnPause)
	ON_BN_CLICKED(IDC_BTN_STOP, &CMFCPlayerDlg::OnBnClickedBtnStop)
	ON_NOTIFY(TRBN_THUMBPOSCHANGING, IDC_SLIDER_TIME, &CMFCPlayerDlg::OnTRBNThumbPosChangingSliderTime)
END_MESSAGE_MAP()


// CMFCPlayerDlg message handlers

BOOL CMFCPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	source_ = NULL;
	audiosource_ = NULL;
	videoDecode_ = NULL;
	audioDecode_ = NULL;

	hReadThread = NULL;
	hDecodeThread = NULL;
	hRenderThread = NULL;
	IsStarted = false;
	IsStop = false;
	IsPause = false;

	synchronise.SetFrameRate(30);
	NeedShowframe = NULL;

	audioPlay.initSDL();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCPlayerDlg::OnDestroy(void)
{
	OnBnClickedBtnStop();
	CDialogEx::OnDestroy();
}

void CMFCPlayerDlg::OnBnClickedBtnOpen()
{
	CFileDialog dlg(TRUE, NULL, NULL, 6, _T("*.mp4||*.mp4||"));
	if (dlg.DoModal() == IDOK)
	{
		CString path = dlg.GetPathName();
		EvoMediaSource *source = new EvoMediaSource();
		std::string file = P2AString(path.GetBuffer());
		int ret = source->Open(file.c_str());
		if (ret != 0)
		{
			delete source;
			source = NULL;
			AfxMessageBox(_T("���ļ�ʧ��"),MB_OK);
			return;
		}

		EvoMediaSource *audioSource = new EvoMediaSource();
		file = P2AString(path.GetBuffer());
		ret = audioSource->Open(file.c_str(),NULL,AVMEDIA_TYPE_AUDIO);
		if (ret != 0)
		{
			delete audioSource;
			audioSource = NULL;
		}

		OnBnClickedBtnStop();
		InitVideo(source);
		InitAudio(audioSource);
	}
}

void CMFCPlayerDlg::InitVideo(EvoMediaSource *source)
{
	if (source_ != NULL)
	{
		delete source_;
	}
	source_ = source;

	if (videoDecode_ != NULL)
	{
		delete videoDecode_;
	}
	AVStream * stream = source->GetVideoStream();
	AVCodec *codec = (AVCodec*)stream->codec->codec;
	if (codec == NULL) codec = avcodec_find_decoder(stream->codec->codec_id);
	if (avcodec_open2(stream->codec, codec, NULL) < 0)
	{
		return;
	}
	videoDecode_ = new VideoDecoder(stream->codec);

#ifndef USE_LIBYUV_CONVERT
	struct EvoVideoInfo info;
	info.Width = 0;
	info.Height = 0;
	info.Format = stream->codec->pix_fmt;

	struct EvoVideoInfo des = info;
	des.Format = AV_PIX_FMT_BGRA;
	convert_.Initialize(info, des);

	if (videoDecode_ != NULL)
	{
		videoDecode_->Attach(&convert_);
	}
#endif
}

void CMFCPlayerDlg::InitAudio(EvoMediaSource *source)
{
	if (audiosource_ != NULL)
	{
		delete audiosource_;
		audiosource_ = NULL;
	}
	audiosource_ = source;
	if (audiosource_ == NULL) return;

	AVStream * stream = source->GetVideoStream();
	AVCodec *codec = (AVCodec*)stream->codec->codec;
	if (codec == NULL) codec = avcodec_find_decoder(stream->codec->codec_id);
	if (avcodec_open2(stream->codec, codec, NULL) < 0)
	{
		return;
	}
	stream->time_base;
	audioDecode_ = new AudioDecoder(stream->codec);
	EvoAudioInfo info = {0};
	if (stream->codec->codec_id == AV_CODEC_ID_MP2 || stream->codec->codec_id == AV_CODEC_ID_MP3)
		info.nb_samples = 1152;
	else
		info.nb_samples = 1024;

	info.BitSize = stream->codec->bit_rate;
	info.Channels = stream->codec->channels;
	info.format = AV_SAMPLE_FMT_S16;// stream->codec->sample_fmt;
	info.SampleRate = stream->codec->sample_rate;
	audioDecode_->SetTargetInfo(info);

	audioPlay.AttachSync(&synchronise);
	audioPlay.closeAudio();
}

void CMFCPlayerDlg::OnBnClickedBtnPlay()
{
	if (source_ == NULL) return;
	if (videoDecode_ == NULL) return;
	Start();
	IsPause = false;
	audioPlay.playAudio();
}

void CMFCPlayerDlg::OnBnClickedBtnPause()
{
	IsPause = true;
	audioPlay.pauseAudio();
}

void CMFCPlayerDlg::Start()
{
	if (!IsStarted)
	{
		IsStop = false;
		IsStarted = true;
		hReadThread = CreateThread(NULL,0, THREADNAME(ReadFrame),this,0,NULL);
		hDecodeThread = CreateThread(NULL, 0, THREADNAME(DecodeFrame), this, 0, NULL);
		hRenderThread = CreateThread(NULL, 0, THREADNAME(RenderFrame), this, 0, NULL);
		if (audiosource_ != NULL)
		{
			hReadAudioThread = CreateThread(NULL, 0, THREADNAME(ReadAudioFrame), this, 0, NULL);
			hDecodeAudioThread = CreateThread(NULL, 0, THREADNAME(DecodeAudioFrame), this, 0, NULL);

			AVStream * stream = audiosource_->GetVideoStream();
			EvoAudioInfo info = { 0 };
			if (stream->codec->codec_id == AV_CODEC_ID_MP2 || stream->codec->codec_id == AV_CODEC_ID_MP3)
				info.nb_samples = 1152;
			else
				info.nb_samples = 1024;

			info.BitSize = stream->codec->bit_rate;
			info.Channels = stream->codec->channels;
			info.format = AV_SAMPLE_FMT_S16;// stream->codec->sample_fmt;
			info.SampleRate = stream->codec->sample_rate;

			audioPlay.closeAudio();
			audioPlay.openAudio(info.SampleRate, info.Channels, info.nb_samples);
		}
	}
}

void CMFCPlayerDlg::OnBnClickedBtnStop()
{
	IsStop = true;
	IsPause = false;
	//��������
	queue.Notify(true);
	frame_queue.Notify(true);
	audio_queue.Notify(true);
	audioPlay.GetQueue().Notify(true);
	audioPlay.closeAudio();
	//�����߳�
	if(hReadThread != NULL) WaitForSingleObject(hReadThread, INFINITE);
	if (hDecodeThread != NULL) WaitForSingleObject(hDecodeThread, INFINITE);
	if (hRenderThread != NULL) WaitForSingleObject(hRenderThread, INFINITE);
	if (hReadAudioThread != NULL) WaitForSingleObject(hReadAudioThread, INFINITE);
	if (hDecodeAudioThread != NULL) WaitForSingleObject(hDecodeAudioThread, INFINITE);
	if (hReadThread != NULL) CloseHandle(hReadThread);
	if (hDecodeThread != NULL) CloseHandle(hDecodeThread);
	if (hRenderThread != NULL) CloseHandle(hRenderThread);
	if (hReadAudioThread != NULL) CloseHandle(hReadAudioThread);
	if (hDecodeAudioThread != NULL) CloseHandle(hDecodeAudioThread);
	hReadThread = NULL;
	hDecodeThread = NULL;
	hRenderThread = NULL;
	hReadAudioThread = NULL;
	hDecodeAudioThread = NULL;
	IsStarted = false;
	//��������
	queue.Clear(true);
	frame_queue.Clear(true);
	audio_queue.Clear(true);
	audioPlay.GetQueue().Clear(true);
	//���û�����
	queue.Restart();
	frame_queue.Restart();
	audio_queue.Restart();
	audioPlay.GetQueue().Restart();
	//������
	if (NeedShowframe != NULL)
	{
		FreeAVFrame(&NeedShowframe);
		NeedShowframe = NULL;
	}
	//Դ�����ļ��ײ�
	if (source_ != 0)
	{
		source_->Seek(0);
	}
	//����ͬ����
	synchronise.Clear();
}

void CMFCPlayerDlg::OnTRBNThumbPosChangingSliderTime(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTRBTHUMBPOSCHANGING *pNMTPC = reinterpret_cast<NMTRBTHUMBPOSCHANGING *>(pNMHDR);
	
	*pResult = 0;
}

DWORD CMFCPlayerDlg::ReadFrame()
{
	if (source_ == NULL) return -1;
	EvoFrame *out = NULL;
	while (true)
	{
		if (IsStop) break;
		if (IsPause) {
			Sleep(1);
			continue;
		}
		int64_t timeBegin = av_gettime() / 1000;
		int ret = source_->ReadFrame(&out);
		if (ret == AVERROR_EOF)
		{
			break;
		}
		else if (out != NULL)
		{
			int64_t timeEnd = av_gettime() / 1000;
			SetValue(IDC_STATIC_READ, timeEnd - timeBegin);
			
			int ret = queue.Enqueue(out);
			if (ret == false)
			{
				EvoFreeFrame(&out);
			}
		}
	}
	return 0;
}

DWORD CMFCPlayerDlg::DecodeFrame()
{
	if (videoDecode_ == NULL)
	{
		return -1;
	}
	while (true)
	{
		if (IsStop) break;
		if (IsPause) {
			Sleep(1);
			continue;
		}

		EvoFrame * out = queue.Dequeue();
		if (out == NULL) continue;

		int64_t timeBegin = av_gettime() / 1000;
		AVFrame * evoResult = NULL;
		int ret = videoDecode_->DecodeFrame(out,&evoResult);
		if (evoResult != NULL)
		{
			int64_t timeEnd = av_gettime() / 1000;
			SetValue(IDC_STATIC_DECODE, timeEnd - timeBegin);

			int ret = frame_queue.Enqueue(evoResult);
			if (ret == false)
			{
				FreeAVFrame(&evoResult);
			}
		} 
		EvoFreeFrame(&out);
	}
	
	return 0;
}

DWORD CMFCPlayerDlg::RenderFrame()
{
	int64_t lastRenderTime = 0;
	int64_t totalRenderTime = 0;
	int renderCount = 0;
	int64_t timeBegin = av_gettime() / 1000;
	while (true)
	{
		if (IsStop) break;
		if (IsPause) {
			Sleep(1);
			continue;
		}
		AVFrame * renderFrame = NeedShowframe;
		NeedShowframe = NULL;
		if (renderFrame == NULL)
		{
			renderFrame = frame_queue.Dequeue();
		}
		if (renderFrame != NULL)
		{
			int64_t timestamp = GetTimeStamp(source_->GetVideoStream()->time_base, renderFrame);
			int ret = 0;// synchronise.CheckForVideoSynchronise(timestamp);
			if (ret < 0)
			{
				FreeAVFrame(&renderFrame);
				renderFrame = NULL;
			}
			else if (ret - lastRenderTime <= 0)
			{
				synchronise.ApplyVideoStamp(timestamp);
				int64_t timeRender = av_gettime() / 1000;
				Render(renderFrame);
				totalRenderTime += av_gettime() / 1000 - timeRender;
				renderCount++;
				lastRenderTime = totalRenderTime / renderCount;

				FreeAVFrame(&renderFrame);
				renderFrame = NULL;
				Rate++;

				int64_t timeEnd = av_gettime() / 1000;
				if (timeEnd - timeBegin >= 1000)
				{
					SetValue(IDC_STATIC_RATE, Rate);
					timeBegin = timeEnd;
					Rate = 0;
				}
			}
			else
			{
				usleep(ret);
				NeedShowframe = renderFrame;
			}
		}
	}
	if (NeedShowframe != NULL)
	{
		FreeAVFrame(&NeedShowframe);
		NeedShowframe = NULL;
	}
	return 0;
}

void CMFCPlayerDlg::SetBITMAPSize(int width, int height)
{
	if (width == bmi_.bmiHeader.biWidth && height == abs(bmi_.bmiHeader.biHeight)) {
		return;
	}
	memset(&bmi_, 0, sizeof(bmi_));
	bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi_.bmiHeader.biPlanes = 1;
	bmi_.bmiHeader.biBitCount = 32;
	bmi_.bmiHeader.biCompression = BI_RGB;
	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
}

void CMFCPlayerDlg::Render(AVFrame * frame)
{
	SetBITMAPSize(frame->width,frame->height);
	CWnd * picture = GetDlgItem(IDC_STATIC_PIC);
#ifdef USE_LIBYUV_CONVERT
	int len = frame->width * frame->height * 4;
	uint8_t * argb = (uint8_t*)malloc(len);
	int32_t yuvSize = frame->width * frame->height * 3 / 2;
	if (argb != NULL)
	{

		DWORD timeBegin = av_gettime() / 1000;

		libyuv::ConvertToARGB(frame->data[0], yuvSize, argb, frame->width * 4, 0, 0, frame->width, frame->height, frame->width, frame->height,
			libyuv::RotationMode::kRotateNone, libyuv::CanonicalFourCC(libyuv::FOURCC_I420));

		DWORD timeEnd = av_gettime() / 1000;
		SetValue(IDC_STATIC_CONVERT, timeEnd - timeBegin);

		DrawPicture(picture, bmi_, argb);
		free(argb);
	}
#else
	int64_t timeBegin = av_gettime() / 1000;

	DrawPicture(picture,bmi_,frame->data[0]);

	int64_t timeEnd = av_gettime() / 1000;
	SetValue(IDC_STATIC_CONVERT, timeEnd - timeBegin);
#endif
}

DWORD CMFCPlayerDlg::ReadAudioFrame()
{
	if (audiosource_ == NULL) return -1;
	EvoFrame *out = NULL;
	while (true)
	{
		if (IsStop) break;
		if (IsPause) {
			Sleep(1);
			continue;
		}
		int64_t timeBegin = av_gettime() / 1000;
		int ret = audiosource_->ReadFrame(&out);
		if (ret == AVERROR_EOF)
		{
			break;
		}
		else if (out != NULL)
		{
			int64_t timeEnd = av_gettime() / 1000;
			SetValue(IDC_STATIC_AUDIO_READ, timeEnd - timeBegin);

			int ret = audio_queue.Enqueue(out);
			if (ret == false)
			{
				EvoFreeFrame(&out);
			}
		}
	}
	return 0;
}

DWORD CMFCPlayerDlg::DecodeAudioFrame()
{
	if (audioDecode_ == NULL)
	{
		return -1;
	}
	while (true)
	{
		if (IsStop) break;
		if (IsPause) {
			Sleep(1);
			continue;
		}

		EvoFrame * out = audio_queue.Dequeue();
		if (out == NULL) continue;

		int64_t timeBegin = av_gettime() / 1000;
		AVFrame * evoResult = NULL;
		int ret = audioDecode_->DecodeFrame(out, &evoResult);
		if (evoResult != NULL)
		{
			int64_t timeEnd = av_gettime() / 1000;
			
			SetValue(IDC_STATIC_AUDIO_DECODE, timeEnd - timeBegin);

			int64_t timeStamp = GetTimeStamp(audiosource_->GetVideoStream()->time_base, evoResult);
			evoResult->pts = timeStamp;
			int ret = audioPlay.GetQueue().Enqueue(evoResult);
			if (ret == false)
			{
				FreeAVFrame(&evoResult);
			}
		}
		EvoFreeFrame(&out);
	}

	return 0;
}

void CMFCPlayerDlg::SetValue(int id,int64_t value)
{
	PostMessage(WM_CHANGEVALUE,id,value);
}

LRESULT CMFCPlayerDlg::OnChangeValue(WPARAM wParam, LPARAM lParam)
{
	int id = (int)wParam;
	int64_t value = lParam;
	CString strValue;
	strValue.Format(_T("%lld"), value);
	GetDlgItem(id)->SetWindowText(strValue);

	return 0;
}
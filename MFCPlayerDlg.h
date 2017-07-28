
// MFCPlayerDlg.h : header file
//

#pragma once
#include "EvoInterface\EvoMediaSource.h"
#include "EvoInterface\VideoDecoder.h"
#include "EvoInterface\EvoVideoConvert.h"
#include "EvoInterface\EvoQueue.hpp"
#include "MediaSynchronise.h"
#include "EvoInterface\AudioDecoder.h"
#include "AudioPlay.h"
#include "EvoMesiaSourceLock.h"
#include "CodeTransport.h"
#include "libyuv.h"
#ifdef _WIN32

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")

#pragma comment(lib,"EvoInterface.lib")

#pragma comment(lib,"libyuv.lib")
#pragma comment(lib,"jpeg.lib")
#pragma comment(lib,"SDL2.lib")
#endif

//#define USE_LIBYUV_CONVERT

#define WM_CHANGEVALUE WM_USER + 1
#define WM_FLUSHSTAMP WM_USER + 2

// CMFCPlayerDlg dialog
class CMFCPlayerDlg : public CDialogEx
{
// Construction
public:
	CMFCPlayerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCPLAYER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
public:
	DWORD ReadFrame();
	DWORD DecodeFrame();
	DWORD RenderFrame();

	DWORD ReadAudioFrame();
	DWORD DecodeAudioFrame();
private:
	int InitVideo(EvoMediaSource *source);
	int InitAudio(EvoMediaSource *source);
	void Start();
	void Stop();

	void Render(AVFrame * frame);
	void SetBITMAPSize(int width, int height);

	void SetValue(int id, int64_t value);
	void ChangePos();
private:
	HANDLE hReadThread;
	HANDLE hDecodeThread;
	HANDLE hRenderThread;
	HANDLE hReadAudioThread;
	HANDLE hDecodeAudioThread;
	bool IsStop;
	bool IsPause;
	bool IsStarted;

	EvoMediaSource * source_;
	VideoDecoder *videoDecode_;
	EvoVideoConvert convert_;

	EvoMediaSource * audiosource_;
	AudioDecoder *audioDecode_;

	EvoQueue<EvoFrame, EvoFreeFrame> queue;
	EvoQueue<AVFrame, FreeAVFrame> frame_queue;
	EvoQueue<EvoFrame, EvoFreeFrame> audio_queue;
	AudioPlayer audioPlay;

	AVFrame * NeedShowframe;
	MediaSynchronise synchronise;
	BITMAPINFO bmi_;
	int Rate;
	int  CurrentPos;
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnHScroll(UINT, UINT, CScrollBar*);
	afx_msg void OnTimer(UINT_PTR);
	afx_msg LRESULT OnChangeValue(WPARAM wParam,LPARAM lParam);
	afx_msg void OnDestroy(void);
	afx_msg void OnBnClickedBtnOpen();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnBnClickedBtnPause();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnTRBNThumbPosChangingSliderTime(NMHDR *pNMHDR, LRESULT *pResult);
};

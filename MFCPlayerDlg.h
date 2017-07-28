
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
	void Render(AVFrame * frame);
	void CMFCPlayerDlg::SetBITMAPSize(int width, int height);

	DWORD ReadAudioFrame();
	DWORD DecodeAudioFrame();
private:
	void InitVideo(EvoMediaSource *source);
	void InitAudio(EvoMediaSource *source);
	void Start();
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
	afx_msg void OnDestroy(void);
	afx_msg void OnBnClickedBtnOpen();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnBnClickedBtnPause();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnTRBNThumbPosChangingSliderTime(NMHDR *pNMHDR, LRESULT *pResult);
};
